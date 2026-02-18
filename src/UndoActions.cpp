/*
  ==============================================================================

    UndoActions.cpp
    Part of Pedalboard3

    Implementation of undoable actions for plugin and connection operations.

  ==============================================================================
*/

#include "UndoActions.h"

#include "FilterGraph.h"

#include <spdlog/spdlog.h>

//==============================================================================
// AddPluginAction
//==============================================================================

bool AddPluginAction::perform()
{
    spdlog::debug("[AddPluginAction::perform] About to call addFilterRaw for: {}",
                  pluginDescription.name.toStdString());
    spdlog::default_logger()->flush();

    // Add the plugin using the raw (non-undoable) method
    nodeId = filterGraph.addFilterRaw(&pluginDescription, x, y);

    spdlog::debug("[AddPluginAction::perform] addFilterRaw returned nodeId: {}", nodeId.uid);
    spdlog::default_logger()->flush();

    bool result = nodeId != juce::AudioProcessorGraph::NodeID();

    spdlog::debug("[AddPluginAction::perform] Returning: {}", result);
    spdlog::default_logger()->flush();

    return result;
}

bool AddPluginAction::undo()
{
    if (nodeId != juce::AudioProcessorGraph::NodeID())
    {
        filterGraph.removeFilterRaw(nodeId);
        return true;
    }
    return false;
}

//==============================================================================
// RemovePluginAction
//==============================================================================

bool RemovePluginAction::perform()
{
    filterGraph.removeFilterRaw(nodeId);
    return true;
}

bool RemovePluginAction::undo()
{
    // Recreate the plugin
    auto newId = filterGraph.addFilterRaw(&pluginDescription, x, y);

    if (newId != juce::AudioProcessorGraph::NodeID())
    {
        // Note: The node ID may be different after recreation.
        // Update our stored nodeId to the new one for future operations.
        nodeId = newId;

        // Restore all connections that involved this node
        for (const auto& conn : connections)
        {
            // Update connection references to use new node ID
            auto srcNode = (conn.source.nodeID == nodeId) ? newId : conn.source.nodeID;
            auto destNode = (conn.destination.nodeID == nodeId) ? newId : conn.destination.nodeID;

            filterGraph.addConnectionRaw(srcNode, conn.source.channelIndex, destNode, conn.destination.channelIndex);
        }
        return true;
    }
    return false;
}

//==============================================================================
// AddConnectionAction
//==============================================================================

bool AddConnectionAction::perform()
{
    return filterGraph.addConnectionRaw(sourceNode, sourceChannel, destNode, destChannel);
}

bool AddConnectionAction::undo()
{
    filterGraph.removeConnectionRaw(sourceNode, sourceChannel, destNode, destChannel);
    return true;
}

//==============================================================================
// RemoveConnectionAction
//==============================================================================

bool RemoveConnectionAction::perform()
{
    filterGraph.removeConnectionRaw(sourceNode, sourceChannel, destNode, destChannel);
    return true;
}

bool RemoveConnectionAction::undo()
{
    return filterGraph.addConnectionRaw(sourceNode, sourceChannel, destNode, destChannel);
}
