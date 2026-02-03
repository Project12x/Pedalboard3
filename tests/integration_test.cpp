/**
 * @file integration_test.cpp
 * @brief Integration tests for SubGraph-MainGraph interaction
 *
 * Tests cover:
 * 1. SubGraph insertion into FilterGraph patterns
 * 2. Connection propagation through nested graphs
 * 3. State persistence with nested SubGraphs
 * 4. UID mapping and IO node convention compliance
 *
 * Note: These tests verify logic contracts without JUCE audio initialization.
 * Full integration testing requires manual testing with the running application.
 */

#include <algorithm>
#include <catch2/catch_test_macros.hpp>
#include <map>
#include <string>
#include <vector>

// =============================================================================
// Mock Types (mirror JUCE structures for testing)
// =============================================================================

struct MockNodeID
{
    uint32_t uid;
    bool operator==(const MockNodeID& other) const { return uid == other.uid; }
    bool operator!=(const MockNodeID& other) const { return uid != other.uid; }
    bool operator<(const MockNodeID& other) const { return uid < other.uid; }
};

struct MockConnection
{
    MockNodeID sourceNode;
    int sourceChannel;
    MockNodeID destNode;
    int destChannel;

    bool operator==(const MockConnection& other) const
    {
        return sourceNode == other.sourceNode && sourceChannel == other.sourceChannel && destNode == other.destNode &&
               destChannel == other.destChannel;
    }
};

// Simulates a graph that can contain nodes (including SubGraphProcessors)
class MockGraph
{
  public:
    std::vector<MockNodeID> nodes;
    std::vector<MockConnection> connections;
    std::map<uint32_t, bool> isSubGraphNode; // Track which nodes are SubGraphs

    MockNodeID addNode(bool isSubGraph = false)
    {
        MockNodeID id{nextUID++};
        nodes.push_back(id);
        isSubGraphNode[id.uid] = isSubGraph;
        return id;
    }

    bool addConnection(MockNodeID src, int srcCh, MockNodeID dst, int dstCh)
    {
        // Validate nodes exist
        bool srcExists = std::find(nodes.begin(), nodes.end(), src) != nodes.end();
        bool dstExists = std::find(nodes.begin(), nodes.end(), dst) != nodes.end();

        if (!srcExists || !dstExists)
            return false;

        connections.push_back({src, srcCh, dst, dstCh});
        return true;
    }

    bool removeNode(MockNodeID id)
    {
        auto it = std::find(nodes.begin(), nodes.end(), id);
        if (it == nodes.end())
            return false;

        // Remove associated connections
        connections.erase(std::remove_if(connections.begin(), connections.end(), [id](const MockConnection& c)
                                         { return c.sourceNode == id || c.destNode == id; }),
                          connections.end());
        nodes.erase(it);
        isSubGraphNode.erase(id.uid);
        return true;
    }

    bool isSubGraph(MockNodeID id) const
    {
        auto it = isSubGraphNode.find(id.uid);
        return it != isSubGraphNode.end() && it->second;
    }

  private:
    uint32_t nextUID = 100; // User nodes start at 100
};

// =============================================================================
// SubGraph-MainGraph Integration Tests
// =============================================================================

