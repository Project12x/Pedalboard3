//	PluginFieldPersistence.cpp - Persistence logic for PluginField
//	----------------------------------------------------------------------------
//	This file is part of Pedalboard3, an audio plugin host.
//	Copyright (c) 2009 Niall Moody.
//
//	This program is free software: you can redistribute it and/or modify
//	it under the terms of the GNU General Public License as published by
//	the Free Software Foundation, either version 3 of the License, or
//	(at your option) any later version.
//
//	This program is distributed in the hope that it will be useful,
//	but WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//	GNU General Public License for more details.
//
//	You should have received a copy of the GNU General Public License
//	along with this program.  If not, see <http://www.gnu.org/licenses/>.
//	----------------------------------------------------------------------------

#include "FilterGraph.h"
#include "InternalFilters.h"
#include "Mapping.h"
#include "PluginComponent.h"
#include "PluginField.h"

#include <set>
#include <spdlog/spdlog.h>
#include <tuple>


using namespace std;

//------------------------------------------------------------------------------
XmlElement* PluginField::getXml() const
{
    int i;
    map<uint32, String>::const_iterator it2;
    multimap<uint32, Mapping*>::const_iterator it;
    XmlElement* retval = new XmlElement("Patch");
    XmlElement* mappingsXml = new XmlElement("Mappings");
    XmlElement* userNamesXml = new XmlElement("UserNames");

    // Update saved window positions.
    for (i = 0; i < getNumChildComponents(); ++i)
    {
        PluginComponent* plugin = dynamic_cast<PluginComponent*>(getChildComponent(i));

        if (plugin)
            plugin->saveWindowState();
    }

    // Set the patch tempo.
    retval->setAttribute("tempo", tempo);

    // Add FilterGraph.
    retval->addChildElement(signalPath->createXml(oscManager));

    // Add Mappings.
    for (it = mappings.begin(); it != mappings.end(); ++it)
        mappingsXml->addChildElement(it->second->getXml());
    retval->addChildElement(mappingsXml);

    // Add user names.
    for (it2 = userNames.begin(); it2 != userNames.end(); ++it2)
    {
        XmlElement* nameXml = new XmlElement("Name");

        nameXml->setAttribute("id", (int)it2->first);
        nameXml->setAttribute("va", it2->second);

        userNamesXml->addChildElement(nameXml);
    }
    retval->addChildElement(userNamesXml);

    return retval;
}

#ifdef __APPLE__
struct NodeAndId
{
    AudioProcessorGraph::Node* node;
    uint32 id;
};
#endif

