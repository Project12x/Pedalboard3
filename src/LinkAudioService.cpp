/*
  LinkAudioService.cpp

  Ableton Link Audio is GPLv2-or-later. This code is built only as part of
  Pedalboard3's non-commercial AGPL/GPL distribution path.
*/

#include "LinkAudioService.h"

#if PEDALBOARD3_ENABLE_LINK_AUDIO
#include <ableton/LinkAudio.hpp>
#endif

#include <algorithm>
#include <cmath>

namespace
{
#if PEDALBOARD3_ENABLE_LINK_AUDIO
constexpr double kQuantum = 4.0;
constexpr std::size_t kMaximumChannels = 2;

int16_t floatToPcm16(float sample) noexcept
{
    const auto clamped = std::clamp(sample, -1.0f, 1.0f);
    return static_cast<int16_t>(std::lrintf(clamped * 32767.0f));
}
#endif
}  // namespace

#if PEDALBOARD3_ENABLE_LINK_AUDIO
LinkAudioService::LinkAudioService()
    : link(std::make_unique<ableton::LinkAudio>(120.0, "Pedalboard3"))
{
}
#else
LinkAudioService::LinkAudioService() = default;
#endif

LinkAudioService::~LinkAudioService() = default;

void LinkAudioService::prepare(double newSampleRate, int maximumBlockSize)
{
    sampleRate.store(newSampleRate, std::memory_order_release);
#if PEDALBOARD3_ENABLE_LINK_AUDIO
    const auto maximumSamples = static_cast<std::size_t>(jmax(1, maximumBlockSize)) * kMaximumChannels;
    if (!masterOutputSink)
        masterOutputSink = std::make_unique<ableton::LinkAudioSink>(*link, "Pedalboard3 Master", maximumSamples);
    else
        masterOutputSink->requestMaxNumSamples(maximumSamples);
#else
    juce::ignoreUnused(maximumBlockSize);
#endif
}

void LinkAudioService::setEnabled(bool shouldEnable)
{
#if PEDALBOARD3_ENABLE_LINK_AUDIO
    if (shouldEnable)
    {
        link->enable(true);
        link->enableLinkAudio(true);
        enabled.store(true, std::memory_order_release);
    }
    else
    {
        enabled.store(false, std::memory_order_release);
        link->enableLinkAudio(false);
        link->enable(false);
    }
#else
    juce::ignoreUnused(shouldEnable);
    enabled.store(false, std::memory_order_release);
#endif
}

bool LinkAudioService::isEnabled() const noexcept
{
    return enabled.load(std::memory_order_acquire);
}

void LinkAudioService::setTempo(double bpm) noexcept
{
    if (bpm > 0.0)
        requestedTempo.store(bpm, std::memory_order_release);
}

void LinkAudioService::publish(const float* const* outputChannels, int numOutputChannels, int numSamples) noexcept
{
#if PEDALBOARD3_ENABLE_LINK_AUDIO
    if (!isEnabled() || !masterOutputSink || outputChannels == nullptr || numOutputChannels <= 0 || numSamples <= 0)
        return;

    const auto numChannels = static_cast<std::size_t>(jmin(numOutputChannels, static_cast<int>(kMaximumChannels)));
    if (outputChannels[0] == nullptr || (numChannels == 2 && outputChannels[1] == nullptr))
        return;

    ableton::LinkAudioSink::BufferHandle buffer(*masterOutputSink);
    const auto requiredSamples = static_cast<std::size_t>(numSamples) * numChannels;
    if (!buffer || requiredSamples > buffer.maxNumSamples)
        return;

    const auto hostTime = link->clock().micros();
    auto sessionState = link->captureAudioSessionState();
    const auto tempo = requestedTempo.exchange(0.0, std::memory_order_acq_rel);
    if (tempo > 0.0)
        sessionState.setTempo(tempo, hostTime);

    const auto beatsAtBufferBegin = sessionState.beatAtTime(hostTime, kQuantum);
    for (int frame = 0; frame < numSamples; ++frame)
        for (std::size_t channel = 0; channel < numChannels; ++channel)
            buffer.samples[static_cast<std::size_t>(frame) * numChannels + channel] =
                floatToPcm16(outputChannels[channel][frame]);

    link->commitAudioSessionState(sessionState);
    buffer.commit(sessionState, beatsAtBufferBegin, kQuantum, static_cast<std::size_t>(numSamples), numChannels,
                  static_cast<uint32_t>(sampleRate.load(std::memory_order_acquire)));
#else
    juce::ignoreUnused(outputChannels, numOutputChannels, numSamples);
#endif
}
