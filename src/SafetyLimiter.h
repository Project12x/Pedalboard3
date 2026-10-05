/*
  ==============================================================================

    SafetyLimiter.h
    Audio safety protection processor with auto-mute capabilities.

  ==============================================================================
*/

#ifndef SAFETYLIMITER_H_INCLUDED
#define SAFETYLIMITER_H_INCLUDED

#include "dsp/MeterSource.h"
#include "dsp/SafetyLimiterCore.h"

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
    bool isMuted() const { return (muteControl.load(std::memory_order_acquire) & 1u) != 0; }
    bool isLimiting() const { return limiting.load(); }

    // Manual unmute (called from Panic)
    void unmute();
    void requestMute() noexcept;
    uint64_t getResetSerial() const noexcept { return resetSerial.load(std::memory_order_acquire); }

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
        return outputMeters.getPeak(channel);
    }

    // Input level metering (peak with decay, read by UI for Audio Input VU)
    float getInputLevel(int channel) const
    {
        return inputMeters.getPeak(channel);
    }

    // VU-ballistic level (300ms integration, read by UI for VU meter display)
    float getOutputVuLevel(int channel) const
    {
        return outputMeters.getVu(channel);
    }
    float getInputVuLevel(int channel) const
    {
        return inputMeters.getVu(channel);
    }

    float getOutputRmsLevel(int channel) const
    {
        return outputMeters.getRms(channel);
    }
    float getInputRmsLevel(int channel) const
    {
        return inputMeters.getRms(channel);
    }
    bool getOutputClip(int channel) const
    {
        return outputMeters.getClip(channel);
    }
    bool getInputClip(int channel) const
    {
        return inputMeters.getClip(channel);
    }
    bool getOutputAndClearClip(int channel)
    {
        return outputMeters.getAndClearClip(channel);
    }
    bool getInputAndClearClip(int channel)
    {
        return inputMeters.getAndClearClip(channel);
    }

    // Called from MeteringProcessorPlayer after graph processes (RT-safe).
    void updateOutputLevelsFromDevice(const float* const* outputData, int numChannels, int numSamples);
    // Called from MeteringProcessorPlayer before graph processes (RT-safe).
    void updateInputLevelsFromDevice(const float* const* inputData, int numChannels, int numSamples);

    // Static instance accessor for PluginComponent to read output levels
    static SafetyLimiterProcessor* getInstance() { return instance; }
    static void setInstance(SafetyLimiterProcessor* inst) { instance = inst; }

  private:
    SafetyLimiterCore core;
    // Bit 0: muted; bit 1: explicit unlock awaiting the processing thread.
    std::atomic<unsigned> muteControl{0};
    std::atomic<uint64_t> resetSerial{0};
    std::atomic<bool> limiting{false}, muteTriggered{false}, audioActive{false};

    // Explicit peak/RMS/VU/clip metering, updated from device callback.
    PedalboardMeterSource inputMeters;
    PedalboardMeterSource outputMeters;

    static SafetyLimiterProcessor* instance;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SafetyLimiterProcessor)
};

#endif // SAFETYLIMITER_H_INCLUDED
