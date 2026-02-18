/*
  ==============================================================================

    SubGraphProcessor.cpp

    Implementation of the rack/sub-graph processor.

  ==============================================================================
*/

#include "SubGraphProcessor.h"

#include "AudioSingletons.h"
#include "BypassableInstance.h"
#include "SubGraphEditorComponent.h"

#include <spdlog/spdlog.h>

//==============================================================================
SubGraphProcessor::SubGraphProcessor()
    : AudioPluginInstance(BusesProperties()
                              .withInput("Input", juce::AudioChannelSet::stereo(), true)
                              .withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
    rackName = "New Rack";
    spdlog::debug("[SubGraphProcessor] Creating new rack: {}", rackName.toStdString());

    // CRITICAL: Initialize graph IMMEDIATELY in constructor with default settings
    // This eliminates ALL race conditions - graph is ready before any thread can access
    initializeInternalGraph();

    // Prepare with default settings (will be updated in prepareToPlay)
    internalGraph.setPlayConfigDetails(2, 2, 44100.0, 512);
    internalGraph.prepareToPlay(44100.0, 512);

    // Mark as initialized - now safe for any thread to access
    internalGraphInitialized.store(true, std::memory_order_release);
    spdlog::debug("[SubGraphProcessor] Constructor complete - graph ready");
}

SubGraphProcessor::~SubGraphProcessor()
{
    spdlog::debug("[SubGraphProcessor] Destroying rack: {}", rackName.toStdString());
}

//==============================================================================
void SubGraphProcessor::initializeInternalGraph()
{
    // Clear any existing nodes
    internalGraph.clear();

    // Create internal I/O processors that bridge to the parent
    auto audioIn = std::make_unique<juce::AudioProcessorGraph::AudioGraphIOProcessor>(
        juce::AudioProcessorGraph::AudioGraphIOProcessor::audioInputNode);
    auto audioOut = std::make_unique<juce::AudioProcessorGraph::AudioGraphIOProcessor>(
        juce::AudioProcessorGraph::AudioGraphIOProcessor::audioOutputNode);
    auto midiIn = std::make_unique<juce::AudioProcessorGraph::AudioGraphIOProcessor>(
        juce::AudioProcessorGraph::AudioGraphIOProcessor::midiInputNode);

    // Add to internal graph and store node IDs
    if (auto node = internalGraph.addNode(std::move(audioIn), juce::AudioProcessorGraph::NodeID(rackAudioInUid)))
    {
        rackAudioInNode = node->nodeID;
        node->properties.set("x", 50.0);
        node->properties.set("y", 100.0);
        spdlog::debug("[SubGraphProcessor] Created rack audio input node: {}", rackAudioInNode.uid);
    }

    if (auto node = internalGraph.addNode(std::move(audioOut), juce::AudioProcessorGraph::NodeID(rackAudioOutUid)))
    {
        rackAudioOutNode = node->nodeID;
        node->properties.set("x", 400.0);
        node->properties.set("y", 100.0);
        spdlog::debug("[SubGraphProcessor] Created rack audio output node: {}", rackAudioOutNode.uid);
    }

    if (auto node = internalGraph.addNode(std::move(midiIn), juce::AudioProcessorGraph::NodeID(rackMidiInUid)))
    {
        rackMidiInNode = node->nodeID;
        node->properties.set("x", 50.0);
        node->properties.set("y", 250.0);
        spdlog::debug("[SubGraphProcessor] Created rack MIDI input node: {}", rackMidiInNode.uid);
    }

    // Connect audio input directly to output (passthrough by default)
    internalGraph.addConnection({{rackAudioInNode, 0}, {rackAudioOutNode, 0}}); // Left
    internalGraph.addConnection({{rackAudioInNode, 1}, {rackAudioOutNode, 1}}); // Right

    spdlog::debug("[SubGraphProcessor] Initialized internal graph with {} nodes", internalGraph.getNumNodes());
}

