//  MidiFilePlayer.h - MIDI file playback processor for feeding synths.
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

#ifndef MIDIFILEPLAYER_H_
#define MIDIFILEPLAYER_H_

#include "PedalboardProcessors.h"

#include <JuceHeader.h>
#include <atomic>

class MidiFilePlayerControl;

//------------------------------------------------------------------------------
/// Processor which plays back a MIDI file and outputs MIDI events.
class MidiFilePlayerProcessor : public PedalboardProcessor, public ChangeBroadcaster
{
  public:
    /// Constructor.
    MidiFilePlayerProcessor();
    /// Destructor.
    ~MidiFilePlayerProcessor();

    /// Sets the MIDI file to play.
    bool setFile(const File& file);
    /// Returns the current MIDI file.
    const File& getFile() const { return midiFile; }

    /// Returns the current playback position (0.0 - 1.0).
    double getPlaybackPosition() const;
    /// Returns the length in seconds.
    double getLengthInSeconds() const { return lengthInSeconds; }
    /// Returns whether playback is active.
    bool isPlaying() const { return playing.load(); }
    /// Returns whether looping is enabled.
    bool isLooping() const { return looping.load(); }

    /// Returns the BPM (beats per minute).
    double getBPM() const { return bpm.load(); }
    /// Sets the BPM.
    void setBPM(double newBPM);

    /// Start playback.
    void play();
    /// Pause playback.
    void pause();
    /// Stop and return to zero.
    void stop();
    /// Toggle looping.
    void setLooping(bool shouldLoop) { looping.store(shouldLoop); }
    /// Seek to position (0.0 - 1.0).
    void seekToPosition(double position);

    /// Returns the number of tracks in the MIDI file.
    int getNumTracks() const { return numTracks; }
    /// Returns whether a track is muted.
    bool isTrackMuted(int trackIndex) const;
    /// Sets track mute state.
    void setTrackMuted(int trackIndex, bool muted);

    //--------------------------------------------------------------------------
    // PedalboardProcessor interface
    Component* getControls() override;
    Point<int> getSize() override { return Point<int>(320, 120); }

    void fillInPluginDescription(PluginDescription& description) const override;
    void processBlock(AudioSampleBuffer& buffer, MidiBuffer& midiMessages) override;

    const String getName() const override { return "MIDI File Player"; }
    void prepareToPlay(double sampleRate, int estimatedSamplesPerBlock) override;
    void releaseResources() override {}

    const String getInputChannelName(int channelIndex) const override { return ""; }
    const String getOutputChannelName(int channelIndex) const override { return ""; }
    bool isInputChannelStereoPair(int index) const override { return false; }
    bool isOutputChannelStereoPair(int index) const override { return false; }
    bool silenceInProducesSilenceOut() const override { return true; }
    double getTailLengthSeconds() const override { return 0.0; }

    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return true; }

    AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    /// Parameter indices
    enum Parameters
    {
        Play = 0,
        Stop,
        Looping,
        BPM,
        Position,

        NumParameters
    };

    int getNumParameters() override { return NumParameters; }
    const String getParameterName(int parameterIndex) override;
    float getParameter(int parameterIndex) override;
    const String getParameterText(int parameterIndex) override;
    void setParameter(int parameterIndex, float newValue) override;

    int getNumPrograms() override { return 0; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int index) override {}
    const String getProgramName(int index) override { return ""; }
    void changeProgramName(int index, const String& newName) override {}

    void getStateInformation(MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

  private:
    /// The MIDI file being played.
    File midiFile;

    /// Parsed MIDI data - combined sequence for playback.
    MidiMessageSequence combinedSequence;

    /// Track sequences (for muting individual tracks).
    OwnedArray<MidiMessageSequence> trackSequences;

    /// Track mute states.
    std::vector<bool> trackMuteStates;

    /// Number of tracks.
    int numTracks = 0;

    /// Playback state.
    std::atomic<bool> playing{false};
    std::atomic<bool> looping{true};
    std::atomic<double> bpm{120.0};

    /// Current playback position in seconds.
    std::atomic<double> playheadSeconds{0.0};

    /// Length of the sequence in seconds.
    double lengthInSeconds = 0.0;

    /// Original tempo from the MIDI file.
    double originalBPM = 120.0;

    /// Sample rate for timing calculations.
    double currentSampleRate = 44100.0;

    /// Index of the next event to play.
    int nextEventIndex = 0;

    /// Rebuild the combined sequence (respecting mutes).
    void rebuildCombinedSequence();

    /// Reset playhead to start.
    void resetPlayhead();

    /// Editor bounds for state saving.
    Rectangle<int> editorBounds;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MidiFilePlayerProcessor)
};

#endif // MIDIFILEPLAYER_H_
