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

#ifndef __JUCE_FILTERGRAPH_JUCEHEADER__
#define __JUCE_FILTERGRAPH_JUCEHEADER__

#include "CrossfadeMixer.h"
#include "IFilterGraph.h"
#include "OscMappingManager.h"
#include "SafetyLimiter.h"

#include <JuceHeader.h>

using juce::uint32;

class FilterInGraph;
class FilterGraph;

const char* const filenameSuffix = ".filtergraph";
const char* const filenameWildcard = "*.filtergraph";

//==============================================================================
/**
    Represents a connection between two pins in a FilterGraph.
*/
class FilterConnection
{
  public:
    //==============================================================================
    FilterConnection(FilterGraph& owner);
    FilterConnection(const FilterConnection& other);
    ~FilterConnection();

    //==============================================================================
    AudioProcessorGraph::NodeID sourceFilterID;
    int sourceChannel;
    AudioProcessorGraph::NodeID destFilterID;
    int destChannel;

    //==============================================================================

  private:
    FilterGraph& owner;

    const FilterConnection& operator=(const FilterConnection&);
};

//==============================================================================
/**
    Represents one of the filters in a FilterGraph.
*/
/*class FilterInGraph   : public ReferenceCountedObject
{
public:
    //==============================================================================
    FilterInGraph (FilterGraph& owner, AudioPluginInstance* const plugin);
    ~FilterInGraph();

    //==============================================================================
    AudioPluginInstance* const filter;
    uint32 uid;

    //==============================================================================
    void showUI (bool useGenericUI);

    double getX() const throw()                     { return x; }
    double getY() const throw()                     { return y; }
    void setPosition (double x, double y) throw();

    XmlElement* createXml() const;

    static FilterInGraph* createForDescription (FilterGraph& owner,
                                                const PluginDescription& desc,
                                                String& errorMessage);

    static FilterInGraph* createFromXml (FilterGraph& owner, const XmlElement&
xml);

    //==============================================================================
    typedef ReferenceCountedObjectPtr <FilterInGraph> Ptr;

    //==============================================================================


private:
    friend class FilterGraphPlayer;
    FilterGraph& owner;
    double x, y;

    friend class PluginWindow;
    Component* activeUI;
    Component* activeGenericUI;
    int lastX, lastY;

    MidiBuffer outputMidi;
    AudioSampleBuffer processedAudio;
    MidiBuffer processedMidi;

    void prepareBuffers (int blockSize);
    void renderBlock (int numSamples,
                      const ReferenceCountedArray <FilterInGraph>& filters,
                      const OwnedArray <FilterConnection>& connections);

    FilterInGraph (const FilterInGraph&);
    const FilterInGraph& operator= (const FilterInGraph&);
};
*/

//==============================================================================
/**
    A collection of filters and some connections between them.
*/
class FilterGraph : public IFilterGraph, public FileBasedDocument
{
  public:
    //==============================================================================
    FilterGraph();
    ~FilterGraph();

    //==============================================================================
    AudioProcessorGraph& getGraph() override { return graph; }

    /// Returns the UndoManager for undo/redo operations
    juce::UndoManager& getUndoManager() override { return undoManager; }

    /// Returns the SafetyLimiter for audio protection state queries
    SafetyLimiterProcessor* getSafetyLimiter() const { return safetyLimiter; }

    /// Returns true if audio device is active and processing audio
    bool isAudioPlaying() const { return graph.getSampleRate() > 0; }

    /// Configures the graph's bus layout to match the audio device
    void setDeviceChannelCounts(int numInputs, int numOutputs);

    /// Returns the CrossfadeMixer for glitch-free patch switching
    CrossfadeMixerProcessor* getCrossfadeMixer() const { return crossfadeMixer; }

    /// Returns true if node is hidden infrastructure (SafetyLimiter, CrossfadeMixer)
    bool isHiddenInfrastructureNode(AudioProcessorGraph::NodeID nodeId) const override
    {
        return nodeId.uid == 0xFFFFFF || nodeId.uid == 0xFFFFFE;
    }

    int getNumFilters() const override;
    AudioProcessorGraph::Node::Ptr getNode(int index) const override;
    AudioProcessorGraph::Node::Ptr getNodeForId(AudioProcessorGraph::NodeID uid) const override;

    //==============================================================================
    // Undoable operations - use these from UI code
    void addFilter(const PluginDescription* desc, double x, double y) override;
    void addFilter(AudioPluginInstance* plugin, double x, double y);
    void removeFilter(const AudioProcessorGraph::NodeID filterUID) override;
    bool addConnection(AudioProcessorGraph::NodeID sourceFilterUID, int sourceFilterChannel,
                       AudioProcessorGraph::NodeID destFilterUID, int destFilterChannel) override;
    void removeConnection(AudioProcessorGraph::NodeID sourceFilterUID, int sourceFilterChannel,
                          AudioProcessorGraph::NodeID destFilterUID, int destFilterChannel) override;

    //==============================================================================
    // Raw operations - used internally by UndoableActions (no undo tracking)
    AudioProcessorGraph::NodeID addFilterRaw(const PluginDescription* desc, double x, double y) override;
    void removeFilterRaw(const AudioProcessorGraph::NodeID filterUID) override;
    bool addConnectionRaw(AudioProcessorGraph::NodeID sourceFilterUID, int sourceFilterChannel,
                          AudioProcessorGraph::NodeID destFilterUID, int destFilterChannel) override;
    void removeConnectionRaw(AudioProcessorGraph::NodeID sourceFilterUID, int sourceFilterChannel,
                             AudioProcessorGraph::NodeID destFilterUID, int destFilterChannel) override;

