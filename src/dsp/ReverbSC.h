// ReverbSC.h - Portable Costello/Varga ReverbSC DSP core.
//
// Close-port reference:
// - Csound Opcodes/reverbsc.c, commit 2932c7fd14681493b5db83df3efdda175c1eb116
// - Soundpipe modules/revsc.c, commit 3efb43bdabd0ed23b17c694292b5a79f1692a3ea
//
// Original ReverbSC authors: Sean Costello and Istvan Varga.
// Soundpipe adaptation: Paul Batchelor.

#pragma once

#include <array>
#include <vector>

namespace pedalboard3::dsp
{

class ReverbSC
{
  public:
    void prepare(double sampleRate, int maxBlockSize);
    void reset() noexcept;

    void setFeedback(float value) noexcept;
    void setDampingHz(float value) noexcept;

    float getFeedback() const noexcept { return feedback_; }
    float getDampingHz() const noexcept { return dampingHz_; }

    void process(const float* inL, const float* inR, float* outL, float* outR, int numSamples) noexcept;

  private:
    struct DelayLine
    {
        int writePos = 0;
        int bufferSize = 0;
        int readPos = 0;
        int readPosFrac = 0;
        int readPosFracInc = 0;
        int seedVal = 0;
        int randLineCount = 0;
        float filterState = 0.0f;
        std::vector<float> buffer;
    };

    int delayLineMaxSamples(int lineIndex) const noexcept;
    void initDelayLine(DelayLine& line, int lineIndex) noexcept;
    void nextRandomLineSegment(DelayLine& line, int lineIndex) noexcept;
    void updateDampingCoefficient() noexcept;
    static float sanitize(float sample) noexcept;

    static constexpr double kDefaultSampleRate = 44100.0;
    static constexpr double kMinSampleRate = 5000.0;
    static constexpr double kMaxSampleRate = 1000000.0;
    static constexpr float kOutputGain = 0.35f;
    static constexpr float kJunctionPressureScale = 0.25f;
    static constexpr int kDelayPosShift = 28;
    static constexpr int kDelayPosScale = 0x10000000;
    static constexpr int kDelayPosMask = 0x0FFFFFFF;
    static constexpr int kPitchMod = 1;

    double sampleRate_ = kDefaultSampleRate;
    float feedback_ = 0.97f;
    float dampingHz_ = 10000.0f;
    float dampingCoefficient_ = 1.0f;
    float lastCoefficientDampingHz_ = -1.0f;
    bool prepared_ = false;

    std::array<DelayLine, 8> delayLines_;
};

} // namespace pedalboard3::dsp
