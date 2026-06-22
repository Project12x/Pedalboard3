#include "../src/dsp/TunerAnalysis.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <cstdint>
#include <vector>

namespace
{
constexpr double kPi = 3.14159265358979323846;

float midiNoteToFrequency(int midiNote, float referenceA4Hz = 440.0f)
{
    return referenceA4Hz * std::pow(2.0f, static_cast<float>(midiNote - 69) / 12.0f);
}

float centsBetween(float frequency, float target)
{
    return 1200.0f * std::log2(frequency / target);
}

void pushSine(pedalboard3::dsp::TunerAnalysis& analysis,
              double sampleRate,
              float frequency,
              int totalSamples,
              int blockSize,
              float amplitude = 0.5f)
{
    std::vector<float> block(static_cast<size_t>(blockSize), 0.0f);
    int written = 0;
    while (written < totalSamples)
    {
        const int count = std::min(blockSize, totalSamples - written);
        for (int i = 0; i < count; ++i)
        {
            const double phase = 2.0 * kPi * static_cast<double>(frequency) *
                                 static_cast<double>(written + i) / sampleRate;
            block[static_cast<size_t>(i)] = amplitude * static_cast<float>(std::sin(phase));
        }
        analysis.pushSamples(block.data(), count);
        written += count;
    }
}

void pushDecayingSine(pedalboard3::dsp::TunerAnalysis& analysis,
                      double sampleRate,
                      float frequency,
                      int totalSamples,
                      int blockSize)
{
    std::vector<float> block(static_cast<size_t>(blockSize), 0.0f);
    int written = 0;
    while (written < totalSamples)
    {
        const int count = std::min(blockSize, totalSamples - written);
        for (int i = 0; i < count; ++i)
        {
            const float t = static_cast<float>(written + i) / static_cast<float>(std::max(1, totalSamples - 1));
            const float tail = 1.0f - t;
            const float amplitude = 0.5f * tail * tail * tail * tail * tail * tail;
            const double phase = 2.0 * kPi * static_cast<double>(frequency) *
                                 static_cast<double>(written + i) / sampleRate;
            block[static_cast<size_t>(i)] = amplitude * static_cast<float>(std::sin(phase));
        }
        analysis.pushSamples(block.data(), count);
        written += count;
    }
}

void pushWhiteNoise(pedalboard3::dsp::TunerAnalysis& analysis, int totalSamples, int blockSize)
{
    std::vector<float> block(static_cast<size_t>(blockSize), 0.0f);
    uint32_t state = 0x12345678u;
    int written = 0;
    while (written < totalSamples)
    {
        const int count = std::min(blockSize, totalSamples - written);
        for (int i = 0; i < count; ++i)
        {
            state = state * 1664525u + 1013904223u;
            const float value = static_cast<float>((state >> 8) & 0xFFFFu) / 32767.5f - 1.0f;
            block[static_cast<size_t>(i)] = value * 0.25f;
        }
        analysis.pushSamples(block.data(), count);
        written += count;
    }
}
} // namespace

TEST_CASE("TunerAnalysis detects generated musical references", "[tuner][analysis]")
{
    struct PitchCase
    {
        int midiNote;
        const char* name;
    };

    const double sampleRates[] = {44100.0, 48000.0, 96000.0};
    const PitchCase pitches[] = {
        {40, "E2"},
        {45, "A2"},
        {69, "A4"},
        {72, "C5"},
    };

    for (const auto sampleRate : sampleRates)
    {
        for (const auto& pitch : pitches)
        {
            INFO("sampleRate=" << sampleRate << " pitch=" << pitch.name);
            pedalboard3::dsp::TunerAnalysis analysis;
            analysis.prepare(sampleRate, 1024);

            const float frequency = midiNoteToFrequency(pitch.midiNote);
            pushSine(analysis, sampleRate, frequency, 12000, 257);

            const auto result = analysis.analyze();

            REQUIRE(result.detected);
            REQUIRE(result.state == pedalboard3::dsp::TunerSignalState::Stable);
            REQUIRE(result.midiNote == pitch.midiNote);
            REQUIRE(std::abs(result.cents) < 3.5f);
            REQUIRE(result.frequencyHz == Catch::Approx(frequency).margin(std::max(0.3f, frequency * 0.004f)));
            REQUIRE(result.confidence > 0.75f);
            REQUIRE(result.referenceA4Hz == Catch::Approx(440.0f));
        }
    }
}

