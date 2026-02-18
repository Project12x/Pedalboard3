#pragma once

#include "PedalboardProcessors.h"

#include <JuceHeader.h>

class NotesControl;

//==============================================================================
/**
    A processor that simply displays text notes on the canvas.
    It has no audio/midi I/O.
*/
class NotesProcessor : public PedalboardProcessor
{
  public:
    NotesProcessor();
    ~NotesProcessor();

    // PedalboardProcessor overrides
    Component* getControls() override;
    Point<int> getSize() override { return Point<int>(200, 150); } // Default size
    void updateEditorBounds(const Rectangle<int>& bounds) { editorBounds = bounds; }

    // AudioPluginInstance overrides
    void fillInPluginDescription(PluginDescription& description) const override;
    const String getName() const override { return "Notes"; }

    void prepareToPlay(double sampleRate, int estimatedSamplesPerBlock) override {}
    void releaseResources() override {}
    void processBlock(AudioSampleBuffer& buffer, MidiBuffer& midiMessages) override {}

    // I/O - Visual only
    const String getInputChannelName(int channelIndex) const override { return ""; }
    const String getOutputChannelName(int channelIndex) const override { return ""; }
    bool isInputChannelStereoPair(int index) const override { return false; }
    bool isOutputChannelStereoPair(int index) const override { return false; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool silenceInProducesSilenceOut() const override { return true; }
    double getTailLengthSeconds() const override { return 0.0; }

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override
    {
        return layouts.inputBuses.isEmpty() && layouts.outputBuses.isEmpty();
    }

    // Editor
    AudioProcessorEditor* createEditor() override; // Dummy editor if needed, but getControls is used
    bool hasEditor() const override { return true; }

    // Parameters (none)
    int getNumParameters() override { return 0; }
    const String getParameterName(int parameterIndex) override { return ""; }
    float getParameter(int parameterIndex) override { return 0.0f; }
    const String getParameterText(int parameterIndex) override { return ""; }
    void setParameter(int parameterIndex, float newValue) override {}

    // Programs (none)
    int getNumPrograms() override { return 0; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int index) override {}
    const String getProgramName(int index) override { return ""; }
    void changeProgramName(int index, const String& newName) override {}

    // State
    void getStateInformation(MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    // Custom
    void setText(const String& newText);
    String getText() const { return currentText; }
    void registerControl(NotesControl* ctrl);
    void unregisterControl(NotesControl* ctrl);

  private:
    String currentText;
    Rectangle<int> editorBounds;

    // Pointer to active UI component (if valid) to sync updates
    Component::SafePointer<NotesControl> activeControl;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NotesProcessor)
};
