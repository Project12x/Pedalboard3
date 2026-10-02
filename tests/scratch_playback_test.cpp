#include "ScratchPlayback.h"
#include "ScratchPcmQueue.h"
#include "ScratchTestFiles.h"
#include <catch2/catch_approx.hpp>
#include <atomic>
#include <cmath>
#include <thread>

namespace
{
struct DecoderControl
{
    std::atomic<bool> stall{false}, entered{false};
    juce::WaitableEvent release;
    int channels = 1;
    int64_t length = 96000;
    std::atomic<int> opens{0};
    std::thread::id caller = std::this_thread::get_id();
    std::atomic<bool> offThread{true};
};
class TestDecoder final : public ScratchPcmDecoder
{
public:
    explicit TestDecoder(DecoderControl& c) : control(c) {}
    juce::Result open(const juce::File&, double) override
    {
        ++control.opens;
        control.offThread.store(std::this_thread::get_id() != control.caller);
        return juce::Result::ok();
    }
    int channels() const override { return control.channels; }
    int64_t lengthFrames() const override { return control.length; }
    void seek(int64_t frame) override { position = frame; }
    int read(float* const* destination, int frames) override
    {
        if (control.stall.load()) { control.entered.store(true); control.release.wait(5000); }
        const int count = static_cast<int>(juce::jmin<int64_t>(frames, control.length - position));
        for (int ch = 0; ch < control.channels; ++ch)
            for (int i = 0; i < count; ++i)
                destination[ch][i] = (position + i < 48000 ? -0.25f : 0.5f) + ch * 0.01f;
        position += count;
        return count;
    }
private:
    DecoderControl& control;
    int64_t position = 0;
};
ScratchTakeLoadResult takeFor(int channels = 1)
{
    ScratchTakeLoadResult take;
    take.state = ScratchTakeLoadState::Ready;
    take.rawReadable = take.wetReadable = true;
    take.take.rawChannelCount = take.take.wetChannelCount = channels;
    return take;
}
template<class Predicate> bool await(Predicate predicate)
{
    const auto end = juce::Time::getMillisecondCounter() + 5000;
    while (!predicate() && juce::Time::getMillisecondCounter() < end) juce::Thread::sleep(1);
    return predicate();
}
struct Harness
{
    DecoderControl control;
    ScratchPlayback playback{[this] { return std::make_unique<TestDecoder>(control); }};
    juce::AudioBuffer<float> audio{16, 8192};
    Harness() { playback.prepare({48000, 128, 16, 16}); }
    ~Harness() { control.stall.store(false); control.release.signal(); playback.audioDeviceStopped(); }
    ScratchPlaybackBlock tick(int frames = 128) { return playback.readBlock(audio.getArrayOfWritePointers(), 16, frames); }
    void start(ScratchPlaybackMode mode = ScratchPlaybackMode::OriginalWet)
    {
        REQUIRE(playback.play(takeFor(control.channels), mode, {}).wasOk());
        REQUIRE(await([&] { tick(); return playback.status().state == ScratchPlaybackState::Playing; }));
        tick(512); // finish documented 5 ms ramp
    }
};
}

TEST_CASE("Scratch PCM queue wraps without changing frame order and clears underflow", "[scratch][scratch-playback]")
{
    ScratchPcmQueue queue;
    juce::AudioBuffer<float> source(16, 4096), destination(16, 4096);
    int sequence = 0;
    for (int block = 0; block < 40; ++block)
    {
        for (int ch = 0; ch < 16; ++ch)
            for (int i = 0; i < 4096; ++i) source.setSample(ch, i, static_cast<float>(sequence + i + ch));
        REQUIRE(queue.write(source.getArrayOfReadPointers(), 16, 4096) == 4096);
        REQUIRE(queue.read(destination.getArrayOfWritePointers(), 16, 4096) == 4096);
        for (int ch : {0, 15})
            for (int i : {0, 2048, 4095}) REQUIRE(destination.getSample(ch, i) == sequence + i + ch);
        sequence += 4096;
    }
    REQUIRE(queue.read(destination.getArrayOfWritePointers(), 16, 4096) == 0);
    REQUIRE(destination.getMagnitude(0, 4096) == 0);
}

