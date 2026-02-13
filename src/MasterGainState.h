/*
  ==============================================================================

    MasterGainState.h
    Global input/output gain state shared across all UI components.
    Thread-safe via std::atomic - read by audio thread, written by UI.

    Master gain (footer/StageView): applied uniformly to all channels.
    Per-channel gain (node sliders): applied per-channel on Audio I/O nodes.
    Final gain = master * per-channel.

  ==============================================================================
*/

#ifndef MASTERGAINSTATE_H_INCLUDED
#define MASTERGAINSTATE_H_INCLUDED

#include <JuceHeader.h>
#include <atomic>

class MasterGainState
{
  public:
    static constexpr int MaxChannels = 16;

    static MasterGainState& getInstance()
    {
        static MasterGainState instance;
        return instance;
    }

    // --- Master gain (footer sliders) ---
    // Gain in dB. Range: -60 to +12, default 0.
    std::atomic<float> masterInputGainDb{0.0f};
    std::atomic<float> masterOutputGainDb{0.0f};

    float getMasterInputGainLinear() const
    {
        return Decibels::decibelsToGain(masterInputGainDb.load(std::memory_order_relaxed), -60.0f);
    }

    float getMasterOutputGainLinear() const
    {
        return Decibels::decibelsToGain(masterOutputGainDb.load(std::memory_order_relaxed), -60.0f);
    }

    // --- Per-channel gain (node sliders) ---
    // Gain in dB. Range: -60 to +12, default 0.
    std::atomic<float> inputChannelGainDb[MaxChannels]{};
    std::atomic<float> outputChannelGainDb[MaxChannels]{};

    float getInputChannelGainLinear(int ch) const
    {
        if (ch >= 0 && ch < MaxChannels)
            return Decibels::decibelsToGain(inputChannelGainDb[ch].load(std::memory_order_relaxed), -60.0f);
        return 1.0f;
    }

    float getOutputChannelGainLinear(int ch) const
    {
        if (ch >= 0 && ch < MaxChannels)
            return Decibels::decibelsToGain(outputChannelGainDb[ch].load(std::memory_order_relaxed), -60.0f);
        return 1.0f;
    }

    // --- Combined gain (master * per-channel) for audio thread ---
    float getInputGainLinear(int ch) const
    {
        return getMasterInputGainLinear() * getInputChannelGainLinear(ch);
    }

    float getOutputGainLinear(int ch) const
    {
        return getMasterOutputGainLinear() * getOutputChannelGainLinear(ch);
    }

    void loadFromSettings();
    void saveToSettings();

  private:
    MasterGainState() = default;
    MasterGainState(const MasterGainState&) = delete;
    MasterGainState& operator=(const MasterGainState&) = delete;
};

#endif // MASTERGAINSTATE_H_INCLUDED
