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
#include "SettingsManager.h"
#include "SubGraphProcessor.h"
#include "UndoActions.h"

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

FilterGraph::FilterGraph()
    : FileBasedDocument(filenameSuffix, filenameWildcard, "Load a filter graph", "Save a filter graph"), lastUID(0)
{
    InternalPluginFormat internalFormat;
    bool audioInput = SettingsManager::getInstance().getBool("AudioInput", true);
    bool midiInput = SettingsManager::getInstance().getBool("MidiInput", true);

    if (audioInput)
    {
        // Use Raw method to avoid adding to undo history
        addFilterRaw(internalFormat.getDescriptionFor(InternalPluginFormat::audioInputFilter), 50.0f, 100.0f);
    }

    if (midiInput)
    {
        // Use Raw method to avoid adding to undo history
        addFilterRaw(internalFormat.getDescriptionFor(InternalPluginFormat::midiInputFilter), 50.0f, 250.0f);
    }

    // Use Raw method to avoid adding to undo history
    addFilterRaw(internalFormat.getDescriptionFor(InternalPluginFormat::audioOutputFilter), 400.0f, 100.0f);

    // Initialize SafetyLimiter for audio protection
    auto limiterProcessor = std::make_unique<SafetyLimiterProcessor>();
    safetyLimiter = limiterProcessor.get(); // Keep raw pointer for state queries
    auto safetyNode = graph.addNode(std::move(limiterProcessor), AudioProcessorGraph::NodeID(0xFFFFFF));
    if (safetyNode)
    {
        safetyLimiterNodeId = safetyNode->nodeID;
        // Position off-screen (not visible to user)
        safetyNode->properties.set("x", -100.0);
        safetyNode->properties.set("y", -100.0);
    }

    // Initialize CrossfadeMixer for glitch-free patch switching
    auto crossfadeProcessor = std::make_unique<CrossfadeMixerProcessor>();
    crossfadeMixer = crossfadeProcessor.get(); // Keep raw pointer for crossfade control
    auto crossfadeNode = graph.addNode(std::move(crossfadeProcessor), AudioProcessorGraph::NodeID(0xFFFFFE));
    if (crossfadeNode)
    {
        crossfadeMixerNodeId = crossfadeNode->nodeID;
        // Position off-screen (not visible to user)
        crossfadeNode->properties.set("x", -100.0);
        crossfadeNode->properties.set("y", -150.0);
    }

    setChangedFlag(false);
}

FilterGraph::~FilterGraph()
{
    graph.clear();
}

uint32 FilterGraph::getNextUID() throw()
{
    return ++lastUID;
}

//==============================================================================
int FilterGraph::getNumFilters() const throw()
{
    return graph.getNumNodes();
}

const AudioProcessorGraph::Node::Ptr FilterGraph::getNode(const int index) const throw()
{
    return graph.getNode(index);
}

const AudioProcessorGraph::Node::Ptr FilterGraph::getNodeForId(const AudioProcessorGraph::NodeID uid) const throw()
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

