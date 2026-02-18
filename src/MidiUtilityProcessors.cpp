/*
  ==============================================================================

    MidiUtilityProcessors.cpp
    Pedalboard3 - MIDI Utility Processors

  ==============================================================================
*/

#include "MidiUtilityProcessors.h"

//==============================================================================
// MidiTransposeProcessor
//==============================================================================

MidiTransposeProcessor::MidiTransposeProcessor() {}

MidiTransposeProcessor::~MidiTransposeProcessor() = default;

void MidiTransposeProcessor::processBlock(AudioBuffer<float>& buffer, MidiBuffer& midiMessages)
{
    // Audio passes through unchanged
    ignoreUnused(buffer);

    int semitones = transpose.load();
    if (semitones == 0)
        return; // No transposition needed

    MidiBuffer processedMidi;

    for (const auto metadata : midiMessages)
    {
        auto message = metadata.getMessage();
        int samplePosition = metadata.samplePosition;

        if (message.isNoteOn() || message.isNoteOff())
        {
            int newNote = message.getNoteNumber() + semitones;

            // Clamp to valid MIDI range
            if (newNote >= 0 && newNote <= 127)
            {
                if (message.isNoteOn())
                    message = MidiMessage::noteOn(message.getChannel(), newNote, message.getVelocity());
                else
                    message = MidiMessage::noteOff(message.getChannel(), newNote, message.getVelocity());

                processedMidi.addEvent(message, samplePosition);
            }
            // Notes outside range are dropped
        }
        else
        {
            // Pass through all non-note messages unchanged
            processedMidi.addEvent(message, samplePosition);
        }
    }

    midiMessages.swapWith(processedMidi);
}

void MidiTransposeProcessor::setTranspose(int semitones)
{
    transpose.store(jlimit(-48, 48, semitones));
}

float MidiTransposeProcessor::getParameter(int parameterIndex)
{
    if (parameterIndex == TransposeParam)
        return (transpose.load() + 48) / 96.0f; // Normalize to 0-1
    return 0.0f;
}

void MidiTransposeProcessor::setParameter(int parameterIndex, float newValue)
{
    if (parameterIndex == TransposeParam)
        setTranspose(static_cast<int>(newValue * 96.0f) - 48);
}

const String MidiTransposeProcessor::getParameterName(int parameterIndex)
{
    if (parameterIndex == TransposeParam)
        return "Transpose";
    return {};
}

const String MidiTransposeProcessor::getParameterText(int parameterIndex)
{
    if (parameterIndex == TransposeParam)
    {
        int val = transpose.load();
        if (val > 0)
            return "+" + String(val);
        return String(val);
    }
    return {};
}

void MidiTransposeProcessor::getStateInformation(MemoryBlock& destData)
{
    auto xml = std::make_unique<XmlElement>("MidiTranspose");
    xml->setAttribute("transpose", transpose.load());
    copyXmlToBinary(*xml, destData);
}

void MidiTransposeProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary(data, sizeInBytes))
    {
        if (xml->hasTagName("MidiTranspose"))
            setTranspose(xml->getIntAttribute("transpose", 0));
    }
}

void MidiTransposeProcessor::fillInPluginDescription(PluginDescription& description) const
{
    description.name = getName();
    description.descriptiveName = "MIDI Note Transpose";
    description.pluginFormatName = "Internal";
    description.category = "MIDI Utility";
    description.manufacturerName = "Pedalboard3";
    description.version = "1.0";
    description.fileOrIdentifier = "MidiTranspose";
    description.isInstrument = false;
    description.numInputChannels = 2;
    description.numOutputChannels = 2;
}

// Note: getControls() and createEditor() are defined in MidiUtilityControls.cpp

//==============================================================================
// MidiRechannelizeProcessor
//==============================================================================

MidiRechannelizeProcessor::MidiRechannelizeProcessor() {}

MidiRechannelizeProcessor::~MidiRechannelizeProcessor() = default;

