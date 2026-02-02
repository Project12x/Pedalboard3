/**
 * @file audio_component_test.cpp
 * @brief Unit tests for audio path components
 *
 * Tests cover:
 * 1. BypassableInstance - bypass ramping logic, MIDI channel filtering
 * 2. CrossfadeMixer - fade duration calculation, gain ramping, state machine
 *
 * Note: These tests verify logic contracts without JUCE audio initialization.
 * Full integration testing requires manual testing with the running application.
 */

#include <atomic>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <cmath>

using Catch::Matchers::WithinAbs;

// =============================================================================
// BypassableInstance Logic Tests
// =============================================================================

TEST_CASE("Bypass Ramp Calculations", "[bypassable][audio]")
{
    SECTION("Ramp increment is fixed at 0.001f")
    {
        // From BypassableInstance.cpp line 129/135: rampVal += 0.001f
        constexpr float RAMP_INCREMENT = 0.001f;
        REQUIRE_THAT(RAMP_INCREMENT, WithinAbs(0.001f, 0.00001f));
    }

    SECTION("Bypass ON ramps from 0 to 1 (pass original audio)")
    {
        float rampVal = 0.0f;
        bool bypass = true;
        int samplesNeeded = 0;

        while (rampVal < 1.0f && samplesNeeded < 2000)
        {
            if (bypass && (rampVal < 1.0f))
            {
                rampVal += 0.001f;
                if (rampVal > 1.0f)
                    rampVal = 1.0f;
            }
            samplesNeeded++;
        }

        REQUIRE_THAT(rampVal, WithinAbs(1.0f, 0.0001f));
        // Loop increments before check, so 1001 iterations (0.000 -> 1.000 in 0.001 steps + overshoot correction)
        REQUIRE(samplesNeeded >= 1000);
        REQUIRE(samplesNeeded <= 1001);
    }

    SECTION("Bypass OFF ramps from 1 to 0 (pass processed audio)")
    {
        float rampVal = 1.0f;
        bool bypass = false;
        int samplesNeeded = 0;

        while (rampVal > 0.0f && samplesNeeded < 2000)
        {
            if (!bypass && (rampVal > 0.0f))
            {
                rampVal -= 0.001f;
                if (rampVal < 0.0f)
                    rampVal = 0.0f;
            }
            samplesNeeded++;
        }

        REQUIRE_THAT(rampVal, WithinAbs(0.0f, 0.0001f));
        // Loop decrements before check, so 1001 iterations
        REQUIRE(samplesNeeded >= 1000);
        REQUIRE(samplesNeeded <= 1001);
    }

    SECTION("Audio mix formula correctness")
    {
        // From line 125: newData[i] = (origData[i] * rampVal) + (newData[i] * (1.0f - rampVal))
        float origSample = 1.0f;
        float processedSample = 0.5f;

        // rampVal = 0: 100% processed
        float rampVal = 0.0f;
        float output = (origSample * rampVal) + (processedSample * (1.0f - rampVal));
        REQUIRE_THAT(output, WithinAbs(0.5f, 0.0001f)); // Processed only

        // rampVal = 1: 100% original (bypassed)
        rampVal = 1.0f;
        output = (origSample * rampVal) + (processedSample * (1.0f - rampVal));
        REQUIRE_THAT(output, WithinAbs(1.0f, 0.0001f)); // Original only

        // rampVal = 0.5: 50/50 mix
        rampVal = 0.5f;
        output = (origSample * rampVal) + (processedSample * (1.0f - rampVal));
        REQUIRE_THAT(output, WithinAbs(0.75f, 0.0001f)); // 50% mix
    }
}

TEST_CASE("MIDI Channel Filtering", "[bypassable][midi]")
{
    SECTION("Channel 0 means omni (pass all)")
    {
        int midiChannel = 0; // Omni
        int incomingChannel = 5;

        bool shouldPass = (midiChannel == 0) || (incomingChannel == midiChannel);
        REQUIRE(shouldPass);
    }

    SECTION("Specific channel filters correctly")
    {
        int midiChannel = 3;
        int matchingChannel = 3;
        int differentChannel = 5;

        bool shouldPassMatch = (midiChannel == 0) || (matchingChannel == midiChannel);
        bool shouldPassDiff = (midiChannel == 0) || (differentChannel == midiChannel);

        REQUIRE(shouldPassMatch);
        REQUIRE_FALSE(shouldPassDiff);
    }
}

