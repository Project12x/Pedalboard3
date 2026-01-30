//	BypassableInstance.h - Wrapper class to provide a bypass to
//						   AudioPluginInstance.
//	----------------------------------------------------------------------------
//	This file is part of Pedalboard3, an audio plugin host.
//	Copyright (c) 2011 Niall Moody.
//
//	This program is free software: you can redistribute it and/or modify
//	it under the terms of the GNU General Public License as published by
//	the Free Software Foundation, either version 3 of the License, or
//	(at your option) any later version.
//
//	This program is distributed in the hope that it will be useful,
//	but WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//	GNU General Public License for more details.
//
//	You should have received a copy of the GNU General Public License
//	along with this program.  If not, see <http://www.gnu.org/licenses/>.
//	----------------------------------------------------------------------------

#ifndef BYPASSABLEINSTANCE_H_
#define BYPASSABLEINSTANCE_H_

#include <JuceHeader.h>

///	Wrapper class to provide a bypass to AudioPluginInstance.
class BypassableInstance : public AudioPluginInstance
{
  public:
    ///	Constructor.
    BypassableInstance(AudioPluginInstance* plug);
    ///	Destructor.
    ~BypassableInstance();

    ///	Sets up our buffer.
    void prepareToPlay(double sampleRate, int estimatedSamplesPerBlock);
    ///	Clears our buffer.
    void releaseResources();
    ///	Handles the audio.
    void processBlock(AudioSampleBuffer& buffer, MidiBuffer& midiMessages);

    ///	Sets the bypass state.
    void setBypass(bool val);
    ///	Returns the bypass state.
    bool getBypass() const { return bypass; };

    ///	Sets the MIDI channel the plugin responds to.
    void setMIDIChannel(int val);
    ///	Returns the plugin's MIDI channel (-1 == omni).
    int getMIDIChannel() const { return midiChannel; };
    ///	Passes a MIDI message to the plugin from the OSC input.
    void addMidiMessage(const MidiMessage& message);

    ///	Returns the plugin instance we're wrapping.
    AudioPluginInstance* getPlugin() { return plugin; };

    ///	Returns the plugin's name.
    const String getName() const { return plugin->getName(); };

    // Modern JUCE 8 channel info - use bus layout instead of deprecated channel APIs
    ///	Returns the channel name for the given bus and channel index.
    String getChannelName(bool isInput, int busIndex, int channelIndex) const
    {
        auto* bus = isInput ? plugin->getBus(true, busIndex) : plugin->getBus(false, busIndex);
        if (bus != nullptr && channelIndex < bus->getCurrentLayout().size())
            return bus->getCurrentLayout().getChannelTypeName(bus->getCurrentLayout().getTypeOfChannel(channelIndex));
        return {};
    };

    // Deprecated channel APIs - redirected to bus layout for compatibility
    [[deprecated("Use getChannelName(true, 0, channelIndex) instead")]]
    const String getInputChannelName(int channelIndex) const
    {
        return getChannelName(true, 0, channelIndex);
    }
    [[deprecated("Use getChannelName(false, 0, channelIndex) instead")]]
    const String getOutputChannelName(int channelIndex) const
    {
        return getChannelName(false, 0, channelIndex);
    }
    [[deprecated("Use bus layout APIs instead")]]
    bool isInputChannelStereoPair(int index) const
    {
        auto* bus = plugin->getBus(true, 0);
        return bus && bus->getCurrentLayout().size() >= 2;
    }
    [[deprecated("Use bus layout APIs instead")]]
    bool isOutputChannelStereoPair(int index) const
    {
        auto* bus = plugin->getBus(false, 0);
        return bus && bus->getCurrentLayout().size() >= 2;
    }

    // Bus forwarding for wrapped plugin - CRITICAL for VSTi audio pins
    // NOTE: AudioProcessor::getBusCount/getBus/getTotalNumChannels are NOT virtual!
    // Callers MUST use getPlugin() to access the wrapped plugin's actual bus state,
    // or use these explicit helper methods.
    int getWrappedBusCount(bool isInput) const { return plugin->getBusCount(isInput); }
    AudioProcessor::Bus* getWrappedBus(bool isInput, int busIndex) { return plugin->getBus(isInput, busIndex); }
    int getWrappedTotalNumInputChannels() const { return plugin->getTotalNumInputChannels(); }
    int getWrappedTotalNumOutputChannels() const { return plugin->getTotalNumOutputChannels(); }

    ///	Returns the length of the plugin's tail.
    double getTailLengthSeconds() const { return plugin->getTailLengthSeconds(); };
    ///	Returns true if the plugin wants MIDI input.
    bool acceptsMidi() const { return plugin->acceptsMidi(); };
    ///	Returns true if the plugin produces MIDI output.
    bool producesMidi() const { return plugin->producesMidi(); };
    ///	Can be used to reset the plugin.
    void reset() { plugin->reset(); };
    ///	Creates the plugin's editor.
    AudioProcessorEditor* createEditor() { return plugin->createEditor(); };
    ///	Returns true if the plugin has an editor.
    bool hasEditor() const { return plugin->hasEditor(); };

