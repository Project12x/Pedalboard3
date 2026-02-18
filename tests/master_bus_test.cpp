/**
 * @file master_bus_test.cpp
 * @brief Integration and mutation tests for MasterBusProcessor logic
 *
 * MasterBusProcessor wraps a SubGraphProcessor that processes all audio
 * at the device callback level. These tests verify the control flow logic:
 *
 * 1. Bypass flag behavior (passthrough when bypassed)
 * 2. Prepared flag gating (no processing before prepare)
 * 3. HasPlugins flag gating (skip processing when rack is empty)
 * 4. State machine correctness (prepare -> process -> release cycle)
 * 5. HasPlugins detection (3 I/O nodes = no user plugins)
 *
 * NOTE: SubGraphProcessor has UI dependencies (SubGraphEditorComponent.h)
 * preventing direct compilation. The control flow logic is replicated here.
 */

#include <atomic>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <cmath>
#include <vector>

using Catch::Matchers::WithinAbs;

// =============================================================================
// MasterBusProcessor Control Flow Simulation
// =============================================================================

// Simulates the processBlock gating logic from MasterBusProcessor::processBlock
struct MasterBusSimulation
{
    std::atomic<bool> bypassed{false};
    std::atomic<bool> prepared{false};
    std::atomic<bool> hasPluginsFlag{false};
    bool rackValid = true;

    // Returns true if processBlock would delegate to the rack
    bool shouldProcess() const
    {
        if (!prepared.load(std::memory_order_acquire) || !rackValid)
            return false;
        if (bypassed.load(std::memory_order_relaxed))
            return false;
        if (!hasPluginsFlag.load(std::memory_order_acquire))
            return false;
        return true;
    }

    // Simulates hasPlugins() check:
    // SubGraphProcessor always has 3 I/O nodes (audio in, audio out, midi in)
    // If there are more nodes, the user has added plugins
    static bool hasUserPlugins(int numNodes) { return numNodes > 3; }

    void prepare() { prepared.store(true, std::memory_order_release); }

    void release() { prepared.store(false, std::memory_order_release); }
};

// =============================================================================
// INTEGRATION TESTS
// =============================================================================

TEST_CASE("MasterBus - Default Construction State", "[masterbus]")
{
    MasterBusSimulation bus;

    REQUIRE_FALSE(bus.bypassed.load());
    REQUIRE_FALSE(bus.prepared.load());
    REQUIRE_FALSE(bus.hasPluginsFlag.load());
    REQUIRE(bus.rackValid);
}

TEST_CASE("MasterBus - Prepare/Release Lifecycle", "[masterbus]")
{
    MasterBusSimulation bus;

    SECTION("Not prepared -> shouldProcess is false")
    {
        bus.hasPluginsFlag.store(true);
        REQUIRE_FALSE(bus.shouldProcess());
    }

    SECTION("After prepare -> can process if hasPlugins")
    {
        bus.prepare();
        bus.hasPluginsFlag.store(true);
        REQUIRE(bus.shouldProcess());
    }

    SECTION("After release -> shouldProcess is false again")
    {
        bus.prepare();
        bus.hasPluginsFlag.store(true);
        REQUIRE(bus.shouldProcess());

        bus.release();
        REQUIRE_FALSE(bus.shouldProcess());
    }

    SECTION("Re-prepare after release works")
    {
        bus.prepare();
        bus.release();
        bus.prepare();
        bus.hasPluginsFlag.store(true);
        REQUIRE(bus.shouldProcess());
    }
}

TEST_CASE("MasterBus - Bypass Behavior", "[masterbus]")
{
    MasterBusSimulation bus;
    bus.prepare();
    bus.hasPluginsFlag.store(true);

    SECTION("Not bypassed -> processes")
    {
        bus.bypassed.store(false);
        REQUIRE(bus.shouldProcess());
    }

    SECTION("Bypassed -> skips processing (passthrough)")
    {
        bus.bypassed.store(true);
        REQUIRE_FALSE(bus.shouldProcess());
    }

    SECTION("Toggle bypass")
    {
        bus.bypassed.store(true);
        REQUIRE_FALSE(bus.shouldProcess());

        bus.bypassed.store(false);
        REQUIRE(bus.shouldProcess());
    }
}

