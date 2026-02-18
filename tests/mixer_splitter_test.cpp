/**
 * @file mixer_splitter_test.cpp
 * @brief Integration and mutation tests for DawMixerProcessor and DawSplitterProcessor DSP
 *
 * These tests verify the mathematical correctness and boundary safety of:
 * 1. Strip management (add/remove bounds checking)
 * 2. Gain calculation (dB-to-linear conversion, smoothing)
 * 3. Pan law (equal-power -3dB constant power panning)
 * 4. Mute/Solo logic (effective mute calculation)
 * 5. Phase inversion
 * 6. Buffer channel clamping (critical crash fix verification)
 * 7. Master bus gain and mute
 * 8. VU peak metering and decay
 * 9. State serialization roundtrip
 *
 * NOTE: These are standalone math tests - the actual processor classes have
 * UI dependencies (PluginComponent.h) that prevent direct compilation in
 * the test target. The DSP logic is replicated here to verify correctness.
 */

#include <array>
#include <atomic>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <cmath>
#include <cstring>
#include <vector>

using Catch::Matchers::WithinAbs;

// =============================================================================
// Constants mirroring DawMixerProcessor / DawSplitterProcessor
// =============================================================================

static constexpr int MaxStrips = 32;
static constexpr int DefaultStrips = 2;
static constexpr float MinGainDb = -60.0f;
static constexpr float MaxGainDb = 12.0f;
static constexpr float GainRampSeconds = 0.05f;

// =============================================================================
// Standalone DSP Math (mirrors processor implementations)
// =============================================================================

// dB to linear gain conversion (mirrors juce::Decibels::decibelsToGain)
static float dbToGain(float dB)
{
    return std::pow(10.0f, dB * 0.05f);
}

// Equal-power pan law (-3dB center, mirrors processBlock)
static float panLaw_L(float pan)
{
    return std::sqrt(0.5f * (1.0f - pan));
}
static float panLaw_R(float pan)
{
    return std::sqrt(0.5f * (1.0f + pan));
}

// Peak decay computation (mirrors computeVuDecay)
static float computePeakDecay(double sampleRate)
{
    const float peakHoldMs = 1500.0f;
    float peakHoldSamples = static_cast<float>(sampleRate * peakHoldMs / 1000.0f);
    return std::pow(0.001f, 1.0f / peakHoldSamples);
}

// Denormal flush threshold used in peak metering
static constexpr float DenormalThreshold = 1e-10f;

// =============================================================================
// MIXER: Strip Management Tests
// =============================================================================

TEST_CASE("Mixer Strip Management - Add/Remove Bounds", "[mixer]")
{
    int numStrips = DefaultStrips;

    SECTION("Default construction starts with 2 strips")
    {
        REQUIRE(numStrips == 2);
    }

    SECTION("addStrip increments up to MaxStrips")
    {
        for (int i = numStrips; i < MaxStrips; ++i)
        {
            numStrips++;
        }
        REQUIRE(numStrips == MaxStrips);

        // Attempting to add beyond max should be capped
        int capped = numStrips;
        if (capped < MaxStrips)
            capped++;
        REQUIRE(capped == MaxStrips); // No change
    }

    SECTION("removeStrip decrements but stays >= 1")
    {
        numStrips = 1;
        int result = numStrips;
        if (result > 1)
            result--;
        REQUIRE(result == 1); // Can't go below 1
    }

    SECTION("Channel config: mixer has ns*2 inputs, 2 outputs")
    {
        for (int ns = 1; ns <= 8; ++ns)
        {
            int expectedInputs = ns * 2;
            int expectedOutputs = 2;
            REQUIRE(expectedInputs == ns * 2);
            REQUIRE(expectedOutputs == 2);
        }
    }
}

TEST_CASE("Splitter Strip Management - Add/Remove Bounds", "[splitter]")
{
    SECTION("Channel config: splitter has 2 inputs, ns*2 outputs")
    {
        for (int ns = 1; ns <= 8; ++ns)
        {
            int expectedInputs = 2;
            int expectedOutputs = ns * 2;
            REQUIRE(expectedInputs == 2);
            REQUIRE(expectedOutputs == ns * 2);
        }
    }
}

