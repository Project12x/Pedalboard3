#include "ReverbSCProcessor.h"

#include "ColourScheme.h"
#include "FontManager.h"

#include <array>
#include <cmath>

namespace
{
constexpr const char* kStateTag = "Pedalboard3ReverbSCSettings";
constexpr int kStateVersion = 1;
constexpr float kMinDampingHz = 200.0f;
constexpr float kMaxDampingHz = 20000.0f;

constexpr double defaultValueForParameter(int parameterIndex) noexcept
{
    switch (parameterIndex)
    {
    case ReverbSCProcessor::MixParam:
        return 0.35;
    case ReverbSCProcessor::FeedbackParam:
        return 0.97;
    case ReverbSCProcessor::DampingParam:
        return 0.4949495;
    case ReverbSCProcessor::WidthParam:
        return 1.0;
    case ReverbSCProcessor::OutputParam:
        return 0.5;
    default:
        return 0.0;
    }
}

Colour colourForReverbParameter(int parameterIndex)
{
    auto& colours = ColourScheme::getInstance().colours;
    switch (parameterIndex)
    {
    case ReverbSCProcessor::MixParam:
        return colours["Graph Category Reverb"];
    case ReverbSCProcessor::FeedbackParam:
        return colours["Warning Colour"].brighter(0.05f);
    case ReverbSCProcessor::DampingParam:
        return colours["Graph Category Delay"].brighter(0.08f);
    case ReverbSCProcessor::WidthParam:
        return colours["Graph Category Reverb"].brighter(0.28f);
    case ReverbSCProcessor::OutputParam:
        return colours["Success Colour"].brighter(0.10f);
    default:
        return colours["Text Colour"];
    }
}

class ReverbSCControl final : public Component, private Timer
{
  public:
    explicit ReverbSCControl(ReverbSCProcessor* proc) : processor(proc)
    {
        setName("ReverbSC Node Controls");
        setSize(280, 146);

        for (int parameter = 0; parameter < ReverbSCProcessor::NumParameters; ++parameter)
        {
            auto& slider = sliders[static_cast<size_t>(parameter)];
            slider.setName(processor->getParameterName(parameter));
            slider.setSliderStyle(Slider::LinearHorizontal);
            slider.setTextBoxStyle(Slider::NoTextBox, false, 0, 0);
            slider.setRange(0.0, 1.0, 0.001);
            slider.setDoubleClickReturnValue(true, defaultValueForParameter(parameter));
            slider.setValue(processor->getParameter(parameter), dontSendNotification);
            slider.setMouseDragSensitivity(140);
            slider.setAlpha(0.01f);

            const int parameterIndex = parameter;
            slider.onValueChange = [this, parameterIndex]()
            {
                if (processor == nullptr || syncingFromProcessor)
                    return;

                processor->setParameter(parameterIndex,
                                        static_cast<float>(sliders[static_cast<size_t>(parameterIndex)].getValue()));
                repaint(parameterAreas[static_cast<size_t>(parameterIndex)].expanded(3, 2));
            };

            addAndMakeVisible(slider);
        }

        startTimerHz(24);
    }

    ~ReverbSCControl() override { stopTimer(); }

    void resized() override
    {
        auto area = getLocalBounds().reduced(8, 7);

        parameterAreas[ReverbSCProcessor::MixParam] = area.removeFromTop(34);
        area.removeFromTop(8);

        const int columnGap = 7;
        const int rowGap = 8;
        const int columnWidth = (area.getWidth() - columnGap) / 2;
        const int rowHeight = 41;

        auto leftColumn = area.removeFromLeft(columnWidth);
        area.removeFromLeft(columnGap);
        auto rightColumn = area.withWidth(columnWidth);

        parameterAreas[ReverbSCProcessor::FeedbackParam] = leftColumn.removeFromTop(rowHeight);
        parameterAreas[ReverbSCProcessor::DampingParam] = rightColumn.removeFromTop(rowHeight);
        leftColumn.removeFromTop(rowGap);
        rightColumn.removeFromTop(rowGap);
        parameterAreas[ReverbSCProcessor::WidthParam] = leftColumn.removeFromTop(rowHeight);
        parameterAreas[ReverbSCProcessor::OutputParam] = rightColumn.removeFromTop(rowHeight);

        for (int parameter = 0; parameter < ReverbSCProcessor::NumParameters; ++parameter)
            sliders[static_cast<size_t>(parameter)].setBounds(parameterAreas[static_cast<size_t>(parameter)].expanded(3, 2));
    }

    void paint(Graphics& g) override
    {
        auto& colours = ColourScheme::getInstance().colours;
        const auto accent = colours["Graph Category Reverb"];

        paintParameterLane(g, parameterAreas[ReverbSCProcessor::MixParam], ReverbSCProcessor::MixParam);
        paintParameterTile(g, parameterAreas[ReverbSCProcessor::FeedbackParam], ReverbSCProcessor::FeedbackParam);
        paintParameterTile(g, parameterAreas[ReverbSCProcessor::DampingParam], ReverbSCProcessor::DampingParam);
        paintParameterTile(g, parameterAreas[ReverbSCProcessor::WidthParam], ReverbSCProcessor::WidthParam);
        paintParameterTile(g, parameterAreas[ReverbSCProcessor::OutputParam], ReverbSCProcessor::OutputParam);

        auto footer = getLocalBounds().reduced(12, 0).removeFromBottom(6).toFloat();
        g.setColour(accent.withAlpha(0.42f));
        g.fillRoundedRectangle(footer.withHeight(2.0f), 1.0f);
    }

  private:
    ReverbSCProcessor* processor = nullptr;
    std::array<Slider, ReverbSCProcessor::NumParameters> sliders;
    std::array<Rectangle<int>, ReverbSCProcessor::NumParameters> parameterAreas;
    bool syncingFromProcessor = false;

