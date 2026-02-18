/**
 * @file vst3_loading_test.cpp
 * @brief Headless test to reproduce VST3 plugin loading crash
 *
 * This test loads a real VST3 plugin (Surge XT or any available VST3),
 * wraps it in BypassableInstance, adds it to an AudioProcessorGraph,
 * and runs processBlock to reproduce the crash that occurs when
 * VST3 plugins are loaded in the full application.
 *
 * Tests are skipped if no VST3 plugins are found on the system.
 */

#include "../src/AudioSingletons.h"
#include "../src/BypassableInstance.h"

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>

#include <atomic>
#include <catch2/catch_test_macros.hpp>
#include <spdlog/spdlog.h>
#include <thread>
#include <vector>

// Common VST3 search paths on Windows
static juce::StringArray getVST3SearchPaths()
{
    juce::StringArray paths;
    paths.add("C:\\Program Files\\Common Files\\VST3");
    paths.add("C:\\Program Files (x86)\\Common Files\\VST3");

    // User-local VST3 folder
    auto appData = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory);
    paths.add(appData.getChildFile("VST3").getFullPathName());

    return paths;
}

// Find a VST3 plugin file on the system. Prefers Surge XT but falls back to any VST3.
static juce::File findVST3Plugin()
{
    auto searchPaths = getVST3SearchPaths();

    // Preferred plugins to test with (known to trigger the crash)
    juce::StringArray preferred = {"Surge XT.vst3", "VAPOR KEYS.vst3"};

    for (auto& name : preferred)
    {
        for (auto& searchPath : searchPaths)
        {
            juce::File dir(searchPath);
            if (!dir.isDirectory())
                continue;
            auto found = dir.getChildFile(name);
            if (found.exists())
                return found;
        }
    }

    // Fall back to any VST3
    for (auto& searchPath : searchPaths)
    {
        juce::File dir(searchPath);
        if (!dir.isDirectory())
            continue;
        auto results = dir.findChildFiles(juce::File::findFilesAndDirectories, false, "*.vst3");
        if (!results.isEmpty())
            return results[0];
    }

    return {};
}

// Helper to create a VST3 plugin instance using the project's format manager singleton
static std::unique_ptr<juce::AudioPluginInstance> createVST3Instance(const juce::File& vst3File,
                                                                     double sampleRate = 44100.0,
                                                                     int blockSize = 512)
{
    auto& formatManager = AudioPluginFormatManagerSingleton::getInstance();

    // Find all plugin types in the VST3 file
    juce::OwnedArray<juce::PluginDescription> descriptions;
    for (int i = 0; i < formatManager.getNumFormats(); ++i)
    {
        auto* format = formatManager.getFormat(i);
        format->findAllTypesForFile(descriptions, vst3File.getFullPathName());
        if (!descriptions.isEmpty())
            break;
    }

    if (descriptions.isEmpty())
    {
        spdlog::warn("[vst3_test] No plugin descriptions found in: {}", vst3File.getFullPathName().toStdString());
        return nullptr;
    }

    juce::String errorMessage;
    auto instance = formatManager.createPluginInstance(*descriptions[0], sampleRate, blockSize, errorMessage);

    if (!instance)
    {
        spdlog::warn("[vst3_test] Failed to create instance: {}", errorMessage.toStdString());
    }

    return instance;
}

// =============================================================================
// Test Cases
// =============================================================================