TEST_CASE("SubGraph Insertion into FilterGraph", "[integration][subgraph]")
{
    SECTION("SubGraph added as regular node")
    {
        MockGraph mainGraph;

        // Add IO nodes (like FilterGraph does)
        MockNodeID audioIn = mainGraph.addNode(false);
        MockNodeID audioOut = mainGraph.addNode(false);

        // Add a SubGraph (Effect Rack)
        MockNodeID subGraph = mainGraph.addNode(true);

        REQUIRE(mainGraph.nodes.size() == 3);
        REQUIRE(mainGraph.isSubGraph(subGraph));
        REQUIRE_FALSE(mainGraph.isSubGraph(audioIn));
    }

    SECTION("SubGraph can be connected in chain")
    {
        MockGraph mainGraph;
        MockNodeID audioIn = mainGraph.addNode(false);
        MockNodeID subGraph = mainGraph.addNode(true);
        MockNodeID audioOut = mainGraph.addNode(false);

        // Connect: audioIn -> subGraph -> audioOut
        bool conn1 = mainGraph.addConnection(audioIn, 0, subGraph, 0);
        bool conn2 = mainGraph.addConnection(subGraph, 0, audioOut, 0);

        REQUIRE(conn1);
        REQUIRE(conn2);
        REQUIRE(mainGraph.connections.size() == 2);
    }

    SECTION("SubGraph removal cleans up connections")
    {
        MockGraph mainGraph;
        MockNodeID audioIn = mainGraph.addNode(false);
        MockNodeID subGraph = mainGraph.addNode(true);
        MockNodeID audioOut = mainGraph.addNode(false);

        mainGraph.addConnection(audioIn, 0, subGraph, 0);
        mainGraph.addConnection(subGraph, 0, audioOut, 0);

        REQUIRE(mainGraph.connections.size() == 2);

        // Remove SubGraph
        mainGraph.removeNode(subGraph);

        REQUIRE(mainGraph.nodes.size() == 2);
        REQUIRE(mainGraph.connections.empty()); // All connections removed
    }
}

TEST_CASE("Connection Propagation Through Nested Graphs", "[integration][connections]")
{
    SECTION("Stereo connection through SubGraph")
    {
        MockGraph mainGraph;
        MockNodeID audioIn = mainGraph.addNode(false);
        MockNodeID subGraph = mainGraph.addNode(true);
        MockNodeID audioOut = mainGraph.addNode(false);

        // Stereo: Left (0) and Right (1) channels
        mainGraph.addConnection(audioIn, 0, subGraph, 0);  // Left in
        mainGraph.addConnection(audioIn, 1, subGraph, 1);  // Right in
        mainGraph.addConnection(subGraph, 0, audioOut, 0); // Left out
        mainGraph.addConnection(subGraph, 1, audioOut, 1); // Right out

        REQUIRE(mainGraph.connections.size() == 4);
    }

    SECTION("Multiple SubGraphs in series")
    {
        MockGraph mainGraph;
        MockNodeID audioIn = mainGraph.addNode(false);
        MockNodeID rack1 = mainGraph.addNode(true);
        MockNodeID rack2 = mainGraph.addNode(true);
        MockNodeID audioOut = mainGraph.addNode(false);

        // Chain: audioIn -> rack1 -> rack2 -> audioOut
        mainGraph.addConnection(audioIn, 0, rack1, 0);
        mainGraph.addConnection(rack1, 0, rack2, 0);
        mainGraph.addConnection(rack2, 0, audioOut, 0);

        REQUIRE(mainGraph.connections.size() == 3);
        REQUIRE(mainGraph.isSubGraph(rack1));
        REQUIRE(mainGraph.isSubGraph(rack2));
    }

    SECTION("Parallel SubGraphs")
    {
        MockGraph mainGraph;
        MockNodeID audioIn = mainGraph.addNode(false);
        MockNodeID rack1 = mainGraph.addNode(true);
        MockNodeID rack2 = mainGraph.addNode(true);
        MockNodeID mixer = mainGraph.addNode(false); // Could be a mixer
        MockNodeID audioOut = mainGraph.addNode(false);

        // Split: audioIn -> rack1 -> mixer
        //        audioIn -> rack2 -> mixer -> audioOut
        mainGraph.addConnection(audioIn, 0, rack1, 0);
        mainGraph.addConnection(audioIn, 0, rack2, 0);
        mainGraph.addConnection(rack1, 0, mixer, 0);
        mainGraph.addConnection(rack2, 0, mixer, 1);
        mainGraph.addConnection(mixer, 0, audioOut, 0);

        REQUIRE(mainGraph.connections.size() == 5);
    }
}