    void timerCallback() override
    {
        if (processor == nullptr)
            return;

        bool needsRepaint = false;
        ScopedValueSetter<bool> syncGuard(syncingFromProcessor, true);

        for (int parameter = 0; parameter < ReverbSCProcessor::NumParameters; ++parameter)
        {
            auto& slider = sliders[static_cast<size_t>(parameter)];
            const auto currentValue = static_cast<double>(processor->getParameter(parameter));
            if (std::abs(slider.getValue() - currentValue) > 0.0005)
            {
                slider.setValue(currentValue, dontSendNotification);
                needsRepaint = true;
            }
        }

        if (needsRepaint)
            repaint();
    }

    void paintParameterLane(Graphics& g, Rectangle<int> area, int parameterIndex)
    {
        auto& colours = ColourScheme::getInstance().colours;
        auto& fonts = FontManager::getInstance();
        const auto accent = colourForReverbParameter(parameterIndex);
        const auto text = colours["Text Colour"];
        const auto surface = colours["Plugin Background"].darker(0.20f).interpolatedWith(accent, 0.045f);
        const float value = processor != nullptr ? processor->getParameter(parameterIndex) : 0.0f;

        auto lane = area.toFloat();
        g.setColour(surface.withAlpha(0.92f));
        g.fillRoundedRectangle(lane, 7.0f);
        g.setColour(accent.withAlpha(0.44f));
        g.drawRoundedRectangle(lane.reduced(0.5f), 7.0f, 0.9f);

        auto content = area.reduced(10, 0);
        auto labelArea = content.removeFromLeft(52);
        auto valueArea = content.removeFromRight(62);
        auto rail = content.reduced(5, 0).withSizeKeepingCentre(content.getWidth() - 10, 8).toFloat();

        g.setFont(fonts.getBadgeFont().withHeight(10.0f));
        g.setColour(text.withAlpha(0.62f));
        g.drawText(processor->getParameterName(parameterIndex).toUpperCase(), labelArea, Justification::centredLeft, true);

        g.setFont(fonts.getMonoFont(10.5f));
        g.setColour(text.withAlpha(0.86f));
        g.drawText(processor->getParameterText(parameterIndex), valueArea, Justification::centredRight, true);

        paintRail(g, rail, value, accent);
    }

    void paintParameterTile(Graphics& g, Rectangle<int> area, int parameterIndex)
    {
        auto& colours = ColourScheme::getInstance().colours;
        auto& fonts = FontManager::getInstance();
        const auto accent = colourForReverbParameter(parameterIndex);
        const auto text = colours["Text Colour"];
        const auto surface = colours["Plugin Background"].darker(0.25f).interpolatedWith(accent, 0.040f);
        const float value = processor != nullptr ? processor->getParameter(parameterIndex) : 0.0f;

        auto tile = area.toFloat();
        g.setColour(surface.withAlpha(0.90f));
        g.fillRoundedRectangle(tile, 6.0f);
        g.setColour(accent.withAlpha(0.36f));
        g.drawRoundedRectangle(tile.reduced(0.5f), 6.0f, 0.8f);

        auto content = area.reduced(8, 4);
        auto top = content.removeFromTop(13);
        auto valueArea = top.removeFromRight(parameterIndex == ReverbSCProcessor::DampingParam ? 68 : 45);

        g.setFont(fonts.getBadgeFont().withHeight(8.6f));
        g.setColour(text.withAlpha(0.58f));
        g.drawText(processor->getParameterName(parameterIndex).toUpperCase(), top, Justification::centredLeft, true);

        g.setFont(fonts.getMonoFont(9.0f));
        g.setColour(text.withAlpha(0.82f));
        g.drawText(processor->getParameterText(parameterIndex), valueArea, Justification::centredRight, true);

        auto rail = area.reduced(9, 0).removeFromBottom(8).withHeight(5).toFloat();
        paintRail(g, rail, value, accent);
    }

    void paintRail(Graphics& g, Rectangle<float> rail, float value, Colour accent)
    {
        if (rail.isEmpty())
            return;

        value = jlimit(0.0f, 1.0f, value);
        g.setColour(Colours::black.withAlpha(0.20f));
        g.fillRoundedRectangle(rail, 2.5f);
        g.setColour(accent.withAlpha(0.16f));
        g.drawRoundedRectangle(rail.reduced(0.35f), 2.5f, 0.7f);

        auto fill = rail.withWidth(rail.getWidth() * value);
        ColourGradient fillGradient(accent.withAlpha(0.78f), fill.getX(), fill.getCentreY(),
                                    accent.brighter(0.40f).withAlpha(0.54f), fill.getRight(), fill.getCentreY(), false);
        g.setGradientFill(fillGradient);
        g.fillRoundedRectangle(fill, 2.5f);

        const float thumbX = jlimit(rail.getX() + 2.0f, rail.getRight() - 2.0f, rail.getX() + rail.getWidth() * value);
        auto thumb = Rectangle<float>(5.0f, 11.0f).withCentre({thumbX, rail.getCentreY()});
        g.setColour(accent.brighter(0.22f).withAlpha(0.88f));
        g.fillRoundedRectangle(thumb, 2.2f);
        g.setColour(Colours::white.withAlpha(0.16f));
        g.drawRoundedRectangle(thumb.reduced(0.5f), 2.2f, 0.6f);
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ReverbSCControl)
};
} // namespace

ReverbSCProcessor::ReverbSCProcessor()
{
    setPlayConfigDetails(2, 2, 44100.0, 512);
}

Component* ReverbSCProcessor::getControls()
{
    return new ReverbSCControl(this);
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
