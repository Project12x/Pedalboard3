/*
  ==============================================================================

   This file is part of the JUCE library - "Jules' Utility Class Extensions"
   Copyright 2004-7 by Raw Material Software ltd.

  ------------------------------------------------------------------------------

   JUCE can be redistributed and/or modified under the terms of the
   GNU General Public License, as published by the Free Software Foundation;
   either version 2 of the License, or (at your option) any later version.

   JUCE is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with JUCE; if not, visit www.gnu.org/licenses or write to the
   Free Software Foundation, Inc., 59 Temple Place, Suite 330,
   Boston, MA 02111-1307 USA

  ------------------------------------------------------------------------------

   If you'd like to release a closed-source product which uses JUCE, commercial
   licenses are also available: visit www.rawmaterialsoftware.com/juce for
   more information.

  ==============================================================================
*/

#include "FilterGraph.h"

#include "AudioSingletons.h"
#include "BypassableInstance.h"
#include "InternalFilters.h"
#include "MidiMappingManager.h"
#include "OscMappingManager.h"
#include "PedalboardProcessors.h"
#include "PluginBlacklist.h"
#include "SettingsManager.h"
#include "SubGraphProcessor.h"
#include "UndoActions.h"
#include "VirtualMidiInputProcessor.h"

#include <chrono>
#include <iostream>
#include <spdlog/spdlog.h>
#include <thread>

using namespace std;

//==============================================================================
FilterConnection::FilterConnection(FilterGraph& owner_) : owner(owner_) {}

FilterConnection::FilterConnection(const FilterConnection& other)
    : sourceFilterID(other.sourceFilterID), sourceChannel(other.sourceChannel), destFilterID(other.destFilterID),
      destChannel(other.destChannel), owner(other.owner)
{
}

FilterConnection::~FilterConnection() {}

//==============================================================================
const int FilterGraph::midiChannelNumber = 0x1000;

void FilterGraph::createInfrastructureNodes()
{
    safetyLimiter = nullptr;
    crossfadeMixer = nullptr;
    safetyLimiterNodeId = {};
    crossfadeMixerNodeId = {};

    auto limiterProcessor = std::make_unique<SafetyLimiterProcessor>();
    safetyLimiter = limiterProcessor.get();
    auto safetyNode = graph.addNode(std::move(limiterProcessor), AudioProcessorGraph::NodeID(0xFFFFFF));
    if (safetyNode)
    {
        safetyLimiterNodeId = safetyNode->nodeID;
        safetyNode->properties.set("x", -100.0);
        safetyNode->properties.set("y", -100.0);
    }

    auto crossfadeProcessor = std::make_unique<CrossfadeMixerProcessor>();
    crossfadeMixer = crossfadeProcessor.get();
    auto crossfadeNode = graph.addNode(std::move(crossfadeProcessor), AudioProcessorGraph::NodeID(0xFFFFFE));
    if (crossfadeNode)
    {
        crossfadeMixerNodeId = crossfadeNode->nodeID;
        crossfadeNode->properties.set("x", -100.0);
        crossfadeNode->properties.set("y", -150.0);
    }
}

FilterGraph::FilterGraph()
    : FileBasedDocument(filenameSuffix, filenameWildcard, "Load a filter graph", "Save a filter graph"), lastUID(0)
{
    InternalPluginFormat internalFormat;
    bool audioInput = SettingsManager::getInstance().getBool("AudioInput", true);
    bool midiInput = SettingsManager::getInstance().getBool("MidiInput", true);

    // Add default nodes at standard positions
    if (audioInput)
        addFilterRaw(internalFormat.getDescriptionFor(InternalPluginFormat::audioInputFilter), 540.0f, 500.0f);

    if (midiInput)
        addFilterRaw(internalFormat.getDescriptionFor(InternalPluginFormat::midiInputFilter), 540.0f, 760.0f);

    // Virtual MIDI Input (for on-screen keyboard) - always added
    addFilterRaw(internalFormat.getDescriptionFor(InternalPluginFormat::virtualMidiInputProcFilter), 540.0f, 660.0f);

    // Use Raw method to avoid adding to undo history
    addFilterRaw(internalFormat.getDescriptionFor(InternalPluginFormat::audioOutputFilter), 1320.0f, 500.0f);

    createInfrastructureNodes();

    setChangedFlag(false);
}

FilterGraph::~FilterGraph()
{
    graph.clear();
}

