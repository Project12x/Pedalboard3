#include "ReverbSC.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>

namespace pedalboard3::dsp
{
namespace
{
constexpr double kPi = 3.14159265358979323846;
constexpr double kReferenceSampleRate = 44100.0;

// delay seconds, modulation depth seconds, modulation frequency Hz, seed
constexpr std::array<std::array<double, 4>, 8> kReverbParams = {{
    {{2473.0 / kReferenceSampleRate, 0.0010, 3.100, 1966.0}},
    {{2767.0 / kReferenceSampleRate, 0.0011, 3.500, 29491.0}},
    {{3217.0 / kReferenceSampleRate, 0.0017, 1.110, 22937.0}},
    {{3557.0 / kReferenceSampleRate, 0.0006, 3.973, 9830.0}},
    {{3907.0 / kReferenceSampleRate, 0.0010, 2.341, 20643.0}},
    {{4127.0 / kReferenceSampleRate, 0.0011, 1.897, 22937.0}},
    {{2143.0 / kReferenceSampleRate, 0.0017, 0.891, 29491.0}},
    {{1933.0 / kReferenceSampleRate, 0.0006, 3.221, 14417.0}},
}};
} // namespace

void ReverbSC::prepare(double sampleRate, int maxBlockSize)
{
    (void)maxBlockSize;

    if (!std::isfinite(sampleRate))
        sampleRate = kDefaultSampleRate;

    sampleRate_ = std::clamp(sampleRate, kMinSampleRate, kMaxSampleRate);

    for (int lineIndex = 0; lineIndex < static_cast<int>(delayLines_.size()); ++lineIndex)
    {
        auto& line = delayLines_[static_cast<size_t>(lineIndex)];
        const int requiredSize = delayLineMaxSamples(lineIndex);
        line.buffer.assign(static_cast<size_t>(requiredSize), 0.0f);
        initDelayLine(line, lineIndex);
    }

    lastCoefficientDampingHz_ = -1.0f;
    updateDampingCoefficient();
    prepared_ = true;
}

void ReverbSC::reset() noexcept
{
    for (int lineIndex = 0; lineIndex < static_cast<int>(delayLines_.size()); ++lineIndex)
    {
        auto& line = delayLines_[static_cast<size_t>(lineIndex)];
        std::fill(line.buffer.begin(), line.buffer.end(), 0.0f);
        initDelayLine(line, lineIndex);
    }
}

void ReverbSC::setFeedback(float value) noexcept
{
    if (!std::isfinite(value))
        value = 0.0f;

    feedback_ = std::clamp(value, 0.0f, 0.99f);
}

void ReverbSC::setDampingHz(float value) noexcept
{
    if (!std::isfinite(value))
        value = 10000.0f;

    dampingHz_ = std::clamp(value, 20.0f, 20000.0f);
}

void ReverbSC::process(const float* inL, const float* inR, float* outL, float* outR, int numSamples) noexcept
{
    if (!prepared_ || inL == nullptr || inR == nullptr || outL == nullptr || outR == nullptr || numSamples <= 0)
        return;

    updateDampingCoefficient();

    for (int sampleIndex = 0; sampleIndex < numSamples; ++sampleIndex)
    {
        float inputL = sanitize(inL[sampleIndex]);
        float inputR = sanitize(inR[sampleIndex]);
        float outputL = 0.0f;
        float outputR = 0.0f;

        float junctionPressure = 0.0f;
        for (const auto& line : delayLines_)
            junctionPressure += line.filterState;

        junctionPressure *= kJunctionPressureScale;

        const float lineInputL = junctionPressure + inputL;
        const float lineInputR = junctionPressure + inputR;

        for (int lineIndex = 0; lineIndex < static_cast<int>(delayLines_.size()); ++lineIndex)
        {
            auto& line = delayLines_[static_cast<size_t>(lineIndex)];
            const int bufferSize = line.bufferSize;

            line.buffer[static_cast<size_t>(line.writePos)] =
                ((lineIndex & 1) != 0 ? lineInputR : lineInputL) - line.filterState;

            if (++line.writePos >= bufferSize)
                line.writePos -= bufferSize;

            if (line.readPosFrac >= kDelayPosScale)
            {
                line.readPos += (line.readPosFrac >> kDelayPosShift);
                line.readPosFrac &= kDelayPosMask;
            }

            if (line.readPos >= bufferSize)
                line.readPos -= bufferSize;

            int readPos = line.readPos;
            const float frac = static_cast<float>(line.readPosFrac) * (1.0f / static_cast<float>(kDelayPosScale));

            float vm1 = 0.0f;
            float v0 = 0.0f;
            float v1 = 0.0f;
            float v2 = 0.0f;

            if (readPos > 0 && readPos < (bufferSize - 2))
            {
                vm1 = line.buffer[static_cast<size_t>(readPos - 1)];
                v0 = line.buffer[static_cast<size_t>(readPos)];
                v1 = line.buffer[static_cast<size_t>(readPos + 1)];
                v2 = line.buffer[static_cast<size_t>(readPos + 2)];
            }
            else
            {
                if (--readPos < 0)
                    readPos += bufferSize;
                vm1 = line.buffer[static_cast<size_t>(readPos)];

                if (++readPos >= bufferSize)
                    readPos -= bufferSize;
                v0 = line.buffer[static_cast<size_t>(readPos)];

                if (++readPos >= bufferSize)
                    readPos -= bufferSize;
                v1 = line.buffer[static_cast<size_t>(readPos)];

                if (++readPos >= bufferSize)
                    readPos -= bufferSize;
                v2 = line.buffer[static_cast<size_t>(readPos)];
            }

            float a2 = frac * frac;
            a2 -= 1.0f;
            a2 *= (1.0f / 6.0f);
            float a1 = frac;
            a1 += 1.0f;
            a1 *= 0.5f;
            float am1 = a1 - 1.0f;
            float a0 = 3.0f * a2;
            a1 -= a0;
            am1 -= a2;
            a0 -= frac;

            v0 = (am1 * vm1 + a0 * v0 + a1 * v1 + a2 * v2) * frac + v0;

            line.readPosFrac += line.readPosFracInc;

            v0 *= feedback_;
            v0 = (line.filterState - v0) * dampingCoefficient_ + v0;
            line.filterState = sanitize(v0);

            if ((lineIndex & 1) != 0)
                outputR += line.filterState;
            else
                outputL += line.filterState;

            if (--line.randLineCount <= 0)
                nextRandomLineSegment(line, lineIndex);
        }

        outL[sampleIndex] = sanitize(outputL * kOutputGain);
        outR[sampleIndex] = sanitize(outputR * kOutputGain);
    }
}

int ReverbSC::delayLineMaxSamples(int lineIndex) const noexcept
{
    const auto& params = kReverbParams[static_cast<size_t>(lineIndex)];
    double maxDelay = params[0];
    maxDelay += params[1] * static_cast<double>(kPitchMod) * 1.125;
    return static_cast<int>(maxDelay * sampleRate_ + 16.5);
}

void ReverbSC::initDelayLine(DelayLine& line, int lineIndex) noexcept
{
    line.bufferSize = delayLineMaxSamples(lineIndex);
    line.writePos = 0;
    line.seedVal = static_cast<int>(kReverbParams[static_cast<size_t>(lineIndex)][3] + 0.5);

    double readPos = static_cast<double>(line.seedVal) * kReverbParams[static_cast<size_t>(lineIndex)][1] / 32768.0;
    readPos = kReverbParams[static_cast<size_t>(lineIndex)][0] + (readPos * static_cast<double>(kPitchMod));
    readPos = static_cast<double>(line.bufferSize) - (readPos * sampleRate_);

    line.readPos = static_cast<int>(readPos);
    readPos = (readPos - static_cast<double>(line.readPos)) * static_cast<double>(kDelayPosScale);
    line.readPosFrac = static_cast<int>(readPos + 0.5);
    line.filterState = 0.0f;

    nextRandomLineSegment(line, lineIndex);
}

void ReverbSC::nextRandomLineSegment(DelayLine& line, int lineIndex) noexcept
{
    if (line.seedVal < 0)
        line.seedVal += 0x10000;

    line.seedVal = (line.seedVal * 15625 + 1) & 0xFFFF;

    if (line.seedVal >= 0x8000)
        line.seedVal -= 0x10000;

    const auto& params = kReverbParams[static_cast<size_t>(lineIndex)];
    line.randLineCount = static_cast<int>((sampleRate_ / params[2]) + 0.5);

    double previousDelay = static_cast<double>(line.writePos);
    previousDelay -= static_cast<double>(line.readPos) +
                     (static_cast<double>(line.readPosFrac) / static_cast<double>(kDelayPosScale));

    while (previousDelay < 0.0)
        previousDelay += static_cast<double>(line.bufferSize);

    previousDelay /= sampleRate_;

    double nextDelay = static_cast<double>(line.seedVal) * params[1] / 32768.0;
    nextDelay = params[0] + (nextDelay * static_cast<double>(kPitchMod));

    double phaseIncrement = (previousDelay - nextDelay) / static_cast<double>(line.randLineCount);
    phaseIncrement = phaseIncrement * sampleRate_ + 1.0;
    line.readPosFracInc = static_cast<int>(phaseIncrement * static_cast<double>(kDelayPosScale) + 0.5);
}

void ReverbSC::updateDampingCoefficient() noexcept
{
    const float coefficientDampingHz =
        std::min(dampingHz_, static_cast<float>(std::max(20.0, sampleRate_ * 0.49)));

    if (coefficientDampingHz == lastCoefficientDampingHz_)
        return;

    lastCoefficientDampingHz_ = coefficientDampingHz;

    const double coefficient = 2.0 - std::cos(static_cast<double>(coefficientDampingHz) * (2.0 * kPi) / sampleRate_);
    const double radicand = std::max(0.0, coefficient * coefficient - 1.0);
    dampingCoefficient_ = static_cast<float>(coefficient - std::sqrt(radicand));
}

float ReverbSC::sanitize(float sample) noexcept
{
    return std::isfinite(sample) ? sample : 0.0f;
}

} // namespace pedalboard3::dsp