    // Modern JUCE 8 parameter access via AudioProcessorParameter array
    ///	Returns the number of parameters the plugin has.
    int getNumPluginParameters() const { return plugin->getParameters().size(); };
    ///	Returns the indexed parameter object.
    AudioProcessorParameter* getPluginParameter(int index)
    {
        auto& params = plugin->getParameters();
        return (index >= 0 && index < params.size()) ? params[index] : nullptr;
    };
    ///	Returns the indexed parameter's name.
    String getPluginParameterName(int parameterIndex)
    {
        if (auto* param = getPluginParameter(parameterIndex))
            return param->getName(128);
        return {};
    };
    ///	Returns the indexed parameter's value (0-1 normalized).
    float getPluginParameterValue(int parameterIndex)
    {
        if (auto* param = getPluginParameter(parameterIndex))
            return param->getValue();
        return 0.0f;
    };
    ///	Returns the indexed parameter's value as a string.
    String getPluginParameterText(int parameterIndex)
    {
        if (auto* param = getPluginParameter(parameterIndex))
            return param->getCurrentValueAsText();
        return {};
    };
    ///	Sets the indexed parameter value (0-1 normalized).
    void setPluginParameterValue(int parameterIndex, float newValue)
    {
        if (auto* param = getPluginParameter(parameterIndex))
            param->setValue(newValue);
    };
    ///	Returns true if the indexed parameter is automatable.
    bool isPluginParameterAutomatable(int parameterIndex) const
    {
        if (auto* param = plugin->getParameters()[parameterIndex])
            return param->isAutomatable();
        return false;
    };

    // Deprecated parameter APIs - kept for backwards compatibility
    [[deprecated("Use getNumPluginParameters() instead")]]
    int getNumParameters()
    {
        return getNumPluginParameters();
    }
    [[deprecated("Use getPluginParameterName() instead")]]
    const String getParameterName(int parameterIndex)
    {
        return getPluginParameterName(parameterIndex);
    }
    [[deprecated("Use getPluginParameterValue() instead")]]
    float getParameter(int parameterIndex)
    {
        return getPluginParameterValue(parameterIndex);
    }
    [[deprecated("Use getPluginParameterText() instead")]]
    const String getParameterText(int parameterIndex)
    {
        return getPluginParameterText(parameterIndex);
    }
    [[deprecated("Use setPluginParameterValue() instead")]]
    void setParameter(int parameterIndex, float newValue)
    {
        setPluginParameterValue(parameterIndex, newValue);
    }
    [[deprecated("Use isPluginParameterAutomatable() instead")]]
    bool isParameterAutomatable(int parameterIndex) const
    {
        return isPluginParameterAutomatable(parameterIndex);
    }
    [[deprecated("Use plugin->getParameters()[parameterIndex]->isMetaParameter() instead")]]
    bool isMetaParameter(int parameterIndex) const
    {
        if (auto* param = plugin->getParameters()[parameterIndex])
            return param->isMetaParameter();
        return false;
    }

    ///	Returns the number of programs the plugin has.
    int getNumPrograms() override { return plugin->getNumPrograms(); };
    ///	Returns the index of the current program.
    int getCurrentProgram() override { return plugin->getCurrentProgram(); };
    ///	Sets the current program.
    void setCurrentProgram(int index) override { plugin->setCurrentProgram(index); };
    ///	Returns the indexed program's name.
    const String getProgramName(int index) override { return plugin->getProgramName(index); };
    ///	Changes the indexed program's name.
    void changeProgramName(int index, const String& newName) override { plugin->changeProgramName(index, newName); };
    ///	Used to save the plugin's internal state.
    void getStateInformation(juce::MemoryBlock& destData) override { plugin->getStateInformation(destData); };
    ///	Used to save the state of the current program.
    void getCurrentProgramStateInformation(juce::MemoryBlock& destData)
    {
        plugin->getCurrentProgramStateInformation(destData);
    };
    ///	Restores the plugin's internal state.
    void setStateInformation(const void* data, int sizeInBytes) override
    {
        plugin->setStateInformation(data, sizeInBytes);
    };
    ///	Sets the state of the current program.
    void setCurrentProgramStateInformation(const void* data, int sizeInBytes)
    {
        plugin->setCurrentProgramStateInformation(data, sizeInBytes);
    };
    ///	Fills in the plugin's description.
    void fillInPluginDescription(PluginDescription& description) const override
    {
        plugin->fillInPluginDescription(description);
    };

    // Note: getPlatformSpecificData() was deprecated in JUCE 8
    // Use getExtensions() with ExtensionsVisitor pattern instead if needed

  private:
    ///	The plugin instance we're wrapping.
    AudioPluginInstance* plugin;

    ///	Buffer used to store the plugin's audio.
    AudioSampleBuffer tempBuffer;

    ///	Whether we are currently bypassing the plugin or not.
    bool bypass;
    ///	Used to ramp the bypass audio.
    float bypassRamp;

    ///	The MIDI channel the plugin responds to.
    int midiChannel;
    ///	Used to pass OSC MIDI messages to the plugin.
    MidiMessageCollector midiCollector;
};

#endif
