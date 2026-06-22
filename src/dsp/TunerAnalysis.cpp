#include "TunerAnalysis.h"

#include <algorithm>
#include <cmath>

namespace pedalboard3::dsp
{

void TunerAnalysis::prepare(double sampleRate, int maxBlockSize) noexcept
{
    (void)maxBlockSize;

    if (std::isfinite(sampleRate) && sampleRate >= 8000.0 && sampleRate <= 384000.0)
        sampleRate_ = sampleRate;
    else
        sampleRate_ = kDefaultSampleRate;

    reset();
}

void TunerAnalysis::reset() noexcept
{
    ringBuffer_.fill(0.0f);
    analysisWindow_.fill(0.0f);
    yinBuffer_.fill(1.0f);
    writePos_ = 0;
    samplesAvailable_ = 0;
    lastResult_ = makeNoSignalResult();
}

void TunerAnalysis::setReferenceA4Hz(float frequencyHz) noexcept
{
    if (!std::isfinite(frequencyHz))
        frequencyHz = kDefaultReferenceA4Hz;

    referenceA4Hz_ = std::clamp(frequencyHz, kMinReferenceA4Hz, kMaxReferenceA4Hz);
    lastResult_.referenceA4Hz = referenceA4Hz_;
}

void TunerAnalysis::pushSamples(const float* samples, int numSamples) noexcept
{
    if (samples == nullptr || numSamples <= 0)
        return;

    for (int i = 0; i < numSamples; ++i)
    {
        ringBuffer_[static_cast<size_t>(writePos_)] = sanitize(samples[i]);
        writePos_ = (writePos_ + 1) % kAnalysisWindowSize;
        samplesAvailable_ = std::min(samplesAvailable_ + 1, kAnalysisWindowSize);
    }
}

TunerAnalysisResult TunerAnalysis::analyze() noexcept
{
    if (samplesAvailable_ < kAnalysisWindowSize)
    {
        lastResult_ = makeNoSignalResult();
        return lastResult_;
    }

    copyAnalysisWindow();

    if (calculateRms() < kMinRms)
    {
        lastResult_ = makeNoSignalResult();
        return lastResult_;
    }

    const auto estimate = detectPitchYin();
    if (estimate.frequencyHz < kMinFrequencyHz || estimate.frequencyHz > kMaxFrequencyHz)
    {
        lastResult_ = makeNoSignalResult();
        return lastResult_;
    }

    if (estimate.confidence >= kStableConfidence)
        lastResult_ = makeStableResult(estimate);
    else if (estimate.confidence >= kUnstableConfidence)
        lastResult_ = makeUnstableResult(estimate);
    else
        lastResult_ = makeNoSignalResult();

    return lastResult_;
}

TunerAnalysisResult TunerAnalysis::makeNoSignalResult() const noexcept
{
    TunerAnalysisResult result;
    result.detected = false;
    result.state = TunerSignalState::NoSignal;
    result.referenceA4Hz = referenceA4Hz_;
    return result;
}

TunerAnalysisResult TunerAnalysis::makeUnstableResult(PitchEstimate estimate) const noexcept
{
    TunerAnalysisResult result;
    result.detected = false;
    result.state = TunerSignalState::Unstable;
    result.frequencyHz = estimate.frequencyHz;
    result.confidence = estimate.confidence;
    result.referenceA4Hz = referenceA4Hz_;
    return result;
}

TunerAnalysisResult TunerAnalysis::makeStableResult(PitchEstimate estimate) const noexcept
{
    TunerAnalysisResult result;
    result.detected = true;
    result.state = TunerSignalState::Stable;
    result.frequencyHz = estimate.frequencyHz;
    result.confidence = estimate.confidence;
    result.referenceA4Hz = referenceA4Hz_;

    const auto midiNoteFloat = 12.0f * std::log2(estimate.frequencyHz / referenceA4Hz_) +
                               static_cast<float>(kA4MidiNote);
    result.midiNote = static_cast<int>(std::round(midiNoteFloat));
    result.cents = (midiNoteFloat - static_cast<float>(result.midiNote)) * 100.0f;
    return result;
}

TunerAnalysis::PitchEstimate TunerAnalysis::detectPitchYin() noexcept
{
    constexpr int halfSize = kAnalysisWindowSize / 2;
    yinBuffer_[0] = 1.0f;

    float runningSum = 0.0f;
    for (int tau = 1; tau < halfSize; ++tau)
    {
        float sum = 0.0f;
        for (int j = 0; j < halfSize; ++j)
        {
            const float delta = analysisWindow_[static_cast<size_t>(j)] -
                                analysisWindow_[static_cast<size_t>(j + tau)];
            sum += delta * delta;
        }

        runningSum += sum;
        yinBuffer_[static_cast<size_t>(tau)] = runningSum > 0.0f ? (sum * static_cast<float>(tau) / runningSum) : 1.0f;
    }

    const int minTau = std::max(2, static_cast<int>(std::floor(sampleRate_ / kMaxFrequencyHz)));
    const int maxTau = std::min(halfSize - 2, static_cast<int>(std::ceil(sampleRate_ / kMinFrequencyHz)));

    int tauEstimate = -1;
    float bestValue = 1.0f;
    int bestTau = -1;

    for (int tau = minTau; tau <= maxTau; ++tau)
    {
        const float value = yinBuffer_[static_cast<size_t>(tau)];
        if (value < bestValue)
        {
            bestValue = value;
            bestTau = tau;
        }

        if (value < kYinThreshold)
        {
            while (tau + 1 <= maxTau &&
                   yinBuffer_[static_cast<size_t>(tau + 1)] < yinBuffer_[static_cast<size_t>(tau)])
            {
                ++tau;
            }
            tauEstimate = tau;
            break;
        }
    }

    if (tauEstimate < 0)
        tauEstimate = bestTau;
    if (tauEstimate < 0)
        return {};

    float betterTau = static_cast<float>(tauEstimate);
    if (tauEstimate > 0 && tauEstimate < halfSize - 1)
    {
        const float s0 = yinBuffer_[static_cast<size_t>(tauEstimate - 1)];
        const float s1 = yinBuffer_[static_cast<size_t>(tauEstimate)];
        const float s2 = yinBuffer_[static_cast<size_t>(tauEstimate + 1)];
        const float denominator = 2.0f * (2.0f * s1 - s2 - s0);
        if (std::abs(denominator) > 1.0e-12f)
        {
            const float correction = (s2 - s0) / denominator;
            if (std::isfinite(correction) && std::abs(correction) <= 1.0f)
                betterTau += correction;
        }
    }

    if (betterTau <= 0.0f)
        return {};

    PitchEstimate estimate;
    estimate.frequencyHz = static_cast<float>(sampleRate_ / static_cast<double>(betterTau));
    estimate.confidence = std::clamp(1.0f - yinBuffer_[static_cast<size_t>(tauEstimate)], 0.0f, 1.0f);
    return estimate;
}

float TunerAnalysis::calculateRms() const noexcept
{
    double sumSquares = 0.0;
    for (const auto sample : analysisWindow_)
        sumSquares += static_cast<double>(sample) * static_cast<double>(sample);

    return static_cast<float>(std::sqrt(sumSquares / static_cast<double>(kAnalysisWindowSize)));
}

void TunerAnalysis::copyAnalysisWindow() noexcept
{
    for (int i = 0; i < kAnalysisWindowSize; ++i)
    {
        const int sourceIndex = (writePos_ + i) % kAnalysisWindowSize;
        analysisWindow_[static_cast<size_t>(i)] = ringBuffer_[static_cast<size_t>(sourceIndex)];
    }
}

float TunerAnalysis::sanitize(float sample) noexcept
{
    if (!std::isfinite(sample))
        return 0.0f;
    return std::clamp(sample, -8.0f, 8.0f);
}

} // namespace pedalboard3::dsp

