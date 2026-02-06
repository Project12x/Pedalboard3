/*
  ==============================================================================

    NAMConvolver.h
    IR Convolution wrapper for NAM processor
    Isolated to avoid namespace conflicts between AudioDSPTools and juce::dsp

  ==============================================================================
*/

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>
#include <memory>

// Forward declaration - implementation in NAMConvolver.cpp
struct ConvolverImpl;

/**
    Wrapper for juce::dsp::Convolution that isolates it from AudioDSPTools
    namespace conflicts. The dsp namespace from AudioDSPTools conflicts with
    juce::dsp, so we keep convolution in a separate compilation unit.
*/
class NAMConvolver
{
public:
    NAMConvolver();
    ~NAMConvolver();

    void prepare(double sampleRate, int blockSize);
    void loadIR(const juce::File& file);
    void process(juce::AudioBuffer<float>& buffer);
    void reset();

private:
    std::unique_ptr<ConvolverImpl> impl;
};
