/*
  ==============================================================================

    SubGraphFilterGraph.cpp

    Implementation of FilterGraph adapter for SubGraphProcessor.

  ==============================================================================
*/

#include "SubGraphFilterGraph.h"

#include "AudioSingletons.h"
#include "BypassableInstance.h"
#include "PluginBlacklist.h"
#include "SubGraphProcessor.h"

#include <spdlog/spdlog.h>

using namespace juce;

//==============================================================================
SubGraphFilterGraph::SubGraphFilterGraph(SubGraphProcessor& owner) : processor(owner) {}

SubGraphFilterGraph::~SubGraphFilterGraph() = default;

AudioProcessorGraph& SubGraphFilterGraph::getGraph()
{
    return processor.getInternalGraph();
}

//==============================================================================
int SubGraphFilterGraph::getNumFilters() const
{
    return processor.getInternalGraph().getNumNodes();
}

AudioProcessorGraph::Node::Ptr SubGraphFilterGraph::getNode(int index) const
{
    return processor.getInternalGraph().getNode(index);
}

AudioProcessorGraph::Node::Ptr SubGraphFilterGraph::getNodeForId(AudioProcessorGraph::NodeID uid) const
{
    return processor.getInternalGraph().getNodeForId(uid);
}

//==============================================================================
void SubGraphFilterGraph::addFilter(const PluginDescription* desc, double x, double y)
{
    if (desc != nullptr)
    {
        undoManager.beginNewTransaction();
        // For now, directly add - can add UndoableAction later if needed
        addFilterRaw(desc, x, y);
    }
}

void SubGraphFilterGraph::removeFilter(AudioProcessorGraph::NodeID id)
{
    if (isFixedIONode(id))
        return; // Don't remove rack I/O nodes

    undoManager.beginNewTransaction();
    removeFilterRaw(id);
}

AudioProcessorGraph::NodeID SubGraphFilterGraph::addFilterRaw(const PluginDescription* desc, double x, double y)
{
    if (desc == nullptr)
        return AudioProcessorGraph::NodeID();

    // Check if plugin is blacklisted
    auto& blacklist = PluginBlacklist::getInstance();
    if (blacklist.isBlacklisted(desc->fileOrIdentifier) || blacklist.isBlacklistedById(desc->createIdentifierString()))
    {
        spdlog::warn("[SubGraphFilterGraph::addFilterRaw] Plugin is blacklisted: {} ({})", desc->name.toStdString(),
                     desc->fileOrIdentifier.toStdString());
        return AudioProcessorGraph::NodeID();
    }

    String errorMessage;
    auto tempInstance =
        AudioPluginFormatManagerSingleton::getInstance().createPluginInstance(*desc, 44100.0, 512, errorMessage);

    if (!tempInstance)
    {
        spdlog::error("[SubGraphFilterGraph::addFilterRaw] Failed to create: {}", errorMessage.toStdString());
        return AudioProcessorGraph::NodeID();
    }

    // Set stereo layout if supported
    AudioProcessor::BusesLayout stereoLayout;
    stereoLayout.inputBuses.add(AudioChannelSet::stereo());
    stereoLayout.outputBuses.add(AudioChannelSet::stereo());
    if (tempInstance->checkBusesLayoutSupported(stereoLayout))
        tempInstance->setBusesLayout(stereoLayout);

    // Don't wrap internal I/O processors or SubGraphProcessor itself
    std::unique_ptr<AudioProcessor> instance;
    if (dynamic_cast<AudioProcessorGraph::AudioGraphIOProcessor*>(tempInstance.get()) ||
        dynamic_cast<SubGraphProcessor*>(tempInstance.get()))
    {
        instance = std::move(tempInstance);
    }
    else
    {
        // Wrap in BypassableInstance for bypass support
        instance = std::make_unique<BypassableInstance>(tempInstance.release());
    }

    AudioProcessorGraph::Node::Ptr node;
    if (instance)
    {
        auto& graph = processor.getInternalGraph();
        const ScopedLock sl(graph.getCallbackLock());
        node = graph.addNode(std::move(instance));
    }

    if (node != nullptr)
    {
        node->properties.set("x", x);
        node->properties.set("y", y);
        changed();

        return node->nodeID;
    }

    spdlog::error("[SubGraphFilterGraph::addFilterRaw] Failed to add to graph");
    return AudioProcessorGraph::NodeID();
}

