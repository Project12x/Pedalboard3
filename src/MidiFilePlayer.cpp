//  MidiFilePlayer.cpp - MIDI file playback processor implementation.
//  ----------------------------------------------------------------------------
//  This file is part of Pedalboard3, an audio plugin host.
//  Copyright (c) 2024-2026 Antigravity.
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

#include "MidiFilePlayer.h"

#include "MidiFilePlayerControl.h"

#include <spdlog/spdlog.h>


//------------------------------------------------------------------------------
MidiFilePlayerProcessor::MidiFilePlayerProcessor()
{
    spdlog::info("[MidiFilePlayer] Created");
}

//------------------------------------------------------------------------------
MidiFilePlayerProcessor::~MidiFilePlayerProcessor()
{
    spdlog::info("[MidiFilePlayer] Destroyed");
}

//------------------------------------------------------------------------------
bool MidiFilePlayerProcessor::setFile(const File& file)
{
    if (!file.existsAsFile())
        return false;

    FileInputStream stream(file);
    if (!stream.openedOk())
        return false;

    MidiFile midi;
    if (!midi.readFrom(stream))
    {
        spdlog::error("[MidiFilePlayer] Failed to parse MIDI file: {}", file.getFullPathName().toStdString());
        return false;
    }

    // Convert to seconds-based timestamps
    midi.convertTimestampTicksToSeconds();

    // Clear existing data
    trackSequences.clear();
    trackMuteStates.clear();
    numTracks = midi.getNumTracks();

    // Extract all tracks
    double maxTime = 0.0;
    for (int i = 0; i < numTracks; ++i)
    {
        auto* seq = new MidiMessageSequence(*midi.getTrack(i));
        trackSequences.add(seq);
        trackMuteStates.push_back(false);

        if (seq->getNumEvents() > 0)
        {
            double trackEnd = seq->getEndTime();
            if (trackEnd > maxTime)
                maxTime = trackEnd;
        }
    }

    lengthInSeconds = maxTime;
    midiFile = file;

    // Try to extract tempo from MIDI file
    originalBPM = 120.0;
    for (int i = 0; i < numTracks; ++i)
    {
        auto* seq = midi.getTrack(i);
        for (int j = 0; j < seq->getNumEvents(); ++j)
        {
            auto& event = *seq->getEventPointer(j);
            if (event.message.isTempoMetaEvent())
            {
                originalBPM = 60.0 / event.message.getTempoSecondsPerQuarterNote();
                break;
            }
        }
    }
    bpm.store(originalBPM);

    // Rebuild combined sequence
    rebuildCombinedSequence();
    resetPlayhead();

    spdlog::info("[MidiFilePlayer] Loaded: {} ({} tracks, {:.2f}s, {:.1f} BPM)", file.getFileName().toStdString(),
                 numTracks, lengthInSeconds, originalBPM);

    sendChangeMessage();
    return true;
}

//------------------------------------------------------------------------------
void MidiFilePlayerProcessor::rebuildCombinedSequence()
{
    combinedSequence.clear();

    for (int i = 0; i < trackSequences.size(); ++i)
    {
        if (!trackMuteStates[i])
        {
            combinedSequence.addSequence(*trackSequences[i], 0.0);
        }
    }

    combinedSequence.sort();
    combinedSequence.updateMatchedPairs();
}

//------------------------------------------------------------------------------
void MidiFilePlayerProcessor::resetPlayhead()
{
    playheadSeconds.store(0.0);
    nextEventIndex = 0;
}

//------------------------------------------------------------------------------
double MidiFilePlayerProcessor::getPlaybackPosition() const
{
    if (lengthInSeconds <= 0.0)
        return 0.0;
    return playheadSeconds.load() / lengthInSeconds;
}

//------------------------------------------------------------------------------
void MidiFilePlayerProcessor::setBPM(double newBPM)
{
    bpm.store(juce::jlimit(20.0, 300.0, newBPM));
}

//------------------------------------------------------------------------------
void MidiFilePlayerProcessor::play()
{
    playing.store(true);
    sendChangeMessage();
}

