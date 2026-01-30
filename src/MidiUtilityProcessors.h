/*
  ==============================================================================

    MidiUtilityProcessors.h
    Pedalboard3 - MIDI Utility Processors

    MIDI processors for transpose, rechannelize, and keyboard split.

  ==============================================================================
*/

#pragma once

#include "PedalboardProcessors.h"

#include <JuceHeader.h>

//==============================================================================
/**
    Transposes all MIDI note messages by a configurable number of semitones.
    Range: -48 to +48 semitones (4 octaves each direction).

    Audio: Passes through unchanged.
    MIDI:  Note On/Off messages are transposed, other messages pass through.
*/
class MidiTransposeProcessor : public PedalboardProcessor
{
  public:
    MidiTransposeProcessor();
    ~MidiTransposeProcessor() override;

    // PedalboardProcessor overrides
    Component* getControls() override;
    Point<int> getSize() override { return Point<int>(100, 60); }

    // AudioProcessor overrides
    void prepareToPlay(double sampleRate, int samplesPerBlock) override {}
    void releaseResources() override {}
    void processBlock(AudioBuffer<float>& buffer, MidiBuffer& midiMessages) override;

    AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const String getName() const override { return "MIDI Transpose"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return true; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 0; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int index) override {}
    const String getProgramName(int index) override { return ""; }
    void changeProgramName(int index, const String& newName) override {}

    void getStateInformation(MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    void fillInPluginDescription(PluginDescription& description) const override;

    // Parameters
    void setTranspose(int semitones);
    int getTranspose() const { return transpose.load(); }

    enum Parameters
    {
        TransposeParam = 0,
        NumParameters
    };

    int getNumParameters() override { return NumParameters; }
    float getParameter(int parameterIndex) override;
    void setParameter(int parameterIndex, float newValue) override;
    const String getParameterName(int parameterIndex) override;
    const String getParameterText(int parameterIndex) override;

  private:
    std::atomic<int> transpose{0}; // -48 to +48 semitones

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MidiTransposeProcessor)
};

//==============================================================================
/**
    Rechannelizes all MIDI messages from any input channel to a specified output channel.
    Also supports filtering to only pass messages from a specific input channel.

    Audio: Passes through unchanged.
    MIDI:  All channel voice messages are rechannelized.
*/
class MidiRechannelizeProcessor : public PedalboardProcessor
{
  public:
    MidiRechannelizeProcessor();
    ~MidiRechannelizeProcessor() override;

    // PedalboardProcessor overrides
    Component* getControls() override;
    Point<int> getSize() override { return Point<int>(100, 80); }

    // AudioProcessor overrides
    void prepareToPlay(double sampleRate, int samplesPerBlock) override {}
    void releaseResources() override {}
    void processBlock(AudioBuffer<float>& buffer, MidiBuffer& midiMessages) override;

    AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const String getName() const override { return "MIDI Rechannelize"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return true; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 0; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int index) override {}
    const String getProgramName(int index) override { return ""; }
    void changeProgramName(int index, const String& newName) override {}

    void getStateInformation(MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    void fillInPluginDescription(PluginDescription& description) const override;

    // Parameters
    void setInputChannel(int channel); // 0 = All, 1-16 = specific channel
    int getInputChannel() const { return inputChannel.load(); }

    void setOutputChannel(int channel); // 1-16
    int getOutputChannel() const { return outputChannel.load(); }

    enum Parameters
    {
        InputChannelParam = 0,
        OutputChannelParam,
        NumParameters
    };

    int getNumParameters() override { return NumParameters; }
    float getParameter(int parameterIndex) override;
    void setParameter(int parameterIndex, float newValue) override;
    const String getParameterName(int parameterIndex) override;
    const String getParameterText(int parameterIndex) override;

  private:
    std::atomic<int> inputChannel{0};  // 0 = All, 1-16 = specific
    std::atomic<int> outputChannel{1}; // 1-16

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MidiRechannelizeProcessor)
};

//==============================================================================
/**
    Splits the keyboard at a configurable split point.
    Notes below the split point go to Output A (channel 1).
    Notes at or above the split point go to Output B (channel 2).

    Audio: Passes through unchanged.
    MIDI:  Note messages are routed based on split point, other messages pass through.
*/
class KeyboardSplitProcessor : public PedalboardProcessor
{
  public:
    KeyboardSplitProcessor();
    ~KeyboardSplitProcessor() override;

    // PedalboardProcessor overrides
    Component* getControls() override;
    Point<int> getSize() override { return Point<int>(160, 100); }

    // AudioProcessor overrides
    void prepareToPlay(double sampleRate, int samplesPerBlock) override {}
    void releaseResources() override {}
    void processBlock(AudioBuffer<float>& buffer, MidiBuffer& midiMessages) override;

    AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const String getName() const override { return "Keyboard Split"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return true; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 0; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int index) override {}
    const String getProgramName(int index) override { return ""; }
    void changeProgramName(int index, const String& newName) override {}

    void getStateInformation(MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    void fillInPluginDescription(PluginDescription& description) const override;

    // Parameters
    void setSplitPoint(int midiNote); // 0-127 (default C4 = 60)
    int getSplitPoint() const { return splitPoint.load(); }

    void setLowerChannel(int channel); // 1-16
    int getLowerChannel() const { return lowerChannel.load(); }

    void setUpperChannel(int channel); // 1-16
    int getUpperChannel() const { return upperChannel.load(); }

    enum Parameters
    {
        SplitPointParam = 0,
        LowerChannelParam,
        UpperChannelParam,
        NumParameters
    };

    int getNumParameters() override { return NumParameters; }
    float getParameter(int parameterIndex) override;
    void setParameter(int parameterIndex, float newValue) override;
    const String getParameterName(int parameterIndex) override;
    const String getParameterText(int parameterIndex) override;

    // Helper to get note name from MIDI number
    static String getNoteNameFromMidi(int midiNote);

  private:
    std::atomic<int> splitPoint{60};  // Default: C4
    std::atomic<int> lowerChannel{1}; // 1-16
    std::atomic<int> upperChannel{2}; // 1-16

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(KeyboardSplitProcessor)
};
