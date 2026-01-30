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
    // Initialize convolver with low latency mode for live use
    convolver.reset();
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
    lowCutFilter.prepare(spec);
    highCutFilter.prepare(spec);

    // Prepare dry buffer for mixing
    dryBuffer.setSize(2, estimatedSamplesPerBlock);

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

    // Load with stereo support and automatic trimming
    convolver.loadImpulseResponse(irFile, juce::dsp::Convolution::Stereo::yes, juce::dsp::Convolution::Trim::yes,
                                  0 // Auto-size (use full IR)
    );

    irLoaded.store(true);
}

//==============================================================================
void IRLoaderProcessor::processBlock(AudioSampleBuffer& buffer, MidiBuffer& midiMessages)
{
    ignoreUnused(midiMessages);

    if (buffer.getNumChannels() == 0 || buffer.getNumSamples() == 0)
        return;

    const int numSamples = buffer.getNumSamples();
    const int numChannels = juce::jmin(buffer.getNumChannels(), 2);
    const float currentMix = mix.load();

    // Ensure we have stereo processing
    if (numChannels < 2 && buffer.getNumChannels() == 1)
    {
        // Mono input: duplicate to process as stereo, then sum back
        // For now, just process mono as-is
    }

    // Store dry signal for mixing
    for (int ch = 0; ch < numChannels; ++ch)
    {
        dryBuffer.copyFrom(ch, 0, buffer, ch, 0, numSamples);
    }

    // Apply low cut filter (pre-IR)
    {
        juce::dsp::AudioBlock<float> block(buffer);
        juce::dsp::ProcessContextReplacing<float> context(block);
        lowCutFilter.process(context);
    }

    // Apply convolution (the IR itself)
    if (irLoaded.load())
    {
        juce::dsp::AudioBlock<float> block(buffer);
        juce::dsp::ProcessContextReplacing<float> context(block);
        convolver.process(context);
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
            {
                wetData[i] = (wetData[i] * wetGain) + (dryData[i] * dryGain);
            }
        }
    }
}

//==============================================================================
void IRLoaderProcessor::updateFilters()
{
    if (!isPrepared)
        return;

    // Low cut (high-pass) filter
    auto lowCutCoeffs = juce::dsp::IIR::Coefficients<float>::makeHighPass(currentSampleRate, lowCut.load());
    *lowCutFilter.state = *lowCutCoeffs;

    // High cut (low-pass) filter
    auto highCutCoeffs = juce::dsp::IIR::Coefficients<float>::makeLowPass(currentSampleRate, highCut.load());
    *highCutFilter.state = *highCutCoeffs;
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
    }
}

//==============================================================================
void IRLoaderProcessor::getStateInformation(MemoryBlock& destData)
{
    MemoryOutputStream stream(destData, false);

    stream.writeInt(1); // Version
    stream.writeString(currentIRFile.getFullPathName());
    stream.writeFloat(mix.load());
    stream.writeFloat(lowCut.load());
    stream.writeFloat(highCut.load());
}

void IRLoaderProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    MemoryInputStream stream(data, static_cast<size_t>(sizeInBytes), false);

    int version = stream.readInt();
    ignoreUnused(version);

    String irPath = stream.readString();
    if (irPath.isNotEmpty())
    {
        File irFile(irPath);
        if (irFile.existsAsFile())
        {
            loadIRFile(irFile);
        }
    }

    mix.store(stream.readFloat());
    lowCut.store(stream.readFloat());
    highCut.store(stream.readFloat());

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