//==============================================================================
void SubGraphProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    // Store for potential later use
    currentSampleRate = sampleRate;
    currentBlockSize = samplesPerBlock;

    spdlog::debug("[SubGraphProcessor] prepareToPlay: {} Hz, {} samples", sampleRate, samplesPerBlock);

    // Re-prepare with actual host settings (graph already initialized in constructor)
    const juce::ScopedLock sl(internalGraph.getCallbackLock());
    internalGraph.setPlayConfigDetails(getTotalNumInputChannels(), getTotalNumOutputChannels(), sampleRate,
                                       samplesPerBlock);
    internalGraph.prepareToPlay(sampleRate, samplesPerBlock);
}

void SubGraphProcessor::releaseResources()
{
    internalGraph.releaseResources();
}

void SubGraphProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    // Graph is already initialized in constructor - just delegate
    internalGraph.processBlock(buffer, midiMessages);
}

//==============================================================================
double SubGraphProcessor::getTailLengthSeconds() const
{
    // Sum up tail lengths from all processors in the internal graph
    double maxTail = 0.0;

    for (int i = 0; i < internalGraph.getNumNodes(); ++i)
    {
        if (auto node = internalGraph.getNode(i))
        {
            if (auto* proc = node->getProcessor())
            {
                maxTail = std::max(maxTail, proc->getTailLengthSeconds());
            }
        }
    }

    return maxTail;
}

//==============================================================================
void SubGraphProcessor::fillInPluginDescription(juce::PluginDescription& description) const
{
    description.name = "Effect Rack"; // Fixed name for plugin list
    description.pluginFormatName = "Internal";
    description.category = "Built-in";
    description.manufacturerName = "Pedalboard3";
    description.version = "1.0";
    description.fileOrIdentifier = "Internal:SubGraph";
    description.isInstrument = false;
    description.numInputChannels = getTotalNumInputChannels();
    description.numOutputChannels = getTotalNumOutputChannels();
    description.uniqueId = 0x53554247; // "SUBG"
}

//==============================================================================
juce::AudioProcessorEditor* SubGraphProcessor::createEditor()
{
    // Minimal test - just setSize in constructor
    return new SubGraphEditorComponent(*this);
}

//==============================================================================
void SubGraphProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    spdlog::debug("[SubGraphProcessor] Saving state for rack: {}", rackName.toStdString());

    if (auto xml = std::unique_ptr<juce::XmlElement>(createRackXml()))
    {
        copyXmlToBinary(*xml, destData);
    }
}

void SubGraphProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    spdlog::debug("[SubGraphProcessor] Restoring state...");

    if (auto xml = getXmlFromBinary(data, sizeInBytes))
    {
        if (xml->hasTagName("RACK"))
        {
            restoreFromRackXml(*xml);
        }
    }
}

//==============================================================================
bool SubGraphProcessor::loadFromFile(const juce::File& rackFile)
{
    spdlog::info("[SubGraphProcessor] Loading rack from: {}", rackFile.getFullPathName().toStdString());

    if (auto xml = juce::XmlDocument::parse(rackFile))
    {
        if (xml->hasTagName("RACK"))
        {
            restoreFromRackXml(*xml);
            rackName = rackFile.getFileNameWithoutExtension();
            return true;
        }
    }

    spdlog::error("[SubGraphProcessor] Failed to load rack file");
    return false;
}

bool SubGraphProcessor::saveToFile(const juce::File& rackFile)
{
    spdlog::info("[SubGraphProcessor] Saving rack to: {}", rackFile.getFullPathName().toStdString());

    if (auto xml = std::unique_ptr<juce::XmlElement>(createRackXml()))
    {
        if (xml->writeTo(rackFile))
        {
            rackName = rackFile.getFileNameWithoutExtension();
            return true;
        }
    }

    spdlog::error("[SubGraphProcessor] Failed to save rack file");
    return false;
}