void FilterGraph::setDeviceChannelCounts(int numInputs, int numOutputs)
{
    spdlog::info("[FilterGraph] Setting device channel counts: {} inputs, {} outputs", numInputs, numOutputs);

    // Configure the graph's bus layout to match the device channels
    AudioProcessor::BusesLayout layout;

    if (numInputs > 0)
        layout.inputBuses.add(AudioChannelSet::discreteChannels(numInputs));
    if (numOutputs > 0)
        layout.outputBuses.add(AudioChannelSet::discreteChannels(numOutputs));

    if (graph.setBusesLayout(layout))
    {
        spdlog::info("[FilterGraph] Graph bus layout set successfully: {} in, {} out", graph.getTotalNumInputChannels(),
                     graph.getTotalNumOutputChannels());
    }
    else
    {
        spdlog::warn("[FilterGraph] Failed to set graph bus layout");
    }
}

void FilterGraph::repositionDefaultInputNodes()
{
    // Default node layout — standard arrangement
    constexpr float nodeX = 540.0f;

    for (int i = 0; i < getNumFilters(); ++i)
    {
        auto node = getNode(i);
        if (!node)
            continue;
        auto name = node->getProcessor()->getName();

        if (name == "Audio Input")
        {
            node->properties.set("x", nodeX);
            node->properties.set("y", 500.0f);
        }
        else if (name == "Virtual MIDI Input")
        {
            node->properties.set("x", nodeX);
            node->properties.set("y", 660.0f);
        }
        else if (name == "MIDI Input")
        {
            node->properties.set("x", nodeX);
            node->properties.set("y", 760.0f);
        }
        else if (name == "Audio Output")
        {
            node->properties.set("x", 1320.0f);
            node->properties.set("y", 500.0f);
        }
        else if (name == "OSC Input")
        {
            node->properties.set("x", nodeX);
            node->properties.set("y", 860.0f);
        }
    }
}

float FilterGraph::getNextInputNodeY() const
{
    // Find the Virtual MIDI Input node and return position after it
    // This is used by PluginField to position OSC Input
    constexpr float gap = 20.0f;

    for (int i = 0; i < getNumFilters(); ++i)
    {
        auto node = getNode(i);
        if (node && node->getProcessor()->getName() == "Virtual MIDI Input")
        {
            float y = static_cast<float>(node->properties.getWithDefault("y", 100.0));

            // Virtual MIDI Input: getSize().y (40) + header (52) = 92px
            auto* proc = dynamic_cast<PedalboardProcessor*>(node->getProcessor());
            float height = proc ? (proc->getSize().y + 52.0f) : 92.0f;

            return y + height + gap;
        }
    }

    // Fallback if Virtual MIDI Input not found
    return 300.0f;
}

uint32 FilterGraph::getNextUID() throw()
{
    return ++lastUID;
}

//==============================================================================
int FilterGraph::getNumFilters() const
{
    return graph.getNumNodes();
}

AudioProcessorGraph::Node::Ptr FilterGraph::getNode(int index) const
{
    return graph.getNode(index);
}

AudioProcessorGraph::Node::Ptr FilterGraph::getNodeForId(AudioProcessorGraph::NodeID uid) const
{
    return graph.getNodeForId(uid);
}

void FilterGraph::addFilter(const PluginDescription* desc, double x, double y)
{
    if (desc != nullptr)
    {
        spdlog::debug("[FilterGraph::addFilter] About to call undoManager.perform for: {}", desc->name.toStdString());
        spdlog::default_logger()->flush();

        undoManager.beginNewTransaction();
        undoManager.perform(new AddPluginAction(*this, *desc, x, y));

        spdlog::debug("[FilterGraph::addFilter] undoManager.perform returned successfully");
        spdlog::default_logger()->flush();
    }
}