TEST_CASE("Scratch seek never consumes the retired generation", "[scratch][scratch-playback]")
{
    Harness h;
    h.start();
    REQUIRE(h.audio.getSample(0, 511) == Catch::Approx(-0.25f));
    h.playback.seek(1.0);
    const auto generation = h.tick().generation;
    REQUIRE(await([&] { h.tick(); return h.playback.status().state == ScratchPlaybackState::Playing; }));
    h.tick(512);
    const auto block = h.tick();
    REQUIRE(block.generation == generation);
    REQUIRE(block.validFrames == 128);
    for (int i = 0; i < 128; ++i) REQUIRE(h.audio.getSample(0, i) == Catch::Approx(0.5f));
    REQUIRE_FALSE(h.playback.status().buffering);
    REQUIRE(h.control.offThread.load());
}

TEST_CASE("Scratch pause holds position and stop/device removal retire without another callback", "[scratch][scratch-playback]")
{
    Harness h;
    h.start();
    h.playback.pause();
    const auto position = h.playback.status().positionSeconds;
    REQUIRE(h.tick(512).replacingLive);
    REQUIRE(h.playback.status().positionSeconds == position);
    REQUIRE(h.tick().replacingLive);
    REQUIRE(h.audio.getMagnitude(0, 128) == 0);
    h.playback.resume();
    h.tick(512);
    REQUIRE(h.playback.status().positionSeconds > position);
    h.playback.stop();
    h.playback.audioDeviceStopped();
    REQUIRE(await([&] { return h.playback.isQuiescent(); }));
    REQUIRE_FALSE(h.playback.status().replacingLive);
    for (int i = 0; i < 3; ++i) { h.playback.prepare({48000, 128, 16, 16}); h.playback.audioDeviceStopped(); }
}

TEST_CASE("Scratch seeking while decoder is stalled rejects old queued PCM", "[scratch][scratch-playback][scratch-generation]")
{
    Harness h;
    h.start();
    h.control.stall.store(true);
    // Drain enough buffered data for the worker to enter another read and block.
    REQUIRE(await([&] { h.tick(128); return h.control.entered.load(); }));
    h.playback.seek(1.0);
    h.tick(512); // retire the old generation's 5 ms tail, acknowledge reset permission
    const auto waiting = h.tick(); // worker still cannot reset/open/prime the new generation
    REQUIRE(waiting.replacingLive);
    REQUIRE(waiting.validFrames == 0);
    REQUIRE(h.audio.getMagnitude(0, 128) == 0);
    h.control.stall.store(false);
    h.control.release.signal();
    REQUIRE(await([&] { h.tick(); return h.playback.status().state == ScratchPlaybackState::Playing; }));
    h.tick(512); h.tick();
    REQUIRE(h.audio.getSample(0, 127) == Catch::Approx(0.5f));
}

TEST_CASE("Scratch stalled decoder cannot block callback or publish a superseded request", "[scratch][scratch-playback]")
{
    Harness h;
    h.control.stall.store(true);
    REQUIRE(h.playback.play(takeFor(), ScratchPlaybackMode::OriginalWet, {}).wasOk());
    h.tick();
    REQUIRE(await([&] { return h.control.entered.load(); }));
    for (int i = 0; i < 50; ++i) h.playback.seek(1.0);
    for (int i = 0; i < 8; ++i) { REQUIRE(h.tick().validFrames == 0); REQUIRE(h.audio.getMagnitude(0, 128) == 0); }
    h.control.stall.store(false);
    h.control.release.signal();
    REQUIRE(await([&] { h.tick(); return h.playback.status().state == ScratchPlaybackState::Playing; }));
    h.tick(512);
    h.tick();
    REQUIRE(h.audio.getSample(0, 127) == Catch::Approx(0.5f));
    REQUIRE(h.control.opens.load() <= 2); // bounded/coalesced command storage
}