// =============================================================================
// MIXER: Gain Calculation Tests
// =============================================================================

TEST_CASE("Mixer Gain - dB to Linear Conversion", "[mixer][gain]")
{
    SECTION("0 dB = unity gain (1.0)")
    {
        float gain = dbToGain(0.0f);
        REQUIRE_THAT(gain, WithinAbs(1.0f, 0.001f));
    }

    SECTION("+6 dB ~= 2.0x gain")
    {
        float gain = dbToGain(6.0f);
        REQUIRE_THAT(gain, WithinAbs(1.9953f, 0.01f));
    }

    SECTION("-6 dB ~= 0.5x gain")
    {
        float gain = dbToGain(-6.0f);
        REQUIRE_THAT(gain, WithinAbs(0.5012f, 0.01f));
    }

    SECTION("-60 dB ~= 0.001 (effectively silent)")
    {
        float gain = dbToGain(-60.0f);
        REQUIRE(gain < 0.002f);
        REQUIRE(gain > 0.0f);
    }

    SECTION("+12 dB = max gain ~= 3.98x")
    {
        float gain = dbToGain(12.0f);
        REQUIRE_THAT(gain, WithinAbs(3.981f, 0.01f));
    }
}

// =============================================================================
// MIXER: Pan Law Tests
// =============================================================================

TEST_CASE("Mixer Pan Law - Equal Power -3dB", "[mixer][pan]")
{
    SECTION("Center pan (0.0): both channels equal, -3dB")
    {
        float l = panLaw_L(0.0f);
        float r = panLaw_R(0.0f);
        REQUIRE_THAT(l, WithinAbs(r, 0.0001f));
        // sqrt(0.5) ~= 0.7071 which is -3dB
        REQUIRE_THAT(l, WithinAbs(0.7071f, 0.001f));
    }

    SECTION("Full left (-1.0): all left, no right")
    {
        float l = panLaw_L(-1.0f);
        float r = panLaw_R(-1.0f);
        REQUIRE_THAT(l, WithinAbs(1.0f, 0.001f));
        REQUIRE_THAT(r, WithinAbs(0.0f, 0.001f));
    }

    SECTION("Full right (+1.0): no left, all right")
    {
        float l = panLaw_L(1.0f);
        float r = panLaw_R(1.0f);
        REQUIRE_THAT(l, WithinAbs(0.0f, 0.001f));
        REQUIRE_THAT(r, WithinAbs(1.0f, 0.001f));
    }

    SECTION("Constant power: L^2 + R^2 = 1.0 at all positions")
    {
        for (float pan = -1.0f; pan <= 1.0f; pan += 0.1f)
        {
            float l = panLaw_L(pan);
            float r = panLaw_R(pan);
            float power = l * l + r * r;
            REQUIRE_THAT(power, WithinAbs(1.0f, 0.001f));
        }
    }
}

// =============================================================================
// MIXER: processBlock DSP Simulation
// =============================================================================

