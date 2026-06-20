/*
  ==============================================================================

    NAMProcessor.cpp
    Neural Amp Modeler processor implementation

  ==============================================================================
*/

#include "NAMProcessor.h"

#include "NAMControl.h"
#include "NAMConvolver.h"
#include "NAMCore.h"
#include "SubGraphProcessor.h"

#include <cmath>
#include <spdlog/spdlog.h>

namespace
{
constexpr float kParamEqMinFrequency = 20.0f;
constexpr float kParamEqMaxFrequency = 20000.0f;
constexpr float kParamEqMinGain = -18.0f;
constexpr float kParamEqMaxGain = 18.0f;
constexpr float kParamEqMinQ = 0.1f;
constexpr float kParamEqMaxQ = 10.0f;

int clampParamEqBandIndex(int bandIndex)
{
    return juce::jlimit(0, NAMProcessor::kParamEqBandCount - 1, bandIndex);
}

int clampParamEqBandCount(int count)
{
    if (count <= 4)
        return 4;
    if (count <= 8)
        return 8;
    if (count <= 10)
        return 10;
    return 12;
}
} // namespace

//==============================================================================
NAMProcessor::NAMProcessor() : PedalboardProcessor()
{
    spdlog::debug("NAMProcessor: Initializing");

    const std::array<float, kParamEqBandCount> defaultFrequencies{80.0f,   120.0f,  250.0f,  650.0f,
                                                                  1000.0f, 1800.0f, 2400.0f, 3600.0f,
                                                                  5200.0f, 7200.0f, 10000.0f, 14000.0f};
    for (int band = 0; band < kParamEqBandCount; ++band)
    {
        paramEqFrequencies[(size_t)band].store(defaultFrequencies[(size_t)band]);
        paramEqGains[(size_t)band].store(0.0f);
        paramEqQs[(size_t)band].store((band == 0 || band == kParamEqBandCount - 1) ? 0.8f : 1.0f);
    }

    // Initialize NAM core (isolated from JUCE to avoid namespace conflicts)
    namCore = std::make_unique<NAMCore>();

    // Initialize convolvers for IR loading
    convolver = std::make_unique<NAMConvolver>();
    convolver2 = std::make_unique<NAMConvolver>();

    // Initialize effects loop (SubGraph for hosting plugins between tone stack and IR)
    effectsLoop = std::make_unique<SubGraphProcessor>();
    effectsLoop->setRackName("FX Loop");
}

NAMProcessor::~NAMProcessor()
{
    spdlog::debug("NAMProcessor: Destroying");
}

//==============================================================================
void NAMProcessor::prepareToPlay(double sampleRate, int estimatedSamplesPerBlock)
{
    spdlog::info("NAMProcessor: prepareToPlay sampleRate={}, blockSize={}", sampleRate, estimatedSamplesPerBlock);

    currentSampleRate = sampleRate;
    currentBlockSize = estimatedSamplesPerBlock;

    // Prepare output buffer for mono NAM processing
    outputBuffer.setSize(1, estimatedSamplesPerBlock, false, false, false);
    outputBuffer.clear();

    // Pre-allocate IR2 blend buffer (RT-safe: avoids per-block allocation)
    ir2Buffer.setSize(2, estimatedSamplesPerBlock, false, false, false);
    ir2Buffer.clear();

    // Prepare NAM core
    namCore->prepare(sampleRate, estimatedSamplesPerBlock);
    syncModelStateAfterCorePrepare();

    // Prepare convolvers for IR
    convolver->prepare(sampleRate, estimatedSamplesPerBlock);
    convolver2->prepare(sampleRate, estimatedSamplesPerBlock);

    // Prepare IR filters
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(estimatedSamplesPerBlock);
    spec.numChannels = 2;

    irLowCutFilter.prepare(spec);
    irHighCutFilter.prepare(spec);
    updateIRFilters();

    resetParametricEqState();
    lastAppliedToneEqMode = -1;

    // Prepare effects loop
    if (effectsLoop)
    {
        effectsLoop->setPlayConfigDetails(2, 2, sampleRate, estimatedSamplesPerBlock);
        effectsLoop->prepareToPlay(sampleRate, estimatedSamplesPerBlock);
    }

    applyDeferredHeavyStateChanges();

    isPrepared.store(true, std::memory_order_release);
}

void NAMProcessor::releaseResources()
{
    isPrepared.store(false, std::memory_order_release);
}

void NAMProcessor::syncModelStateAfterCorePrepare()
{
    if (namCore && modelLoaded.load() && !namCore->isModelLoaded())
    {
        modelLoaded.store(false);
        currentModelFile = juce::File();
        hasDeferredSlimmableSizeApply = false;
        spdlog::warn("NAMProcessor: Core cleared the current model during prepare");
    }
}

