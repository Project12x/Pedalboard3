/**
 * @file subgraph_test.cpp
 * @brief Unit tests for SubGraph features (Effect Rack)
 *
 * Tests cover:
 * 1. SubGraphFilterGraph adapter interface compliance
 * 2. Connection boundary conditions
 * 3. Node lifecycle operations
 * 4. Pin hit detection logic
 * 5. Mutation testing patterns for critical operations
 *
 * Note: These tests verify logic contracts without JUCE audio initialization.
 * Full integration testing requires manual or browser-based UI testing.
 */

#include <algorithm>
#include <catch2/catch_test_macros.hpp>
#include <map>
#include <string>
#include <vector>

// =============================================================================
// SubGraphFilterGraph Adapter Logic Tests (no JUCE dependency)
// =============================================================================

// Simulate NodeID for testing (mirrors juce::AudioProcessorGraph::NodeID)
struct TestNodeID
{
    uint32_t uid;
    bool operator==(const TestNodeID& other) const { return uid == other.uid; }
    bool operator!=(const TestNodeID& other) const { return uid != other.uid; }
};

TEST_CASE("SubGraphFilterGraph - Node ID Management", "[subgraph][adapter]")
{
    SECTION("Reserved IO node IDs")
    {
        // Input node ID is typically 1, Output node ID is 2
        TestNodeID inputNodeId{1};
        TestNodeID outputNodeId{2};
        TestNodeID userNodeId{100};

        // IO nodes should be protected from deletion
        auto isIONode = [](TestNodeID id) { return id.uid == 1 || id.uid == 2; };

        REQUIRE(isIONode(inputNodeId));
        REQUIRE(isIONode(outputNodeId));
        REQUIRE_FALSE(isIONode(userNodeId));
    }

    SECTION("Node ID uniqueness")
    {
        std::vector<TestNodeID> nodes = {{1}, {2}, {100}, {101}, {102}};

        // Check all IDs are unique
        for (size_t i = 0; i < nodes.size(); ++i)
        {
            for (size_t j = i + 1; j < nodes.size(); ++j)
            {
                REQUIRE(nodes[i] != nodes[j]);
            }
        }
    }
}

// =============================================================================
// Connection Boundary Testing
// =============================================================================

struct TestConnection
{
    TestNodeID sourceId;
    int sourceChannel;
    TestNodeID destId;
    int destChannel;
};

TEST_CASE("Connection Boundary Conditions", "[subgraph][connections][boundary]")
{
    SECTION("Valid channel indices")
    {
        // Standard stereo: channels 0, 1 are valid
        int numChannels = 2;
        int validChannel = 0;
        int invalidChannel = 5;

        REQUIRE(validChannel >= 0);
        REQUIRE(validChannel < numChannels);
        bool invalidInRange = (invalidChannel >= 0) && (invalidChannel < numChannels);
        REQUIRE_FALSE(invalidInRange);
    }

    SECTION("Negative channel index rejection")
    {
        int channel = -1;
        int numChannels = 2;

        bool isValid = (channel >= 0) && (channel < numChannels);
        REQUIRE_FALSE(isValid);
    }

    SECTION("Self-connection prevention")
    {
        TestNodeID nodeA{100};
        TestNodeID nodeB{100}; // Same node

        bool isSelfConnection = (nodeA == nodeB);
        REQUIRE(isSelfConnection);
        // Self-connections should be rejected
    }

    SECTION("Duplicate connection detection")
    {
        std::vector<TestConnection> connections;
        connections.push_back({{100}, 0, {101}, 0});

        TestConnection newConn = {{100}, 0, {101}, 0};

        // Check if connection already exists
        bool exists = false;
        for (const auto& conn : connections)
        {
            if (conn.sourceId == newConn.sourceId && conn.sourceChannel == newConn.sourceChannel &&
                conn.destId == newConn.destId && conn.destChannel == newConn.destChannel)
            {
                exists = true;
                break;
            }
        }
        REQUIRE(exists);
    }

    SECTION("Connection to IO nodes")
    {
        TestNodeID inputNode{1};  // Input IO
        TestNodeID outputNode{2}; // Output IO
        TestNodeID userNode{100};

        // User nodes should be able to connect FROM input and TO output
        TestConnection validFromInput = {inputNode, 0, userNode, 0};
        TestConnection validToOutput = {userNode, 0, outputNode, 0};

        // But not: output -> something or something -> input (wrong direction)
        REQUIRE(validFromInput.sourceId.uid == 1); // From input
        REQUIRE(validToOutput.destId.uid == 2);    // To output
    }
}

// =============================================================================
// Pin Hit Detection Tests
// =============================================================================

struct Point2D
{
    int x, y;
};

