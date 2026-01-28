/*
  ==============================================================================

    ToneGeneratorProcessor.h
    Test signal generator with boundary testing capabilities
    For tuner accuracy verification and plugin testing

  ==============================================================================
*/

#pragma once

#include "PedalboardProcessors.h"

#include <atomic>

//==============================================================================
/**
    Tone generator for testing:
    - Tuner accuracy verification (headless or visual)
    - Plugin signal testing

    Supports boundary testing with precise frequency control and detuning.
*/
class ToneGeneratorProcessor : public PedalboardProcessor
{
  public:
    //==========================================================================
    // Waveform types
    enum class Waveform
    {
        Sine,
        Saw,
        Square,
        WhiteNoise
    };

    //==========================================================================
    // Test modes for boundary testing
    enum class TestMode
    {
        Static,     // Fixed frequency
        Sweep,      // Continuous sweep through range
        Drift,      // Slow ±5 cent drift
        OctaveJump, // Jump between octaves of same note
        RandomJump  // Random frequency changes
    };

    //==========================================================================
    ToneGeneratorProcessor();
    ~ToneGeneratorProcessor();

    //==========================================================================
    // PedalboardProcessor interface
    Component* getControls() override;
    Point<int> getSize() override { return Point<int>(280, 180); }

    void updateEditorBounds(const Rectangle<int>& bounds);

    //==========================================================================
    // Control methods (thread-safe)
    void setFrequency(float freqHz);
    void setMidiNote(int midiNote);
    void setDetuneCents(float cents);
    void setWaveform(Waveform wf);
    void setTestMode(TestMode mode);
    void setAmplitude(float amp); // 0.0 - 1.0
    void setPlaying(bool shouldPlay);

    float getFrequency() const { return baseFrequency.load(); }
    float getDetuneCents() const { return detuneCents.load(); }
    float getActualFrequency() const; // baseFreq * cents adjustment
    Waveform getWaveform() const { return currentWaveform.load(); }
    TestMode getTestMode() const { return currentTestMode.load(); }
    float getAmplitude() const { return amplitude.load(); }
    bool isPlaying() const { return playing.load(); }

    // Utility: Convert MIDI note to frequency
    static float midiNoteToFrequency(int midiNote);
    static int frequencyToMidiNote(float frequency);
    static float frequencyToCents(float frequency, int targetNote);

    //==========================================================================
    // AudioProcessor overrides
    void fillInPluginDescription(PluginDescription& description) const override;
    void processBlock(AudioSampleBuffer& buffer, MidiBuffer& midiMessages) override;

    const String getName() const override { return "Tone Generator"; }
    void prepareToPlay(double sampleRate, int estimatedSamplesPerBlock) override;
    void releaseResources() override {}

    const String getInputChannelName(int channelIndex) const override { return ""; }
    const String getOutputChannelName(int channelIndex) const override { return ""; }
    bool isInputChannelStereoPair(int index) const override { return false; }
    bool isOutputChannelStereoPair(int index) const override { return false; }
    bool silenceInProducesSilenceOut() const override { return false; } // Generates audio!
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
    //==========================================================================
    // Oscillator generation
    float generateSample();
    void updatePhase();
    void updateTestModeModulation();

    //==========================================================================
    // Atomic parameters (thread-safe)
    std::atomic<float> baseFrequency{440.0f};
    std::atomic<float> detuneCents{0.0f};
    std::atomic<float> amplitude{0.5f};
    std::atomic<Waveform> currentWaveform{Waveform::Sine};
    std::atomic<TestMode> currentTestMode{TestMode::Static};
    std::atomic<bool> playing{false};

    // Oscillator state
    double sampleRate = 44100.0;
    double phase = 0.0;
    double phaseIncrement = 0.0;

    // Test mode state
    float sweepPosition = 0.0f;
    float driftPhase = 0.0f;
    int octaveJumpIndex = 0;

    // Random number generator for noise/random modes
    Random random;

    // Tuning reference
    static constexpr float A4_FREQ = 440.0f;
    static constexpr int A4_MIDI = 69;

    // Editor bounds
    Rectangle<int> editorBounds;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ToneGeneratorProcessor)
};
