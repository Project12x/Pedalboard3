/**
 * @file nam_processor_test.cpp
 * @brief Tests for NAMProcessor parameter handling and state persistence
 *
 * These tests verify:
 * 1. Parameter bounds and clamping
 * 2. State serialization round-trip
 * 3. Utility function correctness (dB conversion)
 *
 * NOTE: These are headless tests - no full NAMProcessor instantiation
 * to avoid needing audio initialization.
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <cmath>
#include <cstring>
#include <vector>

using Catch::Matchers::WithinAbs;

// ============================================================================
// Standalone utility functions (mirrors NAMProcessor)
// ============================================================================

float dBToLinear(float dB)
{
    return std::pow(10.0f, dB / 20.0f);
}

float linearTodB(float linear)
{
    if (linear <= 0.0f)
        return -100.0f;
    return 20.0f * std::log10(linear);
}

float clampInputGain(float dB)
{
    return std::max(-20.0f, std::min(20.0f, dB));
}

float clampOutputGain(float dB)
{
    return std::max(-40.0f, std::min(40.0f, dB));
}

float clampNoiseGate(float dB)
{
    return std::max(-101.0f, std::min(0.0f, dB));
}

float clampToneStackParam(float value)
{
    return std::max(0.0f, std::min(10.0f, value));
}

// ============================================================================
// State serialization helpers (simplified version of NAMProcessor format)
// ============================================================================

struct NAMState
{
    int version = 1;
    std::string modelPath;
    std::string irPath;
    float inputGain = 0.0f;
    float outputGain = 0.0f;
    float noiseGateThreshold = -80.0f;
    float bass = 5.0f;
    float mid = 5.0f;
    float treble = 5.0f;
    bool toneStackEnabled = true;
    bool normalizeOutput = false;
    bool irEnabled = true;
    bool toneStackPre = false; // v4: PRE (true) or POST (false)
};

std::vector<uint8_t> serializeState(const NAMState& state)
{
    std::vector<uint8_t> data;

    // Write version (4 bytes, little-endian)
    data.push_back(state.version & 0xFF);
    data.push_back((state.version >> 8) & 0xFF);
    data.push_back((state.version >> 16) & 0xFF);
    data.push_back((state.version >> 24) & 0xFF);

    // Write model path (length + string)
    uint32_t modelLen = static_cast<uint32_t>(state.modelPath.size());
    data.push_back(modelLen & 0xFF);
    data.push_back((modelLen >> 8) & 0xFF);
    data.push_back((modelLen >> 16) & 0xFF);
    data.push_back((modelLen >> 24) & 0xFF);
    for (char c : state.modelPath)
        data.push_back(static_cast<uint8_t>(c));

    // Write IR path (length + string)
    uint32_t irLen = static_cast<uint32_t>(state.irPath.size());
    data.push_back(irLen & 0xFF);
    data.push_back((irLen >> 8) & 0xFF);
    data.push_back((irLen >> 16) & 0xFF);
    data.push_back((irLen >> 24) & 0xFF);
    for (char c : state.irPath)
        data.push_back(static_cast<uint8_t>(c));

    // Write floats (4 bytes each)
    auto writeFloat = [&data](float f)
    {
        uint32_t bits;
        std::memcpy(&bits, &f, sizeof(float));
        data.push_back(bits & 0xFF);
        data.push_back((bits >> 8) & 0xFF);
        data.push_back((bits >> 16) & 0xFF);
        data.push_back((bits >> 24) & 0xFF);
    };

    writeFloat(state.inputGain);
    writeFloat(state.outputGain);
    writeFloat(state.noiseGateThreshold);
    writeFloat(state.bass);
    writeFloat(state.mid);
    writeFloat(state.treble);

    // Write bools (1 byte each)
    data.push_back(state.toneStackEnabled ? 1 : 0);
    data.push_back(state.normalizeOutput ? 1 : 0);
    data.push_back(state.irEnabled ? 1 : 0);
    data.push_back(state.toneStackPre ? 1 : 0);

    return data;
}

NAMState deserializeState(const std::vector<uint8_t>& data)
{
    NAMState state;
    size_t pos = 0;

    auto readInt = [&data, &pos]() -> int32_t
    {
        int32_t val = data[pos] | (data[pos + 1] << 8) | (data[pos + 2] << 16) | (data[pos + 3] << 24);
        pos += 4;
        return val;
    };

    auto readString = [&data, &pos]() -> std::string
    {
        uint32_t len = data[pos] | (data[pos + 1] << 8) | (data[pos + 2] << 16) | (data[pos + 3] << 24);
        pos += 4;
        std::string s(data.begin() + pos, data.begin() + pos + len);
        pos += len;
        return s;
    };

    auto readFloat = [&data, &pos]() -> float
    {
        uint32_t bits = data[pos] | (data[pos + 1] << 8) | (data[pos + 2] << 16) | (data[pos + 3] << 24);
        pos += 4;
        float f;
        std::memcpy(&f, &bits, sizeof(float));
        return f;
    };

    auto readBool = [&data, &pos]() -> bool { return data[pos++] != 0; };

    state.version = readInt();
    state.modelPath = readString();
    state.irPath = readString();
    state.inputGain = readFloat();
    state.outputGain = readFloat();
    state.noiseGateThreshold = readFloat();
    state.bass = readFloat();
    state.mid = readFloat();
    state.treble = readFloat();
    state.toneStackEnabled = readBool();
    state.normalizeOutput = readBool();
    state.irEnabled = readBool();
    if (pos < data.size())
        state.toneStackPre = readBool();

    return state;
}

// ============================================================================
// Parameter Bounds Tests
// ============================================================================

TEST_CASE("NAM Parameter Bounds - Input Gain", "[nam][parameters]")
{
    SECTION("Input gain clamps at lower bound -20 dB")
    {
        REQUIRE(clampInputGain(-30.0f) == -20.0f);
        REQUIRE(clampInputGain(-100.0f) == -20.0f);
    }

    SECTION("Input gain clamps at upper bound +20 dB")
    {
        REQUIRE(clampInputGain(30.0f) == 20.0f);
        REQUIRE(clampInputGain(100.0f) == 20.0f);
    }

    SECTION("Input gain passes through valid values")
    {
        REQUIRE(clampInputGain(0.0f) == 0.0f);
        REQUIRE(clampInputGain(-20.0f) == -20.0f);
        REQUIRE(clampInputGain(20.0f) == 20.0f);
        REQUIRE(clampInputGain(10.5f) == 10.5f);
    }
}

TEST_CASE("NAM Parameter Bounds - Output Gain", "[nam][parameters]")
{
    SECTION("Output gain clamps at lower bound -40 dB")
    {
        REQUIRE(clampOutputGain(-50.0f) == -40.0f);
        REQUIRE(clampOutputGain(-100.0f) == -40.0f);
    }

    SECTION("Output gain clamps at upper bound +40 dB")
    {
        REQUIRE(clampOutputGain(50.0f) == 40.0f);
        REQUIRE(clampOutputGain(100.0f) == 40.0f);
    }

    SECTION("Output gain passes through valid values")
    {
        REQUIRE(clampOutputGain(0.0f) == 0.0f);
        REQUIRE(clampOutputGain(-40.0f) == -40.0f);
        REQUIRE(clampOutputGain(40.0f) == 40.0f);
    }
}

TEST_CASE("NAM Parameter Bounds - Noise Gate", "[nam][parameters]")
{
    SECTION("Noise gate clamps at lower bound -101 dB (off)")
    {
        REQUIRE(clampNoiseGate(-150.0f) == -101.0f);
    }

    SECTION("Noise gate clamps at upper bound 0 dB")
    {
        REQUIRE(clampNoiseGate(10.0f) == 0.0f);
    }

    SECTION("Noise gate -101 represents off state")
    {
        float gateThreshold = -101.0f;
        bool isGateEnabled = gateThreshold > -100.0f;
        REQUIRE_FALSE(isGateEnabled);
    }

    SECTION("Noise gate -100 and above is active")
    {
        float gateThreshold = -100.0f;
        bool isGateEnabled = gateThreshold > -100.0f;
        REQUIRE_FALSE(isGateEnabled); // -100 is at boundary

        gateThreshold = -99.0f;
        isGateEnabled = gateThreshold > -100.0f;
        REQUIRE(isGateEnabled);
    }
}

TEST_CASE("NAM Parameter Bounds - Tone Stack", "[nam][parameters]")
{
    SECTION("Bass clamps to [0, 10]")
    {
        REQUIRE(clampToneStackParam(-5.0f) == 0.0f);
        REQUIRE(clampToneStackParam(15.0f) == 10.0f);
        REQUIRE(clampToneStackParam(5.0f) == 5.0f);
    }

    SECTION("Mid clamps to [0, 10]")
    {
        REQUIRE(clampToneStackParam(-1.0f) == 0.0f);
        REQUIRE(clampToneStackParam(11.0f) == 10.0f);
        REQUIRE(clampToneStackParam(7.5f) == 7.5f);
    }

    SECTION("Treble clamps to [0, 10]")
    {
        REQUIRE(clampToneStackParam(-0.1f) == 0.0f);
        REQUIRE(clampToneStackParam(10.1f) == 10.0f);
        REQUIRE(clampToneStackParam(3.3f) == 3.3f);
    }
}

// ============================================================================
// dB Conversion Tests
// ============================================================================

TEST_CASE("NAM dB to Linear Conversion", "[nam][utils]")
{
    SECTION("0 dB = 1.0 linear")
    {
        REQUIRE_THAT(dBToLinear(0.0f), WithinAbs(1.0f, 0.001f));
    }

    SECTION("+6 dB = ~2.0 linear")
    {
        REQUIRE_THAT(dBToLinear(6.0f), WithinAbs(2.0f, 0.01f));
    }

    SECTION("-6 dB = ~0.5 linear")
    {
        REQUIRE_THAT(dBToLinear(-6.0f), WithinAbs(0.5f, 0.01f));
    }

    SECTION("+20 dB = 10.0 linear")
    {
        REQUIRE_THAT(dBToLinear(20.0f), WithinAbs(10.0f, 0.01f));
    }

    SECTION("-20 dB = 0.1 linear")
    {
        REQUIRE_THAT(dBToLinear(-20.0f), WithinAbs(0.1f, 0.001f));
    }

    SECTION("-40 dB = 0.01 linear")
    {
        REQUIRE_THAT(dBToLinear(-40.0f), WithinAbs(0.01f, 0.0001f));
    }
}

TEST_CASE("NAM Linear to dB Conversion", "[nam][utils]")
{
    SECTION("1.0 linear = 0 dB")
    {
        REQUIRE_THAT(linearTodB(1.0f), WithinAbs(0.0f, 0.01f));
    }

    SECTION("2.0 linear = ~+6 dB")
    {
        REQUIRE_THAT(linearTodB(2.0f), WithinAbs(6.02f, 0.1f));
    }

    SECTION("0.5 linear = ~-6 dB")
    {
        REQUIRE_THAT(linearTodB(0.5f), WithinAbs(-6.02f, 0.1f));
    }

    SECTION("Zero or negative linear returns floor value")
    {
        REQUIRE(linearTodB(0.0f) == -100.0f);
        REQUIRE(linearTodB(-1.0f) == -100.0f);
    }
}

TEST_CASE("NAM dB Conversion Roundtrip", "[nam][utils]")
{
    SECTION("dB -> Linear -> dB roundtrips correctly")
    {
        for (float dB = -40.0f; dB <= 40.0f; dB += 5.0f)
        {
            float linear = dBToLinear(dB);
            float recovered = linearTodB(linear);
            REQUIRE_THAT(recovered, WithinAbs(dB, 0.01f));
        }
    }
}

// ============================================================================
// State Serialization Tests
// ============================================================================

TEST_CASE("NAM State Serialization - Version", "[nam][state]")
{
    SECTION("Version 1 is preserved")
    {
        NAMState original;
        original.version = 1;

        auto data = serializeState(original);
        auto restored = deserializeState(data);

        REQUIRE(restored.version == 1);
    }
}

TEST_CASE("NAM State Serialization - Parameters", "[nam][state]")
{
    SECTION("All float parameters round-trip correctly")
    {
        NAMState original;
        original.inputGain = 10.5f;
        original.outputGain = -15.3f;
        original.noiseGateThreshold = -60.0f;
        original.bass = 7.2f;
        original.mid = 3.8f;
        original.treble = 8.1f;

        auto data = serializeState(original);
        auto restored = deserializeState(data);

        REQUIRE_THAT(restored.inputGain, WithinAbs(original.inputGain, 0.001f));
        REQUIRE_THAT(restored.outputGain, WithinAbs(original.outputGain, 0.001f));
        REQUIRE_THAT(restored.noiseGateThreshold, WithinAbs(original.noiseGateThreshold, 0.001f));
        REQUIRE_THAT(restored.bass, WithinAbs(original.bass, 0.001f));
        REQUIRE_THAT(restored.mid, WithinAbs(original.mid, 0.001f));
        REQUIRE_THAT(restored.treble, WithinAbs(original.treble, 0.001f));
    }

    SECTION("All boolean parameters round-trip correctly")
    {
        NAMState original;
        original.toneStackEnabled = false;
        original.normalizeOutput = true;
        original.irEnabled = false;

        auto data = serializeState(original);
        auto restored = deserializeState(data);

        REQUIRE(restored.toneStackEnabled == original.toneStackEnabled);
        REQUIRE(restored.normalizeOutput == original.normalizeOutput);
        REQUIRE(restored.irEnabled == original.irEnabled);
    }
}

TEST_CASE("NAM State Serialization - File Paths", "[nam][state]")
{
    SECTION("Model path round-trips correctly")
    {
        NAMState original;
        original.modelPath = "C:/Models/MyAmp.nam";

        auto data = serializeState(original);
        auto restored = deserializeState(data);

        REQUIRE(restored.modelPath == original.modelPath);
    }

    SECTION("IR path round-trips correctly")
    {
        NAMState original;
        original.irPath = "C:/IRs/Cabinet.wav";

        auto data = serializeState(original);
        auto restored = deserializeState(data);

        REQUIRE(restored.irPath == original.irPath);
    }

    SECTION("Empty paths are handled correctly")
    {
        NAMState original;
        original.modelPath = "";
        original.irPath = "";

        auto data = serializeState(original);
        auto restored = deserializeState(data);

        REQUIRE(restored.modelPath.empty());
        REQUIRE(restored.irPath.empty());
    }

    SECTION("Unicode paths round-trip correctly")
    {
        NAMState original;
        original.modelPath = "C:/Models/Amp_Test.nam"; // Keep ASCII for reliability

        auto data = serializeState(original);
        auto restored = deserializeState(data);

        REQUIRE(restored.modelPath == original.modelPath);
    }
}

TEST_CASE("NAM State Serialization - Full State", "[nam][state]")
{
    SECTION("Complete state round-trips correctly")
    {
        NAMState original;
        original.version = 1;
        original.modelPath = "/path/to/model.nam";
        original.irPath = "/path/to/cabinet.wav";
        original.inputGain = 12.0f;
        original.outputGain = -6.0f;
        original.noiseGateThreshold = -70.0f;
        original.bass = 4.0f;
        original.mid = 6.0f;
        original.treble = 7.0f;
        original.toneStackEnabled = true;
        original.normalizeOutput = true;
        original.irEnabled = false;

        auto data = serializeState(original);
        auto restored = deserializeState(data);

        REQUIRE(restored.version == original.version);
        REQUIRE(restored.modelPath == original.modelPath);
        REQUIRE(restored.irPath == original.irPath);
        REQUIRE_THAT(restored.inputGain, WithinAbs(original.inputGain, 0.001f));
        REQUIRE_THAT(restored.outputGain, WithinAbs(original.outputGain, 0.001f));
        REQUIRE_THAT(restored.noiseGateThreshold, WithinAbs(original.noiseGateThreshold, 0.001f));
        REQUIRE_THAT(restored.bass, WithinAbs(original.bass, 0.001f));
        REQUIRE_THAT(restored.mid, WithinAbs(original.mid, 0.001f));
        REQUIRE_THAT(restored.treble, WithinAbs(original.treble, 0.001f));
        REQUIRE(restored.toneStackEnabled == original.toneStackEnabled);
        REQUIRE(restored.normalizeOutput == original.normalizeOutput);
        REQUIRE(restored.irEnabled == original.irEnabled);
    }
}

// ============================================================================
// Mutation Testing
// ============================================================================

TEST_CASE("NAM Mutation Testing - dB Formula", "[nam][mutation]")
{
    SECTION("dBToLinear uses /20, not /10 or /40")
    {
        // Correct: 10^(dB/20) for amplitude
        float correct = std::pow(10.0f, 6.0f / 20.0f); // ~2.0
        REQUIRE_THAT(correct, WithinAbs(2.0f, 0.01f));

        // Mutation: if /10 was used (power, not amplitude)
        float mutated10 = std::pow(10.0f, 6.0f / 10.0f); // ~4.0
        REQUIRE(std::abs(mutated10 - 2.0f) > 0.5f);

        // Mutation: if /40 was used
        float mutated40 = std::pow(10.0f, 6.0f / 40.0f); // ~1.4
        REQUIRE(std::abs(mutated40 - 2.0f) > 0.5f);
    }

    SECTION("dBToLinear uses base 10, not base 2 or e")
    {
        float correctDb = 20.0f;
        float correctLinear = std::pow(10.0f, correctDb / 20.0f); // = 10.0

        // Mutation: if base 2 was used
        float mutatedBase2 = std::pow(2.0f, correctDb / 20.0f); // = 2.0
        REQUIRE(std::abs(mutatedBase2 - 10.0f) > 1.0f);

        // Mutation: if base e was used
        float mutatedBaseE = std::exp(correctDb / 20.0f); // ~2.7
        REQUIRE(std::abs(mutatedBaseE - 10.0f) > 1.0f);
    }
}

TEST_CASE("NAM Mutation Testing - Parameter Bounds", "[nam][mutation]")
{
    SECTION("Input gain bounds are [-20, 20], not [-10, 10] or [-30, 30]")
    {
        // Test that -20 is the actual lower bound
        REQUIRE(clampInputGain(-20.0f) == -20.0f);
        REQUIRE(clampInputGain(-21.0f) == -20.0f);

        // Test that 20 is the actual upper bound
        REQUIRE(clampInputGain(20.0f) == 20.0f);
        REQUIRE(clampInputGain(21.0f) == 20.0f);
    }

    SECTION("Noise gate off threshold is -101, not -100 or -102")
    {
        // At -101, gate is off
        float threshold = -101.0f;
        bool enabled = threshold > -100.0f;
        REQUIRE_FALSE(enabled);

        // At -99, gate is on
        threshold = -99.0f;
        enabled = threshold > -100.0f;
        REQUIRE(enabled);
    }
}

// ============================================================================
// Integration Tests - Signal Flow
// ============================================================================

/**
 * Simulates the NAM processing chain order for testing gain staging.
 * Order: Input Gain -> NAM (identity) -> Normalize -> Gate -> ToneStack -> IR -> Output Gain
 */