TEST_CASE("VST3 Loading - Basic Load and ProcessBlock", "[vst3][loading]")
{
    auto vst3File = findVST3Plugin();
    if (!vst3File.exists())
    {
        WARN("No VST3 plugins found on system - skipping test");
        return;
    }

    spdlog::info("[vst3_test] Testing with: {}", vst3File.getFullPathName().toStdString());

    SECTION("Direct plugin processBlock without graph")
    {
        auto instance = createVST3Instance(vst3File);
        REQUIRE(instance != nullptr);

        spdlog::info("[vst3_test] Plugin: '{}', inputs={}, outputs={}",
                     instance->getName().toStdString(),
                     instance->getTotalNumInputChannels(),
                     instance->getTotalNumOutputChannels());

        // Configure stereo layout
        juce::AudioProcessor::BusesLayout stereoLayout;
        stereoLayout.inputBuses.add(juce::AudioChannelSet::stereo());
        stereoLayout.outputBuses.add(juce::AudioChannelSet::stereo());
        if (instance->checkBusesLayoutSupported(stereoLayout))
            instance->setBusesLayout(stereoLayout);

        // Prepare
        instance->prepareToPlay(44100.0, 512);

        // Process several blocks
        juce::AudioBuffer<float> buffer(2, 512);
        juce::MidiBuffer midi;

        for (int i = 0; i < 100; ++i)
        {
            buffer.clear();
            instance->processBlock(buffer, midi);
        }

        spdlog::info("[vst3_test] Direct processBlock: 100 blocks OK");

        instance->releaseResources();
    }

    SECTION("BypassableInstance wrapping and processBlock")
    {
        auto instance = createVST3Instance(vst3File);
        REQUIRE(instance != nullptr);

        // Configure stereo layout before wrapping
        juce::AudioProcessor::BusesLayout stereoLayout;
        stereoLayout.inputBuses.add(juce::AudioChannelSet::stereo());
        stereoLayout.outputBuses.add(juce::AudioChannelSet::stereo());
        if (instance->checkBusesLayoutSupported(stereoLayout))
            instance->setBusesLayout(stereoLayout);

        spdlog::info("[vst3_test] Wrapping in BypassableInstance...");
        auto* bypassable = new BypassableInstance(instance.release());

        spdlog::info("[vst3_test] Calling prepareToPlay...");
        bypassable->prepareToPlay(44100.0, 512);

        // Process several blocks
        juce::AudioBuffer<float> buffer(2, 512);
        juce::MidiBuffer midi;

        for (int i = 0; i < 100; ++i)
        {
            buffer.clear();
            bypassable->processBlock(buffer, midi);
        }

        spdlog::info("[vst3_test] BypassableInstance processBlock: 100 blocks OK");

        bypassable->releaseResources();
        delete bypassable;
    }
}

TEST_CASE("VST3 Loading - AudioProcessorGraph Integration", "[vst3][graph]")
{
    auto vst3File = findVST3Plugin();
    if (!vst3File.exists())
    {
        WARN("No VST3 plugins found on system - skipping test");
        return;
    }

    spdlog::info("[vst3_test] Graph test with: {}", vst3File.getFullPathName().toStdString());

    SECTION("Add to graph and process - matches production code path")
    {
        // Create graph (matches FilterGraph setup in production)
        juce::AudioProcessorGraph graph;
        graph.prepareToPlay(44100.0, 512);

        // Create plugin instance
        auto instance = createVST3Instance(vst3File);
        REQUIRE(instance != nullptr);

        // Configure stereo layout
        juce::AudioProcessor::BusesLayout stereoLayout;
        stereoLayout.inputBuses.add(juce::AudioChannelSet::stereo());
        stereoLayout.outputBuses.add(juce::AudioChannelSet::stereo());
        if (instance->checkBusesLayoutSupported(stereoLayout))
            instance->setBusesLayout(stereoLayout);

        // Wrap in BypassableInstance (matches addFilterRaw)
        auto bypassable = std::make_unique<BypassableInstance>(instance.release());

        spdlog::info("[vst3_test] Adding to graph under callback lock...");

        // Add to graph under callback lock (matches addFilterRaw)
        juce::AudioProcessorGraph::Node::Ptr node;
        {
            const juce::ScopedLock sl(graph.getCallbackLock());
            node = graph.addNode(std::move(bypassable));
        }

        REQUIRE(node != nullptr);
        spdlog::info("[vst3_test] Node added, ID={}", node->nodeID.uid);

        // Process graph (simulates audio thread calling processBlock)
        juce::AudioBuffer<float> buffer(2, 512);
        juce::MidiBuffer midi;

        spdlog::info("[vst3_test] Processing 100 blocks through graph...");
        for (int i = 0; i < 100; ++i)
        {
            buffer.clear();
            graph.processBlock(buffer, midi);
        }

        spdlog::info("[vst3_test] Graph processBlock: 100 blocks OK");

        // Clean up
        graph.releaseResources();
    }
}

