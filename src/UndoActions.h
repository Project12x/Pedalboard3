/*
  ==============================================================================

    UndoActions.h
    Part of Pedalboard3

    Undoable actions for plugin and connection operations.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

class FilterGraph;

//==============================================================================
/**
    Base class for FilterGraph undo actions.
    Stores a reference to the FilterGraph.
*/
class FilterGraphAction : public juce::UndoableAction
{
  public:
    explicit FilterGraphAction(FilterGraph& graph) : filterGraph(graph) {}

  protected:
    FilterGraph& filterGraph;
};

//==============================================================================
/**
    Undoable action for adding a plugin.
    Undo removes the plugin.
*/
class AddPluginAction : public FilterGraphAction
{
  public:
    AddPluginAction(FilterGraph& graph, const juce::PluginDescription& desc, double xPos, double yPos)
        : FilterGraphAction(graph), pluginDescription(desc), x(xPos), y(yPos),
          nodeId(juce::AudioProcessorGraph::NodeID())
    {
    }

    bool perform() override;
    bool undo() override;

    int getSizeInUnits() override { return 10; }

    juce::String getName() const { return "Add Plugin: " + pluginDescription.name; }

    // Get the NodeID after perform() - needed for tracking
    juce::AudioProcessorGraph::NodeID getNodeId() const { return nodeId; }

  private:
    juce::PluginDescription pluginDescription;
    double x, y;
    juce::AudioProcessorGraph::NodeID nodeId;
};

//==============================================================================
/**
    Undoable action for removing a plugin.
    Undo recreates the plugin and restores connections.
*/
class RemovePluginAction : public FilterGraphAction
{
  public:
    RemovePluginAction(FilterGraph& graph, juce::AudioProcessorGraph::NodeID id, const juce::PluginDescription& desc,
                       double xPos, double yPos, const std::vector<juce::AudioProcessorGraph::Connection>& conns)
        : FilterGraphAction(graph), nodeId(id), pluginDescription(desc), x(xPos), y(yPos), connections(conns)
    {
    }

    bool perform() override;
    bool undo() override;

    int getSizeInUnits() override { return 20; }

    juce::String getName() const { return "Remove Plugin: " + pluginDescription.name; }

  private:
    juce::AudioProcessorGraph::NodeID nodeId;
    juce::PluginDescription pluginDescription;
    double x, y;
    std::vector<juce::AudioProcessorGraph::Connection> connections;
};

//==============================================================================
/**
    Undoable action for adding a connection.
    Undo removes the connection.
*/
class AddConnectionAction : public FilterGraphAction
{
  public:
    AddConnectionAction(FilterGraph& graph, juce::AudioProcessorGraph::NodeID srcNode, int srcChannel,
                        juce::AudioProcessorGraph::NodeID destNode, int destChannel)
        : FilterGraphAction(graph), sourceNode(srcNode), sourceChannel(srcChannel), destNode(destNode),
          destChannel(destChannel)
    {
    }

    bool perform() override;
    bool undo() override;

    int getSizeInUnits() override { return 5; }

    juce::String getName() const { return "Add Connection"; }

  private:
    juce::AudioProcessorGraph::NodeID sourceNode;
    int sourceChannel;
    juce::AudioProcessorGraph::NodeID destNode;
    int destChannel;
};

//==============================================================================
/**
    Undoable action for removing a connection.
    Undo adds the connection back.
*/
class RemoveConnectionAction : public FilterGraphAction
{
  public:
    RemoveConnectionAction(FilterGraph& graph, juce::AudioProcessorGraph::NodeID srcNode, int srcChannel,
                           juce::AudioProcessorGraph::NodeID destNode, int destChannel)
        : FilterGraphAction(graph), sourceNode(srcNode), sourceChannel(srcChannel), destNode(destNode),
          destChannel(destChannel)
    {
    }

    bool perform() override;
    bool undo() override;

    int getSizeInUnits() override { return 5; }

    juce::String getName() const { return "Remove Connection"; }

  private:
    juce::AudioProcessorGraph::NodeID sourceNode;
    int sourceChannel;
    juce::AudioProcessorGraph::NodeID destNode;
    int destChannel;
};