//------------------------------------------------------------------------------
void MidiFilePlayerProcessor::pause()
{
    playing.store(false);
    sendChangeMessage();
}

//------------------------------------------------------------------------------
void MidiFilePlayerProcessor::stop()
{
    playing.store(false);
    resetPlayhead();
    sendChangeMessage();
}

//------------------------------------------------------------------------------
void MidiFilePlayerProcessor::seekToPosition(double position)
{
    double clampedPos = juce::jlimit(0.0, 1.0, position);
    double targetTime = clampedPos * lengthInSeconds;
    playheadSeconds.store(targetTime);

    // Find the next event index for this position
    nextEventIndex = 0;
    for (int i = 0; i < combinedSequence.getNumEvents(); ++i)
    {
        if (combinedSequence.getEventPointer(i)->message.getTimeStamp() >= targetTime)
        {
            nextEventIndex = i;
            break;
        }
        nextEventIndex = i + 1;
    }

    sendChangeMessage();
}

//------------------------------------------------------------------------------
bool MidiFilePlayerProcessor::isTrackMuted(int trackIndex) const
{
    if (trackIndex >= 0 && trackIndex < (int)trackMuteStates.size())
        return trackMuteStates[trackIndex];
    return false;
}

//------------------------------------------------------------------------------
void MidiFilePlayerProcessor::setTrackMuted(int trackIndex, bool muted)
{
    if (trackIndex >= 0 && trackIndex < (int)trackMuteStates.size())
    {
        trackMuteStates[trackIndex] = muted;
        rebuildCombinedSequence();
    }
}

//------------------------------------------------------------------------------
Component* MidiFilePlayerProcessor::getControls()
{
    return new MidiFilePlayerControl(this);
}

//------------------------------------------------------------------------------
void MidiFilePlayerProcessor::fillInPluginDescription(PluginDescription& description) const
{
    description.name = "MIDI File Player";
    description.descriptiveName = "Plays MIDI files to drive synth plugins";
    description.pluginFormatName = "Internal";
    description.category = "MIDI";
    description.manufacturerName = "Antigravity";
    description.version = "1.0";
    description.fileOrIdentifier = "MidiFilePlayer";
    description.isInstrument = false;
    description.numInputChannels = 0;
    description.numOutputChannels = 0;
    description.uniqueId = 0x4D464950; // "MFIP"
}

//------------------------------------------------------------------------------
void MidiFilePlayerProcessor::prepareToPlay(double sampleRate, int estimatedSamplesPerBlock)
{
    currentSampleRate = sampleRate;
}

//------------------------------------------------------------------------------
void MidiFilePlayerProcessor::processBlock(AudioSampleBuffer& buffer, MidiBuffer& midiMessages)
{
    if (!playing.load() || combinedSequence.getNumEvents() == 0)
        return;

    int numSamples = buffer.getNumSamples();
    double blockDurationSeconds = numSamples / currentSampleRate;

    // Calculate tempo scaling
    double tempoScale = bpm.load() / originalBPM;
    double scaledBlockDuration = blockDurationSeconds * tempoScale;

    double currentTime = playheadSeconds.load();
    double blockEndTime = currentTime + scaledBlockDuration;

    // Add all events that fall within this block
    while (nextEventIndex < combinedSequence.getNumEvents())
    {
        auto* eventHolder = combinedSequence.getEventPointer(nextEventIndex);
        double eventTime = eventHolder->message.getTimeStamp();

        if (eventTime >= blockEndTime)
            break;

        if (eventTime >= currentTime)
        {
            // Calculate sample offset within this block
            double offsetSeconds = (eventTime - currentTime) / tempoScale;
            int sampleOffset = static_cast<int>(offsetSeconds * currentSampleRate);
            sampleOffset = juce::jlimit(0, numSamples - 1, sampleOffset);

            // Don't add meta events to the output
            if (!eventHolder->message.isMetaEvent())
            {
                midiMessages.addEvent(eventHolder->message, sampleOffset);
            }
        }

        ++nextEventIndex;
    }

    // Advance playhead
    playheadSeconds.store(blockEndTime);

    // Handle loop or stop at end
    if (blockEndTime >= lengthInSeconds)
    {
        if (looping.load())
        {
            resetPlayhead();
        }
        else
        {
            playing.store(false);
            resetPlayhead();
            // Notify UI of state change
            MessageManager::callAsync([this]() { sendChangeMessage(); });
        }
    }
}