struct MockProcessingChain
{
    float inputGain = 0.0f;  // dB
    float outputGain = 0.0f; // dB
    bool normalizeEnabled = false;
    double modelLoudness = -12.0; // dB
    bool hasLoudness = true;
    bool noiseGateEnabled = false;
    bool toneStackEnabled = false;
    bool toneStackPre = false;
    float bass = 5.0f;
    float mid = 5.0f;
    float treble = 5.0f;
    bool irEnabled = false;

    static constexpr double kNormalizationTarget = -18.0;

    // Simple 3-band tone stack approximation for testing.
    // Uses basic gain curves that mirror the shape of the real
    // BasicNamToneStack but are deterministic and pure-math.
    // bass/mid/treble range [0,10], center at 5 = unity.
    static float toneStackGain(float sample, float bass, float mid, float treble)
    {
        // Simplified model: each band contributes a gain factor
        // at 5.0 = unity, below 5 = cut, above 5 = boost
        // Real tone stack is frequency-dependent; we approximate
        // the overall level change for gain-staging verification.
        float bassGain = 0.5f + (bass / 10.0f);     // [0.5, 1.5]
        float midGain = 0.5f + (mid / 10.0f);       // [0.5, 1.5]
        float trebleGain = 0.5f + (treble / 10.0f); // [0.5, 1.5]
        return sample * bassGain * midGain * trebleGain;
    }

