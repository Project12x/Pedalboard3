#pragma once

#include <JuceHeader.h>
#include <cstdint>

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
    juce::String takeId;
    juce::Time startTime;
    juce::File takeDirectory;
    juce::File rawFile;
    juce::File wetFile;
    juce::File metadataFile;
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
    bool complete = false;
    juce::String failureReason;

    static juce::String sanitisePathSegment(const juce::String& text);
    static ScratchTake createPending(const ScratchTakeContext& context);

    double durationSeconds() const noexcept;
    juce::String toJsonString() const;
    bool writeMetadata() const;
};
