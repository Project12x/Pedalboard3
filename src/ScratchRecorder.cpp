#include "ScratchRecorder.h"

#include <algorithm>

namespace
{
constexpr int kWavBitDepth = 24;
constexpr int kThreadedWriterBufferSamples = 32768;
constexpr int kWriterThreadStopTimeoutMs = 1000;
constexpr size_t kMaxRecentTakes = 8;

ScratchRecorderState loadState(const std::atomic<int>& state) noexcept
{
    return static_cast<ScratchRecorderState>(state.load(std::memory_order_acquire));
}

uint64_t elapsedFromCounts(uint64_t rawSamples, uint64_t wetSamples) noexcept
{
    return std::min(rawSamples, wetSamples);
}

class ThreadedWavSink final : public ScratchAudioSink
{
public:
    bool open(const juce::File& file, double sampleRate, int channels, juce::TimeSliceThread& thread) override
    {
        close();

        if (channels <= 0 || sampleRate <= 0.0)
            return false;

        auto fileStream = std::make_unique<juce::FileOutputStream>(file);
        if (!fileStream->openedOk())
            return false;

        std::unique_ptr<juce::OutputStream> stream(std::move(fileStream));
        juce::WavAudioFormat wavFormat;
        const auto options = juce::AudioFormatWriter::Options{}
                                 .withSampleRate(sampleRate)
                                 .withNumChannels(channels)
                                 .withBitsPerSample(kWavBitDepth);
        auto writer = wavFormat.createWriterFor(stream, options);

        if (writer == nullptr)
            return false;

        threadedWriter = std::make_unique<juce::AudioFormatWriter::ThreadedWriter>(
            writer.release(), thread, kThreadedWriterBufferSamples);
        channelCount = channels;
        samplesWritten.store(0, std::memory_order_relaxed);
        return true;
    }

    bool write(const float* const* data, int numChannels, int numSamples) noexcept override
    {
        if (threadedWriter == nullptr || data == nullptr || numChannels < channelCount || numSamples <= 0)
            return false;

        for (int channel = 0; channel < channelCount; ++channel)
            if (data[channel] == nullptr)
                return false;

        const auto ok = threadedWriter->write(data, numSamples);
        if (ok)
            samplesWritten.fetch_add(static_cast<uint64_t>(numSamples), std::memory_order_relaxed);

        return ok;
    }

    void close() override
    {
        threadedWriter.reset();
        channelCount = 0;
    }

    uint64_t getSamplesWritten() const noexcept override
    {
        return samplesWritten.load(std::memory_order_relaxed);
    }

private:
    std::unique_ptr<juce::AudioFormatWriter::ThreadedWriter> threadedWriter;
    int channelCount = 0;
    std::atomic<uint64_t> samplesWritten{0};
};
}

std::unique_ptr<ScratchAudioSink> ThreadedWavSinkFactory::create()
{
    return std::make_unique<ThreadedWavSink>();
}

ScratchRecorder::ScratchRecorder(ScratchAudioSinkFactory& factory)
    : sinkFactory(factory), scratchRoot(getDefaultScratchRoot())
{
    status.scratchRoot = scratchRoot;
    writerThread.startThread();
}

ScratchRecorder::~ScratchRecorder()
{
    if (isRecording())
        requestStop();

    finishStop();
    cancelPendingUpdate();
    writerThread.stopThread(kWriterThreadStopTimeoutMs);
}