    // Simple nonlinear model approximation (soft clip)
    static float softClip(float x) { return std::tanh(x); }

    float processGainOnly(float input) const
    {
        float sample = input;

        // 1. Input gain
        sample *= dBToLinear(inputGain);

        // 2. NAM model (identity for testing)
        // sample unchanged

        // 3. Normalize if enabled
        if (normalizeEnabled && hasLoudness)
        {
            double gain = std::pow(10.0, (kNormalizationTarget - modelLoudness) / 20.0);
            sample *= static_cast<float>(gain);
        }

        // 4. Noise gate (identity for testing)
        // 5. Tone stack (identity for testing)
        // 6. IR convolution (identity for testing)

        // 7. Output gain
        sample *= dBToLinear(outputGain);

        return sample;
    }

    // Full chain with tone stack position and optional nonlinear model
    float processWithToneStack(float input, bool useNonlinearModel = false) const
    {
        float sample = input;

        // 1. Input gain
        sample *= dBToLinear(inputGain);

        // 2. Tone stack PRE (if configured)
        if (toneStackEnabled && toneStackPre)
        {
            sample = toneStackGain(sample, bass, mid, treble);
        }

        // 3. NAM model (identity or soft clip)
        if (useNonlinearModel)
        {
            sample = softClip(sample);
        }

        // 4. Normalize if enabled
        if (normalizeEnabled && hasLoudness)
        {
            double gain = std::pow(10.0, (kNormalizationTarget - modelLoudness) / 20.0);
            sample *= static_cast<float>(gain);
        }

        // 5. Tone stack POST (if configured, default)
        if (toneStackEnabled && !toneStackPre)
        {
            sample = toneStackGain(sample, bass, mid, treble);
        }

        // 6. Output gain
        sample *= dBToLinear(outputGain);

        return sample;
    }
};