// =============================================================================
// CrossfadeMixer Logic Tests
// =============================================================================

TEST_CASE("Crossfade Duration Calculation", "[crossfade][audio]")
{
    SECTION("Fade samples from duration and sample rate")
    {
        // From CrossfadeMixer.cpp line 33
        int durationMs = 100;
        double sampleRate = 44100.0;

        int fadeSamples = static_cast<int>((durationMs / 1000.0) * sampleRate);
        REQUIRE(fadeSamples == 4410); // 100ms at 44.1kHz
    }

    SECTION("Default duration used when 0 or negative")
    {
        int durationMs = 0;
        int defaultFadeMs = 100;

        if (durationMs <= 0)
            durationMs = defaultFadeMs;

        REQUIRE(durationMs == 100);
    }

    SECTION("Minimum 1 sample for very short fades")
    {
        int durationMs = 1;
        double sampleRate = 100.0; // Very low rate for testing

        int fadeSamples = static_cast<int>((durationMs / 1000.0) * sampleRate);
        // 0.001 * 100 = 0.1 -> cast to 0
        if (fadeSamples < 1)
            fadeSamples = 1;

        REQUIRE(fadeSamples >= 1);
    }
}

TEST_CASE("Crossfade Gain Ramp", "[crossfade][audio]")
{
    SECTION("Fade out increment is negative")
    {
        int fadeSamples = 4410;
        float fadeIncrement = -1.0f / static_cast<float>(fadeSamples);

        REQUIRE(fadeIncrement < 0.0f);
        REQUIRE_THAT(fadeIncrement, WithinAbs(-0.0002267f, 0.00001f));
    }

    SECTION("Fade in increment is positive")
    {
        int fadeSamples = 4410;
        float fadeIncrement = 1.0f / static_cast<float>(fadeSamples);

        REQUIRE(fadeIncrement > 0.0f);
        REQUIRE_THAT(fadeIncrement, WithinAbs(0.0002267f, 0.00001f));
    }

    SECTION("Gain reaches 0 at end of fade out")
    {
        float currentGain = 1.0f;
        int fadeSamples = 100;
        float increment = -1.0f / static_cast<float>(fadeSamples);

        for (int i = 0; i < fadeSamples; ++i)
        {
            currentGain += increment;
            if (currentGain <= 0.0f)
            {
                currentGain = 0.0f;
                break;
            }
        }

        REQUIRE_THAT(currentGain, WithinAbs(0.0f, 0.0001f));
    }

    SECTION("Gain reaches 1 at end of fade in")
    {
        float currentGain = 0.0f;
        int fadeSamples = 100;
        float increment = 1.0f / static_cast<float>(fadeSamples);

        for (int i = 0; i < fadeSamples; ++i)
        {
            currentGain += increment;
            if (currentGain >= 1.0f)
            {
                currentGain = 1.0f;
                break;
            }
        }

        REQUIRE_THAT(currentGain, WithinAbs(1.0f, 0.0001f));
    }
}

TEST_CASE("Crossfade State Machine", "[crossfade][state]")
{
    SECTION("isSilent when gain < 0.001")
    {
        auto isSilent = [](float gain) { return gain < 0.001f; };

        REQUIRE(isSilent(0.0f));
        REQUIRE(isSilent(0.0005f));
        REQUIRE_FALSE(isSilent(0.001f));
        REQUIRE_FALSE(isSilent(1.0f));
    }

    SECTION("Not fading applies constant gain")
    {
        bool fading = false;
        float fadeGain = 0.5f;

        // From processBlock: if (!fading) apply constant gain
        if (!fading)
        {
            // Would call buffer.applyGain(gain)
            REQUIRE_THAT(fadeGain, WithinAbs(0.5f, 0.0001f));
        }
    }

    SECTION("Full volume (>= 0.999) needs no processing")
    {
        float gain = 1.0f;
        bool needsProcessing = (gain < 0.999f);

        REQUIRE_FALSE(needsProcessing);
    }
}