// JUCE 8: Implementation of getConnectionBetween - finds a matching connection
const AudioProcessorGraph::Connection* FilterGraph::getConnectionBetween(AudioProcessorGraph::NodeID sourceFilterUID,
                                                                         int sourceFilterChannel,
                                                                         AudioProcessorGraph::NodeID destFilterUID,
                                                                         int destFilterChannel) const
{
    // Search through all connections to find a matching one
    auto connections = graph.getConnections();
    for (const auto& conn : connections)
    {
        if (conn.source.nodeID == sourceFilterUID && conn.source.channelIndex == sourceFilterChannel &&
            conn.destination.nodeID == destFilterUID && conn.destination.channelIndex == destFilterChannel)
        {
            // Return pointer to the connection - we need to store it to return a
            // valid pointer
            static AudioProcessorGraph::Connection lastFoundConnection;
            lastFoundConnection = conn;
            return &lastFoundConnection;
        }
    }
    return nullptr;
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

    spdlog::debug("[addFilterRaw] Adding plugin: {} (identifier: {})", desc->name.toStdString(),
                  desc->fileOrIdentifier.toStdString());
    spdlog::default_logger()->flush();

    String errorMessage;
    auto tempInstance =
        AudioPluginFormatManagerSingleton::getInstance().createPluginInstance(*desc, 44100.0, 512, errorMessage);

    spdlog::debug("[addFilterRaw] createPluginInstance returned: {}", tempInstance ? "valid" : "null");
    if (!tempInstance)
        spdlog::error("[addFilterRaw] Error: {}", errorMessage.toStdString());
    spdlog::default_logger()->flush();

    if (tempInstance)
    {
        AudioProcessor::BusesLayout stereoLayout;
        stereoLayout.inputBuses.add(AudioChannelSet::stereo());
        stereoLayout.outputBuses.add(AudioChannelSet::stereo());

        if (tempInstance->checkBusesLayoutSupported(stereoLayout))
            tempInstance->setBusesLayout(stereoLayout);

        spdlog::debug("[addFilterRaw] Plugin channels: in={}, out={}", tempInstance->getTotalNumInputChannels(),
                      tempInstance->getTotalNumOutputChannels());
        spdlog::default_logger()->flush();
    }

    std::unique_ptr<AudioProcessor> instance;
    if (tempInstance)
    {
        spdlog::debug("[addFilterRaw] Checking if plugin needs wrapping...");
        spdlog::default_logger()->flush();

        if (dynamic_cast<AudioProcessorGraph::AudioGraphIOProcessor*>(tempInstance.get()) ||
            dynamic_cast<MidiInterceptor*>(tempInstance.get()) || dynamic_cast<OscInput*>(tempInstance.get()) ||
            dynamic_cast<SubGraphProcessor*>(tempInstance.get()))
        {
            spdlog::debug("[addFilterRaw] Using plugin directly (no BypassableInstance wrapper)");
            instance = std::move(tempInstance);
        }
        else
        {
            spdlog::debug("[addFilterRaw] Wrapping in BypassableInstance");
            instance = std::make_unique<BypassableInstance>(tempInstance.release());
        }
        spdlog::debug("[addFilterRaw] Wrapping complete");
        spdlog::default_logger()->flush();
    }

    AudioProcessorGraph::Node::Ptr node;
    if (instance)
    {
        spdlog::debug("[addFilterRaw] Adding to graph...");
        spdlog::default_logger()->flush();

        // CRITICAL: Lock the audio callback to prevent race with audio thread
        // This ensures the node is fully added before audio processing resumes
        {
            const juce::ScopedLock sl(graph.getCallbackLock());
            node = graph.addNode(std::move(instance));
        }

        spdlog::debug("[addFilterRaw] addNode returned: {}", node ? "valid" : "null");
        spdlog::default_logger()->flush();
    }

    if (node != nullptr)
    {
        spdlog::debug("[addFilterRaw] Setting node properties...");
        spdlog::default_logger()->flush();
        node->properties.set("x", x);
        spdlog::debug("[addFilterRaw] Set x property");
        spdlog::default_logger()->flush();
        node->properties.set("y", y);
        spdlog::debug("[addFilterRaw] Set y property, checking processor type...");
        spdlog::default_logger()->flush();

        // Check processor before dynamic_cast
        AudioProcessor* proc = node->getProcessor();
        spdlog::debug("[addFilterRaw] Got processor pointer: {}", proc ? "valid" : "null");
        spdlog::default_logger()->flush();

        // TEMPORARY TEST: Skip changed() for SubGraphProcessor to isolate crash
        bool isSubGraph = (proc != nullptr) && (dynamic_cast<SubGraphProcessor*>(proc) != nullptr);
        spdlog::debug("[addFilterRaw] isSubGraph = {}", isSubGraph);
        spdlog::default_logger()->flush();

        // Debug: Add delay to see if crash is immediate or async
        spdlog::debug("[addFilterRaw] About to enter if block...");
        spdlog::default_logger()->flush();

        if (isSubGraph)
        {
            spdlog::debug("[addFilterRaw] Inside if block for SubGraphProcessor");
            spdlog::default_logger()->flush();

            // Add explicit delay to see if crash is async
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            spdlog::debug("[addFilterRaw] After 50ms delay - still alive");
            spdlog::default_logger()->flush();

            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            spdlog::debug("[addFilterRaw] After 100ms total - still alive, skipping changed()");
            spdlog::default_logger()->flush();
        }
        else
        {
            spdlog::debug("[addFilterRaw] Properties set, calling changed()...");
            spdlog::default_logger()->flush();
            try
            {
                changed();
                spdlog::debug("[addFilterRaw] changed() completed successfully");
            }
            catch (const std::exception& e)
            {
                spdlog::error("[addFilterRaw] Exception in changed(): {}", e.what());
                spdlog::default_logger()->flush();
            }
            catch (...)
            {
                spdlog::error("[addFilterRaw] Unknown exception in changed()");
                spdlog::default_logger()->flush();
            }
        }
        spdlog::debug("[addFilterRaw] All branches complete, about to access node->nodeID");
        spdlog::default_logger()->flush();

        spdlog::debug("[addFilterRaw] node pointer: {}", node ? "valid" : "null");
        spdlog::default_logger()->flush();

        auto nodeId = node->nodeID;
        spdlog::debug("[addFilterRaw] Successfully added node with ID: {}", nodeId.uid);
        spdlog::default_logger()->flush();

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

void FilterGraph::clear(bool addAudioIn, bool addMidiIn, bool addAudioOut)
{
    InternalPluginFormat internalFormat;

    // PluginWindow::closeAllCurrentlyOpenWindows();

    graph.clear();

    if (addAudioIn)
    {
        addFilter(internalFormat.getDescriptionFor(InternalPluginFormat::audioInputFilter), 50.0f, 100.0f);
    }

    if (addMidiIn)
    {
        addFilter(internalFormat.getDescriptionFor(InternalPluginFormat::midiInputFilter), 50.0f, 250.0f);
    }

    if (addAudioOut)
    {
        addFilter(internalFormat.getDescriptionFor(InternalPluginFormat::audioOutputFilter), 400.0f, 100.0f);
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
        pd.name = subGraph->getName();
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
            dynamic_cast<MidiInterceptor*>(tempInstance.get()) || dynamic_cast<OscInput*>(tempInstance.get()))
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

        bypassable->setMIDIChannel(xml.getIntAttribute("MIDIChanne"));
    }

    node->getProcessor()->setCurrentProgram(xml.getIntAttribute("program"));
}

XmlElement* FilterGraph::createXml(const OscMappingManager& oscManager) const
{
    XmlElement* xml = new XmlElement("FILTERGRAPH");

    int i;
    for (i = 0; i < graph.getNumNodes(); ++i)
        xml->addChildElement(createNodeXml(graph.getNode(i), oscManager));

    // JUCE 8: getConnections returns vector
    auto connections = graph.getConnections();
    for (const auto& fc : connections)
    {
        XmlElement* e = new XmlElement("CONNECTION");

        e->setAttribute("srcFilter", (int)fc.source.nodeID.uid);
        e->setAttribute("srcChannel", fc.source.channelIndex);
        e->setAttribute("dstFilter", (int)fc.destination.nodeID.uid);
        e->setAttribute("dstChannel", fc.destination.channelIndex);

        xml->addChildElement(e);
    }

    spdlog::info("[FilterGraph::createXml] Saved {} nodes and {} connections", graph.getNumNodes(), connections.size());
    return xml;
}

void FilterGraph::restoreFromXml(const XmlElement& xml, OscMappingManager& oscManager)
{
    clear(false, false, false);

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