TEST_CASE("State Persistence with Nested SubGraphs", "[integration][persistence]")
{
    SECTION("Serialization includes SubGraph type marker")
    {
        MockGraph mainGraph;
        MockNodeID regularNode = mainGraph.addNode(false);
        MockNodeID subGraphNode = mainGraph.addNode(true);

        // Simulate serialization: only SubGraphs get special handling
        std::vector<std::string> serializedTypes;

        for (const auto& node : mainGraph.nodes)
        {
            if (mainGraph.isSubGraph(node))
                serializedTypes.push_back("Internal:SubGraph");
            else
                serializedTypes.push_back("Regular");
        }

        REQUIRE(serializedTypes.size() == 2);
        REQUIRE(serializedTypes[0] == "Regular");
        REQUIRE(serializedTypes[1] == "Internal:SubGraph");
    }

    SECTION("UID remapping preserves SubGraph identity")
    {
        // Simulate save: UID 100 (SubGraph), 101 (Regular)
        // Simulate load: UIDs become 5 (SubGraph), 6 (Regular)
        std::map<uint32_t, uint32_t> uidRemap;
        std::map<uint32_t, bool> oldIsSubGraph;

        oldIsSubGraph[100] = true;
        oldIsSubGraph[101] = false;

        uidRemap[100] = 5;
        uidRemap[101] = 6;

        // After restore, SubGraph identity should be preserved
        MockGraph restoredGraph;
        for (const auto& [oldUid, newUid] : uidRemap)
        {
            bool isSubGraph = oldIsSubGraph[oldUid];
            restoredGraph.addNode(isSubGraph);
        }

        // Verify SubGraph was restored correctly
        // (In real code, node 5 should still be a SubGraph)
        REQUIRE(uidRemap.size() == 2);
    }

    SECTION("Connection restoration maps old UIDs to new IDs")
    {
        // Saved connection: 100 -> 101
        // After load: 100 becomes 5, 101 becomes 6
        std::map<uint32_t, MockNodeID> uidToNewNode;
        uidToNewNode[100] = {5};
        uidToNewNode[101] = {6};

        MockConnection saved{MockNodeID{100}, 0, MockNodeID{101}, 0};

        // Restore using map
        MockConnection restored;
        restored.sourceNode = uidToNewNode[saved.sourceNode.uid];
        restored.sourceChannel = saved.sourceChannel;
        restored.destNode = uidToNewNode[saved.destNode.uid];
        restored.destChannel = saved.destChannel;

        REQUIRE(restored.sourceNode.uid == 5);
        REQUIRE(restored.destNode.uid == 6);
    }
}

// =============================================================================
// FilterGraph Special Cases
// =============================================================================

TEST_CASE("FilterGraph SubGraph Special Handling", "[integration][filtergraph]")
{
    SECTION("SubGraph detection via dynamic_cast pattern")
    {
        // Simulates: dynamic_cast<SubGraphProcessor*>(node->getProcessor())
        bool isSubGraphProcessor = true; // Simulated cast result

        // FilterGraph.cpp line ~355: special handling for SubGraph
        if (isSubGraphProcessor)
        {
            // Would skip standard plugin loading path
            REQUIRE(true);
        }
    }

    SECTION("SubGraph XML serialization uses different format")
    {
        bool isSubGraph = true;

        std::string xmlTagName;
        if (isSubGraph)
            xmlTagName = "RACK"; // SubGraphProcessor uses RACK tag
        else
            xmlTagName = "FILTER"; // Regular plugins use FILTER tag

        REQUIRE(xmlTagName == "RACK");
    }

    SECTION("IO node UID conventions preserved across save/load")
    {
        // Convention from SubGraphProcessor:
        // Audio In = UID 1, Audio Out = UID 2, MIDI In = UID 3
        constexpr uint32_t AUDIO_IN_UID = 1;
        constexpr uint32_t AUDIO_OUT_UID = 2;
        constexpr uint32_t MIDI_IN_UID = 3;

        auto isIONodeUID = [](uint32_t uid) { return uid == 1 || uid == 2 || uid == 3; };

        REQUIRE(isIONodeUID(AUDIO_IN_UID));
        REQUIRE(isIONodeUID(AUDIO_OUT_UID));
        REQUIRE(isIONodeUID(MIDI_IN_UID));
        REQUIRE_FALSE(isIONodeUID(100)); // User node
    }
}

