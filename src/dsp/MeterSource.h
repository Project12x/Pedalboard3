//  MeterSource.h - Fixed-capacity audio meter source for audio-to-UI polling
//  ----------------------------------------------------------------------------
//  This file is part of Pedalboard3, an audio plugin host.
//  Copyright (c) 2026.
//
//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//  ----------------------------------------------------------------------------

#pragma once

#include <array>
#include <atomic>

/// RT-safe fixed-capacity source for peak/RMS/VU/clip meter readings.
///
/// Design references, pattern-only:
/// - ffAudio/ff_meters LevelMeterSource.h, commit 968bb8e, BSD-3-Clause:
///   separate audio measurement from GUI polling.
/// - SoundDevelopment/sound_meter sd_MeterLevel.*, commit a614425, MIT:
///   atomic audio/UI handoff with explicit meter ballistics.
class PedalboardMeterSource
{
public:
    static constexpr int MaxChannels = 16;
    static constexpr int RmsWindowBlocks = 16;

    PedalboardMeterSource();

    void prepare(double newSampleRate, int newNumChannels) noexcept;
    void reset() noexcept;

    void process(const float* const* channelData, int numChannels, int numSamples) noexcept;
    void processChannel(int channel, const float* data, int numSamples) noexcept;
    void clearInactiveChannelsFrom(int firstActiveChannel) noexcept;

    int getNumChannels() const noexcept;

    float getPeak(int channel) const noexcept;
    float getRms(int channel) const noexcept;
    float getVu(int channel) const noexcept;
    bool getClip(int channel) const noexcept;
    bool getAndClearClip(int channel) noexcept;

private:
    struct ChannelState
    {
        std::atomic<float> peak{0.0f};
        std::atomic<float> rms{0.0f};
        std::atomic<float> vu{0.0f};
        std::atomic<bool> clip{false};

        float peakValue = 0.0f;
        float vuValue = 0.0f;
        std::array<float, RmsWindowBlocks> rmsSquares{};
        float rmsSquareSum = 0.0f;
        int rmsWriteIndex = 0;
        int rmsBlocksSeen = 0;
    };

    static bool isValidChannel(int channel) noexcept;
    static float sanitiseSample(float sample) noexcept;
    static int clampChannelCount(int channelCount) noexcept;

    void resetChannel(ChannelState& channel) noexcept;
    void pushBlockRms(ChannelState& channel, float blockRms) noexcept;

    std::array<ChannelState, MaxChannels> channels;
    std::atomic<int> activeChannels{0};
    double sampleRate = 44100.0;
    float peakDecayCoeff = 0.99995f;
    float vuCoeff = 0.0001f;
};
