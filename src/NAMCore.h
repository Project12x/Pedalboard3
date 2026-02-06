/*
  ==============================================================================

    NAMCore.h
    NAM DSP wrapper - isolates AudioDSPTools from JUCE to avoid namespace conflicts

  ==============================================================================
*/

#pragma once

#include <memory>
#include <string>

/**
    Metadata extracted from a NAM model file without loading it for DSP.
*/
struct NAMModelInfo
{
    std::string filePath;
    std::string name;           // Filename without extension
    std::string architecture;   // Linear, ConvNet, LSTM, WaveNet, CatLSTM, CatWaveNet
    double expectedSampleRate;  // -1 if unknown
    bool hasLoudness;
    double loudness;            // dB
    std::string version;        // Model config version
    std::string metadata;       // Raw JSON metadata as string (author, description, etc.)

    NAMModelInfo()
        : expectedSampleRate(-1.0)
        , hasLoudness(false)
        , loudness(0.0)
    {}
};

/**
    Opaque wrapper for NAM model processing.
    This class isolates the AudioDSPTools/NAM code from JUCE headers
    to avoid the dsp namespace conflict.
*/
class NAMCore
{
public:
    NAMCore();
    ~NAMCore();

    // Model management
    bool loadModel(const std::string& modelPath);
    void clearModel();
    bool isModelLoaded() const;
    bool hasLoudness() const;
    double getLoudness() const;

    // Static metadata extraction - parses .nam file without loading for DSP
    static bool getModelInfo(const std::string& modelPath, NAMModelInfo& info);

    // Processing
    void prepare(double sampleRate, int blockSize);
    void process(float* input, float* output, int numSamples);
    void finalize(int numSamples);

    // Tone stack
    void setToneStackEnabled(bool enabled);
    void setToneStackParams(float bass, float mid, float treble);
    void processToneStack(float* data, int numSamples);

    // Noise gate
    void setNoiseGateParams(double threshold, double time, double ratio,
                           double openTime, double holdTime, double closeTime);
    void processNoiseGateTrigger(float* input, int numSamples);
    void processNoiseGateGain(float* data, int numSamples);

private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};
