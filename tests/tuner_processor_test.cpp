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

void driveSineBlocks(TunerProcessor& tuner,
                     AudioSampleBuffer& buffer,
                     MidiBuffer& midi,
                     double& phase,
                     double sampleRate,
                     double frequency,
                     int numBlocks)
{
    for (int block = 0; block < numBlocks; ++block)
    {
        fillSineBlock(buffer, phase, sampleRate, frequency);
        tuner.processBlock(buffer, midi);
    }
}

bool waitForDetectedPitch(TunerProcessor& tuner, int attempts = 200)
{
    for (int attempt = 0; attempt < attempts && !tuner.isPitchDetected(); ++attempt)
        std::this_thread::sleep_for(std::chrono::milliseconds(5));

    return tuner.isPitchDetected();
}
} // namespace

TEST_CASE("TunerProcessor publishes background analysis while preserving pass-through and mute",
          "[tuner][processor]")
{
    constexpr double sampleRate = 48000.0;
    constexpr int blockSize = 128;
    constexpr double testFrequencyHz = 440.0;

    TunerProcessor tuner;
    CHECK(tuner.getReferenceA4Hz() == Catch::Approx(440.0f));
    CHECK(tuner.getResponseMode() == TunerProcessor::ResponseMode::Stable);
    tuner.setResponseMode(TunerProcessor::ResponseMode::Fast);
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

    REQUIRE(waitForDetectedPitch(tuner));
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

TEST_CASE("TunerProcessor applies reference pitch and serializes tuner response state",
          "[tuner][processor]")
{
    constexpr double sampleRate = 48000.0;
    constexpr int blockSize = 128;
    constexpr double referenceA4 = 432.0;

    TunerProcessor tuner;
    tuner.setReferenceA4Hz(static_cast<float>(referenceA4));
    tuner.setResponseMode(TunerProcessor::ResponseMode::Fast);
    CHECK(tuner.getReferenceA4Hz() == Catch::Approx(referenceA4));
    CHECK(tuner.getResponseMode() == TunerProcessor::ResponseMode::Fast);

    tuner.prepareToPlay(sampleRate, blockSize);

    AudioSampleBuffer buffer(1, blockSize);
    MidiBuffer midi;
    double phase = 0.0;
    driveSineBlocks(tuner, buffer, midi, phase, sampleRate, referenceA4, 90);

    REQUIRE(waitForDetectedPitch(tuner));
    CHECK(tuner.getDetectedNote() == 69);
    CHECK(tuner.getDetectedFrequency() == Catch::Approx(referenceA4).margin(0.75f));
    CHECK(tuner.getCentsDeviation() == Catch::Approx(0.0f).margin(3.0f));

    MemoryBlock state;
    tuner.getStateInformation(state);
    tuner.releaseResources();

    TunerProcessor restored;
    restored.setReferenceA4Hz(440.0f);
    restored.setResponseMode(TunerProcessor::ResponseMode::Stable);
    restored.setStateInformation(state.getData(), static_cast<int>(state.getSize()));

    CHECK(restored.getReferenceA4Hz() == Catch::Approx(referenceA4));
    CHECK(restored.getResponseMode() == TunerProcessor::ResponseMode::Fast);
}