//------------------------------------------------------------------------------
const String MidiFilePlayerProcessor::getParameterName(int parameterIndex)
{
    switch (parameterIndex)
    {
    case Play:
        return "Play";
    case Stop:
        return "Stop";
    case Looping:
        return "Loop";
    case BPM:
        return "BPM";
    case Position:
        return "Position";
    default:
        return "";
    }
}

//------------------------------------------------------------------------------
float MidiFilePlayerProcessor::getParameter(int parameterIndex)
{
    switch (parameterIndex)
    {
    case Play:
        return playing.load() ? 1.0f : 0.0f;
    case Stop:
        return 0.0f;
    case Looping:
        return looping.load() ? 1.0f : 0.0f;
    case BPM:
        return static_cast<float>((bpm.load() - 20.0) / 280.0); // Normalize 20-300 to 0-1
    case Position:
        return static_cast<float>(getPlaybackPosition());
    default:
        return 0.0f;
    }
}

//------------------------------------------------------------------------------
const String MidiFilePlayerProcessor::getParameterText(int parameterIndex)
{
    switch (parameterIndex)
    {
    case Play:
        return playing.load() ? "Playing" : "Stopped";
    case Stop:
        return "";
    case Looping:
        return looping.load() ? "On" : "Off";
    case BPM:
        return String(bpm.load(), 1) + " BPM";
    case Position:
        return String(getPlaybackPosition() * 100.0, 1) + "%";
    default:
        return "";
    }
}

//------------------------------------------------------------------------------
void MidiFilePlayerProcessor::setParameter(int parameterIndex, float newValue)
{
    switch (parameterIndex)
    {
    case Play:
        if (newValue >= 0.5f)
            play();
        else
            pause();
        break;
    case Stop:
        if (newValue >= 0.5f)
            stop();
        break;
    case Looping:
        setLooping(newValue >= 0.5f);
        break;
    case BPM:
        setBPM(20.0 + newValue * 280.0); // Denormalize 0-1 to 20-300
        break;
    case Position:
        seekToPosition(newValue);
        break;
    }
}

//------------------------------------------------------------------------------
void MidiFilePlayerProcessor::getStateInformation(MemoryBlock& destData)
{
    XmlElement xml("MidiFilePlayer");

    xml.setAttribute("file", midiFile.getFullPathName());
    xml.setAttribute("looping", looping.load());
    xml.setAttribute("bpm", bpm.load());
    xml.setAttribute("position", getPlaybackPosition());

    // Save track mute states
    String muteStates;
    for (bool muted : trackMuteStates)
        muteStates += muted ? "1" : "0";
    xml.setAttribute("trackMutes", muteStates);

    copyXmlToBinary(xml, destData);
}

//------------------------------------------------------------------------------
void MidiFilePlayerProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    auto xml = getXmlFromBinary(data, sizeInBytes);

    if (xml && xml->hasTagName("MidiFilePlayer"))
    {
        String filePath = xml->getStringAttribute("file");
        if (filePath.isNotEmpty())
        {
            File file(filePath);
            if (file.existsAsFile())
                setFile(file);
        }

        setLooping(xml->getBoolAttribute("looping", true));
        setBPM(xml->getDoubleAttribute("bpm", 120.0));

        // Restore track mute states
        String muteStates = xml->getStringAttribute("trackMutes");
        for (int i = 0; i < muteStates.length() && i < (int)trackMuteStates.size(); ++i)
        {
            trackMuteStates[i] = (muteStates[i] == '1');
        }
        rebuildCombinedSequence();

        double position = xml->getDoubleAttribute("position", 0.0);
        seekToPosition(position);
    }
}

//------------------------------------------------------------------------------
AudioProcessorEditor* MidiFilePlayerProcessor::createEditor()
{
    return nullptr; // Uses getControls() instead
}
