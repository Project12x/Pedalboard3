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

inline juce::String formatCapturePairLabel(const ScratchRecorderStatus& status)
{
    const auto* take = getDisplayTake(status);
    if (take == nullptr)
        return "RAW + WET armed";

    return "RAW " + juce::String(take->rawChannelCount) + "ch + WET "
           + juce::String(take->wetChannelCount) + "ch";
}

inline juce::String formatStatusLine(const ScratchRecorderStatus& status)
{
    switch (status.state)
    {
    case ScratchRecorderState::Recording:
        return "Recording " + formatElapsedLabel(status) + "  |  " + formatCapturePairLabel(status);
    case ScratchRecorderState::Saving:
        return "Saving take  |  " + formatCapturePairLabel(status);
    case ScratchRecorderState::Saved:
        return "Saved " + formatElapsedLabel(status) + "  |  " + formatCapturePairLabel(status);
    case ScratchRecorderState::Failed:
        return status.message.isNotEmpty() ? status.message : "Scratch capture failed";
    case ScratchRecorderState::Ready:
        break;
    }

    return "Ready  |  " + formatCapturePairLabel(status);
}

inline juce::String formatFooterStatusLine(const ScratchRecorderStatus& status)
{
    switch (status.state)
    {
    case ScratchRecorderState::Recording:
        return "REC " + formatElapsedLabel(status) + "  |  " + formatCapturePairLabel(status);
    case ScratchRecorderState::Saving:
        return "Saving RAW + WET";
    case ScratchRecorderState::Saved:
        return "Saved " + formatElapsedLabel(status);
    case ScratchRecorderState::Failed:
        return status.message.isNotEmpty() ? status.message : "Scratch failed";
    case ScratchRecorderState::Ready:
        break;
    }

    return "Ready  |  RAW + WET";
}
} // namespace ScratchPanelPresentation