void FilterGraph::addFilter(AudioPluginInstance* plugin, double x, double y)
{
    if (plugin != 0)
    {
        String errorMessage;

        // Log bus state BEFORE enableAllBuses
        spdlog::debug(
            "[addFilter] Plugin '{}' BEFORE enableAllBuses: inputBuses={}, outputBuses={}, totalIn={}, totalOut={}",
            plugin->getName().toStdString(), plugin->getBusCount(true), plugin->getBusCount(false),
            plugin->getTotalNumInputChannels(), plugin->getTotalNumOutputChannels());

        // VST3 instruments may have disabled output buses by default (confirmed by Carla source).
        // Enable all buses before wrapping to ensure output pins are visible.
        plugin->enableAllBuses();

        // Log bus state AFTER enableAllBuses
        spdlog::debug(
            "[addFilter] Plugin '{}' AFTER enableAllBuses: inputBuses={}, outputBuses={}, totalIn={}, totalOut={}",
            plugin->getName().toStdString(), plugin->getBusCount(true), plugin->getBusCount(false),
            plugin->getTotalNumInputChannels(), plugin->getTotalNumOutputChannels());
        spdlog::default_logger()->flush();

        // JUCE 8: addNode takes unique_ptr
        auto instance = std::make_unique<BypassableInstance>(plugin);

        AudioProcessorGraph::Node::Ptr node;

        if (instance)
            node = graph.addNode(std::move(instance));

        if (node != nullptr)
        {
            node->properties.set("x", x);
            node->properties.set("y", y);
            changed();
        }
        else
        {
            AlertWindow::showMessageBox(AlertWindow::WarningIcon, TRANS("Couldn't create filter"), errorMessage);
        }
    }
}

void FilterGraph::removeFilter(const AudioProcessorGraph::NodeID id)
{
    // Get plugin description and connections before removing, for undo support
    PluginDescription desc = getPluginDescription(id);
    auto node = graph.getNodeForId(id);
    if (node != nullptr)
    {
        double x = node->properties.getWithDefault("x", 0.0);
        double y = node->properties.getWithDefault("y", 0.0);
        auto connections = getConnectionsForNode(id);
        undoManager.beginNewTransaction();
        undoManager.perform(new RemovePluginAction(*this, id, desc, x, y, connections));
    }
}

void FilterGraph::disconnectFilter(const AudioProcessorGraph::NodeID id)
{
    // JUCE 8: disconnectNode takes NodeID
    if (graph.disconnectNode(id))
        changed();
}

void FilterGraph::removeIllegalConnections()
{
    if (graph.removeIllegalConnections())
        changed();
}

void FilterGraph::setNodePosition(const int nodeId, double x, double y)
{
    // JUCE 8: getNodeForId takes NodeID
    const AudioProcessorGraph::Node::Ptr n(graph.getNodeForId(AudioProcessorGraph::NodeID(nodeId)));

    if (n != nullptr)
    {
        n->properties.set("x", jlimit(0.0, 1.0, x));
        n->properties.set("y", jlimit(0.0, 1.0, y));
    }
}

void FilterGraph::getNodePosition(const int nodeId, double& x, double& y) const
{
    x = y = 0;

    // JUCE 8: getNodeForId takes NodeID
    const AudioProcessorGraph::Node::Ptr n(graph.getNodeForId(AudioProcessorGraph::NodeID(nodeId)));

    if (n != nullptr)
    {
        // x = n->properties.getDoubleValue ("x");
        x = n->properties.getWithDefault("x", 0.0);
        // y = n->properties.getDoubleValue ("y");
        y = n->properties.getWithDefault("y", 0.0);
    }
}

//==============================================================================
// JUCE 8: Return connections as vector
std::vector<AudioProcessorGraph::Connection> FilterGraph::getConnections() const
{
    return graph.getConnections();
}

// JUCE 8: Implementation of getConnectionBetween - checks if a connection exists
bool FilterGraph::getConnectionBetween(AudioProcessorGraph::NodeID sourceFilterUID, int sourceFilterChannel,
                                       AudioProcessorGraph::NodeID destFilterUID, int destFilterChannel) const
{
    for (const auto& conn : graph.getConnections())
    {
        if (conn.source.nodeID == sourceFilterUID && conn.source.channelIndex == sourceFilterChannel &&
            conn.destination.nodeID == destFilterUID && conn.destination.channelIndex == destFilterChannel)
        {
            return true;
        }
    }
    return false;
}

bool FilterGraph::canConnect(AudioProcessorGraph::NodeID sourceFilterUID, int sourceFilterChannel,
                             AudioProcessorGraph::NodeID destFilterUID, int destFilterChannel) const
{
    // JUCE 8: canConnect takes a Connection struct
    AudioProcessorGraph::Connection conn{{sourceFilterUID, sourceFilterChannel}, {destFilterUID, destFilterChannel}};
    return graph.canConnect(conn);
}

