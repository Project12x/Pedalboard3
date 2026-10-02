#pragma once
#include "ScratchTake.h"
#include <atomic>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

struct ScratchLibrarySnapshot
{
    juce::File root;
    uint64_t generation = 0;
    bool scanning = false;
    std::vector<ScratchTakeLoadResult> entries;
    juce::String message;
};

// Disk work belongs to one owned worker. Snapshots/commands are control-thread
// APIs, never called by the audio callback. The optional loader is a test seam.
class ScratchTakeLibrary final
{
public:
    using Loader = std::function<ScratchTakeLoadResult(const juce::File&)>;
    explicit ScratchTakeLibrary(Loader loader = loadScratchTake);
    ~ScratchTakeLibrary();
    void setRoot(const juce::File&);
    void refresh();
    void upsertFinalizedTake(const ScratchTake&);
    std::shared_ptr<const ScratchLibrarySnapshot> snapshot() const;

private:
    void requestScan(const juce::File&, bool clearEntries);
    void run();
    bool current(uint64_t) const noexcept;
    void publish(uint64_t, const juce::File&, const std::vector<ScratchTakeLoadResult>&,
                 bool scanning, const juce::String& message);
    Loader loadTake;
    mutable std::mutex mutex;
    std::condition_variable wake;
    std::shared_ptr<const ScratchLibrarySnapshot> published = std::make_shared<ScratchLibrarySnapshot>();
    juce::File root;
    std::atomic<uint64_t> requested{0};
    std::atomic<bool> quitting{false};
    std::thread worker;
};