TEST_CASE("VST3 Loading - Concurrent Audio and UI Access", "[vst3][threading][crash]")
{
    auto vst3File = findVST3Plugin();
    if (!vst3File.exists())
    {
        WARN("No VST3 plugins found on system - skipping test");
        return;
    }

    spdlog::info("[vst3_test] Concurrent access test with: {}", vst3File.getFullPathName().toStdString());

    SECTION("Simulate audio thread while UI queries plugin state")
    {
        // Create graph
        juce::AudioProcessorGraph graph;
        graph.prepareToPlay(44100.0, 512);

        // Create and add plugin
        auto instance = createVST3Instance(vst3File);
        REQUIRE(instance != nullptr);

        juce::AudioProcessor::BusesLayout stereoLayout;
        stereoLayout.inputBuses.add(juce::AudioChannelSet::stereo());
        stereoLayout.outputBuses.add(juce::AudioChannelSet::stereo());
        if (instance->checkBusesLayoutSupported(stereoLayout))
            instance->setBusesLayout(stereoLayout);

        auto bypassable = std::make_unique<BypassableInstance>(instance.release());
        auto* bypassablePtr = bypassable.get();

        juce::AudioProcessorGraph::Node::Ptr node;
        {
            const juce::ScopedLock sl(graph.getCallbackLock());
            node = graph.addNode(std::move(bypassable));
        }

        REQUIRE(node != nullptr);

        // Run audio thread and UI thread concurrently
        // This reproduces the race condition that causes the crash
        std::atomic<bool> running{true};
        std::atomic<int> audioBlocks{0};
        std::atomic<int> uiQueries{0};

        // Audio thread: process blocks
        std::thread audioThread([&]() {
            juce::AudioBuffer<float> buffer(2, 512);
            juce::MidiBuffer midi;

            while (running.load(std::memory_order_relaxed))
            {
                buffer.clear();
                graph.processBlock(buffer, midi);
                audioBlocks.fetch_add(1, std::memory_order_relaxed);
            }
        });

        // Simulate UI thread: query plugin state (this is what
        // PluginComponent::determineSize does in production)
        std::thread uiThread([&]() {
            while (running.load(std::memory_order_relaxed))
            {
                auto* proc = node->getProcessor();

                // These are the kinds of queries determineSize() makes
                auto name = proc->getName();
                int numInputs = proc->getTotalNumInputChannels();
                int numOutputs = proc->getTotalNumOutputChannels();
                bool midi = proc->acceptsMidi();
                bool midiOut = proc->producesMidi();

                // Query bus info (this was triggering crashes)
                for (int busIdx = 0; busIdx < proc->getBusCount(true); ++busIdx)
                {
                    if (auto* bus = proc->getBus(true, busIdx))
                    {
                        auto layout = bus->getCurrentLayout();
                        for (int ch = 0; ch < layout.size(); ++ch)
                        {
                            auto chName = layout.getChannelTypeName(layout.getTypeOfChannel(ch));
                            (void)chName;
                        }
                    }
                }

                for (int busIdx = 0; busIdx < proc->getBusCount(false); ++busIdx)
                {
                    if (auto* bus = proc->getBus(false, busIdx))
                    {
                        auto layout = bus->getCurrentLayout();
                        for (int ch = 0; ch < layout.size(); ++ch)
                        {
                            auto chName = layout.getChannelTypeName(layout.getTypeOfChannel(ch));
                            (void)chName;
                        }
                    }
                }

                uiQueries.fetch_add(1, std::memory_order_relaxed);
                (void)numInputs;
                (void)numOutputs;
                (void)midi;
                (void)midiOut;
            }
        });

        // Run for 2 seconds
        std::this_thread::sleep_for(std::chrono::seconds(2));
        running.store(false, std::memory_order_relaxed);

        audioThread.join();
        uiThread.join();

        spdlog::info("[vst3_test] Concurrent test completed: {} audio blocks, {} UI queries",
                     audioBlocks.load(), uiQueries.load());

        REQUIRE(audioBlocks.load() > 0);
        REQUIRE(uiQueries.load() > 0);

        graph.releaseResources();
    }

    SECTION("Simulate audio thread while reading cached channel info (safe path)")
    {
        // This tests the fixed code path that uses cached channel info
        // instead of querying the plugin directly
        juce::AudioProcessorGraph graph;
        graph.prepareToPlay(44100.0, 512);

        auto instance = createVST3Instance(vst3File);
        REQUIRE(instance != nullptr);

        juce::AudioProcessor::BusesLayout stereoLayout;
        stereoLayout.inputBuses.add(juce::AudioChannelSet::stereo());
        stereoLayout.outputBuses.add(juce::AudioChannelSet::stereo());
        if (instance->checkBusesLayoutSupported(stereoLayout))
            instance->setBusesLayout(stereoLayout);

        auto bypassable = std::make_unique<BypassableInstance>(instance.release());
        auto* bypassablePtr = bypassable.get();

        juce::AudioProcessorGraph::Node::Ptr node;
        {
            const juce::ScopedLock sl(graph.getCallbackLock());
            node = graph.addNode(std::move(bypassable));
        }

        REQUIRE(node != nullptr);

        std::atomic<bool> running{true};
        std::atomic<int> audioBlocks{0};
        std::atomic<int> uiQueries{0};

        // Audio thread
        std::thread audioThread([&]() {
            juce::AudioBuffer<float> buffer(2, 512);
            juce::MidiBuffer midi;

            while (running.load(std::memory_order_relaxed))
            {
                buffer.clear();
                graph.processBlock(buffer, midi);
                audioBlocks.fetch_add(1, std::memory_order_relaxed);
            }
        });

        // UI thread using CACHED data (safe - no plugin queries)
        std::thread uiThread([&]() {
            while (running.load(std::memory_order_relaxed))
            {
                // Use cached data instead of querying plugin directly
                int numInputs = bypassablePtr->getCachedInputChannelCount();
                int numOutputs = bypassablePtr->getCachedOutputChannelCount();
                bool midi = bypassablePtr->getCachedAcceptsMidi();
                bool midiOut = bypassablePtr->getCachedProducesMidi();

                for (int ch = 0; ch < numInputs; ++ch)
                {
                    auto name = bypassablePtr->getCachedInputChannelName(ch);
                    (void)name;
                }
                for (int ch = 0; ch < numOutputs; ++ch)
                {
                    auto name = bypassablePtr->getCachedOutputChannelName(ch);
                    (void)name;
                }

                uiQueries.fetch_add(1, std::memory_order_relaxed);
                (void)numInputs;
                (void)numOutputs;
                (void)midi;
                (void)midiOut;
            }
        });

        // Run for 2 seconds
        std::this_thread::sleep_for(std::chrono::seconds(2));
        running.store(false, std::memory_order_relaxed);

        audioThread.join();
        uiThread.join();

        spdlog::info("[vst3_test] Cached data concurrent test: {} audio blocks, {} UI queries",
                     audioBlocks.load(), uiQueries.load());

        REQUIRE(audioBlocks.load() > 0);
        REQUIRE(uiQueries.load() > 0);

        graph.releaseResources();
    }
}
