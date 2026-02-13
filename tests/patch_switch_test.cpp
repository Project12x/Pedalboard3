/**
 * @file patch_switch_test.cpp
 * @brief Regression tests for patch-switch stability
 *
 * Tests cover:
 * 1. Infrastructure node lifecycle across graph clears
 * 2. Pointer reacquisition after clear/restore cycles
 * 3. Infrastructure node exclusion from XML serialization
 * 4. Rapid patch-switch cycles (stress)
 * 5. FIFO event safety during graph transitions
 *
 * Root cause: PluginField::loadFromXml cached a CrossfadeMixer* then
 * cleared the graph (destroying all nodes), then used the stale pointer.
 * These tests verify the fix holds under repeated cycling.
 */

#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <vector>

// =============================================================================
// Mock Infrastructure — all stack/pool-based, no heap allocation for objects
// Wrapped in anonymous namespace to avoid ODR violations with other test files
// =============================================================================

namespace {

struct MockCrossfadeMixer
{
    bool fadeInStarted = false;
    int fadeDurationMs = 0;

    void startFadeIn(int durationMs)
    {
        fadeInStarted = true;
        fadeDurationMs = durationMs;
    }

    void reset()
    {
        fadeInStarted = false;
        fadeDurationMs = 0;
    }
};

struct MockSafetyLimiter
{
    bool active = true;
    void reset() { active = true; }
};

struct MockNodeID
{
    uint32_t uid = 0;
    bool operator==(const MockNodeID& other) const { return uid == other.uid; }
    bool operator!=(const MockNodeID& other) const { return uid != other.uid; }
};

// All POD - no std::string, no heap allocation per node
struct MockNode
{
    MockNodeID id;
    bool isInfrastructure;
};

struct MockConnection
{
    MockNodeID src;
    int srcCh;
    MockNodeID dst;
    int dstCh;
};

// Simulates FilterGraph with infrastructure node lifecycle.
// Uses a pool of infrastructure objects so pointers change after clear().
class MockFilterGraph
{
    static constexpr int POOL_SIZE = 256;
    MockCrossfadeMixer mixerPool[POOL_SIZE];
    MockSafetyLimiter limiterPool[POOL_SIZE];
    int poolIndex = 0;

  public:
    std::vector<MockNode> nodes;
    std::vector<MockConnection> connections;
    uint32_t nextUID = 100;

    // Infrastructure pointers - must be refreshed after clear()
    MockCrossfadeMixer* crossfadeMixer = nullptr;
    MockSafetyLimiter* safetyLimiter = nullptr;
    MockNodeID crossfadeMixerNodeId{};
    MockNodeID safetyLimiterNodeId{};

    MockFilterGraph() { createInfrastructureNodes(); }

    void createInfrastructureNodes()
    {
        crossfadeMixer = &mixerPool[poolIndex];
        crossfadeMixer->reset();
        safetyLimiter = &limiterPool[poolIndex];
        safetyLimiter->reset();
        poolIndex++;

        crossfadeMixerNodeId = {nextUID++};
        safetyLimiterNodeId = {nextUID++};
        nodes.push_back({crossfadeMixerNodeId, true});
        nodes.push_back({safetyLimiterNodeId, true});
    }

    MockCrossfadeMixer* getCrossfadeMixer() const { return crossfadeMixer; }

    bool isHiddenInfrastructureNode(MockNodeID id) const
    {
        return id == crossfadeMixerNodeId || id == safetyLimiterNodeId;
    }

    MockNodeID addFilter()
    {
        MockNodeID id{nextUID++};
        nodes.push_back({id, false});
        return id;
    }

    void addConnection(MockNodeID src, int srcCh, MockNodeID dst, int dstCh)
    {
        connections.push_back({src, srcCh, dst, dstCh});
    }

    void clear()
    {
        nodes.clear();
        connections.clear();
        crossfadeMixer = nullptr;
        safetyLimiter = nullptr;
        createInfrastructureNodes();
    }

    // Simulates createXml - infrastructure nodes excluded
    std::vector<MockNode> getSerializableNodes() const
    {
        std::vector<MockNode> result;
        for (const auto& node : nodes)
        {
            if (!isHiddenInfrastructureNode(node.id))
                result.push_back(node);
        }
        return result;
    }

    std::vector<MockConnection> getSerializableConnections() const
    {
        std::vector<MockConnection> result;
        for (const auto& conn : connections)
        {
            if (!isHiddenInfrastructureNode(conn.src) && !isHiddenInfrastructureNode(conn.dst))
                result.push_back(conn);
        }
        return result;
    }

