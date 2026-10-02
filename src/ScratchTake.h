#pragma once

#include <JuceHeader.h>
#include <cstdint>
#include <optional>

enum class ScratchTakeLoadState { Ready, Incomplete, Invalid, Unsupported };
enum class ScratchSnapshotState { None, Pending, Saved, Failed };

struct ScratchTakeContext
{
    juce::File rootDirectory;
    juce::String patchName;
    int patchIndex = 0;
    juce::String documentPath;
    juce::String deviceName;
    double sampleRate = 44100.0;
    int rawChannelCount = 0;
    int wetChannelCount = 0;
    double masterInputGainDb = 0.0;
    double masterOutputGainDb = 0.0;
    juce::Time startTime = juce::Time::getCurrentTime();
};

struct ScratchTake
{
    int schemaVersion = 2;
    juce::String takeId;
    juce::Time startTime;
    juce::File takeDirectory;
    juce::File rawFile;
    juce::File wetFile;
    juce::File metadataFile;
    juce::File rigFile;
    juce::String patchName;
    int patchIndex = 0;
    juce::String documentPath;
    juce::String deviceName;
    double sampleRate = 44100.0;
    int rawChannelCount = 0;
    int wetChannelCount = 0;
    double masterInputGainDb = 0.0;
    double masterOutputGainDb = 0.0;
    uint64_t durationSamples = 0;
    std::optional<uint64_t> rawDurationSamples;
    std::optional<uint64_t> wetDurationSamples;
    juce::String captureStatus = "recording";
    ScratchSnapshotState snapshotState = ScratchSnapshotState::None;
    juce::String snapshotMessage;
    bool complete = false;
    juce::String failureReason;

    static juce::String sanitisePathSegment(const juce::String& text);
    static ScratchTake createPending(const ScratchTakeContext& context);

    double durationSeconds() const noexcept;
    juce::String displayDateLabel() const;
    juce::String displayTimeLabel() const;
    bool canReveal() const;
    bool canPlayWetPreview() const;
    bool canReampRawCapture() const;
    juce::String toJsonString() const;
    bool writeMetadata() const;
};

struct ScratchTakeLoadResult
{
    ScratchTake take;
    ScratchTakeLoadState state = ScratchTakeLoadState::Invalid;
    juce::String message;
    bool rawReadable = false;
    bool wetReadable = false;
};

// Control/worker-only: bounded metadata parsing and WAV header validation.
ScratchTakeLoadResult loadScratchTake(const juce::File& takeDirectory);