void MidiRechannelizeProcessor::processBlock(AudioBuffer<float>& buffer, MidiBuffer& midiMessages)
{
    // Audio passes through unchanged
    ignoreUnused(buffer);

    int inChan = inputChannel.load();
    int outChan = outputChannel.load();

    MidiBuffer processedMidi;

    for (const auto metadata : midiMessages)
    {
        auto message = metadata.getMessage();
        int samplePosition = metadata.samplePosition;

        // Check if this is a channel voice message
        if (message.getChannel() > 0)
        {
            // Filter by input channel if set
            if (inChan == 0 || message.getChannel() == inChan)
            {
                // Change the channel
                message.setChannel(outChan);
                processedMidi.addEvent(message, samplePosition);
            }
            // Messages not matching input filter are dropped
        }
        else
        {
            // Pass through system messages unchanged
            processedMidi.addEvent(message, samplePosition);
        }
    }

    midiMessages.swapWith(processedMidi);
}

void MidiRechannelizeProcessor::setInputChannel(int channel)
{
    inputChannel.store(jlimit(0, 16, channel));
}

void MidiRechannelizeProcessor::setOutputChannel(int channel)
{
    outputChannel.store(jlimit(1, 16, channel));
}

float MidiRechannelizeProcessor::getParameter(int parameterIndex)
{
    if (parameterIndex == InputChannelParam)
        return inputChannel.load() / 16.0f;
    if (parameterIndex == OutputChannelParam)
        return (outputChannel.load() - 1) / 15.0f;
    return 0.0f;
}

void MidiRechannelizeProcessor::setParameter(int parameterIndex, float newValue)
{
    if (parameterIndex == InputChannelParam)
        setInputChannel(static_cast<int>(newValue * 16.0f));
    else if (parameterIndex == OutputChannelParam)
        setOutputChannel(static_cast<int>(newValue * 15.0f) + 1);
}

const String MidiRechannelizeProcessor::getParameterName(int parameterIndex)
{
    if (parameterIndex == InputChannelParam)
        return "Input Channel";
    if (parameterIndex == OutputChannelParam)
        return "Output Channel";
    return {};
}

const String MidiRechannelizeProcessor::getParameterText(int parameterIndex)
{
    if (parameterIndex == InputChannelParam)
    {
        int ch = inputChannel.load();
        return ch == 0 ? "All" : String(ch);
    }
    if (parameterIndex == OutputChannelParam)
        return String(outputChannel.load());
    return {};
}

void MidiRechannelizeProcessor::getStateInformation(MemoryBlock& destData)
{
    auto xml = std::make_unique<XmlElement>("MidiRechannelize");
    xml->setAttribute("inputChannel", inputChannel.load());
    xml->setAttribute("outputChannel", outputChannel.load());
    copyXmlToBinary(*xml, destData);
}

void MidiRechannelizeProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary(data, sizeInBytes))
    {
        if (xml->hasTagName("MidiRechannelize"))
        {
            setInputChannel(xml->getIntAttribute("inputChannel", 0));
            setOutputChannel(xml->getIntAttribute("outputChannel", 1));
        }
    }
}

void MidiRechannelizeProcessor::fillInPluginDescription(PluginDescription& description) const
{
    description.name = getName();
    description.descriptiveName = "MIDI Channel Remapper";
    description.pluginFormatName = "Internal";
    description.category = "MIDI Utility";
    description.manufacturerName = "Pedalboard3";
    description.version = "1.0";
    description.fileOrIdentifier = "MidiRechannelize";
    description.isInstrument = false;
    description.numInputChannels = 2;
    description.numOutputChannels = 2;
}

// Note: getControls() and createEditor() are defined in MidiUtilityControls.cpp

//==============================================================================
// KeyboardSplitProcessor
//==============================================================================

KeyboardSplitProcessor::KeyboardSplitProcessor() {}

KeyboardSplitProcessor::~KeyboardSplitProcessor() = default;

