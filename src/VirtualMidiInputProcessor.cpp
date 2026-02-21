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
                     processBlockCallCount, buffer.getNumSamples(), (instance == this) ? "CURRENT" : "STALE");
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
        spdlog::info("[VirtualMidiInput] processBlock output {} MIDI messages, bufSamples={}", count,
                     buffer.getNumSamples());
    }
}

//==============================================================================
void VirtualMidiInputProcessor::addMidiMessage(const MidiMessage& msg)
{
    // Apply fixed velocity to note-on messages
    MidiMessage adjusted = msg;
    if (msg.isNoteOn())
    {
        const int vel = fixedVelocity.load();
        adjusted = MidiMessage::noteOn(msg.getChannel(), msg.getNoteNumber(), static_cast<uint8>(vel));
        adjusted.setTimeStamp(msg.getTimeStamp());

        spdlog::info("[VirtualMidiInput] addMidiMessage: noteOn ch={} note={} vel={}", msg.getChannel(),
                     msg.getNoteNumber(), vel);
    }

    // Called from UI thread - MidiMessageCollector handles thread safety
    midiCollector.addMessageToQueue(adjusted);
}

//==============================================================================
Component* VirtualMidiInputProcessor::getControls()
{
    auto* label = new Label("info", "Virtual Keyboard");
    label->setJustificationType(Justification::centredRight);
    label->setFont(Font(FontOptions().withHeight(11.0f)));
    label->setColour(Label::textColourId, Colours::white.withAlpha(0.7f));
    return label;
}

AudioProcessorEditor* VirtualMidiInputProcessor::createEditor()
{
    return nullptr;
}

//==============================================================================
void VirtualMidiInputProcessor::getStateInformation(MemoryBlock& destData)
{
    auto xml = std::make_unique<XmlElement>("VirtualMidiInput");
    xml->setAttribute("version", 1);
    xml->setAttribute("octaveShift", octaveShift.load());
    xml->setAttribute("velocity", fixedVelocity.load());
    xml->setAttribute("sustain", sustainHeld.load());
    copyXmlToBinary(*xml, destData);
}

void VirtualMidiInputProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    auto xml = getXmlFromBinary(data, sizeInBytes);
    if (xml && xml->hasTagName("VirtualMidiInput"))
    {
        int version = xml->getIntAttribute("version", 0);
        if (version >= 1)
        {
            octaveShift.store(jlimit(-3, 3, xml->getIntAttribute("octaveShift", 0)));
            fixedVelocity.store(jlimit(1, 127, xml->getIntAttribute("velocity", 100)));
            sustainHeld.store(xml->getBoolAttribute("sustain", false));
        }
    }
}

//==============================================================================
void VirtualMidiInputProcessor::setSustainHeld(bool held)
{
    sustainHeld.store(held);

    // Send CC64 (sustain pedal) message
    auto cc = MidiMessage::controllerEvent(1, 64, held ? 127 : 0);
    midiCollector.addMessageToQueue(cc);
}

//==============================================================================
float VirtualMidiInputProcessor::getParameter(int parameterIndex)
{
    switch (parameterIndex)
    {
    case OctaveShiftParam:
        return static_cast<float>(octaveShift.load());
    case VelocityParam:
        return static_cast<float>(fixedVelocity.load()) / 127.0f;
    case SustainParam:
        return sustainHeld.load() ? 1.0f : 0.0f;
    default:
        return 0.0f;
    }
}

void VirtualMidiInputProcessor::setParameter(int parameterIndex, float newValue)
{
    switch (parameterIndex)
    {
    case OctaveShiftParam:
        setOctaveShift(static_cast<int>(newValue));
        break;
    case VelocityParam:
        setFixedVelocity(static_cast<int>(newValue * 127.0f));
        break;
    case SustainParam:
        setSustainHeld(newValue > 0.5f);
        break;
    }
}

const String VirtualMidiInputProcessor::getParameterName(int parameterIndex)
{
    switch (parameterIndex)
    {
    case OctaveShiftParam:
        return "Octave";
    case VelocityParam:
        return "Velocity";
    case SustainParam:
        return "Sustain";
    default:
        return {};
    }
}

const String VirtualMidiInputProcessor::getParameterText(int parameterIndex)
{
    switch (parameterIndex)
    {
    case OctaveShiftParam:
    {
        int shift = octaveShift.load();
        if (shift > 0)
            return "+" + String(shift);
        return String(shift);
    }
    case VelocityParam:
        return String(fixedVelocity.load());
    case SustainParam:
        return sustainHeld.load() ? "On" : "Off";
    default:
        return {};
    }
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
