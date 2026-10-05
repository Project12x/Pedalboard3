#pragma once
#include "ScratchPlayback.h"
#include "ScratchRecorder.h"

struct ScratchRenderHooks
{
    void* context = nullptr;
    void (*processLivePath)(void*, const float* const*, int, float* const*, int, int) = nullptr;
    void (*protectPreview)(void*, float* const*, int, int) = nullptr;
};
class ScratchAudioRouter
{
public:
    void prepare(const ScratchPlaybackDevice&);
    void process(ScratchPlayback&, ScratchRecorder*, const float* const*, int,
                 float* const*, int, int, ScratchRenderHooks) noexcept;
private:
    juce::AudioBuffer<float> pcm, silence, discard;
    int graphInputs = 0;
    float rampStep = 1, volumeStep = 1, previewVolume = 0.25118864f;
    float auditionGain = 0, liveGain = 1;
};
