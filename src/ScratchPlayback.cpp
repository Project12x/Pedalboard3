#include "ScratchPlayback.h"
#include <cmath>

namespace
{
class WavDecoder final : public ScratchPcmDecoder
{
public:
    juce::Result open(const juce::File& file, double deviceRate) override
    {
        resampler.reset(); source.reset();
        auto reader = openScratchWavReader(file);
        if (!reader || reader->lengthInSamples <= 0 || reader->numChannels < 1 || reader->numChannels > 16
            || !std::isfinite(reader->sampleRate) || reader->sampleRate < 8000 || reader->sampleRate > 384000
            || !std::isfinite(deviceRate) || deviceRate < 8000 || deviceRate > 384000)
            return juce::Result::fail("Scratch audio is empty, unreadable, or outside supported channel/rate limits");
        channelCount = static_cast<int>(reader->numChannels);
        ratio = reader->sampleRate / deviceRate;
        const auto frames = static_cast<long double>(reader->lengthInSamples) / ratio;
        if (frames >= static_cast<long double>(INT64_MAX)) return juce::Result::fail("Scratch audio is too long");
        total = static_cast<int64_t>(std::llround(frames));
        source = std::make_unique<juce::AudioFormatReaderSource>(reader.release(), true);
        resampler = std::make_unique<juce::ResamplingAudioSource>(source.get(), false, channelCount);
        resampler->setResamplingRatio(ratio);
        resampler->prepareToPlay(2048, deviceRate);
        position = 0;
        return total > 0 ? juce::Result::ok() : juce::Result::fail("Scratch audio contains no destination frames");
    }
    int channels() const override { return channelCount; }
    int64_t lengthFrames() const override { return total; }
    void seek(int64_t frame) override
    {
        position = juce::jlimit<int64_t>(0, total, frame);
        source->setNextReadPosition(static_cast<juce::int64>(position * ratio));
        resampler->flushBuffers();
    }
    int read(float* const* destination, int frames) override
    {
        const int count = static_cast<int>(juce::jmin<int64_t>(frames, total - position));
        if (count <= 0) return 0;
        juce::AudioBuffer<float> view(destination, channelCount, count);
        resampler->getNextAudioBlock({&view, 0, count});
        position += count;
        return count;
    }
private:
    std::unique_ptr<juce::AudioFormatReaderSource> source;
    std::unique_ptr<juce::ResamplingAudioSource> resampler;
    int channelCount = 0;
    int64_t total = 0, position = 0;
    double ratio = 1;
};

juce::Result resolveMap(ScratchChannelMap& map, int sources, int destinations, ScratchPlaybackMode mode)
{
    if (sources < 1 || sources > 16 || destinations < 1 || destinations > 16)
        return juce::Result::fail("Scratch needs 1–16 source and destination channels");
    if (map.explicitMapping)
    {
        if (map.destinationChannels != destinations) return juce::Result::fail("Channel mapping does not match the current device");
        for (int ch = 0; ch < destinations; ++ch)
            if (map.sourceForDestination[ch] < -1 || map.sourceForDestination[ch] >= sources)
                return juce::Result::fail("Channel mapping refers to a missing source channel");
    }
    else
    {
        if (sources > destinations) return juce::Result::fail("Choose an explicit channel map before dropping recorded channels");
        map.sourceForDestination.fill(-1);
        map.destinationChannels = destinations;
        for (int ch = 0; ch < sources; ++ch) map.sourceForDestination[ch] = ch;
        if (sources == 1 && destinations >= 2 && mode == ScratchPlaybackMode::OriginalWet)
            map.sourceForDestination[1] = 0;
    }
    return juce::Result::ok();
}
}
std::unique_ptr<ScratchPcmDecoder> makeScratchPcmDecoder() { return std::make_unique<WavDecoder>(); }

ScratchPlayback::ScratchPlayback(ScratchDecoderFactory decoderFactory)
    : factory(decoderFactory ? std::move(decoderFactory) : makeScratchPcmDecoder), worker([this] { run(); }) {}
