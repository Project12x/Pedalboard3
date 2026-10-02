#include "ScratchPcmQueue.h"
ScratchPcmQueue::ScratchPcmQueue() : samples(channels, storageFrames) { samples.clear(); }
int ScratchPcmQueue::write(const float* const* input, int channelCount, int frames) noexcept
{
    int a, an, b, bn;
    fifo.prepareToWrite(frames, a, an, b, bn);
    for (int ch = 0; ch < channels; ++ch)
    {
        auto* dst = samples.getWritePointer(ch);
        if (ch < channelCount && input[ch])
        {
            juce::FloatVectorOperations::copy(dst + a, input[ch], an);
            juce::FloatVectorOperations::copy(dst + b, input[ch] + an, bn);
        }
        else { juce::FloatVectorOperations::clear(dst + a, an); juce::FloatVectorOperations::clear(dst + b, bn); }
    }
    fifo.finishedWrite(an + bn);
    return an + bn;
}
int ScratchPcmQueue::read(float* const* output, int channelCount, int frames) noexcept
{
    int a, an, b, bn;
    fifo.prepareToRead(frames, a, an, b, bn);
    for (int ch = 0; ch < channelCount; ++ch)
    {
        if (!output[ch]) continue;
        juce::FloatVectorOperations::clear(output[ch], frames);
        if (ch >= channels) continue;
        juce::FloatVectorOperations::copy(output[ch], samples.getReadPointer(ch, a), an);
        juce::FloatVectorOperations::copy(output[ch] + an, samples.getReadPointer(ch, b), bn);
    }
    fifo.finishedRead(an + bn);
    return an + bn;
}