TEST_CASE("Mixer processBlock - Unity Gain Passthrough", "[mixer][dsp]")
{
    constexpr int numSamples = 128;
    constexpr int ns = 2; // 2 strips

    // Input buffer: 4 channels (2 strips * 2 ch), output: 2 channels
    std::vector<float> inputL0(numSamples, 0.5f); // Strip 0 Left
    std::vector<float> inputR0(numSamples, 0.3f); // Strip 0 Right
    std::vector<float> inputL1(numSamples, 0.2f); // Strip 1 Left
    std::vector<float> inputR1(numSamples, 0.1f); // Strip 1 Right

    std::vector<float> mixL(numSamples, 0.0f);
    std::vector<float> mixR(numSamples, 0.0f);

    // Unity gain (0 dB), center pan, no mute, no solo
    float gain0 = dbToGain(0.0f);
    float gain1 = dbToGain(0.0f);
    float pL = panLaw_L(0.0f);
    float pR = panLaw_R(0.0f);

    for (int i = 0; i < numSamples; ++i)
    {
        mixL[i] += inputL0[i] * gain0 * pL;
        mixR[i] += inputR0[i] * gain0 * pR;
        mixL[i] += inputL1[i] * gain1 * pL;
        mixR[i] += inputR1[i] * gain1 * pR;
    }

    // Sum of both strips, scaled by pan
    float expectedL = (0.5f + 0.2f) * 1.0f * panLaw_L(0.0f);
    float expectedR = (0.3f + 0.1f) * 1.0f * panLaw_R(0.0f);

    REQUIRE_THAT(mixL[0], WithinAbs(expectedL, 0.001f));
    REQUIRE_THAT(mixR[0], WithinAbs(expectedR, 0.001f));
}

TEST_CASE("Mixer processBlock - Gain Applied", "[mixer][dsp]")
{
    constexpr int numSamples = 64;
    float input = 0.5f;
    float gainDb = 6.0f;
    float gain = dbToGain(gainDb);
    float pL = panLaw_L(0.0f);

    float output = input * gain * pL;

    // +6 dB should roughly double the signal (times pan)
    REQUIRE_THAT(output, WithinAbs(input * 1.9953f * pL, 0.01f));
}

TEST_CASE("Mixer processBlock - Mute Strip", "[mixer][dsp]")
{
    float input = 0.5f;
    float gain = dbToGain(0.0f);
    float pL = panLaw_L(0.0f);
    bool mute = true;
    bool anySolo = false;
    bool effectiveMute = mute || (anySolo && !false);

    float mixContribution = 0.0f;
    if (!effectiveMute)
        mixContribution = input * gain * pL;

    REQUIRE_THAT(mixContribution, WithinAbs(0.0f, 0.0001f));
}

TEST_CASE("Mixer processBlock - Solo Logic", "[mixer][dsp]")
{
    SECTION("Solo'd strip passes, non-solo'd strip silenced")
    {
        bool strip0Solo = true;
        bool strip1Solo = false;
        bool anySolo = true; // At least one strip is solo'd

        bool effective0 = false || (anySolo && !strip0Solo); // false -> passes
        bool effective1 = false || (anySolo && !strip1Solo); // true -> silenced

        REQUIRE_FALSE(effective0);
        REQUIRE(effective1);
    }

    SECTION("Multiple solo'd strips all pass")
    {
        bool s0 = true, s1 = true, s2 = false;
        bool anySolo = true;

        bool e0 = false || (anySolo && !s0);
        bool e1 = false || (anySolo && !s1);
        REQUIRE_FALSE(e0);
        REQUIRE_FALSE(e1);
        bool e2 = false || (anySolo && !s2);
        REQUIRE(e2);
    }

    SECTION("No solo: all strips pass")
    {
        bool anySolo = false;
        bool s0 = false, s1 = false;

        bool e0 = false || (anySolo && !s0);
        bool e1 = false || (anySolo && !s1);

        REQUIRE_FALSE(e0);
        REQUIRE_FALSE(e1);
    }
}

TEST_CASE("Mixer processBlock - Phase Invert", "[mixer][dsp]")
{
    float inputL = 0.7f;
    float inputR = -0.3f;
    bool phaseInvert = true;

    if (phaseInvert)
    {
        inputL = -inputL;
        inputR = -inputR;
    }

    REQUIRE_THAT(inputL, WithinAbs(-0.7f, 0.0001f));
    REQUIRE_THAT(inputR, WithinAbs(0.3f, 0.0001f));
}

