#include "../src/dsp/ReverbSC.h"
#include "../src/ReverbSCProcessor.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cmath>
#include <vector>

namespace
{
bool isFiniteBuffer(const std::vector<float>& buffer)
{
    return std::all_of(buffer.begin(), buffer.end(), [](const float sample) { return std::isfinite(sample); });
}

float maxAbs(const std::vector<float>& buffer)
{
    float maximum = 0.0f;
    for (const auto sample : buffer)
        maximum = std::max(maximum, std::abs(sample));
    return maximum;
}
} // namespace

TEST_CASE("ReverbSC core keeps silence silent", "[reverbsc][dsp]")
{
    pedalboard3::dsp::ReverbSC reverb;
    reverb.prepare(48000.0, 64);

    std::vector<float> inL(64, 0.0f);
    std::vector<float> inR(64, 0.0f);
    std::vector<float> outL(64, 1.0f);
    std::vector<float> outR(64, 1.0f);

    reverb.process(inL.data(), inR.data(), outL.data(), outR.data(), 64);

    for (int i = 0; i < 64; ++i)
    {
        REQUIRE(outL[static_cast<size_t>(i)] == 0.0f);
        REQUIRE(outR[static_cast<size_t>(i)] == 0.0f);
    }
}

TEST_CASE("ReverbSC core produces a finite stereo impulse tail", "[reverbsc][dsp]")
{
    pedalboard3::dsp::ReverbSC reverb;
    reverb.prepare(48000.0, 512);
    reverb.setFeedback(0.97f);
    reverb.setDampingHz(10000.0f);

    constexpr int totalSamples = 48000;
    std::vector<float> inL(totalSamples, 0.0f);
    std::vector<float> inR(totalSamples, 0.0f);
    std::vector<float> outL(totalSamples, 0.0f);
    std::vector<float> outR(totalSamples, 0.0f);

    inL[0] = 1.0f;
    inR[0] = 1.0f;

    for (int offset = 0; offset < totalSamples; offset += 512)
    {
        const int blockSize = std::min(512, totalSamples - offset);
        reverb.process(inL.data() + offset, inR.data() + offset, outL.data() + offset, outR.data() + offset, blockSize);
    }

    REQUIRE(isFiniteBuffer(outL));
    REQUIRE(isFiniteBuffer(outR));
    REQUIRE(maxAbs(outL) > 0.001f);
    REQUIRE(maxAbs(outR) > 0.001f);
    REQUIRE(maxAbs(outL) <= 2.0f);
    REQUIRE(maxAbs(outR) <= 2.0f);
}

TEST_CASE("ReverbSC core remains finite across sample rates and block sizes", "[reverbsc][dsp]")
{
    const double sampleRates[] = {44100.0, 48000.0, 96000.0, 192000.0};
    const int blockSizes[] = {1, 17, 256};

    for (const auto sampleRate : sampleRates)
    {
        for (const auto blockSize : blockSizes)
        {
            pedalboard3::dsp::ReverbSC reverb;
            reverb.prepare(sampleRate, blockSize);
            reverb.setFeedback(0.99f);
            reverb.setDampingHz(20000.0f);

            std::vector<float> inL(static_cast<size_t>(blockSize), 0.0f);
            std::vector<float> inR(static_cast<size_t>(blockSize), 0.0f);
            std::vector<float> outL(static_cast<size_t>(blockSize), 0.0f);
            std::vector<float> outR(static_cast<size_t>(blockSize), 0.0f);

            inL[0] = 0.5f;
            inR[0] = -0.5f;

            for (int i = 0; i < 1000; ++i)
                reverb.process(inL.data(), inR.data(), outL.data(), outR.data(), blockSize);

            REQUIRE(isFiniteBuffer(outL));
            REQUIRE(isFiniteBuffer(outR));
            REQUIRE(maxAbs(outL) <= 2.0f);
            REQUIRE(maxAbs(outR) <= 2.0f);
        }
    }
}

TEST_CASE("ReverbSC core clamps feedback and damping parameters", "[reverbsc][dsp]")
{
    pedalboard3::dsp::ReverbSC reverb;
    reverb.prepare(48000.0, 64);

    reverb.setFeedback(-1.0f);
    REQUIRE(reverb.getFeedback() == 0.0f);
    reverb.setFeedback(2.0f);
    REQUIRE(reverb.getFeedback() == 0.99f);

    reverb.setDampingHz(-10.0f);
    REQUIRE(reverb.getDampingHz() == 20.0f);
    reverb.setDampingHz(40000.0f);
    REQUIRE(reverb.getDampingHz() == 20000.0f);
}

