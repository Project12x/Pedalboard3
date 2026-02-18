//  DawMixerProcessor.h - DAW-style N-channel mixing console node.
//  --------------------------------------------------------------------------
//  This file is part of Pedalboard3, an audio plugin host.
//
//  Dynamic mono strip mixer with per-strip gain, pan, mute, solo, phase
//  invert, and VU metering. Strips are added/removed at runtime via the UI.
//
//  RT-Safety Invariants:
//    - strips_ is a fixed std::array<MaxStrips>, never resized
//    - stripDsp_ is a fixed std::array<MaxStrips>, never resized
//    - numStrips_ atomic controls how many are active
//    - addStrip/removeStrip only change the atomic counter + init defaults
//    - processBlock reads numStrips_ once, never touches storage beyond it
//    - All UI<->audio communication via atomics (StripState)
//    - SmoothedValue ramps prevent zipper noise, reset in prepareToPlay
//    - tempBuffer_ pre-allocated in prepareToPlay, never in processBlock
//  --------------------------------------------------------------------------

#pragma once

#include "PedalboardProcessors.h"
#include "VuMeterDsp.h"

#include <JuceHeader.h>
#include <array>
#include <atomic>

//==============================================================================
class DawMixerProcessor : public PedalboardProcessor
{
  public:
    static constexpr int MaxStrips = 32;
    static constexpr int DefaultStrips = 2;
    static constexpr float MinGainDb = -60.0f;
    static constexpr float MaxGainDb = 12.0f;
    static constexpr float GainRampSeconds = 0.05f;

    //--------------------------------------------------------------------------
    // Per-strip state -- all fields are atomic for lock-free UI<->audio
    struct StripState
    {
        std::atomic<float> gainDb{0.0f}; // -60 to +12 dB  (UI writes, audio reads)
        std::atomic<float> pan{0.0f};    // -1 (L) to +1 (R)
        std::atomic<bool> mute{false};
        std::atomic<bool> solo{false};
        std::atomic<bool> phaseInvert{false};

        // VU metering -- audio writes, UI reads
        std::atomic<float> vuL{0.0f};
        std::atomic<float> vuR{0.0f};
        std::atomic<float> peakL{0.0f};
        std::atomic<float> peakR{0.0f};

        // Strip name (message thread only -- NOT read by audio thread)
        String name;

        void resetDefaults(int index)
        {
            gainDb.store(0.0f, std::memory_order_relaxed);
            pan.store(0.0f, std::memory_order_relaxed);
            mute.store(false, std::memory_order_relaxed);
            solo.store(false, std::memory_order_relaxed);
            phaseInvert.store(false, std::memory_order_relaxed);
            vuL.store(0.0f, std::memory_order_relaxed);
            vuR.store(0.0f, std::memory_order_relaxed);
            peakL.store(0.0f, std::memory_order_relaxed);
            peakR.store(0.0f, std::memory_order_relaxed);
            name = "Ch " + String(index + 1);
        }
    };

    //--------------------------------------------------------------------------
    // Per-strip audio-thread DSP (VU meter + gain smoothing)
    // Fixed array, all MaxStrips always allocated
    struct StripDsp
    {
        VuMeterDsp vuL, vuR;
        SmoothedValue<float, ValueSmoothingTypes::Multiplicative> smoothedGain;

        void init(double sampleRate)
        {
            vuL.init(static_cast<float>(sampleRate));
            vuR.init(static_cast<float>(sampleRate));
            smoothedGain.reset(sampleRate, GainRampSeconds);
            smoothedGain.setCurrentAndTargetValue(1.0f);
        }
    };

    //--------------------------------------------------------------------------
    DawMixerProcessor();
    ~DawMixerProcessor() override;

    // Strip management (message thread only) -- lock-free via atomic counter
    int getNumStrips() const { return numStrips_.load(std::memory_order_acquire); }
    void addStrip();
    void removeStrip();
    StripState* getStrip(int index);
    const StripState* getStrip(int index) const;

    // Master state (atomic for UI<->audio)
    std::atomic<float> masterGainDb{0.0f};
    std::atomic<bool> masterMute{false};
    std::atomic<float> masterVuL{0.0f};
    std::atomic<float> masterVuR{0.0f};
    std::atomic<float> masterPeakL{0.0f};
    std::atomic<float> masterPeakR{0.0f};

    // PedalboardProcessor overrides
    Component* getControls() override;
    Point<int> getSize() override;

    // Pin alignment: input pins match strip rows, output pins at master row
    PinLayout getInputPinLayout() const override
    {
        // Strip rows start at: controls y(24) + header(24) = 48
        // Pin center should be at strip center: 48 + stripHeight/2 = 70
        // Pin top = center - pinHeight/2 = 70 - 8 = 62
        return {62, 44};
    }
    PinLayout getOutputPinLayout() const override
    {
        // Master row is after all strips: 48 + N*44
        // 2 stereo output pins centered in the master row (44px)
        int n = numStrips_.load(std::memory_order_acquire);
        int masterRowTop = 48 + n * 44;
        // First pin center at masterRowTop + 11, second at masterRowTop + 33
        // → startY = masterRowTop + 11 - 8 = masterRowTop + 3
        return {masterRowTop + 3, 22};
    }

    // AudioProcessor overrides
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void processBlock(AudioBuffer<float>& buffer, MidiBuffer& midiMessages) override;

    AudioProcessorEditor* createEditor() override { return nullptr; }
    bool hasEditor() const override { return false; }

    const String getName() const override { return "DAW Mixer"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    const String getInputChannelName(int channelIndex) const override;
    const String getOutputChannelName(int channelIndex) const override;

    int getNumPrograms() override { return 0; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const String getProgramName(int) override { return ""; }
    void changeProgramName(int, const String&) override {}

    void getStateInformation(MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    void fillInPluginDescription(PluginDescription& description) const override;

    // Deprecated but required by PedalboardProcessor
    bool isInputChannelStereoPair(int) const { return false; }
    bool isOutputChannelStereoPair(int) const { return true; }
    bool silenceInProducesSilenceOut() const { return true; }
    int getNumParameters() { return 0; }
    const String getParameterName(int) { return ""; }
    float getParameter(int) { return 0.0f; }
    const String getParameterText(int) { return ""; }
    void setParameter(int, float) {}

  private:
    // Fixed-size strip storage -- never resized, fully RT-safe
    std::array<StripState, MaxStrips> strips_;
    std::atomic<int> numStrips_{0};

    // Fixed-size per-strip DSP state -- never resized
    std::array<StripDsp, MaxStrips> stripDsp_;

    // Master DSP
    SmoothedValue<float, ValueSmoothingTypes::Multiplicative> smoothedMasterGain_;
    VuMeterDsp masterVuDspL_, masterVuDspR_;

    // Pre-allocated mixing buffer (sized in prepareToPlay)
    AudioBuffer<float> tempBuffer_;

    // Metering
    double currentSampleRate_ = 44100.0;
    float peakDecay_ = 0.0f;

    void computeVuDecay(double sampleRate);
    void updateChannelConfig();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DawMixerProcessor)
};
