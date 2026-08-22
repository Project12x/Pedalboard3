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
// Pedalboard3's device engine supports 16 discrete channels. Link Audio buffers
// are interleaved, so reserve capacity for the full engine width rather than
// narrowing the master bus to stereo.
constexpr std::size_t kMaximumChannels = 16;
#if PEDALBOARD3_ENABLE_LINK_AUDIO
constexpr double kQuantum = 4.0;

int16_t floatToPcm16(float sample) noexcept
{
    const auto clamped = std::clamp(sample, -1.0f, 1.0f);
    return static_cast<int16_t>(std::lrintf(clamped * 32767.0f));
}
#endif
}  // namespace

std::atomic<LinkAudioService*> LinkAudioService::activeInstance{nullptr};

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
    const auto fifoFrames = jmax(32768, maximumBlockSize * 64);
    incomingAudioBuffer.setSize(static_cast<int>(kMaximumChannels), fifoFrames, false, true, true);
    incomingAudioFifo.setTotalSize(fifoFrames);
    incomingAudioFifo.reset();
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

void LinkAudioService::setPeerName(const juce::String& name)
{
#if PEDALBOARD3_ENABLE_LINK_AUDIO
    link->setPeerName(name.trim().isEmpty() ? "Pedalboard3" : name.trim().toStdString());
#else
    juce::ignoreUnused(name);
#endif
}

juce::String LinkAudioService::getPeerName() const
{
#if PEDALBOARD3_ENABLE_LINK_AUDIO
    return juce::String(link->peerName());
#else
    return "Pedalboard3";
#endif
}

int LinkAudioService::getPeerCount() const noexcept
{
#if PEDALBOARD3_ENABLE_LINK_AUDIO
    return static_cast<int>(link->numPeers());
#else
    return 0;
#endif
}

juce::StringArray LinkAudioService::getAvailableChannels() const
{
    juce::StringArray result;
#if PEDALBOARD3_ENABLE_LINK_AUDIO
    for (const auto& channel : link->channels())
        result.add(juce::String(channel.peerName) + " - " + juce::String(channel.name));
#endif
    return result;
}

void LinkAudioService::selectIncomingChannel(int channelIndex)
{
#if PEDALBOARD3_ENABLE_LINK_AUDIO
    incomingAudioSource.reset();
    selectedIncomingChannel.store(-1, std::memory_order_release);
    incomingAudioFifo.reset();
    const auto channels = link->channels();
    if (channelIndex < 0 || channelIndex >= static_cast<int>(channels.size()))
        return;

    incomingAudioSource = std::make_unique<ableton::LinkAudioSource>(
        *link, channels[static_cast<std::size_t>(channelIndex)].id,
        [this, channelIndex](ableton::LinkAudioSource::BufferHandle buffer)
        {
            const auto channelsToCopy = jmin(static_cast<int>(buffer.info.numChannels), static_cast<int>(kMaximumChannels));
            const auto frames = jmin(static_cast<int>(buffer.info.numFrames), incomingAudioFifo.getFreeSpace());
            if (channelsToCopy <= 0 || frames <= 0 || buffer.info.sampleRate != static_cast<uint32_t>(sampleRate.load()))
                return;
            const auto write = incomingAudioFifo.write(frames);
            const auto copy = [&](int start, int count, int offset)
            {
                for (int ch = 0; ch < channelsToCopy; ++ch)
                    for (int frame = 0; frame < count; ++frame)
                        incomingAudioBuffer.setSample(ch, start + frame,
                            static_cast<float>(buffer.samples[(offset + frame) * channelsToCopy + ch]) / 32768.0f);
            };
            copy(write.startIndex1, write.blockSize1, 0);
            copy(write.startIndex2, write.blockSize2, write.blockSize1);
            incomingAudioChannels.store(channelsToCopy, std::memory_order_release);
            selectedIncomingChannel.store(channelIndex, std::memory_order_release);
        });
#else
    juce::ignoreUnused(channelIndex);
#endif
}

int LinkAudioService::getSelectedIncomingChannel() const noexcept
{
    return selectedIncomingChannel.load(std::memory_order_acquire);
}

void LinkAudioService::setActiveInstance(LinkAudioService* service) noexcept
{
    activeInstance.store(service, std::memory_order_release);
}

void LinkAudioService::readIncomingAudio(juce::AudioBuffer<float>& destination) noexcept
{
    if (auto* service = activeInstance.load(std::memory_order_acquire))
        service->readIncoming(destination);
    else
        destination.clear();
}

void LinkAudioService::readIncoming(juce::AudioBuffer<float>& destination) noexcept
{
    destination.clear();
    const auto channels = jmin(destination.getNumChannels(), incomingAudioChannels.load(std::memory_order_acquire));
    const auto read = incomingAudioFifo.read(destination.getNumSamples());
    const auto copy = [&](int start, int count, int offset)
    {
        for (int ch = 0; ch < channels; ++ch)
            destination.copyFrom(ch, offset, incomingAudioBuffer, ch, start, count);
    };
    copy(read.startIndex1, read.blockSize1, 0);
    copy(read.startIndex2, read.blockSize2, read.blockSize1);
}

void LinkAudioService::publish(const float* const* outputChannels, int numOutputChannels, int numSamples) noexcept
{
#if PEDALBOARD3_ENABLE_LINK_AUDIO
    if (!isEnabled() || !masterOutputSink || outputChannels == nullptr || numOutputChannels <= 0 || numSamples <= 0)
        return;

    const auto numChannels = static_cast<std::size_t>(jmin(numOutputChannels, static_cast<int>(kMaximumChannels)));
    for (std::size_t channel = 0; channel < numChannels; ++channel)
        if (outputChannels[channel] == nullptr)
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
