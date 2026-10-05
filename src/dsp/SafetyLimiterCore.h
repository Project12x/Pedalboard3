#pragma once
#include <array>

// Extracted from Pedalboard's existing limiter; deliberately preserves its policy.
// prepare requires stopped processing. reset is bounded, allocation-free DSP state only.
class SafetyLimiterCore
{
public:
    void prepare(double sampleRate);
    void reset() noexcept;
    bool process(float* const* data, int channels, int samples) noexcept;
    bool isLimiting() const noexcept { return limiting; }
private:
    static constexpr float softLimitThreshold = 0.944f;
    static constexpr float dangerousGainThreshold = 2.0f;
    static constexpr float dcOffsetThreshold = 0.5f;
    int dangerousGainHoldSamples = 0, dcOffsetHoldSamples = 0, ultrasonicHoldSamples = 0;
    int dangerousGainCounter = 0, dcOffsetCounter = 0, ultrasonicCounter = 0;
    std::array<float, 16> dcBlockerPreviousInput{}, dcBlockerPreviousOutput{}, ultrasonicPreviousSample{};
    float dcBlockerCoeff = 0.995f, ultrasonicEnergy = 0.0f, ultrasonicDecay = 0.99f;
    float currentGain = 1.0f, releaseCoeff = 0.0f;
    bool limiting = false;
};
