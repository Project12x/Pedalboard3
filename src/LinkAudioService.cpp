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
// Ceiling for the incoming-audio FIFO/buffer and the publish-side channel
// clamp, shared with the rest of the engine (MeteringCallbackBounds). Actual
// channel counts used each callback are always the live device's own count.
constexpr std::size_t kMaximumChannels = LinkAudioService::maxChannels;
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

#if PEDALBOARD3_ENABLE_LINK_AUDIO
namespace
{
// Sink i (0-based) carries channels [i*2, i*2+channelsInSink), 2 channels
// (stereo) unless it's the trailing sink for an odd channel count (mono).
int channelsInOutputSink(std::size_t sinkIndex, int totalChannels) noexcept
{
    return jmin(2, totalChannels - static_cast<int>(sinkIndex) * 2);
}
}  // namespace
#endif

void LinkAudioService::prepare(double newSampleRate, int maximumBlockSize, int numOutputChannels)
{
    sampleRate.store(newSampleRate, std::memory_order_release);
    currentMaxBlockSize.store(maximumBlockSize, std::memory_order_release);
    const auto fifoFrames = jmax(32768, maximumBlockSize * 64);
    incomingAudioBuffer.setSize(static_cast<int>(kMaximumChannels), fifoFrames, false, true, true);
    incomingAudioFifo.setTotalSize(fifoFrames);
    incomingAudioFifo.reset();
#if PEDALBOARD3_ENABLE_LINK_AUDIO
    const auto clampedChannels = jlimit(0, static_cast<int>(kMaximumChannels), numOutputChannels);
    const auto requiredSinks = static_cast<std::size_t>((clampedChannels + 1) / 2);

    if (outputSinks.size() != requiredSinks)
    {
        outputSinks.clear();
        outputSinks.reserve(requiredSinks);
        for (std::size_t i = 0; i < requiredSinks; ++i)
        {
            const auto channelsInSink = channelsInOutputSink(i, clampedChannels);
            const auto firstChannel = static_cast<int>(i) * 2;

            juce::String name("Pedalboard3 Master ");
            name << (firstChannel + 1);
            if (channelsInSink > 1)
                name << "-" << (firstChannel + channelsInSink);

            const auto maximumSamples =
                static_cast<std::size_t>(jmax(1, maximumBlockSize)) * static_cast<std::size_t>(channelsInSink);
            outputSinks.push_back(
                std::make_unique<ableton::LinkAudioSink>(*link, name.toStdString(), maximumSamples));
        }
    }
    else
    {
        for (std::size_t i = 0; i < outputSinks.size(); ++i)
        {
            const auto channelsInSink = channelsInOutputSink(i, clampedChannels);
            const auto maximumSamples =
                static_cast<std::size_t>(jmax(1, maximumBlockSize)) * static_cast<std::size_t>(channelsInSink);
            outputSinks[i]->requestMaxNumSamples(maximumSamples);
        }
    }

    // Re-request buffer sizing for already-registered per-node sinks too -
    // the device's block size can change after a node was opted in. Mirrors
    // the per-sink channel split used above and at registration time, rather
    // than a flat size.
    for (auto& group : nodeSinks)
        for (std::size_t i = 0; i < group->sinks.size(); ++i)
        {
            const auto channelsInSink = channelsInOutputSink(i, group->numChannels);
            group->sinks[i]->requestMaxNumSamples(static_cast<std::size_t>(jmax(1, maximumBlockSize))
                                                  * static_cast<std::size_t>(channelsInSink));
        }
#else
    juce::ignoreUnused(maximumBlockSize, numOutputChannels);
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

LinkAudioService* LinkAudioService::getActiveInstance() noexcept
{
    return activeInstance.load(std::memory_order_acquire);
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
    if (!isEnabled() || outputSinks.empty() || outputChannels == nullptr || numOutputChannels <= 0 || numSamples <= 0)
        return;

    const auto totalChannels = jmin(numOutputChannels, static_cast<int>(kMaximumChannels));
    for (int channel = 0; channel < totalChannels; ++channel)
        if (outputChannels[channel] == nullptr)
            return;

    const auto hostTime = link->clock().micros();
    auto sessionState = link->captureAudioSessionState();
    const auto tempo = requestedTempo.exchange(0.0, std::memory_order_acq_rel);
    if (tempo > 0.0)
        sessionState.setTempo(tempo, hostTime);

    const auto beatsAtBufferBegin = sessionState.beatAtTime(hostTime, kQuantum);
    const auto sr = static_cast<uint32_t>(sampleRate.load(std::memory_order_acquire));

    for (std::size_t sinkIndex = 0; sinkIndex < outputSinks.size(); ++sinkIndex)
    {
        const auto channelsInSink = channelsInOutputSink(sinkIndex, totalChannels);
        if (channelsInSink <= 0)
            break;
        const auto firstChannel = static_cast<int>(sinkIndex) * 2;

        ableton::LinkAudioSink::BufferHandle buffer(*outputSinks[sinkIndex]);
        const auto requiredSamples = static_cast<std::size_t>(numSamples) * static_cast<std::size_t>(channelsInSink);
        if (!buffer || requiredSamples > buffer.maxNumSamples)
            continue;

        for (int frame = 0; frame < numSamples; ++frame)
            for (int ch = 0; ch < channelsInSink; ++ch)
                buffer.samples[static_cast<std::size_t>(frame) * static_cast<std::size_t>(channelsInSink) +
                               static_cast<std::size_t>(ch)] = floatToPcm16(outputChannels[firstChannel + ch][frame]);

        buffer.commit(sessionState, beatsAtBufferBegin, kQuantum, static_cast<std::size_t>(numSamples),
                      static_cast<std::size_t>(channelsInSink), sr);
    }

    link->commitAudioSessionState(sessionState);
#else
    juce::ignoreUnused(outputChannels, numOutputChannels, numSamples);
#endif
}

LinkAudioService::NodeSinkGroup* LinkAudioService::registerNodeSink(AudioProcessorGraph::NodeID nodeId,
                                                                     int numChannels, const juce::String& displayName)
{
#if PEDALBOARD3_ENABLE_LINK_AUDIO
    // Re-registering an already-registered node (e.g. toggled off then back
    // on) replaces its sink group rather than accumulating duplicates.
    unregisterNodeSink(nodeId);

    auto group = std::make_unique<NodeSinkGroup>();
    group->nodeId = nodeId;

    const auto clampedChannels = jlimit(0, static_cast<int>(kMaximumChannels), numChannels);
    group->numChannels = clampedChannels;
    const auto requiredSinks = static_cast<std::size_t>(jmax(0, (clampedChannels + 1) / 2));
    const auto maximumBlockSize = jmax(1, currentMaxBlockSize.load(std::memory_order_acquire));

    group->sinks.reserve(requiredSinks);
    for (std::size_t i = 0; i < requiredSinks; ++i)
    {
        const auto channelsInSink = channelsInOutputSink(i, clampedChannels);
        const auto firstChannel = static_cast<int>(i) * 2;

        juce::String name("Pedalboard3 - ");
        name << displayName << " " << (firstChannel + 1);
        if (channelsInSink > 1)
            name << "-" << (firstChannel + channelsInSink);

        const auto maximumSamples =
            static_cast<std::size_t>(maximumBlockSize) * static_cast<std::size_t>(channelsInSink);
        group->sinks.push_back(std::make_unique<ableton::LinkAudioSink>(*link, name.toStdString(), maximumSamples));
    }

    nodeSinks.push_back(std::move(group));
    return nodeSinks.back().get();
#else
    juce::ignoreUnused(nodeId, numChannels, displayName);
    return nullptr;
#endif
}

void LinkAudioService::unregisterNodeSink(AudioProcessorGraph::NodeID nodeId)
{
    for (const auto& group : nodeSinks)
    {
        if (group->nodeId != nodeId)
            continue;
        // Defensive: a caller that forgot to clear the device-tap alias
        // before unregistering would otherwise leave it dangling.
        if (audioInputTap.load(std::memory_order_acquire) == group.get())
            audioInputTap.store(nullptr, std::memory_order_release);
        if (audioOutputTap.load(std::memory_order_acquire) == group.get())
            audioOutputTap.store(nullptr, std::memory_order_release);
    }

    nodeSinks.erase(std::remove_if(nodeSinks.begin(), nodeSinks.end(),
                                   [nodeId](const std::unique_ptr<NodeSinkGroup>& group)
                                   { return group->nodeId == nodeId; }),
                    nodeSinks.end());
}

void LinkAudioService::clearAllNodeSinks()
{
    // Callers must have already nulled out any AudioTapSource slots pointing
    // into nodeSinks (the nodes themselves are being destroyed by a graph
    // rebuild). The two device-tap aliases point into the same storage, so
    // they'd dangle after the clear below if left as-is.
    audioInputTap.store(nullptr, std::memory_order_release);
    audioOutputTap.store(nullptr, std::memory_order_release);
    nodeSinks.clear();
}

void LinkAudioService::publishNodeAudio(NodeSinkGroup* slot, const float* const* channels, int numChannels,
                                        int numSamples) noexcept
{
#if PEDALBOARD3_ENABLE_LINK_AUDIO
    if (!isEnabled() || slot == nullptr || slot->sinks.empty() || channels == nullptr || numChannels <= 0
        || numSamples <= 0)
        return;

    const auto totalChannels = jmin(numChannels, static_cast<int>(kMaximumChannels));
    for (int channel = 0; channel < totalChannels; ++channel)
        if (channels[channel] == nullptr)
            return;

    const auto hostTime = link->clock().micros();
    auto sessionState = link->captureAudioSessionState();
    const auto beatsAtBufferBegin = sessionState.beatAtTime(hostTime, kQuantum);
    const auto sr = static_cast<uint32_t>(sampleRate.load(std::memory_order_acquire));

    for (std::size_t sinkIndex = 0; sinkIndex < slot->sinks.size(); ++sinkIndex)
    {
        const auto channelsInSink = channelsInOutputSink(sinkIndex, totalChannels);
        if (channelsInSink <= 0)
            break;
        const auto firstChannel = static_cast<int>(sinkIndex) * 2;

        ableton::LinkAudioSink::BufferHandle buffer(*slot->sinks[sinkIndex]);
        const auto requiredSamples = static_cast<std::size_t>(numSamples) * static_cast<std::size_t>(channelsInSink);
        if (!buffer || requiredSamples > buffer.maxNumSamples)
            continue;

        for (int frame = 0; frame < numSamples; ++frame)
            for (int ch = 0; ch < channelsInSink; ++ch)
                buffer.samples[static_cast<std::size_t>(frame) * static_cast<std::size_t>(channelsInSink) +
                               static_cast<std::size_t>(ch)] = floatToPcm16(channels[firstChannel + ch][frame]);

        buffer.commit(sessionState, beatsAtBufferBegin, kQuantum, static_cast<std::size_t>(numSamples),
                      static_cast<std::size_t>(channelsInSink), sr);
    }

    link->commitAudioSessionState(sessionState);
#else
    juce::ignoreUnused(slot, channels, numChannels, numSamples);
#endif
}