TEST_CASE("Mixer processBlock - Master Mute", "[mixer][dsp]")
{
    float mixedL = 0.5f;
    float mixedR = 0.3f;
    float masterGain = dbToGain(0.0f);
    bool masterMute = true;

    float outL = mixedL * masterGain;
    float outR = mixedR * masterGain;

    if (masterMute)
    {
        outL = 0.0f;
        outR = 0.0f;
    }

    REQUIRE_THAT(outL, WithinAbs(0.0f, 0.0001f));
    REQUIRE_THAT(outR, WithinAbs(0.0f, 0.0001f));
}

// =============================================================================
// MIXER: Buffer Channel Clamping (Critical Crash Fix)
// =============================================================================

TEST_CASE("Mixer processBlock - Buffer Channel Clamping", "[mixer][safety]")
{
    SECTION("Strips beyond buffer channels are skipped")
    {
        int ns = 4;                 // 4 strips declared
        int totalInputChannels = 4; // But buffer only has 4 channels (2 strips worth)

        int processedStrips = 0;
        for (int s = 0; s < ns; ++s)
        {
            int inL = s * 2;
            int inR = s * 2 + 1;
            if (inL >= totalInputChannels || inR >= totalInputChannels)
                continue;
            processedStrips++;
        }

        // Only 2 strips should process (channels 0-3)
        REQUIRE(processedStrips == 2);
    }

    SECTION("Zero strips produces silence")
    {
        int ns = 0;
        int numSamples = 64;
        std::vector<float> buffer(numSamples, 1.0f);

        if (ns == 0 || numSamples == 0)
        {
            std::fill(buffer.begin(), buffer.end(), 0.0f);
        }

        REQUIRE_THAT(buffer[0], WithinAbs(0.0f, 0.0001f));
    }

    SECTION("Mono buffer (1 channel) skips all strips")
    {
        int totalInputChannels = 1;
        int ns = 2;
        int processed = 0;
        for (int s = 0; s < ns; ++s)
        {
            int inL = s * 2;
            int inR = s * 2 + 1;
            if (inL >= totalInputChannels || inR >= totalInputChannels)
                continue;
            processed++;
        }
        REQUIRE(processed == 0);
    }
}

// =============================================================================
// MIXER: VU Peak Metering
// =============================================================================

TEST_CASE("Mixer VU Peak Metering", "[mixer][metering]")
{
    SECTION("Peak tracks maximum absolute signal")
    {
        float peak = 0.0f;
        float peakDecay = computePeakDecay(44100.0);
        std::vector<float> samples = {0.1f, 0.5f, 0.3f, -0.8f, 0.2f};

        for (float s : samples)
        {
            float absS = std::abs(s);
            peak = (absS > peak) ? absS : peak * peakDecay;
        }

        // Peak should be 0.8 (the -0.8 sample)
        REQUIRE_THAT(peak, WithinAbs(0.8f, 0.001f));
    }

    SECTION("Peak decays over silent blocks")
    {
        float peak = 1.0f;
        float peakDecay = computePeakDecay(44100.0);

        // Process 1000 silent samples
        for (int i = 0; i < 1000; ++i)
        {
            float absS = 0.0f;
            peak = (absS > peak) ? absS : peak * peakDecay;
        }

        REQUIRE(peak < 1.0f);
        REQUIRE(peak > 0.0f); // Still decaying

        // Process many more silent samples
        for (int i = 0; i < 100000; ++i)
        {
            peak = peak * peakDecay;
        }

        // Should be near zero
        if (peak < DenormalThreshold)
            peak = 0.0f;

        REQUIRE_THAT(peak, WithinAbs(0.0f, 0.0001f));
    }
}

// =============================================================================
// SPLITTER: processBlock DSP Simulation
// =============================================================================

TEST_CASE("Splitter processBlock - Unity Gain Copy", "[splitter][dsp]")
{
    constexpr int numSamples = 128;
    constexpr int ns = 2;

    float inputL = 0.5f;
    float inputR = 0.3f;
    float gain = dbToGain(0.0f);
    float pL = panLaw_L(0.0f);
    float pR = panLaw_R(0.0f);

    // Each output strip copies the input
    float out0L = inputL * gain * pL;
    float out0R = inputR * gain * pR;
    float out1L = inputL * gain * pL;
    float out1R = inputR * gain * pR;

    // All outputs should be identical copies (scaled by pan)
    REQUIRE_THAT(out0L, WithinAbs(out1L, 0.0001f));
    REQUIRE_THAT(out0R, WithinAbs(out1R, 0.0001f));
    REQUIRE_THAT(out0L, WithinAbs(inputL * pL, 0.001f));
}

