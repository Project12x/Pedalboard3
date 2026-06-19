// ReverbSCProcessor.h - Built-in ReverbSC effect node.
//
// Reverb algorithm close-port reference:
// - Csound Opcodes/reverbsc.c, commit 2932c7fd14681493b5db83df3efdda175c1eb116
// - Soundpipe modules/revsc.c, commit 3efb43bdabd0ed23b17c694292b5a79f1692a3ea

#pragma once

#include "PedalboardProcessors.h"
#include "dsp/ReverbSC.h"

#include <atomic>

class ReverbSCProcessor : public PedalboardProcessor
{
  public:
    enum Parameters
    {
        MixParam = 0,
        FeedbackParam,
        DampingParam,
        WidthParam,
        OutputParam,
        NumParameters
    };

    ReverbSCProcessor();
    ~ReverbSCProcessor() override = default;

    Component* getControls() override;
    Point<int> getSize() override { return Point<int>(220, 96); }
    void updateEditorBounds(const Rectangle<int>& bounds);

    void fillInPluginDescription(PluginDescription& description) const override;
    void processBlock(AudioSampleBuffer& buffer, MidiBuffer& midiMessages) override;

    const String getName() const override { return "ReverbSC"; }
    void prepareToPlay(double sampleRate, int estimatedSamplesPerBlock) override;
    void releaseResources() override {}

    const String getInputChannelName(int channelIndex) const override { return ""; }
    const String getOutputChannelName(int channelIndex) const override { return ""; }
    bool isInputChannelStereoPair(int index) const override { return true; }
    bool isOutputChannelStereoPair(int index) const override { return true; }
    bool silenceInProducesSilenceOut() const override { return false; }
    double getTailLengthSeconds() const override { return 5.0; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }

    AudioProcessorEditor* createEditor() override { return nullptr; }
    bool hasEditor() const override { return true; }

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
    static float normalisedToDampingHz(float value) noexcept;
    static float normalisedToOutputGain(float value) noexcept;
    static float clampNormalised(float value) noexcept;

    pedalboard3::dsp::ReverbSC reverb;
    AudioBuffer<float> dryBuffer;
    AudioBuffer<float> wetBuffer;

    std::atomic<float> mix{0.35f};
    std::atomic<float> feedback{0.97f};
    std::atomic<float> damping{0.4949495f};
    std::atomic<float> width{1.0f};
    std::atomic<float> output{0.5f};

    int maxPreparedBlockSize = 0;
    bool prepared = false;
    Rectangle<int> editorBounds;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ReverbSCProcessor)
};
