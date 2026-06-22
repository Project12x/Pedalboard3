/*
  ==============================================================================

    SafetyLimiter.cpp
    Audio safety protection processor implementation.

  ==============================================================================
*/

#include "SafetyLimiter.h"

SafetyLimiterProcessor* SafetyLimiterProcessor::instance = nullptr;

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

    resetRuntimeState();
    inputMeters.prepare(sampleRate, 2);
    outputMeters.prepare(sampleRate, 2);

    setInstance(this);
}

void SafetyLimiterProcessor::resetRuntimeState() noexcept
{
    currentGain = 1.0f;
    dangerousGainCounter = 0;
    dcOffsetCounter = 0;
    ultrasonicCounter = 0;
    ultrasonicEnergy = 0.0f;
    dcBlockerPreviousInput[0] = dcBlockerPreviousInput[1] = 0.0f;
    dcBlockerPreviousOutput[0] = dcBlockerPreviousOutput[1] = 0.0f;
    ultrasonicPreviousSample[0] = ultrasonicPreviousSample[1] = 0.0f;
    limiting.store(false, std::memory_order_relaxed);
}

void SafetyLimiterProcessor::unmute()
{
    resetRuntimeState();
    muted.store(false, std::memory_order_release);
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
    const int chCount = jmin(numChannels, 2);

    if (chCount <= 0 || numSamples <= 0)
        return;

    bool limitingThisBlock = false;

    auto triggerMuteAndClear = [&buffer, this]()
    {
        muted.store(true, std::memory_order_release);
        muteTriggered.store(true, std::memory_order_release);
        buffer.clear();
    };

    for (int sample = 0; sample < numSamples; ++sample)
    {
        float maxPeak = 0.0f;
        float dcSum = 0.0f;
        float dcBlockedSamples[2] = {0.0f, 0.0f};

        for (int ch = 0; ch < chCount; ++ch)
        {
            float* channelData = buffer.getWritePointer(ch);
            float inputSample = channelData[sample];

            if (!std::isfinite(inputSample))
            {
                triggerMuteAndClear();
                return;
            }

            // DC Blocker (high-pass filter)
            const float dcBlockedSample =
                inputSample - dcBlockerPreviousInput[ch] + dcBlockerCoeff * dcBlockerPreviousOutput[ch];
            dcBlockerPreviousInput[ch] = inputSample;
            dcBlockerPreviousOutput[ch] = dcBlockedSample;
            dcBlockedSamples[ch] = dcBlockedSample;

            // Track DC offset (before blocking)
            dcSum += std::abs(inputSample - dcBlockedSample);

            // Track peak for limiting/danger detection
            float absSample = std::abs(dcBlockedSample);
            maxPeak = jmax(maxPeak, absSample);

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
            currentGain = jmin(currentGain, targetGain);
            limitingThisBlock = true;
        }
        else
        {
            // Release gain back to 1.0
            currentGain = currentGain * releaseCoeff + (1.0f - releaseCoeff);
        }

        for (int ch = 0; ch < chCount; ++ch)
        {
            float* channelData = buffer.getWritePointer(ch);
            channelData[sample] = jlimit(-1.0f, 1.0f, dcBlockedSamples[ch] * currentGain);
        }

        // Check for dangerous conditions
        if (maxPeak > dangerousGainThreshold)
        {
            dangerousGainCounter++;
        }
        else
        {
            dangerousGainCounter = jmax(0, dangerousGainCounter - 1);
        }

        if ((dcSum / static_cast<float>(chCount)) > dcOffsetThreshold)
        {
            dcOffsetCounter++;
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
            triggerMuteAndClear();
            return;
        }
    }

    limiting.store(limitingThisBlock, std::memory_order_relaxed);

    // Note: Output level metering for the Audio Output VU is handled by
    // MeteringProcessorPlayer::audioDeviceIOCallbackWithContext, which taps
    // the real device output buffers after graph processing completes.
}

void SafetyLimiterProcessor::updateOutputLevelsFromDevice(const float* const* outputData, int numChannels,
                                                          int numSamples)
{
    outputMeters.process(outputData, jmin(numChannels, 2), numSamples);
}

void SafetyLimiterProcessor::updateInputLevelsFromDevice(const float* const* inputData, int numChannels, int numSamples)
{
    inputMeters.process(inputData, jmin(numChannels, 2), numSamples);
}
