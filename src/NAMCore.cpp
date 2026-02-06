/*
  ==============================================================================

    NAMCore.cpp
    NAM DSP wrapper implementation

    This file is intentionally kept separate from JUCE headers to avoid
    namespace conflicts between AudioDSPTools' dsp:: and juce::dsp::

  ==============================================================================
*/

#include "NAMCore.h"

// Include AudioDSPTools/NAM headers - NO JUCE headers in this file!
#include "../external/AudioDSPTools/dsp/NoiseGate.h"
#include "../external/NeuralAmpModelerCore/wrapper/ResamplingNAM.h"
#include "../external/NeuralAmpModelerCore/wrapper/ToneStack.h"
#include "../external/NeuralAmpModelerCore/NAM/dsp.h"

#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>

//==============================================================================
struct NAMCore::Impl
{
    std::unique_ptr<ResamplingNAM> model;
    std::unique_ptr<ResamplingNAM> stagedModel;
    std::unique_ptr<dsp::tone_stack::BasicNamToneStack> toneStack;
    std::unique_ptr<dsp::noise_gate::Trigger> noiseGateTrigger;
    std::unique_ptr<dsp::noise_gate::Gain> noiseGateGain;

    double sampleRate = 44100.0;
    int blockSize = 512;
    bool modelLoaded = false;
    bool toneStackEnabled = true;

    Impl()
    {
        toneStack = std::make_unique<dsp::tone_stack::BasicNamToneStack>();
        noiseGateTrigger = std::make_unique<dsp::noise_gate::Trigger>();
        noiseGateGain = std::make_unique<dsp::noise_gate::Gain>();
        noiseGateTrigger->AddListener(noiseGateGain.get());

        // Enable fast tanh for better performance
        nam::activations::Activation::enable_fast_tanh();
    }
};

//==============================================================================
NAMCore::NAMCore()
    : impl(std::make_unique<Impl>())
{
}

NAMCore::~NAMCore() = default;

bool NAMCore::loadModel(const std::string& modelPath)
{
    try
    {
        auto path = std::filesystem::u8path(modelPath);
        std::unique_ptr<nam::DSP> dspModel = nam::get_dsp(path);

        if (!dspModel)
        {
            return false;
        }

        auto resamplingModel = std::make_unique<ResamplingNAM>(std::move(dspModel), impl->sampleRate);
        resamplingModel->Reset(impl->sampleRate, impl->blockSize);

        impl->stagedModel = std::move(resamplingModel);
        return true;
    }
    catch (const std::exception&)
    {
        impl->stagedModel = nullptr;
        return false;
    }
}

void NAMCore::clearModel()
{
    impl->model = nullptr;
    impl->stagedModel = nullptr;
    impl->modelLoaded = false;
}

bool NAMCore::isModelLoaded() const
{
    return impl->modelLoaded;
}

bool NAMCore::hasLoudness() const
{
    return impl->model && impl->model->HasLoudness();
}

double NAMCore::getLoudness() const
{
    if (impl->model && impl->model->HasLoudness())
    {
        return impl->model->GetLoudness();
    }
    return 0.0;
}

void NAMCore::prepare(double sampleRate, int blockSize)
{
    impl->sampleRate = sampleRate;
    impl->blockSize = blockSize;

    impl->toneStack->Reset(sampleRate, blockSize);
    impl->noiseGateTrigger->SetSampleRate(sampleRate);

    if (impl->model)
    {
        impl->model->Reset(sampleRate, blockSize);
    }
}

void NAMCore::process(float* input, float* output, int numSamples)
{
    // Apply staged model if available
    if (impl->stagedModel)
    {
        impl->model = std::move(impl->stagedModel);
        impl->stagedModel = nullptr;
        impl->modelLoaded = true;
    }

    if (impl->model)
    {
        impl->model->process(input, output, numSamples);
    }
    else
    {
        // Pass through if no model
        std::copy(input, input + numSamples, output);
    }
}

void NAMCore::finalize(int numSamples)
{
    if (impl->model)
    {
        impl->model->finalize_(numSamples);
    }
}

void NAMCore::setToneStackEnabled(bool enabled)
{
    impl->toneStackEnabled = enabled;
}

void NAMCore::setToneStackParams(float bass, float mid, float treble)
{
    if (impl->toneStack)
    {
        impl->toneStack->SetParam("bass", bass);
        impl->toneStack->SetParam("middle", mid);
        impl->toneStack->SetParam("treble", treble);
    }
}

void NAMCore::processToneStack(float* data, int numSamples)
{
    if (impl->toneStackEnabled && impl->toneStack)
    {
        DSP_SAMPLE* input[1] = {data};
        DSP_SAMPLE** output = impl->toneStack->Process(input, 1, numSamples);
        if (output[0] != data)
        {
            std::copy(output[0], output[0] + numSamples, data);
        }
    }
}

void NAMCore::setNoiseGateParams(double threshold, double time, double ratio,
                                 double openTime, double holdTime, double closeTime)
{
    if (impl->noiseGateTrigger)
    {
        const dsp::noise_gate::TriggerParams params(time, threshold, ratio,
                                                    openTime, holdTime, closeTime);
        impl->noiseGateTrigger->SetParams(params);
    }
}

void NAMCore::processNoiseGateTrigger(float* input, int numSamples)
{
    if (impl->noiseGateTrigger)
    {
        DSP_SAMPLE* inputPtr[1] = {input};
        impl->noiseGateTrigger->Process(inputPtr, 1, numSamples);
    }
}

void NAMCore::processNoiseGateGain(float* data, int numSamples)
{
    if (impl->noiseGateGain)
    {
        DSP_SAMPLE* inputPtr[1] = {data};
        DSP_SAMPLE** output = impl->noiseGateGain->Process(inputPtr, 1, numSamples);
        if (output[0] != data)
        {
            std::copy(output[0], output[0] + numSamples, data);
        }
    }
}

//==============================================================================
bool NAMCore::getModelInfo(const std::string& modelPath, NAMModelInfo& info)
{
    try
    {
        auto path = std::filesystem::u8path(modelPath);
        if (!std::filesystem::exists(path))
        {
            return false;
        }

        std::ifstream file(path);
        if (!file.is_open())
        {
            return false;
        }

        nlohmann::json j;
        file >> j;

        info.filePath = modelPath;
        info.name = path.stem().string();

        // Get version
        if (j.contains("version"))
        {
            info.version = j["version"].get<std::string>();
        }
        else
        {
            info.version = "unknown";
        }

        // Get architecture
        if (j.contains("architecture"))
        {
            info.architecture = j["architecture"].get<std::string>();
        }
        else
        {
            info.architecture = "unknown";
        }

        // Get expected sample rate from config
        if (j.contains("config") && j["config"].contains("sample_rate"))
        {
            info.expectedSampleRate = j["config"]["sample_rate"].get<double>();
        }
        else
        {
            info.expectedSampleRate = -1.0;
        }

        // Get loudness and other metadata
        info.hasLoudness = false;
        info.loudness = 0.0;
        info.metadata = "";

        if (j.contains("metadata") && !j["metadata"].is_null())
        {
            const auto& meta = j["metadata"];

            // Extract loudness if present
            if (meta.contains("loudness"))
            {
                info.loudness = meta["loudness"].get<double>();
                info.hasLoudness = true;
            }

            // Store full metadata as pretty-printed JSON string
            info.metadata = meta.dump(2);
        }

        return true;
    }
    catch (const std::exception&)
    {
        return false;
    }
}
