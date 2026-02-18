/**
 * @file audio_thread_stress_test.cpp
 * @brief Audio thread stress tests for detecting race conditions and thread safety issues
 *
 * These tests simulate high-frequency audio callbacks with concurrent parameter
 * changes to catch:
 * 1. Data races between audio and UI threads
 * 2. Lock contention issues
 * 3. State corruption under concurrent access
 * 4. Memory ordering bugs
 *
 * Run with Thread Sanitizer (TSan) for best results:
 *   cmake -DCMAKE_CXX_FLAGS="-fsanitize=thread" ...
 */

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>

#include <atomic>
#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <cmath>
#include <random>
#include <thread>
#include <vector>


// =============================================================================
// Mock Processor for stress testing audio thread patterns
// =============================================================================

class StressTestProcessor : public juce::AudioProcessor
{
  public:
    StressTestProcessor()
        : AudioProcessor(BusesProperties()
                             .withInput("Input", juce::AudioChannelSet::stereo())
                             .withOutput("Output", juce::AudioChannelSet::stereo()))
    {
    }

    const juce::String getName() const override { return "StressTestProcessor"; }

    void prepareToPlay(double sampleRate, int samplesPerBlock) override
    {
        currentSampleRate.store(sampleRate, std::memory_order_relaxed);
        blockSize.store(samplesPerBlock, std::memory_order_relaxed);
        prepared.store(true, std::memory_order_release);
    }

