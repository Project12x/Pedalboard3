/**
 * @file filtergraph_test.cpp
 * @brief Unit tests for FilterGraph/IFilterGraph interface operations
 *
 * Tests cover:
 * 1. Node management (add/remove/query)
 * 2. Connection management (add/remove/query)
 * 3. Position management
 * 4. Infrastructure node detection
 * 5. Boundary conditions and mutation testing
 *
 * Note: These tests use mock objects to verify logic without JUCE audio initialization.
 */

#include <algorithm>
#include <catch2/catch_test_macros.hpp>
#include <map>
#include <string>
#include <vector>

// =============================================================================
// Mock Types (mirrors IFilterGraph interface)
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
    MockNodeID sourceId;
    int sourceChannel;
    MockNodeID destId;
    int destChannel;

    bool operator==(const MockConnection& other) const
    {
        return sourceId == other.sourceId && sourceChannel == other.sourceChannel && destId == other.destId &&
               destChannel == other.destChannel;
    }
};

// Mock graph simulating IFilterGraph
class MockFilterGraph
{
  public:
    struct NodeInfo
    {
        MockNodeID id;
        double x, y;
        std::string pluginId;
        bool isInfrastructure;
    };

    std::vector<NodeInfo> nodes;
    std::vector<MockConnection> connections;
    uint32_t nextNodeId = 100;

    // Infrastructure nodes (always present)
    MockNodeID audioInputNode{1};
    MockNodeID audioOutputNode{2};
    MockNodeID midiInputNode{3};
    MockNodeID safetyLimiterNode{4};
    MockNodeID crossfadeMixerNode{5};

    MockFilterGraph()
    {
        // Add infrastructure nodes
        nodes.push_back({audioInputNode, 50.0, 100.0, "AudioInput", true});
        nodes.push_back({audioOutputNode, 500.0, 100.0, "AudioOutput", true});
        nodes.push_back({midiInputNode, 50.0, 200.0, "MidiInput", true});
        nodes.push_back({safetyLimiterNode, 400.0, 100.0, "SafetyLimiter", true});
        nodes.push_back({crossfadeMixerNode, 300.0, 100.0, "CrossfadeMixer", true});

        // Default passthrough connection
        connections.push_back({audioInputNode, 0, audioOutputNode, 0});
        connections.push_back({audioInputNode, 1, audioOutputNode, 1});
    }

    // ==========================================================================
    // Node Management
    // ==========================================================================

    int getNumFilters() const { return static_cast<int>(nodes.size()); }

    MockNodeID addFilter(const std::string& pluginId, double x, double y)
    {
        MockNodeID id{nextNodeId++};
        nodes.push_back({id, x, y, pluginId, false});
        return id;
    }

    bool removeFilter(MockNodeID id)
    {
        // Cannot remove infrastructure
        if (isHiddenInfrastructureNode(id))
            return false;

        // Remove all connections first
        connections.erase(std::remove_if(connections.begin(), connections.end(),
                                         [&](const MockConnection& c) { return c.sourceId == id || c.destId == id; }),
                          connections.end());

        // Remove node
        auto it = std::remove_if(nodes.begin(), nodes.end(), [&](const NodeInfo& n) { return n.id == id; });
        if (it != nodes.end())
        {
            nodes.erase(it, nodes.end());
            return true;
        }
        return false;
    }

    NodeInfo* getNode(MockNodeID id)
    {
        for (auto& n : nodes)
            if (n.id == id)
                return &n;
        return nullptr;
    }

    const NodeInfo* getNode(MockNodeID id) const
    {
        for (const auto& n : nodes)
            if (n.id == id)
                return &n;
        return nullptr;
    }

    bool nodeExists(MockNodeID id) const { return getNode(id) != nullptr; }

    // ==========================================================================
    // Connection Management
    // ==========================================================================

    bool addConnection(MockNodeID srcId, int srcChannel, MockNodeID dstId, int dstChannel)
    {
        // Validate nodes exist
        if (!nodeExists(srcId) || !nodeExists(dstId))
            return false;

        // Prevent self-connection
        if (srcId == dstId)
            return false;

        // Validate channels (assume stereo max)
        if (srcChannel < 0 || dstChannel < 0)
            return false;

        // Check for duplicate
        MockConnection newConn{srcId, srcChannel, dstId, dstChannel};
        for (const auto& c : connections)
            if (c == newConn)
                return false;

        connections.push_back(newConn);
        return true;
    }

    bool removeConnection(MockNodeID srcId, int srcChannel, MockNodeID dstId, int dstChannel)
    {
        MockConnection target{srcId, srcChannel, dstId, dstChannel};
        auto it = std::remove_if(connections.begin(), connections.end(),
                                 [&](const MockConnection& c) { return c == target; });
        if (it != connections.end())
        {
            connections.erase(it, connections.end());
            return true;
        }
        return false;
    }

