/*
  LinkAudioService.h

  Runtime-opt-in Ableton Link Audio publisher for Pedalboard3's master output.
*/

#pragma once

#include "MeteringCallbackBounds.h"

#include <JuceHeader.h>

#include <atomic>
#include <memory>
#include <vector>

#if PEDALBOARD3_ENABLE_LINK_AUDIO
namespace ableton
{
class LinkAudio;
class LinkAudioSink;
class LinkAudioSource;
}  // namespace ableton
#endif

class LinkAudioService
{
  public:
    static constexpr const char* settingsKey = "enableAbletonLinkAudio";
    static constexpr int maxChannels = MeteringCallbackBounds::MaxChannels;

    /// One node's set of per-channel-pair Link Audio sinks, alongside the
    /// master sinks in outputSinks. Owned by nodeSinks below; a stable
    /// pointer to one of these is what opted-in nodes (see AudioTapSource)
    /// hold and read from the audio thread.
    struct NodeSinkGroup
    {
        AudioProcessorGraph::NodeID nodeId;
#if PEDALBOARD3_ENABLE_LINK_AUDIO
        std::vector<std::unique_ptr<ableton::LinkAudioSink>> sinks;
        // Clamped channel count sinks was registered with - kept so prepare()
        // can correctly re-split channelsInOutputSink() per sink when the
        // device's block size changes, instead of over-allocating flatly.
        int numChannels = 0;
#endif
    };

    LinkAudioService();
    ~LinkAudioService();

    // numOutputChannels is the live active-output-channel count (e.g. from
    // AudioIODevice::getActiveOutputChannels()); it determines how many
    // stereo/mono Link Audio sinks get announced (see .cpp).
    void prepare(double sampleRate, int maximumBlockSize, int numOutputChannels);
    void setEnabled(bool shouldEnable);
    bool isEnabled() const noexcept;
    void setTempo(double bpm) noexcept;
    void setPeerName(const juce::String& name);
    juce::String getPeerName() const;
    int getPeerCount() const noexcept;
    juce::StringArray getAvailableChannels() const;
    void selectIncomingChannel(int channelIndex);
    int getSelectedIncomingChannel() const noexcept;
    static void setActiveInstance(LinkAudioService* service) noexcept;
    static LinkAudioService* getActiveInstance() noexcept;
    static void readIncomingAudio(juce::AudioBuffer<float>& destination) noexcept;
    void publish(const float* const* outputChannels, int numOutputChannels, int numSamples) noexcept;

    // Per-node sink lifecycle. Always called from the message thread. The
    // returned/looked-up NodeSinkGroup* is stable for as long as the node
    // stays registered - it's what AudioTapSource-implementing nodes store
    // and read on the audio thread via a single atomic pointer.
    // Callers are responsible for RT-safety around teardown: clear the
    // node's tap slot (AudioTapSource::setLinkAudioSinkSlot(nullptr)) and
    // call unregisterNodeSink() under the owning AudioProcessorGraph's
    // callback lock, so the audio thread can never be mid-publish when the
    // sink group is destroyed. Returns nullptr if Link Audio support isn't
    // compiled in.
    NodeSinkGroup* registerNodeSink(AudioProcessorGraph::NodeID nodeId, int numChannels,
                                    const juce::String& displayName);
    void unregisterNodeSink(AudioProcessorGraph::NodeID nodeId);
    void clearAllNodeSinks();

    // RT-safe: writes into slot's pre-allocated sinks only. No lookup beyond
    // the caller-supplied pointer, no allocation. Mirrors publish()'s
    // master-bus session/commit logic, scoped to one node's sinks. Called by
    // AudioTapSource via LinkAudioService::getActiveInstance() - see
    // AudioTapSource.h.
    void publishNodeAudio(NodeSinkGroup* slot, const float* const* channels, int numChannels,
                          int numSamples) noexcept;

    // The graph's Audio Input / Audio Output nodes are AudioGraphIOProcessor
    // instances - JUCE's AudioProcessorGraph render sequence special-cases
    // these and copies buffers directly (see AudioInOp/AudioOutOp in
    // juce_AudioProcessorGraph.cpp), never actually calling their
    // processBlock(). They can't implement AudioTapSource like every other
    // node, so they get two dedicated slots instead, fed directly from
    // MeteringProcessorPlayer's audio callback (which already has the raw
    // device input and final device output buffers to hand).
    void setAudioInputTapSlot(NodeSinkGroup* slot) noexcept { audioInputTap.store(slot, std::memory_order_release); }
    void setAudioOutputTapSlot(NodeSinkGroup* slot) noexcept
    {
        audioOutputTap.store(slot, std::memory_order_release);
    }
    NodeSinkGroup* getAudioInputTapSlot() const noexcept { return audioInputTap.load(std::memory_order_acquire); }
    NodeSinkGroup* getAudioOutputTapSlot() const noexcept { return audioOutputTap.load(std::memory_order_acquire); }

  private:
#if PEDALBOARD3_ENABLE_LINK_AUDIO
    std::unique_ptr<ableton::LinkAudio> link;
    // Ableton's LinkAudio SDK models a "channel" as one mono/stereo sink
    // (see BufferHandle::commit()'s doc comment) - multichannel output is
    // published as multiple stereo (or a trailing mono) sinks, not one wide
    // sink. Rebuilt in prepare() when the required sink count changes.
    std::vector<std::unique_ptr<ableton::LinkAudioSink>> outputSinks;
    std::unique_ptr<ableton::LinkAudioSource> incomingAudioSource;
#endif
    void readIncoming(juce::AudioBuffer<float>& destination) noexcept;
    std::atomic<bool> enabled{false};
    std::atomic<double> requestedTempo{0.0};
    std::atomic<double> sampleRate{44100.0};
    std::atomic<int> currentMaxBlockSize{0};
    juce::AudioBuffer<float> incomingAudioBuffer;
    juce::AbstractFifo incomingAudioFifo{2};
    std::atomic<int> incomingAudioChannels{0};
    std::atomic<int> selectedIncomingChannel{-1};
    static std::atomic<LinkAudioService*> activeInstance;

    // Message-thread only - node sinks are only ever registered/unregistered
    // in response to UI actions or patch load, never from the audio thread.
    // unique_ptr-per-entry keeps NodeSinkGroup addresses stable across
    // insertions/erasures of other entries.
    std::vector<std::unique_ptr<NodeSinkGroup>> nodeSinks;
    std::atomic<NodeSinkGroup*> audioInputTap{nullptr};
    std::atomic<NodeSinkGroup*> audioOutputTap{nullptr};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LinkAudioService)
};
