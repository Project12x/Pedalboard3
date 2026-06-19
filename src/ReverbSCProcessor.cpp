#include "ReverbSCProcessor.h"

#include <cmath>

namespace
{
constexpr const char* kStateTag = "Pedalboard3ReverbSCSettings";
constexpr int kStateVersion = 1;
constexpr float kMinDampingHz = 200.0f;
constexpr float kMaxDampingHz = 20000.0f;
} // namespace

ReverbSCProcessor::ReverbSCProcessor()
{
    setPlayConfigDetails(2, 2, 44100.0, 512);
}

Component* ReverbSCProcessor::getControls()
{
    return new Component();
}

void ReverbSCProcessor::updateEditorBounds(const Rectangle<int>& bounds)
{
    editorBounds = bounds;
}

void ReverbSCProcessor::fillInPluginDescription(PluginDescription& description) const
{
    description.name = "ReverbSC";
    description.descriptiveName = "Sean Costello ReverbSC";
    description.pluginFormatName = "Internal";
    description.category = "Effects";
    description.manufacturerName = "Pedalboard3";
    description.version = "1.0";
    description.fileOrIdentifier = "ReverbSC";
    description.lastFileModTime = Time();
    description.lastInfoUpdateTime = Time();
    description.uniqueId = 0x72767363; // "rvsc"
    description.isInstrument = false;
    description.numInputChannels = 2;
    description.numOutputChannels = 2;
}

void ReverbSCProcessor::prepareToPlay(double sampleRate, int estimatedSamplesPerBlock)
{
    maxPreparedBlockSize = jmax(1, estimatedSamplesPerBlock);

    dryBuffer.setSize(2, maxPreparedBlockSize, false, false, true);
    wetBuffer.setSize(2, maxPreparedBlockSize, false, false, true);

    reverb.prepare(sampleRate, maxPreparedBlockSize);
    reverb.setFeedback(feedback.load(std::memory_order_relaxed));
    reverb.setDampingHz(normalisedToDampingHz(damping.load(std::memory_order_relaxed)));
    prepared = true;
}

void ReverbSCProcessor::processBlock(AudioSampleBuffer& buffer, MidiBuffer& midiMessages)
{
    ignoreUnused(midiMessages);

    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    if (!prepared || numSamples <= 0 || numChannels < 2 || numSamples > maxPreparedBlockSize)
    {
        buffer.clear();
        return;
    }

    dryBuffer.copyFrom(0, 0, buffer, 0, 0, numSamples);
    dryBuffer.copyFrom(1, 0, buffer, 1, 0, numSamples);
    wetBuffer.clear(0, 0, numSamples);
    wetBuffer.clear(1, 0, numSamples);

    const float currentMix = mix.load(std::memory_order_relaxed);
    const float currentFeedback = feedback.load(std::memory_order_relaxed);
    const float currentDamping = damping.load(std::memory_order_relaxed);
    const float currentWidth = width.load(std::memory_order_relaxed);
    const float currentOutputGain = normalisedToOutputGain(output.load(std::memory_order_relaxed));

    reverb.setFeedback(currentFeedback);
    reverb.setDampingHz(normalisedToDampingHz(currentDamping));
    reverb.process(dryBuffer.getReadPointer(0), dryBuffer.getReadPointer(1), wetBuffer.getWritePointer(0),
                   wetBuffer.getWritePointer(1), numSamples);

    auto* outL = buffer.getWritePointer(0);
    auto* outR = buffer.getWritePointer(1);
    const auto* dryL = dryBuffer.getReadPointer(0);
    const auto* dryR = dryBuffer.getReadPointer(1);
    const auto* wetL = wetBuffer.getReadPointer(0);
    const auto* wetR = wetBuffer.getReadPointer(1);

    for (int sample = 0; sample < numSamples; ++sample)
    {
        const float mid = 0.5f * (wetL[sample] + wetR[sample]);
        const float side = 0.5f * (wetL[sample] - wetR[sample]) * currentWidth;
        const float widenedWetL = mid + side;
        const float widenedWetR = mid - side;

        outL[sample] = ((dryL[sample] * (1.0f - currentMix)) + (widenedWetL * currentMix)) * currentOutputGain;
        outR[sample] = ((dryR[sample] * (1.0f - currentMix)) + (widenedWetR * currentMix)) * currentOutputGain;
    }

    for (int channel = 2; channel < numChannels; ++channel)
        buffer.clear(channel, 0, numSamples);
}

