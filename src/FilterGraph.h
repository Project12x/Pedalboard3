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

#include "OscMappingManager.h"

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


      private : FilterGraph& owner;

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
class FilterGraph : public FileBasedDocument
{
  public:
    //==============================================================================
    FilterGraph();
    ~FilterGraph();

    //==============================================================================
    AudioProcessorGraph& getGraph() throw() { return graph; }

    /// Returns the UndoManager for undo/redo operations
    juce::UndoManager& getUndoManager() { return undoManager; }

    int getNumFilters() const;
    const AudioProcessorGraph::Node::Ptr getNode(const int index) const;
    const AudioProcessorGraph::Node::Ptr getNodeForId(const AudioProcessorGraph::NodeID uid) const;

    //==============================================================================
    // Undoable operations - use these from UI code
    void addFilter(const PluginDescription* desc, double x, double y);
    void addFilter(AudioPluginInstance* plugin, double x, double y);
    void removeFilter(const AudioProcessorGraph::NodeID filterUID);
    bool addConnection(AudioProcessorGraph::NodeID sourceFilterUID, int sourceFilterChannel,
                       AudioProcessorGraph::NodeID destFilterUID, int destFilterChannel);
    void removeConnection(AudioProcessorGraph::NodeID sourceFilterUID, int sourceFilterChannel,
                          AudioProcessorGraph::NodeID destFilterUID, int destFilterChannel);

    //==============================================================================
    // Raw operations - used internally by UndoableActions (no undo tracking)
    AudioProcessorGraph::NodeID addFilterRaw(const PluginDescription* desc, double x, double y);
    void removeFilterRaw(const AudioProcessorGraph::NodeID filterUID);
    bool addConnectionRaw(AudioProcessorGraph::NodeID sourceFilterUID, int sourceFilterChannel,
                          AudioProcessorGraph::NodeID destFilterUID, int destFilterChannel);
    void removeConnectionRaw(AudioProcessorGraph::NodeID sourceFilterUID, int sourceFilterChannel,
                             AudioProcessorGraph::NodeID destFilterUID, int destFilterChannel);

    //==============================================================================
    // Plugin description helper for undo
    PluginDescription getPluginDescription(AudioProcessorGraph::NodeID nodeId) const;
    std::vector<AudioProcessorGraph::Connection> getConnectionsForNode(AudioProcessorGraph::NodeID nodeId) const;

    //==============================================================================
    void disconnectFilter(const AudioProcessorGraph::NodeID filterUID);
    void removeIllegalConnections();

    void setNodePosition(const int nodeId, double x, double y);
    void getNodePosition(const int nodeId, double& x, double& y) const;

    //==============================================================================
    /// @brief JUCE 8: Connection API uses std::vector
    std::vector<AudioProcessorGraph::Connection> getConnections() const;

    const AudioProcessorGraph::Connection* getConnectionBetween(AudioProcessorGraph::NodeID sourceFilterUID,
                                                                int sourceFilterChannel,
                                                                AudioProcessorGraph::NodeID destFilterUID,
                                                                int destFilterChannel) const;

    bool canConnect(AudioProcessorGraph::NodeID sourceFilterUID, int sourceFilterChannel,
                    AudioProcessorGraph::NodeID destFilterUID, int destFilterChannel) const;

    // void clear(bool addAudioIO = true);
    void clear(bool addAudioIn = true, bool addMidiIn = true, bool addAudioOut = true);

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


      private :
      // friend class FilterGraphPlayer;
      // ReferenceCountedArray <FilterInGraph> filters;
      // OwnedArray <FilterConnection> connections;

      AudioProcessorGraph graph;
    AudioProcessorPlayer player;
    juce::UndoManager undoManager;

    uint32 lastUID;
    uint32 getNextUID() throw();

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