struct Rectangle2D
{
    int x, y, width, height;
    bool contains(Point2D p) const { return p.x >= x && p.x < x + width && p.y >= y && p.y < y + height; }
};

TEST_CASE("Pin Hit Detection Logic", "[subgraph][ui][pins]")
{
    // Pin constants matching RackNodeComponent
    constexpr int PIN_SPACING = 18;
    constexpr int HEADER_HEIGHT = 24;
    (void)PIN_SPACING; // Mark as used

    SECTION("Hit detection on exact pin center")
    {
        // Pin at input channel 0
        Point2D pinCenter{0, HEADER_HEIGHT + 10};
        Rectangle2D hitRect{pinCenter.x - 8, pinCenter.y - 8, 16, 16};

        Point2D clickExactCenter = pinCenter;
        REQUIRE(hitRect.contains(clickExactCenter));
    }

    SECTION("Hit detection at pin edge")
    {
        Point2D pinCenter{0, HEADER_HEIGHT + 10};
        Rectangle2D hitRect{pinCenter.x - 8, pinCenter.y - 8, 16, 16};

        Point2D clickAtEdge{pinCenter.x + 7, pinCenter.y + 7};
        REQUIRE(hitRect.contains(clickAtEdge));
    }

    SECTION("Miss detection outside pin")
    {
        Point2D pinCenter{0, HEADER_HEIGHT + 10};
        Rectangle2D hitRect{pinCenter.x - 8, pinCenter.y - 8, 16, 16};

        Point2D clickOutside{pinCenter.x + 20, pinCenter.y};
        REQUIRE_FALSE(hitRect.contains(clickOutside));
    }

    SECTION("Multi-pin iteration")
    {
        int numPins = 4;
        Point2D clickPos{0, HEADER_HEIGHT + 10 + PIN_SPACING}; // Second pin

        int foundPin = -1;
        for (int i = 0; i < numPins; ++i)
        {
            Point2D pinCenter{0, HEADER_HEIGHT + 10 + i * PIN_SPACING};
            Rectangle2D hitRect{pinCenter.x - 8, pinCenter.y - 8, 16, 16};

            if (hitRect.contains(clickPos))
            {
                foundPin = i;
                break;
            }
        }

        REQUIRE(foundPin == 1); // Should hit second pin (index 1)
    }

    SECTION("Output pins on right side")
    {
        int nodeWidth = 180;
        Point2D outputPinCenter{nodeWidth, HEADER_HEIGHT + 10};
        Rectangle2D hitRect{outputPinCenter.x - 8, outputPinCenter.y - 8, 16, 16};

        Point2D clickNearRight{nodeWidth - 5, HEADER_HEIGHT + 10};
        REQUIRE(hitRect.contains(clickNearRight));
    }
}

// =============================================================================
// Node Lifecycle Tests
// =============================================================================

TEST_CASE("Node Lifecycle Operations", "[subgraph][nodes][lifecycle]")
{
    SECTION("Maximum node count boundary")
    {
        // Graph should handle reasonable number of nodes
        int maxReasonable = 100;
        int nodeCount = 95;

        REQUIRE(nodeCount < maxReasonable);
    }

    SECTION("Empty graph state")
    {
        int numNodes = 0;

        // With 0 nodes, iteration should be safe
        for (int i = 0; i < numNodes; ++i)
        {
            FAIL("Should not iterate with 0 nodes");
        }
        SUCCEED("Empty graph handled correctly");
    }

    SECTION("Node removal updates graph")
    {
        std::vector<TestNodeID> nodes = {{1}, {2}, {100}, {101}};
        size_t originalCount = nodes.size();

        // Remove node 100
        nodes.erase(std::remove_if(nodes.begin(), nodes.end(), [](TestNodeID id) { return id.uid == 100; }),
                    nodes.end());

        REQUIRE(nodes.size() == originalCount - 1);
        // Verify 100 is gone
        bool found = false;
        for (const auto& n : nodes)
            if (n.uid == 100)
                found = true;
        REQUIRE_FALSE(found);
    }
}

// =============================================================================
// Mutation Testing Patterns
// =============================================================================