TEST_CASE("Scratch channel mapping distinguishes wet mono from raw mono and explicit loss", "[scratch][scratch-playback]")
{
    Harness h;
    h.playback.prepare({48000, 128, 2, 2});
    h.start(); h.tick();
    REQUIRE(h.audio.getSample(1, 127) == Catch::Approx(-0.25f));
    h.playback.stop(); h.playback.audioDeviceStopped();
    h.playback.prepare({48000, 128, 2, 2});
    h.start(ScratchPlaybackMode::CurrentRigRaw); h.tick();
    REQUIRE(h.audio.getSample(1, 127) == 0);
    h.playback.stop(); h.playback.audioDeviceStopped();
    h.playback.prepare({48000, 128, 2, 2});
    REQUIRE(h.playback.play(takeFor(16), ScratchPlaybackMode::OriginalWet, {}).failed());
    ScratchChannelMap map;
    map.explicitMapping = true; map.destinationChannels = 2;
    map.sourceForDestination[0] = 15; map.sourceForDestination[1] = 0;
    h.control.channels = 16;
    REQUIRE(h.playback.play(takeFor(16), ScratchPlaybackMode::OriginalWet, map).wasOk());
    REQUIRE(await([&] { h.tick(); return h.playback.status().state == ScratchPlaybackState::Playing; }));
    h.tick(512); h.tick();
    REQUIRE(h.audio.getSample(0, 127) == Catch::Approx(-0.10f));
    REQUIRE(h.audio.getSample(1, 127) == Catch::Approx(-0.25f));
    h.playback.setVolumeDb(12);
    REQUIRE(h.tick().previewGain == 1);
    h.playback.setVolumeDb(-100);
    REQUIRE(h.tick().previewGain == 0);
}

TEST_CASE("Scratch EOF stops, looping wraps, and empty decoders release live input", "[scratch][scratch-playback]")
{
    Harness h;
    h.control.length = 512;
    REQUIRE(h.playback.play(takeFor(), ScratchPlaybackMode::OriginalWet, {}).wasOk());
    REQUIRE(await([&] { h.tick(); return h.playback.status().state == ScratchPlaybackState::Stopped; }));
    REQUIRE_FALSE(h.tick().replacingLive);
    h.control.length = 0;
    REQUIRE(h.playback.play(takeFor(), ScratchPlaybackMode::OriginalWet, {}).wasOk());
    REQUIRE(await([&] { h.tick(); return h.playback.status().state == ScratchPlaybackState::Error; }));
    REQUIRE_FALSE(h.tick().replacingLive);
    REQUIRE(h.playback.status().message.isNotEmpty());
    h.control.length = 512;
    h.playback.setLooping(true);
    h.start();
    for (int i = 0; i < 12; ++i) { h.tick(); REQUIRE(h.playback.status().positionSeconds < 512.0 / 48000); }
    REQUIRE(h.playback.status().state == ScratchPlaybackState::Playing);
}

TEST_CASE("Scratch production decoder preserves same-rate PCM and resamples pitch and duration", "[scratch][scratch-playback]")
{
    ScratchTestFiles files;
    const auto file = files.root.getChildFile("tone.wav");
    {
        juce::WavAudioFormat format;
        std::unique_ptr<juce::OutputStream> stream(file.createOutputStream());
        auto writer = format.createWriterFor(stream, juce::AudioFormatWriter::Options{}
            .withSampleRate(44100).withNumChannels(1).withBitsPerSample(24));
        REQUIRE(writer != nullptr);
        juce::AudioBuffer<float> tone(1, 44100);
        for (int i = 0; i < 44100; ++i) tone.setSample(0, i, 0.5f * std::sin(juce::MathConstants<double>::twoPi * 440 * i / 44100));
        REQUIRE(writer->writeFromAudioSampleBuffer(tone, 0, 44100));
    }
    auto decoder = makeScratchPcmDecoder();
    REQUIRE(decoder->open(file, 44100).wasOk());
    juce::AudioBuffer<float> decoded(1, 48000);
    REQUIRE(decoder->read(decoded.getArrayOfWritePointers(), 128) == 128);
    REQUIRE(decoded.getSample(0, 100) == Catch::Approx(0.5f * std::sin(juce::MathConstants<double>::twoPi * 440 * 100 / 44100)).margin(0.000001));
    REQUIRE(decoder->open(file, 48000).wasOk());
    REQUIRE(decoder->lengthFrames() == 48000);
    int offset = 0;
    while (offset < 48000)
    {
        float* channel = decoded.getWritePointer(0, offset);
        const auto count = decoder->read(&channel, juce::jmin(2048, 48000 - offset));
        REQUIRE(count > 0);
        offset += count;
    }
    int crossings = 0;
    for (int i = 4801; i < 43200; ++i)
        if (decoded.getSample(0, i - 1) <= 0 && decoded.getSample(0, i) > 0) ++crossings;
    REQUIRE(crossings / 0.8 == Catch::Approx(440).margin(1));
}