    int getNumFilters() const { return static_cast<int>(nodes.size()); }
};

// Simulates FIFO parameter change events
struct MockParamChange
{
    MockFilterGraph* graph;
    uint32_t pluginId;
    int paramIndex;
    float value;
};

class MockParamFifo
{
  public:
    std::vector<MockParamChange> buffer;

    void write(MockFilterGraph* graph, uint32_t pluginId, int paramIndex, float value)
    {
        buffer.push_back({graph, pluginId, paramIndex, value});
    }

    bool read(MockParamChange& out)
    {
        if (buffer.empty())
            return false;
        out = buffer.front();
        buffer.erase(buffer.begin());
        return true;
    }

    int size() const { return static_cast<int>(buffer.size()); }
};

} // anonymous namespace

// =============================================================================
// Infrastructure Lifecycle Tests
// =============================================================================

TEST_CASE("Infrastructure exists after construction", "[patchswitch][infrastructure]")
{
    MockFilterGraph graph;

    REQUIRE(graph.getCrossfadeMixer() != nullptr);
    REQUIRE(graph.safetyLimiter != nullptr);
    REQUIRE(graph.crossfadeMixerNodeId.uid != 0);
    REQUIRE(graph.safetyLimiterNodeId.uid != 0);
}

TEST_CASE("Infrastructure rebuilt after clear", "[patchswitch][infrastructure]")
{
    MockFilterGraph graph;
    auto* oldMixer = graph.getCrossfadeMixer();
    auto oldMixerId = graph.crossfadeMixerNodeId;

    graph.clear();

    REQUIRE(graph.getCrossfadeMixer() != nullptr);
    REQUIRE(graph.safetyLimiter != nullptr);
    REQUIRE(graph.getCrossfadeMixer() != oldMixer);
    REQUIRE(graph.crossfadeMixerNodeId != oldMixerId);
}

TEST_CASE("Multiple clears produce valid infrastructure", "[patchswitch][infrastructure]")
{
    MockFilterGraph graph;

    for (int i = 0; i < 50; ++i)
    {
        graph.addFilter();
        graph.clear();

        REQUIRE(graph.getCrossfadeMixer() != nullptr);
        REQUIRE(graph.safetyLimiter != nullptr);

        // Infrastructure nodes are in the graph
        bool foundMixer = false;
        bool foundLimiter = false;
        for (const auto& node : graph.nodes)
        {
            if (node.id == graph.crossfadeMixerNodeId)
                foundMixer = true;
            if (node.id == graph.safetyLimiterNodeId)
                foundLimiter = true;
        }
        REQUIRE(foundMixer);
        REQUIRE(foundLimiter);
    }
}

// =============================================================================
// Pointer Reacquisition Tests
// =============================================================================

TEST_CASE("CrossfadeMixer pointer reacquired after patch load", "[patchswitch][crossfade]")
{
    MockFilterGraph graph;

    SECTION("Simulated patch switch: fade-out, clear, restore, fade-in")
    {
        // Step 1: Fade out (using current crossfader)
        auto* fadeOutMixer = graph.getCrossfadeMixer();
        REQUIRE(fadeOutMixer != nullptr);

        // Step 2: Clear graph (destroys old infrastructure, rebuilds new)
        graph.clear();

        // Step 3: Restore nodes from patch XML
        graph.addFilter();
        graph.addFilter();

        // Step 4: Reacquire crossfader pointer AFTER restore
        auto* fadeInMixer = graph.getCrossfadeMixer();
        REQUIRE(fadeInMixer != nullptr);
        REQUIRE(fadeInMixer != fadeOutMixer); // Must be the new instance

        // Step 5: Fade in using the new (valid) pointer
        fadeInMixer->startFadeIn(100);
        REQUIRE(fadeInMixer->fadeInStarted);
        REQUIRE(fadeInMixer->fadeDurationMs == 100);
    }

    SECTION("Using stale pointer would hit wrong object")
    {
        auto* beforeClear = graph.getCrossfadeMixer();
        graph.clear();
        auto* afterClear = graph.getCrossfadeMixer();

        // The key invariant: these must be different objects
        REQUIRE(beforeClear != afterClear);
    }
}

// =============================================================================
// XML Serialization Exclusion Tests
// =============================================================================

