#pragma once

#include "ScratchRecorder.h"

#include <JuceHeader.h>

namespace ScratchPanelPresentation
{
inline const ScratchTake* getDisplayTake(const ScratchRecorderStatus& status)
{
    if (status.activeTake.has_value())
        return &*status.activeTake;

    if (status.lastTake.has_value())
        return &*status.lastTake;

    return nullptr;
}

inline juce::String formatDurationLabel(uint64_t samples, double sampleRate)
{
    int seconds = 0;
    if (sampleRate > 0.0)
        seconds = static_cast<int>(static_cast<double>(samples) / sampleRate);

    return juce::String::formatted("%02d:%02d", seconds / 60, seconds % 60);
}

inline juce::String formatElapsedLabel(const ScratchRecorderStatus& status)
{
    const auto* take = getDisplayTake(status);
    const double sampleRate = take != nullptr ? take->sampleRate : 0.0;
    return formatDurationLabel(status.elapsedSamples, sampleRate);
}
} // namespace ScratchPanelPresentation