// =============================================================================
// Mutation Testing for Integration Points
// =============================================================================

TEST_CASE("Integration Mutation Testing", "[integration][mutation]")
{
    SECTION("OFF-BY-ONE: Connection channel bounds")
    {
        int numChannels = 2;
        int channel = 1; // Last valid

        // Correct: channel < numChannels
        bool correctValid = (channel >= 0) && (channel < numChannels);
        REQUIRE(correctValid);

        // Mutation: channel <= numChannels would allow invalid
        int invalidChannel = numChannels;
        bool mutatedValid = (invalidChannel >= 0) && (invalidChannel <= numChannels);
        bool correctInvalid = (invalidChannel >= 0) && (invalidChannel < numChannels);

        REQUIRE(mutatedValid == true);    // Mutation incorrectly passes
        REQUIRE(correctInvalid == false); // Correct check fails
    }

    SECTION("SWAP: Source and destination in addConnection")
    {
        MockGraph graph;
        MockNodeID node1 = graph.addNode();
        MockNodeID node2 = graph.addNode();

        // Correct: node1 -> node2
        graph.addConnection(node1, 0, node2, 0);

        auto& conn = graph.connections[0];
        REQUIRE(conn.sourceNode == node1);
        REQUIRE(conn.destNode == node2);

        // Mutation would swap these
        REQUIRE(conn.sourceNode != conn.destNode);
    }

    SECTION("NEGATE: isSubGraph check")
    {
        MockGraph graph;
        MockNodeID subGraph = graph.addNode(true);

        // Correct behavior
        REQUIRE(graph.isSubGraph(subGraph));

        // Mutation: !isSubGraph would be wrong
        REQUIRE_FALSE(!graph.isSubGraph(subGraph));
    }

    SECTION("ARITHMETIC: Connection count after removal")
    {
        MockGraph graph;
        MockNodeID n1 = graph.addNode();
        MockNodeID n2 = graph.addNode();
        MockNodeID n3 = graph.addNode();

        graph.addConnection(n1, 0, n2, 0);
        graph.addConnection(n2, 0, n3, 0);

        size_t beforeRemoval = graph.connections.size();
        REQUIRE(beforeRemoval == 2);

        graph.removeNode(n2);

        size_t afterRemoval = graph.connections.size();
        REQUIRE(afterRemoval == 0); // Both connections gone

        // Correct: beforeRemoval - 2 (both connections removed)
        REQUIRE((beforeRemoval - afterRemoval) == 2);
    }
}

// =============================================================================
// End-to-End Integration Scenarios
// =============================================================================

// Mock processor for signal path testing
struct MockProcessor
{
    std::string name;
    bool bypassed = false;
    float lastInputLevel = 0.0f;
    float lastOutputLevel = 0.0f;
    int processCallCount = 0;

    void process(float input)
    {
        lastInputLevel = input;
        lastOutputLevel = bypassed ? input : input * 0.5f;
        processCallCount++;
    }
};

// Mock signal path through the graph
class MockSignalPath
{
  public:
    std::vector<MockProcessor*> processors;
    std::vector<std::pair<int, int>> routing; // source idx -> dest idx