bool FilterGraph::addConnection(AudioProcessorGraph::NodeID sourceFilterUID, int sourceFilterChannel,
                                AudioProcessorGraph::NodeID destFilterUID, int destFilterChannel)
{
    undoManager.beginNewTransaction();
    undoManager.perform(
        new AddConnectionAction(*this, sourceFilterUID, sourceFilterChannel, destFilterUID, destFilterChannel));
    // Check if connection exists now
    AudioProcessorGraph::Connection conn{{sourceFilterUID, sourceFilterChannel}, {destFilterUID, destFilterChannel}};
    return graph.isConnected(conn);
}

void FilterGraph::removeConnection(AudioProcessorGraph::NodeID sourceFilterUID, int sourceFilterChannel,
                                   AudioProcessorGraph::NodeID destFilterUID, int destFilterChannel)
{
    undoManager.beginNewTransaction();
    undoManager.perform(
        new RemoveConnectionAction(*this, sourceFilterUID, sourceFilterChannel, destFilterUID, destFilterChannel));
}

//==============================================================================
// Raw operations - used by UndoableActions (no undo tracking)
//==============================================================================

AudioProcessorGraph::NodeID FilterGraph::addFilterRaw(const PluginDescription* desc, double x, double y)
{
    if (desc == nullptr)
        return AudioProcessorGraph::NodeID();

    // Check if plugin is blacklisted
    auto& blacklist = PluginBlacklist::getInstance();
    if (blacklist.isBlacklisted(desc->fileOrIdentifier) || blacklist.isBlacklistedById(desc->createIdentifierString()))
    {
        spdlog::warn("[addFilterRaw] Plugin is blacklisted: {} ({})", desc->name.toStdString(),
                     desc->fileOrIdentifier.toStdString());
        return AudioProcessorGraph::NodeID();
    }

    spdlog::debug("[addFilterRaw] Adding plugin: {}", desc->name.toStdString());

    String errorMessage;
    auto tempInstance =
        AudioPluginFormatManagerSingleton::getInstance().createPluginInstance(*desc, 44100.0, 512, errorMessage);

    if (!tempInstance)
    {
        spdlog::error("[addFilterRaw] createPluginInstance failed: {}", errorMessage.toStdString());
        return AudioProcessorGraph::NodeID();
    }

    // Try stereo layout
    AudioProcessor::BusesLayout stereoLayout;
    stereoLayout.inputBuses.add(AudioChannelSet::stereo());
    stereoLayout.outputBuses.add(AudioChannelSet::stereo());

    if (tempInstance->checkBusesLayoutSupported(stereoLayout))
        tempInstance->setBusesLayout(stereoLayout);

    // Wrap external plugins in BypassableInstance; internal processors go directly
    std::unique_ptr<AudioProcessor> instance;
    if (dynamic_cast<AudioProcessorGraph::AudioGraphIOProcessor*>(tempInstance.get()) ||
        dynamic_cast<MidiInterceptor*>(tempInstance.get()) || dynamic_cast<OscInput*>(tempInstance.get()) ||
        dynamic_cast<SubGraphProcessor*>(tempInstance.get()) ||
        dynamic_cast<VirtualMidiInputProcessor*>(tempInstance.get()))
    {
        instance = std::move(tempInstance);
    }
    else
    {
        instance = std::make_unique<BypassableInstance>(tempInstance.release());
    }

    // Lock the audio callback to prevent race with audio thread
    AudioProcessorGraph::Node::Ptr node;
    {
        const juce::ScopedLock sl(graph.getCallbackLock());
        node = graph.addNode(std::move(instance));
    }

    if (node != nullptr)
    {
        node->properties.set("x", x);
        node->properties.set("y", y);

        // Notify listeners that graph changed - creates UI components
        try
        {
            changed();
        }
        catch (const std::exception& e)
        {
            spdlog::error("[addFilterRaw] Exception in changed(): {}", e.what());
        }
        catch (...)
        {
            spdlog::error("[addFilterRaw] Unknown exception in changed()");
        }

        auto nodeId = node->nodeID;
        spdlog::debug("[addFilterRaw] Added node ID={}", nodeId.uid);
        return nodeId;
    }

    spdlog::error("[addFilterRaw] Failed to add plugin to graph");
    return AudioProcessorGraph::NodeID();
}

void FilterGraph::removeFilterRaw(const AudioProcessorGraph::NodeID id)
{
    if (graph.removeNode(id))
        changed();
}

