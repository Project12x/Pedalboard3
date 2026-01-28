/*
  ==============================================================================

    RoutingProcessors.h
    Created: 27 Jan 2026

    Processors for A/B routing (Splitter and Mixer).

  ==============================================================================
*/

#pragma once

#include "PedalboardProcessors.h"

#include <JuceHeader.h>

//==============================================================================
/**
    Splits a stereo input into two stereo pairs (A and B).
    Input:  2 channels (Stereo)
    Output: 4 channels (Stereo A + Stereo B)
*/
class SplitterProcessor : public PedalboardProcessor
{
  public:
    SplitterProcessor();
    ~SplitterProcessor() override;

    // PedalboardProcessor overrides
    Component* getControls() override;
    Point<int> getSize() override { return Point<int>(100, 60); }

    // AudioProcessor overrides
    void prepareToPlay(double sampleRate, int samplesPerBlock) override {}
    void releaseResources() override {}
    void processBlock(AudioBuffer<float>& buffer, MidiBuffer& midiMessages) override;

    AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const String getName() const override { return "Splitter"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    const String getInputChannelName(int channelIndex) const override;
    const String getOutputChannelName(int channelIndex) const override;

    int getNumPrograms() override { return 0; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int index) override {}
    const String getProgramName(int index) override { return ""; }
    void changeProgramName(int index, const String& newName) override {}

    void getStateInformation(MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    void fillInPluginDescription(PluginDescription& description) const override;

    // Parameters
    void setOutputMute(int outputIndex, bool shouldMute);
    bool getOutputMute(int outputIndex) const;

  private:
    std::atomic<bool> muteA{false};
    std::atomic<bool> muteB{false};

    Rectangle<int> editorBounds;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SplitterProcessor)
};

//==============================================================================
/**
    Mixes two stereo pairs (A and B) into one stereo output.
    Input:  4 channels (Stereo A + Stereo B)
    Output: 2 channels (Stereo Mix)
*/
class MixerProcessor : public PedalboardProcessor
{
  public:
    MixerProcessor();
    ~MixerProcessor() override;

    // PedalboardProcessor overrides
    Component* getControls() override;
    Point<int> getSize() override { return Point<int>(100, 100); }

    // AudioProcessor overrides
    void prepareToPlay(double sampleRate, int samplesPerBlock) override {}
    void releaseResources() override {}
    void processBlock(AudioBuffer<float>& buffer, MidiBuffer& midiMessages) override;

    AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const String getName() const override { return "Mixer"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    const String getInputChannelName(int channelIndex) const override;
    const String getOutputChannelName(int channelIndex) const override;

    int getNumPrograms() override { return 0; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int index) override {}
    const String getProgramName(int index) override { return ""; }
    void changeProgramName(int index, const String& newName) override {}

    void getStateInformation(MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    void fillInPluginDescription(PluginDescription& description) const override;

    // Parameters
    void setLevelA(float level);
    float getLevelA() const { return levelA; }

    void setLevelB(float level);
    float getLevelB() const { return levelB; }

    // Parameter handling
    enum Parameters
    {
        LevelA = 0,
        LevelB,
        NumParameters
    };

    int getNumParameters() override { return NumParameters; }
    float getParameter(int parameterIndex) override;
    void setParameter(int parameterIndex, float newValue) override;
    const String getParameterName(int parameterIndex) override;
    const String getParameterText(int parameterIndex) override;

  private:
    float levelA{0.707f}; // Unityish
    float levelB{0.707f};

    Rectangle<int> editorBounds;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MixerProcessor)
};
