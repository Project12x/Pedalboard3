#include "ScratchTakeLibrary.h"
#include <algorithm>
#include <filesystem>

namespace
{
std::filesystem::path pathFor(const juce::File& file)
{
    return std::filesystem::u8path(file.getFullPathName().toStdString());
}

juce::File canonical(const juce::File& file)
{
    if (file == juce::File()) return {};
    std::error_code error;
    const auto path = std::filesystem::weakly_canonical(pathFor(file), error);
    return error ? file : juce::File(juce::String(path.u8string()));
}

void sortEntries(std::vector<ScratchTakeLoadResult>& entries)
{
    std::sort(entries.begin(), entries.end(), [](const auto& a, const auto& b) {
        if (a.take.startTime != b.take.startTime) return a.take.startTime > b.take.startTime;
        return a.take.takeDirectory < b.take.takeDirectory;
    });
}
}

ScratchTakeLibrary::ScratchTakeLibrary(Loader loader)
    : loadTake(std::move(loader)), worker([this] { run(); }) {}

ScratchTakeLibrary::~ScratchTakeLibrary()
{
    {
        const std::lock_guard<std::mutex> lock(mutex);
        quitting.store(true, std::memory_order_release);
    }
    wake.notify_one();
    if (worker.joinable()) worker.join(); // owner/control thread, never audio
}

void ScratchTakeLibrary::setRoot(const juce::File& directory)
{
    requestScan(canonical(directory), true);
}

void ScratchTakeLibrary::refresh()
{
    const auto state = snapshot();
    requestScan(state->root, false);
}

void ScratchTakeLibrary::upsertFinalizedTake(const ScratchTake& take)
{
    const auto state = snapshot();
    if (canonical(take.takeDirectory.getParentDirectory().getParentDirectory()) == state->root)
        refresh(); // new generation invalidates scans that started before finalization
}

std::shared_ptr<const ScratchLibrarySnapshot> ScratchTakeLibrary::snapshot() const
{
    const std::lock_guard<std::mutex> lock(mutex);
    return published;
}

void ScratchTakeLibrary::requestScan(const juce::File& directory, bool clearEntries)
{
    {
        const std::lock_guard<std::mutex> lock(mutex);
        root = directory;
        auto next = std::make_shared<ScratchLibrarySnapshot>();
        if (!clearEntries && published->root == directory) next->entries = published->entries;
        next->root = directory;
        next->generation = requested.fetch_add(1, std::memory_order_acq_rel) + 1;
        next->scanning = true;
        next->message = "Scanning takes...";
        published = std::move(next);
    }
    wake.notify_one();
}

bool ScratchTakeLibrary::current(uint64_t generation) const noexcept
{
    return !quitting.load(std::memory_order_acquire) && requested.load(std::memory_order_acquire) == generation;
}

void ScratchTakeLibrary::publish(uint64_t generation, const juce::File& directory,
                                 const std::vector<ScratchTakeLoadResult>& entries,
                                 bool scanning, const juce::String& message)
{
    auto next = std::make_shared<ScratchLibrarySnapshot>();
    next->root = directory;
    next->generation = generation;
    next->scanning = scanning;
    next->entries = entries;
    sortEntries(next->entries);
    next->message = message;
    const std::lock_guard<std::mutex> lock(mutex);
    if (current(generation)) published = std::move(next);
}

void ScratchTakeLibrary::run()
{
    uint64_t finished = 0;
    for (;;)
    {
        juce::File directory;
        uint64_t generation = 0;
        {
            std::unique_lock<std::mutex> lock(mutex);
            wake.wait(lock, [&] { return quitting.load(std::memory_order_acquire) || requested.load() != finished; });
            if (quitting.load(std::memory_order_acquire)) return;
            directory = root;
            generation = requested.load(std::memory_order_acquire);
        }

        std::vector<ScratchTakeLoadResult> entries;
        juce::String message;
        if (directory == juce::File() || !directory.isDirectory())
            message = "Scratch folder is missing or unavailable";
        else
        {
            // Exactly root/day/take. Never recurse into arbitrary subtrees or links.
            std::error_code dayError;
            std::filesystem::directory_iterator day(pathFor(directory), dayError), end;
            for (; !dayError && day != end && current(generation); day.increment(dayError))
            {
                const juce::File dayFile(juce::String(day->path().u8string()));
                if (dayFile.isSymbolicLink() || !dayFile.isDirectory()) continue;
                std::error_code takeError;
                std::filesystem::directory_iterator take(pathFor(dayFile), takeError);
                for (; !takeError && take != end && current(generation); take.increment(takeError))
                {
                    const juce::File takeFile(juce::String(take->path().u8string()));
                    if (takeFile.isSymbolicLink() || !takeFile.isDirectory()) continue;
                    try
                    {
                        auto loaded = loadTake(takeFile);
                        if (!current(generation)) break;
                        entries.push_back(std::move(loaded));
                    }
                    catch (const std::exception& exception)
                    {
                        ScratchTakeLoadResult invalid;
                        invalid.take.takeDirectory = takeFile;
                        invalid.take.patchName = takeFile.getFileName();
                        invalid.message = "Could not inspect take: " + juce::String(exception.what());
                        entries.push_back(std::move(invalid));
                    }
                    if (entries.size() % 128 == 0)
                        publish(generation, directory, entries, true, "Scanning takes...");
                }
                if (takeError) message = "Some take folders could not be read";
            }
            if (dayError) message = "Scratch folder could not be read: " + juce::String(dayError.message());
        }
        publish(generation, directory, entries, false, message);
        finished = generation;
    }
}
