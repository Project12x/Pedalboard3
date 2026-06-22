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
#include <algorithm>

// Static instance pointer
DeviceMeterTap* DeviceMeterTap::instance = nullptr;

//------------------------------------------------------------------------------
DeviceMeterTap::DeviceMeterTap()
{
    inputMeters.prepare(44100.0, 0);
    outputMeters.prepare(44100.0, 0);
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
    const int sampleCount = std::max(0, numSamples);

    // Update input levels from device inputs.
    const int inputCount = juce::jlimit(0, MaxChannels, numInputChannels);
    inputMeters.process(inputChannelData, inputCount, sampleCount);

    // Update output levels from processed output buffer. The output buffer
    // contains audio from the graphPlayer callback that ran before us.
    const int outputCount = juce::jlimit(0, MaxChannels, numOutputChannels);
    const float* outputReadPtrs[MaxChannels] = {};
    for (int ch = 0; ch < outputCount; ++ch)
        outputReadPtrs[ch] = outputChannelData != nullptr ? outputChannelData[ch] : nullptr;
    outputMeters.process(outputReadPtrs, outputCount, sampleCount);

    numInputs.store(inputCount, std::memory_order_relaxed);
    numOutputs.store(outputCount, std::memory_order_relaxed);

    // CRITICAL: Zero our output contribution!
    // When JUCE has multiple callbacks, it mixes their outputs together.
    // If we don't zero our output buffers, garbage gets mixed into the audio.
    const int channelsToClear = std::max(0, numOutputChannels);
    for (int ch = 0; ch < channelsToClear; ++ch)
    {
        if (outputChannelData != nullptr && outputChannelData[ch] != nullptr)
            juce::FloatVectorOperations::clear(outputChannelData[ch], sampleCount);
    }
}

//------------------------------------------------------------------------------
void DeviceMeterTap::audioDeviceAboutToStart(juce::AudioIODevice* device)
{
    if (device != nullptr)
    {
        // Store device name for display
        deviceName = device->getName();

        double sampleRate = device->getCurrentSampleRate();
        const int inputCount = juce::jlimit(0, MaxChannels, device->getActiveInputChannels().countNumberOfSetBits());
        const int outputCount = juce::jlimit(0, MaxChannels, device->getActiveOutputChannels().countNumberOfSetBits());

        inputMeters.prepare(sampleRate, inputCount);
        outputMeters.prepare(sampleRate, outputCount);
        numInputs.store(inputCount, std::memory_order_relaxed);
        numOutputs.store(outputCount, std::memory_order_relaxed);
    }
}

//------------------------------------------------------------------------------
void DeviceMeterTap::audioDeviceStopped()
{
    inputMeters.prepare(44100.0, 0);
    outputMeters.prepare(44100.0, 0);
    numInputs.store(0, std::memory_order_relaxed);
    numOutputs.store(0, std::memory_order_relaxed);
}

//------------------------------------------------------------------------------
float DeviceMeterTap::getInputLevel(int channel) const
{
    return inputMeters.getPeak(channel);
}

//------------------------------------------------------------------------------
float DeviceMeterTap::getOutputLevel(int channel) const
{
    return outputMeters.getPeak(channel);
}

//------------------------------------------------------------------------------
float DeviceMeterTap::getInputRmsLevel(int channel) const
{
    return inputMeters.getRms(channel);
}

//------------------------------------------------------------------------------
float DeviceMeterTap::getOutputRmsLevel(int channel) const
{
    return outputMeters.getRms(channel);
}

//------------------------------------------------------------------------------
float DeviceMeterTap::getInputVuLevel(int channel) const
{
    return inputMeters.getVu(channel);
}

//------------------------------------------------------------------------------
float DeviceMeterTap::getOutputVuLevel(int channel) const
{
    return outputMeters.getVu(channel);
}

//------------------------------------------------------------------------------
bool DeviceMeterTap::getInputClip(int channel) const
{
    return inputMeters.getClip(channel);
}

//------------------------------------------------------------------------------
bool DeviceMeterTap::getOutputClip(int channel) const
{
    return outputMeters.getClip(channel);
}

//------------------------------------------------------------------------------
bool DeviceMeterTap::getInputAndClearClip(int channel)
{
    return inputMeters.getAndClearClip(channel);
}

//------------------------------------------------------------------------------
bool DeviceMeterTap::getOutputAndClearClip(int channel)
{
    return outputMeters.getAndClearClip(channel);
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

//------------------------------------------------------------------------------
#if PEDALBOARD3_TESTS
void DeviceMeterTap::prepareForTest(double sampleRate)
{
    inputMeters.prepare(sampleRate, 0);
    outputMeters.prepare(sampleRate, 0);
    numInputs.store(0, std::memory_order_relaxed);
    numOutputs.store(0, std::memory_order_relaxed);
}
#endif