    void releaseResources() override { prepared.store(false, std::memory_order_release); }

    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) override
    {
        // Simulate real processing with parameter reads
        float gain = gainValue.load(std::memory_order_relaxed);
        bool bypassed = bypass.load(std::memory_order_relaxed);

        processBlockCount.fetch_add(1, std::memory_order_relaxed);

        if (bypassed)
            return;

        // Apply gain to verify audio path works
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            auto* data = buffer.getWritePointer(ch);
            for (int i = 0; i < buffer.getNumSamples(); ++i)
            {
                data[i] *= gain;
            }
        }
    }

    // Thread-safe parameter setters (simulating UI thread calls)
    void setGain(float newGain) { gainValue.store(newGain, std::memory_order_relaxed); }
    void setBypass(bool newBypass) { bypass.store(newBypass, std::memory_order_relaxed); }

    // Accessors for verification
    int getProcessBlockCount() const { return processBlockCount.load(std::memory_order_relaxed); }
    bool isPrepared() const { return prepared.load(std::memory_order_acquire); }

    // Required overrides
    double getTailLengthSeconds() const override { return 0.0; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    juce::AudioProcessorEditor* createEditor() override { return nullptr; }
    bool hasEditor() const override { return false; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override {}
    void setStateInformation(const void*, int) override {}

  private:
    std::atomic<float> gainValue{1.0f};
    std::atomic<bool> bypass{false};
    std::atomic<bool> prepared{false};
    std::atomic<double> currentSampleRate{44100.0};
    std::atomic<int> blockSize{512};
    std::atomic<int> processBlockCount{0};
};

// =============================================================================
// Stress Tests
// =============================================================================

TEST_CASE("Audio Thread Stress - Concurrent Parameter Changes", "[audio][stress][threading]")
{
    StressTestProcessor processor;
    processor.prepareToPlay(44100.0, 512);

    juce::AudioBuffer<float> buffer(2, 512);
    juce::MidiBuffer midi;

    // Fill buffer with test signal
    for (int ch = 0; ch < 2; ++ch)
    {
        auto* data = buffer.getWritePointer(ch);
        for (int i = 0; i < 512; ++i)
            data[i] = 0.5f;
    }

    std::atomic<bool> running{true};
    std::atomic<int> parameterChanges{0};

    SECTION("Rapid parameter changes during processBlock")
    {
        // Simulate UI thread changing parameters
        std::thread uiThread(
            [&]()
            {
                std::mt19937 rng(42);
                std::uniform_real_distribution<float> gainDist(0.0f, 1.0f);

                while (running.load(std::memory_order_relaxed))
                {
                    processor.setGain(gainDist(rng));
                    processor.setBypass(rng() % 2 == 0);
                    parameterChanges.fetch_add(1, std::memory_order_relaxed);
                    // No sleep - maximum stress
                }
            });

        // Simulate audio thread calling processBlock rapidly
        auto startTime = std::chrono::steady_clock::now();
        int iterations = 0;

        while (std::chrono::steady_clock::now() - startTime < std::chrono::milliseconds(100))
        {
            processor.processBlock(buffer, midi);
            ++iterations;
        }

        running.store(false, std::memory_order_relaxed);
        uiThread.join();

        // Verify we did substantial work
        REQUIRE(iterations > 1000);
        REQUIRE(parameterChanges.load() > 1000);
        REQUIRE(processor.getProcessBlockCount() == iterations);

        INFO("Audio blocks processed: " << iterations);
        INFO("Parameter changes: " << parameterChanges.load());
    }

    SECTION("Bypass toggling stress test")
    {
        std::atomic<int> bypassToggles{0};

        std::thread toggleThread(
            [&]()
            {
                while (running.load(std::memory_order_relaxed))
                {
                    processor.setBypass(true);
                    processor.setBypass(false);
                    bypassToggles.fetch_add(2, std::memory_order_relaxed);
                }
            });

        // Process blocks while bypass is being toggled
        for (int i = 0; i < 10000; ++i)
        {
            processor.processBlock(buffer, midi);
        }

        running.store(false, std::memory_order_relaxed);
        toggleThread.join();

        REQUIRE(bypassToggles.load() > 1000);
        REQUIRE(processor.getProcessBlockCount() == 10000);
    }

    processor.releaseResources();
}

TEST_CASE("Audio Thread Stress - Buffer Integrity", "[audio][stress][buffer]")
{
    StressTestProcessor processor;
    processor.prepareToPlay(44100.0, 512);

    SECTION("Output buffer contains valid samples after processing")
    {
        juce::AudioBuffer<float> buffer(2, 512);
        juce::MidiBuffer midi;

        // Fill with known signal
        for (int ch = 0; ch < 2; ++ch)
        {
            auto* data = buffer.getWritePointer(ch);
            for (int i = 0; i < 512; ++i)
                data[i] = 1.0f;
        }

        processor.setGain(0.5f);
        processor.processBlock(buffer, midi);

        // Verify output
        for (int ch = 0; ch < 2; ++ch)
        {
            auto* data = buffer.getReadPointer(ch);
            for (int i = 0; i < 512; ++i)
            {
                REQUIRE(std::abs(data[i] - 0.5f) < 0.0001f);
            }
        }
    }

    SECTION("NaN detection in output")
    {
        juce::AudioBuffer<float> buffer(2, 512);
        juce::MidiBuffer midi;

        // Process many times and check for NaN
        for (int iteration = 0; iteration < 1000; ++iteration)
        {
            // Fill with test signal
            for (int ch = 0; ch < 2; ++ch)
            {
                auto* data = buffer.getWritePointer(ch);
                for (int i = 0; i < 512; ++i)
                    data[i] = 0.5f;
            }

            processor.setGain((float)iteration / 1000.0f);
            processor.processBlock(buffer, midi);

            // Check for NaN/Inf
            for (int ch = 0; ch < 2; ++ch)
            {
                auto* data = buffer.getReadPointer(ch);
                for (int i = 0; i < 512; ++i)
                {
                    REQUIRE_FALSE(std::isnan(data[i]));
                    REQUIRE_FALSE(std::isinf(data[i]));
                }
            }
        }
    }

    processor.releaseResources();
}

TEST_CASE("Audio Thread Stress - PrepareToPlay/ReleaseResources", "[audio][stress][lifecycle]")
{
    StressTestProcessor processor;

    SECTION("Rapid prepare/release cycles")
    {
        for (int i = 0; i < 100; ++i)
        {
            processor.prepareToPlay(44100.0, 512);
            REQUIRE(processor.isPrepared());

            // Process a few blocks
            juce::AudioBuffer<float> buffer(2, 512);
            juce::MidiBuffer midi;
            processor.processBlock(buffer, midi);
            processor.processBlock(buffer, midi);

            processor.releaseResources();
            REQUIRE_FALSE(processor.isPrepared());
        }
    }

    SECTION("Sample rate changes during processing")
    {
        double sampleRates[] = {44100.0, 48000.0, 88200.0, 96000.0, 192000.0};

        for (double sr : sampleRates)
        {
            processor.prepareToPlay(sr, 512);

            juce::AudioBuffer<float> buffer(2, 512);
            juce::MidiBuffer midi;

            for (int i = 0; i < 100; ++i)
            {
                processor.processBlock(buffer, midi);
            }

            processor.releaseResources();
        }

        // Should complete without crashes or hangs
        REQUIRE(true);
    }
}

TEST_CASE("Audio Thread Stress - Memory Ordering", "[audio][stress][memory]")
{
    SECTION("Atomic operations maintain consistency")
    {
        std::atomic<int> counter{0};
        std::atomic<bool> done{false};

        constexpr int numThreads = 4;
        constexpr int incrementsPerThread = 10000;

        std::vector<std::thread> threads;

        for (int t = 0; t < numThreads; ++t)
        {
            threads.emplace_back(
                [&]()
                {
                    for (int i = 0; i < incrementsPerThread; ++i)
                    {
                        counter.fetch_add(1, std::memory_order_relaxed);
                    }
                });
        }

        for (auto& t : threads)
            t.join();

        REQUIRE(counter.load() == numThreads * incrementsPerThread);
    }

    SECTION("Release-acquire synchronization")
    {
        std::atomic<int> data{0};
        std::atomic<bool> ready{false};

        std::thread writer(
            [&]()
            {
                data.store(42, std::memory_order_relaxed);
                ready.store(true, std::memory_order_release);
            });

        // Spin until ready
        while (!ready.load(std::memory_order_acquire))
        {
            std::this_thread::yield();
        }

        REQUIRE(data.load(std::memory_order_relaxed) == 42);

        writer.join();
    }
}

// =============================================================================
// Mutation Tests for Audio Thread Safety Patterns
// =============================================================================

TEST_CASE("Audio Thread Safety Mutation Tests", "[audio][mutation][threading]")
{
    SECTION("Atomic load/store pattern verification")
    {
        std::atomic<float> value{1.0f};

        // This pattern should be consistent
        value.store(0.5f, std::memory_order_relaxed);
        float loaded = value.load(std::memory_order_relaxed);
        REQUIRE(loaded == 0.5f);

        // Mutation: if we used non-atomic, TSan would catch it
        // This test documents the expected pattern
    }

    SECTION("Process block count accuracy")
    {
        StressTestProcessor processor;
        processor.prepareToPlay(44100.0, 512);

        juce::AudioBuffer<float> buffer(2, 512);
        juce::MidiBuffer midi;

        int expected = 0;
        for (int i = 0; i < 100; ++i)
        {
            processor.processBlock(buffer, midi);
            ++expected;
            REQUIRE(processor.getProcessBlockCount() == expected);
        }

        processor.releaseResources();
    }
}
