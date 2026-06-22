#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "DeviceMeterTap.h"
#include "SafetyLimiter.h"
#include "dsp/MeterSource.h"

#include <array>

using Catch::Approx;

namespace
{
void processMonoBlock(PedalboardMeterSource& source, const std::array<float, 4>& block)
{
    const float* channels[] = {block.data()};
    source.process(channels, 1, static_cast<int>(block.size()));
}
} // namespace

TEST_CASE("PedalboardMeterSource peak tracks signal and decays over silence", "[meter][source]")
{
    PedalboardMeterSource source;
    source.prepare(48000.0, 1);

    processMonoBlock(source, {0.1f, -0.75f, 0.2f, 0.4f});
    REQUIRE(source.getPeak(0) == Approx(0.75f).margin(0.001f));

    const float afterSignal = source.getPeak(0);
    const float* silence[] = {nullptr};
    source.process(silence, 1, 48000);

    REQUIRE(source.getPeak(0) < afterSignal);
    REQUIRE(source.getPeak(0) > 0.0f);
}

TEST_CASE("PedalboardMeterSource reports block RMS through a fixed rolling window", "[meter][source]")
{
    PedalboardMeterSource source;
    source.prepare(48000.0, 1);

    processMonoBlock(source, {0.5f, -0.5f, 0.5f, -0.5f});
    REQUIRE(source.getRms(0) == Approx(0.5f).margin(0.001f));

    processMonoBlock(source, {0.0f, 0.0f, 0.0f, 0.0f});
    REQUIRE(source.getRms(0) < 0.5f);
    REQUIRE(source.getRms(0) > 0.0f);
}

TEST_CASE("PedalboardMeterSource VU has slower response than peak", "[meter][source]")
{
    PedalboardMeterSource source;
    source.prepare(48000.0, 1);

    std::array<float, 128> constant{};
    constant.fill(1.0f);
    const float* channels[] = {constant.data()};

    source.process(channels, 1, static_cast<int>(constant.size()));
    REQUIRE(source.getPeak(0) == Approx(1.0f).margin(0.001f));
    REQUIRE(source.getVu(0) > 0.0f);
    REQUIRE(source.getVu(0) < source.getPeak(0));

    for (int i = 0; i < 120; ++i)
        source.process(channels, 1, static_cast<int>(constant.size()));

    REQUIRE(source.getVu(0) > 0.8f);
    REQUIRE(source.getVu(0) <= 1.05f);
}

TEST_CASE("PedalboardMeterSource clip latch is explicit and clearable", "[meter][source]")
{
    PedalboardMeterSource source;
    source.prepare(48000.0, 1);

    processMonoBlock(source, {0.1f, 1.01f, -0.2f, 0.4f});
    REQUIRE(source.getClip(0));
    REQUIRE(source.getAndClearClip(0));
    REQUIRE_FALSE(source.getClip(0));
    REQUIRE_FALSE(source.getAndClearClip(0));
}

TEST_CASE("PedalboardMeterSource clamps channel bounds and reset state", "[meter][source]")
{
    PedalboardMeterSource source;
    source.prepare(48000.0, PedalboardMeterSource::MaxChannels + 8);
    REQUIRE(source.getNumChannels() == PedalboardMeterSource::MaxChannels);

    processMonoBlock(source, {0.5f, 0.5f, 0.5f, 0.5f});
    REQUIRE(source.getPeak(-1) == 0.0f);
    REQUIRE(source.getRms(PedalboardMeterSource::MaxChannels) == 0.0f);
    REQUIRE(source.getVu(PedalboardMeterSource::MaxChannels) == 0.0f);
    REQUIRE_FALSE(source.getClip(PedalboardMeterSource::MaxChannels));
    REQUIRE_FALSE(source.getAndClearClip(PedalboardMeterSource::MaxChannels));

    source.reset();
    REQUIRE(source.getPeak(0) == 0.0f);
    REQUIRE(source.getRms(0) == 0.0f);
    REQUIRE(source.getVu(0) == 0.0f);
    REQUIRE_FALSE(source.getClip(0));
}