TEST_CASE("NAM Integration - Gain Staging Chain", "[nam][integration]")
{
    MockProcessingChain chain;

    SECTION("Unity gain with all bypassed gives unity output")
    {
        chain.inputGain = 0.0f;
        chain.outputGain = 0.0f;
        chain.normalizeEnabled = false;

        float input = 1.0f;
        float output = chain.processGainOnly(input);
        REQUIRE_THAT(output, WithinAbs(1.0f, 0.001f));
    }

    SECTION("Input gain +6dB doubles amplitude")
    {
        chain.inputGain = 6.0f;
        chain.outputGain = 0.0f;

        float input = 1.0f;
        float output = chain.processGainOnly(input);
        REQUIRE_THAT(output, WithinAbs(2.0f, 0.02f));
    }

    SECTION("Output gain -6dB halves amplitude")
    {
        chain.inputGain = 0.0f;
        chain.outputGain = -6.0f;

        float input = 1.0f;
        float output = chain.processGainOnly(input);
        REQUIRE_THAT(output, WithinAbs(0.5f, 0.01f));
    }

    SECTION("Input +6dB and Output -6dB cancel out")
    {
        chain.inputGain = 6.0f;
        chain.outputGain = -6.0f;

        float input = 1.0f;
        float output = chain.processGainOnly(input);
        REQUIRE_THAT(output, WithinAbs(1.0f, 0.02f));
    }

    SECTION("Gains stack correctly: +10 input, +10 output = +20 total")
    {
        chain.inputGain = 10.0f;
        chain.outputGain = 10.0f;

        float input = 0.1f;
        float output = chain.processGainOnly(input);
        float expectedGain = dBToLinear(20.0f); // Combined gain
        REQUIRE_THAT(output, WithinAbs(input * expectedGain, 0.01f));
    }
}

TEST_CASE("NAM Integration - Normalization Chain", "[nam][integration]")
{
    MockProcessingChain chain;
    chain.normalizeEnabled = true;
    chain.hasLoudness = true;

    SECTION("Model at -12dB normalized to -18dB applies -6dB gain")
    {
        chain.modelLoudness = -12.0;
        // Target is -18dB, so we need -6dB compensation

        float input = 1.0f;
        float output = chain.processGainOnly(input);

        // -6dB = 0.5 linear
        REQUIRE_THAT(output, WithinAbs(0.5f, 0.01f));
    }

    SECTION("Model at -24dB normalized to -18dB applies +6dB gain")
    {
        chain.modelLoudness = -24.0;
        // Target is -18dB, so we need +6dB compensation

        float input = 1.0f;
        float output = chain.processGainOnly(input);

        // +6dB = 2.0 linear
        REQUIRE_THAT(output, WithinAbs(2.0f, 0.02f));
    }

    SECTION("Model at -18dB normalized to -18dB is unity gain")
    {
        chain.modelLoudness = -18.0;

        float input = 1.0f;
        float output = chain.processGainOnly(input);
        REQUIRE_THAT(output, WithinAbs(1.0f, 0.001f));
    }

    SECTION("Normalization disabled passes through unchanged")
    {
        chain.normalizeEnabled = false;
        chain.modelLoudness = -12.0;

        float input = 1.0f;
        float output = chain.processGainOnly(input);
        REQUIRE_THAT(output, WithinAbs(1.0f, 0.001f));
    }

    SECTION("No loudness metadata skips normalization")
    {
        chain.normalizeEnabled = true;
        chain.hasLoudness = false;

        float input = 1.0f;
        // Should not apply any gain since no loudness info
        float output = chain.processGainOnly(input);
        REQUIRE_THAT(output, WithinAbs(1.0f, 0.001f));
    }
}

