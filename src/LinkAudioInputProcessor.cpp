#include "LinkAudioInputProcessor.h"
#include "LinkAudioService.h"

LinkAudioInputProcessor::LinkAudioInputProcessor()
{
    setPlayConfigDetails(0, 16, 44100.0, 512);
}

juce::Component* LinkAudioInputProcessor::getControls()
{
    return new juce::Component();
}

void LinkAudioInputProcessor::fillInPluginDescription(juce::PluginDescription& description) const
{
    description.name = getName();
    description.descriptiveName = "Receives the selected Ableton Link Audio channel";
    description.pluginFormatName = "Internal";
    description.category = "Built-in";
    description.manufacturerName = "Pedalboard3";
    description.fileOrIdentifier = "linkaudioinput";
    description.uniqueId = 0x4c494e4b;
    description.numInputChannels = 0;
    description.numOutputChannels = 16;
}

void LinkAudioInputProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    LinkAudioService::readIncomingAudio(buffer);
}
