#pragma once
#include "ScratchTake.h"
#include <JuceHeader.h>
#include <mutex>

// Control/UI only. The cache must outlive its sources and published pairs.
struct ScratchWaveformPair
{
    ScratchWaveformPair(juce::AudioFormatManager& formats, juce::AudioThumbnailCache& cache)
        : raw(512, formats, cache), wet(512, formats, cache) {}
    juce::File directory;
    juce::AudioThumbnail raw, wet;
};

struct ScratchWaveformSlot
{
    std::mutex mutex;
    uint64_t generation = 0;
    std::shared_ptr<ScratchWaveformPair> data;
};

class ScratchWaveformCache
{
public:
    ScratchWaveformCache();
    ~ScratchWaveformCache();
    void request(const std::shared_ptr<ScratchWaveformSlot>&, uint64_t, ScratchTakeLoadResult);
private:
    juce::AudioFormatManager formats;
    juce::AudioThumbnailCache thumbnails{32};
    juce::ThreadPool loader{1};
};

class ScratchWaveformSource
{
public:
    explicit ScratchWaveformSource(ScratchWaveformCache& cacheToUse) : cache(cacheToUse) {}
    ~ScratchWaveformSource() { clear(); }
    void setTake(const ScratchTakeLoadResult&);
    void clear();
    std::shared_ptr<ScratchWaveformPair> snapshot() const;
private:
    ScratchWaveformCache& cache;
    std::shared_ptr<ScratchWaveformSlot> slot = std::make_shared<ScratchWaveformSlot>();
};

class ScratchWaveformView final : public juce::Component, private juce::Timer
{
public:
    explicit ScratchWaveformView(ScratchWaveformCache&, bool compact = false);
    void setTake(const ScratchTakeLoadResult&);
    void clear();
    void setPlayheadSeconds(double);
    int preferredHeight() const;
    std::function<void(double)> onSeek;
    void paint(juce::Graphics&) override;
    void mouseDown(const juce::MouseEvent&) override;
private:
    void timerCallback() override;
    ScratchWaveformSource source;
    std::shared_ptr<ScratchWaveformPair> data;
    bool compact = false, requested = false;
    double playhead = 0;
};