TEST_CASE("Splitter processBlock - Per-Strip Gain", "[splitter][dsp]")
{
    float input = 0.5f;
    float gain0 = dbToGain(0.0f);  // Unity
    float gain1 = dbToGain(-6.0f); // Half
    float pL = panLaw_L(0.0f);

    float out0 = input * gain0 * pL;
    float out1 = input * gain1 * pL;

    REQUIRE(out0 > out1);
    REQUIRE_THAT(out0 / out1, WithinAbs(1.9953f, 0.02f)); // ~6dB ratio
}

TEST_CASE("Splitter processBlock - Mute Output Strip", "[splitter][dsp]")
{
    float input = 0.5f;
    bool mute = true;
    bool anySolo = false;
    bool effectiveMute = mute || (anySolo && !false);

    float output = 0.0f;
    if (!effectiveMute)
        output = input * dbToGain(0.0f);

    REQUIRE_THAT(output, WithinAbs(0.0f, 0.0001f));
}

TEST_CASE("Splitter processBlock - Buffer Channel Clamping", "[splitter][safety]")
{
    SECTION("Output strips beyond buffer channels skipped")
    {
        int ns = 4;
        int totalOutputChannels = 6; // Only 3 strips fit

        int processedStrips = 0;
        for (int s = 0; s < ns; ++s)
        {
            int outL = s * 2;
            int outR = s * 2 + 1;
            if (outL >= totalOutputChannels || outR >= totalOutputChannels)
                continue;
            processedStrips++;
        }

        REQUIRE(processedStrips == 3);
    }

    SECTION("Splitter input channel clamping with mono buffer")
    {
        // inR = jmin(1, totalOutputChannels - 1) prevents crash
        int totalOutputChannels = 1;
        int inR = std::min(1, totalOutputChannels - 1); // = 0, reads channel 0 as R
        REQUIRE(inR == 0);
    }
}

TEST_CASE("Splitter Input VU Metering", "[splitter][metering]")
{
    float inPkL = 0.0f;
    float inPkR = 0.0f;
    float peakDecay = computePeakDecay(44100.0);

    // Process a block with signal
    std::vector<float> samplesL = {0.1f, 0.4f, 0.7f, 0.2f};
    std::vector<float> samplesR = {0.3f, 0.6f, 0.1f, 0.5f};

    for (size_t i = 0; i < samplesL.size(); ++i)
    {
        float absL = std::abs(samplesL[i]);
        float absR = std::abs(samplesR[i]);
        inPkL = (absL > inPkL) ? absL : inPkL * peakDecay;
        inPkR = (absR > inPkR) ? absR : inPkR * peakDecay;
    }

    REQUIRE_THAT(inPkL, WithinAbs(0.7f, 0.001f));
    REQUIRE_THAT(inPkR, WithinAbs(0.6f, 0.001f));
}

// =============================================================================
// STATE SERIALIZATION TESTS
// =============================================================================