    //==============================================================================
    // Plugin description helper for undo
    PluginDescription getPluginDescription(AudioProcessorGraph::NodeID nodeId) const;
    std::vector<AudioProcessorGraph::Connection> getConnectionsForNode(AudioProcessorGraph::NodeID nodeId) const;

    //==============================================================================
    void disconnectFilter(const AudioProcessorGraph::NodeID filterUID) override;
    void removeIllegalConnections();

    void setNodePosition(int nodeId, double x, double y) override;
    void getNodePosition(int nodeId, double& x, double& y) const override;

    //==============================================================================
    /// @brief JUCE 8: Connection API uses std::vector
    std::vector<AudioProcessorGraph::Connection> getConnections() const override;

    bool getConnectionBetween(AudioProcessorGraph::NodeID sourceFilterUID, int sourceFilterChannel,
                              AudioProcessorGraph::NodeID destFilterUID, int destFilterChannel) const override;

    bool canConnect(AudioProcessorGraph::NodeID sourceFilterUID, int sourceFilterChannel,
                    AudioProcessorGraph::NodeID destFilterUID, int destFilterChannel) const;

    // void clear(bool addAudioIO = true);
    void clear(bool addAudioIn = true, bool addMidiIn = true, bool addAudioOut = true, bool addVirtualMidiIn = true);

    /// Repositions the default input nodes (Audio Input, MIDI Input, Virtual MIDI Input)
    /// based on their actual heights. Called after adding nodes and when device changes.
    void repositionDefaultInputNodes();

    /// Returns the next available Y position for adding input nodes (below all default input nodes)
    float getNextInputNodeY() const;

    //==============================================================================

    XmlElement* createXml(const OscMappingManager& oscManager) const;
    void restoreFromXml(const XmlElement& xml, OscMappingManager& oscManager);

    //==============================================================================
    String getDocumentTitle();
    Result loadDocument(const File& file);
    Result saveDocument(const File& file);
    File getLastDocumentOpened();
    void setLastDocumentOpened(const File& file);

    /** The special channel index used to refer to a filter's midi channel.
     */
    static const int midiChannelNumber;

    //==============================================================================

  private:
    // friend class FilterGraphPlayer;
    // ReferenceCountedArray <FilterInGraph> filters;
    // OwnedArray <FilterConnection> connections;

    AudioProcessorGraph graph;
    AudioProcessorPlayer player;
    juce::UndoManager undoManager;

    // Audio safety protection (always active before output)
    SafetyLimiterProcessor* safetyLimiter = nullptr; // Owned by graph
    AudioProcessorGraph::NodeID safetyLimiterNodeId;

    // Crossfade mixer for glitch-free patch switching
    CrossfadeMixerProcessor* crossfadeMixer = nullptr; // Owned by graph
    AudioProcessorGraph::NodeID crossfadeMixerNodeId;

    uint32 lastUID;
    uint32 getNextUID() throw();

    /// Recreates hidden infrastructure processors (SafetyLimiter/CrossfadeMixer)
    /// after graph resets and refreshes cached raw pointers.
    void createInfrastructureNodes();

    void createNodeFromXml(const XmlElement& xml, OscMappingManager& oscManager);

    FilterGraph(const FilterGraph&);
    const FilterGraph& operator=(const FilterGraph&);
};

//==============================================================================
/**

*/
/*class FilterGraphPlayer   : public AudioIODeviceCallback,
                            public MidiInputCallback,
                            public ChangeListener

{
public:
    //==============================================================================
    FilterGraphPlayer (FilterGraph& graph);
    ~FilterGraphPlayer();

    //==============================================================================
    void setAudioDeviceManager (AudioDeviceManager* dm);
    AudioDeviceManager* getAudioDeviceManager() const throw()   { return
deviceManager; }

    //==============================================================================
    void audioDeviceIOCallback (const float** inputChannelData,
                                int totalNumInputChannels,
                                float** outputChannelData,
                                int totalNumOutputChannels,
                                int numSamples);
    void audioDeviceAboutToStart (double sampleRate, int numSamplesPerBlock);
    void audioDeviceStopped();

    void handleIncomingMidiMessage (MidiInput* source, const MidiMessage&
message);

    void changeListenerCallback (void*);

    //==============================================================================
    static int compareElements (FilterInGraph* const first, FilterInGraph* const
second) throw();

    const float** inputChannelData;
    int totalNumInputChannels;
    float** outputChannelData;
    int totalNumOutputChannels;
    MidiBuffer incomingMidi;

    MidiKeyboardState keyState;
    MidiMessageCollector messageCollector;

    //==============================================================================
    class PlayerAwareFilter
    {
    public:
        virtual void setPlayer (FilterGraphPlayer* newPlayer) = 0;
    };

private:
    FilterGraph& graph;
    CriticalSection processLock;
    double sampleRate;
    int blockSize;
    AudioDeviceManager* deviceManager;

    ReferenceCountedArray <FilterInGraph> filters;
    OwnedArray <FilterConnection> connections;

    void update();

    FilterGraphPlayer (const FilterGraphPlayer&);
    const FilterGraphPlayer& operator= (const FilterGraphPlayer&);
};
*/

#endif