TEST_CASE("Scratch underflow fades continuously to silence and resumes after priming", "[scratch][scratch-playback]")
{
    Harness h;
    h.start();
    h.control.stall.store(true);
    REQUIRE(await([&] { h.tick(); return h.control.entered.load(); }));
    ScratchPlaybackBlock block;
    do { block = h.tick(8192); } while (block.validFrames == 8192);
    REQUIRE(block.replacingLive);
    REQUIRE(h.playback.status().buffering);
    if (block.validFrames > 0 && block.validFrames < 8192)
        REQUIRE(h.audio.getSample(0, block.validFrames) == Catch::Approx(h.audio.getSample(0, block.validFrames - 1) * 239.0f / 240.0f));
    h.tick(512);
    REQUIRE(h.audio.getSample(0, 511) == 0);
    h.control.stall.store(false); h.control.release.signal();
    REQUIRE(await([&] { return !h.playback.status().buffering; }));
    REQUIRE(h.tick().validFrames == 128);
}

TEST_CASE("Scratch stop without callbacks waits for decoder ownership before quiescence", "[scratch][scratch-playback]")
{
    Harness h;
    h.control.stall.store(true);
    REQUIRE(h.playback.play(takeFor(), ScratchPlaybackMode::OriginalWet, {}).wasOk()); h.tick();
    REQUIRE(await([&] { return h.control.entered.load(); }));
    h.playback.stop(); h.playback.audioDeviceStopped();
    REQUIRE_FALSE(h.playback.isQuiescent());
    h.control.stall.store(false); h.control.release.signal();
    REQUIRE(await([&] { return h.playback.isQuiescent(); }));
}

TEST_CASE("Scratch seeks clamp and full-width maps preserve all channel indices", "[scratch][scratch-playback]")
{
    Harness h;
    h.control.channels = 16;
    h.start(); h.tick();
    for (int ch = 0; ch < 16; ++ch) REQUIRE(h.audio.getSample(ch, 127) == Catch::Approx(-0.25f + ch * 0.01f));
    h.playback.seek(999);
    REQUIRE(await([&] { h.tick(); return h.playback.status().state == ScratchPlaybackState::Stopped; }));
    REQUIRE(h.playback.status().positionSeconds == 2);
    h.playback.seek(-10);
    REQUIRE(await([&] { h.tick(); return h.playback.status().state == ScratchPlaybackState::Playing; }));
    h.tick(512); h.tick();
    REQUIRE(h.audio.getSample(0, 127) == Catch::Approx(-0.25f));
    h.playback.stop();
    h.tick(512);
    REQUIRE(await([&] { return h.playback.isQuiescent(); }));
    REQUIRE_FALSE(h.tick().replacingLive);
}

TEST_CASE("Scratch production decoder rejects empty WAV and unsafe conversion rates", "[scratch][scratch-playback]")
{
    ScratchTestFiles files;
    const auto empty = files.root.getChildFile("empty.wav");
    ScratchTestFiles::wav(empty, 1, 0);
    auto decoder = makeScratchPcmDecoder();
    REQUIRE(decoder->open(empty, 48000).failed());
    const auto valid = files.root.getChildFile("valid.wav");
    ScratchTestFiles::wav(valid);
    REQUIRE(decoder->open(valid, 1.0e99).failed());
    REQUIRE(decoder->open(valid, 1000).failed());
}

TEST_CASE("Scratch disabling loop stops at the exact end despite decoded-ahead loop data", "[scratch][scratch-playback][scratch-loop-end]")
{
    Harness h;
    h.control.length = 500; // deliberately not a multiple of callback size
    h.playback.setLooping(true);
    h.start();
    h.playback.setLooping(false);
    REQUIRE(await([&] { h.tick(128); return h.playback.status().state == ScratchPlaybackState::Stopped; }));
    REQUIRE(h.playback.status().positionSeconds == Catch::Approx(500.0 / 48000.0));
    REQUIRE_FALSE(h.tick().replacingLive);
}