TEST_CASE("TunerAnalysis handles varied push block sizes", "[tuner][analysis]")
{
    const int blockSizes[] = {1, 17, 64, 511, 512, 1024};

    for (const auto blockSize : blockSizes)
    {
        INFO("blockSize=" << blockSize);
        pedalboard3::dsp::TunerAnalysis analysis;
        analysis.prepare(48000.0, blockSize);

        pushSine(analysis, 48000.0, 440.0f, 12000, blockSize);
        const auto result = analysis.analyze();

        REQUIRE(result.detected);
        REQUIRE(result.state == pedalboard3::dsp::TunerSignalState::Stable);
        REQUIRE(result.midiNote == 69);
        REQUIRE(std::abs(result.cents) < 2.5f);
    }
}

TEST_CASE("TunerAnalysis applies configurable A4 reference", "[tuner][analysis]")
{
    pedalboard3::dsp::TunerAnalysis analysis;
    analysis.prepare(48000.0, 512);
    analysis.setReferenceA4Hz(442.0f);

    pushSine(analysis, 48000.0, 442.0f, 12000, 512);
    auto result = analysis.analyze();
    REQUIRE(result.detected);
    REQUIRE(result.midiNote == 69);
    REQUIRE(std::abs(result.cents) < 2.0f);
    REQUIRE(result.referenceA4Hz == Catch::Approx(442.0f));

    analysis.reset();
    analysis.setReferenceA4Hz(442.0f);
    pushSine(analysis, 48000.0, 440.0f, 12000, 512);
    result = analysis.analyze();
    REQUIRE(result.detected);
    REQUIRE(result.midiNote == 69);
    REQUIRE(result.cents == Catch::Approx(centsBetween(440.0f, 442.0f)).margin(2.0f));
}

TEST_CASE("TunerAnalysis rejects silence weak signal noise and decays", "[tuner][analysis]")
{
    pedalboard3::dsp::TunerAnalysis analysis;
    analysis.prepare(48000.0, 512);

    std::vector<float> silence(4096, 0.0f);
    analysis.pushSamples(silence.data(), static_cast<int>(silence.size()));
    auto result = analysis.analyze();
    REQUIRE_FALSE(result.detected);
    REQUIRE(result.state == pedalboard3::dsp::TunerSignalState::NoSignal);
    REQUIRE(result.confidence == 0.0f);

    analysis.reset();
    pushSine(analysis, 48000.0, 440.0f, 12000, 512, 0.0001f);
    result = analysis.analyze();
    REQUIRE_FALSE(result.detected);
    REQUIRE(result.state == pedalboard3::dsp::TunerSignalState::NoSignal);

    analysis.reset();
    pushWhiteNoise(analysis, 12000, 512);
    result = analysis.analyze();
    REQUIRE_FALSE(result.detected);
    REQUIRE(result.state != pedalboard3::dsp::TunerSignalState::Stable);
    REQUIRE(result.confidence < 0.6f);

    analysis.reset();
    pushDecayingSine(analysis, 48000.0, 440.0f, 12000, 512);
    result = analysis.analyze();
    REQUIRE_FALSE(result.detected);
    REQUIRE(result.state == pedalboard3::dsp::TunerSignalState::NoSignal);
}

TEST_CASE("TunerAnalysis reset clears previous result", "[tuner][analysis]")
{
    pedalboard3::dsp::TunerAnalysis analysis;
    analysis.prepare(48000.0, 512);

    pushSine(analysis, 48000.0, 440.0f, 12000, 512);
    REQUIRE(analysis.analyze().detected);

    analysis.reset();
    const auto result = analysis.getLastResult();
    REQUIRE_FALSE(result.detected);
    REQUIRE(result.state == pedalboard3::dsp::TunerSignalState::NoSignal);
    REQUIRE(result.midiNote == -1);
    REQUIRE(result.frequencyHz == 0.0f);
}
