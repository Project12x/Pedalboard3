/*
  ==============================================================================

    SubGraphProcessor.h

    A rack/sub-graph processor that contains its own AudioProcessorGraph.
    Allows nesting of effect chains as reusable nodes.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

//==============================================================================
/**
    A processor that wraps an internal AudioProcessorGraph, allowing users to
    create reusable effect chains (racks) that appear as single nodes in the
    main graph.

    Pattern inspired by Kushview Element's "Graph Internal Plugins".
*/
class SubGraphProcessor : public juce::AudioPluginInstance
{
  public:
    //==============================================================================
    SubGraphProcessor();
    ~SubGraphProcessor() override;

    //==============================================================================
    // AudioProcessor interface
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    //==============================================================================
    const juce::String getName() const override { return rackName; }

    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return true; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override;

    // AudioPluginInstance requirement
    void fillInPluginDescription(juce::PluginDescription& description) const override;

    //==============================================================================
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    //==============================================================================
    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    //==============================================================================
    // Sub-graph management

    /** Returns the internal graph for editing. */
    juce::AudioProcessorGraph& getInternalGraph() { return internalGraph; }
    const juce::AudioProcessorGraph& getInternalGraph() const { return internalGraph; }

    /** Load rack configuration from a .rack file. */
    bool loadFromFile(const juce::File& rackFile);

    /** Save rack configuration to a .rack file. */
    bool saveToFile(const juce::File& rackFile);

    /** Get the rack's display name. */
    juce::String getRackName() const { return rackName; }

    /** Set the rack's display name. */
    void setRackName(const juce::String& name) { rackName = name; }

    /** Get the internal audio input node ID. */
    juce::AudioProcessorGraph::NodeID getRackAudioInputNodeId() const { return rackAudioInNode; }

    /** Get the internal audio output node ID. */
    juce::AudioProcessorGraph::NodeID getRackAudioOutputNodeId() const { return rackAudioOutNode; }

    /** Get the internal MIDI input node ID. */
    juce::AudioProcessorGraph::NodeID getRackMidiInputNodeId() const { return rackMidiInNode; }

  private:
    //==============================================================================
    void initializeInternalGraph();
    juce::XmlElement* createRackXml() const;
    void restoreFromRackXml(const juce::XmlElement& xml);

    //==============================================================================
    juce::AudioProcessorGraph internalGraph;
    juce::String rackName = "New Rack";

    // Fixed internal I/O node IDs
    juce::AudioProcessorGraph::NodeID rackAudioInNode;
    juce::AudioProcessorGraph::NodeID rackAudioOutNode;
    juce::AudioProcessorGraph::NodeID rackMidiInNode;

    // Cached sample rate and block size for initialization
    double currentSampleRate = 44100.0;
    int currentBlockSize = 512;

    // Thread safety: atomic flag for deferred initialization pattern
    // Internal graph is initialized on FIRST processBlock call (audio thread)
    // This eliminates message thread / audio thread race condition
    std::atomic<bool> internalGraphInitialized{false};

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SubGraphProcessor)
};
