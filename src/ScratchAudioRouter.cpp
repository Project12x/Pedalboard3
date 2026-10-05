#include "ScratchAudioRouter.h"
#include <algorithm>

void ScratchAudioRouter::prepare(const ScratchPlaybackDevice& device)
{
    pcm.setSize(16, 8192); silence.setSize(16, 8192); discard.setSize(16, 8192);
    pcm.clear(); silence.clear(); discard.clear();
    graphInputs = std::clamp(device.graphInputChannels, 0, 16);
    rampStep = 1.0f / static_cast<float>(std::max(1.0, device.sampleRate * 0.005));
    volumeStep = 1.0f / static_cast<float>(std::max(1.0, device.sampleRate * 0.020));
    auditionGain = 0; liveGain = 1;
}

void ScratchAudioRouter::process(ScratchPlayback& playback, ScratchRecorder* recorder,
                                 const float* const* input, int inputs, float* const* output,
                                 int outputs, int samples, ScratchRenderHooks hooks) noexcept
{
    juce::ScopedNoDenormals noDenormals;
    if (samples <= 0 || samples > pcm.getNumSamples() || inputs < 0 || inputs > 16 || outputs < 0 || outputs > 16 || !hooks.processLivePath)
    {
        for (int ch = 0; ch < outputs; ++ch) if (output[ch]) juce::FloatVectorOperations::clear(output[ch], samples);
        return;
    }
    const auto block = playback.readBlock(pcm.getArrayOfWritePointers(), 16, samples);
    if (!block.replacingLive)
    {
        const bool lease = recorder && recorder->beginDeviceBlock(input, inputs, samples);
        hooks.processLivePath(hooks.context, input, inputs, output, outputs, samples);
        for (int i = 0; i < samples; ++i)
        {
            liveGain = std::min(1.0f, liveGain + rampStep);
            for (int ch = 0; ch < outputs; ++ch) if (output[ch]) output[ch][i] *= liveGain;
        }
        if (lease) recorder->endDeviceBlock(output, outputs, samples);
        auditionGain = 0;
        return;
    }
    liveGain = 0;
    if (block.mode == ScratchPlaybackMode::OriginalWet)
    {
        // Consume graph MIDI and advance plugin state, but none of its output is audible.
        hooks.processLivePath(hooks.context, silence.getArrayOfReadPointers(), graphInputs,
                              discard.getArrayOfWritePointers(), outputs, samples);
        for (int ch = 0; ch < outputs; ++ch)
            if (output[ch]) juce::FloatVectorOperations::copy(output[ch], pcm.getReadPointer(ch), samples);
    }
    else
        hooks.processLivePath(hooks.context, pcm.getArrayOfReadPointers(), graphInputs, output, outputs, samples);

    for (int i = 0; i < samples; ++i)
    {
        // Gate post-rig generators/tails during pause/loading/underflow as well as PCM.
        auditionGain = std::clamp(auditionGain + (i < block.validFrames ? rampStep : -rampStep), 0.0f, 1.0f);
        previewVolume += std::clamp(block.previewGain - previewVolume, -volumeStep, volumeStep);
        for (int ch = 0; ch < outputs; ++ch) if (output[ch]) output[ch][i] *= auditionGain * previewVolume;
    }
    if (hooks.protectPreview) hooks.protectPreview(hooks.context, output, outputs, samples);
}
