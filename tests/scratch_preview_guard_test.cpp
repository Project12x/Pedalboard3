#include "SafetyLimiter.h"
#include "ScratchPreviewGuard.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <cmath>
#include <limits>

TEST_CASE("Scratch limiter extraction preserves existing impulse and linked overload response", "[scratch][scratch-safety]")
{
    SafetyLimiterProcessor limiter;
    limiter.prepareToPlay(48000, 64);
    juce::AudioBuffer<float> audio(2, 64);
    juce::MidiBuffer midi;
    audio.clear();
    SECTION("normal impulse and DC blocker decay")
    {
        audio.setSample(0, 0, 0.25f); audio.setSample(1, 0, 0.125f);
        limiter.processBlock(audio, midi);
        REQUIRE(audio.getSample(0, 0) == 0.25f);
        REQUIRE(audio.getSample(0, 1) == Catch::Approx(-0.00125f).margin(1e-7));
        REQUIRE(audio.getSample(0, 63) == Catch::Approx(-0.00125f * std::pow(0.995f, 62)).margin(1e-7));
        REQUIRE(audio.getSample(1, 63) == Catch::Approx(audio.getSample(0, 63) * 0.5f));
        REQUIRE_FALSE(limiter.isLimiting());
        REQUIRE_FALSE(limiter.isMuted());
    }
    SECTION("linked soft reduction and hard ceiling")
    {
        audio.setSample(0, 0, 2.0f); audio.setSample(1, 0, 0.5f);
        limiter.processBlock(audio, midi);
        REQUIRE(audio.getSample(0, 0) == 1.0f);
        REQUIRE(audio.getSample(1, 0) == Catch::Approx(0.36440467f).margin(1e-6));
        REQUIRE(audio.getSample(0, 1) == Catch::Approx(-0.007289226f).margin(1e-6));
        REQUIRE(limiter.isLimiting());
        REQUIRE_FALSE(limiter.isMuted());
    }
    SECTION("non-finite trip and prepare retain mute")
    {
        audio.setSample(1, 20, std::numeric_limits<float>::infinity());
        limiter.processBlock(audio, midi);
        REQUIRE(limiter.isMuted());
        REQUIRE(audio.getMagnitude(0, 64) == 0);
        limiter.prepareToPlay(96000, 64);
        REQUIRE(limiter.isMuted());
    }
    SafetyLimiterProcessor::setInstance(nullptr);
}

TEST_CASE("Scratch preview guard does not steal the limiter instance", "[scratch][scratch-safety]")
{
    SafetyLimiterProcessor limiter;
    limiter.prepareToPlay(48000, 128);
    auto* graphLimiter = SafetyLimiterProcessor::getInstance();
    juce::AudioBuffer<float> audio(16, 128);
    for (int ch = 0; ch < 16; ++ch) for (int i = 0; i < 128; ++i) audio.setSample(ch, i, 0.25f);
    {
        ScratchPreviewGuard guard;
        guard.prepare(48000);
        REQUIRE(SafetyLimiterProcessor::getInstance() == graphLimiter);
        REQUIRE_FALSE(guard.process(audio.getArrayOfWritePointers(), 16, 128, true));
        for (int ch = 0; ch < 16; ++ch) REQUIRE(audio.getMagnitude(ch, 0, 128) == 0);
    }
    REQUIRE(SafetyLimiterProcessor::getInstance() == graphLimiter);
    SafetyLimiterProcessor::setInstance(nullptr);
}

TEST_CASE("Scratch preview protection covers channel sixteen and latches across preparation", "[scratch][scratch-safety]")
{
    ScratchPreviewGuard guard;
    guard.prepare(48000);
    juce::AudioBuffer<float> audio(16, 128);
    audio.clear(); audio.setSample(15, 10, std::numeric_limits<float>::quiet_NaN());
    REQUIRE(guard.process(audio.getArrayOfWritePointers(), 16, 128, false));
    for (int ch = 0; ch < 16; ++ch) REQUIRE(audio.getMagnitude(ch, 0, 128) == 0);
    guard.prepare(96000);
    audio.setSample(0, 0, 0.25f);
    REQUIRE_FALSE(guard.process(audio.getArrayOfWritePointers(), 16, 128, false));
    REQUIRE(audio.getMagnitude(0, 128) == 0);
    guard.reset();
    audio.setSample(0, 0, 0.25f);
    REQUIRE_FALSE(guard.process(audio.getArrayOfWritePointers(), 16, 128, false));
    REQUIRE(audio.getSample(0, 0) == 0.25f);
}

TEST_CASE("Scratch preview linked ceiling and finite checks protect every output", "[scratch][scratch-safety]")
{
    ScratchPreviewGuard guard;
    guard.prepare(48000);
    juce::AudioBuffer<float> audio(16, 128);
    audio.clear();
    for (int ch = 0; ch < 15; ++ch) audio.setSample(ch, 0, 0.5f);
    audio.setSample(15, 0, 2.0f);
    REQUIRE_FALSE(guard.process(audio.getArrayOfWritePointers(), 16, 128, false));
    for (int ch = 0; ch < 15; ++ch) REQUIRE(audio.getSample(ch, 0) == Catch::Approx(0.36440467f).margin(1e-6));
    REQUIRE(audio.getSample(15, 0) == 1);
    for (int ch = 0; ch < 16; ++ch) for (int i = 0; i < 128; ++i)
    {
        REQUIRE(std::isfinite(audio.getSample(ch, i)));
        REQUIRE(std::abs(audio.getSample(ch, i)) <= 1.0f);
    }
    guard.reset(); audio.clear();
    audio.setSample(15, 0, std::numeric_limits<float>::max());
    audio.setSample(15, 1, -std::numeric_limits<float>::max());
    REQUIRE(guard.process(audio.getArrayOfWritePointers(), 16, 128, false));
    for (int ch = 0; ch < 16; ++ch) REQUIRE(audio.getMagnitude(ch, 0, 128) == 0);
}

TEST_CASE("Scratch safety mute wins over an earlier unmute request", "[scratch][scratch-safety][rt]")
{
    SafetyLimiterProcessor limiter;
    limiter.prepareToPlay(48000, 128);
    limiter.requestMute();
    limiter.unmute();
    REQUIRE(limiter.isMuted()); // unmute is consumed by processing, never resets DSP from UI
    limiter.requestMute();
    juce::AudioBuffer<float> audio(2, 128); audio.clear(); audio.setSample(0, 0, 0.25f);
    juce::MidiBuffer midi;
    limiter.processBlock(audio, midi);
    REQUIRE(limiter.isMuted());
    REQUIRE(audio.getMagnitude(0, 128) == 0);
    const auto serial = limiter.getResetSerial();
    limiter.unmute();
    limiter.processBlock(audio, midi);
    REQUIRE_FALSE(limiter.isMuted());
    REQUIRE(limiter.getResetSerial() == serial + 1);
    SafetyLimiterProcessor::setInstance(nullptr);
}
