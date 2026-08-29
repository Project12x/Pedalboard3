#pragma once

#include "PedalboardProcessors.h"

class LinkAudioInputProcessor : public PedalboardProcessor
{
  public:
    LinkAudioInputProcessor();
    juce::Component* getControls() override;
    juce::Point<int> getSize() override { return {160, 48}; }
    void fillInPluginDescription(juce::PluginDescription& description) const override;
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) override;
    const juce::String getName() const override { return "Link Audio Input"; }
    void prepareToPlay(double, int) override {}
    void releaseResources() override {}
    const juce::String getInputChannelName(int) const override { return {}; }
    const juce::String getOutputChannelName(int) const override { return {}; }
    bool isInputChannelStereoPair(int) const override { return false; }
    bool isOutputChannelStereoPair(int) const override { return false; }
    bool silenceInProducesSilenceOut() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    juce::AudioProcessorEditor* createEditor() override { return nullptr; }
    bool hasEditor() const override { return false; }
    int getNumParameters() override { return 0; }
    const juce::String getParameterName(int) override { return {}; }
    float getParameter(int) override { return 0.0f; }
    const juce::String getParameterText(int) override { return {}; }
    void setParameter(int, float) override {}
    int getNumPrograms() override { return 0; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override {}
    void setStateInformation(const void*, int) override {}
};