bool FilterGraph::addConnectionRaw(AudioProcessorGraph::NodeID sourceFilterUID, int sourceFilterChannel,
                                   AudioProcessorGraph::NodeID destFilterUID, int destFilterChannel)
{
    AudioProcessorGraph::Connection conn{{sourceFilterUID, sourceFilterChannel}, {destFilterUID, destFilterChannel}};
    const bool result = graph.addConnection(conn);

    // DEBUG: Log connection attempts with channel info when failing
    if (!result)
    {
        auto srcNode = graph.getNodeForId(sourceFilterUID);
        auto dstNode = graph.getNodeForId(destFilterUID);
        spdlog::warn("[addConnectionRaw] FAILED {}:{} -> {}:{} | src({} out={}) dst({} in={}) canConnect={}",
                     (int)sourceFilterUID.uid, sourceFilterChannel, (int)destFilterUID.uid, destFilterChannel,
                     srcNode ? srcNode->getProcessor()->getName().toStdString() : "NULL",
                     srcNode ? srcNode->getProcessor()->getTotalNumOutputChannels() : -1,
                     dstNode ? dstNode->getProcessor()->getName().toStdString() : "NULL",
                     dstNode ? dstNode->getProcessor()->getTotalNumInputChannels() : -1, graph.canConnect(conn));
    }
    else
    {
        spdlog::info("[addConnectionRaw] OK {}:{} -> {}:{}", (int)sourceFilterUID.uid, sourceFilterChannel,
                     (int)destFilterUID.uid, destFilterChannel);
    }

    if (result)
        changed();
    return result;
}

void FilterGraph::removeConnectionRaw(AudioProcessorGraph::NodeID sourceFilterUID, int sourceFilterChannel,
                                      AudioProcessorGraph::NodeID destFilterUID, int destFilterChannel)
{
    AudioProcessorGraph::Connection conn{{sourceFilterUID, sourceFilterChannel}, {destFilterUID, destFilterChannel}};
    if (graph.removeConnection(conn))
        changed();
}

//==============================================================================
// Helper functions for undo
//==============================================================================

PluginDescription FilterGraph::getPluginDescription(AudioProcessorGraph::NodeID nodeId) const
{
    PluginDescription desc;
    auto node = graph.getNodeForId(nodeId);
    if (node != nullptr && node->getProcessor() != nullptr)
    {
        // Try to get the inner plugin from BypassableInstance
        auto* bypassable = dynamic_cast<BypassableInstance*>(node->getProcessor());
        if (bypassable)
        {
            // BypassableInstance has fillInPluginDescription that delegates to inner plugin
            bypassable->fillInPluginDescription(desc);
        }
        else
        {
            // For AudioPluginInstance (which has fillInPluginDescription)
            auto* pluginInstance = dynamic_cast<AudioPluginInstance*>(node->getProcessor());
            if (pluginInstance)
            {
                pluginInstance->fillInPluginDescription(desc);
            }
        }
    }
    return desc;
}

std::vector<AudioProcessorGraph::Connection>
FilterGraph::getConnectionsForNode(AudioProcessorGraph::NodeID nodeId) const
{
    std::vector<AudioProcessorGraph::Connection> nodeConnections;
    auto allConnections = getConnections();

    for (const auto& conn : allConnections)
    {
        if (conn.source.nodeID == nodeId || conn.destination.nodeID == nodeId)
        {
            nodeConnections.push_back(conn);
        }
    }

    return nodeConnections;
}

void FilterGraph::clear(bool addAudioIn, bool addMidiIn, bool addAudioOut, bool addVirtualMidiIn)
{
    InternalPluginFormat internalFormat;

    // PluginWindow::closeAllCurrentlyOpenWindows();

    graph.clear();
    createInfrastructureNodes();

    // Add nodes with temporary Y positions (will be repositioned below)
    if (addAudioIn)
    {
        addFilter(internalFormat.getDescriptionFor(InternalPluginFormat::audioInputFilter), 540.0f, 500.0f);
    }

    if (addMidiIn)
    {
        addFilter(internalFormat.getDescriptionFor(InternalPluginFormat::midiInputFilter), 540.0f, 760.0f);
    }

    // Virtual MIDI Input (for on-screen keyboard)
    if (addVirtualMidiIn)
    {
        addFilter(internalFormat.getDescriptionFor(InternalPluginFormat::virtualMidiInputProcFilter), 540.0f, 660.0f);
    }

    if (addAudioOut)
    {
        addFilter(internalFormat.getDescriptionFor(InternalPluginFormat::audioOutputFilter), 1320.0f, 500.0f);
    }

    changed();
}