    void disconnectFilter(MockNodeID id)
    {
        connections.erase(std::remove_if(connections.begin(), connections.end(),
                                         [&](const MockConnection& c) { return c.sourceId == id || c.destId == id; }),
                          connections.end());
    }

    const MockConnection* getConnectionBetween(MockNodeID srcId, int srcChannel, MockNodeID dstId, int dstChannel) const
    {
        MockConnection target{srcId, srcChannel, dstId, dstChannel};
        for (const auto& c : connections)
            if (c == target)
                return &c;
        return nullptr;
    }

    int getNumConnections() const { return static_cast<int>(connections.size()); }

    // ==========================================================================
    // Position Management
    // ==========================================================================

    void setNodePosition(MockNodeID id, double x, double y)
    {
        if (auto* node = getNode(id))
        {
            node->x = x;
            node->y = y;
        }
    }

    bool getNodePosition(MockNodeID id, double& x, double& y) const
    {
        if (const auto* node = getNode(id))
        {
            x = node->x;
            y = node->y;
            return true;
        }
        return false;
    }

    // ==========================================================================
    // Infrastructure Detection
    // ==========================================================================

    bool isHiddenInfrastructureNode(MockNodeID id) const
    {
        return id == audioInputNode || id == audioOutputNode || id == midiInputNode || id == safetyLimiterNode ||
               id == crossfadeMixerNode;
    }
};

// =============================================================================
// Node Management Tests
// =============================================================================

TEST_CASE("FilterGraph Node Management", "[filtergraph][nodes]")
{
    MockFilterGraph graph;

    SECTION("Initial state has infrastructure nodes")
    {
        REQUIRE(graph.getNumFilters() == 5); // Audio in/out, MIDI in, SafetyLimiter, CrossfadeMixer
        REQUIRE(graph.nodeExists(graph.audioInputNode));
        REQUIRE(graph.nodeExists(graph.audioOutputNode));
        REQUIRE(graph.nodeExists(graph.midiInputNode));
    }

    SECTION("Add user filter")
    {
        int beforeCount = graph.getNumFilters();
        MockNodeID newNode = graph.addFilter("com.vendor.plugin", 200.0, 150.0);

        REQUIRE(graph.getNumFilters() == beforeCount + 1);
        REQUIRE(graph.nodeExists(newNode));

        auto* node = graph.getNode(newNode);
        REQUIRE(node != nullptr);
        REQUIRE(node->pluginId == "com.vendor.plugin");
    }

    SECTION("Remove user filter")
    {
        MockNodeID plugin = graph.addFilter("TestPlugin", 100.0, 100.0);
        REQUIRE(graph.nodeExists(plugin));

        bool removed = graph.removeFilter(plugin);
        REQUIRE(removed);
        REQUIRE_FALSE(graph.nodeExists(plugin));
    }

    SECTION("Cannot remove infrastructure nodes")
    {
        bool removedInput = graph.removeFilter(graph.audioInputNode);
        bool removedOutput = graph.removeFilter(graph.audioOutputNode);
        bool removedMidi = graph.removeFilter(graph.midiInputNode);

        REQUIRE_FALSE(removedInput);
        REQUIRE_FALSE(removedOutput);
        REQUIRE_FALSE(removedMidi);

        // All still exist
        REQUIRE(graph.nodeExists(graph.audioInputNode));
        REQUIRE(graph.nodeExists(graph.audioOutputNode));
        REQUIRE(graph.nodeExists(graph.midiInputNode));
    }

    SECTION("Remove non-existent filter is safe")
    {
        MockNodeID ghost{9999};
        bool removed = graph.removeFilter(ghost);
        REQUIRE_FALSE(removed);
    }

    SECTION("Unique node IDs")
    {
        MockNodeID n1 = graph.addFilter("P1", 0, 0);
        MockNodeID n2 = graph.addFilter("P2", 0, 0);
        MockNodeID n3 = graph.addFilter("P3", 0, 0);

        REQUIRE(n1.uid != n2.uid);
        REQUIRE(n2.uid != n3.uid);
        REQUIRE(n1.uid != n3.uid);
    }
}

// =============================================================================
// Connection Management Tests
// =============================================================================

