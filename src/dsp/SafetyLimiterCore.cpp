#include "SafetyLimiterCore.h"
#include <algorithm>
#include <cmath>

void SafetyLimiterCore::prepare(double sampleRate)
{
    dangerousGainHoldSamples = static_cast<int>(sampleRate * 0.1);
    dcOffsetHoldSamples = static_cast<int>(sampleRate * 0.5);
    ultrasonicHoldSamples = static_cast<int>(sampleRate * 0.2);
    releaseCoeff = std::exp(-1.0f / static_cast<float>(sampleRate * 0.05));
    reset();
}

void SafetyLimiterCore::reset() noexcept
{
    currentGain = 1.0f;
    dangerousGainCounter = dcOffsetCounter = ultrasonicCounter = 0;
    ultrasonicEnergy = 0.0f;
    dcBlockerPreviousInput.fill(0.0f);
    dcBlockerPreviousOutput.fill(0.0f);
    ultrasonicPreviousSample.fill(0.0f);
    limiting = false;
}

bool SafetyLimiterCore::process(float* const* data, int numChannels, int numSamples) noexcept
{
    const int chCount = std::min(numChannels, 16);

    if (chCount <= 0 || numSamples <= 0)
        return false;

    bool limitingThisBlock = false;

    auto clear = [&] { for (int ch = 0; ch < chCount; ++ch) if (data[ch]) std::fill_n(data[ch], numSamples, 0.0f); };

    for (int sample = 0; sample < numSamples; ++sample)
    {
        float maxPeak = 0.0f;
        float dcSum = 0.0f;
        float dcBlockedSamples[16] = {};

        for (int ch = 0; ch < chCount; ++ch)
        {
            float* channelData = data[ch];
            if (!channelData) continue;
            float inputSample = channelData[sample];

            if (!std::isfinite(inputSample))
            {
                clear();
                return true;
            }

            // DC Blocker (high-pass filter)
            const float dcBlockedSample =
                inputSample - dcBlockerPreviousInput[ch] + dcBlockerCoeff * dcBlockerPreviousOutput[ch];
            // Finite inputs can still overflow the recurrence. Trip before a
            // clamp can disguise infinity as apparently safe full-scale audio.
            if (!std::isfinite(dcBlockedSample))
            {
                clear();
                return true;
            }
            dcBlockerPreviousInput[ch] = inputSample;
            dcBlockerPreviousOutput[ch] = dcBlockedSample;
            dcBlockedSamples[ch] = dcBlockedSample;

            // Track DC offset (before blocking)
            dcSum += std::abs(inputSample - dcBlockedSample);

            // Track peak for limiting/danger detection
            float absSample = std::abs(dcBlockedSample);
            maxPeak = std::max(maxPeak, absSample);

            // Simple ultrasonic detection (track high-frequency energy)
            // This is a rough approximation - we detect large sample-to-sample changes
            float delta = std::abs(dcBlockedSample - ultrasonicPreviousSample[ch]);
            ultrasonicPreviousSample[ch] = dcBlockedSample;
            ultrasonicEnergy = ultrasonicEnergy * ultrasonicDecay + delta * delta;
        }

        if (maxPeak > softLimitThreshold)
        {
            // Linked output protection: one gain value is applied to every active
            // channel so stereo image and phase relationships stay intact.
            const float excess = maxPeak - softLimitThreshold;
            const float reduction = excess / (1.0f + excess);
            const float targetGain = (softLimitThreshold + reduction) / maxPeak;
            currentGain = std::min(currentGain, targetGain);
            limitingThisBlock = true;
        }
        else
        {
            // Release gain back to 1.0
            currentGain = currentGain * releaseCoeff + (1.0f - releaseCoeff);
        }

        for (int ch = 0; ch < chCount; ++ch)
        {
            float* channelData = data[ch];
            if (channelData) channelData[sample] = std::clamp(dcBlockedSamples[ch] * currentGain, -1.0f, 1.0f);
        }

        // Check for dangerous conditions
        if (maxPeak > dangerousGainThreshold)
        {
            dangerousGainCounter++;
        }
        else
        {
            dangerousGainCounter = std::max(0, dangerousGainCounter - 1);
        }

        if ((dcSum / static_cast<float>(chCount)) > dcOffsetThreshold)
        {
            dcOffsetCounter++;
        }
        else
        {
            dcOffsetCounter = std::max(0, dcOffsetCounter - 1);
        }

        // Check ultrasonic (threshold is empirical)
        if (ultrasonicEnergy > 0.1f)
        {
            ultrasonicCounter++;
        }
        else
        {
            ultrasonicCounter = std::max(0, ultrasonicCounter - 1);
        }

        // Trigger auto-mute if any threshold exceeded
        if (dangerousGainCounter > dangerousGainHoldSamples || dcOffsetCounter > dcOffsetHoldSamples ||
            ultrasonicCounter > ultrasonicHoldSamples)
        {
            clear();
            return true;
        }
    }

    limiting = limitingThisBlock;
    return false;
}

