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

void SafetyLimiterProcessor::prepareToPlay(double sampleRate, int)
{
    core.prepare(sampleRate);
    limiting.store(false, std::memory_order_relaxed);
    inputMeters.prepare(sampleRate, 2);
    outputMeters.prepare(sampleRate, 2);
    setInstance(this);
}

void SafetyLimiterProcessor::requestMute() noexcept
{
    // One bounded atomic operation: a newer mute cancels any pending unlock.
    muteControl.exchange(1, std::memory_order_acq_rel);
    muteTriggered.store(true, std::memory_order_release);
}

void SafetyLimiterProcessor::unmute()
{
    muteControl.fetch_or(2, std::memory_order_release);
}

void SafetyLimiterProcessor::releaseResources() {}

void SafetyLimiterProcessor::processBlock(AudioBuffer<float>& buffer, MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    auto control = muteControl.load(std::memory_order_acquire);
    if ((control & 2u) != 0)
    {
        core.reset(); // fixed arrays/scalars only, owned by this processing thread
        limiting.store(false, std::memory_order_relaxed);
        if (muteControl.compare_exchange_strong(control, 0, std::memory_order_acq_rel))
            resetSerial.fetch_add(1, std::memory_order_release);
    }
    if (isMuted()) { buffer.clear(); return; }
    if (core.process(buffer.getArrayOfWritePointers(), jmin(buffer.getNumChannels(), 2), buffer.getNumSamples()))
    {
        requestMute();
        buffer.clear();
        return;
    }
    limiting.store(core.isLimiting(), std::memory_order_relaxed);
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