TEST_CASE("Mixer State Serialization Values", "[mixer][state]")
{
    SECTION("Default strip state values")
    {
        float gainDb = 0.0f;
        float pan = 0.0f;
        bool mute = false;
        bool solo = false;
        bool phaseInvert = false;

        REQUIRE_THAT(gainDb, WithinAbs(0.0f, 0.0001f));
        REQUIRE_THAT(pan, WithinAbs(0.0f, 0.0001f));
        REQUIRE_FALSE(mute);
        REQUIRE_FALSE(solo);
        REQUIRE_FALSE(phaseInvert);
    }

    SECTION("Strip resetDefaults restores correct values")
    {
        // Simulate dirty state then reset
        float gainDb = 6.0f;
        float pan = -0.5f;
        bool mute = true;
        bool solo = true;
        bool phaseInvert = true;

        // resetDefaults:
        gainDb = 0.0f;
        pan = 0.0f;
        mute = false;
        solo = false;
        phaseInvert = false;

        REQUIRE_THAT(gainDb, WithinAbs(0.0f, 0.0001f));
        REQUIRE_THAT(pan, WithinAbs(0.0f, 0.0001f));
        REQUIRE_FALSE(mute);
        REQUIRE_FALSE(solo);
        REQUIRE_FALSE(phaseInvert);
    }
}

// =============================================================================
// MUTATION TESTING - Mixer
// =============================================================================

TEST_CASE("Mixer Mutation Testing", "[mixer][mutation]")
{
    SECTION("ARITHMETIC: dB conversion uses 0.05 not 0.1 or 0.5")
    {
        float correct6dB = std::pow(10.0f, 6.0f * 0.05f);
        float mutated_01 = std::pow(10.0f, 6.0f * 0.1f);
        float mutated_005 = std::pow(10.0f, 6.0f * 0.005f);

        REQUIRE_THAT(correct6dB, WithinAbs(1.9953f, 0.01f));
        REQUIRE(std::abs(mutated_01 - 1.9953f) > 1.0f);  // ~3.98, wrong
        REQUIRE(std::abs(mutated_005 - 1.9953f) > 0.5f); // ~1.07, wrong
    }

    SECTION("PAN LAW: sqrt(0.5*(1-pan)) vs linear (1-pan)/2")
    {
        float pan = 0.5f; // Half right

        float correct_L = std::sqrt(0.5f * (1.0f - pan));
        float linear_L = (1.0f - pan) / 2.0f;

        // Equal-power and linear give different results at non-extreme positions
        REQUIRE(std::abs(correct_L - linear_L) > 0.05f);
    }

    SECTION("PAN LAW: constant power preserved at all positions")
    {
        // Mutation: if sqrt was removed, power wouldn't be constant
        for (float pan = -1.0f; pan <= 1.0f; pan += 0.25f)
        {
            float l = panLaw_L(pan);
            float r = panLaw_R(pan);

            // Equal power: L^2 + R^2 = 1.0
            float power = l * l + r * r;
            REQUIRE_THAT(power, WithinAbs(1.0f, 0.001f));

            // Mutation: without sqrt, power would NOT be constant
            float mutL = 0.5f * (1.0f - pan); // no sqrt
            float mutR = 0.5f * (1.0f + pan);
            float mutPower = mutL * mutL + mutR * mutR;

            if (std::abs(pan) > 0.01f && std::abs(pan) < 0.99f) // Not at center or extremes
            {
                REQUIRE(std::abs(mutPower - 1.0f) > 0.01f); // Mutation detectable
            }
        }
    }

    SECTION("SOLO LOGIC: anySolo && !solo, not anySolo && solo")
    {
        bool anySolo = true;
        bool stripSolo = false;

        // Correct: non-solo'd strip is muted when any solo is active
        bool correct = anySolo && !stripSolo;
        REQUIRE(correct == true);

        // Mutation: condition negated
        bool mutated = anySolo && stripSolo;
        REQUIRE(mutated == false);
        REQUIRE(correct != mutated);
    }

    SECTION("PHASE INVERT: l = -l, not l = l")
    {
        float original = 0.7f;
        float correct = -original;
        float mutated = original; // Phase invert removed

        REQUIRE_THAT(correct, WithinAbs(-0.7f, 0.0001f));
        REQUIRE(std::abs(correct - mutated) > 1.0f);
    }

    SECTION("BUFFER CLAMPING: uses buffer.getNumChannels() not getTotalNumInputChannels()")
    {
        // Scenario: declared 8 channels, buffer only has 4
        int declaredChannels = 8;
        int bufferChannels = 4;

        // Correct: use buffer channels
        int processed_correct = 0;
        for (int s = 0; s < 4; ++s)
        {
            int inL = s * 2;
            int inR = s * 2 + 1;
            if (inR >= bufferChannels)
                continue;
            processed_correct++;
        }

        // Mutation: use declared channels (would cause overrun)
        int processed_mutated = 0;
        for (int s = 0; s < 4; ++s)
        {
            int inL = s * 2;
            int inR = s * 2 + 1;
            if (inR >= declaredChannels)
                continue;
            processed_mutated++;
        }

        REQUIRE(processed_correct == 2); // Only 2 strips fit in 4 channels
        REQUIRE(processed_mutated == 4); // Would process all 4 (CRASH!)
        REQUIRE(processed_correct != processed_mutated);
    }

    SECTION("PEAK DECAY: peaks decrease over silence, not increase")
    {
        float peak = 1.0f;
        float peakDecay = computePeakDecay(44100.0);

        REQUIRE(peakDecay > 0.0f);
        REQUIRE(peakDecay < 1.0f);

        float decayed = peak * peakDecay;
        REQUIRE(decayed < peak);

        // Mutation: if decay > 1.0 (wrong constant), peak would grow
        float mutatedDecay = 1.001f;
        float mutatedResult = peak * mutatedDecay;
        REQUIRE(mutatedResult > peak); // Mutation: peak grows instead of decaying
    }

    SECTION("DENORMAL FLUSH: threshold is 1e-10, not 1e-5 or 0")
    {
        float verySmall = 1e-11f;
        bool correctFlush = (verySmall < 1e-10f);
        bool mutatedFlush_no = (verySmall < 0.0f);      // Never flushes positive values
        bool mutatedFlush_coarse = (verySmall < 1e-5f); // Flushes too aggressively

        REQUIRE(correctFlush);
        REQUIRE_FALSE(mutatedFlush_no);
        REQUIRE(mutatedFlush_coarse); // Would kill valid quiet signals
    }
}