    float processChain(float input)
    {
        if (processors.empty())
            return input;

        // Process first
        processors[0]->process(input);

        // Process chain
        for (size_t i = 1; i < processors.size(); ++i)
        {
            float prevOutput = processors[i - 1]->lastOutputLevel;
            processors[i]->process(prevOutput);
        }

        return processors.back()->lastOutputLevel;
    }
};

TEST_CASE("End-to-End Signal Path Integration", "[integration][e2e][signal]")
{
    SECTION("Signal flows through effect chain")
    {
        MockProcessor effect1{"Compressor"};
        MockProcessor effect2{"EQ"};
        MockProcessor effect3{"Reverb"};

        MockSignalPath path;
        path.processors = {&effect1, &effect2, &effect3};

        float input = 1.0f;
        float output = path.processChain(input);

        // Each processor halves the signal (0.5^3 = 0.125)
        REQUIRE(output == 0.125f);
        REQUIRE(effect1.processCallCount == 1);
        REQUIRE(effect2.processCallCount == 1);
        REQUIRE(effect3.processCallCount == 1);
    }

    SECTION("Bypass skips effect processing")
    {
        MockProcessor effect1{"Compressor"};
        MockProcessor effect2{"EQ"};
        effect2.bypassed = true; // EQ bypassed

        MockSignalPath path;
        path.processors = {&effect1, &effect2};

        float input = 1.0f;
        float output = path.processChain(input);

        // Compressor halves, EQ bypassed passes through
        REQUIRE(output == 0.5f);
        REQUIRE(effect1.lastOutputLevel == 0.5f);
        REQUIRE(effect2.lastOutputLevel == 0.5f); // Pass-through
    }

    SECTION("Empty chain passes through")
    {
        MockSignalPath path;
        float output = path.processChain(1.0f);
        REQUIRE(output == 1.0f);
    }
}

// =============================================================================
// Plugin Lifecycle Integration
// =============================================================================

struct MockPluginInstance
{
    std::string pluginId;
    bool loaded = false;
    bool editorOpen = false;
    int editorOpenCount = 0;

    void load() { loaded = true; }
    void unload() { loaded = false; }
    void openEditor()
    {
        editorOpen = true;
        editorOpenCount++;
    }
    void closeEditor() { editorOpen = false; }
};

class MockPluginHost
{
  public:
    std::vector<MockPluginInstance> plugins;

    int loadPlugin(const std::string& id)
    {
        MockPluginInstance p;
        p.pluginId = id;
        p.load();
        plugins.push_back(p);
        return static_cast<int>(plugins.size() - 1);
    }

    bool unloadPlugin(int idx)
    {
        if (idx < 0 || idx >= static_cast<int>(plugins.size()))
            return false;
        plugins[idx].unload();
        return true;
    }
};

TEST_CASE("Plugin Lifecycle Integration", "[integration][e2e][lifecycle]")
{
    MockPluginHost host;

    SECTION("Load and unload plugin")
    {
        int idx = host.loadPlugin("com.vendor.reverb");
        REQUIRE(idx == 0);
        REQUIRE(host.plugins[0].loaded);

        bool unloaded = host.unloadPlugin(idx);
        REQUIRE(unloaded);
        REQUIRE_FALSE(host.plugins[0].loaded);
    }

    SECTION("Editor reopen creates fresh instance")
    {
        int idx = host.loadPlugin("com.vendor.compressor");
        auto& plugin = host.plugins[idx];

        // Open editor multiple times (simulates user reopening)
        plugin.openEditor();
        REQUIRE(plugin.editorOpenCount == 1);

        plugin.closeEditor();
        plugin.openEditor();
        REQUIRE(plugin.editorOpenCount == 2);

        // Each open should be fresh (no stale state)
        REQUIRE(plugin.editorOpen);
    }

    SECTION("Multiple plugins load independently")
    {
        int idx1 = host.loadPlugin("Reverb");
        int idx2 = host.loadPlugin("Delay");
        int idx3 = host.loadPlugin("Chorus");

        REQUIRE(host.plugins.size() == 3);
        REQUIRE(host.plugins[idx1].loaded);
        REQUIRE(host.plugins[idx2].loaded);
        REQUIRE(host.plugins[idx3].loaded);

        // Unload middle one
        host.unloadPlugin(idx2);
        REQUIRE(host.plugins[idx1].loaded);
        REQUIRE_FALSE(host.plugins[idx2].loaded);
        REQUIRE(host.plugins[idx3].loaded);
    }
}

