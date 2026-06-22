//  MeterSource.cpp - Fixed-capacity audio meter source for audio-to-UI polling
//  ----------------------------------------------------------------------------
//  This file is part of Pedalboard3, an audio plugin host.
//  Copyright (c) 2026.
//
//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//  ----------------------------------------------------------------------------

#include "MeterSource.h"

#include <algorithm>
#include <cmath>

namespace
{
constexpr float ClipThreshold = 1.0f;
constexpr float DenormalFloor = 1.0e-10f;
constexpr float MaxReasonableMeterSample = 10.0f;

float coefficientFor99PercentInSeconds(double sampleRate, double seconds) noexcept
{
    if (sampleRate <= 0.0 || seconds <= 0.0)
        return 1.0f;

    return static_cast<float>(1.0 - std::pow(0.01, 1.0 / (sampleRate * seconds)));
}

float decayCoefficientToMinus60InSeconds(double sampleRate, double seconds) noexcept
{
    if (sampleRate <= 0.0 || seconds <= 0.0)
        return 0.0f;

    return static_cast<float>(std::pow(0.001, 1.0 / (sampleRate * seconds)));
}
} // namespace

PedalboardMeterSource::PedalboardMeterSource()
{
    prepare(44100.0, 0);
}

void PedalboardMeterSource::prepare(double newSampleRate, int newNumChannels) noexcept
{
    sampleRate = newSampleRate > 0.0 ? newSampleRate : 44100.0;
    activeChannels.store(clampChannelCount(newNumChannels), std::memory_order_relaxed);

    peakDecayCoeff = decayCoefficientToMinus60InSeconds(sampleRate, 3.0);
    vuCoeff = coefficientFor99PercentInSeconds(sampleRate, 0.3);

    reset();
}

void PedalboardMeterSource::reset() noexcept
{
    for (auto& channel : channels)
        resetChannel(channel);
}

void PedalboardMeterSource::process(const float* const* channelData, int numChannels, int numSamples) noexcept
{
    if (numSamples <= 0)
        return;

    const int channelCount = clampChannelCount(numChannels);
    activeChannels.store(channelCount, std::memory_order_relaxed);

    for (int channel = 0; channel < channelCount; ++channel)
    {
        const float* data = channelData != nullptr ? channelData[channel] : nullptr;
        processChannel(channel, data, numSamples);
    }

    clearInactiveChannelsFrom(channelCount);
}

void PedalboardMeterSource::processChannel(int channel, const float* data, int numSamples) noexcept
{
    if (!isValidChannel(channel) || numSamples <= 0)
        return;

    auto& state = channels[static_cast<size_t>(channel)];
    float peak = state.peakValue;
    float vu = state.vuValue;
    float squareSum = 0.0f;
    int validSamples = 0;

    for (int sampleIndex = 0; sampleIndex < numSamples; ++sampleIndex)
    {
        const float sample = data != nullptr ? sanitiseSample(data[sampleIndex]) : 0.0f;
        const float magnitude = std::abs(sample);

        if (magnitude > ClipThreshold)
            state.clip.store(true, std::memory_order_relaxed);

        peak = magnitude > peak ? magnitude : peak * peakDecayCoeff;
        vu += vuCoeff * (magnitude - vu);

        squareSum += sample * sample;
        ++validSamples;
    }

    if (peak < DenormalFloor)
        peak = 0.0f;
    if (vu < DenormalFloor)
        vu = 0.0f;

    state.peakValue = peak;
    state.vuValue = vu;
    state.peak.store(peak, std::memory_order_relaxed);
    state.vu.store(vu, std::memory_order_relaxed);

    const float blockRms =
        validSamples > 0 ? std::sqrt(squareSum / static_cast<float>(validSamples)) : 0.0f;
    pushBlockRms(state, blockRms);
}

void PedalboardMeterSource::clearInactiveChannelsFrom(int firstActiveChannel) noexcept
{
    const int first = clampChannelCount(firstActiveChannel);
    for (int channel = first; channel < MaxChannels; ++channel)
        resetChannel(channels[static_cast<size_t>(channel)]);
}

int PedalboardMeterSource::getNumChannels() const noexcept
{
    return activeChannels.load(std::memory_order_relaxed);
}

float PedalboardMeterSource::getPeak(int channel) const noexcept
{
    if (!isValidChannel(channel))
        return 0.0f;

    return channels[static_cast<size_t>(channel)].peak.load(std::memory_order_relaxed);
}

float PedalboardMeterSource::getRms(int channel) const noexcept
{
    if (!isValidChannel(channel))
        return 0.0f;

    return channels[static_cast<size_t>(channel)].rms.load(std::memory_order_relaxed);
}

float PedalboardMeterSource::getVu(int channel) const noexcept
{
    if (!isValidChannel(channel))
        return 0.0f;

    return channels[static_cast<size_t>(channel)].vu.load(std::memory_order_relaxed);
}

bool PedalboardMeterSource::getClip(int channel) const noexcept
{
    if (!isValidChannel(channel))
        return false;

    return channels[static_cast<size_t>(channel)].clip.load(std::memory_order_relaxed);
}

bool PedalboardMeterSource::getAndClearClip(int channel) noexcept
{
    if (!isValidChannel(channel))
        return false;

    return channels[static_cast<size_t>(channel)].clip.exchange(false, std::memory_order_relaxed);
}

bool PedalboardMeterSource::isValidChannel(int channel) noexcept
{
    return channel >= 0 && channel < MaxChannels;
}

float PedalboardMeterSource::sanitiseSample(float sample) noexcept
{
    if (!std::isfinite(sample))
        return 0.0f;

    if (std::abs(sample) > MaxReasonableMeterSample)
        return 0.0f;

    return sample;
}

int PedalboardMeterSource::clampChannelCount(int channelCount) noexcept
{
    return std::max(0, std::min(channelCount, MaxChannels));
}

void PedalboardMeterSource::resetChannel(ChannelState& channel) noexcept
{
    channel.peak.store(0.0f, std::memory_order_relaxed);
    channel.rms.store(0.0f, std::memory_order_relaxed);
    channel.vu.store(0.0f, std::memory_order_relaxed);
    channel.clip.store(false, std::memory_order_relaxed);

    channel.peakValue = 0.0f;
    channel.vuValue = 0.0f;
    channel.rmsSquares.fill(0.0f);
    channel.rmsSquareSum = 0.0f;
    channel.rmsWriteIndex = 0;
    channel.rmsBlocksSeen = 0;
}

void PedalboardMeterSource::pushBlockRms(ChannelState& channel, float blockRms) noexcept
{
    blockRms = std::max(0.0f, blockRms);
    const float square = blockRms * blockRms;
    const auto index = static_cast<size_t>(channel.rmsWriteIndex);

    channel.rmsSquareSum -= channel.rmsSquares[index];
    channel.rmsSquares[index] = square;
    channel.rmsSquareSum += square;
    channel.rmsWriteIndex = (channel.rmsWriteIndex + 1) % RmsWindowBlocks;
    channel.rmsBlocksSeen = std::min(channel.rmsBlocksSeen + 1, RmsWindowBlocks);

    const float divisor = static_cast<float>(std::max(channel.rmsBlocksSeen, 1));
    const float rms = std::sqrt(std::max(0.0f, channel.rmsSquareSum) / divisor);
    channel.rms.store(rms < DenormalFloor ? 0.0f : rms, std::memory_order_relaxed);
}