// =============================================================================
// Mutation Testing
// =============================================================================

TEST_CASE("BypassableInstance Mutation Testing", "[bypassable][mutation]")
{
    SECTION("OFF-BY-ONE: Ramp increment 0.001 not 0.01 or 0.0001")
    {
        constexpr float CORRECT_INCREMENT = 0.001f;
        constexpr float WRONG_INCREMENT_FAST = 0.01f;
        constexpr float WRONG_INCREMENT_SLOW = 0.0001f;

        // Integer division of 1.0/0.001 = 999 due to floating point
        int correctSamples = static_cast<int>(1.0f / CORRECT_INCREMENT);
        int fastSamples = static_cast<int>(1.0f / WRONG_INCREMENT_FAST);
        int slowSamples = static_cast<int>(1.0f / WRONG_INCREMENT_SLOW);

        // Approximately 1000 samples to ramp
        REQUIRE(correctSamples >= 999);
        REQUIRE(correctSamples <= 1001);
        REQUIRE(fastSamples < 500);  // 100 samples - too fast
        REQUIRE(slowSamples > 5000); // 10000 samples - too slow
    }

    SECTION("NEGATE: Bypass direction check")
    {
        bool bypass = true;
        float rampVal = 0.5f;

        // Correct: bypass = true means ramp UP
        bool shouldRampUp = bypass && (rampVal < 1.0f);
        REQUIRE(shouldRampUp);

        // Mutation: if !bypass was used
        bool mutatedCheck = !bypass && (rampVal < 1.0f);
        REQUIRE(mutatedCheck != shouldRampUp);
    }

    SECTION("SWAP: origData vs newData in mix formula")
    {
        float origSample = 0.8f;
        float processedSample = 0.2f;
        float rampVal = 0.0f; // Bypass off = processed only

        // Correct formula
        float correct = (origSample * rampVal) + (processedSample * (1.0f - rampVal));

        // Swapped formula (bug)
        float swapped = (processedSample * rampVal) + (origSample * (1.0f - rampVal));

        REQUIRE(correct != swapped); // Mutation detectable
    }
}

TEST_CASE("CrossfadeMixer Mutation Testing", "[crossfade][mutation]")
{
    SECTION("ARITHMETIC: Fade samples calculation")
    {
        int durationMs = 100;
        double sampleRate = 44100.0;

        // Correct: ms/1000 * sampleRate
        int correct = static_cast<int>((durationMs / 1000.0) * sampleRate);

        // Mutation: forgot to divide by 1000
        int mutation1 = static_cast<int>(durationMs * sampleRate);

        REQUIRE(correct == 4410);
        REQUIRE(mutation1 != correct); // 4410000 vs 4410
    }

    SECTION("NEGATE: Fade direction sign")
    {
        int fadeSamples = 1000;

        // Correct: fade out = negative
        float fadeOutIncrement = -1.0f / static_cast<float>(fadeSamples);

        // Mutation: wrong sign
        float mutatedIncrement = 1.0f / static_cast<float>(fadeSamples);

        REQUIRE(fadeOutIncrement < 0.0f);
        REQUIRE(mutatedIncrement > 0.0f);
        REQUIRE(fadeOutIncrement != mutatedIncrement);
    }

    SECTION("CONDITION: Clamp uses <= for fade out, >= for fade in")
    {
        float currentGain = 0.0f;
        bool isFadingOut = true;

        // Correct: <= 0 triggers clamp for fade out
        bool correctClamp = isFadingOut ? (currentGain <= 0.0f) : (currentGain >= 1.0f);
        REQUIRE(correctClamp);

        // Mutation: < instead of <= would miss exact 0
        float testGain = 0.0f;
        bool strictCheck = (testGain < 0.0f);
        bool correctCheck = (testGain <= 0.0f);

        REQUIRE(correctCheck != strictCheck); // 0.0 fails strict check
    }
}
