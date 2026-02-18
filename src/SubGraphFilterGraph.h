/*
  ==============================================================================

    SubGraphFilterGraph.h

    Adapter that provides FilterGraph-compatible API over SubGraphProcessor's
    internal AudioProcessorGraph. Enables reusing PluginField in rack editor.

  ==============================================================================
*/

#pragma once

#include "IFilterGraph.h"

#include <JuceHeader.h>

class SubGraphProcessor;

/**
    Lightweight FilterGraph adapter for SubGraphProcessor.

    Provides same core API as FilterGraph but operates on SubGraphProcessor's
    internal graph. Each SubGraphFilterGraph has its own UndoManager for
    per-rack undo/redo support.

    This allows reusing PluginField and PluginComponent inside Effect Racks.
*/
class SubGraphFilterGraph : public IFilterGraph
{
  public:
    //==============================================================================
    SubGraphFilterGraph(SubGraphProcessor& owner);
    ~SubGraphFilterGraph() override;

    //==============================================================================
    // IFilterGraph implementation
    juce::AudioProcessorGraph& getGraph() override;
    juce::UndoManager& getUndoManager() override { return undoManager; }

    int getNumFilters() const override;
    juce::AudioProcessorGraph::Node::Ptr getNode(int index) const override;
    juce::AudioProcessorGraph::Node::Ptr getNodeForId(juce::AudioProcessorGraph::NodeID uid) const override;

    void addFilter(const juce::PluginDescription* desc, double x, double y) override;
    void removeFilter(juce::AudioProcessorGraph::NodeID id) override;
    void disconnectFilter(juce::AudioProcessorGraph::NodeID id) override;

    juce::AudioProcessorGraph::NodeID addFilterRaw(const juce::PluginDescription* desc, double x, double y) override;
    void removeFilterRaw(juce::AudioProcessorGraph::NodeID id) override;

    bool addConnection(juce::AudioProcessorGraph::NodeID sourceId, int sourceChannel,
                       juce::AudioProcessorGraph::NodeID destId, int destChannel) override;
    void removeConnection(juce::AudioProcessorGraph::NodeID sourceId, int sourceChannel,
                          juce::AudioProcessorGraph::NodeID destId, int destChannel) override;

    bool addConnectionRaw(juce::AudioProcessorGraph::NodeID sourceId, int sourceChannel,
                          juce::AudioProcessorGraph::NodeID destId, int destChannel) override;
    void removeConnectionRaw(juce::AudioProcessorGraph::NodeID sourceId, int sourceChannel,
                             juce::AudioProcessorGraph::NodeID destId, int destChannel) override;

    std::vector<juce::AudioProcessorGraph::Connection> getConnections() const override;
    bool getConnectionBetween(juce::AudioProcessorGraph::NodeID sourceId, int sourceChannel,
                              juce::AudioProcessorGraph::NodeID destId, int destChannel) const override;

    void setNodePosition(int nodeId, double x, double y) override;
    void getNodePosition(int nodeId, double& x, double& y) const override;

    void changed();

    bool isHiddenInfrastructureNode(juce::AudioProcessorGraph::NodeID nodeId) const override;

    //==============================================================================
    // SubGraphFilterGraph-specific (not in interface)
    bool isFixedIONode(juce::AudioProcessorGraph::NodeID nodeId) const;

  private:
    SubGraphProcessor& processor;
    juce::UndoManager undoManager;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SubGraphFilterGraph)
};
