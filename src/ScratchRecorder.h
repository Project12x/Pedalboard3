#pragma once

#include "ScratchTake.h"

#include <JuceHeader.h>
#include <atomic>
#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

enum class ScratchRecorderState
{
    Ready,
    Preparing,
    Recording,
    Saving,
    Saved,
    Failed
};

struct ScratchRecorderStatus
{
    ScratchRecorderState state = ScratchRecorderState::Ready;
    juce::String message = "Ready";
    uint64_t elapsedSamples = 0;
    uint64_t rawSamplesWritten = 0;
    uint64_t wetSamplesWritten = 0;
    uint64_t droppedBlocksAfterStop = 0;
    uint64_t writerFailureCount = 0;
    uint64_t stopDrainTimeoutCount = 0;
    int maxActiveAudioWrites = 0;
    juce::File scratchRoot;
    std::optional<ScratchTakeContext> armedContext;
    std::optional<ScratchTake> activeTake;
    std::optional<ScratchTake> lastTake;
    std::vector<ScratchTake> recentTakes;
};

class ScratchAudioSink
{
public:
    virtual ~ScratchAudioSink() = default;

    virtual bool open(const juce::File& file, double sampleRate, int channels, juce::TimeSliceThread& thread) = 0;
    virtual bool write(const float* const* data, int numChannels, int numSamples) noexcept = 0;
    virtual void close() = 0;
    virtual uint64_t getSamplesWritten() const noexcept = 0;
};

class ScratchAudioSinkFactory
{
public:
    virtual ~ScratchAudioSinkFactory() = default;

    virtual std::unique_ptr<ScratchAudioSink> create() = 0;
};

class ThreadedWavSinkFactory final : public ScratchAudioSinkFactory
{
public:
    std::unique_ptr<ScratchAudioSink> create() override;
};

class ScratchRecorder final : private juce::AsyncUpdater
{
public:
    explicit ScratchRecorder(ScratchAudioSinkFactory& factory);
    ~ScratchRecorder() override;

    bool start(const ScratchTakeContext& context);
    juce::Result prepareTake(const ScratchTakeContext&);
    juce::Result startPreparedTake();
    void cancelPreparedTake(const juce::String& reason);
    void setPreparedSnapshotResult(ScratchSnapshotState, const juce::String&);
    // Control-thread completion gate: stale workers cannot start a newer take.
    juce::Result completePreparedSnapshot(uint64_t requestId, ScratchSnapshotState, const juce::String&);
    uint64_t getPreparationId() const noexcept { return preparationId; }
    void requestStop();
    void stopForDeviceChange();
    void stopForPatchChange();

    void writeRawBlock(const float* const* inputChannelData, int numInputChannels, int numSamples) noexcept;
    void writeWetBlock(float* const* outputChannelData, int numOutputChannels, int numSamples) noexcept;

    // A successful begin requires exactly one end on the same device callback.
    bool beginDeviceBlock(const float* const*, int channels, int samples) noexcept;
    void endDeviceBlock(float* const*, int channels, int samples) noexcept;

    ScratchRecorderStatus getStatus() const;
    bool isRecording() const noexcept;
    static juce::File getDefaultScratchRoot();
    juce::File getScratchRoot() const;
    void setScratchRoot(const juce::File& root);
    void resetScratchRootToDefault();

    void finishPendingStopForTests();

    // Message-thread finalization notification; invoked after releasing stateLock.
    std::function<void(const ScratchTake&)> onTakeFinalized;

private:
    void handleAsyncUpdate() override;
    void finishStop();
    void stopWithFailureReason(const juce::String& reason);
    void failStart(const juce::String& message);
    bool beginAudioWrite() noexcept;
    void endAudioWrite() noexcept;
    void updateMaxActiveAudioWrites(int activeWrites) noexcept;
    void addRecentTake(const ScratchTake& take);

    ScratchAudioSinkFactory& sinkFactory;
    juce::TimeSliceThread writerThread{"Scratch Recorder Writer"};
    mutable juce::CriticalSection stateLock;
    std::unique_ptr<ScratchAudioSink> rawSink;
    std::unique_ptr<ScratchAudioSink> wetSink;
    ScratchTake currentTake;
    uint64_t preparationId = 0; // message-thread preparation lifecycle only
    ScratchRecorderStatus status;
    std::vector<ScratchTake> recentTakes;
    juce::File scratchRoot;
    std::atomic<int> state{static_cast<int>(ScratchRecorderState::Ready)};
    std::atomic<uint64_t> rawSamplesWritten{0};
    std::atomic<uint64_t> wetSamplesWritten{0};
    std::atomic<bool> stopRequested{false};
    std::atomic<bool> writeError{false};
    std::atomic<int> activeAudioWrites{0};
    std::atomic<uint64_t> droppedBlocksAfterStop{0};
    std::atomic<uint64_t> writerFailureCount{0};
    std::atomic<uint64_t> stopDrainTimeoutCount{0};
    std::atomic<int> maxActiveAudioWrites{0};
};