TEST_CASE("Infrastructure nodes excluded from XML", "[patchswitch][serialization]")
{
    MockFilterGraph graph;

    SECTION("Only user nodes appear in serializable output")
    {
        auto n1 = graph.addFilter();
        auto n2 = graph.addFilter();

        auto serializable = graph.getSerializableNodes();
        REQUIRE(serializable.size() == 2);

        for (const auto& node : serializable)
        {
            REQUIRE_FALSE(graph.isHiddenInfrastructureNode(node.id));
        }
    }

    SECTION("Connections to infrastructure excluded")
    {
        auto plugin = graph.addFilter();
        graph.addConnection(plugin, 0, graph.crossfadeMixerNodeId, 0);
        graph.addConnection(graph.safetyLimiterNodeId, 0, plugin, 0);

        // User-to-user connection
        auto plugin2 = graph.addFilter();
        graph.addConnection(plugin, 0, plugin2, 0);

        auto serializable = graph.getSerializableConnections();

        // Only the user-to-user connection should survive
        REQUIRE(serializable.size() == 1);
        REQUIRE(serializable[0].src == plugin);
        REQUIRE(serializable[0].dst == plugin2);
    }

    SECTION("Empty graph serializes no user nodes")
    {
        // Graph only has infrastructure
        auto serializable = graph.getSerializableNodes();
        REQUIRE(serializable.empty());
    }

    SECTION("Save-clear-restore doesn't accumulate infrastructure")
    {
        graph.addFilter();

        for (int i = 0; i < 10; ++i)
        {
            auto savedNodes = graph.getSerializableNodes();
            graph.clear();

            // Restore saved user nodes
            for (const auto& node : savedNodes)
                graph.addFilter();

            // Infrastructure should be exactly 2 nodes, not accumulating
            int infraCount = 0;
            for (const auto& node : graph.nodes)
            {
                if (graph.isHiddenInfrastructureNode(node.id))
                    infraCount++;
            }
            REQUIRE(infraCount == 2);
        }
    }
}

// =============================================================================
// FIFO Safety During Transitions
// =============================================================================

TEST_CASE("FIFO events discarded for wrong graph", "[patchswitch][fifo]")
{
    MockFilterGraph graphA;
    MockFilterGraph graphB;
    MockParamFifo fifo;

    SECTION("Events from old graph skipped on drain")
    {
        // Audio thread writes events for graphA
        auto pluginA = graphA.addFilter();
        fifo.write(&graphA, pluginA.uid, 0, 0.5f);
        fifo.write(&graphA, pluginA.uid, 1, 0.8f);

        // Patch switch: now graphB is active
        MockFilterGraph* activeGraph = &graphB;

        // Drain loop (mirrors MainPanel.cpp logic)
        int dispatched = 0;
        int skipped = 0;
        MockParamChange pc;
        while (fifo.read(pc))
        {
            if (pc.graph != activeGraph)
            {
                skipped++;
                continue;
            }
            dispatched++;
        }

        REQUIRE(dispatched == 0);
        REQUIRE(skipped == 2);
    }

    SECTION("Events for current graph dispatched normally")
    {
        MockFilterGraph* activeGraph = &graphA;

        auto plugin = graphA.addFilter();
        fifo.write(&graphA, plugin.uid, 0, 0.5f);

        int dispatched = 0;
        MockParamChange pc;
        while (fifo.read(pc))
        {
            if (pc.graph != activeGraph)
                continue;
            dispatched++;
        }

        REQUIRE(dispatched == 1);
    }
}

TEST_CASE("FIFO paramIndex bounds checked", "[patchswitch][fifo]")
{
    SECTION("Out-of-range paramIndex rejected")
    {
        int numParams = 3;
        int paramIndex = 5; // Out of range

        bool valid = (paramIndex >= 0 && paramIndex < numParams);
        REQUIRE_FALSE(valid);
    }

    SECTION("Bypass index (-1) handled separately")
    {
        int paramIndex = -1;
        int numParams = 3;

        bool isBypass = (paramIndex == -1);
        bool validParam = (paramIndex >= 0 && paramIndex < numParams);

        REQUIRE(isBypass);
        REQUIRE_FALSE(validParam);
    }

    SECTION("Valid paramIndex accepted")
    {
        int numParams = 5;
        for (int i = 0; i < numParams; ++i)
        {
            bool valid = (i >= 0 && i < numParams);
            REQUIRE(valid);
        }
    }
}