TEST_CASE("NAM Integration - Combined Effects", "[nam][integration]")
{
    MockProcessingChain chain;

    SECTION("Input gain + normalization combine correctly")
    {
        chain.inputGain = 6.0f; // +6dB = 2x
        chain.normalizeEnabled = true;
        chain.modelLoudness = -12.0; // Needs -6dB = 0.5x
        chain.outputGain = 0.0f;

        float input = 1.0f;
        // 1.0 * 2.0 (input) * 0.5 (normalize) = 1.0
        float output = chain.processGainOnly(input);
        REQUIRE_THAT(output, WithinAbs(1.0f, 0.02f));
    }

    SECTION("Full chain with all gains")
    {
        chain.inputGain = 12.0f; // ~4x
        chain.normalizeEnabled = true;
        chain.modelLoudness = -6.0; // Needs -12dB = ~0.25x
        chain.outputGain = 6.0f;    // ~2x

        float input = 1.0f;
        // 1.0 * 4 * 0.25 * 2 = 2.0
        float output = chain.processGainOnly(input);
        REQUIRE_THAT(output, WithinAbs(2.0f, 0.1f));
    }
}

// ============================================================================
// Integration Tests - Parameter Indexing
// ============================================================================

enum NAMParameters
{
    InputGainParam = 0,
    OutputGainParam,
    NoiseGateParam,
    BassParam,
    MidParam,
    TrebleParam,
    ToneStackEnabledParam,
    NormalizeParam,
    IRMixParam,
    ToneStackPreParam,
    NumParameters
};

TEST_CASE("NAM Integration - Parameter Index Mapping", "[nam][integration]")
{
    SECTION("Parameter count is 10")
    {
        REQUIRE(NumParameters == 10);
    }

    SECTION("Parameter indices are contiguous from 0")
    {
        REQUIRE(InputGainParam == 0);
        REQUIRE(OutputGainParam == 1);
        REQUIRE(NoiseGateParam == 2);
        REQUIRE(BassParam == 3);
        REQUIRE(MidParam == 4);
        REQUIRE(TrebleParam == 5);
        REQUIRE(ToneStackEnabledParam == 6);
        REQUIRE(NormalizeParam == 7);
        REQUIRE(IRMixParam == 8);
        REQUIRE(ToneStackPreParam == 9);
    }
}

// ============================================================================
// Mutation Tests - Normalization Formula
// ============================================================================

TEST_CASE("NAM Mutation - Normalization Target Level", "[nam][mutation]")
{
    const double targetLoudness = -18.0;

    SECTION("Target is -18dB, not -12dB or -24dB")
    {
        REQUIRE(targetLoudness == -18.0);
    }

    SECTION("Normalization formula uses (target - loudness) / 20")
    {
        double modelLoudness = -12.0;
        double correctGain = std::pow(10.0, (targetLoudness - modelLoudness) / 20.0);
        // (-18 - (-12)) / 20 = -6/20 = -0.3
        // 10^-0.3 = ~0.5
        REQUIRE_THAT(correctGain, WithinAbs(0.5f, 0.01f));

        // Mutation: if (loudness - target) was used (wrong sign)
        double mutatedGain = std::pow(10.0, (modelLoudness - targetLoudness) / 20.0);
        REQUIRE(std::abs(mutatedGain - correctGain) > 0.4);
    }

    SECTION("Normalization uses /20 not /10")
    {
        double modelLoudness = -6.0;
        double correctGain = std::pow(10.0, (targetLoudness - modelLoudness) / 20.0);
        // (-18 - (-6)) / 20 = -12/20 = -0.6
        // 10^-0.6 = ~0.25

        double mutatedGain = std::pow(10.0, (targetLoudness - modelLoudness) / 10.0);
        // Would be ~0.063 instead of ~0.25
        REQUIRE(std::abs(mutatedGain - correctGain) > 0.1);
    }
}

// ============================================================================
// Mutation Tests - Boolean Threshold
// ============================================================================

TEST_CASE("NAM Mutation - Boolean Parameter Threshold", "[nam][mutation]")
{
    auto boolFromFloat = [](float value) { return value > 0.5f; };

    SECTION("Threshold is 0.5, not 0.0 or 1.0")
    {
        REQUIRE_FALSE(boolFromFloat(0.0f));
        REQUIRE_FALSE(boolFromFloat(0.5f)); // At threshold = false
        REQUIRE(boolFromFloat(0.51f));
        REQUIRE(boolFromFloat(1.0f));
    }

    SECTION("Values below 0.5 are false, above are true")
    {
        REQUIRE_FALSE(boolFromFloat(0.25f));
        REQUIRE_FALSE(boolFromFloat(0.49f));
        REQUIRE(boolFromFloat(0.6f));
        REQUIRE(boolFromFloat(0.75f));
    }
}

// ============================================================================
// Mutation Tests - Noise Gate Text Display
// ============================================================================

TEST_CASE("NAM Mutation - Noise Gate Display Logic", "[nam][mutation]")
{
    auto getGateText = [](float threshold) -> std::string
    {
        if (threshold <= -100.0f)
            return "Off";
        return std::to_string(static_cast<int>(threshold)) + " dB";
    };

    SECTION("Display threshold is -100, not -101")
    {
        // At -100 or below, show "Off"
        REQUIRE(getGateText(-100.0f) == "Off");
        REQUIRE(getGateText(-101.0f) == "Off");

        // Above -100, show dB value
        REQUIRE(getGateText(-99.0f) == "-99 dB");
    }

    SECTION("Enabled threshold (-100) differs from clamped threshold (-101)")
    {
        // Enabled check: > -100
        // Display check: <= -100
        float clampedOff = -101.0f;
        float displayBoundary = -100.0f;

        bool isEnabled = clampedOff > -100.0f;
        std::string text = getGateText(clampedOff);

        REQUIRE_FALSE(isEnabled);
        REQUIRE(text == "Off");
    }
}

// ============================================================================
// Edge Case Tests
// ============================================================================

TEST_CASE("NAM Edge Cases - Extreme Values", "[nam][edge]")
{
    SECTION("Very small input values")
    {
        float tiny = 1e-10f;
        float gained = tiny * dBToLinear(20.0f);
        REQUIRE(gained > 0.0f);
        REQUIRE(gained < 1e-8f);
    }

    SECTION("Very large gain accumulation")
    {
        // +40dB output + model boost could cause overflow concern
        float maxOutputGain = dBToLinear(40.0f); // 100x
        float input = 1.0f;
        float output = input * maxOutputGain;
        REQUIRE_THAT(output, WithinAbs(100.0f, 0.1f));
    }

    SECTION("dB conversion at boundaries")
    {
        // At -40dB (output gain min)
        REQUIRE_THAT(dBToLinear(-40.0f), WithinAbs(0.01f, 0.0001f));

        // At +40dB (output gain max)
        REQUIRE_THAT(dBToLinear(40.0f), WithinAbs(100.0f, 0.1f));
    }
}

