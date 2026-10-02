#pragma once
#include <JuceHeader.h>

// One producer, one consumer. reset() requires BOTH endpoints quiescent.
class ScratchPcmQueue
{
public:
    static constexpr int channels = 16, storageFrames = 65537;
    ScratchPcmQueue();
    int write(const float* const*, int channelCount, int frames) noexcept;
    int read(float* const*, int channelCount, int frames) noexcept;
    int ready() const noexcept { return fifo.getNumReady(); }
    int free() const noexcept { return fifo.getFreeSpace(); }
    void reset() noexcept { fifo.reset(); }
private:
    juce::AudioBuffer<float> samples;
    juce::AbstractFifo fifo{storageFrames};
};