ScratchPlayback::~ScratchPlayback()
{
    audioDeviceStopped();
    { const std::lock_guard<std::mutex> lock(mutex); quitting.store(true); }
    wake.notify_one();
    if (worker.joinable()) worker.join();
}
void ScratchPlayback::requestLocked()
{
    message.clear();
    workerIdle.store(0, std::memory_order_release);
    const auto generation = requested.fetch_add(1, std::memory_order_release) + 1;
    if (deviceStopped.load(std::memory_order_acquire)) acknowledged.store(generation, std::memory_order_release);
    wake.notify_one();
}
void ScratchPlayback::prepare(const ScratchPlaybackDevice& device)
{
    audioDeviceStopped();
    std::unique_lock<std::mutex> lock(mutex);
    wake.wait(lock, [&] { return workerIdle.load(std::memory_order_acquire) == requested.load(); });
    command.device = device;
    fadeFrames = std::isfinite(device.sampleRate) && device.sampleRate >= 8000 && device.sampleRate <= 384000
        ? juce::jmax(1, static_cast<int>(device.sampleRate * 0.005)) : 1;
    last.fill(0); tailStart.fill(0); tailRemaining = fadeInRemaining = 0;
    held = false; seenRequest = requested.load(); playingGeneration = 0;
    deviceStopped.store(false, std::memory_order_release);
}
juce::Result ScratchPlayback::play(const ScratchTakeLoadResult& take, ScratchPlaybackMode mode, const ScratchChannelMap& supplied)
{
    const std::lock_guard<std::mutex> lock(mutex);
    if (deviceStopped.load() || !std::isfinite(command.device.sampleRate) || command.device.sampleRate < 8000
        || command.device.sampleRate > 384000 || command.device.maxBlockSize < 1 || command.device.maxBlockSize > 8192)
        return juce::Result::fail("No supported audio device is running");
    if (take.state == ScratchTakeLoadState::Invalid || take.state == ScratchTakeLoadState::Unsupported
        || !(mode == ScratchPlaybackMode::OriginalWet ? take.wetReadable : take.rawReadable))
        return juce::Result::fail("This take has no readable audio for the requested mode");
    auto map = supplied;
    const auto result = resolveMap(map, mode == ScratchPlaybackMode::OriginalWet ? take.take.wetChannelCount : take.take.rawChannelCount,
        mode == ScratchPlaybackMode::OriginalWet ? command.device.outputChannels : command.device.graphInputChannels, mode);
    if (result.failed()) return result;
    command.take = take; command.mode = mode; command.map = map; command.seekSeconds = 0; command.active = true;
    wanted.store(true); paused.store(false); state.store(ScratchPlaybackState::Loading); buffering.store(true);
    requestLocked();
    return juce::Result::ok();
}
void ScratchPlayback::pause() { paused.store(true, std::memory_order_release); }
void ScratchPlayback::resume() { paused.store(false, std::memory_order_release); }
void ScratchPlayback::stop()
{
    const std::lock_guard<std::mutex> lock(mutex);
    command.active = false; wanted.store(false); paused.store(false);
    state.store(ScratchPlaybackState::Stopped); requestLocked();
}
void ScratchPlayback::seek(double seconds)
{
    if (!std::isfinite(seconds)) return;
    const std::lock_guard<std::mutex> lock(mutex);
    if (!command.active) return;
    command.seekSeconds = juce::jmax(0.0, seconds);
    wanted.store(true); state.store(ScratchPlaybackState::Loading); buffering.store(true); requestLocked();
}
void ScratchPlayback::setLooping(bool loop) { looping.store(loop); wake.notify_one(); }
void ScratchPlayback::setVolumeDb(double db)
{
    if (!std::isfinite(db)) return;
    db = juce::jlimit(-60.0, 0.0, db); volumeDb.store(db);
    volumeGain.store(db <= -60 ? 0.0f : juce::Decibels::decibelsToGain(static_cast<float>(db)));
}
void ScratchPlayback::audioDeviceStopped()
{
    deviceStopped.store(true, std::memory_order_release);
    stop();
    acknowledged.store(requested.load(), std::memory_order_release);
    last.fill(0); tailRemaining = 0;
    wake.notify_one();
}
bool ScratchPlayback::isQuiescent() const noexcept
{
    const auto generation = requested.load(std::memory_order_acquire);
    return !wanted.load() && acknowledged.load(std::memory_order_acquire) == generation
        && workerIdle.load(std::memory_order_acquire) == generation;
}
ScratchPlaybackStatus ScratchPlayback::status() const
{
    const std::lock_guard<std::mutex> lock(mutex);
    ScratchPlaybackStatus result;
    result.state = state.load();
    if (result.state == ScratchPlaybackState::Playing && paused.load()) result.state = ScratchPlaybackState::Paused;
    if (ended.load() == requested.load() && requested.load() != 0) result.state = ScratchPlaybackState::Stopped;
    result.mode = command.mode; result.takeDirectory = command.take.take.takeDirectory;
    if (command.device.sampleRate > 0)
    {
        result.positionSeconds = static_cast<double>(position.load()) / command.device.sampleRate;
        result.durationSeconds = static_cast<double>(length.load()) / command.device.sampleRate;
    }
    result.volumeDb = volumeDb.load(); result.looping = looping.load(); result.buffering = buffering.load();
    result.replacingLive = wanted.load() && ended.load() != requested.load(); result.message = message;
    return result;
}
void ScratchPlayback::fail(uint64_t generation, const juce::String& reason)
{
    const std::lock_guard<std::mutex> lock(mutex);
    if (requested.load() != generation) return;
    wanted.store(false); command.active = false;
    state.store(ScratchPlaybackState::Error); buffering.store(false);
    requestLocked(); // retire any published samples with the normal callback-side fade
    message = reason;
}
void ScratchPlayback::run()
{
    uint64_t generation = 0;
    Command active;
    std::unique_ptr<ScratchPcmDecoder> decoder;
    juce::AudioBuffer<float> decoded(16, 2048), mapped(16, 2048);
    int64_t produced = 0;
    for (;;)
    {
        if (quitting.load(std::memory_order_acquire)) return;
        const auto request = requested.load(std::memory_order_acquire);
        if (request != generation)
        {
            // Stop producing first; consumer acknowledgement grants reset ownership.
            if (acknowledged.load(std::memory_order_acquire) != request)
            {
                std::unique_lock<std::mutex> lock(mutex); wake.wait_for(lock, std::chrono::milliseconds(1)); continue;
            }
            {
                const std::lock_guard<std::mutex> lock(mutex);
                if (requested.load() != request) continue;
                active = command;
            }
            decoder.reset(); queue.reset(); published.store(0, std::memory_order_release);
            generation = request; eof.store(false); produced = 0;
            if (!active.active)
            {
                workerIdle.store(generation, std::memory_order_release); buffering.store(false); wake.notify_one(); continue;
            }
            try
            {
                decoder = factory();
                const auto result = decoder ? decoder->open(active.mode == ScratchPlaybackMode::OriginalWet
                    ? active.take.take.wetFile : active.take.take.rawFile, active.device.sampleRate)
                    : juce::Result::fail("Scratch decoder unavailable");
                if (requested.load() != generation) continue;
                if (result.failed()) { decoder.reset(); fail(generation, result.getErrorMessage()); continue; }
                const auto mapResult = resolveMap(active.map, decoder->channels(), active.map.destinationChannels, active.mode);
                if (decoder->lengthFrames() <= 0 || mapResult.failed())
                {
                    decoder.reset(); fail(generation, mapResult.failed() ? mapResult.getErrorMessage() : "Scratch audio is empty"); continue;
                }
                publication.mode = active.mode; publication.length = decoder->lengthFrames();
                publication.start = static_cast<int64_t>(std::min<long double>(publication.length,
                    active.seekSeconds * static_cast<long double>(active.device.sampleRate)));
                decoder->seek(publication.start);
                if (requested.load(std::memory_order_acquire) != generation) continue;
                produced = publication.start;
                position.store(produced); length.store(publication.length);
            }
            catch (const std::exception& exception) { decoder.reset(); fail(generation, exception.what()); continue; }
        }
        if (decoder && ended.load(std::memory_order_acquire) == generation)
        {
            decoder.reset(); published.store(0);
            const std::lock_guard<std::mutex> lock(mutex);
            if (requested.load() == generation)
            {
                wanted.store(false); buffering.store(false);
                workerIdle.store(generation, std::memory_order_release); wake.notify_one();
            }
        }
        bool didWork = false;
        if (decoder && queue.free() >= 2048)
        {
            try
            {
                if (produced >= publication.length && looping.load()) { decoder->seek(0); produced = 0; eof.store(false); }
                if (produced < publication.length)
                {
                    const int count = decoder->read(decoded.getArrayOfWritePointers(), 2048);
                    if (requested.load(std::memory_order_acquire) != generation) continue;
                    if (count <= 0 || count > 2048) { decoder.reset(); fail(generation, "Scratch audio read failed"); continue; }
                    for (int ch = 0; ch < active.map.destinationChannels; ++ch)
                    {
                        const int source = active.map.sourceForDestination[ch];
                        if (source < 0) mapped.clear(ch, 0, count);
                        else mapped.copyFrom(ch, 0, decoded, source, 0, count);
                    }
                    queue.write(mapped.getArrayOfReadPointers(), active.map.destinationChannels, count);
                    produced += count; didWork = true;
                }
                if (produced >= publication.length && !looping.load()) eof.store(true, std::memory_order_release);
                if (queue.ready() >= active.device.maxBlockSize * 2 || eof.load())
                {
                    buffering.store(false, std::memory_order_release);
                    if (published.load() != generation)
                    {
                        published.store(generation, std::memory_order_release);
                        const std::lock_guard<std::mutex> lock(mutex);
                        if (requested.load() == generation) state.store(ScratchPlaybackState::Playing);
                    }
                }
            }
            catch (const std::exception& exception) { decoder.reset(); fail(generation, exception.what()); }
        }
        if (!didWork) { std::unique_lock<std::mutex> lock(mutex); wake.wait_for(lock, std::chrono::milliseconds(2)); }
    }
}
void ScratchPlayback::beginTail() noexcept
{
    tailStart = last;
    tailRemaining = 0;
    for (const auto sample : last) if (sample != 0) { tailRemaining = fadeFrames; break; }
}
void ScratchPlayback::renderTail(float* const* output, int channels, int start, int frames) noexcept
{
    for (int i = start; i < frames; ++i)
    {
        const float gain = tailRemaining > 0 ? static_cast<float>(--tailRemaining) / fadeFrames : 0;
        for (int ch = 0; ch < channels; ++ch)
        {
            last[ch] = gain > 0 ? tailStart[ch] * gain : 0;
            if (output[ch]) output[ch][i] = last[ch];
        }
    }
}
ScratchPlaybackBlock ScratchPlayback::readBlock(float* const* output, int channels, int samples) noexcept
{
    ScratchPlaybackBlock block;
    block.mode = audioMode; block.generation = requested.load(std::memory_order_acquire); block.previewGain = volumeGain.load();
    for (int ch = 0; ch < channels; ++ch) if (output[ch]) juce::FloatVectorOperations::clear(output[ch], samples);
    if (channels < 1 || channels > 16 || samples < 1 || samples > 8192 || deviceStopped.load()) return block;
    const auto generation = block.generation;
    if (seenRequest != generation) { seenRequest = generation; beginTail(); held = true; }
    if (acknowledged.load(std::memory_order_relaxed) != generation)
    {
        block.replacingLive = wanted.load() || tailRemaining > 0;
        renderTail(output, channels, 0, samples);
        if (tailRemaining == 0) acknowledged.store(generation, std::memory_order_release);
        return block;
    }
    if (!wanted.load() || ended.load(std::memory_order_acquire) == generation) return block;
    block.replacingLive = true;
    // A queued old generation must NEVER be consumed while its replacement loads.
    if (published.load(std::memory_order_acquire) != generation) return block;
    if (playingGeneration != generation)
    {
        playingGeneration = generation; audioMode = publication.mode;
        fadeInRemaining = fadeFrames; last.fill(0); tailRemaining = 0; held = false;
    }
    block.mode = audioMode;
    const bool hold = paused.load() || (buffering.load(std::memory_order_acquire) && !eof.load());
    if (hold)
    {
        if (!held) beginTail();
        held = true; renderTail(output, channels, 0, samples); return block;
    }
    if (held) { fadeInRemaining = fadeFrames; held = false; }
    const auto start = position.load(std::memory_order_relaxed);
    const bool loop = looping.load();
    const int framesToRead = loop ? samples
        : static_cast<int>(juce::jlimit<int64_t>(0, samples, publication.length - start));
    block.validFrames = queue.read(output, channels, framesToRead);
    for (int i = 0; i < block.validFrames; ++i)
    {
        float gain = fadeInRemaining > 0 ? 1.0f - static_cast<float>(--fadeInRemaining) / fadeFrames : 1.0f;
        const auto frame = loop ? (start + i) % publication.length : start + i;
        gain *= juce::jlimit(0.0f, 1.0f, static_cast<float>(publication.length - frame - 1) / fadeFrames);
        if (loop) gain *= juce::jlimit(0.0f, 1.0f, static_cast<float>(frame + 1) / fadeFrames);
        for (int ch = 0; ch < channels; ++ch)
            if (output[ch]) last[ch] = (output[ch][i] *= gain);
    }
    const auto next = start + block.validFrames;
    position.store(loop ? next % publication.length : next, std::memory_order_relaxed);
    if (!loop && next >= publication.length)
    {
        ended.store(generation, std::memory_order_release); last.fill(0);
    }
    else if (block.validFrames < samples)
    {
        buffering.store(true, std::memory_order_release); held = true; beginTail();
        renderTail(output, channels, block.validFrames, samples);
    }
    return block;
}