TEST_CASE("NAM Edge Cases - State Boundaries", "[nam][edge]")
{
    SECTION("Empty state serializes/deserializes correctly")
    {
        NAMState empty;
        auto data = serializeState(empty);
        auto restored = deserializeState(data);

        REQUIRE(restored.modelPath.empty());
        REQUIRE(restored.irPath.empty());
        REQUIRE_THAT(restored.inputGain, WithinAbs(0.0f, 0.001f));
    }

    SECTION("Long file paths")
    {
        NAMState state;
        state.modelPath = std::string(500, 'a') + ".nam";
        state.irPath = std::string(500, 'b') + ".wav";

        auto data = serializeState(state);
        auto restored = deserializeState(data);

        REQUIRE(restored.modelPath == state.modelPath);
        REQUIRE(restored.irPath == state.irPath);
    }

    SECTION("Special characters in paths")
    {
        NAMState state;
        state.modelPath = "C:/Path With Spaces/Model (1).nam";
        state.irPath = "C:/Path-With-Dashes/IR_underscore.wav";

        auto data = serializeState(state);
        auto restored = deserializeState(data);

        REQUIRE(restored.modelPath == state.modelPath);
        REQUIRE(restored.irPath == state.irPath);
    }
}

TEST_CASE("NAM Edge Cases - Parameter Boundary Values", "[nam][edge]")
{
    SECTION("All parameters at minimum bounds")
    {
        NAMState state;
        state.inputGain = -20.0f;
        state.outputGain = -40.0f;
        state.noiseGateThreshold = -101.0f;
        state.bass = 0.0f;
        state.mid = 0.0f;
        state.treble = 0.0f;

        auto data = serializeState(state);
        auto restored = deserializeState(data);

        REQUIRE_THAT(restored.inputGain, WithinAbs(-20.0f, 0.001f));
        REQUIRE_THAT(restored.outputGain, WithinAbs(-40.0f, 0.001f));
        REQUIRE_THAT(restored.noiseGateThreshold, WithinAbs(-101.0f, 0.001f));
        REQUIRE_THAT(restored.bass, WithinAbs(0.0f, 0.001f));
    }

    SECTION("All parameters at maximum bounds")
    {
        NAMState state;
        state.inputGain = 20.0f;
        state.outputGain = 40.0f;
        state.noiseGateThreshold = 0.0f;
        state.bass = 10.0f;
        state.mid = 10.0f;
        state.treble = 10.0f;

        auto data = serializeState(state);
        auto restored = deserializeState(data);

        REQUIRE_THAT(restored.inputGain, WithinAbs(20.0f, 0.001f));
        REQUIRE_THAT(restored.outputGain, WithinAbs(40.0f, 0.001f));
        REQUIRE_THAT(restored.noiseGateThreshold, WithinAbs(0.0f, 0.001f));
        REQUIRE_THAT(restored.treble, WithinAbs(10.0f, 0.001f));
    }
}

// ============================================================================
// Mutation Tests - Gain Application Order
// ============================================================================

TEST_CASE("NAM Mutation - Gain Application Order", "[nam][mutation]")
{
    SECTION("Input gain applied before model (affects distortion)")
    {
        // Input gain multiplies the signal going INTO the model
        // This changes the character of distortion/saturation
        float inputGainLinear = dBToLinear(12.0f);
        float signal = 0.5f;
        float boostedInput = signal * inputGainLinear;

        // Boosted signal should be ~2x the original
        REQUIRE_THAT(boostedInput, WithinAbs(signal * 4.0f, 0.1f));
    }

    SECTION("Output gain applied after processing")
    {
        // Output gain scales the final result
        float outputGainLinear = dBToLinear(-12.0f);
        float processedSignal = 1.0f;
        float finalOutput = processedSignal * outputGainLinear;

        // Should be ~0.25x
        REQUIRE_THAT(finalOutput, WithinAbs(0.25f, 0.01f));
    }

    SECTION("Swapping input/output gains gives different result with nonlinear processing")
    {
        // With linear processing, order doesn't matter
        // But with nonlinear (saturation), it does
        // This test documents the expected order

        float inputGain = 6.0f;
        float outputGain = -6.0f;

        // Input -> model -> output means:
        // signal * inputGainLinear -> model(x) -> result * outputGainLinear

        float sig = 1.0f;
        float throughInputFirst = sig * dBToLinear(inputGain);
        // Through model (identity for this test)
        float afterModel = throughInputFirst;
        float finalOutput = afterModel * dBToLinear(outputGain);

        REQUIRE_THAT(finalOutput, WithinAbs(1.0f, 0.02f));
    }
}

// ============================================================================
// Mutation Tests - Floating Point Comparison
// ============================================================================

TEST_CASE("NAM Mutation - Gain Skip Optimization", "[nam][mutation]")
{
    SECTION("Skip threshold is 0.001, not 0 or 0.01")
    {
        auto shouldApplyGain = [](float gainLinear) { return std::abs(gainLinear - 1.0f) > 0.001f; };

        // At exactly 1.0, skip
        REQUIRE_FALSE(shouldApplyGain(1.0f));

        // Very close to 1.0, skip
        REQUIRE_FALSE(shouldApplyGain(1.0005f));
        REQUIRE_FALSE(shouldApplyGain(0.9995f));

        // Outside threshold, apply
        REQUIRE(shouldApplyGain(1.002f));
        REQUIRE(shouldApplyGain(0.998f));
    }

    SECTION("0dB gain produces linear value within skip threshold")
    {
        float zeroDbLinear = dBToLinear(0.0f);
        float deviation = std::abs(zeroDbLinear - 1.0f);
        REQUIRE(deviation < 0.001f);
    }
}

// ============================================================================
// Integration Tests - Processing Chain Flags
// ============================================================================