bool ScratchRecorder::start(const ScratchTakeContext& context)
{
    const juce::ScopedLock lock(stateLock);

    if (loadState(state) == ScratchRecorderState::Recording || loadState(state) == ScratchRecorderState::Saving)
        return false;

    if (context.rawChannelCount <= 0)
    {
        failStart("No input channels available");
        return false;
    }

    if (context.wetChannelCount <= 0)
    {
        failStart("No output channels available");
        return false;
    }

    ScratchTakeContext resolved = context;
    if (resolved.rootDirectory == juce::File())
        resolved.rootDirectory = scratchRoot;

    currentTake = ScratchTake::createPending(resolved);
    if (!currentTake.failureReason.isEmpty())
    {
        currentTake.writeMetadata();
        addRecentTake(currentTake);
        status.state = ScratchRecorderState::Failed;
        status.message = currentTake.failureReason;
        status.elapsedSamples = 0;
        status.rawSamplesWritten = 0;
        status.wetSamplesWritten = 0;
        status.scratchRoot = scratchRoot;
        status.activeTake.reset();
        status.lastTake = currentTake;
        status.recentTakes = recentTakes;
        state.store(static_cast<int>(ScratchRecorderState::Failed), std::memory_order_release);
        return false;
    }

    rawSink = sinkFactory.create();
    wetSink = sinkFactory.create();

    if (rawSink == nullptr || wetSink == nullptr ||
        !rawSink->open(currentTake.rawFile, currentTake.sampleRate, currentTake.rawChannelCount, writerThread) ||
        !wetSink->open(currentTake.wetFile, currentTake.sampleRate, currentTake.wetChannelCount, writerThread))
    {
        if (rawSink != nullptr)
            rawSink->close();
        if (wetSink != nullptr)
            wetSink->close();

        rawSink.reset();
        wetSink.reset();

        currentTake.failureReason = "Could not create scratch WAV writers";
        currentTake.writeMetadata();
        addRecentTake(currentTake);

        status.state = ScratchRecorderState::Failed;
        status.message = currentTake.failureReason;
        status.elapsedSamples = 0;
        status.rawSamplesWritten = 0;
        status.wetSamplesWritten = 0;
        status.scratchRoot = scratchRoot;
        status.activeTake.reset();
        status.lastTake = currentTake;
        status.recentTakes = recentTakes;
        state.store(static_cast<int>(ScratchRecorderState::Failed), std::memory_order_release);
        return false;
    }

    rawSamplesWritten.store(0, std::memory_order_relaxed);
    wetSamplesWritten.store(0, std::memory_order_relaxed);
    stopRequested.store(false, std::memory_order_release);
    writeError.store(false, std::memory_order_release);
    activeAudioWrites.store(0, std::memory_order_release);

    status.state = ScratchRecorderState::Recording;
    status.message = "Recording";
    status.elapsedSamples = 0;
    status.rawSamplesWritten = 0;
    status.wetSamplesWritten = 0;
    status.scratchRoot = scratchRoot;
    status.activeTake = currentTake;
    status.lastTake.reset();
    status.recentTakes = recentTakes;

    state.store(static_cast<int>(ScratchRecorderState::Recording), std::memory_order_release);
    return true;
}

void ScratchRecorder::requestStop()
{
    if (!isRecording())
        return;

    stopRequested.store(true, std::memory_order_release);
    triggerAsyncUpdate();
}

void ScratchRecorder::stopForDeviceChange()
{
    stopWithFailureReason("Audio device changed during scratch capture");
}

void ScratchRecorder::stopForPatchChange()
{
    stopWithFailureReason("Patch changed during scratch capture");
}

void ScratchRecorder::stopWithFailureReason(const juce::String& reason)
{
    if (!isRecording())
        return;

    {
        const juce::ScopedLock lock(stateLock);
        if (currentTake.failureReason.isEmpty())
            currentTake.failureReason = reason;
    }

    requestStop();
}

void ScratchRecorder::writeRawBlock(const float* const* inputChannelData, int numInputChannels, int numSamples) noexcept
{
    if (!beginAudioWrite())
        return;

    auto* sink = rawSink.get();
    if (sink == nullptr || !sink->write(inputChannelData, numInputChannels, numSamples))
    {
        writeError.store(true, std::memory_order_release);
    }
    else
    {
        rawSamplesWritten.store(sink->getSamplesWritten(), std::memory_order_relaxed);
    }

    endAudioWrite();
}

void ScratchRecorder::writeWetBlock(float* const* outputChannelData, int numOutputChannels, int numSamples) noexcept
{
    if (!beginAudioWrite())
        return;

    auto* sink = wetSink.get();
    if (sink == nullptr || !sink->write(const_cast<const float* const*>(outputChannelData), numOutputChannels, numSamples))
    {
        writeError.store(true, std::memory_order_release);
    }
    else
    {
        wetSamplesWritten.store(sink->getSamplesWritten(), std::memory_order_relaxed);
    }

    endAudioWrite();
}

ScratchRecorderStatus ScratchRecorder::getStatus() const
{
    const juce::ScopedLock lock(stateLock);
    auto copy = status;

    copy.state = loadState(state);
    copy.rawSamplesWritten = rawSamplesWritten.load(std::memory_order_relaxed);
    copy.wetSamplesWritten = wetSamplesWritten.load(std::memory_order_relaxed);
    copy.elapsedSamples = elapsedFromCounts(copy.rawSamplesWritten, copy.wetSamplesWritten);
    copy.scratchRoot = scratchRoot;

    if (copy.state == ScratchRecorderState::Recording)
    {
        copy.message = "Recording";
        copy.activeTake = currentTake;
        if (currentTake.takeDirectory != juce::File())
            copy.scratchRoot = currentTake.takeDirectory.getParentDirectory().getParentDirectory();
    }
    else if (copy.state == ScratchRecorderState::Saving)
        copy.message = "Saving";

    return copy;
}

bool ScratchRecorder::isRecording() const noexcept
{
    return loadState(state) == ScratchRecorderState::Recording;
}

juce::File ScratchRecorder::getDefaultScratchRoot()
{
    return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
        .getChildFile("Pedalboard3")
        .getChildFile("Scratch Ideas");
}