void NAMProcessor::applyDeferredHeavyStateChanges()
{
    if (hasDeferredModelClear)
    {
        namCore->clearModel();
        modelLoaded.store(false);
        currentModelFile = juce::File();
        hasDeferredModelClear = false;
    }

    if (hasDeferredModelLoad)
    {
        auto file = deferredModelFile;
        deferredModelFile = juce::File();
        hasDeferredModelLoad = false;
        loadModel(file);
    }

    if (hasDeferredSlimmableSizeApply)
        applySlimmableSizeAtNonAudioBoundary();

    if (hasDeferredIRClear)
    {
        convolver->reset();
        irLoaded.store(false);
        currentIRFile = juce::File();
        hasDeferredIRClear = false;
    }

    if (hasDeferredIRLoad)
    {
        auto file = deferredIRFile;
        deferredIRFile = juce::File();
        hasDeferredIRLoad = false;
        loadIR(file);
    }

    if (hasDeferredIR2Clear)
    {
        convolver2->reset();
        ir2Loaded.store(false);
        currentIRFile2 = juce::File();
        hasDeferredIR2Clear = false;
    }

    if (hasDeferredIR2Load)
    {
        auto file = deferredIR2File;
        deferredIR2File = juce::File();
        hasDeferredIR2Load = false;
        loadIR2(file);
    }
}

//==============================================================================
bool NAMProcessor::loadModel(const juce::File& modelFile)
{
    if (!modelFile.existsAsFile())
    {
        spdlog::error("NAMProcessor: Model file does not exist: {}", modelFile.getFullPathName().toStdString());
        return false;
    }

    const bool wasPrepared = isPrepared.load(std::memory_order_acquire);
    const bool wasSuspended = isSuspended();
    deferredModelFile = juce::File();
    hasDeferredModelLoad = false;
    hasDeferredModelClear = false;

    if (wasPrepared)
    {
        spdlog::info("NAMProcessor: Suspending processing for model load: {}",
                     modelFile.getFullPathName().toStdString());
        suspendProcessing(true);
    }

    spdlog::info("NAMProcessor: Loading model: {}", modelFile.getFullPathName().toStdString());

    bool success = namCore->loadModel(modelFile.getFullPathName().toStdString());

    if (success)
    {
        currentModelFile = modelFile;
        modelLoaded.store(true);
        applySlimmableSizeAtNonAudioBoundary();
        hasDeferredModelLoad = false;
        hasDeferredModelClear = false;
        spdlog::info("NAMProcessor: Model loaded successfully");
    }
    else
    {
        spdlog::error("NAMProcessor: Failed to load model");
    }

    if (wasPrepared)
        suspendProcessing(wasSuspended);

    return success;
}

void NAMProcessor::clearModel()
{
    const bool wasPrepared = isPrepared.load(std::memory_order_acquire);
    const bool wasSuspended = isSuspended();
    deferredModelFile = juce::File();
    hasDeferredModelLoad = false;
    hasDeferredModelClear = false;

    if (wasPrepared)
    {
        spdlog::info("NAMProcessor: Suspending processing for model clear");
        suspendProcessing(true);
    }

    namCore->clearModel();
    modelLoaded.store(false);
    currentModelFile = juce::File();

    if (wasPrepared)
        suspendProcessing(wasSuspended);
}

juce::String NAMProcessor::getModelName() const
{
    if (currentModelFile.existsAsFile())
    {
        return currentModelFile.getFileNameWithoutExtension();
    }
    return "No Model";
}

juce::String NAMProcessor::getModelArchitectureBadge() const
{
    if (!modelLoaded.load() || !currentModelFile.existsAsFile())
        return {};

    NAMModelInfo info;
    if (!NAMCore::getModelInfo(currentModelFile.getFullPathName().toStdString(), info))
        return "NAM";

    if (info.architectureVersion == 2)
        return "A2";
    if (info.architectureVersion == 1)
        return "A1";

    const auto architecture = juce::String(info.architecture).trim();
    if (architecture.isEmpty() || architecture.equalsIgnoreCase("unknown"))
        return "NAM";

    return architecture.length() > 8 ? architecture.substring(0, 8) : architecture;
}

bool NAMProcessor::isCurrentModelSlimmable() const
{
    return modelLoaded.load() && namCore && namCore->isSlimmableModel();
}

void NAMProcessor::setSlimmableSize(float size)
{
    const float clamped = juce::jlimit(0.0f, 1.0f, size);
    slimmableSize.store(clamped);

    if (!namCore || !isCurrentModelSlimmable())
    {
        hasDeferredSlimmableSizeApply = false;
        return;
    }

    if (isPrepared.load(std::memory_order_acquire))
    {
        hasDeferredSlimmableSizeApply = true;
        return;
    }

    applySlimmableSizeAtNonAudioBoundary();
}

void NAMProcessor::applySlimmableSizeAtNonAudioBoundary()
{
    if (!namCore || !modelLoaded.load() || !namCore->isSlimmableModel())
    {
        hasDeferredSlimmableSizeApply = false;
        return;
    }

    namCore->setSlimmableSize(slimmableSize.load());
    hasDeferredSlimmableSizeApply = false;
}