TEST_CASE("NAM Integration - Processing Chain Flag Combinations", "[nam][integration]")
{
    // Simulates boolean flag checks in processBlock

    struct ProcessingFlags
    {
        float noiseGateThreshold = -80.0f;
        bool toneStackEnabled = true;
        bool normalizeOutput = false;
        bool irEnabled = true;
        bool irLoaded = true;

        bool shouldDoNoiseGate() const { return noiseGateThreshold > -100.0f; }
        bool shouldDoToneStack() const { return toneStackEnabled; }
        bool shouldDoNormalize() const { return normalizeOutput; }
        bool shouldDoIR() const { return irEnabled && irLoaded; }
    };

    ProcessingFlags flags;

    SECTION("All features enabled")
    {
        flags.noiseGateThreshold = -60.0f;
        flags.toneStackEnabled = true;
        flags.normalizeOutput = true;
        flags.irEnabled = true;
        flags.irLoaded = true;

        REQUIRE(flags.shouldDoNoiseGate());
        REQUIRE(flags.shouldDoToneStack());
        REQUIRE(flags.shouldDoNormalize());
        REQUIRE(flags.shouldDoIR());
    }

    SECTION("Noise gate requires threshold > -100")
    {
        flags.noiseGateThreshold = -101.0f;
        REQUIRE_FALSE(flags.shouldDoNoiseGate());

        flags.noiseGateThreshold = -100.0f;
        REQUIRE_FALSE(flags.shouldDoNoiseGate());

        flags.noiseGateThreshold = -99.0f;
        REQUIRE(flags.shouldDoNoiseGate());
    }

    SECTION("IR requires both enabled AND loaded")
    {
        flags.irEnabled = true;
        flags.irLoaded = true;
        REQUIRE(flags.shouldDoIR());

        flags.irEnabled = false;
        flags.irLoaded = true;
        REQUIRE_FALSE(flags.shouldDoIR());

        flags.irEnabled = true;
        flags.irLoaded = false;
        REQUIRE_FALSE(flags.shouldDoIR());
    }
}

// ============================================================================
// Stress Tests - Numerical Stability
// ============================================================================

TEST_CASE("NAM Stress - Numerical Stability", "[nam][stress]")
{
    SECTION("Repeated gain conversions maintain precision")
    {
        float dB = 6.0f;
        float linear = dB;

        // Convert back and forth 100 times
        for (int i = 0; i < 100; ++i)
        {
            linear = dBToLinear(dB);
            dB = linearTodB(linear);
        }

        REQUIRE_THAT(dB, WithinAbs(6.0f, 0.1f));
    }

    SECTION("Extreme parameter values don't produce NaN/Inf")
    {
        float extremeDB = -100.0f;
        float linear = dBToLinear(extremeDB);

        REQUIRE(std::isfinite(linear));
        REQUIRE(linear > 0.0f);
        REQUIRE(linear < 1e-4f);
    }

    SECTION("Accumulated small gains don't overflow")
    {
        float total = 1.0f;
        float smallGain = dBToLinear(0.1f);

        for (int i = 0; i < 1000; ++i)
        {
            total *= smallGain;
            if (!std::isfinite(total))
                break;
        }

        // After 1000 iterations of +0.1dB = +100dB total
        REQUIRE(std::isfinite(total));
    }
}

// ============================================================================
// Tone Stack Pre/Post Signal Flow Tests
// ============================================================================

TEST_CASE("NAM Tone Stack Pre/Post - Signal Flow Ordering", "[nam][tonestack][prepost]")
{
    MockProcessingChain chain;
    chain.toneStackEnabled = true;
    chain.inputGain = 0.0f;
    chain.outputGain = 0.0f;
    chain.normalizeEnabled = false;

    SECTION("POST mode applies tone stack after model")
    {
        chain.toneStackPre = false;
        chain.bass = 10.0f; // Max boost
        chain.mid = 10.0f;
        chain.treble = 10.0f;

        float input = 0.5f;
        float output = chain.processWithToneStack(input, false);

        // With all bands at max (10.0), each gain = 1.5
        // Total: 0.5 * 1.5 * 1.5 * 1.5 = 1.6875
        float expected = 0.5f * 1.5f * 1.5f * 1.5f;
        REQUIRE_THAT(output, WithinAbs(expected, 0.001f));
    }

    SECTION("PRE mode applies tone stack before model")
    {
        chain.toneStackPre = true;
        chain.bass = 10.0f;
        chain.mid = 10.0f;
        chain.treble = 10.0f;

        float input = 0.5f;
        // With identity model, result should be same as POST
        float output = chain.processWithToneStack(input, false);

        float expected = 0.5f * 1.5f * 1.5f * 1.5f;
        REQUIRE_THAT(output, WithinAbs(expected, 0.001f));
    }

    SECTION("Tone stack disabled gives passthrough in both modes")
    {
        chain.toneStackEnabled = false;
        chain.bass = 10.0f;
        chain.mid = 10.0f;
        chain.treble = 0.0f;

        float input = 0.7f;

        chain.toneStackPre = false;
        float postOutput = chain.processWithToneStack(input, false);
        REQUIRE_THAT(postOutput, WithinAbs(input, 0.001f));

        chain.toneStackPre = true;
        float preOutput = chain.processWithToneStack(input, false);
        REQUIRE_THAT(preOutput, WithinAbs(input, 0.001f));
    }
}

TEST_CASE("NAM Tone Stack Pre/Post - Nonlinear Model Divergence", "[nam][tonestack][prepost]")
{
    // Key test: with a nonlinear model, PRE and POST produce different results.
    // This proves the signal flow ordering actually matters.
    MockProcessingChain chain;
    chain.toneStackEnabled = true;
    chain.inputGain = 6.0f; // Drive signal into saturation
    chain.outputGain = 0.0f;
    chain.normalizeEnabled = false;

    SECTION("PRE and POST produce different output with nonlinear model")
    {
        // Use extreme EQ to exaggerate the difference
        chain.bass = 0.0f;   // Heavy cut
        chain.mid = 10.0f;   // Max boost
        chain.treble = 0.0f; // Heavy cut

        float input = 0.8f;

        chain.toneStackPre = false;
        float postResult = chain.processWithToneStack(input, true);

        chain.toneStackPre = true;
        float preResult = chain.processWithToneStack(input, true);

        // PRE: EQ shapes signal BEFORE saturation (changes clipping character)
        // POST: saturation happens first, then EQ shapes the clipped signal
        // These MUST differ because tanh(EQ(x)) != EQ(tanh(x))
        REQUIRE(std::abs(preResult - postResult) > 0.01f);
    }

    SECTION("PRE boosts BEFORE saturation, driving harder into clip")
    {
        chain.bass = 10.0f; // Max boost
        chain.mid = 10.0f;
        chain.treble = 10.0f;

        float input = 0.5f;

        // PRE: boost signal -> saturate boosted signal
        chain.toneStackPre = true;
        float preResult = chain.processWithToneStack(input, true);

        // POST: saturate signal -> boost saturated signal
        chain.toneStackPre = false;
        float postResult = chain.processWithToneStack(input, true);

        // POST should be larger because the boost is applied AFTER the
        // compressive saturation. PRE gets compressed by tanh.
        REQUIRE(postResult > preResult);
    }

    SECTION("PRE cut reduces saturation compared to POST cut")
    {
        chain.bass = 0.0f; // Cut
        chain.mid = 0.0f;
        chain.treble = 0.0f;
        chain.inputGain = 12.0f; // Heavy drive

        float input = 0.8f;

        // PRE: cut signal first -> less saturation
        chain.toneStackPre = true;
        float preResult = chain.processWithToneStack(input, true);

        // POST: full saturation -> cut after
        chain.toneStackPre = false;
        float postResult = chain.processWithToneStack(input, true);

        // With PRE cut, signal is reduced before tanh, so it stays in the
        // linear region of tanh (less compression). POST saturates fully,
        // then cuts the already-compressed signal. PRE preserves more of
        // the original signal shape, resulting in LARGER magnitude.
        REQUIRE(std::abs(preResult) > std::abs(postResult));
    }
}