void KeyboardSplitProcessor::processBlock(AudioBuffer<float>& buffer, MidiBuffer& midiMessages)
{
    // Audio passes through unchanged
    ignoreUnused(buffer);

    int split = splitPoint.load();
    int lowerChan = lowerChannel.load();
    int upperChan = upperChannel.load();

    MidiBuffer processedMidi;

    for (const auto metadata : midiMessages)
    {
        auto message = metadata.getMessage();
        int samplePosition = metadata.samplePosition;

        if (message.isNoteOn() || message.isNoteOff())
        {
            int noteNumber = message.getNoteNumber();

            // Route based on split point
            int targetChannel = (noteNumber < split) ? lowerChan : upperChan;
            message.setChannel(targetChannel);
            processedMidi.addEvent(message, samplePosition);
        }
        else
        {
            // Pass through all other messages unchanged
            processedMidi.addEvent(message, samplePosition);
        }
    }

    midiMessages.swapWith(processedMidi);
}

void KeyboardSplitProcessor::setSplitPoint(int midiNote)
{
    splitPoint.store(jlimit(0, 127, midiNote));
}

void KeyboardSplitProcessor::setLowerChannel(int channel)
{
    lowerChannel.store(jlimit(1, 16, channel));
}

void KeyboardSplitProcessor::setUpperChannel(int channel)
{
    upperChannel.store(jlimit(1, 16, channel));
}

float KeyboardSplitProcessor::getParameter(int parameterIndex)
{
    if (parameterIndex == SplitPointParam)
        return splitPoint.load() / 127.0f;
    if (parameterIndex == LowerChannelParam)
        return (lowerChannel.load() - 1) / 15.0f;
    if (parameterIndex == UpperChannelParam)
        return (upperChannel.load() - 1) / 15.0f;
    return 0.0f;
}

void KeyboardSplitProcessor::setParameter(int parameterIndex, float newValue)
{
    if (parameterIndex == SplitPointParam)
        setSplitPoint(static_cast<int>(newValue * 127.0f));
    else if (parameterIndex == LowerChannelParam)
        setLowerChannel(static_cast<int>(newValue * 15.0f) + 1);
    else if (parameterIndex == UpperChannelParam)
        setUpperChannel(static_cast<int>(newValue * 15.0f) + 1);
}

const String KeyboardSplitProcessor::getParameterName(int parameterIndex)
{
    if (parameterIndex == SplitPointParam)
        return "Split Point";
    if (parameterIndex == LowerChannelParam)
        return "Lower Channel";
    if (parameterIndex == UpperChannelParam)
        return "Upper Channel";
    return {};
}

const String KeyboardSplitProcessor::getParameterText(int parameterIndex)
{
    if (parameterIndex == SplitPointParam)
        return getNoteNameFromMidi(splitPoint.load());
    if (parameterIndex == LowerChannelParam)
        return String(lowerChannel.load());
    if (parameterIndex == UpperChannelParam)
        return String(upperChannel.load());
    return {};
}

String KeyboardSplitProcessor::getNoteNameFromMidi(int midiNote)
{
    static const char* noteNames[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
    int octave = (midiNote / 12) - 1;
    int note = midiNote % 12;
    return String(noteNames[note]) + String(octave);
}

void KeyboardSplitProcessor::getStateInformation(MemoryBlock& destData)
{
    auto xml = std::make_unique<XmlElement>("KeyboardSplit");
    xml->setAttribute("splitPoint", splitPoint.load());
    xml->setAttribute("lowerChannel", lowerChannel.load());
    xml->setAttribute("upperChannel", upperChannel.load());
    copyXmlToBinary(*xml, destData);
}

void KeyboardSplitProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary(data, sizeInBytes))
    {
        if (xml->hasTagName("KeyboardSplit"))
        {
            setSplitPoint(xml->getIntAttribute("splitPoint", 60));
            setLowerChannel(xml->getIntAttribute("lowerChannel", 1));
            setUpperChannel(xml->getIntAttribute("upperChannel", 2));
        }
    }
}

void KeyboardSplitProcessor::fillInPluginDescription(PluginDescription& description) const
{
    description.name = getName();
    description.descriptiveName = "Keyboard Split";
    description.pluginFormatName = "Internal";
    description.category = "MIDI Utility";
    description.manufacturerName = "Pedalboard3";
    description.version = "1.0";
    description.fileOrIdentifier = "KeyboardSplit";
    description.isInstrument = false;
    description.numInputChannels = 2;
    description.numOutputChannels = 2;
}

// Note: getControls() and createEditor() are defined in MidiUtilityControls.cpp
