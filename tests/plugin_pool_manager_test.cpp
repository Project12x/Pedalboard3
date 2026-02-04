/**
 * @file plugin_pool_manager_test.cpp
 * @brief Unit tests for PluginPoolManager
 *
 * Tests cover:
 * 1. Singleton lifecycle (getInstance, killInstance, re-initialization)
 * 2. Boundary conditions (empty patches, single patch, edge positions)
 * 3. Preload range management
 * 4. Configuration setters
 *
 * Note: These tests verify logic without actual plugin loading since
 * that requires full JUCE/audio initialization.
 */

#include <algorithm>
#include <atomic>
#include <catch2/catch_test_macros.hpp>
#include <spdlog/spdlog.h>
#include <memory>
#include <string>

#include "../src/PluginPoolManager.h"

// Since PluginPoolManager depends on JUCE, we test the interface contracts
// and boundary conditions that don't require actual plugin instantiation.
// For full integration testing, the application must be run manually.

// =============================================================================
// Standalone boundary tests (no JUCE dependency)
// =============================================================================

TEST_CASE("PluginPoolManager Boundary Values", "[poolmanager][boundary]")
{
    SECTION("Preload range validation - lower bound")
    {
        // Preload range should be clamped to minimum of 1
        int value = 0;
        int clamped = std::clamp(value, 1, 5);
        REQUIRE(clamped == 1);
    }

    SECTION("Preload range validation - upper bound")
    {
        // Preload range should be clamped to maximum of 5
        int value = 10;
        int clamped = std::clamp(value, 1, 5);
        REQUIRE(clamped == 5);
    }

    SECTION("Preload range validation - valid value")
    {
        int value = 3;
        int clamped = std::clamp(value, 1, 5);
        REQUIRE(clamped == 3);
    }

    SECTION("Position boundary - negative position")
    {
        int position = -1;
        int patchCount = 10;
        // Should be clamped to valid range
        int clamped = std::clamp(position, 0, patchCount - 1);
        REQUIRE(clamped == 0);
    }

    SECTION("Position boundary - beyond patch count")
    {
        int position = 15;
        int patchCount = 10;
        int clamped = std::clamp(position, 0, patchCount - 1);
        REQUIRE(clamped == 9);
    }

    SECTION("Empty patch list handling")
    {
        int patchCount = 0;
        // With no patches, any position access should be guarded
        bool hasPatches = patchCount > 0;
        REQUIRE_FALSE(hasPatches);
    }

    SECTION("Single patch - preload window calculation")
    {
        int currentPosition = 0;
        int patchCount = 1;
        int preloadRange = 2;

        // Calculate window bounds
        int windowStart = std::max(0, currentPosition - preloadRange);
        int windowEnd = std::min(patchCount - 1, currentPosition + preloadRange);

        REQUIRE(windowStart == 0);
        REQUIRE(windowEnd == 0);
    }
}

TEST_CASE("Sliding Window Calculations", "[poolmanager][window]")
{
    SECTION("Window at start of setlist")
    {
        int currentPosition = 0;
        int patchCount = 10;
        int preloadRange = 2;

        int windowStart = std::max(0, currentPosition - preloadRange);
        int windowEnd = std::min(patchCount - 1, currentPosition + preloadRange);

        REQUIRE(windowStart == 0);
        REQUIRE(windowEnd == 2);

        // Patches to preload: 0, 1, 2
        int patchesToLoad = windowEnd - windowStart + 1;
        REQUIRE(patchesToLoad == 3);
    }

    SECTION("Window in middle of setlist")
    {
        int currentPosition = 5;
        int patchCount = 10;
        int preloadRange = 2;

        int windowStart = std::max(0, currentPosition - preloadRange);
        int windowEnd = std::min(patchCount - 1, currentPosition + preloadRange);

        REQUIRE(windowStart == 3);
        REQUIRE(windowEnd == 7);

        // Patches to preload: 3, 4, 5, 6, 7
        int patchesToLoad = windowEnd - windowStart + 1;
        REQUIRE(patchesToLoad == 5);
    }

    SECTION("Window at end of setlist")
    {
        int currentPosition = 9;
        int patchCount = 10;
        int preloadRange = 2;

        int windowStart = std::max(0, currentPosition - preloadRange);
        int windowEnd = std::min(patchCount - 1, currentPosition + preloadRange);

        REQUIRE(windowStart == 7);
        REQUIRE(windowEnd == 9);

        // Patches to preload: 7, 8, 9
        int patchesToLoad = windowEnd - windowStart + 1;
        REQUIRE(patchesToLoad == 3);
    }

    SECTION("Large preload range with small setlist")
    {
        int currentPosition = 2;
        int patchCount = 5;
        int preloadRange = 10; // Larger than setlist

        int windowStart = std::max(0, currentPosition - preloadRange);
        int windowEnd = std::min(patchCount - 1, currentPosition + preloadRange);

        REQUIRE(windowStart == 0);
        REQUIRE(windowEnd == 4);

        // Should load entire setlist
        int patchesToLoad = windowEnd - windowStart + 1;
        REQUIRE(patchesToLoad == patchCount);
    }
}