//==============================================================================
bool NAMProcessor::loadIR(const juce::File& irFile)
{
    if (!irFile.existsAsFile())
    {
        spdlog::error("NAMProcessor: IR file does not exist: {}", irFile.getFullPathName().toStdString());
        return false;
    }

    if (isPrepared.load(std::memory_order_acquire))
    {
        deferredIRFile = irFile;
        hasDeferredIRLoad = true;
        hasDeferredIRClear = false;
        spdlog::warn("NAMProcessor: Deferred IR load until processor is inactive: {}",
                     irFile.getFullPathName().toStdString());
        return false;
    }

    spdlog::info("NAMProcessor: Loading IR: {}", irFile.getFullPathName().toStdString());

    try
    {
        convolver->loadIR(irFile);

        currentIRFile = irFile;
        irLoaded.store(true);
        hasDeferredIRLoad = false;
        hasDeferredIRClear = false;

        spdlog::info("NAMProcessor: IR loaded successfully");
        return true;
    }
    catch (const std::exception& e)
    {
        spdlog::error("NAMProcessor: Exception loading IR: {}", e.what());
        irLoaded.store(false);
        return false;
    }
}

void NAMProcessor::clearIR()
{
    if (isPrepared.load(std::memory_order_acquire))
    {
        hasDeferredIRClear = true;
        hasDeferredIRLoad = false;
        deferredIRFile = juce::File();
        spdlog::warn("NAMProcessor: Deferred IR clear until processor is inactive");
        return;
    }

    convolver->reset();
    irLoaded.store(false);
    currentIRFile = juce::File();
    hasDeferredIRClear = false;
}

juce::String NAMProcessor::getIRName() const
{
    if (currentIRFile.existsAsFile())
    {
        return currentIRFile.getFileNameWithoutExtension();
    }
    return "No IR";
}

//==============================================================================
bool NAMProcessor::loadIR2(const juce::File& irFile)
{
    if (!irFile.existsAsFile())
    {
        spdlog::error("NAMProcessor: IR2 file does not exist: {}", irFile.getFullPathName().toStdString());
        return false;
    }

    if (isPrepared.load(std::memory_order_acquire))
    {
        deferredIR2File = irFile;
        hasDeferredIR2Load = true;
        hasDeferredIR2Clear = false;
        spdlog::warn("NAMProcessor: Deferred IR2 load until processor is inactive: {}",
                     irFile.getFullPathName().toStdString());
        return false;
    }

    spdlog::info("NAMProcessor: Loading IR2: {}", irFile.getFullPathName().toStdString());

    try
    {
        convolver2->loadIR(irFile);
        currentIRFile2 = irFile;
        ir2Loaded.store(true);
        hasDeferredIR2Load = false;
        hasDeferredIR2Clear = false;
        spdlog::info("NAMProcessor: IR2 loaded successfully");
        return true;
    }
    catch (const std::exception& e)
    {
        spdlog::error("NAMProcessor: Exception loading IR2: {}", e.what());
        ir2Loaded.store(false);
        return false;
    }
}

void NAMProcessor::clearIR2()
{
    if (isPrepared.load(std::memory_order_acquire))
    {
        hasDeferredIR2Clear = true;
        hasDeferredIR2Load = false;
        deferredIR2File = juce::File();
        spdlog::warn("NAMProcessor: Deferred IR2 clear until processor is inactive");
        return;
    }

    convolver2->reset();
    ir2Loaded.store(false);
    currentIRFile2 = juce::File();
    hasDeferredIR2Clear = false;
}

juce::String NAMProcessor::getIR2Name() const
{
    if (currentIRFile2.existsAsFile())
    {
        return currentIRFile2.getFileNameWithoutExtension();
    }
    return "No IR 2";
}

bool NAMProcessor::hasEffectsLoopContent() const
{
    if (!effectsLoop)
        return false;

    // Check if the effects loop has any nodes beyond the built-in I/O nodes
    const auto& graph = effectsLoop->getInternalGraph();
    // SubGraphProcessor has 3 built-in nodes: audio in, audio out, midi in
    return graph.getNumNodes() > 3;
}

