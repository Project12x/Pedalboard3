/*
  ==============================================================================

    TunerProcessor.h
    Dual-mode chromatic tuner: Simple (YIN) and Pro (Strobe) modes

  ==============================================================================
*/

#pragma once

#include "PedalboardProcessors.h"

#include <array>
#include <atomic>

//==============================================================================
/**
    Chromatic tuner with two modes:
    - Simple: YIN-based pitch detection (±2 cents)
    - Pro: Phase-based strobe for ±0.1 cent accuracy
*/
class TunerProcessor : public PedalboardProcessor
{
  public:
    TunerProcessor();
    ~TunerProcessor();

    //==========================================================================
    // PedalboardProcessor interface
    Component* getControls() override;
    Point<int> getSize() override { return Point<int>(300, 200); }

    void updateEditorBounds(const Rectangle<int>& bounds);

    //==========================================================================
    // Pitch detection results (thread-safe getters)
    float getDetectedFrequency() const { return detectedFrequency.load(); }
    float getCentsDeviation() const { return centsDeviation.load(); }
    int getDetectedNote() const { return detectedNote.load(); }
    bool isPitchDetected() const { return pitchDetected.load(); }

    /// For strobe mode: phase accumulator (0-1)
    float getStrobePhase() const { return strobePhase.load(); }

    //==========================================================================
    // AudioProcessor overrides
    void fillInPluginDescription(PluginDescription& description) const override;
    void processBlock(AudioSampleBuffer& buffer, MidiBuffer& midiMessages) override;

    const String getName() const override { return "Tuner"; }
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

    /// Sets whether the tuner should mute its output (pass silence)
    void setMuteOutput(bool shouldMute) { muteOutput.store(shouldMute); }

  private:
    std::atomic<bool> muteOutput{false};

    //==========================================================================
    // YIN pitch detection
    float detectPitchYIN(const float* samples, int numSamples);

    // Calculate cents deviation from nearest note
    void updateNoteAndCents(float frequency);

    // Update strobe phase based on frequency error
    void updateStrobePhase(float frequency);

    //==========================================================================
    // Audio buffer for analysis
    static constexpr int BUFFER_SIZE = 2048;
    std::array<float, BUFFER_SIZE> analysisBuffer;
    int bufferWritePos = 0;

    // YIN working arrays
    std::array<float, BUFFER_SIZE / 2> yinBuffer;

    // Detection results (atomic for thread safety)
    std::atomic<float> detectedFrequency{0.0f};
    std::atomic<float> centsDeviation{0.0f};
    std::atomic<int> detectedNote{-1};
    std::atomic<bool> pitchDetected{false};
    std::atomic<float> strobePhase{0.0f};

    // Processing state
    double sampleRate = 44100.0;
    int samplesUntilNextAnalysis = 0;
    static constexpr int ANALYSIS_HOP = 512;

    // Tuning reference (A4 = 440 Hz)
    static constexpr float A4_FREQ = 440.0f;
    static constexpr int A4_MIDI = 69;

    // Editor bounds
    Rectangle<int> editorBounds;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TunerProcessor)
};