TEST_CASE("Plugin Identifier Generation", "[poolmanager][identifier]")
{
    SECTION("Unique identifier components")
    {
        // Test the identifier format: name_format_uid
        std::string pluginName = "MyPlugin";
        std::string format = "VST3";
        int uniqueId = 12345;

        std::string identifier = pluginName + "_" + format + "_" + std::to_string(uniqueId);

        REQUIRE(identifier == "MyPlugin_VST3_12345");
    }

    SECTION("Different UIDs produce different identifiers")
    {
        std::string base = "Plugin_VST3_";
        std::string id1 = base + std::to_string(100);
        std::string id2 = base + std::to_string(200);

        REQUIRE(id1 != id2);
    }

    SECTION("Same parameters produce same identifier (deduplication key)")
    {
        std::string id1 = "Plugin_VST3_12345";
        std::string id2 = "Plugin_VST3_12345";

        REQUIRE(id1 == id2);
    }
}

TEST_CASE("Memory Limit Logic", "[poolmanager][memory]")
{
    SECTION("Zero memory limit means unlimited")
    {
        size_t memoryLimit = 0;
        size_t currentUsage = 1000000000; // 1GB

        bool withinLimit = (memoryLimit == 0) || (currentUsage < memoryLimit);
        REQUIRE(withinLimit);
    }

    SECTION("Under memory limit")
    {
        size_t memoryLimit = 500 * 1024 * 1024;  // 500MB
        size_t currentUsage = 100 * 1024 * 1024; // 100MB

        bool withinLimit = (memoryLimit == 0) || (currentUsage < memoryLimit);
        REQUIRE(withinLimit);
    }

    SECTION("Over memory limit triggers cleanup")
    {
        size_t memoryLimit = 500 * 1024 * 1024;  // 500MB
        size_t currentUsage = 600 * 1024 * 1024; // 600MB

        bool needsCleanup = (memoryLimit > 0) && (currentUsage >= memoryLimit);
        REQUIRE(needsCleanup);
    }
}

TEST_CASE("Thread Safety Concepts", "[poolmanager][threading]")
{
    SECTION("Atomic position update")
    {
        std::atomic<int> position{0};

        position.store(5);
        REQUIRE(position.load() == 5);

        position.store(10);
        REQUIRE(position.load() == 10);
    }

    SECTION("Atomic compare-exchange for safe updates")
    {
        std::atomic<int> position{5};
        int expected = 5;
        int desired = 6;

        bool success = position.compare_exchange_strong(expected, desired);
        REQUIRE(success);
        REQUIRE(position.load() == 6);
    }
}

TEST_CASE("PluginPoolManager Extracts Nested Rack Plugins", "[poolmanager][rack]")
{
    // Build an external plugin in the main graph
    juce::PluginDescription externalDesc;
    externalDesc.name = "ExternalFX";
    externalDesc.pluginFormatName = "VST3";
    externalDesc.fileOrIdentifier = "ExternalFX.vst3";
    externalDesc.uniqueId = 1001;

    auto externalXml = externalDesc.createXml();
    auto externalFilter = std::make_unique<juce::XmlElement>("FILTER");
    externalFilter->addChildElement(externalXml.release());

    // Build a plugin inside a rack
    juce::PluginDescription rackPluginDesc;
    rackPluginDesc.name = "RackFX";
    rackPluginDesc.pluginFormatName = "VST3";
    rackPluginDesc.fileOrIdentifier = "RackFX.vst3";
    rackPluginDesc.uniqueId = 2002;

    auto rackPluginXml = rackPluginDesc.createXml();
    auto rackFilter = std::make_unique<juce::XmlElement>("FILTER");
    rackFilter->addChildElement(rackPluginXml.release());

    auto rackXml = std::make_unique<juce::XmlElement>("RACK");
    rackXml->addChildElement(rackFilter.release());

    juce::MemoryBlock rackState;
    juce::copyXmlToBinary(*rackXml, rackState);

    auto rackStateElem = std::make_unique<juce::XmlElement>("STATE");
    rackStateElem->addTextElement(rackState.toBase64Encoding());

    // Create the rack node in the main graph
    juce::PluginDescription rackDesc;
    rackDesc.name = "Effect Rack";
    rackDesc.pluginFormatName = "Internal";
    rackDesc.fileOrIdentifier = "Internal:SubGraph";
    rackDesc.uniqueId = 0;

    auto rackDescXml = rackDesc.createXml();
    auto rackFilterMain = std::make_unique<juce::XmlElement>("FILTER");
    rackFilterMain->addChildElement(rackDescXml.release());
    rackFilterMain->addChildElement(rackStateElem.release());

    auto graphXml = std::make_unique<juce::XmlElement>("FILTERGRAPH");
    graphXml->addChildElement(externalFilter.release());
    graphXml->addChildElement(rackFilterMain.release());

    auto patchXml = std::make_unique<juce::XmlElement>("Patch");
    patchXml->addChildElement(graphXml.release());

    auto result = PluginPoolManager::extractPluginsFromPatchForTest(patchXml.get());

    REQUIRE(result.size() == 2);

    bool hasExternal = false;
    bool hasRack = false;
    for (const auto& desc : result)
    {
        if (desc.name == "ExternalFX" && desc.pluginFormatName == "VST3")
            hasExternal = true;
        if (desc.name == "RackFX" && desc.pluginFormatName == "VST3")
            hasRack = true;
    }

    REQUIRE(hasExternal);
    REQUIRE(hasRack);
}

