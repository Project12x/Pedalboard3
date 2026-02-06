/*
  ==============================================================================

    ChannelRoutingProcessors.h
    Created: 6 Feb 2026

    Flexible channel routing processors for selecting device channels.
    These act as system nodes similar to Audio Input/Output.

  ==============================================================================
*/

#pragma once

#include "PedalboardProcessors.h"

#include <JuceHeader.h>
#include <atomic>

//==============================================================================
/**
    Device channel input selector - acts as a flexible Audio Input.

    This processor receives input from the audioInputNode and routes
    specific channels to its outputs. It gets automatically connected
    to the audioInputNode when added to the graph.

    Modes:
    - Mono: Selects 1 device channel -> 1 output
    - MonoToStereo: Selects 1 device channel -> duplicated to 2 outputs
    - Stereo: Selects a stereo pair from device -> 2 outputs

    The internal inputs are hidden in the UI to make it appear as a source node.
*/
class ChannelInputProcessor : public PedalboardProcessor
{
  public:
    enum class Mode
    {
        Mono = 0,       // 1 in, 1 out (selected channel)
        MonoToStereo,   // 1 in, 2 out (duplicated)
        Stereo          // 2 in, 2 out (stereo pair)
    };

    ChannelInputProcessor();
    ~ChannelInputProcessor() override;

    // Connect to the graph to access device input channels
    void setGraph(AudioProcessorGraph* g) { graph = g; }

    // Mode and channel configuration
    Mode getMode() const { return mode.load(); }
    void setMode(Mode newMode);

    // For Mono/MonoToStereo: which device channel to use (0-based)
    int getSelectedChannel() const { return selectedChannel.load(); }
    void setSelectedChannel(int channel);

    // For Stereo: which device channel pair (0=1+2, 1=3+4, etc.)
    int getSelectedPair() const { return selectedPair.load(); }
    void setSelectedPair(int pair);

    // PedalboardProcessor interface
    Component* getControls() override;
    Point<int> getSize() override { return Point<int>(120, 80); }

    // AudioProcessor overrides
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void processBlock(AudioBuffer<float>& buffer, MidiBuffer& midiMessages) override;

    AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const String getName() const override { return "Channel Input"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    const String getInputChannelName(int channelIndex) const override { return {}; }
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
    enum Parameters
    {
        ModeParam = 0,
        ChannelParam,
        PairParam,
        NumParameters
    };

    int getNumParameters() override { return NumParameters; }
    float getParameter(int parameterIndex) override;
    void setParameter(int parameterIndex, float newValue) override;
    const String getParameterName(int parameterIndex) override;
    const String getParameterText(int parameterIndex) override;

  private:
    void updateChannelConfig();
    int getOutputChannelCount() const;
    int getInputChannelCount() const;

    AudioProcessorGraph* graph = nullptr;

    std::atomic<Mode> mode{Mode::Stereo};
    std::atomic<int> selectedChannel{0};  // For mono modes (0-based)
    std::atomic<int> selectedPair{0};     // For stereo mode (0-based)

    Rectangle<int> editorBounds;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChannelInputProcessor)
};

//==============================================================================
/**
    Device channel output selector - acts as a sink node like Audio Output.

    This is a system node that routes audio to specific device channels.
    It has input pins and NO output pins (sink).

    Modes:
    - Mono: 1 input channel -> selected device channel
    - StereoToMono: 2 input channels summed -> selected device channel
    - Stereo: 2 input channels -> selected stereo pair

    The processor needs to be connected to the graph to access device outputs.
*/
class ChannelOutputProcessor : public PedalboardProcessor
{
  public:
    enum class Mode
    {
        Mono = 0,       // 1 in, 0 out
        StereoToMono,   // 2 in, 0 out (summed)
        Stereo          // 2 in, 0 out
    };

    ChannelOutputProcessor();
    ~ChannelOutputProcessor() override;

    // Connect to the graph to access device output channels
    void setGraph(AudioProcessorGraph* g) { graph = g; }

    // Mode and channel configuration
    Mode getMode() const { return mode.load(); }
    void setMode(Mode newMode);

    // For Mono/StereoToMono: which device output channel (0-based)
    int getSelectedChannel() const { return selectedChannel.load(); }
    void setSelectedChannel(int channel);

    // For Stereo: which device output pair (0=1+2, 1=3+4, etc.)
    int getSelectedPair() const { return selectedPair.load(); }
    void setSelectedPair(int pair);

    // PedalboardProcessor interface
    Component* getControls() override;
    Point<int> getSize() override { return Point<int>(120, 80); }

    // AudioProcessor overrides
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void processBlock(AudioBuffer<float>& buffer, MidiBuffer& midiMessages) override;

    AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const String getName() const override { return "Channel Output"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    const String getInputChannelName(int channelIndex) const override;
    const String getOutputChannelName(int channelIndex) const override { return {}; }

    int getNumPrograms() override { return 0; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int index) override {}
    const String getProgramName(int index) override { return ""; }
    void changeProgramName(int index, const String& newName) override {}

    void getStateInformation(MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    void fillInPluginDescription(PluginDescription& description) const override;

    // Parameters
    enum Parameters
    {
        ModeParam = 0,
        ChannelParam,
        PairParam,
        NumParameters
    };

    int getNumParameters() override { return NumParameters; }
    float getParameter(int parameterIndex) override;
    void setParameter(int parameterIndex, float newValue) override;
    const String getParameterName(int parameterIndex) override;
    const String getParameterText(int parameterIndex) override;

  private:
    void updateChannelConfig();
    int getInputChannelCount() const;

    AudioProcessorGraph* graph = nullptr;

    std::atomic<Mode> mode{Mode::Stereo};
    std::atomic<int> selectedChannel{0};  // For mono modes (0-based)
    std::atomic<int> selectedPair{0};     // For stereo mode (0-based)

    Rectangle<int> editorBounds;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChannelOutputProcessor)
};
