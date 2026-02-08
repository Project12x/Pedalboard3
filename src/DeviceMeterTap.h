//  DeviceMeterTap.h - Device-level audio metering for I/O nodes
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

#pragma once

#include <JuceHeader.h>
#include <atomic>

/// Taps into device-level audio to provide per-channel input/output levels.
/// Used for built-in VU meters on Audio I/O nodes and Soundcheck dialog.
class DeviceMeterTap : public juce::AudioIODeviceCallback
{
public:
    static constexpr int MaxChannels = 16;

    DeviceMeterTap();
    ~DeviceMeterTap() override;

    // AudioIODeviceCallback interface
    void audioDeviceIOCallbackWithContext(
        const float* const* inputChannelData,
        int numInputChannels,
        float* const* outputChannelData,
        int numOutputChannels,
        int numSamples,
        const juce::AudioIODeviceCallbackContext& context) override;

    void audioDeviceAboutToStart(juce::AudioIODevice* device) override;
    void audioDeviceStopped() override;

    /// Returns the current input level for a channel (linear 0.0-1.0+)
    float getInputLevel(int channel) const;

    /// Returns the current output level for a channel (linear 0.0-1.0+)
    float getOutputLevel(int channel) const;

    /// Returns the number of active input channels
    int getNumInputChannels() const;

    /// Returns the number of active output channels
    int getNumOutputChannels() const;

    /// Returns the current audio device name
    juce::String getDeviceName() const;

    /// Static instance accessor (set by MainPanel)
    static DeviceMeterTap* getInstance();
    static void setInstance(DeviceMeterTap* instance);

private:
    /// Updates level with peak detection and exponential decay
    void updateLevel(std::atomic<float>& level, const float* data, int numSamples);

    // Per-channel levels (atomic for thread-safety)
    std::atomic<float> inputLevels[MaxChannels];
    std::atomic<float> outputLevels[MaxChannels];

    // Channel counts
    std::atomic<int> numInputs{0};
    std::atomic<int> numOutputs{0};

    // Decay coefficient (per-sample multiplier for exponential decay)
    float decayCoeff{0.99995f};

    // Static instance pointer
    static DeviceMeterTap* instance;

    // Current device name
    juce::String deviceName;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DeviceMeterTap)
};
