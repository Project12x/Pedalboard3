/*
  ==============================================================================

    NAMProcessor.h
    Neural Amp Modeler processor for amp/pedal simulation
    Uses NeuralAmpModelerCore library for machine learning-based DSP

  ==============================================================================
*/

#pragma once

#include "PedalboardProcessors.h"

#include <atomic>
#include <memory>

// Forward declarations for isolated DSP wrappers
class NAMCore;
class NAMConvolver;
class SubGraphProcessor;

//==============================================================================
/**
    Neural Amp Modeler processor for amp/pedal/cab simulation.
    Loads .nam model files trained with the NAM trainer.
    Features:
    - NAM model loading with automatic sample rate conversion
    - Built-in tone stack (bass/mid/treble EQ)
    - Noise gate for clean playing
    - Input/output level controls
    - Optional IR loading for cabinet simulation
*/
class NAMProcessor : public PedalboardProcessor
{
  public:
    NAMProcessor();
    ~NAMProcessor();

    //==========================================================================
    // PedalboardProcessor interface
    Component* getControls() override;
    Point<int> getSize() override { return Point<int>(400, 310); }

    void updateEditorBounds(const juce::Rectangle<int>& bounds);

    //==========================================================================
    // NAM model management
    bool loadModel(const juce::File& modelFile);
    void clearModel();
    bool isModelLoaded() const { return modelLoaded.load(); }
    juce::String getModelName() const;
    const juce::File& getModelFile() const { return currentModelFile; }

    //==========================================================================
    // IR (Cabinet) management
    bool loadIR(const juce::File& irFile);
    void clearIR();
    bool isIRLoaded() const { return irLoaded.load(); }
    juce::String getIRName() const;
    const juce::File& getIRFile() const { return currentIRFile; }

    //==========================================================================
    // Parameters
    float getInputGain() const { return inputGain.load(); }
    void setInputGain(float dB) { inputGain.store(juce::jlimit(-20.0f, 20.0f, dB)); }

    float getOutputGain() const { return outputGain.load(); }
    void setOutputGain(float dB) { outputGain.store(juce::jlimit(-40.0f, 40.0f, dB)); }

    float getNoiseGateThreshold() const { return noiseGateThreshold.load(); }
    void setNoiseGateThreshold(float dB);

    float getBass() const { return bass.load(); }
    void setBass(float value);

    float getMid() const { return mid.load(); }
    void setMid(float value);

    float getTreble() const { return treble.load(); }
    void setTreble(float value);

    bool isToneStackEnabled() const { return toneStackEnabled.load(); }
    void setToneStackEnabled(bool enabled) { toneStackEnabled.store(enabled); }

    bool isNormalizeOutput() const { return normalizeOutput.load(); }
    void setNormalizeOutput(bool enabled) { normalizeOutput.store(enabled); }

    bool isIREnabled() const { return irEnabled.load(); }
    void setIREnabled(bool enabled) { irEnabled.store(enabled); }

    // IR filters (high-pass before IR, low-pass after IR)
    float getIRLowCut() const { return irLowCut.load(); }
    void setIRLowCut(float freqHz);

    float getIRHighCut() const { return irHighCut.load(); }
    void setIRHighCut(float freqHz);

    //==========================================================================
    // Effects Loop (SubGraph between tone stack and IR)
    SubGraphProcessor* getEffectsLoop() { return effectsLoop.get(); }
    bool isEffectsLoopEnabled() const { return effectsLoopEnabled.load(); }
    void setEffectsLoopEnabled(bool enabled) { effectsLoopEnabled.store(enabled); }
    bool hasEffectsLoopContent() const;

    //==========================================================================
    // AudioProcessor overrides
    void fillInPluginDescription(PluginDescription& description) const override;
    void processBlock(AudioSampleBuffer& buffer, MidiBuffer& midiMessages) override;

    const String getName() const override { return "NAM Loader"; }
    void prepareToPlay(double sampleRate, int estimatedSamplesPerBlock) override;
    void releaseResources() override;

    const String getInputChannelName(int channelIndex) const override { return ""; }
    const String getOutputChannelName(int channelIndex) const override { return ""; }
    bool isInputChannelStereoPair(int index) const override { return true; }
    bool isOutputChannelStereoPair(int index) const override { return true; }
    bool silenceInProducesSilenceOut() const override { return true; }
    double getTailLengthSeconds() const override { return 0.5; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }

    AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    // Parameters
    enum Parameters
    {
        InputGainParam = 0,
        OutputGainParam,
        NoiseGateParam,
        BassParam,
        MidParam,
        TrebleParam,
        ToneStackEnabledParam,
        NormalizeParam,
        IRMixParam,
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
    void updateNoiseGate();
    void updateToneStack();
    void updateIRFilters();
    void normalizeModelOutput(float* output, int numSamples);
    static float dBToLinear(float dB);

    //==========================================================================
    // NAM DSP core (isolated from JUCE to avoid namespace conflicts)
    std::unique_ptr<NAMCore> namCore;
    std::atomic<bool> modelLoaded{false};
    juce::File currentModelFile;

    // IR convolution for cabinet simulation (isolated to avoid dsp namespace conflict)
    std::unique_ptr<NAMConvolver> convolver;
    std::atomic<bool> irLoaded{false};
    std::atomic<bool> irEnabled{true};
    juce::File currentIRFile;

    // IR filters (high-pass before convolution, low-pass after)
    std::atomic<float> irLowCut{80.0f};      // Hz (high-pass cutoff)
    std::atomic<float> irHighCut{12000.0f};  // Hz (low-pass cutoff)
    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>,
                                    juce::dsp::IIR::Coefficients<float>> irLowCutFilter;
    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>,
                                    juce::dsp::IIR::Coefficients<float>> irHighCutFilter;

    // Audio-thread-only tracking for lazy coefficient updates
    float lastIRLowCut = 0.0f;
    float lastIRHighCut = 0.0f;

    // Effects loop (SubGraphProcessor for hosting plugins between tone stack and IR)
    std::unique_ptr<SubGraphProcessor> effectsLoop;
    std::atomic<bool> effectsLoopEnabled{false};

    // Processing buffers
    juce::AudioBuffer<float> outputBuffer;

    //==========================================================================
    // Parameters (atomic for thread safety)
    std::atomic<float> inputGain{0.0f};      // dB
    std::atomic<float> outputGain{0.0f};     // dB
    std::atomic<float> noiseGateThreshold{-80.0f}; // dB, -101 = off
    std::atomic<float> bass{5.0f};           // 0-10
    std::atomic<float> mid{5.0f};            // 0-10
    std::atomic<float> treble{5.0f};         // 0-10
    std::atomic<bool> toneStackEnabled{true};
    std::atomic<bool> normalizeOutput{false};

    //==========================================================================
    // State
    double currentSampleRate = 44100.0;
    int currentBlockSize = 512;
    bool isPrepared = false;

    // Noise gate fixed parameters
    static constexpr double kNoiseGateTime = 0.01;
    static constexpr double kNoiseGateRatio = 0.1;
    static constexpr double kNoiseGateOpenTime = 0.001;
    static constexpr double kNoiseGateHoldTime = 0.01;
    static constexpr double kNoiseGateCloseTime = 0.05;

    // Editor bounds
    juce::Rectangle<int> editorBounds;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NAMProcessor)
};