//==============================================================================
void NAMProcessor::processBlock(AudioSampleBuffer& buffer, MidiBuffer& midiMessages)
{
    juce::ignoreUnused(midiMessages);

    if (isSuspended())
    {
        buffer.clear();
        midiMessages.clear();
        return;
    }

    if (buffer.getNumChannels() == 0 || buffer.getNumSamples() == 0)
        return;

    const int numSamples = buffer.getNumSamples();
    const bool doNoiseGate = noiseGateThreshold.load() > -100.0f;
    const bool doToneStack = toneStackEnabled.load();
    const bool doNormalize = normalizeOutput.load();
    const bool doIR = irEnabled.load() && irLoaded.load();
    const bool doIR2 = ir2Loaded.load() && ir2Enabled.load();

    // Get mono input (use left channel)
    float* inputData = buffer.getWritePointer(0);
    float* outputData = outputBuffer.getWritePointer(0);

    // Noise gate trigger (pre-model)
    if (doNoiseGate)
    {
        updateNoiseGate();
        namCore->processNoiseGateTrigger(inputData, numSamples);
    }

    // Apply input gain
    const float inputGainLinear = dBToLinear(inputGain.load());
    if (std::abs(inputGainLinear - 1.0f) > 0.001f)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            inputData[i] *= inputGainLinear;
        }
    }

    // Apply tone stack PRE-model if configured
    if (doToneStack && toneStackPre.load())
    {
        applySelectedToneEq(inputData, numSamples);
    }

    // Process through NAM model
    namCore->process(inputData, outputData, numSamples);
    namCore->finalize(numSamples);

    // Normalize loudness if enabled
    if (doNormalize)
    {
        normalizeModelOutput(outputData, numSamples);
    }

    // Apply noise gate gain
    if (doNoiseGate)
    {
        namCore->processNoiseGateGain(outputData, numSamples);
    }

    // Apply tone stack POST-model if configured (default)
    if (doToneStack && !toneStackPre.load())
    {
        applySelectedToneEq(outputData, numSamples);
    }

    // Copy to both channels (dual mono)
    float* leftChannel = buffer.getWritePointer(0);
    float* rightChannel = buffer.getNumChannels() > 1 ? buffer.getWritePointer(1) : nullptr;

    for (int i = 0; i < numSamples; ++i)
    {
        leftChannel[i] = outputData[i];
        if (rightChannel)
        {
            rightChannel[i] = outputData[i];
        }
    }

    // Process through effects loop (between preamp and cab)
    if (effectsLoopEnabled.load() && effectsLoop)
    {
        effectsLoop->processBlock(buffer, midiMessages);
    }

    // Apply IR convolution with filters if enabled
    if (doIR)
    {
        // Update filter coefficients on the audio thread if parameters changed
        updateIRFilters();

        // Low cut (high-pass) filter BEFORE convolution - removes rumble
        {
            juce::dsp::AudioBlock<float> block(buffer);
            juce::dsp::ProcessContextReplacing<float> context(block);
            irLowCutFilter.process(context);
        }

        if (doIR2)
        {
            // Dual IR mode: process both convolvers and blend
            const int numChannels = buffer.getNumChannels();
            ir2Buffer.setSize(numChannels, numSamples, false, false, true); // no-alloc if already big enough
            for (int ch = 0; ch < numChannels; ++ch)
                ir2Buffer.copyFrom(ch, 0, buffer, ch, 0, numSamples);

            // Convolve IR1 into buffer, IR2 into ir2Buffer
            convolver->process(buffer);
            convolver2->process(ir2Buffer);

            // Equal-power crossfade blend
            const float blendVal = irBlend.load();
            const float angle = blendVal * juce::MathConstants<float>::halfPi;
            const float gain1 = std::cos(angle);
            const float gain2 = std::sin(angle);

            for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            {
                float* dst = buffer.getWritePointer(ch);
                const float* src2 = ir2Buffer.getReadPointer(ch);
                for (int i = 0; i < numSamples; ++i)
                {
                    dst[i] = dst[i] * gain1 + src2[i] * gain2;
                }
            }
        }
        else
        {
            // Single IR mode
            convolver->process(buffer);
        }

        // High cut (low-pass) filter AFTER convolution - tames harshness
        {
            juce::dsp::AudioBlock<float> block(buffer);
            juce::dsp::ProcessContextReplacing<float> context(block);
            irHighCutFilter.process(context);
        }
    }

    // Apply output gain
    const float outputGainLinear = dBToLinear(outputGain.load());
    if (std::abs(outputGainLinear - 1.0f) > 0.001f)
    {
        buffer.applyGain(outputGainLinear);
    }
}

//==============================================================================
void NAMProcessor::updateNoiseGate()
{
    namCore->setNoiseGateParams(noiseGateThreshold.load(), kNoiseGateTime, kNoiseGateRatio, kNoiseGateOpenTime,
                                kNoiseGateHoldTime, kNoiseGateCloseTime);
}

void NAMProcessor::updateToneStack()
{
    namCore->setToneStackParams(bass.load(), mid.load(), treble.load());
}

void NAMProcessor::applySelectedToneEq(float* data, int numSamples)
{
    const int currentMode = toneEqMode.load();
    if (currentMode != lastAppliedToneEqMode)
    {
        resetParametricEqState();
        lastAppliedToneEqMode = currentMode;
    }

    if (getToneEqMode() == ToneEqMode::Parametric)
    {
        applyParametricEq(data, numSamples);
        return;
    }

    updateToneStack();
    namCore->processToneStack(data, numSamples);
}

void NAMProcessor::applyParametricEq(float* data, int numSamples)
{
    if (data == nullptr || numSamples <= 0)
        return;

    updateParametricEqCoefficients();

    for (int sample = 0; sample < numSamples; ++sample)
    {
        float value = data[sample];

        const int activeBandCount = getActiveParamEqBandCount();
        for (int bandIndex = 0; bandIndex < activeBandCount; ++bandIndex)
        {
            auto& band = paramEqBands[(size_t)bandIndex];
            const float filtered = band.b0 * value + band.z1;
            band.z1 = band.b1 * value - band.a1 * filtered + band.z2;
            band.z2 = band.b2 * value - band.a2 * filtered;
            value = filtered;
        }

        data[sample] = std::isfinite(value) ? value : 0.0f;
    }
}