//==============================================================================
juce::XmlElement* SubGraphProcessor::createRackXml() const
{
    auto* xml = new juce::XmlElement("RACK");
    xml->setAttribute("name", rackName);
    xml->setAttribute("version", 1);

    // Save all nodes (except built-in I/O nodes which are recreated)
    for (int i = 0; i < internalGraph.getNumNodes(); ++i)
    {
        if (auto node = internalGraph.getNode(i))
        {
            // Skip internal I/O nodes - they're recreated on load
            if (node->nodeID == rackAudioInNode || node->nodeID == rackAudioOutNode || node->nodeID == rackMidiInNode)
                continue;

            if (auto* plugin = dynamic_cast<juce::AudioPluginInstance*>(node->getProcessor()))
            {
                auto* nodeXml = new juce::XmlElement("FILTER");
                nodeXml->setAttribute("uid", (int)node->nodeID.uid);
                nodeXml->setAttribute("x", (double)node->properties.getWithDefault("x", 0.0));
                nodeXml->setAttribute("y", (double)node->properties.getWithDefault("y", 0.0));

                // Save plugin description
                juce::PluginDescription pd;
                plugin->fillInPluginDescription(pd);
                nodeXml->addChildElement(pd.createXml().release());

                // Save plugin state
                juce::MemoryBlock state;
                plugin->getStateInformation(state);
                auto* stateXml = new juce::XmlElement("STATE");
                stateXml->addTextElement(state.toBase64Encoding());
                nodeXml->addChildElement(stateXml);

                xml->addChildElement(nodeXml);
            }
        }
    }

    // Save connections
    for (const auto& conn : internalGraph.getConnections())
    {
        auto* connXml = new juce::XmlElement("CONNECTION");
        connXml->setAttribute("srcNode", (int)conn.source.nodeID.uid);
        connXml->setAttribute("srcChannel", conn.source.channelIndex);
        connXml->setAttribute("dstNode", (int)conn.destination.nodeID.uid);
        connXml->setAttribute("dstChannel", conn.destination.channelIndex);
        xml->addChildElement(connXml);
    }

    spdlog::debug("[SubGraphProcessor] Created XML with {} child elements", xml->getNumChildElements());
    return xml;
}