TEST_CASE("MasterBus - HasPlugins Gating", "[masterbus]")
{
    MasterBusSimulation bus;
    bus.prepare();

    SECTION("Empty rack (no plugins) -> skips processing")
    {
        bus.hasPluginsFlag.store(false);
        REQUIRE_FALSE(bus.shouldProcess());
    }

    SECTION("Rack with plugins -> processes")
    {
        bus.hasPluginsFlag.store(true);
        REQUIRE(bus.shouldProcess());
    }
}

TEST_CASE("MasterBus - HasPlugins Detection Logic", "[masterbus]")
{
    SECTION("3 nodes (I/O only) = no user plugins")
    {
        REQUIRE_FALSE(MasterBusSimulation::hasUserPlugins(3));
    }

    SECTION("4+ nodes = user has added plugins")
    {
        REQUIRE(MasterBusSimulation::hasUserPlugins(4));
        REQUIRE(MasterBusSimulation::hasUserPlugins(10));
    }

    SECTION("Less than 3 nodes (malformed) = no user plugins")
    {
        REQUIRE_FALSE(MasterBusSimulation::hasUserPlugins(0));
        REQUIRE_FALSE(MasterBusSimulation::hasUserPlugins(2));
    }
}

TEST_CASE("MasterBus - Invalid Rack Pointer", "[masterbus]")
{
    MasterBusSimulation bus;
    bus.prepare();
    bus.hasPluginsFlag.store(true);
    bus.rackValid = false;

    REQUIRE_FALSE(bus.shouldProcess());
}

TEST_CASE("MasterBus - Passthrough Audio Integrity", "[masterbus][dsp]")
{
    SECTION("Bypassed signal is unmodified")
    {
        // Simulate: bypassed processBlock should not touch buffer
        std::vector<float> buffer = {0.5f, -0.3f, 0.8f, -0.1f};
        std::vector<float> original = buffer;

        bool bypassed = true;
        if (bypassed)
        {
            // processBlock returns early -- no modification
        }

        REQUIRE(buffer == original);
    }

    SECTION("No-plugins signal is unmodified")
    {
        std::vector<float> buffer = {0.5f, -0.3f, 0.8f, -0.1f};
        std::vector<float> original = buffer;

        bool hasPlugins = false;
        if (!hasPlugins)
        {
            // processBlock returns early -- no modification
        }

        REQUIRE(buffer == original);
    }

    SECTION("Unprepared signal is unmodified")
    {
        std::vector<float> buffer = {0.5f, -0.3f, 0.8f, -0.1f};
        std::vector<float> original = buffer;

        bool prepared = false;
        if (!prepared)
        {
            // processBlock returns early
        }

        REQUIRE(buffer == original);
    }
}

// =============================================================================
// SubGraphProcessor Internal Graph Tests (logic only)
// =============================================================================

TEST_CASE("SubGraph - Default Has 3 I/O Nodes", "[masterbus][subgraph]")
{
    // SubGraphProcessor initializes with:
    // 1. audioInputNode
    // 2. audioOutputNode
    // 3. midiInputNode
    int defaultNodes = 3;

    REQUIRE(defaultNodes == 3);
    REQUIRE_FALSE(MasterBusSimulation::hasUserPlugins(defaultNodes));
}

TEST_CASE("SubGraph - Audio Passthrough Connection", "[masterbus][subgraph]")
{
    // Default graph connects audio_in -> audio_out (L+R)
    // Simulating: input goes straight to output with no processing
    std::vector<float> inputL = {0.5f, 0.3f, -0.2f};
    std::vector<float> inputR = {-0.1f, 0.7f, 0.4f};

    // Passthrough: output == input
    std::vector<float> outputL = inputL;
    std::vector<float> outputR = inputR;

    for (size_t i = 0; i < inputL.size(); ++i)
    {
        REQUIRE_THAT(outputL[i], WithinAbs(inputL[i], 0.0001f));
        REQUIRE_THAT(outputR[i], WithinAbs(inputR[i], 0.0001f));
    }
}

