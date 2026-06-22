/*
  ==============================================================================

    TunerProcessor.h
    Monophonic chromatic tuner with needle, strobe-view, and string-reference displays

  ==============================================================================
*/

#pragma once

#include "PedalboardProcessors.h"
#include "dsp/TunerAnalysis.h"

#include <array>
#include <atomic>
#include <thread>

//==============================================================================
/**
    Chromatic tuner with background monophonic pitch analysis and multiple
    display views. Strobe and string modes are visual interpretations of the
    same detected pitch.
*/
class TunerProcessor : public PedalboardProcessor
{
  public:
    enum class ResponseMode
    {
        Fast = 0,
        Stable = 1
    };

    TunerProcessor();
    ~TunerProcessor();

    //==========================================================================
    // PedalboardProcessor interface
    Component* getControls() override;
    Point<int> getSize() override { return Point<int>(360, 276); }
    NodeShellPolicy getNodeShellPolicy() const override { return NodeShellPolicy::directPainted(); }
    PinLayout getInputPinLayout() const override;
    PinLayout getOutputPinLayout() const override;

    void updateEditorBounds(const Rectangle<int>& bounds);

    //==========================================================================
    // Pitch detection results (thread-safe getters)
    float getDetectedFrequency() const { return detectedFrequency.load(); }
    float getCentsDeviation() const { return centsDeviation.load(); }
    int getDetectedNote() const { return detectedNote.load(); }
    bool isPitchDetected() const { return pitchDetected.load(); }
    float getReferenceA4Hz() const { return referenceA4Hz.load(); }
    void setReferenceA4Hz(float frequencyHz) noexcept;
    ResponseMode getResponseMode() const noexcept;
    void setResponseMode(ResponseMode mode) noexcept;

    /// For strobe view: display phase accumulator (0-1)
    float getStrobePhase() const { return strobePhase.load(); }

    //==========================================================================
    // AudioProcessor overrides
    void fillInPluginDescription(PluginDescription& description) const override;
    void processBlock(AudioSampleBuffer& buffer, MidiBuffer& midiMessages) override;

    const String getName() const override { return "Tuner"; }
    void prepareToPlay(double sampleRate, int estimatedSamplesPerBlock) override;
    void releaseResources() override;

    const String getInputChannelName(int channelIndex) const override { return ""; }
    const String getOutputChannelName(int channelIndex) const override { return ""; }
    bool isInputChannelStereoPair(int index) const override { return false; }
    bool isOutputChannelStereoPair(int index) const override { return false; }
    bool silenceInProducesSilenceOut() const override { return true; }
    double getTailLengthSeconds() const override { return 0.0; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }

    AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    int getNumParameters() override { return 0; }
    const String getParameterName(int parameterIndex) override { return ""; }
    float getParameter(int parameterIndex) override { return 0.0f; }
    const String getParameterText(int parameterIndex) override { return ""; }
    void setParameter(int parameterIndex, float newValue) override {}

    int getNumPrograms() override { return 0; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int index) override {}
    const String getProgramName(int index) override { return ""; }
    void changeProgramName(int index, const String& newName) override {}

    void getStateInformation(MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    /// Sets whether the tuner should mute its output (pass silence)
    void setMuteOutput(bool shouldMute) { muteOutput.store(shouldMute); }

  private:
    std::atomic<bool> muteOutput{false};

    //==========================================================================
    // Background analysis boundary
    void startAnalysisThread();
    void stopAnalysisThread() noexcept;
    void publishAnalysisWindow() noexcept;
    void analysisThreadMain() noexcept;
    void applyAnalysisResult(const pedalboard3::dsp::TunerAnalysisResult& result) noexcept;
    void clearAnalysisResult() noexcept;
    void resetResponseSmoothing() noexcept;

    // Update display strobe phase based on frequency error.
    void updateStrobePhase(float frequency, int midiNote, float refA4Hz) noexcept;

    //==========================================================================
    // Fixed audio-thread storage for publishing complete analysis windows.
    static constexpr int ANALYSIS_HOP = pedalboard3::dsp::TunerAnalysis::kAnalysisHopSize;
    std::array<float, pedalboard3::dsp::TunerAnalysis::kAnalysisWindowSize> analysisRing;
    std::array<std::array<float, pedalboard3::dsp::TunerAnalysis::kAnalysisWindowSize>, 2> analysisWindows;
    int bufferWritePos = 0;
    int samplesAvailable = 0;
    int samplesUntilNextAnalysis = 0;
    int writeAnalysisWindowSlot = 0;
    std::atomic<int> publishedAnalysisWindowSlot{-1};
    std::atomic<unsigned int> publishedAnalysisSequence{0};
    std::atomic<bool> analysisWindowPending{false};
    std::atomic<bool> stopAnalysisThreadFlag{false};
    std::thread analysisThread;
    pedalboard3::dsp::TunerAnalysis backgroundAnalyzer;

    // Detection results (atomic for thread safety)
    std::atomic<float> detectedFrequency{0.0f};
    std::atomic<float> centsDeviation{0.0f};
    std::atomic<int> detectedNote{-1};
    std::atomic<bool> pitchDetected{false};
    std::atomic<float> strobePhase{0.0f};
    std::atomic<float> referenceA4Hz{440.0f};
    std::atomic<int> responseMode{static_cast<int>(ResponseMode::Stable)};

    // Processing state
    double sampleRate = 44100.0;

    // Tuning reference (A4 = 440 Hz)
    static constexpr int A4_MIDI = 69;

    // Background-thread-only smoothing state.
    int candidateNote = -1;
    int candidateHitCount = 0;
    int heldMissCount = 0;

    // Editor bounds
    Rectangle<int> editorBounds;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TunerProcessor)
};
