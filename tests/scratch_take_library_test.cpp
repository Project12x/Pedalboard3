#include "ScratchTakeLibrary.h"
#include "ScratchTestFiles.h"
#include <nlohmann/json.hpp>
#include <thread>
#include <filesystem>

namespace
{
bool scanFinished(ScratchTakeLibrary& library)
{
    const auto deadline = juce::Time::getMillisecondCounter() + 5000;
    while (library.snapshot()->scanning && juce::Time::getMillisecondCounter() < deadline)
        juce::Thread::sleep(1);
    return !library.snapshot()->scanning;
}

void makeLibrary(ScratchTestFiles& files, int count)
{
    for (int i = 0; i < count; ++i)
    {
        auto take = files.makeTake();
        take.takeId = "same-id"; // IDs are not directory identity.
        take.startTime = juce::Time(1700000000000LL + i * 1000);
        REQUIRE(take.writeMetadata());
    }
}
}

TEST_CASE("Scratch library survives restart without an eight take limit or duplicate ID merging", "[scratch][scratch-library]")
{
    ScratchTestFiles files;
    makeLibrary(files, 12);
    std::vector<juce::File> first;
    for (int run = 0; run < 2; ++run)
    {
        ScratchTakeLibrary library;
        library.setRoot(files.root);
        REQUIRE(scanFinished(library));
        const auto result = library.snapshot();
        REQUIRE(result->entries.size() == 12);
        REQUIRE(result->entries[0].take.startTime > result->entries[11].take.startTime);
        for (size_t i = 0; i < result->entries.size(); ++i)
        {
            REQUIRE(result->entries[i].take.takeId == "same-id");
            if (run == 0) first.push_back(result->entries[i].take.takeDirectory);
            else REQUIRE(first[i] == result->entries[i].take.takeDirectory);
        }
    }
}

TEST_CASE("Scratch library leaves corrupt neighbors and metadata untouched", "[scratch][scratch-library]")
{
    ScratchTestFiles files;
    auto good = files.makeTake();
    REQUIRE(good.writeMetadata());
    auto bad = files.makeTake();
    REQUIRE(bad.metadataFile.replaceWithText("broken"));
    const auto original = good.metadataFile.loadFileAsString();
    ScratchTakeLibrary library;
    library.setRoot(files.root);
    REQUIRE(scanFinished(library));
    const auto result = library.snapshot();
    REQUIRE(result->entries.size() == 2);
    int invalid = 0;
    for (const auto& entry : result->entries)
        invalid += entry.state == ScratchTakeLoadState::Invalid ? 1 : 0;
    REQUIRE(invalid == 1);
    REQUIRE(good.metadataFile.loadFileAsString() == original);
    REQUIRE(bad.metadataFile.loadFileAsString() == "broken");
}

TEST_CASE("Scratch library rejects obsolete root results", "[scratch][scratch-library]")
{
    ScratchTestFiles a, b;
    makeLibrary(a, 1);
    makeLibrary(b, 12);
    juce::WaitableEvent entered, release;
    const auto caller = std::this_thread::get_id();
    std::atomic<bool> workerThread{false};
    ScratchTakeLibrary library([&](const juce::File& directory) {
        workerThread.store(std::this_thread::get_id() != caller);
        if (directory.isAChildOf(a.root))
        {
            entered.signal();
            release.wait(5000);
        }
        return loadScratchTake(directory);
    });
    library.setRoot(a.root);
    const bool didEnter = entered.wait(5000);
    library.setRoot(b.root);
    const auto pending = library.snapshot();
    release.signal(); // release before REQUIRE so teardown never waits on a failed assertion
    REQUIRE(didEnter);
    REQUIRE(pending->root == b.root);
    REQUIRE(pending->entries.empty());
    REQUIRE(scanFinished(library));
    const auto result = library.snapshot();
    REQUIRE(result->root == b.root);
    REQUIRE(result->entries.size() == 12);
    REQUIRE_FALSE(result->scanning);
    REQUIRE(workerThread.load());
}

TEST_CASE("Scratch library refresh keeps a take finalized during a scan", "[scratch][scratch-library]")
{
    ScratchTestFiles files;
    auto take = files.makeTake();
    REQUIRE(take.writeMetadata());
    juce::WaitableEvent entered, release;
    std::atomic<bool> first{true};
    ScratchTakeLibrary library([&](const juce::File& directory) {
        auto loaded = loadScratchTake(directory);
        if (first.exchange(false)) { entered.signal(); release.wait(5000); }
        return loaded;
    });
    library.setRoot(files.root);
    const bool didEnter = entered.wait(5000);
    auto newest = files.makeTake();
    REQUIRE(newest.writeMetadata());
    library.upsertFinalizedTake(newest);
    release.signal();
    REQUIRE(didEnter);
    REQUIRE(scanFinished(library));
    const auto result = library.snapshot();
    REQUIRE(result->entries.size() == 2);
}

TEST_CASE("Scratch library fixed-depth scanning is read-only and deterministic", "[scratch][scratch-library]")
{
    ScratchTestFiles files;
    auto first = files.makeTake();
    auto second = files.makeTake();
    first.startTime = second.startTime = juce::Time(1700000000000LL);
    REQUIRE(first.writeMetadata());
    REQUIRE(second.writeMetadata());
    const auto nested = first.takeDirectory.getChildFile("unexpected/nested");
    REQUIRE(nested.createDirectory().wasOk());
    REQUIRE(nested.getChildFile("take.json").replaceWithText(first.toJsonString()));
    ScratchTestFiles::wav(first.rawFile);
    juce::MemoryBlock before, after;
    REQUIRE(first.rawFile.loadFileAsData(before));
    ScratchTakeLibrary library;
    library.setRoot(files.root);
    REQUIRE(scanFinished(library));
    const auto initial = library.snapshot();
    REQUIRE(initial->entries.size() == 2);
    REQUIRE(initial->entries[0].take.takeDirectory < initial->entries[1].take.takeDirectory);
    library.refresh();
    REQUIRE(scanFinished(library));
    REQUIRE(library.snapshot()->entries[0].take.takeDirectory == initial->entries[0].take.takeDirectory);
    REQUIRE(first.rawFile.loadFileAsData(after));
    REQUIRE(before == after);
    const auto absent = files.root.getChildFile("not-created");
    library.setRoot(absent);
    REQUIRE(scanFinished(library));
    REQUIRE_FALSE(absent.exists());
    REQUIRE(library.snapshot()->message.isNotEmpty());
    REQUIRE(library.snapshot()->entries.empty());
}

TEST_CASE("Scratch library excludes linked day directories", "[scratch][scratch-library]")
{
    ScratchTestFiles files, external;
    auto take = external.makeTake();
    REQUIRE(take.writeMetadata());
    const auto link = files.root.getChildFile("linked-day");
    std::error_code error;
    std::filesystem::create_directory_symlink(
        std::filesystem::u8path(take.takeDirectory.getParentDirectory().getFullPathName().toStdString()),
        std::filesystem::u8path(link.getFullPathName().toStdString()), error);
    if (error) SKIP("Host does not permit symlink creation: " << error.message());
    ScratchTakeLibrary library;
    library.setRoot(files.root);
    REQUIRE(scanFinished(library));
    REQUIRE(library.snapshot()->entries.empty());
    REQUIRE(take.metadataFile.existsAsFile());
}
