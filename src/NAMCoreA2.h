/*
  ==============================================================================

    NAMCoreA2.h
    C++17-compatible facade for the NeuralAmpModelerCore A2 runtime adapter.

  ==============================================================================
*/

#pragma once

#include <memory>
#include <string>

class NAMCoreA2
{
public:
    NAMCoreA2();
    ~NAMCoreA2();

    NAMCoreA2(const NAMCoreA2&) = delete;
    NAMCoreA2& operator=(const NAMCoreA2&) = delete;

    bool loadModel(const std::string& modelPath, double sampleRate, int blockSize, bool enforceSampleRate);
    void clearModel();

    bool isModelLoaded() const;
    bool hasLoudness() const;
    double getLoudness() const;

    void prepare(double sampleRate, int blockSize);
    void process(float* input, float* output, int numSamples);

private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};
