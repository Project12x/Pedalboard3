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
constexpr int kReverbSCLeftPinY = 34;
constexpr int kReverbSCRightPinY = 56;

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
        setSize(340, 190);

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
        auto area = getLocalBounds().reduced(12, 8);

        auto hero = area.removeFromTop(66);
        glyphArea = hero.removeFromLeft(66);
        hero.removeFromLeft(10);
        parameterAreas[ReverbSCProcessor::MixParam] = hero;
        area.removeFromTop(8);

        const int columnGap = 8;
        const int rowGap = 8;
        const int columnWidth = (area.getWidth() - columnGap) / 2;
        const int rowHeight = (area.getHeight() - rowGap) / 2;

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

        paintPanelLighting(g);
        paintReverbGlyph(g, glyphArea, accent, processor != nullptr ? processor->getParameter(ReverbSCProcessor::MixParam) : 0.0f);
        paintParameterLane(g, parameterAreas[ReverbSCProcessor::MixParam], ReverbSCProcessor::MixParam);
        paintParameterTile(g, parameterAreas[ReverbSCProcessor::FeedbackParam], ReverbSCProcessor::FeedbackParam);
        paintParameterTile(g, parameterAreas[ReverbSCProcessor::DampingParam], ReverbSCProcessor::DampingParam);
        paintParameterTile(g, parameterAreas[ReverbSCProcessor::WidthParam], ReverbSCProcessor::WidthParam);
        paintParameterTile(g, parameterAreas[ReverbSCProcessor::OutputParam], ReverbSCProcessor::OutputParam);

        auto footer = getLocalBounds().reduced(10, 0).removeFromBottom(5).toFloat();
        g.setColour(accent.withAlpha(0.42f));
        g.fillRoundedRectangle(footer.withHeight(2.0f), 1.0f);
    }

  private:
    ReverbSCProcessor* processor = nullptr;
    std::array<Slider, ReverbSCProcessor::NumParameters> sliders;
    std::array<Rectangle<int>, ReverbSCProcessor::NumParameters> parameterAreas;
    Rectangle<int> glyphArea;
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

    void paintPanelLighting(Graphics& g)
    {
        auto& colours = ColourScheme::getInstance().colours;
        const auto accent = colours["Graph Category Reverb"];
        auto bounds = getLocalBounds().toFloat();

        ColourGradient wash(accent.withAlpha(0.11f), bounds.getCentreX(), bounds.getY(),
                            colours["Plugin Background"].withAlpha(0.0f), bounds.getCentreX(), bounds.getBottom(), false);
        wash.addColour(0.42, accent.darker(0.40f).withAlpha(0.045f));
        wash.addColour(1.0, colours["Window Background"].darker(0.35f).withAlpha(0.03f));
        g.setGradientFill(wash);
        g.fillRect(bounds);

        g.setColour(colours["Text Colour"].withAlpha(0.045f));
        g.drawLine(bounds.getX() + 12.0f, bounds.getY() + 2.0f, bounds.getRight() - 20.0f, bounds.getY() + 2.0f,
                   1.0f);
        g.setColour(accent.withAlpha(0.11f));
        g.drawLine(bounds.getX() + 18.0f, bounds.getBottom() - 4.0f, bounds.getRight() - 18.0f,
                   bounds.getBottom() - 4.0f, 1.0f);
    }

    void paintReverbGlyph(Graphics& g, Rectangle<int> area, Colour accent, float mixValue)
    {
        if (area.isEmpty())
            return;

        auto& colours = ColourScheme::getInstance().colours;
        auto tile = area.toFloat();
        const float active = 0.58f + 0.32f * jlimit(0.0f, 1.0f, mixValue);

        ColourGradient body(colours["Plugin Background"].darker(0.06f).interpolatedWith(accent, 0.16f), tile.getX(),
                            tile.getY(), colours["Plugin Background"].darker(0.42f).interpolatedWith(accent, 0.07f),
                            tile.getX(), tile.getBottom(), false);
        body.addColour(0.36, colours["Plugin Background"].interpolatedWith(accent, 0.10f));
        body.addColour(1.0, colours["Plugin Background"].darker(0.50f).interpolatedWith(accent, 0.10f));
        g.setGradientFill(body);
        g.fillRoundedRectangle(tile, 9.0f);

        g.setColour(colours["Text Colour"].withAlpha(0.075f));
        g.drawLine(tile.getX() + 9.0f, tile.getY() + 1.5f, tile.getRight() - 9.0f, tile.getY() + 1.5f, 1.0f);
        g.setColour(accent.withAlpha(0.50f));
        g.drawRoundedRectangle(tile.reduced(0.5f), 9.0f, 1.1f);
        g.setColour(accent.withAlpha(0.12f));
        g.drawRoundedRectangle(tile.reduced(4.0f), 6.0f, 0.9f);

        const auto icon = tile.reduced(10.0f, 8.0f);
        const auto centre = icon.getCentre();

        for (int i = 0; i < 4; ++i)
        {
            const float size = icon.getWidth() * (0.30f + 0.16f * i);
            auto ring = Rectangle<float>(size, size).withCentre(centre.translated(1.5f * i, -0.7f * i));
            g.setColour(accent.withAlpha((0.10f + 0.035f * i) * active));
            g.drawEllipse(ring, 1.0f);
        }

        Path spiral;
        constexpr int kSpiralSteps = 42;
        for (int i = 0; i < kSpiralSteps; ++i)
        {
            const float t = static_cast<float>(i) / static_cast<float>(kSpiralSteps - 1);
            const float angle = MathConstants<float>::twoPi * (0.12f + 1.75f * t);
            const float radius = icon.getWidth() * (0.08f + 0.34f * t);
            const float x = centre.x + std::cos(angle) * radius;
            const float y = centre.y + std::sin(angle) * radius * 0.72f;
            if (i == 0)
                spiral.startNewSubPath(x, y);
            else
                spiral.lineTo(x, y);
        }

        g.setColour(accent.brighter(0.32f).withAlpha(0.86f * active));
        g.strokePath(spiral, PathStrokeType(2.0f, PathStrokeType::curved, PathStrokeType::rounded));

        for (int i = 0; i < 5; ++i)
        {
            const float y = icon.getY() + icon.getHeight() * (0.18f + 0.15f * i);
            const float x1 = icon.getX() + icon.getWidth() * (0.06f + 0.055f * i);
            const float x2 = icon.getX() + icon.getWidth() * (0.25f + 0.12f * i);
            g.setColour(accent.withAlpha((0.16f - 0.017f * i) * active));
            g.drawLine(x1, y, x2, y + 3.0f, 1.1f);
        }

        g.setColour(colours["Text Colour"].withAlpha(0.20f));
        g.fillEllipse(centre.x - 2.0f, centre.y - 2.0f, 4.0f, 4.0f);
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
        ColourGradient body(surface.brighter(0.08f).withAlpha(0.96f), lane.getX(), lane.getY(),
                            surface.darker(0.26f).withAlpha(0.96f), lane.getX(), lane.getBottom(), false);
        body.addColour(0.46, surface.withAlpha(0.95f));
        body.addColour(1.0, surface.darker(0.34f).interpolatedWith(accent, 0.04f).withAlpha(0.96f));
        g.setGradientFill(body);
        g.fillRoundedRectangle(lane, 7.0f);

        paintDiffusionTexture(g, area.reduced(7, 4), accent, value);

        g.setColour(colours["Text Colour"].withAlpha(0.07f));
        g.drawLine(lane.getX() + 7.0f, lane.getY() + 1.0f, lane.getRight() - 7.0f, lane.getY() + 1.0f, 1.0f);
        g.setColour(accent.withAlpha(0.50f));
        g.drawRoundedRectangle(lane.reduced(0.5f), 7.0f, 0.9f);

        auto content = area.reduced(11, 1);
        auto top = content.removeFromTop(24);
        auto labelArea = top.removeFromLeft(70);
        auto valueArea = top.removeFromRight(72).reduced(0, 4);
        auto rail = content.removeFromBottom(17).reduced(3, 4).toFloat();

        g.setFont(fonts.getBadgeFont().withHeight(11.8f));
        g.setColour(text.withAlpha(0.68f));
        g.drawText(processor->getParameterName(parameterIndex).toUpperCase(), labelArea, Justification::centredLeft, true);

        paintValueChip(g, valueArea, processor->getParameterText(parameterIndex), accent);

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
        ColourGradient body(surface.brighter(0.07f).withAlpha(0.94f), tile.getX(), tile.getY(),
                            surface.darker(0.22f).withAlpha(0.95f), tile.getX(), tile.getBottom(), false);
        body.addColour(0.52, surface.withAlpha(0.93f));
        g.setGradientFill(body);
        g.fillRoundedRectangle(tile, 6.0f);

        g.setColour(colours["Text Colour"].withAlpha(0.055f));
        g.drawLine(tile.getX() + 6.0f, tile.getY() + 1.0f, tile.getRight() - 6.0f, tile.getY() + 1.0f, 1.0f);
        g.setColour(accent.withAlpha(0.39f));
        g.drawRoundedRectangle(tile.reduced(0.5f), 6.0f, 0.8f);

        auto content = area.reduced(9, 5);
        auto top = content.removeFromTop(19);
        auto valueArea = top.removeFromRight(parameterIndex == ReverbSCProcessor::DampingParam ? 74 : 50);

        g.setFont(fonts.getBadgeFont().withHeight(10.2f));
        g.setColour(text.withAlpha(0.64f));
        g.drawText(processor->getParameterName(parameterIndex).toUpperCase(), top, Justification::centredLeft, true);

        paintValueChip(g, valueArea.reduced(0, 1), processor->getParameterText(parameterIndex), accent);

        auto rail = content.removeFromBottom(14).reduced(3, 4).toFloat();
        paintRail(g, rail, value, accent);
    }

    void paintDiffusionTexture(Graphics& g, Rectangle<int> area, Colour accent, float energy)
    {
        auto textureArea = area.toFloat();
        const float alpha = 0.065f + 0.055f * jlimit(0.0f, 1.0f, energy);

        for (int i = 0; i < 7; ++i)
        {
            const float t = static_cast<float>(i) / 6.0f;
            const float x = textureArea.getX() + textureArea.getWidth() * t;
            const float heightScale = 1.0f - std::abs(0.5f - t) * 1.15f;
            const float top = textureArea.getCentreY() - textureArea.getHeight() * (0.15f + 0.18f * heightScale);
            const float bottom = textureArea.getCentreY() + textureArea.getHeight() * (0.12f + 0.16f * heightScale);

            g.setColour(accent.withAlpha(alpha * (0.52f + 0.32f * heightScale)));
            g.drawLine(x, top, x + 9.0f, bottom, 0.75f);
        }

        Path tail;
        tail.startNewSubPath(textureArea.getX() + 64.0f, textureArea.getCentreY() + 3.0f);
        tail.cubicTo(textureArea.getX() + 108.0f, textureArea.getY() - 2.0f, textureArea.getRight() - 90.0f,
                     textureArea.getBottom() + 4.0f, textureArea.getRight() - 22.0f, textureArea.getCentreY() - 1.0f);

        g.setColour(accent.brighter(0.18f).withAlpha(alpha * 0.78f));
        g.strokePath(tail, PathStrokeType(1.05f));
    }

    void paintValueChip(Graphics& g, Rectangle<int> area, const String& valueText, Colour accent)
    {
        if (area.isEmpty())
            return;

        auto& colours = ColourScheme::getInstance().colours;
        auto& fonts = FontManager::getInstance();
        auto chip = area.toFloat().reduced(0.5f);

        ColourGradient body(colours["Window Background"].darker(0.35f).withAlpha(0.19f), chip.getX(), chip.getY(),
                            accent.darker(0.55f).withAlpha(0.25f), chip.getX(), chip.getBottom(), false);
        body.addColour(0.45, colours["Plugin Background"].darker(0.45f).interpolatedWith(accent, 0.075f).withAlpha(0.88f));
        g.setGradientFill(body);
        g.fillRoundedRectangle(chip, 5.5f);
        g.setColour(accent.withAlpha(0.42f));
        g.drawRoundedRectangle(chip, 5.5f, 0.7f);

        g.setFont(fonts.getMonoFont(jmin(12.2f, area.getHeight() * 0.78f)));
        g.setColour(colours["Text Colour"].withAlpha(0.88f));
        g.drawFittedText(valueText, area.reduced(4, 0), Justification::centred, 1);
    }

    void paintRail(Graphics& g, Rectangle<float> rail, float value, Colour accent)
    {
        if (rail.isEmpty())
            return;

        auto& colours = ColourScheme::getInstance().colours;
        value = jlimit(0.0f, 1.0f, value);
        ColourGradient bed(colours["Window Background"].darker(0.35f).withAlpha(0.34f), rail.getX(), rail.getY(),
                           colours["Text Colour"].withAlpha(0.06f), rail.getX(), rail.getBottom(), false);
        bed.addColour(0.48, colours["Field Background"].darker(0.34f).withAlpha(0.20f));
        g.setGradientFill(bed);
        g.fillRoundedRectangle(rail, 2.5f);
        g.setColour(accent.withAlpha(0.18f));
        g.drawRoundedRectangle(rail.reduced(0.35f), 2.5f, 0.7f);

        auto fill = rail.withWidth(rail.getWidth() * value);
        ColourGradient fillGradient(accent.withAlpha(0.78f), fill.getX(), fill.getCentreY(),
                                    accent.brighter(0.40f).withAlpha(0.54f), fill.getRight(), fill.getCentreY(), false);
        fillGradient.addColour(0.58, accent.brighter(0.18f).withAlpha(0.66f));
        g.setGradientFill(fillGradient);
        g.fillRoundedRectangle(fill, 2.5f);

        g.setColour(colours["Text Colour"].withAlpha(0.10f));
        g.drawLine(rail.getX() + 2.0f, rail.getY() + 1.0f, rail.getRight() - 2.0f, rail.getY() + 1.0f, 0.8f);

        const float thumbX = jlimit(rail.getX() + 2.0f, rail.getRight() - 2.0f, rail.getX() + rail.getWidth() * value);
        auto thumb = Rectangle<float>(5.0f, 11.0f).withCentre({thumbX, rail.getCentreY()});
        g.setColour(accent.withAlpha(0.18f));
        g.fillEllipse(thumb.expanded(4.0f, 3.0f));
        g.setColour(accent.brighter(0.22f).withAlpha(0.88f));
        g.fillRoundedRectangle(thumb, 2.2f);
        g.setColour(colours["Text Colour"].withAlpha(0.16f));
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

PedalboardProcessor::PinLayout ReverbSCProcessor::getInputPinLayout() const
{
    PinLayout layout;
    layout.pinY.push_back(kReverbSCLeftPinY);
    layout.pinY.push_back(kReverbSCRightPinY);
    return layout;
}

PedalboardProcessor::PinLayout ReverbSCProcessor::getOutputPinLayout() const
{
    PinLayout layout;
    layout.pinY.push_back(kReverbSCLeftPinY);
    layout.pinY.push_back(kReverbSCRightPinY);
    return layout;
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

const String ReverbSCProcessor::getInputChannelName(int channelIndex) const
{
    if (channelIndex == 0)
        return "L";
    if (channelIndex == 1)
        return "R";
    return "";
}

const String ReverbSCProcessor::getOutputChannelName(int channelIndex) const
{
    if (channelIndex == 0)
        return "L";
    if (channelIndex == 1)
        return "R";
    return "";
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