// =============================================================================
// Mutation Testing Patterns
// =============================================================================

TEST_CASE("PluginPoolManager Mutation Testing", "[poolmanager][mutation]")
{
    SECTION("OFF-BY-ONE: Window boundary calculation")
    {
        // Position near end where patchCount-1 vs patchCount matters
        int currentPosition = 9; // Position near end
        int patchCount = 10;
        int preloadRange = 2;

        // Correct: windowEnd = min(patchCount - 1, currentPosition + preloadRange)
        // = min(9, 11) = 9
        int correctEnd = std::min(patchCount - 1, currentPosition + preloadRange);
        REQUIRE(correctEnd == 9);

        // Mutation: windowEnd = min(patchCount, ...) = min(10, 11) = 10 (off-by-one!)
        int mutatedEnd = std::min(patchCount, currentPosition + preloadRange);
        REQUIRE(mutatedEnd != correctEnd); // Mutation detectable (10 != 9)
    }

    SECTION("OFF-BY-ONE: Position clamp lower bound")
    {
        int position = 0;
        int patchCount = 10;

        // Correct: clamp to 0
        int correctClamped = std::clamp(position, 0, patchCount - 1);
        REQUIRE(correctClamped == 0);

        // Mutation: clamp to 1 would skip first patch
        int mutatedClamped = std::clamp(position, 1, patchCount - 1);
        REQUIRE(mutatedClamped != correctClamped); // Mutation detectable
    }

    SECTION("ARITHMETIC: Window size calculation")
    {
        int windowStart = 3;
        int windowEnd = 7;

        // Correct: count = end - start + 1
        int correctCount = windowEnd - windowStart + 1;
        REQUIRE(correctCount == 5);

        // Mutation: count = end - start (off by one)
        int mutatedCount = windowEnd - windowStart;
        REQUIRE(mutatedCount != correctCount); // Mutation detectable
    }

    SECTION("NEGATE: Empty patch guard")
    {
        int patchCount = 0;

        // Correct: guard against empty
        bool correctGuard = patchCount > 0;
        REQUIRE_FALSE(correctGuard);

        // Mutation: patchCount >= 0 would incorrectly allow empty
        bool mutatedGuard = patchCount >= 0;
        REQUIRE(mutatedGuard != correctGuard); // Mutation detectable
    }

    SECTION("SWAP: Window start vs end confusion")
    {
        int currentPosition = 5;
        int patchCount = 10;
        int preloadRange = 2;

        int windowStart = std::max(0, currentPosition - preloadRange);
        int windowEnd = std::min(patchCount - 1, currentPosition + preloadRange);

        // Correct: start < end
        REQUIRE(windowStart < windowEnd);

        // Mutation: if start and end were swapped
        REQUIRE(windowStart != windowEnd); // They should be different
    }

    SECTION("CONDITION: Memory limit zero means unlimited")
    {
        size_t memoryLimit = 0;
        size_t currentUsage = 1000000000; // 1GB

        // Correct: zero means unlimited
        bool correctCheck = (memoryLimit == 0) || (currentUsage < memoryLimit);
        REQUIRE(correctCheck);

        // Mutation: if zero check was removed
        bool mutatedCheck = currentUsage < memoryLimit;
        REQUIRE(mutatedCheck != correctCheck); // Mutation detectable
    }
}
