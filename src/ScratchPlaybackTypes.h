#pragma once
#include "ScratchTake.h"
#include <array>
#include <functional>

enum class ScratchPlaybackMode { OriginalWet, CurrentRigRaw };
enum class ScratchPlaybackState { Idle, Loading, Playing, Paused, Stopped, Error };
class ScratchPcmDecoder
{
public:
    virtual ~ScratchPcmDecoder() = default;
    virtual juce::Result open(const juce::File&, double deviceRate) = 0;
    virtual int channels() const = 0;
    virtual int64_t lengthFrames() const = 0;
    virtual void seek(int64_t destinationFrame) = 0;
    virtual int read(float* const*, int frames) = 0;
};
using ScratchDecoderFactory = std::function<std::unique_ptr<ScratchPcmDecoder>()>;
std::unique_ptr<ScratchPcmDecoder> makeScratchPcmDecoder();
struct ScratchChannelMap
{
    ScratchChannelMap() { sourceForDestination.fill(-1); }
    std::array<int, 16> sourceForDestination;
    int destinationChannels = 0;
    bool explicitMapping = false;
};
struct ScratchPlaybackDevice
{
    double sampleRate = 0;
    int maxBlockSize = 0, graphInputChannels = 0, outputChannels = 0;
};
struct ScratchPlaybackStatus
{
    ScratchPlaybackState state = ScratchPlaybackState::Idle;
    ScratchPlaybackMode mode = ScratchPlaybackMode::OriginalWet;
    juce::File takeDirectory;
    double positionSeconds = 0, durationSeconds = 0, volumeDb = -12;
    bool looping = false, buffering = false, replacingLive = false;
    juce::String message;
};
struct ScratchPlaybackBlock
{
    ScratchPlaybackMode mode = ScratchPlaybackMode::OriginalWet;
    uint64_t generation = 0;
    int validFrames = 0;
    float previewGain = 0.25118864f;
    bool replacingLive = false;
};
