/*
  ==============================================================================

    VirtualMidiInputProcessor.h
    Pedalboard3 - Virtual MIDI Keyboard Input

    Produces MIDI output from the on-screen virtual keyboard.

  ==============================================================================
*/

#pragma once

#include "PedalboardProcessors.h"

#include <JuceHeader.h>
#include <atomic>

//==============================================================================
/**
    Produces MIDI output from the on-screen virtual keyboard.
    This processor receives MIDI messages from the MidiKeyboardComponent
    in MainPanel and outputs them to the audio graph.

    Audio: Passes through unchanged.
    MIDI:  Outputs messages received from the virtual keyboard.
*/
class VirtualMidiInputProcessor : public PedalboardProcessor
{
  public:
    VirtualMidiInputProcessor();
    ~VirtualMidiInputProcessor() override;

    // PedalboardProcessor overrides
    Component* getControls() override;
    Point<int> getSize() override { return Point<int>(100, 40); }

    // AudioProcessor overrides
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void processBlock(AudioBuffer<float>& buffer, MidiBuffer& midiMessages) override;

    AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return false; }

    const String getName() const override { return "Virtual MIDI Input"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return true; }
    double getTailLengthSeconds() const override { return 0.0; }

    // MIDI-only processor - no audio buses
    bool isBusesLayoutSupported(const AudioProcessor::BusesLayout& layouts) const override
    {
        return layouts.getMainInputChannels() == 0 && layouts.getMainOutputChannels() == 0;
    }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int index) override {}
    const String getProgramName(int index) override { return ""; }
    void changeProgramName(int index, const String& newName) override {}

    void getStateInformation(MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    void fillInPluginDescription(PluginDescription& description) const override;

    //==========================================================================
    // Parameters
    enum Parameters
    {
        OctaveShiftParam = 0,
        VelocityParam,
        SustainParam,
        NumParameters
    };

    int getNumParameters() override { return NumParameters; }
    float getParameter(int parameterIndex) override;
    void setParameter(int parameterIndex, float newValue) override;
    const String getParameterName(int parameterIndex) override;
    const String getParameterText(int parameterIndex) override;

    // Octave shift (-3..+3, shifts displayed keyboard range)
    int getOctaveShift() const { return octaveShift.load(); }
    void setOctaveShift(int shift) { octaveShift.store(jlimit(-3, 3, shift)); }

    // Fixed velocity for mouse/QWERTY input (1-127)
    int getFixedVelocity() const { return fixedVelocity.load(); }
    void setFixedVelocity(int vel) { fixedVelocity.store(jlimit(1, 127, vel)); }

    // Sustain pedal toggle
    bool isSustainHeld() const { return sustainHeld.load(); }
    void setSustainHeld(bool held);

    // Called from UI thread to inject MIDI messages from the virtual keyboard
    void addMidiMessage(const MidiMessage& msg);

    // Static instance accessor (set when processor is created)
    static VirtualMidiInputProcessor* getInstance();
    static void setInstance(VirtualMidiInputProcessor* instance);

  private:
    // Thread-safe MIDI message collection
    MidiMessageCollector midiCollector;
    double currentSampleRate = 44100.0;

    // Parameters
    std::atomic<int> octaveShift{0};     // -3..+3 octaves
    std::atomic<int> fixedVelocity{100}; // 1-127
    std::atomic<bool> sustainHeld{false};

    // DEBUG: processBlock call counter for periodic logging
    int processBlockCallCount = 0;

    // Static instance pointer for keyboard routing
    static VirtualMidiInputProcessor* instance;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VirtualMidiInputProcessor)
};
