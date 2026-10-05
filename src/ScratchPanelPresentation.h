#pragma once

#include "ScratchRecorder.h"
#include "ScratchTakeLibrary.h"

#include <JuceHeader.h>

inline bool shouldHandleScratchSpace(bool panelFocused, bool textInputFocused) noexcept
{
    return panelFocused && !textInputFocused;
}
inline bool scratchCaptureBlocksAudition(ScratchRecorderState state) noexcept
{
    return state == ScratchRecorderState::Preparing || state == ScratchRecorderState::Recording || state == ScratchRecorderState::Saving;
}
inline juce::String scratchAuditionDisabledReason(ScratchRecorderState state)
{
    switch (state)
    {
    case ScratchRecorderState::Preparing: return "Preparing rig context - cancel REC before auditioning";
    case ScratchRecorderState::Recording: return "Stop capture before auditioning";
    case ScratchRecorderState::Saving: return "Wait for this take to finish saving";
    default: return {};
    }
}
inline juce::String scratchSnapshotLabel(const ScratchTake& take)
{
    switch (take.snapshotState)
    {
    case ScratchSnapshotState::Saved: return "Rig context saved (assets referenced, not copied)";
    case ScratchSnapshotState::Failed: return "Rig context unavailable: " + take.snapshotMessage;
    case ScratchSnapshotState::Pending: return "Rig context pending";
    default: return "No saved rig context (legacy take)";
    }
}

inline int findScratchTakeRow(const ScratchLibrarySnapshot& snapshot, const juce::File& directory)
{
    for (size_t i = 0; i < snapshot.entries.size(); ++i)
        if (snapshot.entries[i].take.takeDirectory == directory) return static_cast<int>(i);
    return -1;
}

inline juce::File reconcileScratchSelection(const ScratchLibrarySnapshot& snapshot, const juce::File& selected)
{
    if (findScratchTakeRow(snapshot, selected) >= 0) return selected;
    // A partial batch may not contain the selection yet. Don't jump to another take.
    if (snapshot.scanning && selected != juce::File()
        && selected.getParentDirectory().getParentDirectory() == snapshot.root) return selected;
    return snapshot.entries.empty() ? juce::File() : snapshot.entries.front().take.takeDirectory;
}

struct ScratchTakeActions { bool play, reamp, reveal; };
inline ScratchTakeActions scratchTakeActions(const ScratchTakeLoadResult& entry)
{
    return {entry.state == ScratchTakeLoadState::Ready && entry.wetReadable,
            entry.state == ScratchTakeLoadState::Ready && entry.rawReadable,
            entry.take.takeDirectory != juce::File()};
}

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

inline const ScratchTakeContext* getDisplayContext(const ScratchRecorderStatus& status)
{
    if (status.armedContext.has_value())
        return &*status.armedContext;

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
    {
        const auto* context = getDisplayContext(status);
        if (context != nullptr)
        {
            return "RAW " + juce::String(context->rawChannelCount) + "ch + WET "
                   + juce::String(context->wetChannelCount) + "ch";
        }

        return "RAW + WET armed";
    }

    return "RAW " + juce::String(take->rawChannelCount) + "ch + WET "
           + juce::String(take->wetChannelCount) + "ch";
}

inline juce::String formatStatusLine(const ScratchRecorderStatus& status)
{
    switch (status.state)
    {
    case ScratchRecorderState::Preparing:
        return "Preparing rig context - REC cancels";
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
    case ScratchRecorderState::Preparing:
        return "Preparing rig context";
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

    return "Ready  |  " + formatCapturePairLabel(status);
}
} // namespace ScratchPanelPresentation
