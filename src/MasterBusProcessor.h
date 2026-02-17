/*
  ==============================================================================

    MasterBusProcessor.h
    Lightweight manager for the master bus insert rack.
    Wraps a SubGraphProcessor to process all audio at the device callback level.

    Lifecycle:
      - Created once at startup via MasterGainState
      - prepare()/release() called from MeteringProcessorPlayer
      - processBlock() called in the device audio callback
      - State persisted globally via SettingsManager (not per-patch)

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <atomic>
#include <memory>

class SubGraphProcessor;

/**
    Master bus insert rack processor.

    Owns a SubGraphProcessor that runs in the device audio callback,
    processing ALL output audio between the graph and the output gain stage.
    Thread-safe: processBlock is lock-free, prepare/release are called from
    the device thread, state save/load from the message thread.
*/
class MasterBusProcessor
{
  public:
    MasterBusProcessor();
    ~MasterBusProcessor();

    // Audio lifecycle - called from MeteringProcessorPlayer
    void prepare(double sampleRate, int samplesPerBlock);
    void release();

    // Process audio in-place. Called from device audio callback.
    // Buffer contains interleaved output from the graph.
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi);

    // Bypass the insert rack (pass-through)
    void setBypass(bool shouldBypass) { bypassed.store(shouldBypass, std::memory_order_relaxed); }
    bool isBypassed() const { return bypassed.load(std::memory_order_relaxed); }

    // Access the internal SubGraphProcessor for UI (opening rack editor)
    SubGraphProcessor* getRack() { return rack.get(); }
    const SubGraphProcessor* getRack() const { return rack.get(); }

    // State persistence (called from MasterGainState save/load)
    void saveState(juce::MemoryBlock& destData) const;
    void restoreState(const void* data, int sizeInBytes);

    // Check if the rack has any plugins inserted
    bool hasPlugins() const;

  private:
    std::unique_ptr<SubGraphProcessor> rack;
    std::atomic<bool> bypassed{false};
    std::atomic<bool> prepared{false};

    double currentSampleRate = 44100.0;
    int currentBlockSize = 512;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MasterBusProcessor)
};