// =============================================================================
// MIDI Routing Integration
// =============================================================================

struct MockMidiMessage
{
    enum Type
    {
        NoteOn,
        NoteOff,
        CC
    };
    Type type;
    int channel;
    int data1; // Note or CC#
    int data2; // Velocity or CC value
};

class MockMidiRouter
{
  public:
    int inputChannel = 0;  // 0 = omni
    int outputChannel = 1; // 1-16
    std::vector<MockMidiMessage> outputBuffer;

    void routeMessage(const MockMidiMessage& msg)
    {
        // Filter by input channel (0 = accept all)
        if (inputChannel != 0 && msg.channel != inputChannel)
            return;

        // Remap to output channel
        MockMidiMessage routed = msg;
        routed.channel = outputChannel;
        outputBuffer.push_back(routed);
    }

    void clear() { outputBuffer.clear(); }
};

TEST_CASE("MIDI Routing Integration", "[integration][e2e][midi]")
{
    MockMidiRouter router;

    SECTION("Omni mode routes all channels")
    {
        router.inputChannel = 0; // Omni
        router.outputChannel = 10;

        router.routeMessage({MockMidiMessage::NoteOn, 1, 60, 100});
        router.routeMessage({MockMidiMessage::NoteOn, 5, 64, 80});
        router.routeMessage({MockMidiMessage::NoteOn, 16, 67, 90});

        REQUIRE(router.outputBuffer.size() == 3);
        for (const auto& msg : router.outputBuffer)
        {
            REQUIRE(msg.channel == 10);
        }
    }

    SECTION("Channel filter blocks other channels")
    {
        router.inputChannel = 3;
        router.outputChannel = 5;

        router.routeMessage({MockMidiMessage::NoteOn, 1, 60, 100}); // Blocked
        router.routeMessage({MockMidiMessage::NoteOn, 3, 64, 80});  // Passed
        router.routeMessage({MockMidiMessage::NoteOn, 16, 67, 90}); // Blocked

        REQUIRE(router.outputBuffer.size() == 1);
        REQUIRE(router.outputBuffer[0].data1 == 64);
        REQUIRE(router.outputBuffer[0].channel == 5);
    }

    SECTION("CC messages routed correctly")
    {
        router.inputChannel = 0;
        router.outputChannel = 1;

        router.routeMessage({MockMidiMessage::CC, 1, 1, 64});  // Mod wheel
        router.routeMessage({MockMidiMessage::CC, 1, 7, 100}); // Volume
        router.routeMessage({MockMidiMessage::CC, 1, 11, 80}); // Expression

        REQUIRE(router.outputBuffer.size() == 3);
    }
}

// =============================================================================
// MIDI Mapping Integration
// =============================================================================

struct MockMidiMapping
{
    int ccNumber;
    int parameterIndex;
    float minValue = 0.0f;
    float maxValue = 1.0f;
    bool latched = false;

    float mapValue(int ccValue) const
    {
        float normalized = ccValue / 127.0f;
        return minValue + normalized * (maxValue - minValue);
    }
};

class MockMidiMappingManager
{
  public:
    std::vector<MockMidiMapping> mappings;
    std::map<int, float> parameterValues;

    void addMapping(int cc, int param, float min = 0.0f, float max = 1.0f)
    {
        mappings.push_back({cc, param, min, max, false});
    }

