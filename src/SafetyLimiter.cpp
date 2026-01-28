/*
  ==============================================================================

    SafetyLimiter.cpp
    Audio safety protection processor implementation.

  ==============================================================================
*/

#include "SafetyLimiter.h"

SafetyLimiterProcessor::SafetyLimiterProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Input", AudioChannelSet::stereo(), true)
                         .withOutput("Output", AudioChannelSet::stereo(), true))
{
}

void SafetyLimiterProcessor::prepareToPlay(double sampleRate, int /*samplesPerBlock*/)
{
    currentSampleRate = sampleRate;

    // Calculate timing thresholds in samples
    dangerousGainHoldSamples = static_cast<int>(sampleRate * 0.1); // 100ms
    dcOffsetHoldSamples = static_cast<int>(sampleRate * 0.5);      // 500ms
    ultrasonicHoldSamples = static_cast<int>(sampleRate * 0.2);    // 200ms

    // Release coefficient for ~50ms release time
    releaseCoeff = std::exp(-1.0f / static_cast<float>(sampleRate * 0.05));

    // Reset state
    currentGain = 1.0f;
    dangerousGainCounter = 0;
    dcOffsetCounter = 0;
    ultrasonicCounter = 0;
    ultrasonicEnergy = 0.0f;
    dcBlockerState[0] = dcBlockerState[1] = 0.0f;
}

void SafetyLimiterProcessor::releaseResources()
{
    // Nothing to release
}

void SafetyLimiterProcessor::processBlock(AudioBuffer<float>& buffer, MidiBuffer& /*midiMessages*/)
{
    // If muted, output silence
    if (muted.load())
    {
        buffer.clear();
        return;
    }

    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();

    bool limitingThisBlock = false;
    bool dangerousDetected = false;
    bool dcDetected = false;

    for (int sample = 0; sample < numSamples; ++sample)
    {
        float maxPeak = 0.0f;
        float dcSum = 0.0f;

        // Process each channel
        for (int ch = 0; ch < jmin(numChannels, 2); ++ch)
        {
            float* channelData = buffer.getWritePointer(ch);
            float inputSample = channelData[sample];

            // DC Blocker (high-pass filter)
            float dcBlockedSample = inputSample - dcBlockerState[ch] + dcBlockerCoeff * dcBlockerState[ch];
            dcBlockerState[ch] = inputSample;

            // Track DC offset (before blocking)
            dcSum += std::abs(inputSample - dcBlockedSample);

            // Use DC-blocked signal
            inputSample = dcBlockedSample;

            // Track peak for limiting/danger detection
            float absSample = std::abs(inputSample);
            maxPeak = jmax(maxPeak, absSample);

            // Simple ultrasonic detection (track high-frequency energy)
            // This is a rough approximation - we detect large sample-to-sample changes
            if (sample > 0)
            {
                float prevSample = channelData[sample - 1];
                float delta = std::abs(inputSample - prevSample);
                ultrasonicEnergy = ultrasonicEnergy * ultrasonicDecay + delta * delta;
            }

            // Apply limiting
            float outputSample = inputSample;

            if (absSample > softLimitThreshold)
            {
                // Soft knee limiting
                float excess = absSample - softLimitThreshold;
                float reduction = excess / (1.0f + excess);
                float targetGain = (softLimitThreshold + reduction) / absSample;
                currentGain = jmin(currentGain, targetGain);
                limitingThisBlock = true;
            }
            else
            {
                // Release gain back to 1.0
                currentGain = currentGain * releaseCoeff + (1.0f - releaseCoeff);
            }

            outputSample = inputSample * currentGain;

            // Final hard clip at 1.0 (safety net)
            outputSample = jlimit(-1.0f, 1.0f, outputSample);

            channelData[sample] = outputSample;
        }

        // Check for dangerous conditions
        if (maxPeak > dangerousGainThreshold)
        {
            dangerousGainCounter++;
            dangerousDetected = true;
        }
        else
        {
            dangerousGainCounter = jmax(0, dangerousGainCounter - 1);
        }

        if ((dcSum / numChannels) > dcOffsetThreshold)
        {
            dcOffsetCounter++;
            dcDetected = true;
        }
        else
        {
            dcOffsetCounter = jmax(0, dcOffsetCounter - 1);
        }

        // Check ultrasonic (threshold is empirical)
        if (ultrasonicEnergy > 0.1f)
        {
            ultrasonicCounter++;
        }
        else
        {
            ultrasonicCounter = jmax(0, ultrasonicCounter - 1);
        }

        // Trigger auto-mute if any threshold exceeded
        if (dangerousGainCounter > dangerousGainHoldSamples || dcOffsetCounter > dcOffsetHoldSamples ||
            ultrasonicCounter > ultrasonicHoldSamples)
        {
            muted.store(true);
            muteTriggered.store(true);
            buffer.clear();
            return;
        }
    }

    limiting.store(limitingThisBlock);
}
