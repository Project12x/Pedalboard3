/*
  ==============================================================================

    IRLoaderProcessor.cpp
    Cabinet Impulse Response loader implementation

  ==============================================================================
*/

#include "IRLoaderProcessor.h"

#include "IRLoaderControl.h"

//==============================================================================
IRLoaderProcessor::IRLoaderProcessor() : PedalboardProcessor()
{
    convolver.reset();
    convolver2.reset();
}

IRLoaderProcessor::~IRLoaderProcessor() {}

//==============================================================================
void IRLoaderProcessor::prepareToPlay(double sampleRate, int estimatedSamplesPerBlock)
{
    currentSampleRate = sampleRate;

    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(estimatedSamplesPerBlock);
    spec.numChannels = 2;

    convolver.prepare(spec);
    convolver2.prepare(spec);
    lowCutFilter.prepare(spec);
    highCutFilter.prepare(spec);

    dryBuffer.setSize(2, estimatedSamplesPerBlock);
    ir2Buffer.setSize(2, estimatedSamplesPerBlock);

    updateFilters();
    isPrepared = true;
}

//==============================================================================
void IRLoaderProcessor::loadIRFile(const File& irFile)
{
    if (!irFile.existsAsFile())
    {
        irLoaded.store(false);
        return;
    }

    currentIRFile = irFile;
    convolver.loadImpulseResponse(irFile, juce::dsp::Convolution::Stereo::yes, juce::dsp::Convolution::Trim::yes, 0);
    irLoaded.store(true);
}

//==============================================================================
void IRLoaderProcessor::loadIRFile2(const File& irFile)
{
    if (!irFile.existsAsFile())
    {
        ir2Loaded.store(false);
        return;
    }

    currentIRFile2 = irFile;
    convolver2.loadImpulseResponse(irFile, juce::dsp::Convolution::Stereo::yes, juce::dsp::Convolution::Trim::yes, 0);
    ir2Loaded.store(true);
}

void IRLoaderProcessor::clearIR2()
{
    currentIRFile2 = File();
    ir2Loaded.store(false);
    convolver2.reset();
}

//==============================================================================
void IRLoaderProcessor::processBlock(AudioSampleBuffer& buffer, MidiBuffer& midiMessages)
{
    ignoreUnused(midiMessages);

    if (buffer.getNumChannels() == 0 || buffer.getNumSamples() == 0)
        return;

    updateFilters();

    const int numSamples = buffer.getNumSamples();
    const int numChannels = juce::jmin(buffer.getNumChannels(), 2);
    const float currentMix = mix.load();
    const float blend = irBlend.load();
    const bool hasIR1 = irLoaded.load();
    const bool hasIR2 = ir2Loaded.load();

    // Store dry signal for wet/dry mixing
    for (int ch = 0; ch < numChannels; ++ch)
        dryBuffer.copyFrom(ch, 0, buffer, ch, 0, numSamples);

    // Apply low cut filter (pre-IR)
    {
        juce::dsp::AudioBlock<float> block(buffer);
        juce::dsp::ProcessContextReplacing<float> context(block);
        lowCutFilter.process(context);
    }

    // Dual IR processing with equal-power crossfade
    if (hasIR1 && hasIR2 && blend > 0.0f && blend < 1.0f)
    {
        // Copy pre-filtered signal for IR2 processing
        for (int ch = 0; ch < numChannels; ++ch)
            ir2Buffer.copyFrom(ch, 0, buffer, ch, 0, numSamples);

        // Process IR1
        {
            juce::dsp::AudioBlock<float> block(buffer);
            juce::dsp::ProcessContextReplacing<float> context(block);
            convolver.process(context);
        }

        // Process IR2
        {
            juce::dsp::AudioBlock<float> block2(ir2Buffer);
            juce::dsp::ProcessContextReplacing<float> context2(block2);
            convolver2.process(context2);
        }

        // Equal-power crossfade: gain1 = cos(blend * pi/2), gain2 = sin(blend * pi/2)
        const float angle = blend * juce::MathConstants<float>::halfPi;
        const float gain1 = std::cos(angle);
        const float gain2 = std::sin(angle);

        for (int ch = 0; ch < numChannels; ++ch)
        {
            float* out = buffer.getWritePointer(ch);
            const float* ir2Data = ir2Buffer.getReadPointer(ch);
            for (int i = 0; i < numSamples; ++i)
                out[i] = out[i] * gain1 + ir2Data[i] * gain2;
        }
    }
    else if (hasIR1 && (blend == 0.0f || !hasIR2))
    {
        // IR1 only
        juce::dsp::AudioBlock<float> block(buffer);
        juce::dsp::ProcessContextReplacing<float> context(block);
        convolver.process(context);
    }
    else if (hasIR2 && (blend == 1.0f || !hasIR1))
    {
        // IR2 only
        juce::dsp::AudioBlock<float> block(buffer);
        juce::dsp::ProcessContextReplacing<float> context(block);
        convolver2.process(context);
    }

    // Apply high cut filter (post-IR)
    {
        juce::dsp::AudioBlock<float> block(buffer);
        juce::dsp::ProcessContextReplacing<float> context(block);
        highCutFilter.process(context);
    }

    // Mix dry and wet signals
    if (currentMix < 1.0f)
    {
        const float wetGain = currentMix;
        const float dryGain = 1.0f - currentMix;

        for (int ch = 0; ch < numChannels; ++ch)
        {
            float* wetData = buffer.getWritePointer(ch);
            const float* dryData = dryBuffer.getReadPointer(ch);
            for (int i = 0; i < numSamples; ++i)
                wetData[i] = (wetData[i] * wetGain) + (dryData[i] * dryGain);
        }
    }
}

