/*
  ==============================================================================

    CrossfadeMixer.h
    Pedalboard3 - Glitch-Free Patch Switching

    Audio processor that provides smooth crossfading during patch changes.
    Inserted at the end of the audio chain before output.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <atomic>

//==============================================================================
/**
    CrossfadeMixerProcessor provides smooth audio crossfading during patch switching.

    When a patch change is triggered:
    1. startFadeOut() is called - audio fades to silence over fadeMs
    2. Patch is loaded while audio is silent
    3. startFadeIn() is called - audio fades back in over fadeMs

    All operations are audio-thread safe using atomics.
*/
class CrossfadeMixerProcessor : public AudioProcessor
{
  public:
    CrossfadeMixerProcessor();
    ~CrossfadeMixerProcessor() override = default;

    //==============================================================================
    // Crossfade control (call from message thread)

    /// Start fading audio out. Call before clearing the graph.
    void startFadeOut(int durationMs = 100);

    /// Start fading audio in. Call after loading the new patch.
    void startFadeIn(int durationMs = 100);

    /// Returns true if currently fading (out or in)
    bool isFading() const { return fading.load(); }

    /// Returns true if currently faded out (silent)
    bool isSilent() const { return fadeGain.load() < 0.001f; }

    /// Set the default fade duration (stored in settings)
    void setDefaultFadeDuration(int ms) { defaultFadeMs = ms; }
    int getDefaultFadeDuration() const { return defaultFadeMs; }

    //==============================================================================
    // AudioProcessor implementation

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void processBlock(AudioBuffer<float>& buffer, MidiBuffer& midi) override;

    //==============================================================================
    // AudioProcessor boilerplate

    const String getName() const override { return "Crossfade Mixer"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return true; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const String getProgramName(int) override { return {}; }
    void changeProgramName(int, const String&) override {}

    void getStateInformation(MemoryBlock&) override {}
    void setStateInformation(const void*, int) override {}

    AudioProcessorEditor* createEditor() override { return nullptr; }
    bool hasEditor() const override { return false; }

  private:
    //==============================================================================
    // Fade state (all atomic for audio thread safety)
    std::atomic<bool> fading{false};
    std::atomic<bool> fadingOut{false};     // true = fading out, false = fading in
    std::atomic<float> fadeGain{1.0f};      // Current gain (0.0 to 1.0)
    std::atomic<float> fadeIncrement{0.0f}; // Per-sample gain change

    double currentSampleRate = 44100.0;
    int defaultFadeMs = 100;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CrossfadeMixerProcessor)
};