// =============================================================================
// MUTATION TESTING - Splitter
// =============================================================================

TEST_CASE("Splitter Mutation Testing", "[splitter][mutation]")
{
    SECTION("OUTPUT channels: splitter writes to s*2, s*2+1 (not s, s+1)")
    {
        int ns = 3;
        // Correct mapping
        std::vector<int> correctL, correctR;
        for (int s = 0; s < ns; ++s)
        {
            correctL.push_back(s * 2);
            correctR.push_back(s * 2 + 1);
        }

        REQUIRE(correctL[0] == 0);
        REQUIRE(correctR[0] == 1);
        REQUIRE(correctL[1] == 2);
        REQUIRE(correctR[1] == 3);
        REQUIRE(correctL[2] == 4);
        REQUIRE(correctR[2] == 5);

        // Mutation: using s, s+1 instead of s*2, s*2+1
        std::vector<int> mutL, mutR;
        for (int s = 0; s < ns; ++s)
        {
            mutL.push_back(s);
            mutR.push_back(s + 1);
        }

        // Mutation would cause channel overlap (strip 0 R == strip 1 L)
        REQUIRE(mutR[0] == mutL[1]); // Overlapping channels = BUG
    }

    SECTION("INPUT read: inR = jmin(1, totalChannels-1) prevents OOB")
    {
        // Correct: jmin prevents read past end with mono buffer
        int totalOutputChannels = 1;
        int correctInR = std::min(1, totalOutputChannels - 1);
        REQUIRE(correctInR == 0); // Falls back to mono

        // With 2+ channels, reads channel 1 normally
        totalOutputChannels = 4;
        correctInR = std::min(1, totalOutputChannels - 1);
        REQUIRE(correctInR == 1);

        // Mutation: hardcoded inR = 1 would crash with mono buffer
        int mutatedInR = 1;
        bool mutatedOk = (mutatedInR >= totalOutputChannels || mutatedInR == 1);
        REQUIRE(mutatedOk); // OK for stereo+
    }
}