//==============================================================================
void IRLoaderProcessor::updateFilters()
{
    if (!isPrepared)
        return;

    // Only recompute coefficients when values actually changed (audio thread only)
    const float currentLowCut = lowCut.load();
    const float currentHighCut = highCut.load();

    if (currentLowCut != lastLowCut || currentHighCut != lastHighCut)
    {
        *lowCutFilter.state = *juce::dsp::IIR::Coefficients<float>::makeHighPass(currentSampleRate, currentLowCut);
        *highCutFilter.state = *juce::dsp::IIR::Coefficients<float>::makeLowPass(currentSampleRate, currentHighCut);
        lastLowCut = currentLowCut;
        lastHighCut = currentHighCut;
    }
}

//==============================================================================
const String IRLoaderProcessor::getParameterName(int parameterIndex)
{
    switch (parameterIndex)
    {
    case MixParam:
        return "Mix";
    case LowCutParam:
        return "Low Cut";
    case HighCutParam:
        return "High Cut";
    case BlendParam:
        return "IR Blend";
    default:
        return "";
    }
}

float IRLoaderProcessor::getParameter(int parameterIndex)
{
    switch (parameterIndex)
    {
    case MixParam:
        return mix.load();
    case LowCutParam:
        return lowCut.load();
    case HighCutParam:
        return highCut.load();
    case BlendParam:
        return irBlend.load();
    default:
        return 0.0f;
    }
}

const String IRLoaderProcessor::getParameterText(int parameterIndex)
{
    switch (parameterIndex)
    {
    case MixParam:
        return String(static_cast<int>(mix.load() * 100.0f)) + "%";
    case LowCutParam:
        return String(static_cast<int>(lowCut.load())) + " Hz";
    case HighCutParam:
        return String(static_cast<int>(highCut.load())) + " Hz";
    case BlendParam:
        return String(static_cast<int>(irBlend.load() * 100.0f)) + "%";
    default:
        return "";
    }
}

void IRLoaderProcessor::setParameter(int parameterIndex, float newValue)
{
    switch (parameterIndex)
    {
    case MixParam:
        setMix(newValue);
        break;
    case LowCutParam:
        setLowCut(newValue);
        break;
    case HighCutParam:
        setHighCut(newValue);
        break;
    case BlendParam:
        setBlend(newValue);
        break;
    }
}

//==============================================================================
void IRLoaderProcessor::getStateInformation(MemoryBlock& destData)
{
    MemoryOutputStream stream(destData, false);

    stream.writeInt(2); // Version 2: added IR2 + blend
    stream.writeString(currentIRFile.getFullPathName());
    stream.writeFloat(mix.load());
    stream.writeFloat(lowCut.load());
    stream.writeFloat(highCut.load());
    // v2 fields
    stream.writeString(currentIRFile2.getFullPathName());
    stream.writeFloat(irBlend.load());
}

void IRLoaderProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    MemoryInputStream stream(data, static_cast<size_t>(sizeInBytes), false);

    int version = stream.readInt();

    String irPath = stream.readString();
    if (irPath.isNotEmpty())
    {
        File irFile(irPath);
        if (irFile.existsAsFile())
            loadIRFile(irFile);
    }

    mix.store(stream.readFloat());
    lowCut.store(stream.readFloat());
    highCut.store(stream.readFloat());

    // v2: IR2 + blend
    if (version >= 2 && !stream.isExhausted())
    {
        String ir2Path = stream.readString();
        if (ir2Path.isNotEmpty())
        {
            File ir2File(ir2Path);
            if (ir2File.existsAsFile())
                loadIRFile2(ir2File);
        }
        irBlend.store(stream.readFloat());
    }

    updateFilters();
}

//==============================================================================
Component* IRLoaderProcessor::getControls()
{
    return new IRLoaderControl(this);
}

AudioProcessorEditor* IRLoaderProcessor::createEditor()
{
    return nullptr; // Not used - we use getControls() instead
}

void IRLoaderProcessor::updateEditorBounds(const Rectangle<int>& bounds)
{
    editorBounds = bounds;
}

//==============================================================================
void IRLoaderProcessor::fillInPluginDescription(PluginDescription& description) const
{
    description.name = "IR Loader";
    description.descriptiveName = "Cabinet Impulse Response Loader";
    description.pluginFormatName = "Internal";
    description.category = "Effects";
    description.manufacturerName = "Pedalboard3";
    description.version = "1.0.0";
    description.fileOrIdentifier = "IR Loader";
    description.uniqueId = 0x49524C44; // "IRLD"
    description.isInstrument = false;
    description.numInputChannels = 2;
    description.numOutputChannels = 2;
}