void SubGraphProcessor::restoreFromRackXml(const juce::XmlElement& xml)
{
    rackName = xml.getStringAttribute("name", "Loaded Rack");
    spdlog::debug("[SubGraphProcessor] Restoring rack: {}", rackName.toStdString());

    // --- Phase 1: Create plugin instances OUTSIDE the callback lock ---
    // Plugin creation can be slow (DLL loading, initialization), so we must not
    // hold the audio callback lock during this phase.
    struct PreparedNode
    {
        int oldUid;
        double x, y;
        juce::String name;
        std::unique_ptr<juce::AudioProcessor> processor;
    };
    std::vector<PreparedNode> preparedNodes;

    for (auto* filterXml : xml.getChildWithTagNameIterator("FILTER"))
    {
        int oldUid = filterXml->getIntAttribute("uid");
        double x = filterXml->getDoubleAttribute("x", 200.0);
        double y = filterXml->getDoubleAttribute("y", 200.0);

        juce::PluginDescription pd;
        bool foundDesc = false;

        for (auto* child : filterXml->getChildIterator())
        {
            if (pd.loadFromXml(*child))
            {
                foundDesc = true;
                break;
            }
        }

        if (!foundDesc)
        {
            spdlog::warn("[SubGraphProcessor] Could not find plugin description for uid {}", oldUid);
            continue;
        }

        // Create plugin instance (potentially slow - outside lock)
        juce::String errorMessage;
        auto instance = AudioPluginFormatManagerSingleton::getInstance().createPluginInstance(
            pd, currentSampleRate, currentBlockSize, errorMessage);

        if (!instance)
        {
            spdlog::error("[SubGraphProcessor] Failed to create plugin: {} - {}", pd.name.toStdString(),
                          errorMessage.toStdString());
            continue;
        }

        // Mirror SubGraphFilterGraph::addFilterRaw stereo layout setup
        juce::AudioProcessor::BusesLayout stereoLayout;
        stereoLayout.inputBuses.add(juce::AudioChannelSet::stereo());
        stereoLayout.outputBuses.add(juce::AudioChannelSet::stereo());
        if (instance->checkBusesLayoutSupported(stereoLayout))
            instance->setBusesLayout(stereoLayout);

        // Restore state
        if (auto* stateXml = filterXml->getChildByName("STATE"))
        {
            juce::MemoryBlock state;
            state.fromBase64Encoding(stateXml->getAllSubText());
            instance->setStateInformation(state.getData(), (int)state.getSize());
        }

        // Wrap for bypass support (mirrors SubGraphFilterGraph::addFilterRaw)
        std::unique_ptr<juce::AudioProcessor> processor;
        if (dynamic_cast<juce::AudioProcessorGraph::AudioGraphIOProcessor*>(instance.get()) ||
            dynamic_cast<SubGraphProcessor*>(instance.get()))
        {
            processor = std::move(instance);
        }
        else
        {
            processor = std::make_unique<BypassableInstance>(instance.release());
        }

        preparedNodes.push_back({oldUid, x, y, pd.name, std::move(processor)});
    }

    // Parse connection XML (lightweight, no lock needed)
    struct PreparedConnection
    {
        int srcUid, srcChannel, dstUid, dstChannel;
    };
    std::vector<PreparedConnection> preparedConnections;

    for (auto* connXml : xml.getChildWithTagNameIterator("CONNECTION"))
    {
        preparedConnections.push_back({connXml->getIntAttribute("srcNode"), connXml->getIntAttribute("srcChannel"),
                                       connXml->getIntAttribute("dstNode"), connXml->getIntAttribute("dstChannel")});
    }

    // --- Phase 2: Mutate the graph UNDER the callback lock ---
    {
        const juce::ScopedLock sl(internalGraph.getCallbackLock());

        // Reinitialize with fresh I/O nodes
        initializeInternalGraph();

        // Clear the default passthrough connections
        auto connections = internalGraph.getConnections();
        for (const auto& conn : connections)
            internalGraph.removeConnection(conn);

        // Map old UIDs to new node IDs for connection restoration
        std::map<int, juce::AudioProcessorGraph::NodeID> uidToNodeId;

        // Add pre-created nodes to graph
        for (auto& prepared : preparedNodes)
        {
            if (auto node = internalGraph.addNode(std::move(prepared.processor)))
            {
                node->properties.set("x", prepared.x);
                node->properties.set("y", prepared.y);
                uidToNodeId[prepared.oldUid] = node->nodeID;
                spdlog::debug("[SubGraphProcessor] Loaded plugin: {} as node {}", prepared.name.toStdString(),
                              node->nodeID.uid);
            }
        }

        // Add connections
        for (const auto& pc : preparedConnections)
        {
            auto srcIt = uidToNodeId.find(pc.srcUid);
            auto dstIt = uidToNodeId.find(pc.dstUid);

            juce::AudioProcessorGraph::NodeID srcNode, dstNode;

            if (srcIt != uidToNodeId.end())
                srcNode = srcIt->second;
            else if (pc.srcUid == rackAudioInUid)
                srcNode = rackAudioInNode;
            else if (pc.srcUid == rackMidiInUid)
                srcNode = rackMidiInNode;
            else
            {
                spdlog::warn("[SubGraphProcessor] Unknown source node UID: {}", pc.srcUid);
                continue;
            }

            if (dstIt != uidToNodeId.end())
                dstNode = dstIt->second;
            else if (pc.dstUid == rackAudioOutUid)
                dstNode = rackAudioOutNode;
            else
            {
                spdlog::warn("[SubGraphProcessor] Unknown destination node UID: {}", pc.dstUid);
                continue;
            }

            internalGraph.addConnection({{srcNode, pc.srcChannel}, {dstNode, pc.dstChannel}});
        }
    }

    spdlog::info("[SubGraphProcessor] Restored rack with {} nodes", internalGraph.getNumNodes());
}