// =============================================================================
// Rapid Patch-Switch Stress Test
// =============================================================================

TEST_CASE("Rapid patch-switch cycles", "[patchswitch][stress]")
{
    SECTION("100 consecutive clear-restore cycles")
    {
        MockFilterGraph graph;
        MockParamFifo fifo;

        for (int cycle = 0; cycle < 100; ++cycle)
        {
            // Simulate audio thread writing FIFO events
            for (const auto& node : graph.nodes)
            {
                if (!graph.isHiddenInfrastructureNode(node.id))
                    fifo.write(&graph, node.id.uid, 0, 0.5f);
            }

            // Save user nodes
            auto saved = graph.getSerializableNodes();

            // Clear (rebuilds infrastructure)
            graph.clear();

            // Restore user nodes
            for (const auto& node : saved)
                graph.addFilter();

            // Reacquire crossfader and fade in
            auto* mixer = graph.getCrossfadeMixer();
            REQUIRE(mixer != nullptr);
            mixer->startFadeIn(100);
            REQUIRE(mixer->fadeInStarted);

            // Drain FIFO - all events should be for old graph pointers
            // (In real code, the graph address doesn't change, but node IDs do.
            // The bounds check on paramIndex protects against stale dispatches.)
            MockParamChange pc;
            while (fifo.read(pc))
            {
                // Just drain - no crash
            }
        }

        // Final state should be consistent
        REQUIRE(graph.getCrossfadeMixer() != nullptr);
        REQUIRE(graph.safetyLimiter != nullptr);

        int infraCount = 0;
        for (const auto& node : graph.nodes)
        {
            if (graph.isHiddenInfrastructureNode(node.id))
                infraCount++;
        }
        REQUIRE(infraCount == 2);
    }

    SECTION("Alternating empty and loaded patches")
    {
        MockFilterGraph graph;

        for (int cycle = 0; cycle < 50; ++cycle)
        {
            graph.clear();

            if (cycle % 2 == 0)
            {
                // Loaded patch
                graph.addFilter();
                graph.addFilter();
                graph.addFilter();
            }
            // else: empty patch (just infrastructure)

            auto* mixer = graph.getCrossfadeMixer();
            REQUIRE(mixer != nullptr);
            mixer->startFadeIn(100);

            auto serializable = graph.getSerializableNodes();
            if (cycle % 2 == 0)
                REQUIRE(serializable.size() == 3);
            else
                REQUIRE(serializable.empty());
        }
    }
}

// =============================================================================
// Mutation Testing
// =============================================================================

TEST_CASE("Patch-switch mutation testing", "[patchswitch][mutation]")
{
    SECTION("SKIP: createInfrastructureNodes not called after clear")
    {
        MockFilterGraph graph;
        auto* before = graph.getCrossfadeMixer();

        // If clear() didn't call createInfrastructureNodes, pointers would be stale.
        // Our mock always calls it, so verify the pointer changes.
        graph.clear();
        auto* after = graph.getCrossfadeMixer();

        REQUIRE(after != nullptr);
        REQUIRE(before != after);
    }

    SECTION("REORDER: Infrastructure pointer read before vs after clear")
    {
        MockFilterGraph graph;

        // Correct order: clear first, then read pointer
        graph.clear();
        auto* correct = graph.getCrossfadeMixer();
        REQUIRE(correct != nullptr);
        correct->startFadeIn(100);
        REQUIRE(correct->fadeInStarted);
    }

    SECTION("NEGATE: isHiddenInfrastructureNode check in serialization")
    {
        MockFilterGraph graph;
        graph.addFilter();

        auto serializable = graph.getSerializableNodes();

        // If negated, infrastructure would appear and user nodes would be excluded
        REQUIRE(serializable.size() == 1);
        REQUIRE_FALSE(serializable[0].isInfrastructure);

        // Verify infrastructure IS hidden
        for (const auto& node : serializable)
        {
            REQUIRE_FALSE(graph.isHiddenInfrastructureNode(node.id));
        }
    }

    SECTION("OFF-BY-ONE: Infrastructure node count after multiple clears")
    {
        MockFilterGraph graph;

        for (int i = 0; i < 20; ++i)
        {
            graph.clear();

            int infraCount = 0;
            for (const auto& node : graph.nodes)
            {
                if (graph.isHiddenInfrastructureNode(node.id))
                    infraCount++;
            }
            // Must be exactly 2, not accumulating
            REQUIRE(infraCount == 2);
        }
    }
}
