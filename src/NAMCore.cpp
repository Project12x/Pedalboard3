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
#include "NAMCoreA2.h"
#include "../external/AudioDSPTools/dsp/NoiseGate.h"
#include "../external/NeuralAmpModelerCore/wrapper/ResamplingNAM.h"
#include "../external/NeuralAmpModelerCore/wrapper/ToneStack.h"
#include "../external/NeuralAmpModelerCore/NAM/dsp.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <sstream>

namespace
{
struct NamFileVersion
{
    int major = 0;
    int minor = 0;
    int patch = 0;
    bool valid = false;
};

int readArchitectureVersion(const nlohmann::json& j)
{
    if (!j.is_object())
        return 0;

    const auto architectureVersion = j.find("architecture_version");
    if (architectureVersion == j.end() || architectureVersion->is_null())
        return 0;

    if (architectureVersion->is_number_integer())
        return architectureVersion->get<int>();

    if (architectureVersion->is_number())
        return static_cast<int>(architectureVersion->get<double>());

    if (architectureVersion->is_string())
    {
        try
        {
            return std::stoi(architectureVersion->get<std::string>());
        }
        catch (const std::exception&)
        {
            return 0;
        }
    }

    return 0;
}

NamFileVersion readNamFileVersion(const nlohmann::json& j)
{
    if (!j.is_object())
        return {};

    const auto version = j.find("version");
    if (version == j.end() || !version->is_string())
        return {};

    NamFileVersion parsed;
    char dot1 = '\0';
    char dot2 = '\0';
    std::istringstream stream(version->get<std::string>());
    if (!(stream >> parsed.major >> dot1 >> parsed.minor >> dot2 >> parsed.patch))
        return {};

    if (dot1 != '.' || dot2 != '.' || parsed.major < 0 || parsed.minor < 0 || parsed.patch < 0)
        return {};

    parsed.valid = true;
    return parsed;
}

bool isNamVersionAtLeast(const NamFileVersion& version, int major, int minor, int patch)
{
    if (!version.valid)
        return false;

    if (version.major != major)
        return version.major > major;

    if (version.minor != minor)
        return version.minor > minor;

    return version.patch >= patch;
}

bool hasSlimmableLayerConfig(const nlohmann::json& config)
{
    const nlohmann::json* modelConfig = &config;
    const auto wrappedModel = config.find("model");
    if (wrappedModel != config.end() && wrappedModel->is_object())
        modelConfig = &(*wrappedModel);

    const auto layers = modelConfig->find("layers");
    if (layers == modelConfig->end() || !layers->is_array())
        return false;

    for (const auto& layer : *layers)
    {
        if (!layer.is_object())
            continue;

        const auto slimmable = layer.find("slimmable");
        if (slimmable != layer.end() && !slimmable->is_null())
            return true;
    }

    return false;
}

bool usesA2OnlyModelShape(const nlohmann::json& j)
{
    if (!j.is_object())
        return false;

    const auto architecture = j.find("architecture");
    if (architecture == j.end() || !architecture->is_string())
        return false;

    const auto architectureName = architecture->get<std::string>();
    if (architectureName == "SlimmableContainer")
        return true;

    const auto config = j.find("config");
    return architectureName == "WaveNet" && config != j.end() && config->is_object() && hasSlimmableLayerConfig(*config);
}

bool shouldTryA2BeforeLegacy(const nlohmann::json& j)
{
    if (readArchitectureVersion(j) == 2 || usesA2OnlyModelShape(j))
        return true;

    return isNamVersionAtLeast(readNamFileVersion(j), 0, 6, 0);
}

bool shouldTryA2AfterLegacyFailure(const nlohmann::json& j)
{
    return shouldTryA2BeforeLegacy(j) || isNamVersionAtLeast(readNamFileVersion(j), 0, 5, 0);
}

bool readNamModelJson(const std::filesystem::path& path, nlohmann::json& j)
{
    std::ifstream file(path);
    if (!file.is_open())
        return false;

    try
    {
        file >> j;
        return true;
    }
    catch (const std::exception&)
    {
        return false;
    }
}
} // namespace