void NAMProcessor::updateParametricEqCoefficients()
{
    if (!isPrepared.load(std::memory_order_acquire))
        return;

    const double sr = currentSampleRate > 1.0 ? currentSampleRate : 44100.0;
    const float maxFrequency = juce::jlimit(kParamEqMinFrequency, kParamEqMaxFrequency, static_cast<float>(sr * 0.49));

    const int activeBandCount = getActiveParamEqBandCount();
    for (int bandIndex = 0; bandIndex < activeBandCount; ++bandIndex)
    {
        auto& band = paramEqBands[(size_t)bandIndex];
        const float frequency = juce::jlimit(kParamEqMinFrequency, maxFrequency, getParamEqBandFrequency(bandIndex));
        const float gain = juce::jlimit(kParamEqMinGain, kParamEqMaxGain, getParamEqBandGain(bandIndex));
        const float q = juce::jlimit(kParamEqMinQ, kParamEqMaxQ, getParamEqBandQ(bandIndex));

        if (frequency == band.lastFrequency && gain == band.lastGain && q == band.lastQ)
            continue;

        if (std::abs(gain) < 0.001f)
        {
            band.b0 = 1.0f;
            band.b1 = 0.0f;
            band.b2 = 0.0f;
            band.a1 = 0.0f;
            band.a2 = 0.0f;
            band.z1 = 0.0f;
            band.z2 = 0.0f;
            band.lastFrequency = frequency;
            band.lastGain = gain;
            band.lastQ = q;
            continue;
        }

        // RBJ peaking EQ. Fixed runtime storage keeps this RT-safe when parameters change.
        const double omega = juce::MathConstants<double>::twoPi * static_cast<double>(frequency) / sr;
        const double sinOmega = std::sin(omega);
        const double cosOmega = std::cos(omega);
        const double alpha = sinOmega / (2.0 * static_cast<double>(q));
        const double amplitude = std::pow(10.0, static_cast<double>(gain) / 40.0);

        const double b0 = 1.0 + alpha * amplitude;
        const double b1 = -2.0 * cosOmega;
        const double b2 = 1.0 - alpha * amplitude;
        const double a0 = 1.0 + alpha / amplitude;
        const double a1 = -2.0 * cosOmega;
        const double a2 = 1.0 - alpha / amplitude;

        if (std::abs(a0) < 1.0e-12)
            continue;

        const double invA0 = 1.0 / a0;
        band.b0 = static_cast<float>(b0 * invA0);
        band.b1 = static_cast<float>(b1 * invA0);
        band.b2 = static_cast<float>(b2 * invA0);
        band.a1 = static_cast<float>(a1 * invA0);
        band.a2 = static_cast<float>(a2 * invA0);
        band.lastFrequency = frequency;
        band.lastGain = gain;
        band.lastQ = q;
    }
}

void NAMProcessor::resetParametricEqState()
{
    for (auto& band : paramEqBands)
    {
        band.z1 = 0.0f;
        band.z2 = 0.0f;
        band.lastFrequency = -1.0f;
        band.lastGain = 999.0f;
        band.lastQ = -1.0f;
    }
}

void NAMProcessor::normalizeModelOutput(float* output, int numSamples)
{
    if (!namCore->hasLoudness())
        return;

    const double loudness = namCore->getLoudness();
    const double targetLoudness = -18.0;
    const double gain = std::pow(10.0, (targetLoudness - loudness) / 20.0);

    for (int i = 0; i < numSamples; ++i)
    {
        output[i] *= static_cast<float>(gain);
    }
}

float NAMProcessor::dBToLinear(float dB)
{
    return std::pow(10.0f, dB / 20.0f);
}

//==============================================================================
void NAMProcessor::setNoiseGateThreshold(float dB)
{
    noiseGateThreshold.store(juce::jlimit(-101.0f, 0.0f, dB));
}

void NAMProcessor::setBass(float value)
{
    bass.store(juce::jlimit(0.0f, 10.0f, value));
}

void NAMProcessor::setMid(float value)
{
    mid.store(juce::jlimit(0.0f, 10.0f, value));
}

void NAMProcessor::setTreble(float value)
{
    treble.store(juce::jlimit(0.0f, 10.0f, value));
}

NAMProcessor::ToneEqMode NAMProcessor::getToneEqMode() const
{
    return toneEqMode.load() == static_cast<int>(ToneEqMode::Parametric) ? ToneEqMode::Parametric : ToneEqMode::Stack;
}

void NAMProcessor::setToneEqMode(ToneEqMode mode)
{
    toneEqMode.store(mode == ToneEqMode::Parametric ? static_cast<int>(ToneEqMode::Parametric)
                                                    : static_cast<int>(ToneEqMode::Stack));
}

