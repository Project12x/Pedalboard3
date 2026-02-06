/*
  ==============================================================================

    NAMConvolver.cpp
    IR Convolution wrapper for NAM processor
    This file is intentionally kept separate from NAMProcessor.cpp to avoid
    namespace conflicts between AudioDSPTools' dsp:: and juce::dsp::

  ==============================================================================
*/

#include "NAMConvolver.h"
#include <juce_dsp/juce_dsp.h>

//==============================================================================
struct ConvolverImpl
{
    juce::dsp::Convolution convolution;
    juce::dsp::ProcessSpec spec;
};

//==============================================================================
NAMConvolver::NAMConvolver()
    : impl(std::make_unique<ConvolverImpl>())
{
}

NAMConvolver::~NAMConvolver() = default;

void NAMConvolver::prepare(double sampleRate, int blockSize)
{
    impl->spec.sampleRate = sampleRate;
    impl->spec.maximumBlockSize = static_cast<juce::uint32>(blockSize);
    impl->spec.numChannels = 2;
    impl->convolution.prepare(impl->spec);
}

void NAMConvolver::loadIR(const juce::File& file)
{
    impl->convolution.loadImpulseResponse(file, juce::dsp::Convolution::Stereo::yes,
                                          juce::dsp::Convolution::Trim::yes, 0);
}

void NAMConvolver::process(juce::AudioBuffer<float>& buffer)
{
    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);
    impl->convolution.process(context);
}

void NAMConvolver::reset()
{
    impl->convolution.reset();
}
