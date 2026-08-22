/*
  LinkAudioService.h

  Runtime-opt-in Ableton Link Audio publisher for Pedalboard3's master output.
*/

#pragma once

#include <JuceHeader.h>

#include <atomic>
#include <memory>

#if PEDALBOARD3_ENABLE_LINK_AUDIO
namespace ableton
{
class LinkAudio;
class LinkAudioSink;
}  // namespace ableton
#endif

class LinkAudioService
{
  public:
    static constexpr const char* settingsKey = "enableAbletonLinkAudio";

    LinkAudioService();
    ~LinkAudioService();

    void prepare(double sampleRate, int maximumBlockSize);
    void setEnabled(bool shouldEnable);
    bool isEnabled() const noexcept;
    void setTempo(double bpm) noexcept;
    void publish(const float* const* outputChannels, int numOutputChannels, int numSamples) noexcept;

  private:
#if PEDALBOARD3_ENABLE_LINK_AUDIO
    std::unique_ptr<ableton::LinkAudio> link;
    std::unique_ptr<ableton::LinkAudioSink> masterOutputSink;
#endif
    std::atomic<bool> enabled{false};
    std::atomic<double> requestedTempo{0.0};
    std::atomic<double> sampleRate{44100.0};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LinkAudioService)
};