int NAMProcessor::getActiveParamEqBandCount() const
{
    return clampParamEqBandCount(activeParamEqBandCount.load());
}

void NAMProcessor::setActiveParamEqBandCount(int count)
{
    activeParamEqBandCount.store(clampParamEqBandCount(count));
    resetParametricEqState();
}

float NAMProcessor::getParamEqBandFrequency(int bandIndex) const
{
    return paramEqFrequencies[(size_t)clampParamEqBandIndex(bandIndex)].load();
}

void NAMProcessor::setParamEqBandFrequency(int bandIndex, float freqHz)
{
    const float maxFrequency =
        juce::jlimit(kParamEqMinFrequency, kParamEqMaxFrequency, static_cast<float>(currentSampleRate * 0.49));
    const float clamped = juce::jlimit(kParamEqMinFrequency, maxFrequency, freqHz);

    paramEqFrequencies[(size_t)clampParamEqBandIndex(bandIndex)].store(clamped);
}

float NAMProcessor::getParamEqBandGain(int bandIndex) const
{
    return paramEqGains[(size_t)clampParamEqBandIndex(bandIndex)].load();
}

void NAMProcessor::setParamEqBandGain(int bandIndex, float gainDb)
{
    const float clamped = juce::jlimit(kParamEqMinGain, kParamEqMaxGain, gainDb);

    paramEqGains[(size_t)clampParamEqBandIndex(bandIndex)].store(clamped);
}

float NAMProcessor::getParamEqBandQ(int bandIndex) const
{
    return paramEqQs[(size_t)clampParamEqBandIndex(bandIndex)].load();
}

void NAMProcessor::setParamEqBandQ(int bandIndex, float q)
{
    const float clamped = juce::jlimit(kParamEqMinQ, kParamEqMaxQ, q);

    paramEqQs[(size_t)clampParamEqBandIndex(bandIndex)].store(clamped);
}

void NAMProcessor::setIRLowCut(float freqHz)
{
    irLowCut.store(juce::jlimit(20.0f, 500.0f, freqHz));
}

void NAMProcessor::setIRHighCut(float freqHz)
{
    irHighCut.store(juce::jlimit(2000.0f, 20000.0f, freqHz));
}

void NAMProcessor::updateIRFilters()
{
    if (!isPrepared.load(std::memory_order_acquire))
        return;

    // Only recompute coefficients when values actually changed (audio thread only)
    const float currentLowCut = irLowCut.load();
    const float currentHighCut = irHighCut.load();

    if (currentLowCut != lastIRLowCut || currentHighCut != lastIRHighCut)
    {
        // RT-safe: compute 2nd-order Butterworth biquad coefficients inline
        // (avoids heap allocation from JUCE's makeHighPass/makeLowPass factory methods)
        const double sr = currentSampleRate;
        constexpr double sqrt2 = 1.4142135623730951; // std::sqrt(2.0)

        // High-pass (low-cut) coefficients via bilinear transform
        {
            const double wc = juce::MathConstants<double>::twoPi * currentLowCut / sr;
            const double K = std::tan(wc * 0.5);
            const double K2 = K * K;
            const double norm = 1.0 / (1.0 + sqrt2 * K + K2);

            float* c = irLowCutFilter.state->coefficients.getRawDataPointer();
            c[0] = (float)(norm);                          // b0
            c[1] = (float)(-2.0 * norm);                   // b1
            c[2] = (float)(norm);                          // b2
            c[3] = 1.0f;                                   // a0
            c[4] = (float)(2.0 * (K2 - 1.0) * norm);       // a1
            c[5] = (float)((1.0 - sqrt2 * K + K2) * norm); // a2
        }

        // Low-pass (high-cut) coefficients via bilinear transform
        {
            const double wc = juce::MathConstants<double>::twoPi * currentHighCut / sr;
            const double K = std::tan(wc * 0.5);
            const double K2 = K * K;
            const double norm = 1.0 / (1.0 + sqrt2 * K + K2);

            float* c = irHighCutFilter.state->coefficients.getRawDataPointer();
            c[0] = (float)(K2 * norm);                     // b0
            c[1] = (float)(2.0 * K2 * norm);               // b1
            c[2] = (float)(K2 * norm);                     // b2
            c[3] = 1.0f;                                   // a0
            c[4] = (float)(2.0 * (K2 - 1.0) * norm);       // a1
            c[5] = (float)((1.0 - sqrt2 * K + K2) * norm); // a2
        }

        lastIRLowCut = currentLowCut;
        lastIRHighCut = currentHighCut;
    }
}

//==============================================================================
const String NAMProcessor::getParameterName(int parameterIndex)
{
    switch (parameterIndex)
    {
    case InputGainParam:
        return "Input";
    case OutputGainParam:
        return "Output";
    case NoiseGateParam:
        return "Gate";
    case BassParam:
        return "Bass";
    case MidParam:
        return "Mid";
    case TrebleParam:
        return "Treble";
    case ToneStackEnabledParam:
        return "EQ On";
    case NormalizeParam:
        return "Normalize";
    case IRMixParam:
        return "IR Mix";
    case ToneStackPreParam:
        return "EQ Pre";
    case IRBlendParam:
        return "IR Blend";
    default:
        return "";
    }
}