//==============================================================================
String FilterGraph::getDocumentTitle()
{
    if (!getFile().exists())
        return "Unnamed";

    return getFile().getFileNameWithoutExtension();
}

Result FilterGraph::loadDocument(const File& file)
{
    /*XmlDocument doc (file);
    XmlElement* xml = doc.getDocumentElement();

    if (xml == 0 || ! xml->hasTagName ("FILTERGRAPH"))
    {
        delete xml;
                return Result::fail("Not a valid filter graph file");
    }

    restoreFromXml (*xml);
    delete xml;*/

    jassert(false);
    return Result::ok();
}

Result FilterGraph::saveDocument(const File& file)
{
    /*XmlElement* xml = createXml();

    String error;

    if (! xml->writeToFile (file, String()))
                return Result::fail("Couldn't write to the file");

    delete xml;*/
    jassert(false); // Pretty sure this is never used, but just to be sure...
    return Result::ok();
}

File FilterGraph::getLastDocumentOpened()
{
    RecentlyOpenedFilesList recentFiles;
    recentFiles.restoreFromString(SettingsManager::getInstance().getString("recentFilterGraphFiles"));

    return recentFiles.getFile(0);
}

void FilterGraph::setLastDocumentOpened(const File& file)
{
    RecentlyOpenedFilesList recentFiles;
    recentFiles.restoreFromString(SettingsManager::getInstance().getString("recentFilterGraphFiles"));

    recentFiles.addFile(file);

    SettingsManager::getInstance().setValue("recentFilterGraphFiles", recentFiles.toString());
}

//==============================================================================
static XmlElement* createNodeXml(AudioProcessorGraph::Node::Ptr node, const OscMappingManager& oscManager)
{
    spdlog::debug("[createNodeXml] Processing node: {}", node->getProcessor()->getName().toStdString());
    spdlog::default_logger()->flush();

    // Check if this is a SubGraphProcessor (Effect Rack) - needs special handling
    // SubGraphProcessor is NOT an AudioPluginInstance, so we handle it separately
    SubGraphProcessor* subGraph = dynamic_cast<SubGraphProcessor*>(node->getProcessor());
    if (subGraph != nullptr)
    {
        spdlog::debug("[createNodeXml] Found SubGraphProcessor, creating XML");
        spdlog::default_logger()->flush();

        XmlElement* e = new XmlElement("FILTER");
        e->setAttribute("uid", (int)node->nodeID.uid);
        e->setAttribute("x", (double)node->properties.getWithDefault("x", 0.0));
        e->setAttribute("y", (double)node->properties.getWithDefault("y", 0.0));
        e->setAttribute("uiLastX", (int)node->properties.getWithDefault("uiLastX", 0.0));
        e->setAttribute("uiLastY", (int)node->properties.getWithDefault("uiLastY", 0.0));
        e->setAttribute("windowOpen", (bool)node->properties.getWithDefault("windowOpen", false));
        e->setAttribute("program", 0);

        // Create plugin description for SubGraphProcessor
        PluginDescription pd;
        pd.name = "Effect Rack"; // Must match InternalPluginFormat::subGraphProcDesc.name for restore
        pd.pluginFormatName = "Internal";
        pd.fileOrIdentifier = "Internal:SubGraph";
        pd.uniqueId = 0;
        pd.isInstrument = false;
        pd.numInputChannels = subGraph->getTotalNumInputChannels();
        pd.numOutputChannels = subGraph->getTotalNumOutputChannels();

        e->addChildElement(pd.createXml().release());

        // Save SubGraphProcessor state
        XmlElement* state = new XmlElement("STATE");
        MemoryBlock m;
        subGraph->getStateInformation(m);
        state->addTextElement(m.toBase64Encoding());
        e->addChildElement(state);

        spdlog::debug("[createNodeXml] SubGraphProcessor XML created successfully");
        spdlog::default_logger()->flush();
        return e;
    }

    AudioPluginInstance* plugin = dynamic_cast<AudioPluginInstance*>(node->getProcessor());
    BypassableInstance* bypassable = dynamic_cast<BypassableInstance*>(plugin);

    if (plugin == nullptr)
    {
        spdlog::error("[createNodeXml] ERROR: plugin is nullptr for: {}",
                      node->getProcessor()->getName().toStdString());
        spdlog::default_logger()->flush();
        jassertfalse;
        return nullptr;
    }

    XmlElement* e = new XmlElement("FILTER");
    // JUCE 8: NodeID needs .uid for integer conversion
    e->setAttribute("uid", (int)node->nodeID.uid);
    /*e->setAttribute("x", node->properties.getDoubleValue("x"));
    e->setAttribute("y", node->properties.getDoubleValue("y"));
    e->setAttribute("uiLastX", node->properties.getIntValue("uiLastX"));
    e->setAttribute("uiLastY", node->properties.getIntValue("uiLastY"));*/
    e->setAttribute("x", (double)node->properties.getWithDefault("x", 0.0));
    e->setAttribute("y", (double)node->properties.getWithDefault("y", 0.0));
    e->setAttribute("uiLastX", (int)node->properties.getWithDefault("uiLastX", 0.0));
    e->setAttribute("uiLastY", (int)node->properties.getWithDefault("uiLastY", 0.0));
    e->setAttribute("windowOpen", (bool)node->properties.getWithDefault("windowOpen", false));
    e->setAttribute("program", node->getProcessor()->getCurrentProgram());
    if (bypassable)
    {
        e->setAttribute("oscMIDIAddress", oscManager.getMIDIProcessorAddress(bypassable));
        e->setAttribute("MIDIChannel", bypassable->getMIDIChannel());
        e->setAttribute("bypass", bypassable->getBypass());
    }

    PluginDescription pd;

    plugin->fillInPluginDescription(pd);

    // JUCE 8: createXml returns unique_ptr, use .release()
    e->addChildElement(pd.createXml().release());

    XmlElement* state = new XmlElement("STATE");

    MemoryBlock m;
    node->getProcessor()->getStateInformation(m);
    state->addTextElement(m.toBase64Encoding());
    e->addChildElement(state);

    return e;
}

