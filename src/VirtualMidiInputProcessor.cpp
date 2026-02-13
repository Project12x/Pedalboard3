/*
  ==============================================================================

    VirtualMidiInputProcessor.cpp
    Pedalboard3 - Virtual MIDI Keyboard Input

  ==============================================================================
*/

#include "VirtualMidiInputProcessor.h"

#include <spdlog/spdlog.h>

// Static instance pointer
VirtualMidiInputProcessor* VirtualMidiInputProcessor::instance = nullptr;

//==============================================================================
VirtualMidiInputProcessor::VirtualMidiInputProcessor()
{
    // Configure as MIDI-only: remove default stereo buses
    AudioProcessor::BusesLayout emptyLayout;
    setBusesLayout(emptyLayout);
}

VirtualMidiInputProcessor::~VirtualMidiInputProcessor()
{
    if (instance == this)
        instance = nullptr;
}

//==============================================================================
void VirtualMidiInputProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    spdlog::info("[VirtualMidiInput] prepareToPlay sr={} blockSize={}", sampleRate, samplesPerBlock);
    currentSampleRate = sampleRate;
    midiCollector.reset(sampleRate);

    // Register as the active instance when actually in the graph
    // (not in constructor, to avoid temp instances during plugin enumeration)
    setInstance(this);
}

void VirtualMidiInputProcessor::processBlock(AudioBuffer<float>& buffer, MidiBuffer& midiMessages)
{
    // Audio passes through unchanged
    ignoreUnused(buffer);

    // Retrieve any MIDI messages that were added from the UI thread
    midiCollector.removeNextBlockOfMessages(midiMessages, buffer.getNumSamples());

    // DEBUG: Periodic confirmation that processBlock is being called
    ++processBlockCallCount;
    if (processBlockCallCount == 1 || processBlockCallCount % 5000 == 0)
    {
        spdlog::info("[VirtualMidiInput] processBlock alive (call #{}, bufSamples={}, instance={})",
                     processBlockCallCount, buffer.getNumSamples(),
                     (instance == this) ? "CURRENT" : "STALE");
    }

    // DEBUG: Log when MIDI messages are produced
    if (!midiMessages.isEmpty())
    {
        int count = 0;
        for (const auto metadata : midiMessages)
        {
            ignoreUnused(metadata);
            ++count;
        }
        spdlog::info("[VirtualMidiInput] processBlock output {} MIDI messages, bufSamples={}",
                     count, buffer.getNumSamples());
    }
}

//==============================================================================
void VirtualMidiInputProcessor::addMidiMessage(const MidiMessage& msg)
{
    // DEBUG: Log incoming MIDI from virtual keyboard
    if (msg.isNoteOn())
        spdlog::info("[VirtualMidiInput] addMidiMessage: noteOn ch={} note={} vel={}",
                     msg.getChannel(), msg.getNoteNumber(), msg.getVelocity());

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
