/*
  ==============================================================================

    NAMCoreA2.cpp
    Adapter for NeuralAmpModelerCore v0.5.x Architecture 2 models.

    This file intentionally remaps upstream namespace nam -> pedalboard3_nam_a2
    so the A2 core can link beside the legacy vendored NAM 0.1.x runtime.

  ==============================================================================
*/

#include "NAMCoreA2.h"

#define nam pedalboard3_nam_a2
#include "../external/NeuralAmpModelerCoreA2/NAM/get_dsp.h"
#include "../external/NeuralAmpModelerCoreA2/NAM/slimmable.h"
#undef nam

#include <algorithm>
#include <cmath>
#include <exception>
#include <filesystem>
#include <memory>

namespace nam_a2 = pedalboard3_nam_a2;

namespace
{
constexpr double kAssumedNamSampleRate = 48000.0;
constexpr double kSampleRateTolerance = 1.0;

double getEffectiveModelSampleRate(const nam_a2::DSP& model)
{
    const double reported = model.GetExpectedSampleRate();
    return reported <= 0.0 ? kAssumedNamSampleRate : reported;
}

bool sampleRatesMatch(double hostSampleRate, double modelSampleRate)
{
    return std::abs(hostSampleRate - modelSampleRate) <= kSampleRateTolerance;
}
} // namespace

struct NAMCoreA2::Impl
{
    std::unique_ptr<nam_a2::DSP> model;
    double sampleRate = 44100.0;
    int blockSize = 512;
    bool modelLoaded = false;
};

NAMCoreA2::NAMCoreA2()
    : impl(std::make_unique<Impl>())
{
}

NAMCoreA2::~NAMCoreA2() = default;

bool NAMCoreA2::loadModel(const std::string& modelPath, double sampleRate, int blockSize, bool enforceSampleRate)
{
    try
    {
        auto path = std::filesystem::u8path(modelPath);
        auto dspModel = nam_a2::get_dsp(path);

        if (!dspModel)
            return false;

        if (dspModel->NumInputChannels() != 1 || dspModel->NumOutputChannels() != 1)
            return false;

        const double modelSampleRate = getEffectiveModelSampleRate(*dspModel);
        if (enforceSampleRate && !sampleRatesMatch(sampleRate, modelSampleRate))
            return false;

        dspModel->Reset(sampleRate, blockSize);

        impl->sampleRate = sampleRate;
        impl->blockSize = blockSize;
        impl->model = std::move(dspModel);
        impl->modelLoaded = true;
        return true;
    }
    catch (const std::exception&)
    {
        return false;
    }
}

void NAMCoreA2::clearModel()
{
    impl->model = nullptr;
    impl->modelLoaded = false;
}

bool NAMCoreA2::isModelLoaded() const
{
    return impl->modelLoaded;
}

bool NAMCoreA2::isSlimmableModel() const
{
    return impl->model && dynamic_cast<nam_a2::SlimmableModel*>(impl->model.get()) != nullptr;
}

bool NAMCoreA2::hasLoudness() const
{
    return impl->model && impl->model->HasLoudness();
}

double NAMCoreA2::getLoudness() const
{
    if (impl->model && impl->model->HasLoudness())
        return impl->model->GetLoudness();

    return 0.0;
}

bool NAMCoreA2::setSlimmableSize(double size)
{
    if (!impl->model)
        return false;

    auto* slimmable = dynamic_cast<nam_a2::SlimmableModel*>(impl->model.get());
    if (!slimmable)
        return false;

    try
    {
        slimmable->SetSlimmableSize(std::clamp(size, 0.0, 1.0));
        return true;
    }
    catch (const std::exception&)
    {
        return false;
    }
}

void NAMCoreA2::prepare(double sampleRate, int blockSize)
{
    impl->sampleRate = sampleRate;
    impl->blockSize = blockSize;

    if (impl->model)
    {
        const double modelSampleRate = getEffectiveModelSampleRate(*impl->model);
        if (!sampleRatesMatch(sampleRate, modelSampleRate))
        {
            clearModel();
            return;
        }

        impl->model->Reset(sampleRate, blockSize);
    }
}

void NAMCoreA2::process(float* input, float* output, int numSamples)
{
    if (!impl->model)
    {
        std::copy(input, input + numSamples, output);
        return;
    }

    float* inputs[1] = {input};
    float* outputs[1] = {output};
    impl->model->process(inputs, outputs, numSamples);
}
