/*
  ==============================================================================

    RoutingProcessors.h
    Created: 27 Jan 2026

    Processors for A/B routing (Splitter and Mixer).

  ==============================================================================
*/

#pragma once

#include "PedalboardProcessors.h"
#include "VuMeterDsp.h"

#include <JuceHeader.h>

//==============================================================================
/**
    Splits a stereo input into two stereo pairs (A and B).
    Input:  2 channels (Stereo)
    Output: 4 channels (Stereo A + Stereo B)
*/
class SplitterProcessor : public PedalboardProcessor
{
  public:
    SplitterProcessor();
    ~SplitterProcessor() override;

    // PedalboardProcessor overrides
    Component* getControls() override;
    Point<int> getSize() override { return Point<int>(100, 60); }

    // AudioProcessor overrides
    void prepareToPlay(double sampleRate, int samplesPerBlock) override {}
    void releaseResources() override {}
    void processBlock(AudioBuffer<float>& buffer, MidiBuffer& midiMessages) override;

    AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const String getName() const override { return "Splitter"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    const String getInputChannelName(int channelIndex) const override;
    const String getOutputChannelName(int channelIndex) const override;

    int getNumPrograms() override { return 0; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int index) override {}
    const String getProgramName(int index) override { return ""; }
    void changeProgramName(int index, const String& newName) override {}

    void getStateInformation(MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    void fillInPluginDescription(PluginDescription& description) const override;

    // Parameters
    void setOutputMute(int outputIndex, bool shouldMute);
    bool getOutputMute(int outputIndex) const;

  private:
    std::atomic<bool> muteA{false};
    std::atomic<bool> muteB{false};

    Rectangle<int> editorBounds;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SplitterProcessor)
};

//==============================================================================
/**
    Mixes two stereo pairs (A and B) into one stereo output.
    Input:  4 channels (Stereo A + Stereo B)
    Output: 2 channels (Stereo Mix)

    Features:
    - Per-channel gain (dB), pan (equal-power -3dB law), mute, solo, phase invert
    - SmoothedValue gain ramps (50ms, zipper-free)
    - VU metering per channel (IEC 60268-17, 300ms integration)
*/
class MixerProcessor : public PedalboardProcessor
{
  public:
    MixerProcessor();
    ~MixerProcessor() override;

    // PedalboardProcessor overrides
    Component* getControls() override;
    Point<int> getSize() override { return Point<int>(100, 80); }

    // AudioProcessor overrides
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void processBlock(AudioBuffer<float>& buffer, MidiBuffer& midiMessages) override;

    AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const String getName() const override { return "Mixer"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    const String getInputChannelName(int channelIndex) const override;
    const String getOutputChannelName(int channelIndex) const override;

    int getNumPrograms() override { return 0; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int index) override {}
    const String getProgramName(int index) override { return ""; }
    void changeProgramName(int index, const String& newName) override {}

    void getStateInformation(MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    void fillInPluginDescription(PluginDescription& description) const override;

    //==========================================================================
    // Per-channel state (A = 0, B = 1)
    static constexpr int NumChannels = 2;

    struct ChannelState
    {
        std::atomic<float> gainDb{0.0f}; // -60 to +12 dB
        std::atomic<float> pan{0.0f};    // -1.0 (L) to +1.0 (R)
        std::atomic<bool> mute{false};
        std::atomic<bool> solo{false};
        std::atomic<bool> phaseInvert{false};
        // VU metering (written on audio thread, read by UI)
        VuMeterDsp vuL, vuR;
        std::atomic<float> vuLevelL{0.0f}, vuLevelR{0.0f};
        std::atomic<float> peakL{0.0f}, peakR{0.0f};
    };

    ChannelState channels[NumChannels];

    // Convenience accessors
    float getChannelGainDb(int ch) const { return channels[ch].gainDb.load(std::memory_order_relaxed); }
    void setChannelGainDb(int ch, float db) { channels[ch].gainDb.store(db, std::memory_order_relaxed); }
    float getChannelPan(int ch) const { return channels[ch].pan.load(std::memory_order_relaxed); }
    void setChannelPan(int ch, float p) { channels[ch].pan.store(p, std::memory_order_relaxed); }
    bool getChannelMute(int ch) const { return channels[ch].mute.load(std::memory_order_relaxed); }
    void setChannelMute(int ch, bool m) { channels[ch].mute.store(m, std::memory_order_relaxed); }
    bool getChannelSolo(int ch) const { return channels[ch].solo.load(std::memory_order_relaxed); }
    void setChannelSolo(int ch, bool s) { channels[ch].solo.store(s, std::memory_order_relaxed); }
    bool getChannelPhaseInvert(int ch) const { return channels[ch].phaseInvert.load(std::memory_order_relaxed); }
    void setChannelPhaseInvert(int ch, bool p) { channels[ch].phaseInvert.store(p, std::memory_order_relaxed); }

    // Legacy parameter interface (for MIDI mapping compatibility)
    enum Parameters
    {
        ParamGainA = 0,
        ParamGainB,
        ParamPanA,
        ParamPanB,
        NumParameters
    };
    int getNumParameters() override { return NumParameters; }
    float getParameter(int parameterIndex) override;
    void setParameter(int parameterIndex, float newValue) override;
    const String getParameterName(int parameterIndex) override;
    const String getParameterText(int parameterIndex) override;

  private:
    // Gain smoothing (50ms multiplicative ramp)
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative> smoothedGain[NumChannels];
    float peakDecayCoeff = 0.9995f;

    Rectangle<int> editorBounds;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MixerProcessor)
};
