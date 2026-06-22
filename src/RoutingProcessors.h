/*
  ==============================================================================

    RoutingProcessors.h
    Created: 27 Jan 2026

    Processors for A/B routing (Splitter and Mixer).

  ==============================================================================
*/

#pragma once

#include "PedalboardProcessors.h"

#include <JuceHeader.h>
#include <array>
#include <atomic>

//==============================================================================
/**
    Splits a stereo input into two stereo pairs (A and B).
    Input:  2 channels (Stereo)
    Output: 4 channels (Stereo A + Stereo B)
*/
class SplitterProcessor : public PedalboardProcessor
{
  public:
    static constexpr int MaxStrips = 32;
    static constexpr int DefaultStrips = 2;
    static constexpr float MinGainDb = -60.0f;
    static constexpr float MaxGainDb = 12.0f;
    static constexpr float GainRampSeconds = 0.05f;

    struct StripState
    {
        std::atomic<float> gainDb{0.0f};
        std::atomic<float> pan{0.0f};
        std::atomic<bool> mute{false};
        std::atomic<bool> solo{false};
        std::atomic<bool> stereo{true};
        std::atomic<bool> phaseInvert{false};
        std::atomic<float> vuL{0.0f};
        std::atomic<float> vuR{0.0f};
        std::atomic<float> peakL{0.0f};
        std::atomic<float> peakR{0.0f};
        String name;

        void resetDefaults(int index)
        {
            gainDb.store(0.0f, std::memory_order_relaxed);
            pan.store(0.0f, std::memory_order_relaxed);
            mute.store(false, std::memory_order_relaxed);
            solo.store(false, std::memory_order_relaxed);
            stereo.store(true, std::memory_order_relaxed);
            phaseInvert.store(false, std::memory_order_relaxed);
            vuL.store(0.0f, std::memory_order_relaxed);
            vuR.store(0.0f, std::memory_order_relaxed);
            peakL.store(0.0f, std::memory_order_relaxed);
            peakR.store(0.0f, std::memory_order_relaxed);
            name = "Out " + String(index + 1);
        }
    };

    struct StripDsp
    {
        SmoothedValue<float, ValueSmoothingTypes::Multiplicative> smoothedGain;

        void init(double sampleRate)
        {
            smoothedGain.reset(sampleRate, GainRampSeconds);
            smoothedGain.setCurrentAndTargetValue(1.0f);
        }
    };

    SplitterProcessor();
    ~SplitterProcessor() override;

    int getNumStrips() const { return numStrips_.load(std::memory_order_acquire); }
    void addStrip();
    void removeStrip();
    StripState* getStrip(int index);
    const StripState* getStrip(int index) const;

    std::atomic<float> inputVuL{0.0f};
    std::atomic<float> inputVuR{0.0f};
    std::atomic<float> inputPeakL{0.0f};
    std::atomic<float> inputPeakR{0.0f};

    // PedalboardProcessor overrides
    Component* getControls() override;
    Point<int> getSize() override;
    NodeShellPolicy getNodeShellPolicy() const override { return NodeShellPolicy::compactPinLabels(); }
    PinLayout getInputPinLayout() const override;
    PinLayout getOutputPinLayout() const override;

    // AudioProcessor overrides
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void processBlock(AudioBuffer<float>& buffer, MidiBuffer& midiMessages) override;

    AudioProcessorEditor* createEditor() override { return nullptr; }
    bool hasEditor() const override { return false; }

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
    void setOutputGainDb(int outputIndex, float db);
    float getOutputGainDb(int outputIndex) const;
    void setOutputPan(int outputIndex, float pan);
    float getOutputPan(int outputIndex) const;
    void setOutputSolo(int outputIndex, bool shouldSolo);
    bool getOutputSolo(int outputIndex) const;
    void setOutputStereo(int outputIndex, bool shouldBeStereo);
    bool getOutputStereo(int outputIndex) const;
    void setOutputPhaseInvert(int outputIndex, bool shouldInvert);
    bool getOutputPhaseInvert(int outputIndex) const;

    bool isInputChannelStereoPair(int) const { return true; }
    bool isOutputChannelStereoPair(int channelIndex) const;
    bool silenceInProducesSilenceOut() const { return true; }
    int getNumParameters() override { return 0; }
    float getParameter(int) override { return 0.0f; }
    void setParameter(int, float) override {}
    const String getParameterName(int) override { return ""; }
    const String getParameterText(int) override { return ""; }
    int countTotalOutputChannels() const;
    void updateChannelConfig();

  private:
    std::array<StripState, MaxStrips> strips_;
    std::atomic<int> numStrips_{0};
    std::array<StripDsp, MaxStrips> stripDsp_;
    AudioBuffer<float> inputSnapshot_;
    double currentSampleRate_ = 44100.0;
    float peakDecay_ = 0.0f;

    Rectangle<int> editorBounds;

