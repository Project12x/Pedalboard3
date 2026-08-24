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
    static void readIncomingAudio(juce::AudioBuffer<float>& destination) noexcept;
    void publish(const float* const* outputChannels, int numOutputChannels, int numSamples) noexcept;

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
    juce::AudioBuffer<float> incomingAudioBuffer;
    juce::AbstractFifo incomingAudioFifo{2};
    std::atomic<int> incomingAudioChannels{0};
    std::atomic<int> selectedIncomingChannel{-1};
    static std::atomic<LinkAudioService*> activeInstance;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LinkAudioService)
};
