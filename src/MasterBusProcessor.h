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

    RT Safety:
      - hasPluginsCached is std::atomic<bool>, updated from message thread
        via ChangeListener when the internal graph changes.
      - Audio thread only reads atomics, never touches the graph node list.

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
class MasterBusProcessor : private juce::ChangeListener
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

    // RT-safe check: does the rack have user-inserted plugins?
    // Reads a cached atomic flag, NOT the graph's node list.
    bool hasPluginsCached() const { return hasPluginsFlag.load(std::memory_order_acquire); }

    // Message-thread check: reads the graph node list directly.
    // Use for UI/state logic only - NOT from the audio thread.
    bool hasPlugins() const;

    // Access the internal SubGraphProcessor for UI (opening rack editor)
    SubGraphProcessor* getRack() { return rack.get(); }
    const SubGraphProcessor* getRack() const { return rack.get(); }

    // State persistence (called from MasterGainState save/load)
    void saveState(juce::MemoryBlock& destData) const;
    void restoreState(const void* data, int sizeInBytes);

  private:
    // ChangeListener - called on message thread when graph nodes change
    void changeListenerCallback(juce::ChangeBroadcaster* source) override;

    // Recalculate hasPluginsFlag from the graph (message thread only)
    void refreshHasPluginsFlag();

    std::unique_ptr<SubGraphProcessor> rack;
    std::atomic<bool> bypassed{false};
    std::atomic<bool> prepared{false};
    std::atomic<bool> hasPluginsFlag{false}; // RT-safe cached plugin state

    double currentSampleRate = 44100.0;
    int currentBlockSize = 512;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MasterBusProcessor)
};