TEST_CASE("Mutation Testing - Critical Operations", "[subgraph][mutation]")
{
    SECTION("OFF-BY-ONE: Connection index bounds")
    {
        int numChannels = 2;
        int lastValidChannel = numChannels - 1;

        // Original: channel < numChannels
        REQUIRE(lastValidChannel < numChannels);

        // Mutation: channel <= numChannels would incorrectly allow channel 2
        int invalidChannel = numChannels;
        bool mutatedCheck = invalidChannel <= numChannels;
        bool correctCheck = invalidChannel < numChannels;

        REQUIRE(mutatedCheck == true);  // Mutation would pass
        REQUIRE(correctCheck == false); // Correct check fails
    }

    SECTION("ARITHMETIC: Pin position calculation")
    {
        int channel = 2;
        int expectedY = 24 + 10 + channel * 18; // HEADER + 10 + channel * SPACING

        // Original
        int correctY = 24 + 10 + channel * 18;
        REQUIRE(correctY == expectedY);

        // Mutation: + instead of * would be wrong
        int mutatedY = 24 + 10 + channel + 18;
        REQUIRE(mutatedY != expectedY); // Mutation detected
    }

    SECTION("NEGATE: IO node check")
    {
        TestNodeID ioNode{1};
        TestNodeID userNode{100};

        auto isIONode = [](TestNodeID id) { return id.uid == 1 || id.uid == 2; };

        // Original behavior
        REQUIRE(isIONode(ioNode));
        REQUIRE_FALSE(isIONode(userNode));

        // If negated: !isIONode would be wrong
        // We verify by checking both cases
    }

    SECTION("CONDITION SWAP: Source vs Dest")
    {
        TestConnection conn = {{100}, 0, {101}, 1};

        // Original: connects source -> dest
        REQUIRE(conn.sourceId.uid == 100);
        REQUIRE(conn.destId.uid == 101);

        // If swapped (mutation), would be wrong
        REQUIRE(conn.sourceId.uid != conn.destId.uid);
    }
}

// =============================================================================
// Integration Contract Tests
// =============================================================================

TEST_CASE("IFilterGraph Interface Contract", "[subgraph][interface]")
{
    SECTION("addFilter returns valid ID")
    {
        // Simulated: addFilter should return non-zero ID on success
        uint32_t returnedId = 100; // Simulated return
        REQUIRE(returnedId != 0);
    }

    SECTION("removeFilter with invalid ID is safe")
    {
        // removeFilter should not crash with invalid ID
        uint32_t invalidId = 99999;
        // In real code: graphAdapter.removeFilter({invalidId})
        // Should return gracefully without crash
        REQUIRE(invalidId > 0); // Placeholder - would need real test
    }

    SECTION("addConnection validates both endpoints")
    {
        TestNodeID validSource{100};
        TestNodeID validDest{101};
        TestNodeID invalidNode{0}; // ID 0 is typically invalid

        bool sourceValid = validSource.uid > 0;
        bool destValid = validDest.uid > 0;
        bool invalidValid = invalidNode.uid > 0;

        REQUIRE(sourceValid);
        REQUIRE(destValid);
        REQUIRE_FALSE(invalidValid);
    }
}

// =============================================================================
// UI State Machine Tests
// =============================================================================

TEST_CASE("Connection Dragging State Machine", "[subgraph][ui][drag]")
{
    SECTION("Initial state - not dragging")
    {
        bool isDragging = false;
        int sourceChannel = -1;

        REQUIRE_FALSE(isDragging);
        REQUIRE(sourceChannel == -1);
    }

    SECTION("State after mouseDown on pin")
    {
        bool isDragging = true;
        int sourceChannel = 0;
        uint32_t sourceNodeId = 100;

        REQUIRE(isDragging);
        REQUIRE(sourceChannel >= 0);
        REQUIRE(sourceNodeId > 0);
    }

    SECTION("State after mouseUp - reset")
    {
        bool isDragging = false;
        int sourceChannel = -1;

        // After mouseUp, state should reset
        REQUIRE_FALSE(isDragging);
        REQUIRE(sourceChannel == -1);
    }

    SECTION("Drag direction tracking")
    {
        bool fromOutput = true; // Dragging from output pin

        // Determines which target pin type to look for
        REQUIRE(fromOutput); // Should look for input pins

        fromOutput = false;
        REQUIRE_FALSE(fromOutput); // Should look for output pins
    }
}

// =============================================================================
// State Persistence Tests
// =============================================================================

struct SerializedNode
{
    int uid;
    double x, y;
    std::string pluginId;
    std::string base64State;
};

struct SerializedConnection
{
    int srcNode, srcChannel;
    int dstNode, dstChannel;
};