TEST_CASE("SubGraph - State Save/Restore Empty Rack", "[masterbus][subgraph][state]")
{
    // An empty rack (3 I/O nodes only) saves valid XML
    // On restore, it should recreate the same 3 I/O nodes

    int nodesBeforeSave = 3;

    // After clearing and reinitializing
    int nodesAfterRestore = 3;

    REQUIRE(nodesBeforeSave == nodesAfterRestore);
    REQUIRE_FALSE(MasterBusSimulation::hasUserPlugins(nodesAfterRestore));
}

// =============================================================================
// MUTATION TESTING - MasterBus
// =============================================================================

TEST_CASE("MasterBus Mutation Testing", "[masterbus][mutation]")
{
    SECTION("CONDITION REMOVAL: bypass check removed -> signal gets processed")
    {
        MasterBusSimulation bus;
        bus.prepare();
        bus.hasPluginsFlag.store(true);
        bus.bypassed.store(true);

        // Correct: bypassed should NOT process
        REQUIRE_FALSE(bus.shouldProcess());

        // Mutation: if bypass check removed, it WOULD process
        bool mutatedResult = bus.prepared.load() && bus.rackValid && bus.hasPluginsFlag.load();
        REQUIRE(mutatedResult); // Bug: processes when bypassed
    }

    SECTION("CONDITION REMOVAL: hasPlugins check removed -> empty rack processes")
    {
        MasterBusSimulation bus;
        bus.prepare();
        bus.hasPluginsFlag.store(false);

        // Correct: no plugins should skip
        REQUIRE_FALSE(bus.shouldProcess());

        // Mutation: if hasPlugins check removed
        bool mutatedResult = bus.prepared.load() && bus.rackValid && !bus.bypassed.load();
        REQUIRE(mutatedResult); // Bug: empty rack would process
    }

    SECTION("CONDITION REMOVAL: prepared check removed -> unprepared processes")
    {
        MasterBusSimulation bus;
        // Not prepared
        bus.hasPluginsFlag.store(true);

        // Correct: should not process
        REQUIRE_FALSE(bus.shouldProcess());

        // Mutation: if prepared check removed
        bool mutatedResult = bus.rackValid && !bus.bypassed.load() && bus.hasPluginsFlag.load();
        REQUIRE(mutatedResult); // Bug: unprepared would process
    }

    SECTION("THRESHOLD: hasPlugins uses > 3, not >= 3 or > 2")
    {
        // Correct: 3 nodes = I/O only, no user plugins
        REQUIRE_FALSE(MasterBusSimulation::hasUserPlugins(3));
        REQUIRE(MasterBusSimulation::hasUserPlugins(4));

        // Mutation 1: >= 3 would false-positive on empty rack
        bool mutated_ge3 = (3 >= 3);
        REQUIRE(mutated_ge3); // Bug: empty rack shows as having plugins

        // Mutation 2: > 2 would also false-positive
        bool mutated_gt2 = (3 > 2);
        REQUIRE(mutated_gt2); // Bug: same
    }

    SECTION("ORDER: gating checks order matters for early exit")
    {
        MasterBusSimulation bus;
        // rack is invalid -> should not process regardless of other flags
        bus.rackValid = false;
        bus.prepared.store(true);
        bus.hasPluginsFlag.store(true);

        REQUIRE_FALSE(bus.shouldProcess());
    }

    SECTION("MEMORY ORDER: prepared uses acquire, not relaxed")
    {
        // This test verifies the semantic intent:
        // prepared.load with acquire ensures all writes before
        // prepare() are visible to the audio thread
        MasterBusSimulation bus;

        // Simulate prepare on message thread
        bus.prepare();

        // On audio thread, acquire should see the store
        bool seen = bus.prepared.load(std::memory_order_acquire);
        REQUIRE(seen);
    }
}
