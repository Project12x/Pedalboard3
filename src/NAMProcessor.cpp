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

#include <spdlog/spdlog.h>


//==============================================================================
NAMProcessor::NAMProcessor() : PedalboardProcessor()
{
    spdlog::debug("NAMProcessor: Initializing");

    // Initialize NAM core (isolated from JUCE to avoid namespace conflicts)
    namCore = std::make_unique<NAMCore>();

    // Initialize convolver for IR loading
    convolver = std::make_unique<NAMConvolver>();

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

    // Prepare NAM core
    namCore->prepare(sampleRate, estimatedSamplesPerBlock);

    // Prepare convolver for IR
    convolver->prepare(sampleRate, estimatedSamplesPerBlock);

    // Prepare IR filters
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(estimatedSamplesPerBlock);
    spec.numChannels = 2;

    irLowCutFilter.prepare(spec);
    irHighCutFilter.prepare(spec);
    updateIRFilters();

    // Prepare effects loop
    if (effectsLoop)
    {
        effectsLoop->setPlayConfigDetails(2, 2, sampleRate, estimatedSamplesPerBlock);
        effectsLoop->prepareToPlay(sampleRate, estimatedSamplesPerBlock);
    }

    isPrepared = true;
}

void NAMProcessor::releaseResources()
{
    isPrepared = false;
}

//==============================================================================
bool NAMProcessor::loadModel(const juce::File& modelFile)
{
    if (!modelFile.existsAsFile())
    {
        spdlog::error("NAMProcessor: Model file does not exist: {}", modelFile.getFullPathName().toStdString());
        return false;
    }

    spdlog::info("NAMProcessor: Loading model: {}", modelFile.getFullPathName().toStdString());

    bool success = namCore->loadModel(modelFile.getFullPathName().toStdString());

    if (success)
    {
        currentModelFile = modelFile;
        modelLoaded.store(true);
        spdlog::info("NAMProcessor: Model loaded successfully");
    }
    else
    {
        spdlog::error("NAMProcessor: Failed to load model");
    }

    return success;
}

void NAMProcessor::clearModel()
{
    namCore->clearModel();
    modelLoaded.store(false);
    currentModelFile = juce::File();
}

juce::String NAMProcessor::getModelName() const
{
    if (currentModelFile.existsAsFile())
    {
        return currentModelFile.getFileNameWithoutExtension();
    }
    return "No Model";
}

//==============================================================================
bool NAMProcessor::loadIR(const juce::File& irFile)
{
    if (!irFile.existsAsFile())
    {
        spdlog::error("NAMProcessor: IR file does not exist: {}", irFile.getFullPathName().toStdString());
        return false;
    }

    spdlog::info("NAMProcessor: Loading IR: {}", irFile.getFullPathName().toStdString());

    try
    {
        convolver->loadIR(irFile);

        currentIRFile = irFile;
        irLoaded.store(true);

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
    convolver->reset();
    irLoaded.store(false);
    currentIRFile = juce::File();
}

juce::String NAMProcessor::getIRName() const
{
    if (currentIRFile.existsAsFile())
    {
        return currentIRFile.getFileNameWithoutExtension();
    }
    return "No IR";
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

    if (buffer.getNumChannels() == 0 || buffer.getNumSamples() == 0)
        return;

    const int numSamples = buffer.getNumSamples();
    const bool doNoiseGate = noiseGateThreshold.load() > -100.0f;
    const bool doToneStack = toneStackEnabled.load();
    const bool doNormalize = normalizeOutput.load();
    const bool doIR = irEnabled.load() && irLoaded.load();

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
        updateToneStack();
        // Tone stack operates on mono inputData before NAM model
        namCore->processToneStack(inputData, numSamples);
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
        updateToneStack();
        namCore->processToneStack(outputData, numSamples);
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

        // Apply IR convolution
        convolver->process(buffer);

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
    if (!isPrepared)
        return;

    // Only recompute coefficients when values actually changed (audio thread only)
    const float currentLowCut = irLowCut.load();
    const float currentHighCut = irHighCut.load();

    if (currentLowCut != lastIRLowCut || currentHighCut != lastIRHighCut)
    {
        *irLowCutFilter.state = *juce::dsp::IIR::Coefficients<float>::makeHighPass(currentSampleRate, currentLowCut);
        *irHighCutFilter.state = *juce::dsp::IIR::Coefficients<float>::makeLowPass(currentSampleRate, currentHighCut);
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
    }
}

//==============================================================================
void NAMProcessor::getStateInformation(MemoryBlock& destData)
{
    MemoryOutputStream stream(destData, false);

    stream.writeInt(4); // Version (4 = added tone stack pre/post)

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