const String ReverbSCProcessor::getParameterName(int parameterIndex)
{
    switch (parameterIndex)
    {
    case MixParam:
        return "Mix";
    case FeedbackParam:
        return "Feedback";
    case DampingParam:
        return "Damping";
    case WidthParam:
        return "Width";
    case OutputParam:
        return "Output";
    default:
        return "";
    }
}

float ReverbSCProcessor::getParameter(int parameterIndex)
{
    switch (parameterIndex)
    {
    case MixParam:
        return mix.load(std::memory_order_relaxed);
    case FeedbackParam:
        return feedback.load(std::memory_order_relaxed);
    case DampingParam:
        return damping.load(std::memory_order_relaxed);
    case WidthParam:
        return width.load(std::memory_order_relaxed);
    case OutputParam:
        return output.load(std::memory_order_relaxed);
    default:
        return 0.0f;
    }
}

const String ReverbSCProcessor::getParameterText(int parameterIndex)
{
    switch (parameterIndex)
    {
    case MixParam:
        return String(static_cast<int>(mix.load(std::memory_order_relaxed) * 100.0f)) + "%";
    case FeedbackParam:
        return String(feedback.load(std::memory_order_relaxed), 2);
    case DampingParam:
        return String(static_cast<int>(normalisedToDampingHz(damping.load(std::memory_order_relaxed)))) + " Hz";
    case WidthParam:
        return String(static_cast<int>(width.load(std::memory_order_relaxed) * 100.0f)) + "%";
    case OutputParam:
        return String(normalisedToOutputGain(output.load(std::memory_order_relaxed)), 2) + "x";
    default:
        return "";
    }
}

void ReverbSCProcessor::setParameter(int parameterIndex, float newValue)
{
    const float value = clampNormalised(newValue);

    switch (parameterIndex)
    {
    case MixParam:
        mix.store(value, std::memory_order_relaxed);
        break;
    case FeedbackParam:
        feedback.store(std::min(value, 0.99f), std::memory_order_relaxed);
        break;
    case DampingParam:
        damping.store(value, std::memory_order_relaxed);
        break;
    case WidthParam:
        width.store(value, std::memory_order_relaxed);
        break;
    case OutputParam:
        output.store(value, std::memory_order_relaxed);
        break;
    default:
        break;
    }
}

void ReverbSCProcessor::getStateInformation(MemoryBlock& destData)
{
    XmlElement xml(kStateTag);
    xml.setAttribute("version", kStateVersion);
    xml.setAttribute("mix", mix.load(std::memory_order_relaxed));
    xml.setAttribute("feedback", feedback.load(std::memory_order_relaxed));
    xml.setAttribute("damping", damping.load(std::memory_order_relaxed));
    xml.setAttribute("width", width.load(std::memory_order_relaxed));
    xml.setAttribute("output", output.load(std::memory_order_relaxed));
    xml.setAttribute("editorX", editorBounds.getX());
    xml.setAttribute("editorY", editorBounds.getY());
    xml.setAttribute("editorW", editorBounds.getWidth());
    xml.setAttribute("editorH", editorBounds.getHeight());

    copyXmlToBinary(xml, destData);
}

void ReverbSCProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    if (xmlState == nullptr || !xmlState->hasTagName(kStateTag))
        return;

    setParameter(MixParam, static_cast<float>(xmlState->getDoubleAttribute("mix", 0.35)));
    setParameter(FeedbackParam, static_cast<float>(xmlState->getDoubleAttribute("feedback", 0.97)));
    setParameter(DampingParam, static_cast<float>(xmlState->getDoubleAttribute("damping", 0.4949495)));
    setParameter(WidthParam, static_cast<float>(xmlState->getDoubleAttribute("width", 1.0)));
    setParameter(OutputParam, static_cast<float>(xmlState->getDoubleAttribute("output", 0.5)));

    editorBounds.setX(xmlState->getIntAttribute("editorX"));
    editorBounds.setY(xmlState->getIntAttribute("editorY"));
    editorBounds.setWidth(xmlState->getIntAttribute("editorW"));
    editorBounds.setHeight(xmlState->getIntAttribute("editorH"));
}

float ReverbSCProcessor::normalisedToDampingHz(float value) noexcept
{
    value = clampNormalised(value);
    return kMinDampingHz + (value * (kMaxDampingHz - kMinDampingHz));
}

float ReverbSCProcessor::normalisedToOutputGain(float value) noexcept
{
    return clampNormalised(value) * 2.0f;
}

float ReverbSCProcessor::clampNormalised(float value) noexcept
{
    if (!std::isfinite(value))
        return 0.0f;

    return jlimit(0.0f, 1.0f, value);
}
