/*
  ==============================================================================

    IFilterGraph.h

    Interface abstracting FilterGraph and SubGraphFilterGraph for PluginField.
    This allows PluginField to work with both the main graph and subgraph racks.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <vector>

/**
    Abstract interface for graph management used by PluginField.

    Implemented by:
    - FilterGraph (main application graph)
    - SubGraphFilterGraph (effect rack graph adapter)
*/
class IFilterGraph
{
  public:
    virtual ~IFilterGraph() = default;

    //==============================================================================
    // Core graph access
    virtual juce::AudioProcessorGraph& getGraph() = 0;
    virtual juce::UndoManager& getUndoManager() = 0;

    //==============================================================================
    // Node management
    virtual int getNumFilters() const = 0;
    virtual juce::AudioProcessorGraph::Node::Ptr getNode(int index) const = 0;
    virtual juce::AudioProcessorGraph::Node::Ptr getNodeForId(juce::AudioProcessorGraph::NodeID uid) const = 0;

    //==============================================================================
    // Add/remove filters (with undo)
    virtual void addFilter(const juce::PluginDescription* desc, double x, double y) = 0;
    virtual void removeFilter(juce::AudioProcessorGraph::NodeID id) = 0;
    virtual void disconnectFilter(juce::AudioProcessorGraph::NodeID id) = 0;

    // Raw operations (no undo)
    virtual juce::AudioProcessorGraph::NodeID addFilterRaw(const juce::PluginDescription* desc, double x, double y) = 0;
    virtual void removeFilterRaw(juce::AudioProcessorGraph::NodeID id) = 0;

    //==============================================================================
    // Add/remove connections (with undo)
    virtual bool addConnection(juce::AudioProcessorGraph::NodeID sourceId, int sourceChannel,
                               juce::AudioProcessorGraph::NodeID destId, int destChannel) = 0;
    virtual void removeConnection(juce::AudioProcessorGraph::NodeID sourceId, int sourceChannel,
                                  juce::AudioProcessorGraph::NodeID destId, int destChannel) = 0;

    // Raw operations (no undo)
    virtual bool addConnectionRaw(juce::AudioProcessorGraph::NodeID sourceId, int sourceChannel,
                                  juce::AudioProcessorGraph::NodeID destId, int destChannel) = 0;
    virtual void removeConnectionRaw(juce::AudioProcessorGraph::NodeID sourceId, int sourceChannel,
                                     juce::AudioProcessorGraph::NodeID destId, int destChannel) = 0;

    virtual std::vector<juce::AudioProcessorGraph::Connection> getConnections() const = 0;
    virtual const juce::AudioProcessorGraph::Connection*
    getConnectionBetween(juce::AudioProcessorGraph::NodeID sourceId, int sourceChannel,
                         juce::AudioProcessorGraph::NodeID destId, int destChannel) const = 0;

    //==============================================================================
    // Position management
    virtual void setNodePosition(int nodeId, double x, double y) = 0;
    virtual void getNodePosition(int nodeId, double& x, double& y) const = 0;

    //==============================================================================
    // Check if a node is infrastructure that shouldn't be removed
    virtual bool isHiddenInfrastructureNode(juce::AudioProcessorGraph::NodeID nodeId) const = 0;

    //==============================================================================
    // MIDI channel constant
    static const int midiChannelNumber = 0x1000;
};