TEST_CASE("FilterGraph Connection Management", "[filtergraph][connections]")
{
    MockFilterGraph graph;

    SECTION("Initial passthrough connections exist")
    {
        // Default stereo passthrough: AudioIn -> AudioOut
        REQUIRE(graph.getConnectionBetween(graph.audioInputNode, 0, graph.audioOutputNode, 0) != nullptr);
        REQUIRE(graph.getConnectionBetween(graph.audioInputNode, 1, graph.audioOutputNode, 1) != nullptr);
    }

    SECTION("Add connection between plugins")
    {
        MockNodeID p1 = graph.addFilter("P1", 100, 100);
        MockNodeID p2 = graph.addFilter("P2", 200, 100);

        bool added = graph.addConnection(p1, 0, p2, 0);
        REQUIRE(added);
        REQUIRE(graph.getConnectionBetween(p1, 0, p2, 0) != nullptr);
    }

    SECTION("Connect plugin into audio chain")
    {
        MockNodeID plugin = graph.addFilter("Effect", 150, 100);

        // Remove default passthrough
        graph.removeConnection(graph.audioInputNode, 0, graph.audioOutputNode, 0);

        // Insert plugin: AudioIn -> Plugin -> AudioOut
        REQUIRE(graph.addConnection(graph.audioInputNode, 0, plugin, 0));
        REQUIRE(graph.addConnection(plugin, 0, graph.audioOutputNode, 0));

        REQUIRE(graph.getConnectionBetween(graph.audioInputNode, 0, plugin, 0) != nullptr);
        REQUIRE(graph.getConnectionBetween(plugin, 0, graph.audioOutputNode, 0) != nullptr);
    }

    SECTION("Self-connection rejected")
    {
        MockNodeID plugin = graph.addFilter("P1", 100, 100);
        bool added = graph.addConnection(plugin, 0, plugin, 1);
        REQUIRE_FALSE(added);
    }

    SECTION("Duplicate connection rejected")
    {
        MockNodeID p1 = graph.addFilter("P1", 100, 100);
        MockNodeID p2 = graph.addFilter("P2", 200, 100);

        graph.addConnection(p1, 0, p2, 0);
        bool duplicate = graph.addConnection(p1, 0, p2, 0);
        REQUIRE_FALSE(duplicate);
    }

    SECTION("Connection to non-existent node fails")
    {
        MockNodeID plugin = graph.addFilter("P1", 100, 100);
        MockNodeID ghost{9999};

        bool toGhost = graph.addConnection(plugin, 0, ghost, 0);
        bool fromGhost = graph.addConnection(ghost, 0, plugin, 0);

        REQUIRE_FALSE(toGhost);
        REQUIRE_FALSE(fromGhost);
    }

    SECTION("Negative channel rejected")
    {
        MockNodeID p1 = graph.addFilter("P1", 100, 100);
        MockNodeID p2 = graph.addFilter("P2", 200, 100);

        bool badSrc = graph.addConnection(p1, -1, p2, 0);
        bool badDst = graph.addConnection(p1, 0, p2, -1);

        REQUIRE_FALSE(badSrc);
        REQUIRE_FALSE(badDst);
    }

    SECTION("Remove connection")
    {
        MockNodeID p1 = graph.addFilter("P1", 100, 100);
        MockNodeID p2 = graph.addFilter("P2", 200, 100);

        graph.addConnection(p1, 0, p2, 0);
        REQUIRE(graph.getConnectionBetween(p1, 0, p2, 0) != nullptr);

        bool removed = graph.removeConnection(p1, 0, p2, 0);
        REQUIRE(removed);
        REQUIRE(graph.getConnectionBetween(p1, 0, p2, 0) == nullptr);
    }

    SECTION("Disconnect filter removes all connections")
    {
        MockNodeID plugin = graph.addFilter("Effect", 150, 100);

        graph.addConnection(graph.audioInputNode, 0, plugin, 0);
        graph.addConnection(graph.audioInputNode, 1, plugin, 1);
        graph.addConnection(plugin, 0, graph.audioOutputNode, 0);
        graph.addConnection(plugin, 1, graph.audioOutputNode, 1);

        int beforeCount = graph.getNumConnections();
        graph.disconnectFilter(plugin);

        // Plugin connections should be gone
        REQUIRE(graph.getConnectionBetween(graph.audioInputNode, 0, plugin, 0) == nullptr);
        REQUIRE(graph.getConnectionBetween(plugin, 0, graph.audioOutputNode, 0) == nullptr);
        REQUIRE(graph.getNumConnections() < beforeCount);
    }

    SECTION("Remove filter also removes connections")
    {
        MockNodeID plugin = graph.addFilter("Effect", 150, 100);

        graph.addConnection(graph.audioInputNode, 0, plugin, 0);
        graph.addConnection(plugin, 0, graph.audioOutputNode, 0);

        graph.removeFilter(plugin);

        // Connections should be gone
        REQUIRE(graph.getConnectionBetween(graph.audioInputNode, 0, plugin, 0) == nullptr);
        REQUIRE(graph.getConnectionBetween(plugin, 0, graph.audioOutputNode, 0) == nullptr);
    }
}

// =============================================================================
// Position Management Tests
// =============================================================================