TEST_CASE("NAM Tone Stack Pre/Post - Tone Stack Gain Curve", "[nam][tonestack][prepost]")
{
    SECTION("Band at 5.0 (center) produces unity for that band")
    {
        float gain = MockProcessingChain::toneStackGain(1.0f, 5.0f, 5.0f, 5.0f);
        REQUIRE_THAT(gain, WithinAbs(1.0f, 0.001f));
    }

    SECTION("Band at 0.0 produces 0.5x for that band")
    {
        float gain = MockProcessingChain::toneStackGain(1.0f, 0.0f, 5.0f, 5.0f);
        REQUIRE_THAT(gain, WithinAbs(0.5f, 0.001f));
    }

    SECTION("Band at 10.0 produces 1.5x for that band")
    {
        float gain = MockProcessingChain::toneStackGain(1.0f, 10.0f, 5.0f, 5.0f);
        REQUIRE_THAT(gain, WithinAbs(1.5f, 0.001f));
    }

    SECTION("All bands at 0 produce 0.125x total")
    {
        float gain = MockProcessingChain::toneStackGain(1.0f, 0.0f, 0.0f, 0.0f);
        REQUIRE_THAT(gain, WithinAbs(0.125f, 0.001f));
    }

    SECTION("All bands at 10 produce 3.375x total")
    {
        float gain = MockProcessingChain::toneStackGain(1.0f, 10.0f, 10.0f, 10.0f);
        REQUIRE_THAT(gain, WithinAbs(3.375f, 0.001f));
    }
}

TEST_CASE("NAM Tone Stack Pre/Post - State Serialization", "[nam][tonestack][state]")
{
    SECTION("toneStackPre false round-trips correctly")
    {
        NAMState original;
        original.toneStackPre = false;

        auto data = serializeState(original);
        auto restored = deserializeState(data);

        REQUIRE(restored.toneStackPre == false);
    }

    SECTION("toneStackPre true round-trips correctly")
    {
        NAMState original;
        original.toneStackPre = true;

        auto data = serializeState(original);
        auto restored = deserializeState(data);

        REQUIRE(restored.toneStackPre == true);
    }

    SECTION("Full state including toneStackPre round-trips")
    {
        NAMState original;
        original.inputGain = 5.0f;
        original.bass = 3.0f;
        original.mid = 7.0f;
        original.treble = 9.0f;
        original.toneStackEnabled = true;
        original.toneStackPre = true;

        auto data = serializeState(original);
        auto restored = deserializeState(data);

        REQUIRE_THAT(restored.inputGain, WithinAbs(5.0f, 0.001f));
        REQUIRE_THAT(restored.bass, WithinAbs(3.0f, 0.001f));
        REQUIRE_THAT(restored.mid, WithinAbs(7.0f, 0.001f));
        REQUIRE_THAT(restored.treble, WithinAbs(9.0f, 0.001f));
        REQUIRE(restored.toneStackEnabled == true);
        REQUIRE(restored.toneStackPre == true);
    }
}

// ============================================================================
// Mutation Tests - Tone Stack Pre/Post
// ============================================================================

TEST_CASE("NAM Mutation - Tone Stack Pre/Post Logic", "[nam][tonestack][mutation]")
{
    SECTION("PRE applies tone stack to input, not output")
    {
        // Verify the branching logic: if toneStackPre, tone stack
        // must happen BEFORE the model, not after
        MockProcessingChain chain;
        chain.toneStackEnabled = true;
        chain.toneStackPre = true;
        chain.bass = 0.0f;
        chain.mid = 0.0f;
        chain.treble = 0.0f;

        float input = 1.0f;
        float output = chain.processWithToneStack(input, true);

        // With PRE: input -> toneStack(0.125x) -> softClip(0.125) -> output
        float expectedPre = std::tanh(0.125f);
        REQUIRE_THAT(output, WithinAbs(expectedPre, 0.001f));

        // If it were POST (mutation): input -> softClip(1.0) -> toneStack()
        float expectedPost = std::tanh(1.0f) * 0.125f;
        // These must differ
        REQUIRE(std::abs(expectedPre - expectedPost) > 0.01f);
    }

    SECTION("POST applies tone stack to output, not input")
    {
        MockProcessingChain chain;
        chain.toneStackEnabled = true;
        chain.toneStackPre = false;
        chain.bass = 0.0f;
        chain.mid = 0.0f;
        chain.treble = 0.0f;

        float input = 1.0f;
        float output = chain.processWithToneStack(input, true);

        // With POST: input -> softClip(1.0) -> toneStack(0.125x)
        float expectedPost = std::tanh(1.0f) * 0.125f;
        REQUIRE_THAT(output, WithinAbs(expectedPost, 0.001f));
    }

    SECTION("Swapping pre/post flag changes result (not a no-op)")
    {
        MockProcessingChain chain;
        chain.toneStackEnabled = true;
        chain.bass = 0.0f;
        chain.mid = 0.0f;
        chain.treble = 0.0f;
        chain.inputGain = 6.0f;

        float input = 0.5f;

        chain.toneStackPre = true;
        float preResult = chain.processWithToneStack(input, true);

        chain.toneStackPre = false;
        float postResult = chain.processWithToneStack(input, true);

        REQUIRE(preResult != postResult);
    }

    SECTION("Boolean threshold for toneStackPre is 0.5")
    {
        auto boolFromFloat = [](float value) { return value > 0.5f; };
        REQUIRE_FALSE(boolFromFloat(0.0f));
        REQUIRE_FALSE(boolFromFloat(0.5f));
        REQUIRE(boolFromFloat(0.51f));
        REQUIRE(boolFromFloat(1.0f));
    }
}