//==============================================================================
struct NAMCore::Impl
{
    std::unique_ptr<ResamplingNAM> model;
    std::unique_ptr<NAMCoreA2> a2Model;
    std::unique_ptr<dsp::tone_stack::BasicNamToneStack> toneStack;
    std::unique_ptr<dsp::noise_gate::Trigger> noiseGateTrigger;
    std::unique_ptr<dsp::noise_gate::Gain> noiseGateGain;

    double sampleRate = 44100.0;
    int blockSize = 512;
    bool prepared = false;
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

        nlohmann::json modelJson;
        const bool hasModelJson = readNamModelJson(path, modelJson);
        const bool tryA2First = hasModelJson && shouldTryA2BeforeLegacy(modelJson);
        const bool allowA2Fallback = hasModelJson && shouldTryA2AfterLegacyFailure(modelJson);

        const auto loadA2Model = [this, &modelPath]()
        {
            auto a2Model = std::make_unique<NAMCoreA2>();
            if (!a2Model->loadModel(modelPath, impl->sampleRate, impl->blockSize, impl->prepared))
                return false;

            impl->model = nullptr;
            impl->a2Model = std::move(a2Model);
            impl->modelLoaded = true;
            return true;
        };

        const auto loadLegacyModel = [this, &path]()
        {
            std::unique_ptr<nam::DSP> dspModel = nam::get_dsp(path);
            if (!dspModel)
                return false;

            auto resamplingModel = std::make_unique<ResamplingNAM>(std::move(dspModel), impl->sampleRate);
            resamplingModel->Reset(impl->sampleRate, impl->blockSize);

            impl->model = std::move(resamplingModel);
            impl->a2Model = nullptr;
            impl->modelLoaded = true;
            return true;
        };

        if (tryA2First)
            return loadA2Model();

        try
        {
            if (loadLegacyModel())
                return true;
        }
        catch (const std::exception&)
        {
        }

        if (allowA2Fallback)
            return loadA2Model();

        return false;
    }
    catch (const std::exception&)
    {
        return false;
    }
}

void NAMCore::clearModel()
{
    impl->model = nullptr;
    impl->a2Model = nullptr;
    impl->modelLoaded = false;
}

bool NAMCore::isModelLoaded() const
{
    return impl->modelLoaded;
}

bool NAMCore::isSlimmableModel() const
{
    return impl->a2Model && impl->a2Model->isSlimmableModel();
}

bool NAMCore::hasLoudness() const
{
    if (impl->a2Model)
    {
        return impl->a2Model->hasLoudness();
    }

    return impl->model && impl->model->HasLoudness();
}

double NAMCore::getLoudness() const
{
    if (impl->a2Model)
    {
        return impl->a2Model->getLoudness();
    }

    if (impl->model && impl->model->HasLoudness())
    {
        return impl->model->GetLoudness();
    }
    return 0.0;
}

bool NAMCore::setSlimmableSize(float size)
{
    if (!impl->a2Model)
        return false;

    return impl->a2Model->setSlimmableSize(size);
}

void NAMCore::prepare(double sampleRate, int blockSize)
{
    impl->sampleRate = sampleRate;
    impl->blockSize = blockSize;
    impl->prepared = true;

    impl->toneStack->Reset(sampleRate, blockSize);
    impl->noiseGateTrigger->SetSampleRate(sampleRate);

    if (impl->model)
    {
        impl->model->Reset(sampleRate, blockSize);
    }

    if (impl->a2Model)
    {
        impl->a2Model->prepare(sampleRate, blockSize);
        if (!impl->a2Model->isModelLoaded())
            impl->modelLoaded = false;
    }
}

void NAMCore::process(float* input, float* output, int numSamples)
{
    if (impl->a2Model)
    {
        impl->a2Model->process(input, output, numSamples);
        return;
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

        info.architectureVersion = readArchitectureVersion(j);

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
