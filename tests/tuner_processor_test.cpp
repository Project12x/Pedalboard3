#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "TunerProcessor.h"

#include <chrono>
#include <cmath>
#include <thread>

namespace
{
constexpr double kPi = 3.14159265358979323846;

void fillSineBlock(AudioSampleBuffer& buffer, double& phase, double sampleRate, double frequency)
{
    const auto phaseDelta = 2.0 * kPi * frequency / sampleRate;
    auto* samples = buffer.getWritePointer(0);

    for (int i = 0; i < buffer.getNumSamples(); ++i)
    {
        samples[i] = static_cast<float>(0.25 * std::sin(phase));
        phase += phaseDelta;
        if (phase >= 2.0 * kPi)
            phase -= 2.0 * kPi;
    }
}
} // namespace

TEST_CASE("TunerProcessor publishes background analysis while preserving pass-through and mute",
          "[tuner][processor]")
{
    constexpr double sampleRate = 48000.0;
    constexpr int blockSize = 128;
    constexpr double testFrequencyHz = 440.0;

    TunerProcessor tuner;
    tuner.prepareToPlay(sampleRate, blockSize);

    AudioSampleBuffer buffer(1, blockSize);
    MidiBuffer midi;
    double phase = 0.0;

    for (int block = 0; block < 80; ++block)
    {
        fillSineBlock(buffer, phase, sampleRate, testFrequencyHz);
        const auto firstSample = buffer.getSample(0, 0);
        tuner.processBlock(buffer, midi);
        CHECK(buffer.getSample(0, 0) == Catch::Approx(firstSample).margin(1.0e-6f));
    }

    for (int attempt = 0; attempt < 200 && !tuner.isPitchDetected(); ++attempt)
        std::this_thread::sleep_for(std::chrono::milliseconds(5));

    REQUIRE(tuner.isPitchDetected());
    CHECK(tuner.getDetectedNote() == 69);
    CHECK(tuner.getDetectedFrequency() == Catch::Approx(440.0f).margin(0.75f));
    CHECK(tuner.getCentsDeviation() == Catch::Approx(0.0f).margin(3.0f));

    tuner.setMuteOutput(true);
    buffer.clear();
    for (int i = 0; i < buffer.getNumSamples(); ++i)
        buffer.setSample(0, i, 0.5f);
    tuner.processBlock(buffer, midi);

    for (int i = 0; i < buffer.getNumSamples(); ++i)
        CHECK(buffer.getSample(0, i) == Catch::Approx(0.0f));

    tuner.releaseResources();
}