void FilterGraph::createNodeFromXml(const XmlElement& xml, OscMappingManager& oscManager)
{
    String midiAddress;
    String errorMessage;
    PluginDescription pd;
    AudioPluginInstance* instance = 0;
    BypassableInstance* bypassable = 0;
    std::unique_ptr<AudioPluginInstance> tempInstance;

    forEachXmlChildElement(xml, e)
    {
        if (pd.loadFromXml(*e))
            break;
    }

    int uid = xml.getIntAttribute("uid");
    spdlog::debug("[createNodeFromXml] Creating node uid={} plugin={}", uid, pd.name.toStdString());

    // JUCE 8: createPluginInstance (not createPluginInstanceSync)
    tempInstance =
        AudioPluginFormatManagerSingleton::getInstance().createPluginInstance(pd, 44100.0, 512, errorMessage);

    // VST3 instruments may have disabled output buses by default (confirmed by Carla source).
    // Enable all buses to ensure output pins are visible for synths.
    if (tempInstance)
    {
        tempInstance->enableAllBuses();
    }

    std::unique_ptr<AudioProcessor> instancePtr;
    if (tempInstance)
    {
        if (dynamic_cast<AudioProcessorGraph::AudioGraphIOProcessor*>(tempInstance.get()) ||
            dynamic_cast<MidiInterceptor*>(tempInstance.get()) || dynamic_cast<OscInput*>(tempInstance.get()) ||
            dynamic_cast<SubGraphProcessor*>(tempInstance.get()) ||
            dynamic_cast<VirtualMidiInputProcessor*>(tempInstance.get()))
            instancePtr = std::move(tempInstance);
        else
        {
            bypassable = new BypassableInstance(tempInstance.release());
            instancePtr.reset(bypassable);
        }
    }

    if (!instancePtr)
    {
        spdlog::error("[createNodeFromXml] FAILED to create plugin uid={} name={} error={}", uid, pd.name.toStdString(),
                      errorMessage.toStdString());
        return;
    }

    // JUCE 8: addNode takes unique_ptr and NodeID
    AudioProcessorGraph::Node::Ptr node(graph.addNode(std::move(instancePtr), AudioProcessorGraph::NodeID(uid)));

    if (!node)
    {
        spdlog::error("[createNodeFromXml] addNode returned null for uid={}", uid);
        return;
    }

    spdlog::debug("[createNodeFromXml] SUCCESS node uid={} actual_uid={}", uid, (int)node->nodeID.uid);

    const XmlElement* const state = xml.getChildByName("STATE");

    if (state != 0)
    {
        MemoryBlock m;
        m.fromBase64Encoding(state->getAllSubText());

        node->getProcessor()->setStateInformation(m.getData(), m.getSize());
    }

    node->properties.set("x", xml.getDoubleAttribute("x"));
    node->properties.set("y", xml.getDoubleAttribute("y"));
    node->properties.set("uiLastX", xml.getIntAttribute("uiLastX"));
    node->properties.set("uiLastY", xml.getIntAttribute("uiLastY"));
    node->properties.set("windowOpen", xml.getIntAttribute("windowOpen"));

    midiAddress = xml.getStringAttribute("oscMIDIAddress");
    if (bypassable)
    {
        if (!midiAddress.isEmpty())
            oscManager.registerMIDIProcessor(midiAddress, bypassable);

        bypassable->setMIDIChannel(xml.getIntAttribute("MIDIChannel"));
        bypassable->setBypass(xml.getBoolAttribute("bypass", false));
    }

    node->getProcessor()->setCurrentProgram(xml.getIntAttribute("program"));
}

