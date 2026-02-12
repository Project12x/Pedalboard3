/*
  ==============================================================================

    VirtualMidiInputProcessor.cpp
    Pedalboard3 - Virtual MIDI Keyboard Input

  ==============================================================================
*/

#include "VirtualMidiInputProcessor.h"

// Static instance pointer
VirtualMidiInputProcessor* VirtualMidiInputProcessor::instance = nullptr;

//==============================================================================
VirtualMidiInputProcessor::VirtualMidiInputProcessor()
{
    // Register this as the active instance
    setInstance(this);

    // Configure as MIDI-only processor (no audio buses)
    setPlayConfigDetails(0, 0, 44100.0, 512);
}

VirtualMidiInputProcessor::~VirtualMidiInputProcessor()
{
    if (instance == this)
        instance = nullptr;
}

//==============================================================================
void VirtualMidiInputProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    midiCollector.reset(sampleRate);
}

void VirtualMidiInputProcessor::processBlock(AudioBuffer<float>& buffer, MidiBuffer& midiMessages)
{
    // Audio passes through unchanged
    ignoreUnused(buffer);

    // Retrieve any MIDI messages that were added from the UI thread
    midiCollector.removeNextBlockOfMessages(midiMessages, buffer.getNumSamples());
}

//==============================================================================
void VirtualMidiInputProcessor::addMidiMessage(const MidiMessage& msg)
{
    // Called from UI thread - MidiMessageCollector handles thread safety
    midiCollector.addMessageToQueue(msg);
}

//==============================================================================
Component* VirtualMidiInputProcessor::getControls()
{
    // Simple label indicating this receives from virtual keyboard
    auto* label = new Label("info", "Virtual Keyboard");
    label->setJustificationType(Justification::centred);
    label->setColour(Label::textColourId, Colours::white);
    return label;
}

AudioProcessorEditor* VirtualMidiInputProcessor::createEditor()
{
    return nullptr;
}

//==============================================================================
void VirtualMidiInputProcessor::getStateInformation(MemoryBlock& destData)
{
    // No state to save
    auto xml = std::make_unique<XmlElement>("VirtualMidiInput");
    copyXmlToBinary(*xml, destData);
}

void VirtualMidiInputProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    // No state to restore
    ignoreUnused(data, sizeInBytes);
}

void VirtualMidiInputProcessor::fillInPluginDescription(PluginDescription& description) const
{
    description.name = getName();
    description.descriptiveName = "Virtual MIDI Keyboard Input";
    description.pluginFormatName = "Internal";
    description.category = "MIDI Utility";
    description.manufacturerName = "Pedalboard3";
    description.version = "1.0";
    description.fileOrIdentifier = "VirtualMidiInput";
    description.isInstrument = false;
    description.numInputChannels = 0;  // MIDI-only, no audio inputs
    description.numOutputChannels = 0; // MIDI-only, no audio outputs
}

//==============================================================================
VirtualMidiInputProcessor* VirtualMidiInputProcessor::getInstance()
{
    return instance;
}

void VirtualMidiInputProcessor::setInstance(VirtualMidiInputProcessor* inst)
{
    instance = inst;
}