//------------------------------------------------------------------------------
void PluginField::loadFromXml(XmlElement* patch)
{
    int i, j;
    Array<uint32> paramConnections;

    // === GLITCH-FREE PATCH SWITCHING ===
    // Start crossfade out before making any changes
    auto* crossfader = signalPath->getCrossfadeMixer();
    if (crossfader != nullptr)
    {
        crossfader->startFadeOut(100); // 100ms fade out

        // Wait for fade to complete (blocking, but short)
        // This ensures audio is silent before we destroy plugins
        int maxWaitMs = 150; // Slightly longer than fade time for safety
        int waited = 0;
        while (crossfader->isFading() && waited < maxWaitMs)
        {
            Thread::sleep(5);
            waited += 5;
        }
    }

    // Delete all the filter and connection components.
    {
        // If we don't do this, the connections will try to contact their pins,
        // which may have already been deleted.
        for (i = (getNumChildComponents() - 1); i >= 0; --i)
        {
            PluginConnection* connection = dynamic_cast<PluginConnection*>(getChildComponent(i));

            if (connection)
            {
                removeChildComponent(connection);
                delete connection;
            }
        }
    }
    deleteAllChildren();
    repaint();

    // Wipe userNames.
    userNames.clear();

    // Clear and possibly load the signal path.
    clearMappings();
    if (patch)
    {
        tempo = patch->getDoubleAttribute("tempo", 120.0);

        if (auto* graphXml = patch->getChildByName("FILTERGRAPH"))
            signalPath->restoreFromXml(*graphXml, oscManager);
        else
            signalPath->clear(audioInputEnabled, midiInputEnabled);
    }
    else
        signalPath->clear(audioInputEnabled, midiInputEnabled);

    // === FADE BACK IN ===
    // Start crossfade in after loading is complete
    if (auto* fadeInCrossfader = signalPath->getCrossfadeMixer())
    {
        fadeInCrossfader->startFadeIn(100); // 100ms fade in
    }

    // Add the filter components.
    for (i = 0; i < signalPath->getNumFilters(); ++i)
        addFilter(i, false);

    // Update any plugin names.
    if (patch)
    {
        XmlElement* userNamesXml = patch->getChildByName("UserNames");

        if (userNamesXml)
        {
            forEachXmlChildElement(*userNamesXml, e)
            {
                if (e->hasTagName("Name"))
                {
                    uint32 id = e->getIntAttribute("id");
                    String name = e->getStringAttribute("va");

                    for (i = 0; i < getNumChildComponents(); ++i)
                    {
                        PluginComponent* pluginComp = dynamic_cast<PluginComponent*>(getChildComponent(i));

                        if (pluginComp)
                        {
                            if (pluginComp->getNode()->nodeID.uid == id)
                            {
                                pluginComp->setUserName(name);
                                userNames[id] = name;
                                break;
                            }
                        }
                    }
                }
            }
        }
    }

    // Add the audio/midi connections.
    {
#ifndef __APPLE__ // Stupid gcc...
        struct NodeAndId
        {
            AudioProcessorGraph::Node::Ptr node; // JUCE 8: Use smart pointer
            uint32 id;
        };
#endif
        Array<NodeAndId> tempNodes;

        // Stick all the nodes and their ids in an array.
        for (i = 0; i < signalPath->getNumFilters(); ++i)
        {
            NodeAndId n;

            n.node = signalPath->getNode(i);
            n.id = n.node->nodeID.uid; // JUCE 8: NodeID struct
            tempNodes.add(n);
        }

        // Add the audio/midi connections.
        for (const auto& connection : signalPath->getConnections())
        {
            AudioProcessorGraph::Node* sourceNode = nullptr;
            AudioProcessorGraph::Node* destNode = nullptr;
            PluginComponent* sourceComp = 0;
            PluginComponent* destComp = 0;
            PluginPinComponent* sourcePin;
            PluginPinComponent* destPin;

            // Fill out sourceNode and destNode.
            for (j = 0; j < tempNodes.size(); ++j)
            {
                if (tempNodes[j].id == connection.source.nodeID.uid)
                    sourceNode = tempNodes[j].node.get();
                else if (tempNodes[j].id == connection.destination.nodeID.uid)
                    destNode = tempNodes[j].node.get();
            }

            if (!sourceNode || !destNode)
            {
                jassertfalse;
                continue;
            }
            else if (destNode->getProcessor()->getName() == "Midi Interceptor")
                continue;

            // Now get the source and destination components.
            for (j = 0; j < getNumChildComponents(); ++j)
            {
                PluginComponent* pluginComp = dynamic_cast<PluginComponent*>(getChildComponent(j));

                if (pluginComp)
                {
                    if (pluginComp->getNode() == sourceNode)
                        sourceComp = pluginComp;
                    else if (pluginComp->getNode() == destNode)
                        destComp = pluginComp;
                }
            }

            if (!sourceComp || !destComp)
            {
                jassertfalse;
                continue;
            }

            if ((connection.source.channelIndex == connection.destination.channelIndex) &&
                (connection.source.channelIndex == AudioProcessorGraph::midiChannelIndex))
            {
                sourcePin = sourceComp->getParamPin(0);
                destPin = destComp->getParamPin(0);

                paramConnections.add(connection.destination.nodeID.uid);
            }
            else
            {
                sourcePin = sourceComp->getOutputPin(connection.source.channelIndex);
                destPin = destComp->getInputPin(connection.destination.channelIndex);
            }

            if (!sourcePin || !destPin)
            {
                jassertfalse;
                continue;
            }

            addAndMakeVisible(new PluginConnection(sourcePin, destPin));
        }
    }

    // Add the mappings.
    if (patch)
    {
        XmlElement* mappingsXml = patch->getChildByName("Mappings");

        if (mappingsXml)
        {
            forEachXmlChildElement(*mappingsXml, e)
            {
                if (e->hasTagName("MidiMapping"))
                {
                    MidiMapping* mapping = new MidiMapping(&midiManager, signalPath, e);
                    midiManager.registerMapping(mapping->getCc(), mapping);

                    mappings.insert(make_pair(mapping->getPluginId(), mapping));
                }
                else if (e->hasTagName("OscMapping"))
                {
                    OscMapping* mapping = new OscMapping(&oscManager, signalPath, e);
                    oscManager.registerMapping(mapping->getAddress(), mapping);

                    mappings.insert(make_pair(mapping->getPluginId(), mapping));
                }
            }
        }
    }

    // Connect the Midi Interceptor to the MidiMappingManager.
    if (midiInputEnabled)
    {
        MidiInterceptor* interceptor = 0;

        for (i = 0; i < signalPath->getNumFilters(); ++i)
        {
            interceptor = dynamic_cast<MidiInterceptor*>(signalPath->getNode(i)->getProcessor());

            if (interceptor)
            {
                interceptor->setManager(&midiManager);
                break;
            }
        }
    }

    // Add in any parameter mapping connections.
    {
        multimap<uint32, Mapping*>::iterator it;
        PluginPinComponent* midiInput = 0;
        PluginPinComponent* oscInput = 0;

        // Get the Midi Input and OSC Input pins.
        for (i = 0; i < getNumChildComponents(); ++i)
        {
            PluginComponent* comp = dynamic_cast<PluginComponent*>(getChildComponent(i));

            if (comp)
            {
                String tempstr = comp->getNode()->getProcessor()->getName();

                if (tempstr == "Midi Input")
                    midiInput = comp->getParamPin(0);
                else if (tempstr == "OSC Input")
                    oscInput = comp->getParamPin(0);
            }
        }

        /*jassert(midiInput);
        jassert(oscInput);*/

        if (midiInputEnabled && midiInput)
        {
            // Add a connection for each Midi mapping, checking that we don't already
            // have an identical one.
            for (it = mappings.begin(); it != mappings.end(); ++it)
            {
                MidiMapping* midiMapping = dynamic_cast<MidiMapping*>(it->second);

                if (midiMapping)
                {
                    uint32 uid = midiMapping->getPluginId();

                    if (!paramConnections.contains(uid))
                    {
                        // Find the PluginComponent matching this uid.
                        for (i = 0; i < getNumChildComponents(); ++i)
                        {
                            PluginComponent* comp = dynamic_cast<PluginComponent*>(getChildComponent(i));

                            if (comp)
                            {
                                if (comp->getNode()->nodeID.uid == uid)
                                {
                                    PluginPinComponent* paramInput = 0;

                                    for (j = 0; j < comp->getNumParamPins(); ++j)
                                    {
                                        if (!comp->getParamPin(j)->getDirection())
                                        {
                                            paramInput = comp->getParamPin(j);
                                            break;
                                        }
                                    }

                                    jassert(paramInput);

                                    addAndMakeVisible(new PluginConnection(midiInput, paramInput));
                                    paramConnections.add(uid);

                                    break;
                                }
                            }
                        }
                    }
                }
            }
        }

        if (oscInputEnabled && oscInput)
        {
            // Ditto for the osc mappings.
            paramConnections.clear();
            for (it = mappings.begin(); it != mappings.end(); ++it)
            {
                OscMapping* oscMapping = dynamic_cast<OscMapping*>(it->second);

                if (oscMapping)
                {
                    uint32 uid = oscMapping->getPluginId();

                    if (!paramConnections.contains(uid))
                    {
                        // Find the PluginComponent matching this uid.
                        for (i = 0; i < getNumChildComponents(); ++i)
                        {
                            PluginComponent* comp = dynamic_cast<PluginComponent*>(getChildComponent(i));

                            if (comp)
                            {
                                if (comp->getNode()->nodeID.uid == uid)
                                {
                                    PluginPinComponent* paramInput = 0;

                                    for (j = 0; j < comp->getNumParamPins(); ++j)
                                    {
                                        if (!comp->getParamPin(j)->getDirection())
                                        {
                                            paramInput = comp->getParamPin(j);
                                            break;
                                        }
                                    }

                                    jassert(paramInput);

                                    addAndMakeVisible(new PluginConnection(oscInput, paramInput));
                                    paramConnections.add(uid);

                                    break;
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // Make sure any disabled inputs/outputs don't get accidentally re-enabled.
    if (!audioInputEnabled)
        enableAudioInput(audioInputEnabled);
    if (!midiInputEnabled)
        enableMidiInput(midiInputEnabled);
    if (!oscInputEnabled)
        enableOscInput(oscInputEnabled);

    moveConnectionsBehind();
    repaint();

    // Auto-fit view to show all nodes after patch load
    fitToScreen();
}

//------------------------------------------------------------------------------
void PluginField::clear()
{
    int i;

    // Delete all the filter and connection components.
    {
        // If we don't do this, the connections will try to contact their pins,
        // which may have already been deleted.
        for (i = (getNumChildComponents() - 1); i >= 0; --i)
        {
            PluginConnection* connection = dynamic_cast<PluginConnection*>(getChildComponent(i));

            if (connection)
            {
                removeChildComponent(connection);
                delete connection;
            }
        }
    }
    deleteAllChildren();
    repaint();

    // Wipe userNames.
    userNames.clear();

    // Clear any mappings.
    clearMappings();

    // Clear the signal path.
    signalPath->clear(audioInputEnabled, midiInputEnabled);

    // Add OSC input.
    if (oscInputEnabled)
    {
        OscInput p;
        PluginDescription desc;

        p.fillInPluginDescription(desc);

        signalPath->addFilter(&desc, 50, 400);
    }

    // Setup gui.
    for (i = 0; i < signalPath->getNumFilters(); ++i)
        addFilter(i);

    // Add MidiInterceptor.
    if (midiInputEnabled)
    {
        MidiInterceptor p;
        PluginDescription desc;

        p.fillInPluginDescription(desc);

        signalPath->addFilter(&desc, 100, 100);

        // And connect it up to the midi input.
        {
            AudioProcessorGraph::NodeID midiInput;
            AudioProcessorGraph::NodeID midiInterceptor;

            for (i = 0; i < signalPath->getNumFilters(); ++i)
            {
                if (signalPath->getNode(i)->getProcessor()->getName() == "Midi Input")
                    midiInput = signalPath->getNode(i)->nodeID; // JUCE 8: NodeID struct
                else if (signalPath->getNode(i)->getProcessor()->getName() == "Midi Interceptor")
                {
                    midiInterceptor = signalPath->getNode(i)->nodeID; // JUCE 8: NodeID struct
                    dynamic_cast<MidiInterceptor*>(signalPath->getNode(i)->getProcessor())->setManager(&midiManager);
                }
            }
            signalPath->addConnection(midiInput, AudioProcessorGraph::midiChannelIndex, midiInterceptor,
                                      AudioProcessorGraph::midiChannelIndex);
        }
    }

    repaint();
}

//------------------------------------------------------------------------------
void PluginField::clearDoubleClickMessage()
{
    displayDoubleClickMessage = false;
    repaint();
}

//------------------------------------------------------------------------------
void PluginField::syncWithGraph()
{
    // Build a set of all NodeIDs in the graph
    std::set<uint32> graphNodeIds;
    for (int i = 0; i < signalPath->getNumFilters(); ++i)
    {
        auto node = signalPath->getNode(i);
        if (node != nullptr)
            graphNodeIds.insert(node->nodeID.uid);
    }

    // Helper lambda to safely get uid from PluginComponent
    // Uses the pins which store uid as a member (not a pointer that can dangle)
    auto getComponentUid = [](PluginComponent* comp) -> uint32
    {
        if (comp->getNumInputPins() > 0)
            return comp->getInputPin(0)->getUid();
        if (comp->getNumOutputPins() > 0)
            return comp->getOutputPin(0)->getUid();
        if (comp->getNumParamPins() > 0)
            return comp->getParamPin(0)->getUid();
        return 0;
    };

    // Find PluginComponents that no longer have a corresponding graph node
    std::vector<PluginComponent*> toRemove;
    for (int i = 0; i < getNumChildComponents(); ++i)
    {
        PluginComponent* comp = dynamic_cast<PluginComponent*>(getChildComponent(i));
        if (comp != nullptr)
        {
            uint32 uid = getComponentUid(comp);
            if (uid != 0 && graphNodeIds.find(uid) == graphNodeIds.end())
            {
                toRemove.push_back(comp);
            }
        }
    }

    // Remove orphan PluginComponents
    for (auto* comp : toRemove)
    {
        comp->removeChangeListener(this);
        removeChildComponent(comp);
        delete comp;
    }

    // Find graph nodes that don't have a PluginComponent
    std::set<uint32> uiNodeIds;
    for (int i = 0; i < getNumChildComponents(); ++i)
    {
        PluginComponent* comp = dynamic_cast<PluginComponent*>(getChildComponent(i));
        if (comp != nullptr)
        {
            uint32 uid = getComponentUid(comp);
            if (uid != 0)
                uiNodeIds.insert(uid);
        }
    }

    for (int i = 0; i < signalPath->getNumFilters(); ++i)
    {
        auto node = signalPath->getNode(i);
        if (node != nullptr && uiNodeIds.find(node->nodeID.uid) == uiNodeIds.end())
        {
            // Skip hidden infrastructure nodes (CrossfadeMixer, SafetyLimiter)
            if (signalPath->isHiddenInfrastructureNode(node->nodeID))
                continue;

            // Add missing PluginComponent
            addFilter(i, false);
        }
    }

    // Sync connections: remove UI connections not in graph, add graph connections not in UI
    auto graphConnections = signalPath->getConnections();

    // Build set of graph connections for fast lookup
    std::set<std::tuple<uint32, int, uint32, int>> graphConnSet;
    for (const auto& conn : graphConnections)
    {
        graphConnSet.insert(std::make_tuple(conn.source.nodeID.uid, conn.source.channelIndex,
                                            conn.destination.nodeID.uid, conn.destination.channelIndex));
    }

    // Build set of UI connections
    std::set<std::tuple<uint32, int, uint32, int>> uiConnSet;
    std::vector<PluginConnection*> connectionsToRemove;
    for (int i = 0; i < getNumChildComponents(); ++i)
    {
        PluginConnection* conn = dynamic_cast<PluginConnection*>(getChildComponent(i));
        if (conn != nullptr)
        {
            auto* src = conn->getSource();
            auto* dest = conn->getDestination();

            if (src != nullptr && dest != nullptr)
            {
                auto key = std::make_tuple(src->getUid(), src->getChannel(), dest->getUid(), dest->getChannel());
                uiConnSet.insert(key);

                // Check if this UI connection exists in graph
                if (graphConnSet.find(key) == graphConnSet.end())
                {
                    connectionsToRemove.push_back(conn);
                }
            }
            else
            {
                // Invalid connection, remove it
                connectionsToRemove.push_back(conn);
            }
        }
    }

    // Remove UI connections that aren't in graph
    for (auto* conn : connectionsToRemove)
    {
        removeChildComponent(conn);
        delete conn;
    }

    // Add graph connections that aren't in UI
    for (const auto& conn : graphConnections)
    {
        auto key = std::make_tuple(conn.source.nodeID.uid, conn.source.channelIndex, conn.destination.nodeID.uid,
                                   conn.destination.channelIndex);

        if (uiConnSet.find(key) == uiConnSet.end())
        {
            // Find the source and destination PluginComponents and their pins
            PluginComponent* sourceComp = nullptr;
            PluginComponent* destComp = nullptr;

            for (int i = 0; i < getNumChildComponents(); ++i)
            {
                PluginComponent* pc = dynamic_cast<PluginComponent*>(getChildComponent(i));
                if (pc != nullptr)
                {
                    if (pc->getNumOutputPins() > 0 && pc->getOutputPin(0)->getUid() == conn.source.nodeID.uid)
                        sourceComp = pc;
                    if (pc->getNumInputPins() > 0 && pc->getInputPin(0)->getUid() == conn.destination.nodeID.uid)
                        destComp = pc;
                }
            }

            if (sourceComp != nullptr && destComp != nullptr)
            {
                PluginPinComponent* sourcePin = nullptr;
                PluginPinComponent* destPin = nullptr;

                // Find the source pin by channel
                bool isMidiChannel = (conn.source.channelIndex == AudioProcessorGraph::midiChannelIndex);
                if (isMidiChannel)
                {
                    sourcePin = sourceComp->getParamPin(0);
                    destPin = destComp->getParamPin(0);
                }
                else
                {
                    if (conn.source.channelIndex < sourceComp->getNumOutputPins())
                        sourcePin = sourceComp->getOutputPin(conn.source.channelIndex);
                    if (conn.destination.channelIndex < destComp->getNumInputPins())
                        destPin = destComp->getInputPin(conn.destination.channelIndex);
                }

                if (sourcePin != nullptr && destPin != nullptr)
                {
                    addAndMakeVisible(new PluginConnection(sourcePin, destPin));
                }
            }
        }
    }

    repaint();
}

//------------------------------------------------------------------------------
void PluginField::clearMappings()
{
    multimap<uint32, Mapping*>::iterator it;

    for (it = mappings.begin(); it != mappings.end(); ++it)
        delete it->second;

    mappings.clear();
}