    void computeVuDecay(double sampleRate);

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
    static constexpr int MaxStrips = 32;
    static constexpr int DefaultStrips = 2;
    static constexpr float MinGainDb = -60.0f;
    static constexpr float MaxGainDb = 12.0f;
    static constexpr float GainRampSeconds = 0.05f;

    struct StripState
    {
        std::atomic<float> gainDb{0.0f};
        std::atomic<float> pan{0.0f};
        std::atomic<bool> mute{false};
        std::atomic<bool> solo{false};
        std::atomic<bool> stereo{true};
        std::atomic<bool> phaseInvert{false};
        std::atomic<float> vuL{0.0f};
        std::atomic<float> vuR{0.0f};
        std::atomic<float> peakL{0.0f};
        std::atomic<float> peakR{0.0f};
        String name;

        void resetDefaults(int index)
        {
            gainDb.store(0.0f, std::memory_order_relaxed);
            pan.store(0.0f, std::memory_order_relaxed);
            mute.store(false, std::memory_order_relaxed);
            solo.store(false, std::memory_order_relaxed);
            stereo.store(true, std::memory_order_relaxed);
            phaseInvert.store(false, std::memory_order_relaxed);
            vuL.store(0.0f, std::memory_order_relaxed);
            vuR.store(0.0f, std::memory_order_relaxed);
            peakL.store(0.0f, std::memory_order_relaxed);
            peakR.store(0.0f, std::memory_order_relaxed);
            name = "Ch " + String(index + 1);
        }
    };

    struct StripDsp
    {
        SmoothedValue<float, ValueSmoothingTypes::Multiplicative> smoothedGain;

        void init(double sampleRate)
        {
            smoothedGain.reset(sampleRate, GainRampSeconds);
            smoothedGain.setCurrentAndTargetValue(1.0f);
        }
    };

    MixerProcessor();
    ~MixerProcessor() override;

    int getNumStrips() const { return numStrips_.load(std::memory_order_acquire); }
    bool isVerticalLayout() const { return verticalLayout_.load(std::memory_order_acquire); }
    void setVerticalLayout(bool vertical) { verticalLayout_.store(vertical, std::memory_order_release); }
    void addStrip();
    void removeStrip();
    StripState* getStrip(int index);
    const StripState* getStrip(int index) const;

    std::atomic<float> masterGainDb{0.0f};
    std::atomic<bool> masterMute{false};
    std::atomic<float> masterVuL{0.0f};
    std::atomic<float> masterVuR{0.0f};
    std::atomic<float> masterPeakL{0.0f};
    std::atomic<float> masterPeakR{0.0f};

    // PedalboardProcessor overrides
    Component* getControls() override;
    Point<int> getSize() override;
    NodeShellPolicy getNodeShellPolicy() const override { return NodeShellPolicy::compactPinLabels(); }
    PinLayout getInputPinLayout() const override;
    PinLayout getOutputPinLayout() const override;

    // AudioProcessor overrides
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void processBlock(AudioBuffer<float>& buffer, MidiBuffer& midiMessages) override;

    AudioProcessorEditor* createEditor() override { return nullptr; }
    bool hasEditor() const override { return false; }

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

    // Convenience accessors
    float getChannelGainDb(int ch) const;
    void setChannelGainDb(int ch, float db);
    float getChannelPan(int ch) const;
    void setChannelPan(int ch, float p);
    bool getChannelMute(int ch) const;
    void setChannelMute(int ch, bool m);
    bool getChannelSolo(int ch) const;
    void setChannelSolo(int ch, bool s);
    bool getChannelStereo(int ch) const;
    void setChannelStereo(int ch, bool s);
    bool getChannelPhaseInvert(int ch) const;
    void setChannelPhaseInvert(int ch, bool p);
    float getMasterGainDb() const { return masterGainDb.load(std::memory_order_relaxed); }
    void setMasterGainDb(float db) { masterGainDb.store(db, std::memory_order_relaxed); }
    bool getMasterMute() const { return masterMute.load(std::memory_order_relaxed); }
    void setMasterMute(bool m) { masterMute.store(m, std::memory_order_relaxed); }

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

    bool isInputChannelStereoPair(int channelIndex) const;
    bool isOutputChannelStereoPair(int) const { return true; }
    bool silenceInProducesSilenceOut() const { return true; }
    int countTotalInputChannels() const;
    void updateChannelConfig();

  private:
    std::array<StripState, MaxStrips> strips_;
    std::atomic<int> numStrips_{0};
    std::atomic<bool> verticalLayout_{false};
    std::array<StripDsp, MaxStrips> stripDsp_;
    SmoothedValue<float, ValueSmoothingTypes::Multiplicative> smoothedMasterGain_;
    AudioBuffer<float> tempBuffer_;
    double currentSampleRate_ = 44100.0;
    float peakDecay_ = 0.0f;

    Rectangle<int> editorBounds;

    void computeVuDecay(double sampleRate);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MixerProcessor)
};
