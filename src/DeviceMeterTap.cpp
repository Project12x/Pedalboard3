//  DeviceMeterTap.cpp - Device-level audio metering for I/O nodes
//  ----------------------------------------------------------------------------
//  This file is part of Pedalboard3, an audio plugin host.
//  Copyright (c) 2024.
//
//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.
//  ----------------------------------------------------------------------------

#include "DeviceMeterTap.h"
#include <cmath>

// Static instance pointer
DeviceMeterTap* DeviceMeterTap::instance = nullptr;

//------------------------------------------------------------------------------
DeviceMeterTap::DeviceMeterTap()
{
    // Initialize all levels to zero
    for (int i = 0; i < MaxChannels; ++i)
    {
        inputLevels[i].store(0.0f, std::memory_order_relaxed);
        outputLevels[i].store(0.0f, std::memory_order_relaxed);
    }
}

//------------------------------------------------------------------------------
DeviceMeterTap::~DeviceMeterTap()
{
    if (instance == this)
        instance = nullptr;
}

//------------------------------------------------------------------------------
void DeviceMeterTap::audioDeviceIOCallbackWithContext(
    const float* const* inputChannelData,
    int numInputChannels,
    float* const* outputChannelData,
    int numOutputChannels,
    int numSamples,
    const juce::AudioIODeviceCallbackContext& /*context*/)
{
    // Update input levels from device inputs
    int inputCount = juce::jmin(numInputChannels, MaxChannels);
    for (int ch = 0; ch < inputCount; ++ch)
    {
        if (inputChannelData[ch] != nullptr)
            updateLevel(inputLevels[ch], inputChannelData[ch], numSamples);
    }
    // Zero out unused input channels
    for (int ch = inputCount; ch < MaxChannels; ++ch)
        inputLevels[ch].store(0.0f, std::memory_order_relaxed);

    // Update output levels from processed output buffer
    // (contains audio from graphPlayer callback that ran before us)
    int outputCount = juce::jmin(numOutputChannels, MaxChannels);
    for (int ch = 0; ch < outputCount; ++ch)
    {
        if (outputChannelData[ch] != nullptr)
            updateLevel(outputLevels[ch], outputChannelData[ch], numSamples);
    }
    // Zero out unused output channels
    for (int ch = outputCount; ch < MaxChannels; ++ch)
        outputLevels[ch].store(0.0f, std::memory_order_relaxed);

    numInputs.store(numInputChannels, std::memory_order_relaxed);
    numOutputs.store(numOutputChannels, std::memory_order_relaxed);

    // CRITICAL: Zero our output contribution!
    // When JUCE has multiple callbacks, it mixes their outputs together.
    // If we don't zero our output buffers, garbage gets mixed into the audio.
    for (int ch = 0; ch < numOutputChannels; ++ch)
    {
        if (outputChannelData[ch] != nullptr)
            juce::FloatVectorOperations::clear(outputChannelData[ch], numSamples);
    }
}

//------------------------------------------------------------------------------
void DeviceMeterTap::audioDeviceAboutToStart(juce::AudioIODevice* device)
{
    if (device != nullptr)
    {
        // Store device name for display
        deviceName = device->getName();

        // Calculate decay coefficient based on sample rate
        // Target: ~3 second decay time from 1.0 to ~0.001 (-60dB)
        double sampleRate = device->getCurrentSampleRate();
        if (sampleRate > 0)
        {
            // decayCoeff^(samples_for_3_seconds) = 0.001
            // samples_for_3_seconds = sampleRate * 3
            // decayCoeff = 0.001^(1/(sampleRate*3))
            double samplesFor3Seconds = sampleRate * 3.0;
            decayCoeff = static_cast<float>(std::pow(0.001, 1.0 / samplesFor3Seconds));
        }
    }
}

//------------------------------------------------------------------------------
void DeviceMeterTap::audioDeviceStopped()
{
    // Reset all levels when device stops
    for (int i = 0; i < MaxChannels; ++i)
    {
        inputLevels[i].store(0.0f, std::memory_order_relaxed);
        outputLevels[i].store(0.0f, std::memory_order_relaxed);
    }
    numInputs.store(0, std::memory_order_relaxed);
    numOutputs.store(0, std::memory_order_relaxed);
}

//------------------------------------------------------------------------------
void DeviceMeterTap::updateLevel(std::atomic<float>& level, const float* data, int numSamples)
{
    float currentLevel = level.load(std::memory_order_relaxed);

    for (int i = 0; i < numSamples; ++i)
    {
        float sample = std::abs(data[i]);
        // Sanity check: ignore unreasonable values (likely garbage/uninitialized data)
        if (sample > 10.0f)
            continue;
        if (sample > currentLevel)
            currentLevel = sample;
        else
            currentLevel *= decayCoeff;
    }

    // Clamp very small values to zero to avoid denormals
    if (currentLevel < 1e-10f)
        currentLevel = 0.0f;

    level.store(currentLevel, std::memory_order_relaxed);
}

//------------------------------------------------------------------------------
float DeviceMeterTap::getInputLevel(int channel) const
{
    if (channel >= 0 && channel < MaxChannels)
        return inputLevels[channel].load(std::memory_order_relaxed);
    return 0.0f;
}

//------------------------------------------------------------------------------
float DeviceMeterTap::getOutputLevel(int channel) const
{
    if (channel >= 0 && channel < MaxChannels)
        return outputLevels[channel].load(std::memory_order_relaxed);
    return 0.0f;
}

//------------------------------------------------------------------------------
int DeviceMeterTap::getNumInputChannels() const
{
    return numInputs.load(std::memory_order_relaxed);
}

//------------------------------------------------------------------------------
int DeviceMeterTap::getNumOutputChannels() const
{
    return numOutputs.load(std::memory_order_relaxed);
}

//------------------------------------------------------------------------------
DeviceMeterTap* DeviceMeterTap::getInstance()
{
    return instance;
}

//------------------------------------------------------------------------------
void DeviceMeterTap::setInstance(DeviceMeterTap* inst)
{
    instance = inst;
}

//------------------------------------------------------------------------------
juce::String DeviceMeterTap::getDeviceName() const
{
    return deviceName;
}