float NAMProcessor::getParameter(int parameterIndex)
{
    switch (parameterIndex)
    {
    case InputGainParam:
        return inputGain.load();
    case OutputGainParam:
        return outputGain.load();
    case NoiseGateParam:
        return noiseGateThreshold.load();
    case BassParam:
        return bass.load();
    case MidParam:
        return mid.load();
    case TrebleParam:
        return treble.load();
    case ToneStackEnabledParam:
        return toneStackEnabled.load() ? 1.0f : 0.0f;
    case NormalizeParam:
        return normalizeOutput.load() ? 1.0f : 0.0f;
    case IRMixParam:
        return irEnabled.load() ? 1.0f : 0.0f;
    case ToneStackPreParam:
        return toneStackPre.load() ? 1.0f : 0.0f;
    case IRBlendParam:
        return irBlend.load();
    default:
        return 0.0f;
    }
}

const String NAMProcessor::getParameterText(int parameterIndex)
{
    switch (parameterIndex)
    {
    case InputGainParam:
        return String(inputGain.load(), 1) + " dB";
    case OutputGainParam:
        return String(outputGain.load(), 1) + " dB";
    case NoiseGateParam:
    {
        float threshold = noiseGateThreshold.load();
        if (threshold <= -100.0f)
            return "Off";
        return String(static_cast<int>(threshold)) + " dB";
    }
    case BassParam:
        return String(bass.load(), 1);
    case MidParam:
        return String(mid.load(), 1);
    case TrebleParam:
        return String(treble.load(), 1);
    case ToneStackEnabledParam:
        return toneStackEnabled.load() ? "On" : "Off";
    case NormalizeParam:
        return normalizeOutput.load() ? "On" : "Off";
    case IRMixParam:
        return irEnabled.load() ? "On" : "Off";
    case ToneStackPreParam:
        return toneStackPre.load() ? "Pre" : "Post";
    case IRBlendParam:
        return String(static_cast<int>(irBlend.load() * 100.0f)) + "%";
    default:
        return "";
    }
}

void NAMProcessor::setParameter(int parameterIndex, float newValue)
{
    switch (parameterIndex)
    {
    case InputGainParam:
        setInputGain(newValue);
        break;
    case OutputGainParam:
        setOutputGain(newValue);
        break;
    case NoiseGateParam:
        setNoiseGateThreshold(newValue);
        break;
    case BassParam:
        setBass(newValue);
        break;
    case MidParam:
        setMid(newValue);
        break;
    case TrebleParam:
        setTreble(newValue);
        break;
    case ToneStackEnabledParam:
        setToneStackEnabled(newValue > 0.5f);
        break;
    case NormalizeParam:
        setNormalizeOutput(newValue > 0.5f);
        break;
    case IRMixParam:
        setIREnabled(newValue > 0.5f);
        break;
    case ToneStackPreParam:
        setToneStackPre(newValue > 0.5f);
        break;
    case IRBlendParam:
        setIRBlend(newValue);
        break;
    }
}

//==============================================================================
void NAMProcessor::getStateInformation(MemoryBlock& destData)
{
    MemoryOutputStream stream(destData, false);

    stream.writeInt(9); // Version (9 = NAM A2 slimmable size)

    // Model and IR paths
    stream.writeString(currentModelFile.getFullPathName());
    stream.writeString(currentIRFile.getFullPathName());

    // Parameters
    stream.writeFloat(inputGain.load());
    stream.writeFloat(outputGain.load());
    stream.writeFloat(noiseGateThreshold.load());
    stream.writeFloat(bass.load());
    stream.writeFloat(mid.load());
    stream.writeFloat(treble.load());
    stream.writeBool(toneStackEnabled.load());
    stream.writeBool(normalizeOutput.load());
    stream.writeBool(irEnabled.load());

    // Effects loop (v2+)
    stream.writeBool(effectsLoopEnabled.load());
    if (effectsLoop)
    {
        MemoryBlock fxLoopState;
        effectsLoop->getStateInformation(fxLoopState);
        stream.writeInt(static_cast<int>(fxLoopState.getSize()));
        stream.write(fxLoopState.getData(), fxLoopState.getSize());
    }
    else
    {
        stream.writeInt(0);
    }

    // IR filters (v3+)
    stream.writeFloat(irLowCut.load());
    stream.writeFloat(irHighCut.load());

    // Tone stack pre/post (v4+)
    stream.writeBool(toneStackPre.load());

    // Dual IR + blend (v5+)
    stream.writeString(currentIRFile2.getFullPathName());
    stream.writeFloat(irBlend.load());

    // IR2 enable toggle (v6+)
    stream.writeBool(ir2Enabled.load());

    // NAM EQ mode + parametric EQ bands (v7+)
    stream.writeInt(toneEqMode.load());
    stream.writeInt(getActiveParamEqBandCount());
    for (int bandIndex = 0; bandIndex < kParamEqBandCount; ++bandIndex)
    {
        stream.writeFloat(getParamEqBandFrequency(bandIndex));
        stream.writeFloat(getParamEqBandGain(bandIndex));
        stream.writeFloat(getParamEqBandQ(bandIndex));
    }

    // NAM A2 slimmable size (v9+)
    stream.writeFloat(slimmableSize.load());
}

void NAMProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    MemoryInputStream stream(data, static_cast<size_t>(sizeInBytes), false);

    int version = stream.readInt();

    // Load model
    String modelPath = stream.readString();
    if (modelPath.isNotEmpty())
    {
        File modelFile(modelPath);
        if (modelFile.existsAsFile())
        {
            loadModel(modelFile);
        }
    }

    // Load IR
    String irPath = stream.readString();
    if (irPath.isNotEmpty())
    {
        File irFile(irPath);
        if (irFile.existsAsFile())
        {
            loadIR(irFile);
        }
    }

    // Parameters
    inputGain.store(stream.readFloat());
    outputGain.store(stream.readFloat());
    noiseGateThreshold.store(stream.readFloat());
    bass.store(stream.readFloat());
    mid.store(stream.readFloat());
    treble.store(stream.readFloat());
    toneStackEnabled.store(stream.readBool());
    normalizeOutput.store(stream.readBool());
    irEnabled.store(stream.readBool());

    // Effects loop (v2+)
    if (version >= 2 && !stream.isExhausted())
    {
        effectsLoopEnabled.store(stream.readBool());
        int fxLoopStateSize = stream.readInt();
        if (fxLoopStateSize > 0 && effectsLoop)
        {
            MemoryBlock fxLoopState;
            fxLoopState.setSize(static_cast<size_t>(fxLoopStateSize));
            stream.read(fxLoopState.getData(), static_cast<int>(fxLoopState.getSize()));
            effectsLoop->setStateInformation(fxLoopState.getData(), static_cast<int>(fxLoopState.getSize()));
        }
    }

    // IR filters (v3+)
    if (version >= 3 && !stream.isExhausted())
    {
        irLowCut.store(stream.readFloat());
        irHighCut.store(stream.readFloat());
        updateIRFilters();
    }

    // Tone stack pre/post (v4+)
    if (version >= 4 && !stream.isExhausted())
    {
        toneStackPre.store(stream.readBool());
    }

    // Dual IR + blend (v5+)
    if (version >= 5 && !stream.isExhausted())
    {
        String ir2Path = stream.readString();
        if (ir2Path.isNotEmpty())
        {
            File ir2File(ir2Path);
            if (ir2File.existsAsFile())
            {
                loadIR2(ir2File);
            }
        }
        irBlend.store(stream.readFloat());
    }

    // IR2 enable toggle (v6+)
    if (version >= 6 && !stream.isExhausted())
    {
        ir2Enabled.store(stream.readBool());
    }

    // NAM EQ mode + parametric EQ bands (v7+)
    if (version >= 7 && !stream.isExhausted())
    {
        const int restoredMode = stream.readInt();
        setToneEqMode(restoredMode == static_cast<int>(ToneEqMode::Parametric) ? ToneEqMode::Parametric
                                                                               : ToneEqMode::Stack);
        if (version >= 8 && !stream.isExhausted())
            setActiveParamEqBandCount(stream.readInt());
        else
            setActiveParamEqBandCount(4);

        for (int bandIndex = 0; bandIndex < kParamEqBandCount && !stream.isExhausted(); ++bandIndex)
        {
            setParamEqBandFrequency(bandIndex, stream.readFloat());
            setParamEqBandGain(bandIndex, stream.readFloat());
            setParamEqBandQ(bandIndex, stream.readFloat());
        }

        resetParametricEqState();
        lastAppliedToneEqMode = -1;
    }

    // NAM A2 slimmable size (v9+)
    if (version >= 9 && !stream.isExhausted())
    {
        setSlimmableSize(stream.readFloat());
    }
}

//==============================================================================
Component* NAMProcessor::getControls()
{
    return new NAMControl(this);
}

AudioProcessorEditor* NAMProcessor::createEditor()
{
    return nullptr; // Not used - we use getControls() instead
}

void NAMProcessor::updateEditorBounds(const juce::Rectangle<int>& bounds)
{
    editorBounds = bounds;
}

//==============================================================================
void NAMProcessor::fillInPluginDescription(PluginDescription& description) const
{
    description.name = "NAM Loader";
    description.descriptiveName = "Neural Amp Modeler Loader";
    description.pluginFormatName = "Internal";
    description.category = "Effects";
    description.manufacturerName = "Pedalboard3";
    description.version = "1.0.0";
    description.fileOrIdentifier = "NAM Loader";
    description.uniqueId = 0x4E414D4C; // "NAML"
    description.isInstrument = false;
    description.numInputChannels = 2;
    description.numOutputChannels = 2;
}
