/*
  ==============================================================================

    SafetyLimiter.h
    Audio safety protection processor with auto-mute capabilities.

  ==============================================================================
*/

#ifndef SAFETYLIMITER_H_INCLUDED
#define SAFETYLIMITER_H_INCLUDED

#include "VuMeterDsp.h"

#include <JuceHeader.h>
#include <atomic>


/**
    SafetyLimiterProcessor

    Final output protection that:
    - Soft-limits peaks above -0.5 dBFS
    - Auto-mutes on sustained dangerous levels (+6 dBFS for 100ms)
    - Auto-mutes on DC offset (>0.5 for 500ms)
    - Auto-mutes on sustained ultrasonic content (>18kHz)
    - Requires manual unmute via Panic command
*/
class SafetyLimiterProcessor : public AudioProcessor
{
  public:
    SafetyLimiterProcessor();
    ~SafetyLimiterProcessor() override = default;

    //==============================================================================
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(AudioBuffer<float>& buffer, MidiBuffer& midiMessages) override;

    //==============================================================================
    const String getName() const override { return "SafetyLimiter"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const String getProgramName(int) override { return {}; }
    void changeProgramName(int, const String&) override {}

    void getStateInformation(MemoryBlock&) override {}
    void setStateInformation(const void*, int) override {}

    bool hasEditor() const override { return false; }
    AudioProcessorEditor* createEditor() override { return nullptr; }

    //==============================================================================
    // Safety state queries (thread-safe)
    bool isMuted() const { return muted.load(); }
    bool isLimiting() const { return limiting.load(); }

    // Manual unmute (called from Panic)
    void unmute() { muted.store(false); }

    // Check if mute was triggered since last check (for toast notification)
    bool checkAndClearMuteTriggered()
    {
        bool expected = true;
        return muteTriggered.compare_exchange_strong(expected, false);
    }

    // Audio activity detection for wire glow
    bool isAudioActive() const { return audioActive.load(); }

    // Output level metering (peak with decay, read by UI for Audio Output VU)
    float getOutputLevel(int channel) const
    {
        if (channel >= 0 && channel < 2)
            return outputLevels[channel].load(std::memory_order_relaxed);
        return 0.0f;
    }

    // Input level metering (peak with decay, read by UI for Audio Input VU)
    float getInputLevel(int channel) const
    {
        if (channel >= 0 && channel < 2)
            return inputLevels[channel].load(std::memory_order_relaxed);
        return 0.0f;
    }

    // VU-ballistic level (300ms integration, read by UI for VU meter display)
    float getOutputVuLevel(int channel) const
    {
        if (channel >= 0 && channel < 2)
            return outputVuLevels[channel].load(std::memory_order_relaxed);
        return 0.0f;
    }
    float getInputVuLevel(int channel) const
    {
        if (channel >= 0 && channel < 2)
            return inputVuLevels[channel].load(std::memory_order_relaxed);
        return 0.0f;
    }

    // Called from MeteringProcessorPlayer after graph processes (RT-safe).
    void updateOutputLevelsFromDevice(const float* const* outputData, int numChannels, int numSamples);
    // Called from MeteringProcessorPlayer before graph processes (RT-safe).
    void updateInputLevelsFromDevice(const float* const* inputData, int numChannels, int numSamples);

    // Static instance accessor for PluginComponent to read output levels
    static SafetyLimiterProcessor* getInstance() { return instance; }
    static void setInstance(SafetyLimiterProcessor* inst) { instance = inst; }

  private:
    //==============================================================================
    // Thresholds
    static constexpr float softLimitThreshold = 0.944f;   // -0.5 dBFS
    static constexpr float dangerousGainThreshold = 2.0f; // +6 dBFS
    static constexpr float dcOffsetThreshold = 0.5f;

    // Timing (in samples, set in prepareToPlay)
    int dangerousGainHoldSamples = 0; // 100ms
    int dcOffsetHoldSamples = 0;      // 500ms
    int ultrasonicHoldSamples = 0;    // 200ms

    // State
    std::atomic<bool> muted{false};
    std::atomic<bool> limiting{false};
    std::atomic<bool> muteTriggered{false};
    std::atomic<bool> audioActive{false}; // Set when audio is flowing

    // Detection counters
    int dangerousGainCounter = 0;
    int dcOffsetCounter = 0;
    int ultrasonicCounter = 0;

    // DC blocker state (per channel)
    float dcBlockerState[2] = {0.0f, 0.0f};
    float dcBlockerCoeff = 0.995f;

    // Ultrasonic detection (simple high-pass energy tracker)
    float ultrasonicEnergy = 0.0f;
    float ultrasonicDecay = 0.99f;

    // Limiter state
    float currentGain = 1.0f;
    float releaseCoeff = 0.0f; // Calculated in prepareToPlay

    double currentSampleRate = 44100.0;

    // Level metering (per-channel peak with decay, updated from device callback)
    std::atomic<float> outputLevels[2] = {{0.0f}, {0.0f}};
    std::atomic<float> inputLevels[2] = {{0.0f}, {0.0f}};
    float outputDecayCoeff = 0.9995f; // ~300ms decay at 44100Hz, refined in prepareToPlay

    // VU meter DSP (2-pole lowpass, 300ms integration per IEC 60268-17)
    VuMeterDsp inputVu[2];
    VuMeterDsp outputVu[2];
    std::atomic<float> inputVuLevels[2] = {{0.0f}, {0.0f}};
    std::atomic<float> outputVuLevels[2] = {{0.0f}, {0.0f}};

    static SafetyLimiterProcessor* instance;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SafetyLimiterProcessor)
};

#endif // SAFETYLIMITER_H_INCLUDED
