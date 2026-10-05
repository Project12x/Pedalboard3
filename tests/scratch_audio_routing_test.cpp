#include "ScratchAudioRouter.h"
#include "ScratchPreviewGuard.h"
#include "ScratchTestFiles.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

namespace
{
class ConstantDecoder : public ScratchPcmDecoder
{
public:
    juce::Result open(const juce::File& file, double) override { value = file.getFileName() == "raw.wav" ? 0.2f : 0.3f; return juce::Result::ok(); }
    int channels() const override { return 1; }
    int64_t lengthFrames() const override { return 480000; }
    void seek(int64_t) override {}
    int read(float* const* data, int frames) override { std::fill_n(data[0], frames, value); return frames; }
    float value = 0;
};
struct RoutingProbe
{
    int renders = 0, protections = 0;
    float lastInputPeak = 0;
    bool muted = false;
    static void render(void* context, const float* const* in, int ins, float* const* out, int outs, int frames)
    {
        auto& p = *static_cast<RoutingProbe*>(context); ++p.renders; p.lastInputPeak = 0;
        for (int ch = 0; ch < outs; ++ch) for (int i = 0; i < frames; ++i)
        {
            const float x = ch < ins && in[ch] ? in[ch][i] : 0;
            p.lastInputPeak = std::max(p.lastInputPeak, std::abs(x));
            // Input gain 2 -> nonlinear graph -> master insert offset -> output gain 3.
            out[ch][i] = ((x * 2) * (x * 2) + 0.01f) * 3;
        }
    }
    static void protect(void* context, float* const* out, int channels, int frames)
    {
        auto& p = *static_cast<RoutingProbe*>(context); ++p.protections;
        if (p.muted) for (int ch = 0; ch < channels; ++ch) std::fill_n(out[ch], frames, 0.0f);
    }
};
struct RoutingHarness
{
    ScratchPlayback playback{[] { return std::make_unique<ConstantDecoder>(); }};
    ScratchAudioRouter router;
    RoutingProbe probe;
    ScratchRecorder* recorder = nullptr;
    juce::AudioBuffer<float> input{2, 128}, output{2, 128};
    RoutingHarness()
    {
        playback.prepare({48000, 128, 2, 2}); router.prepare({48000, 128, 2, 2});
        for (int ch = 0; ch < 2; ++ch) std::fill_n(input.getWritePointer(ch), 128, 0.1f);
    }
    ~RoutingHarness() { playback.audioDeviceStopped(); }
    void tick()
    {
        router.process(playback, recorder, input.getArrayOfReadPointers(), 2, output.getArrayOfWritePointers(), 2, 128,
                       {&probe, &RoutingProbe::render, &RoutingProbe::protect});
    }
    void start(ScratchPlaybackMode mode)
    {
        ScratchTakeLoadResult take; take.state = ScratchTakeLoadState::Ready;
        take.rawReadable = take.wetReadable = true;
        take.take.rawChannelCount = take.take.wetChannelCount = 1;
        auto root = juce::File::getSpecialLocation(juce::File::tempDirectory);
        take.take.rawFile = root.getChildFile("raw.wav"); take.take.wetFile = root.getChildFile("wet.wav");
        REQUIRE(playback.play(take, mode, {}).wasOk());
        for (int n = 0; n < 5000 && playback.status().state != ScratchPlaybackState::Playing; ++n)
        { tick(); juce::Thread::sleep(1); }
        REQUIRE(playback.status().state == ScratchPlaybackState::Playing);
        for (int n = 0; n < 12; ++n) tick();
    }
};
}

TEST_CASE("Scratch wet bypasses the current processing path", "[scratch][scratch-routing]")
{
    RoutingHarness h; h.start(ScratchPlaybackMode::OriginalWet);
    const int before = h.probe.renders; h.tick();
    REQUIRE(h.probe.renders == before + 1);
    REQUIRE(h.probe.lastInputPeak == 0);
    REQUIRE(h.output.getSample(0, 127) == Catch::Approx(0.3f * 0.25118864f));
    REQUIRE(h.output.getSample(1, 127) == Catch::Approx(0.3f * 0.25118864f));
    h.probe.muted = true; h.tick(); REQUIRE(h.output.getMagnitude(0, 128) == 0);
}
TEST_CASE("Scratch raw enters the current processing path once without hardware leakage", "[scratch][scratch-routing]")
{
    RoutingHarness h; h.start(ScratchPlaybackMode::CurrentRigRaw);
    const int before = h.probe.renders; h.tick();
    REQUIRE(h.probe.renders == before + 1);
    REQUIRE(h.probe.lastInputPeak == Catch::Approx(0.2f));
    REQUIRE(h.output.getSample(0, 127) == Catch::Approx(0.51f * 0.25118864f));
    // Mono RAW is not duplicated: the second graph input is silent, but rack output may have a tail/generator.
    REQUIRE(h.output.getSample(1, 127) == Catch::Approx(0.03f * 0.25118864f));
    h.playback.stop();
    for (int n = 0; n < 5000 && !h.playback.isQuiescent(); ++n) { h.tick(); juce::Thread::sleep(1); }
    REQUIRE(h.playback.isQuiescent()); h.tick(); h.tick(); // finish the live-input return ramp
    REQUIRE(h.probe.lastInputPeak == Catch::Approx(0.1f));
    REQUIRE(h.output.getSample(0, 127) == Catch::Approx(0.15f));
}

TEST_CASE("Paused Scratch raw suppresses current-rig generators and tails", "[scratch][scratch-routing]")
{
    RoutingHarness h; h.start(ScratchPlaybackMode::CurrentRigRaw);
    h.playback.pause();
    for (int n = 0; n < 4; ++n) h.tick();
    REQUIRE(h.output.getMagnitude(0, 128) == 0);
    REQUIRE(h.probe.lastInputPeak == 0);
    REQUIRE(h.playback.status().replacingLive);
}

TEST_CASE("Neither Scratch preview route admits recording taps", "[scratch][scratch-routing]")
{
    ScratchTestFiles files;
    ThreadedWavSinkFactory factory; ScratchRecorder recorder(factory);
    RoutingHarness h;
    SECTION("WET") { h.start(ScratchPlaybackMode::OriginalWet); }
    SECTION("RAW") { h.start(ScratchPlaybackMode::CurrentRigRaw); }
    ScratchTakeContext context; context.rootDirectory = files.root; context.sampleRate = 48000;
    context.rawChannelCount = context.wetChannelCount = 2;
    REQUIRE(recorder.start(context)); h.recorder = &recorder;
    h.tick(); h.tick();
    const auto status = recorder.getStatus();
    REQUIRE(status.rawSamplesWritten == 0);
    REQUIRE(status.wetSamplesWritten == 0);
    h.recorder = nullptr; recorder.requestStop(); recorder.finishPendingStopForTests();
}