XmlElement* FilterGraph::createXml(const OscMappingManager& oscManager) const
{
    XmlElement* xml = new XmlElement("FILTERGRAPH");

    int savedNodes = 0;
    for (int i = 0; i < graph.getNumNodes(); ++i)
    {
        auto node = graph.getNode(i);
        if (node == nullptr || isHiddenInfrastructureNode(node->nodeID))
            continue;

        if (auto* nodeXml = createNodeXml(node, oscManager))
        {
            xml->addChildElement(nodeXml);
            ++savedNodes;
        }
    }

    // JUCE 8: getConnections returns vector
    auto connections = graph.getConnections();
    int savedConnections = 0;
    for (const auto& fc : connections)
    {
        if (isHiddenInfrastructureNode(fc.source.nodeID) || isHiddenInfrastructureNode(fc.destination.nodeID))
            continue;

        XmlElement* e = new XmlElement("CONNECTION");

        e->setAttribute("srcFilter", (int)fc.source.nodeID.uid);
        e->setAttribute("srcChannel", fc.source.channelIndex);
        e->setAttribute("dstFilter", (int)fc.destination.nodeID.uid);
        e->setAttribute("dstChannel", fc.destination.channelIndex);

        xml->addChildElement(e);
        ++savedConnections;
    }

    spdlog::info("[FilterGraph::createXml] Saved {} nodes and {} connections", savedNodes, savedConnections);
    return xml;
}

void FilterGraph::restoreFromXml(const XmlElement& xml, OscMappingManager& oscManager)
{
    clear(false, false, false, false);

    int nodeCount = 0;
    forEachXmlChildElementWithTagName(xml, e, "FILTER")
    {
        createNodeFromXml(*e, oscManager);
        changed();
        nodeCount++;
    }

    int connectionCount = 0;
    forEachXmlChildElementWithTagName(xml, e2, "CONNECTION")
    {
        auto srcFilter = AudioProcessorGraph::NodeID((uint32)e2->getIntAttribute("srcFilter"));
        auto srcChannel = e2->getIntAttribute("srcChannel");
        auto dstFilter = AudioProcessorGraph::NodeID((uint32)e2->getIntAttribute("dstFilter"));
        auto dstChannel = e2->getIntAttribute("dstChannel");

        spdlog::debug("[restoreFromXml] Adding connection: src={}/{} -> dst={}/{}", (int)srcFilter.uid, srcChannel,
                      (int)dstFilter.uid, dstChannel);

        // Use addConnectionRaw to bypass undo manager during restore
        bool success = addConnectionRaw(srcFilter, srcChannel, dstFilter, dstChannel);
        spdlog::debug("[restoreFromXml] Connection add result: {}", success ? "SUCCESS" : "FAILED");

        connectionCount++;
    }

    spdlog::info("[FilterGraph::restoreFromXml] Loaded {} nodes, {} connections from XML", nodeCount, connectionCount);

    auto beforeRemove = graph.getConnections().size();
    graph.removeIllegalConnections();
    auto afterRemove = graph.getConnections().size();

    spdlog::info("[FilterGraph::restoreFromXml] After removeIllegalConnections: {} -> {} connections", beforeRemove,
                 afterRemove);
}