juce::File ScratchRecorder::getScratchRoot() const
{
    const juce::ScopedLock lock(stateLock);
    return scratchRoot;
}

void ScratchRecorder::setScratchRoot(const juce::File& root)
{
    const juce::ScopedLock lock(stateLock);
    scratchRoot = root;
    status.scratchRoot = scratchRoot;
}

void ScratchRecorder::resetScratchRootToDefault()
{
    setScratchRoot(getDefaultScratchRoot());
}

void ScratchRecorder::finishPendingStopForTests()
{
    cancelPendingUpdate();
    finishStop();
}

void ScratchRecorder::handleAsyncUpdate()
{
    finishStop();
}

void ScratchRecorder::finishStop()
{
    auto expected = static_cast<int>(ScratchRecorderState::Recording);
    if (!state.compare_exchange_strong(expected,
                                       static_cast<int>(ScratchRecorderState::Saving),
                                       std::memory_order_acq_rel,
                                       std::memory_order_acquire))
        return;

    stopRequested.store(true, std::memory_order_release);

    while (activeAudioWrites.load(std::memory_order_acquire) > 0)
        juce::Thread::yield();

    const juce::ScopedLock lock(stateLock);

    status.state = ScratchRecorderState::Saving;
    status.message = "Saving";

    if (rawSink != nullptr)
        rawSink->close();
    if (wetSink != nullptr)
        wetSink->close();

    rawSink.reset();
    wetSink.reset();

    const auto rawSamples = rawSamplesWritten.load(std::memory_order_relaxed);
    const auto wetSamples = wetSamplesWritten.load(std::memory_order_relaxed);
    const auto durationSamples = elapsedFromCounts(rawSamples, wetSamples);
    const auto countsMatch = rawSamples == wetSamples;
    const auto hadWriteError = writeError.load(std::memory_order_acquire);

    currentTake.durationSamples = durationSamples;

    if (hadWriteError && currentTake.failureReason.isEmpty())
        currentTake.failureReason = "Scratch capture write failed";
    else if (!countsMatch && currentTake.failureReason.isEmpty())
        currentTake.failureReason = "Scratch capture raw and wet sample counts did not match";

    currentTake.complete = !hadWriteError && countsMatch && currentTake.failureReason.isEmpty();

    if (!currentTake.writeMetadata())
    {
        currentTake.complete = false;
        if (currentTake.failureReason.isEmpty())
            currentTake.failureReason = "Unable to write scratch take metadata";
    }

    addRecentTake(currentTake);

    const auto finalState = currentTake.complete ? ScratchRecorderState::Saved : ScratchRecorderState::Failed;
    status.state = finalState;
    status.message = currentTake.complete ? "Saved" : currentTake.failureReason;
    status.elapsedSamples = durationSamples;
    status.rawSamplesWritten = rawSamples;
    status.wetSamplesWritten = wetSamples;
    status.scratchRoot = scratchRoot;
    status.activeTake.reset();
    status.lastTake = currentTake;
    status.recentTakes = recentTakes;

    stopRequested.store(false, std::memory_order_release);
    state.store(static_cast<int>(finalState), std::memory_order_release);
}

void ScratchRecorder::failStart(const juce::String& message)
{
    rawSink.reset();
    wetSink.reset();
    rawSamplesWritten.store(0, std::memory_order_relaxed);
    wetSamplesWritten.store(0, std::memory_order_relaxed);
    stopRequested.store(false, std::memory_order_release);
    writeError.store(false, std::memory_order_release);
    activeAudioWrites.store(0, std::memory_order_release);

    status.state = ScratchRecorderState::Failed;
    status.message = message;
    status.elapsedSamples = 0;
    status.rawSamplesWritten = 0;
    status.wetSamplesWritten = 0;
    status.scratchRoot = scratchRoot;
    status.activeTake.reset();
    status.lastTake.reset();
    status.recentTakes = recentTakes;

    state.store(static_cast<int>(ScratchRecorderState::Failed), std::memory_order_release);
}

bool ScratchRecorder::beginAudioWrite() noexcept
{
    activeAudioWrites.fetch_add(1, std::memory_order_acq_rel);

    if (!isRecording() || stopRequested.load(std::memory_order_acquire))
    {
        endAudioWrite();
        return false;
    }

    return true;
}

void ScratchRecorder::endAudioWrite() noexcept
{
    activeAudioWrites.fetch_sub(1, std::memory_order_acq_rel);
}

void ScratchRecorder::addRecentTake(const ScratchTake& take)
{
    recentTakes.insert(recentTakes.begin(), take);

    if (recentTakes.size() > kMaxRecentTakes)
        recentTakes.resize(kMaxRecentTakes);
}