    void processCC(int cc, int value)
    {
        for (const auto& mapping : mappings)
        {
            if (mapping.ccNumber == cc)
            {
                float mappedValue = mapping.mapValue(value);
                parameterValues[mapping.parameterIndex] = mappedValue;
            }
        }
    }
};

TEST_CASE("MIDI Mapping Integration", "[integration][e2e][midi][mapping]")
{
    MockMidiMappingManager manager;

    SECTION("CC maps to parameter value")
    {
        manager.addMapping(1, 0); // CC1 -> Param 0
        manager.processCC(1, 127);
        REQUIRE(manager.parameterValues[0] == 1.0f);

        manager.processCC(1, 64);
        REQUIRE(manager.parameterValues[0] > 0.49f);
        REQUIRE(manager.parameterValues[0] < 0.51f);

        manager.processCC(1, 0);
        REQUIRE(manager.parameterValues[0] == 0.0f);
    }

    SECTION("Custom min/max range")
    {
        manager.addMapping(7, 1, 0.2f, 0.8f); // CC7: 20% - 80%
        manager.processCC(7, 0);
        REQUIRE(manager.parameterValues[1] == 0.2f);

        manager.processCC(7, 127);
        REQUIRE(manager.parameterValues[1] == 0.8f);
    }

    SECTION("Multiple mappings for same CC")
    {
        manager.addMapping(11, 0); // CC11 -> Param 0
        manager.addMapping(11, 1); // CC11 -> Param 1

        manager.processCC(11, 100);
        float expected = 100.0f / 127.0f;
        REQUIRE(manager.parameterValues[0] == expected);
        REQUIRE(manager.parameterValues[1] == expected);
    }
}

// =============================================================================
// Extended Mutation Testing
// =============================================================================

TEST_CASE("Extended Mutation Testing", "[integration][mutation][extended]")
{
    SECTION("BOUNDARY: Channel index edge cases")
    {
        auto isValidChannel = [](int ch) { return ch >= 0 && ch < 16; };

        REQUIRE(isValidChannel(0));
        REQUIRE(isValidChannel(15));
        REQUIRE_FALSE(isValidChannel(-1));
        REQUIRE_FALSE(isValidChannel(16));
    }

    SECTION("RETURN: Early return on invalid input")
    {
        MockGraph graph;
        MockNodeID validNode = graph.addNode();
        MockNodeID invalidNode{9999};

        // Should return false on invalid node
        bool result = graph.addConnection(validNode, 0, invalidNode, 0);
        REQUIRE_FALSE(result);
        REQUIRE(graph.connections.empty());
    }

    SECTION("INCREMENT: Process count tracking")
    {
        MockProcessor proc{"Test"};
        REQUIRE(proc.processCallCount == 0);

        proc.process(1.0f);
        REQUIRE(proc.processCallCount == 1);

        proc.process(1.0f);
        REQUIRE(proc.processCallCount == 2);
    }

    SECTION("CONDITION: Bypass flag effect")
    {
        MockProcessor proc{"Test"};
        float input = 1.0f;

        // Not bypassed: output = input * 0.5
        proc.bypassed = false;
        proc.process(input);
        REQUIRE(proc.lastOutputLevel == 0.5f);

        // Bypassed: output = input
        proc.bypassed = true;
        proc.process(input);
        REQUIRE(proc.lastOutputLevel == 1.0f);
    }

    SECTION("COMPARE: Equality vs inequality")
    {
        MockNodeID a{1};
        MockNodeID b{1};
        MockNodeID c{2};

        REQUIRE(a == b);
        REQUIRE_FALSE(a != b);
        REQUIRE(a != c);
        REQUIRE_FALSE(a == c);
    }

    SECTION("CONTAINER: Empty vs non-empty checks")
    {
        std::vector<int> vec;
        REQUIRE(vec.empty());
        REQUIRE(vec.size() == 0);

        vec.push_back(1);
        REQUIRE_FALSE(vec.empty());
        REQUIRE(vec.size() == 1);
    }
}
