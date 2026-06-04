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
    void requestStop();
    void stopForDeviceChange();
    void stopForPatchChange();

    void writeRawBlock(const float* const* inputChannelData, int numInputChannels, int numSamples) noexcept;
    void writeWetBlock(float* const* outputChannelData, int numOutputChannels, int numSamples) noexcept;

    ScratchRecorderStatus getStatus() const;
    bool isRecording() const noexcept;
    juce::File getScratchRoot() const;
    void setScratchRoot(const juce::File& root);

    void finishPendingStopForTests();

private:
    void handleAsyncUpdate() override;
    void finishStop();
    void stopWithFailureReason(const juce::String& reason);
    void failStart(const juce::String& message);
    bool beginAudioWrite() noexcept;
    void endAudioWrite() noexcept;
    void addRecentTake(const ScratchTake& take);

    ScratchAudioSinkFactory& sinkFactory;
    juce::TimeSliceThread writerThread{"Scratch Recorder Writer"};
    mutable juce::CriticalSection stateLock;
    std::unique_ptr<ScratchAudioSink> rawSink;
    std::unique_ptr<ScratchAudioSink> wetSink;
    ScratchTake currentTake;
    ScratchRecorderStatus status;
    std::vector<ScratchTake> recentTakes;
    juce::File scratchRoot;
    std::atomic<int> state{static_cast<int>(ScratchRecorderState::Ready)};
    std::atomic<uint64_t> rawSamplesWritten{0};
    std::atomic<uint64_t> wetSamplesWritten{0};
    std::atomic<bool> stopRequested{false};
    std::atomic<bool> writeError{false};
    std::atomic<int> activeAudioWrites{0};
};