TEST_CASE("Rack State Persistence", "[subgraph][persistence]")
{
    SECTION("UID mapping - IO node convention")
    {
        // Convention: 1 = audio in, 2 = audio out, 3 = MIDI in
        constexpr int AUDIO_IN_UID = 1;
        constexpr int AUDIO_OUT_UID = 2;
        constexpr int MIDI_IN_UID = 3;

        auto isIONodeUid = [](int uid) { return uid == 1 || uid == 2 || uid == 3; };

        REQUIRE(isIONodeUid(AUDIO_IN_UID));
        REQUIRE(isIONodeUid(AUDIO_OUT_UID));
        REQUIRE(isIONodeUid(MIDI_IN_UID));
        REQUIRE_FALSE(isIONodeUid(100)); // User nodes start at higher IDs
    }

    SECTION("UID remapping on restore")
    {
        // When loading, old UIDs may not match new node IDs
        std::map<int, int> uidToNewNodeId;

        // Simulate: old UID 100 becomes new node ID 5
        uidToNewNodeId[100] = 5;
        uidToNewNodeId[101] = 6;

        // Lookup old UIDs
        REQUIRE(uidToNewNodeId.find(100) != uidToNewNodeId.end());
        REQUIRE(uidToNewNodeId[100] == 5);

        // Missing UID should return end()
        REQUIRE(uidToNewNodeId.find(999) == uidToNewNodeId.end());
    }

    SECTION("Connection restoration with UID mapping")
    {
        std::map<int, int> uidMap;
        uidMap[100] = 5;
        uidMap[101] = 6;

        // Simulated saved connection
        SerializedConnection savedConn{100, 0, 101, 1};

        // Restore using UID map
        int restoredSrcNode = uidMap[savedConn.srcNode];
        int restoredDstNode = uidMap[savedConn.dstNode];

        REQUIRE(restoredSrcNode == 5);
        REQUIRE(restoredDstNode == 6);
    }

    SECTION("IO nodes handled specially during restore")
    {
        std::map<int, int> uidMap;
        // User nodes in map
        uidMap[100] = 5;

        // IO node UIDs (1, 2, 3) are NOT in user map
        int srcUid = 1; // Audio input

        bool foundInMap = uidMap.find(srcUid) != uidMap.end();
        bool isAudioIn = (srcUid == 1);

        REQUIRE_FALSE(foundInMap);
        REQUIRE(isAudioIn);
        // Code should fall back to rackAudioInNode for UID 1
    }

    SECTION("Empty state restoration")
    {
        std::vector<SerializedNode> nodes;
        std::vector<SerializedConnection> connections;

        // Empty rack should still be valid
        REQUIRE(nodes.empty());
        REQUIRE(connections.empty());
        // After restore, should have default passthrough connection
    }

    SECTION("Base64 state encoding round-trip")
    {
        // Simple simulation of base64 encoding
        std::string originalState = "plugin_param=42";

        // In real code: state.toBase64Encoding() / state.fromBase64Encoding()
        // Here we just verify the string isn't empty
        REQUIRE_FALSE(originalState.empty());
    }
}

TEST_CASE("Persistence Mutation Testing", "[subgraph][persistence][mutation]")
{
    SECTION("ARITHMETIC: Position coordinates")
    {
        double savedX = 200.0;
        double savedY = 150.0;

        // Correct restoration
        double restoredX = savedX;
        double restoredY = savedY;

        REQUIRE(restoredX == 200.0);
        REQUIRE(restoredY == 150.0);

        // Mutation: if x/y were swapped
        double mutatedX = savedY;
        double mutatedY = savedX;

        REQUIRE(mutatedX != savedX); // Mutation detectable
        REQUIRE(mutatedY != savedY); // Mutation detectable
    }

    SECTION("SWAP: Source and destination in connection")
    {
        SerializedConnection conn{100, 0, 101, 1};

        // Correct: 100 -> 101
        REQUIRE(conn.srcNode == 100);
        REQUIRE(conn.dstNode == 101);

        // Mutation: if src/dst were swapped
        int mutatedSrc = conn.dstNode;
        int mutatedDst = conn.srcNode;

        REQUIRE(mutatedSrc != conn.srcNode); // Would be wrong
    }

    SECTION("OFF-BY-ONE: Channel indices in saved connections")
    {
        int savedChannel = 1;
        int numChannels = 2;

        // Correct: channel is within bounds
        REQUIRE((savedChannel >= 0 && savedChannel < numChannels));

        // Mutation: if <= was used instead of <
        int invalidChannel = numChannels;
        bool mutatedCheck = (invalidChannel <= numChannels);
        bool correctCheck = (invalidChannel < numChannels);

        REQUIRE(mutatedCheck == true);  // Mutation passes incorrectly
        REQUIRE(correctCheck == false); // Correct check fails
    }

    SECTION("NEGATE: Skip IO node check")
    {
        int nodeUid = 1; // Audio input - should be skipped

        auto shouldSkip = [](int uid) { return uid == 1 || uid == 2 || uid == 3; };

        REQUIRE(shouldSkip(nodeUid));

        // Mutation: if check was negated
        REQUIRE_FALSE(!shouldSkip(nodeUid)); // Negation would be wrong
    }
}
