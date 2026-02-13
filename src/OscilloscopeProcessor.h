/*
  ==============================================================================

    OscilloscopeProcessor.h
    Real-time audio oscilloscope with embedded display

  ==============================================================================
*/

#pragma once

#include "PedalboardProcessors.h"
#include <array>
#include <atomic>

//==============================================================================
/**
    Simple real-time oscilloscope processor with embedded waveform display.
    Uses zero-crossing trigger for stable waveform visualization.
*/
class OscilloscopeProcessor : public PedalboardProcessor
{
  public:
    OscilloscopeProcessor();
    ~OscilloscopeProcessor();

    //==========================================================================
    // PedalboardProcessor interface
    Component* getControls() override;
    Point<int> getSize() override { return Point<int>(240, 120); }

    void updateEditorBounds(const Rectangle<int>& bounds);

    //==========================================================================
    // Thread-safe data access for UI
    static constexpr int DISPLAY_SAMPLES = 512;
    void getDisplayBuffer(std::array<float, DISPLAY_SAMPLES>& output) const;
    float getTriggerLevel() const { return triggerLevel.load(); }
    void setTriggerLevel(float level) { triggerLevel.store(level); }

    //==========================================================================
    // AudioProcessor overrides
    void fillInPluginDescription(PluginDescription& description) const override;
    void processBlock(AudioSampleBuffer& buffer, MidiBuffer& midiMessages) override;

    const String getName() const override { return "Oscilloscope"; }
    void prepareToPlay(double sampleRate, int estimatedSamplesPerBlock) override;
    void releaseResources() override {}

    const String getInputChannelName(int channelIndex) const override { return ""; }
    const String getOutputChannelName(int channelIndex) const override { return ""; }
    bool isInputChannelStereoPair(int index) const override { return false; }
    bool isOutputChannelStereoPair(int index) const override { return false; }
    bool silenceInProducesSilenceOut() const override { return true; }
    double getTailLengthSeconds() const override { return 0.0; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }

    AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    int getNumParameters() override { return 0; }
    const String getParameterName(int parameterIndex) override { return ""; }
    float getParameter(int parameterIndex) override { return 0.0f; }
    const String getParameterText(int parameterIndex) override { return ""; }
    void setParameter(int parameterIndex, float newValue) override {}

    int getNumPrograms() override { return 0; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int index) override {}
    const String getProgramName(int index) override { return ""; }
    void changeProgramName(int index, const String& newName) override {}

    void getStateInformation(MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

  private:
    // Circular buffer for audio capture
    static constexpr int BUFFER_SIZE = 2048;
    std::array<float, BUFFER_SIZE> circularBuffer;
    int writePos = 0;

    // Double-buffer for thread-safe display.
    // Audio writes to displayBuffers[backBuffer], flips frontBuffer on capture complete.
    // UI reads from displayBuffers[frontBuffer.load()].
    std::array<float, DISPLAY_SAMPLES> displayBuffers[2];
    std::atomic<int> frontBuffer{0};
    int backBuffer = 1;

    // Simple zero-crossing trigger
    std::atomic<float> triggerLevel{0.0f};
    bool lastSampleWasNegative = true;
    int samplesSinceTrigger = 0;

    double currentSampleRate = 44100.0;
    Rectangle<int> editorBounds;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OscilloscopeProcessor)
};
