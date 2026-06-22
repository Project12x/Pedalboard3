// TunerAnalysis.h - Portable monophonic tuner analysis core.
//
// Pattern references recorded in docs/superpowers/plans/2026-06-22-tuner-meter-upgrade.md.
// This file intentionally contains no JUCE dependency.

#pragma once

#include <array>

namespace pedalboard3::dsp
{

enum class TunerSignalState
{
    NoSignal,
    Unstable,
    Stable
};

struct TunerAnalysisResult
{
    bool detected = false;
    TunerSignalState state = TunerSignalState::NoSignal;
    float frequencyHz = 0.0f;
    int midiNote = -1;
    float cents = 0.0f;
    float confidence = 0.0f;
    float referenceA4Hz = 440.0f;
};

class TunerAnalysis
{
  public:
    static constexpr int kAnalysisWindowSize = 4096;
    static constexpr int kAnalysisHopSize = 512;

    void prepare(double sampleRate, int maxBlockSize) noexcept;
    void reset() noexcept;

    void setReferenceA4Hz(float frequencyHz) noexcept;
    float getReferenceA4Hz() const noexcept { return referenceA4Hz_; }

    void pushSamples(const float* samples, int numSamples) noexcept;

    TunerAnalysisResult analyze() noexcept;
    TunerAnalysisResult getLastResult() const noexcept { return lastResult_; }

  private:
    struct PitchEstimate
    {
        float frequencyHz = 0.0f;
        float confidence = 0.0f;
    };

    TunerAnalysisResult makeNoSignalResult() const noexcept;
    TunerAnalysisResult makeUnstableResult(PitchEstimate estimate) const noexcept;
    TunerAnalysisResult makeStableResult(PitchEstimate estimate) const noexcept;
    PitchEstimate detectPitchYin() noexcept;
    float calculateRms() const noexcept;
    void copyAnalysisWindow() noexcept;

    static float sanitize(float sample) noexcept;

    static constexpr double kDefaultSampleRate = 44100.0;
    static constexpr float kDefaultReferenceA4Hz = 440.0f;
    static constexpr float kMinReferenceA4Hz = 400.0f;
    static constexpr float kMaxReferenceA4Hz = 480.0f;
    static constexpr float kMinRms = 0.0025f;
    static constexpr float kStableConfidence = 0.75f;
    static constexpr float kUnstableConfidence = 0.45f;
    static constexpr float kYinThreshold = 0.15f;
    static constexpr float kMinFrequencyHz = 20.0f;
    static constexpr float kMaxFrequencyHz = 5000.0f;
    static constexpr int kA4MidiNote = 69;

    double sampleRate_ = kDefaultSampleRate;
    float referenceA4Hz_ = kDefaultReferenceA4Hz;
    int writePos_ = 0;
    int samplesAvailable_ = 0;

    std::array<float, kAnalysisWindowSize> ringBuffer_{};
    std::array<float, kAnalysisWindowSize> analysisWindow_{};
    std::array<float, kAnalysisWindowSize / 2> yinBuffer_{};
    TunerAnalysisResult lastResult_{};
};

} // namespace pedalboard3::dsp

