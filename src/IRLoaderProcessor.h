/*
  ==============================================================================

    IRLoaderProcessor.h
    Cabinet Impulse Response loader using JUCE's built-in convolution

  ==============================================================================
*/

#pragma once

#include <juce_dsp/juce_dsp.h>

#include "PedalboardProcessors.h"

#include <atomic>


//==============================================================================
/**
    IR Loader processor for cabinet simulation.
    Uses juce::dsp::Convolution for FFT-based convolution.
    Supports .wav and .aiff impulse response files.
*/
class IRLoaderProcessor : public PedalboardProcessor
{
  public:
    IRLoaderProcessor();
    ~IRLoaderProcessor();

    //==========================================================================
    // PedalboardProcessor interface
    Component* getControls() override;
    Point<int> getSize() override { return Point<int>(280, 150); }

    void updateEditorBounds(const Rectangle<int>& bounds);

    //==========================================================================
    // IR file management
    void loadIRFile(const File& irFile);
    const File& getIRFile() const { return currentIRFile; }
    bool isIRLoaded() const { return irLoaded.load(); }
    String getIRName() const { return currentIRFile.getFileNameWithoutExtension(); }

    //==========================================================================
    // Parameters
    float getMix() const { return mix.load(); }
    void setMix(float newMix) { mix.store(juce::jlimit(0.0f, 1.0f, newMix)); }

    float getLowCut() const { return lowCut.load(); }
    void setLowCut(float freqHz)
    {
        lowCut.store(juce::jlimit(20.0f, 500.0f, freqHz));
        updateFilters();
    }

    float getHighCut() const { return highCut.load(); }
    void setHighCut(float freqHz)
    {
        highCut.store(juce::jlimit(2000.0f, 20000.0f, freqHz));
        updateFilters();
    }

    //==========================================================================
    // AudioProcessor overrides
    void fillInPluginDescription(PluginDescription& description) const override;
    void processBlock(AudioSampleBuffer& buffer, MidiBuffer& midiMessages) override;

    const String getName() const override { return "IR Loader"; }
    void prepareToPlay(double sampleRate, int estimatedSamplesPerBlock) override;
    void releaseResources() override {}

    const String getInputChannelName(int channelIndex) const override { return ""; }
    const String getOutputChannelName(int channelIndex) const override { return ""; }
    bool isInputChannelStereoPair(int index) const override { return true; }
    bool isOutputChannelStereoPair(int index) const override { return true; }
    bool silenceInProducesSilenceOut() const override { return true; }
    double getTailLengthSeconds() const override { return 0.5; } // IR tail length
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }

    AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    // Parameters
    enum Parameters
    {
        MixParam = 0,
        LowCutParam,
        HighCutParam,
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
    void updateFilters();

    //==========================================================================
    // Convolution engine
    juce::dsp::Convolution convolver;
    juce::dsp::ProcessSpec spec;

    // Pre/post filters for tone shaping
    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>> lowCutFilter;
    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>> highCutFilter;

    // Dry buffer for wet/dry mixing
    juce::AudioBuffer<float> dryBuffer;

    //==========================================================================
    // State
    File currentIRFile;
    std::atomic<bool> irLoaded{false};
    std::atomic<float> mix{1.0f};         // 0 = dry, 1 = wet
    std::atomic<float> lowCut{80.0f};     // Hz
    std::atomic<float> highCut{12000.0f}; // Hz

    double currentSampleRate = 44100.0;
    bool isPrepared = false;

    // Editor bounds
    Rectangle<int> editorBounds;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(IRLoaderProcessor)
};