void SubGraphFilterGraph::removeFilterRaw(AudioProcessorGraph::NodeID id)
{
    if (processor.getInternalGraph().removeNode(id))
        changed();
}

//==============================================================================
bool SubGraphFilterGraph::addConnection(AudioProcessorGraph::NodeID sourceId, int sourceChannel,
                                        AudioProcessorGraph::NodeID destId, int destChannel)
{
    undoManager.beginNewTransaction();
    return addConnectionRaw(sourceId, sourceChannel, destId, destChannel);
}

void SubGraphFilterGraph::removeConnection(AudioProcessorGraph::NodeID sourceId, int sourceChannel,
                                           AudioProcessorGraph::NodeID destId, int destChannel)
{
    undoManager.beginNewTransaction();
    removeConnectionRaw(sourceId, sourceChannel, destId, destChannel);
}

bool SubGraphFilterGraph::addConnectionRaw(AudioProcessorGraph::NodeID sourceId, int sourceChannel,
                                           AudioProcessorGraph::NodeID destId, int destChannel)
{
    auto& graph = processor.getInternalGraph();
    AudioProcessorGraph::Connection conn{{sourceId, sourceChannel}, {destId, destChannel}};

    if (graph.addConnection(conn))
    {
        changed();
        return true;
    }
    return false;
}

void SubGraphFilterGraph::removeConnectionRaw(AudioProcessorGraph::NodeID sourceId, int sourceChannel,
                                              AudioProcessorGraph::NodeID destId, int destChannel)
{
    auto& graph = processor.getInternalGraph();
    AudioProcessorGraph::Connection conn{{sourceId, sourceChannel}, {destId, destChannel}};

    if (graph.removeConnection(conn))
        changed();
}

std::vector<AudioProcessorGraph::Connection> SubGraphFilterGraph::getConnections() const
{
    return processor.getInternalGraph().getConnections();
}

void SubGraphFilterGraph::disconnectFilter(AudioProcessorGraph::NodeID id)
{
    auto& graph = processor.getInternalGraph();
    for (auto& conn : graph.getConnections())
    {
        if (conn.source.nodeID == id || conn.destination.nodeID == id)
            graph.removeConnection(conn);
    }
    changed();
}

const AudioProcessorGraph::Connection* SubGraphFilterGraph::getConnectionBetween(AudioProcessorGraph::NodeID sourceId,
                                                                                 int sourceChannel,
                                                                                 AudioProcessorGraph::NodeID destId,
                                                                                 int destChannel) const
{
    // JUCE 8 removed getConnectionBetween, so we iterate manually
    auto& graph = processor.getInternalGraph();
    for (auto& conn : graph.getConnections())
    {
        if (conn.source.nodeID == sourceId && conn.source.channelIndex == sourceChannel &&
            conn.destination.nodeID == destId && conn.destination.channelIndex == destChannel)
        {
            return &conn;
        }
    }
    return nullptr;
}

//==============================================================================
void SubGraphFilterGraph::setNodePosition(int nodeId, double x, double y)
{
    auto node = processor.getInternalGraph().getNodeForId(AudioProcessorGraph::NodeID(nodeId));
    if (node != nullptr)
    {
        node->properties.set("x", x);
        node->properties.set("y", y);
    }
}

void SubGraphFilterGraph::getNodePosition(int nodeId, double& x, double& y) const
{
    x = y = 0;
    auto node = processor.getInternalGraph().getNodeForId(AudioProcessorGraph::NodeID(nodeId));
    if (node != nullptr)
    {
        x = node->properties.getWithDefault("x", 0.0);
        y = node->properties.getWithDefault("y", 0.0);
    }
}

//==============================================================================
void SubGraphFilterGraph::changed()
{
    // Notify the SubGraphProcessor that its internal state changed
    // This triggers proper re-save of processor state
    processor.getInternalGraph().sendChangeMessage();
}

bool SubGraphFilterGraph::isFixedIONode(AudioProcessorGraph::NodeID nodeId) const
{
    return (nodeId == processor.getRackAudioInputNodeId() || nodeId == processor.getRackAudioOutputNodeId() ||
            nodeId == processor.getRackMidiInputNodeId());
}

bool SubGraphFilterGraph::isHiddenInfrastructureNode(AudioProcessorGraph::NodeID nodeId) const
{
    // For subgraphs, the I/O nodes are visible infrastructure, not hidden
    // We return false for all - subgraphs don't have hidden infrastructure like SafetyLimiter
    return false;
}