TEST_CASE("FilterGraph Position Management", "[filtergraph][position]")
{
    MockFilterGraph graph;

    SECTION("Get initial node position")
    {
        double x, y;
        bool found = graph.getNodePosition(graph.audioInputNode, x, y);

        REQUIRE(found);
        REQUIRE(x == 50.0);
        REQUIRE(y == 100.0);
    }

    SECTION("Set node position")
    {
        MockNodeID plugin = graph.addFilter("P1", 0, 0);

        graph.setNodePosition(plugin, 300.0, 250.0);

        double x, y;
        graph.getNodePosition(plugin, x, y);
        REQUIRE(x == 300.0);
        REQUIRE(y == 250.0);
    }

    SECTION("Get position of non-existent node returns false")
    {
        MockNodeID ghost{9999};
        double x = -1, y = -1;

        bool found = graph.getNodePosition(ghost, x, y);
        REQUIRE_FALSE(found);
    }
}

// =============================================================================
// Infrastructure Detection Tests
// =============================================================================

TEST_CASE("FilterGraph Infrastructure Detection", "[filtergraph][infrastructure]")
{
    MockFilterGraph graph;

    SECTION("IO nodes are infrastructure")
    {
        REQUIRE(graph.isHiddenInfrastructureNode(graph.audioInputNode));
        REQUIRE(graph.isHiddenInfrastructureNode(graph.audioOutputNode));
        REQUIRE(graph.isHiddenInfrastructureNode(graph.midiInputNode));
    }

    SECTION("Internal processors are infrastructure")
    {
        REQUIRE(graph.isHiddenInfrastructureNode(graph.safetyLimiterNode));
        REQUIRE(graph.isHiddenInfrastructureNode(graph.crossfadeMixerNode));
    }

    SECTION("User plugins are not infrastructure")
    {
        MockNodeID plugin = graph.addFilter("UserPlugin", 100, 100);
        REQUIRE_FALSE(graph.isHiddenInfrastructureNode(plugin));
    }
}

// =============================================================================
// Mutation Testing
// =============================================================================

TEST_CASE("FilterGraph Mutation Testing", "[filtergraph][mutation]")
{
    SECTION("OFF-BY-ONE: Node count after add/remove")
    {
        MockFilterGraph graph;
        int initialCount = graph.getNumFilters();

        MockNodeID p1 = graph.addFilter("P1", 0, 0);
        REQUIRE(graph.getNumFilters() == initialCount + 1);

        graph.removeFilter(p1);
        REQUIRE(graph.getNumFilters() == initialCount);
    }

    SECTION("NEGATE: Infrastructure check inversion")
    {
        MockFilterGraph graph;

        // If negated, user plugins would appear as infrastructure
        MockNodeID plugin = graph.addFilter("Test", 0, 0);

        bool isInfra = graph.isHiddenInfrastructureNode(plugin);
        bool isIO = graph.isHiddenInfrastructureNode(graph.audioInputNode);

        REQUIRE_FALSE(isInfra);
        REQUIRE(isIO);

        // Negation would flip these
        REQUIRE(isInfra != isIO);
    }

    SECTION("SWAP: Source/dest in connection lookup")
    {
        MockFilterGraph graph;
        MockNodeID p1 = graph.addFilter("P1", 0, 0);
        MockNodeID p2 = graph.addFilter("P2", 100, 0);

        graph.addConnection(p1, 0, p2, 0);

        // Correct direction
        REQUIRE(graph.getConnectionBetween(p1, 0, p2, 0) != nullptr);

        // Swapped direction - should NOT find
        REQUIRE(graph.getConnectionBetween(p2, 0, p1, 0) == nullptr);
    }

    SECTION("DELETE: Connection cleanup on node removal")
    {
        MockFilterGraph graph;
        MockNodeID plugin = graph.addFilter("P1", 100, 100);

        // Add connections
        graph.addConnection(graph.audioInputNode, 0, plugin, 0);
        graph.addConnection(plugin, 0, graph.audioOutputNode, 0);

        int connectionsBefore = graph.getNumConnections();

        // Remove node
        graph.removeFilter(plugin);

        // Mutation: If connections weren't cleaned up
        int connectionsAfter = graph.getNumConnections();
        REQUIRE(connectionsAfter < connectionsBefore);
    }

    SECTION("CONDITION: Self-connection check")
    {
        MockFilterGraph graph;
        MockNodeID plugin = graph.addFilter("P1", 0, 0);

        // Self-connection should fail
        bool selfConnect = graph.addConnection(plugin, 0, plugin, 0);
        REQUIRE_FALSE(selfConnect);

        // Different nodes should succeed
        MockNodeID p2 = graph.addFilter("P2", 100, 0);
        bool normalConnect = graph.addConnection(plugin, 0, p2, 0);
        REQUIRE(normalConnect);
    }
}
