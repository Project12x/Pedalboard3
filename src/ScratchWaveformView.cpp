#include "ScratchWaveformView.h"
#include "ColourScheme.h"
#include "FontManager.h"

namespace
{
juce::Colour colour(const char* key, juce::Colour fallback)
{
    const auto& palette = ColourScheme::getInstance().colours;
    const auto found = palette.find(key);
    return found == palette.end() ? fallback : found->second;
}
}

ScratchWaveformCache::ScratchWaveformCache() { formats.registerFormat(new juce::WavAudioFormat(), true); }
ScratchWaveformCache::~ScratchWaveformCache() { loader.removeAllJobs(true, -1); }

void ScratchWaveformCache::request(const std::shared_ptr<ScratchWaveformSlot>& slot,
                                  uint64_t generation, ScratchTakeLoadResult take)
{
    const std::weak_ptr<ScratchWaveformSlot> weak = slot;
    loader.addJob(std::function<void()>([this, weak, generation, take = std::move(take)] {
        const auto target = weak.lock();
        if (!target) return;
        {
            const std::lock_guard<std::mutex> lock(target->mutex);
            if (target->generation != generation) return;
        }
        auto pair = std::make_shared<ScratchWaveformPair>(formats, thumbnails);
        pair->directory = take.take.takeDirectory;
        // Revalidate on the worker: files may have changed since library discovery.
        const auto checked = loadScratchTake(pair->directory);
        const auto open = [this](juce::AudioThumbnail& thumbnail, const juce::File& file, bool readable) {
            if (!readable) return;
            const auto identity = file.getFullPathName() + "|" + juce::String(file.getSize()) + "|"
                + juce::String(file.getLastModificationTime().toMilliseconds());
            auto reader = std::unique_ptr<juce::AudioFormatReader>(formats.createReaderFor(file));
            if (reader) thumbnail.setReader(reader.release(), identity.hashCode64());
        };
        open(pair->raw, checked.take.rawFile, checked.rawReadable);
        open(pair->wet, checked.take.wetFile, checked.wetReadable);
        const std::lock_guard<std::mutex> lock(target->mutex);
        if (target->generation == generation) target->data = std::move(pair);
    }));
}

void ScratchWaveformSource::setTake(const ScratchTakeLoadResult& take)
{
    uint64_t generation;
    {
        const std::lock_guard<std::mutex> lock(slot->mutex);
        generation = ++slot->generation;
        slot->data.reset();
    }
    cache.request(slot, generation, take);
}
void ScratchWaveformSource::clear()
{
    const std::lock_guard<std::mutex> lock(slot->mutex);
    ++slot->generation;
    slot->data.reset();
}
std::shared_ptr<ScratchWaveformPair> ScratchWaveformSource::snapshot() const
{
    const std::lock_guard<std::mutex> lock(slot->mutex);
    return slot->data;
}

ScratchWaveformView::ScratchWaveformView(ScratchWaveformCache& cache, bool compactToUse)
    : source(cache), compact(compactToUse) { startTimerHz(10); }
void ScratchWaveformView::setTake(const ScratchTakeLoadResult& take)
{
    data.reset();
    requested = true;
    playhead = 0;
    source.setTake(take);
    repaint();
}
void ScratchWaveformView::clear()
{
    data.reset();
    source.clear();
    requested = false;
    repaint();
}
void ScratchWaveformView::setPlayheadSeconds(double seconds)
{
    playhead = juce::jmax(0.0, seconds);
    repaint();
}
int ScratchWaveformView::preferredHeight() const
{
    if (compact || !data) return 76;
    return juce::jmax(76, (juce::jmax(1, data->raw.getNumChannels())
                         + juce::jmax(1, data->wet.getNumChannels())) * 22 + 16);
}
void ScratchWaveformView::timerCallback()
{
    data = source.snapshot();
    if (data) repaint();
}
void ScratchWaveformView::paint(juce::Graphics& g)
{
    const auto text = colour("Text Colour", juce::Colours::white);
    const auto accent = colour("Accent Colour", juce::Colours::cyan);
    const auto rawColour = colour("Audio Connection", accent);
    g.setFont(FontManager::getInstance().getCaptionFont());
    auto area = getLocalBounds().reduced(6, 4);
    if (!data)
    {
        g.setColour(text.withAlpha(0.6f));
        g.drawFittedText(requested ? "Loading audio..." : "Select a take to see its RAW / WET audio",
                        area, juce::Justification::centred, 2);
        return;
    }
    const double duration = juce::jmax(data->raw.getTotalLength(), data->wet.getTotalLength());
    const int rawCount = compact ? 1 : juce::jmax(1, data->raw.getNumChannels());
    const int wetCount = compact ? 1 : juce::jmax(1, data->wet.getNumChannels());
    const int height = juce::jmax(1, (area.getHeight() - 12) / (rawCount + wetCount));
    const auto draw = [&](juce::AudioThumbnail& thumbnail, const char* name, int count, juce::Colour ink) {
        for (int channel = 0; channel < count; ++channel)
        {
            auto lane = area.removeFromTop(height);
            g.setColour(text.withAlpha(0.72f));
            g.drawFittedText(juce::String(name) + " " + juce::String(channel + 1), lane.removeFromLeft(48),
                            juce::Justification::centredLeft, 1);
            g.setColour(ink.withAlpha(0.8f));
            if (thumbnail.getNumChannels() <= channel || duration <= 0)
            {
                g.drawFittedText("Unavailable", lane, juce::Justification::centredLeft, 1);
                continue;
            }
            if (!thumbnail.isFullyLoaded())
            {
                g.drawFittedText("Loading...", lane, juce::Justification::centredLeft, 1);
                continue;
            }
            // Cached overview only: drawChannel can seek/read the WAV from paint at high zoom.
            for (int x = 0; x < lane.getWidth(); ++x)
            {
                float low = 0, high = 0;
                thumbnail.getApproximateMinMax(duration * x / lane.getWidth(),
                    duration * (x + 1) / lane.getWidth(), channel, low, high);
                const auto scale = static_cast<float>(lane.getHeight()) * 0.46f;
                const auto middle = static_cast<float>(lane.getCentreY());
                g.drawVerticalLine(lane.getX() + x, middle - high * scale, middle - low * scale + 1.0f);
            }
            if (onSeek && playhead <= duration)
            {
                g.setColour(text);
                const auto x = lane.getX() + static_cast<int>(playhead / duration * lane.getWidth());
                g.drawVerticalLine(x, static_cast<float>(lane.getY()), static_cast<float>(lane.getBottom()));
            }
        }
    };
    draw(data->raw, "RAW", rawCount, rawColour);
    draw(data->wet, "WET", wetCount, accent);
    g.setColour(text.withAlpha(0.5f));
    if (!compact) g.drawText(juce::String(duration, 2) + " s", area, juce::Justification::centredRight);
}
void ScratchWaveformView::mouseDown(const juce::MouseEvent& event)
{
    if (!onSeek || !data || getWidth() <= 60) return;
    const auto duration = juce::jmax(data->raw.getTotalLength(), data->wet.getTotalLength());
    onSeek(juce::jlimit(0.0, 1.0, (event.position.x - 54.0) / (getWidth() - 60.0)) * duration);
}