TEST_CASE("ReverbSCProcessor exposes stable host metadata", "[reverbsc][processor]")
{
    ReverbSCProcessor processor;

    REQUIRE(processor.getName() == "ReverbSC");
    REQUIRE_FALSE(processor.acceptsMidi());
    REQUIRE_FALSE(processor.producesMidi());
    REQUIRE(processor.getNumParameters() == ReverbSCProcessor::NumParameters);
    REQUIRE(processor.getParameterName(ReverbSCProcessor::MixParam) == "Mix");
    REQUIRE(processor.getParameterName(ReverbSCProcessor::FeedbackParam) == "Feedback");
    REQUIRE(processor.getParameterName(ReverbSCProcessor::DampingParam) == "Damping");
    REQUIRE(processor.getParameterName(ReverbSCProcessor::WidthParam) == "Width");
    REQUIRE(processor.getParameterName(ReverbSCProcessor::OutputParam) == "Output");
}

TEST_CASE("ReverbSCProcessor state round-trips parameters", "[reverbsc][processor]")
{
    ReverbSCProcessor source;
    source.setParameter(ReverbSCProcessor::MixParam, 0.25f);
    source.setParameter(ReverbSCProcessor::FeedbackParam, 0.75f);
    source.setParameter(ReverbSCProcessor::DampingParam, 0.5f);
    source.setParameter(ReverbSCProcessor::WidthParam, 0.4f);
    source.setParameter(ReverbSCProcessor::OutputParam, 0.8f);

    juce::MemoryBlock state;
    source.getStateInformation(state);

    ReverbSCProcessor restored;
    restored.setStateInformation(state.getData(), static_cast<int>(state.getSize()));

    REQUIRE(restored.getParameter(ReverbSCProcessor::MixParam) == Catch::Approx(0.25f));
    REQUIRE(restored.getParameter(ReverbSCProcessor::FeedbackParam) == Catch::Approx(0.75f));
    REQUIRE(restored.getParameter(ReverbSCProcessor::DampingParam) == Catch::Approx(0.5f));
    REQUIRE(restored.getParameter(ReverbSCProcessor::WidthParam) == Catch::Approx(0.4f));
    REQUIRE(restored.getParameter(ReverbSCProcessor::OutputParam) == Catch::Approx(0.8f));
}

TEST_CASE("ReverbSCProcessor processes a finite stereo impulse", "[reverbsc][processor]")
{
    ReverbSCProcessor processor;
    processor.prepareToPlay(48000.0, 4096);
    processor.setParameter(ReverbSCProcessor::MixParam, 1.0f);
    processor.setParameter(ReverbSCProcessor::FeedbackParam, 0.9f);
    processor.setParameter(ReverbSCProcessor::DampingParam, 0.7f);
    processor.setParameter(ReverbSCProcessor::WidthParam, 1.0f);
    processor.setParameter(ReverbSCProcessor::OutputParam, 0.5f);

    juce::AudioBuffer<float> buffer(2, 4096);
    juce::MidiBuffer midi;
    buffer.clear();
    buffer.setSample(0, 0, 1.0f);
    buffer.setSample(1, 0, 1.0f);

    processor.processBlock(buffer, midi);

    std::vector<float> left(static_cast<size_t>(buffer.getNumSamples()));
    std::vector<float> right(static_cast<size_t>(buffer.getNumSamples()));
    std::copy(buffer.getReadPointer(0), buffer.getReadPointer(0) + buffer.getNumSamples(), left.begin());
    std::copy(buffer.getReadPointer(1), buffer.getReadPointer(1) + buffer.getNumSamples(), right.begin());

    REQUIRE(isFiniteBuffer(left));
    REQUIRE(isFiniteBuffer(right));
    REQUIRE(maxAbs(left) > 0.0001f);
    REQUIRE(maxAbs(right) > 0.0001f);
    REQUIRE(maxAbs(left) <= 2.0f);
    REQUIRE(maxAbs(right) <= 2.0f);
}