TEST_CASE("DeviceMeterTap exposes peak RMS VU clip and clears on device stop", "[meter][device-tap]")
{
    DeviceMeterTap tap;
    tap.prepareForTest(48000.0);

    std::array<float, 4> input = {0.25f, -0.5f, 0.25f, 1.02f};
    std::array<float, 4> output = {0.1f, 0.2f, -0.75f, 0.2f};
    const float* inputs[] = {input.data()};
    float* outputs[] = {output.data()};
    juce::AudioIODeviceCallbackContext context;

    tap.audioDeviceIOCallbackWithContext(inputs, 1, outputs, 1, static_cast<int>(input.size()), context);

    REQUIRE(tap.getInputLevel(0) == Approx(1.02f).margin(0.001f));
    REQUIRE(tap.getInputRmsLevel(0) > 0.0f);
    REQUIRE(tap.getInputVuLevel(0) > 0.0f);
    REQUIRE(tap.getInputAndClearClip(0));
    REQUIRE_FALSE(tap.getInputAndClearClip(0));

    REQUIRE(tap.getOutputLevel(0) == Approx(0.75f).margin(0.001f));
    REQUIRE(tap.getOutputRmsLevel(0) > 0.0f);
    REQUIRE(tap.getOutputVuLevel(0) > 0.0f);
    REQUIRE_FALSE(tap.getOutputAndClearClip(0));

    for (float sample : output)
        REQUIRE(sample == 0.0f);

    tap.audioDeviceStopped();
    REQUIRE(tap.getInputLevel(0) == 0.0f);
    REQUIRE(tap.getInputRmsLevel(0) == 0.0f);
    REQUIRE(tap.getInputVuLevel(0) == 0.0f);
    REQUIRE(tap.getNumInputChannels() == 0);
    REQUIRE(tap.getNumOutputChannels() == 0);
}

TEST_CASE("DeviceMeterTap clears output channels beyond meter capacity", "[meter][device-tap]")
{
    DeviceMeterTap tap;
    tap.prepareForTest(48000.0);

    std::array<std::array<float, 4>, DeviceMeterTap::MaxChannels + 1> outputStorage{};
    std::array<float*, DeviceMeterTap::MaxChannels + 1> outputs{};
    for (size_t channel = 0; channel < outputStorage.size(); ++channel)
    {
        outputStorage[channel].fill(0.5f);
        outputs[channel] = outputStorage[channel].data();
    }

    juce::AudioIODeviceCallbackContext context;
    tap.audioDeviceIOCallbackWithContext(nullptr, 0, outputs.data(), static_cast<int>(outputs.size()), 4, context);

    REQUIRE(tap.getNumOutputChannels() == DeviceMeterTap::MaxChannels);
    for (const auto& channel : outputStorage)
        for (float sample : channel)
            REQUIRE(sample == 0.0f);
}

TEST_CASE("SafetyLimiter publishes explicit meter semantics from device buffers", "[meter][safety-limiter]")
{
    SafetyLimiterProcessor limiter;
    limiter.prepareToPlay(48000.0, 4);

    std::array<float, 4> input = {0.2f, -0.4f, 1.03f, 0.2f};
    std::array<float, 4> output = {0.1f, -0.6f, 0.3f, 0.2f};
    const float* inputs[] = {input.data()};
    const float* outputs[] = {output.data()};

    limiter.updateInputLevelsFromDevice(inputs, 1, static_cast<int>(input.size()));
    limiter.updateOutputLevelsFromDevice(outputs, 1, static_cast<int>(output.size()));

    REQUIRE(limiter.getInputLevel(0) == Approx(1.03f).margin(0.001f));
    REQUIRE(limiter.getInputRmsLevel(0) > 0.0f);
    REQUIRE(limiter.getInputVuLevel(0) > 0.0f);
    REQUIRE(limiter.getInputAndClearClip(0));
    REQUIRE_FALSE(limiter.getInputClip(0));

    REQUIRE(limiter.getOutputLevel(0) == Approx(0.6f).margin(0.001f));
    REQUIRE(limiter.getOutputRmsLevel(0) > 0.0f);
    REQUIRE(limiter.getOutputVuLevel(0) > 0.0f);
    REQUIRE_FALSE(limiter.getOutputAndClearClip(0));
}
