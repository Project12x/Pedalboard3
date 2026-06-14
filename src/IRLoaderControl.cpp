/*
  ==============================================================================

    IRLoaderControl.cpp
    UI control for the IR Loader processor
    Professional styling matching NAM Loader aesthetic

  ==============================================================================
*/

#include "IRLoaderControl.h"

#include "ColourScheme.h"
#include "FontManager.h"
#include "IconManager.h"
#include "IRLoaderProcessor.h"
#include "NAMModelBrowser.h"
#include "PluginComponent.h"

namespace
{
struct IRLoaderPalette
{
    Colour top;
    Colour bottom;
    Colour face;
    Colour face2;
    Colour inset;
    Colour edge;
    Colour edgeHi;
    Colour accent;
    Colour accent2;
    Colour led;
    Colour text;
    Colour textDim;
};

IRLoaderPalette makeIRLoaderPalette()
{
    auto& scheme = ::ColourScheme::getInstance();
    auto& colours = scheme.colours;
    const auto preset = scheme.presetName;

    auto palette = [](uint32 top, uint32 bottom, uint32 face, uint32 face2, uint32 inset, uint32 edge,
                      uint32 edgeHi, uint32 accent, uint32 accent2, uint32 led, uint32 text, uint32 textDim)
    {
        return IRLoaderPalette{Colour(top),    Colour(bottom), Colour(face),  Colour(face2),
                               Colour(inset),  Colour(edge),   Colour(edgeHi), Colour(accent),
                               Colour(accent2), Colour(led),   Colour(text),  Colour(textDim)};
    };

    if (preset == "Midnight")
        return palette(0xFF211A2B, 0xFF140F1B, 0xFF271F33, 0xFF30273D, 0xFF0E0A14, 0xFF473A57,
                       0xFF5B4C6E, 0xFFFFB020, 0xFF36C8FF, 0xFF3DDC84, 0xFFF4ECDD, 0xFFB8AFC8);
    if (preset == "Deep Ocean")
        return palette(0xFF102029, 0xFF08131B, 0xFF142A36, 0xFF1B3543, 0xFF07121A, 0xFF2C5563,
                       0xFF3C6B7A, 0xFFFF9E3D, 0xFF2BD4FF, 0xFF00E0AD, 0xFFEAF3F1, 0xFFA4C5CA);
    if (preset == "Synthwave")
        return palette(0xFF1E0A28, 0xFF0F0518, 0xFF2A1139, 0xFF351747, 0xFF0C0414, 0xFF5A2D72,
                       0xFF76439A, 0xFFFF8A3D, 0xFFFF45FF, 0xFF1FFFA0, 0xFFF6EBFF, 0xFFC9A8D8);
    if (preset == "Forest")
        return palette(0xFF1C1D13, 0xFF10110A, 0xFF26281A, 0xFF2F3120, 0xFF0E0F08, 0xFF4A4D2E,
                       0xFF5F633D, 0xFFE6AD36, 0xFF79D479, 0xFF7CE87C, 0xFFF1EEDA, 0xFFBAB99B);
    if (preset == "Daylight")
        return palette(0xFF3B332A, 0xFF2B241C, 0xFF473E33, 0xFF52483B, 0xFF241F18, 0xFF615648,
                       0xFF796B58, 0xFFFFB43A, 0xFF3AA6EC, 0xFF4DDC84, 0xFFF5EDDE, 0xFFCABCA6);

    const auto accent = colours["Warning Colour"];
    const auto accent2 = colours["Audio Connection"];
    const auto text = colours["Text Colour"];
    const auto face = colours["Dialog Inner Background"].interpolatedWith(accent, 0.07f);
    const auto edge = colours["Plugin Border"].interpolatedWith(accent, 0.12f);

    return {colours["Window Background"].interpolatedWith(accent, 0.07f),
            colours["Window Background"].darker(0.18f),
            face,
            face.brighter(0.08f).interpolatedWith(accent, 0.05f),
            colours["Window Background"].darker(0.08f).interpolatedWith(accent, 0.025f),
            edge,
            edge.brighter(0.2f),
            accent,
            accent2,
            colours["Success Colour"],
            text,
            text.withAlpha(0.62f)};
}

void drawIRLoaderLed(Graphics& g, Rectangle<float> dot, Colour colour, bool active)
{
    if (active)
    {
        g.setColour(colour.withAlpha(0.2f));
        g.fillEllipse(dot.expanded(6.0f));
        g.setColour(colour.withAlpha(0.3f));
        g.fillEllipse(dot.expanded(3.0f));
    }

    ColourGradient ledGradient(colour.brighter(0.36f), dot.getX(), dot.getY(), colour.darker(0.35f), dot.getRight(),
                               dot.getBottom(), false);
    g.setGradientFill(ledGradient);
    g.fillEllipse(dot);
    g.setColour(Colours::black.withAlpha(0.46f));
    g.drawEllipse(dot, 1.0f);

    if (active)
    {
        auto spark = dot.withSizeKeepingCentre(dot.getWidth() * 0.32f, dot.getHeight() * 0.32f)
                         .translated(-dot.getWidth() * 0.16f, -dot.getHeight() * 0.16f);
        g.setColour(Colours::white.withAlpha(0.52f));
        g.fillEllipse(spark);
    }
}

void drawIRLoaderCabinetGlyph(Graphics& g, Rectangle<float> area, const IRLoaderPalette& palette, bool active)
{
    IconManager::getInstance().drawDomainGlyphTile(g, area, IconManager::DomainGlyph::Cabinet, palette.accent2, active,
                                                   5.0f);
}
} // namespace

//==============================================================================
// IRLoaderLookAndFeel Implementation
//==============================================================================
IRLoaderLookAndFeel::IRLoaderLookAndFeel()
{
    refreshColours();
}

void IRLoaderLookAndFeel::refreshColours()
{
    const auto palette = makeIRLoaderPalette();
    setColour(Slider::backgroundColourId, palette.inset);
    setColour(Slider::trackColourId, palette.accent2);
    setColour(Slider::thumbColourId, palette.text);
    setColour(TextButton::buttonColourId, palette.face2);
    setColour(TextButton::textColourOffId, palette.text);
    setColour(TextButton::textColourOnId, palette.text);
    setColour(Label::textColourId, palette.text);
}

void IRLoaderLookAndFeel::drawLinearSlider(Graphics& g, int x, int y, int width, int height, float sliderPos,
                                           float minSliderPos, float maxSliderPos, const Slider::SliderStyle style,
                                           Slider& slider)
{
    ignoreUnused(minSliderPos, maxSliderPos, slider);

    const auto palette = makeIRLoaderPalette();
    const bool isHorizontal = (style == Slider::LinearHorizontal || style == Slider::LinearBar);
    const float trackThickness = 4.0f;

    Rectangle<float> track;
    if (isHorizontal)
    {
        track = Rectangle<float>(static_cast<float>(x), y + (height - trackThickness) * 0.5f, static_cast<float>(width),
                                 trackThickness);
    }
    else
    {
        track = Rectangle<float>(x + (width - trackThickness) * 0.5f, static_cast<float>(y), trackThickness,
                                 static_cast<float>(height));
    }

    // Track background (inset effect)
    g.setColour(palette.inset.darker(0.12f));
    g.fillRoundedRectangle(track, 2.0f);
    g.setColour(Colours::black.withAlpha(0.42f));
    g.drawRoundedRectangle(track, 2.0f, 1.0f);

    // Filled portion
    Rectangle<float> filledTrack;
    if (isHorizontal)
    {
        const float fillWidth = jmax(0.0f, sliderPos - (float)x);
        filledTrack = Rectangle<float>(static_cast<float>(x), track.getY(), fillWidth, trackThickness);
    }
    else
    {
        const float fillHeight = jmax(0.0f, (float)(y + height) - sliderPos);
        filledTrack = Rectangle<float>(track.getX(), sliderPos, trackThickness, fillHeight);
    }

    if (!filledTrack.isEmpty())
    {
        ColourGradient fillGradient(palette.accent2.brighter(0.18f), filledTrack.getX(), filledTrack.getY(),
                                    palette.accent.interpolatedWith(palette.accent2, 0.28f), filledTrack.getRight(),
                                    filledTrack.getBottom(), false);
        g.setGradientFill(fillGradient);
        g.fillRoundedRectangle(filledTrack, 2.0f);
        g.setColour(Colours::white.withAlpha(0.08f));
        g.drawLine(filledTrack.getX() + 2.0f, filledTrack.getY() + 1.0f, filledTrack.getRight() - 2.0f,
                   filledTrack.getY() + 1.0f, 1.0f);
    }

    // Thumb
    const float thumbSize = 14.0f;
    float thumbX, thumbY;
    if (isHorizontal)
    {
        thumbX = sliderPos - thumbSize * 0.5f;
        thumbY = y + (height - thumbSize) * 0.5f;
    }
    else
    {
        thumbX = x + (width - thumbSize) * 0.5f;
        thumbY = sliderPos - thumbSize * 0.5f;
    }

    // Thumb shadow
    g.setColour(Colours::black.withAlpha(0.3f));
    g.fillEllipse(thumbX + 1, thumbY + 1, thumbSize, thumbSize);

    // Thumb body
    ColourGradient thumbGradient(palette.face2.brighter(0.22f), thumbX, thumbY, palette.face.darker(0.16f), thumbX,
                                 thumbY + thumbSize, false);
    g.setGradientFill(thumbGradient);
    g.fillEllipse(thumbX, thumbY, thumbSize, thumbSize);

    // Thumb highlight
    g.setColour(palette.edgeHi.withAlpha(0.78f));
    g.drawEllipse(thumbX, thumbY, thumbSize, thumbSize, 1.0f);
    g.setColour(Colours::white.withAlpha(0.12f));
    g.fillEllipse(thumbX + 4.0f, thumbY + 3.0f, 3.5f, 3.5f);
}

void IRLoaderLookAndFeel::drawButtonBackground(Graphics& g, Button& button, const Colour& backgroundColour,
                                               bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
{
    ignoreUnused(backgroundColour);

    const auto palette = makeIRLoaderPalette();
    auto bounds = button.getLocalBounds().toFloat().reduced(0.5f);

    Colour baseColour = shouldDrawButtonAsDown          ? palette.face.darker(0.16f)
                        : shouldDrawButtonAsHighlighted ? palette.face2.brighter(0.10f)
                                                        : palette.face2;

    // Button shadow
    if (!shouldDrawButtonAsDown)
    {
        g.setColour(Colours::black.withAlpha(0.28f));
        g.fillRoundedRectangle(bounds.translated(0, 1), 5.0f);
    }

    // Button body gradient
    ColourGradient buttonGradient(baseColour.brighter(0.18f), bounds.getX(), bounds.getY(),
                                  baseColour.darker(shouldDrawButtonAsDown ? 0.2f : 0.12f), bounds.getX(),
                                  bounds.getBottom(), false);
    g.setGradientFill(buttonGradient);
    g.fillRoundedRectangle(bounds, 5.0f);

    g.setColour(Colours::white.withAlpha(shouldDrawButtonAsHighlighted ? 0.12f : 0.07f));
    g.drawLine(bounds.getX() + 4.0f, bounds.getY() + 1.0f, bounds.getRight() - 4.0f, bounds.getY() + 1.0f, 1.0f);

    // Border
    g.setColour(shouldDrawButtonAsHighlighted ? palette.accent2.withAlpha(0.74f) : palette.edge.withAlpha(0.76f));
    g.drawRoundedRectangle(bounds, 5.0f, shouldDrawButtonAsHighlighted ? 1.25f : 1.0f);
}

void IRLoaderLookAndFeel::drawButtonText(Graphics& g, TextButton& button, bool shouldDrawButtonAsHighlighted,
                                         bool shouldDrawButtonAsDown)
{
    ignoreUnused(shouldDrawButtonAsDown);

    const auto palette = makeIRLoaderPalette();
    auto area = button.getLocalBounds().reduced(3, 0);
    g.setFont(FontManager::getInstance().getBadgeFont().withHeight(jlimit(8.5f, 10.5f, button.getHeight() * 0.42f)));
    g.setColour(palette.text.withAlpha(button.isEnabled() ? (shouldDrawButtonAsHighlighted ? 0.96f : 0.82f) : 0.35f));
    g.drawFittedText(button.getButtonText().toUpperCase(), area, Justification::centred, 1);
}

//==============================================================================
// IRLoaderControl Implementation
//==============================================================================
IRLoaderControl::IRLoaderControl(IRLoaderProcessor* processor) : irProcessor(processor)
{
    setLookAndFeel(&irLookAndFeel);
    auto& fonts = FontManager::getInstance();
    const auto palette = makeIRLoaderPalette();

    // Load button
    loadButton = std::make_unique<TextButton>("Load");
    loadButton->addListener(this);
    addAndMakeVisible(loadButton.get());

    // Browse button (opens IR browser window)
    browseButton = std::make_unique<TextButton>("Browse");
    browseButton->setTooltip("Browse IR Library");
    browseButton->addListener(this);
    addAndMakeVisible(browseButton.get());

    // Clear button
    clearButton = std::make_unique<TextButton>("X");
    clearButton->setTooltip("Clear IR");
    clearButton->addListener(this);
    addAndMakeVisible(clearButton.get());

    // IR name display
    irNameLabel = std::make_unique<Label>("irName", "No IR Loaded");
    irNameLabel->setJustificationType(Justification::centredLeft);
    irNameLabel->setFont(fonts.getCaptionFont());
    irNameLabel->setColour(Label::textColourId, palette.text);
    irNameLabel->setColour(Label::backgroundColourId, Colours::transparentBlack);
    irNameLabel->setColour(Label::outlineColourId, Colours::transparentBlack);
    addAndMakeVisible(irNameLabel.get());

    // IR2 controls
    loadButton2 = std::make_unique<TextButton>("Load");
    loadButton2->addListener(this);
    addAndMakeVisible(loadButton2.get());

    browseButton2 = std::make_unique<TextButton>("Browse");
    browseButton2->setTooltip("Browse IR Library (Slot 2)");
    browseButton2->addListener(this);
    addAndMakeVisible(browseButton2.get());

    clearButton2 = std::make_unique<TextButton>("X");
    clearButton2->setTooltip("Clear IR 2");
    clearButton2->addListener(this);
    addAndMakeVisible(clearButton2.get());

    irName2Label = std::make_unique<Label>("irName2", "No IR 2 Loaded");
    irName2Label->setJustificationType(Justification::centredLeft);
    irName2Label->setFont(fonts.getCaptionFont());
    irName2Label->setColour(Label::textColourId, palette.text);
    irName2Label->setColour(Label::backgroundColourId, Colours::transparentBlack);
    irName2Label->setColour(Label::outlineColourId, Colours::transparentBlack);
    addAndMakeVisible(irName2Label.get());

    // Blend slider (0 = IR1 only, 100 = IR2 only)
    blendSlider = std::make_unique<Slider>(Slider::LinearHorizontal, Slider::TextBoxRight);
    blendSlider->setRange(0.0, 100.0, 1.0);
    blendSlider->setValue(irProcessor->getBlend() * 100.0);
    blendSlider->addListener(this);
    blendSlider->setTextValueSuffix("%");
    blendSlider->setNumDecimalPlacesToDisplay(0);
    blendSlider->setTextBoxStyle(Slider::TextBoxRight, false, 45, 18);
    addAndMakeVisible(blendSlider.get());

    blendLabel = std::make_unique<Label>("blendLabel", "BLEND");
    blendLabel->setJustificationType(Justification::centredRight);
    blendLabel->setColour(Label::textColourId, palette.textDim);
    blendLabel->setFont(fonts.getBadgeFont());
    addAndMakeVisible(blendLabel.get());

    // Mix slider (0-100% display)
    mixSlider = std::make_unique<Slider>(Slider::LinearHorizontal, Slider::TextBoxRight);
    mixSlider->setRange(0.0, 100.0, 1.0);
    mixSlider->setValue(irProcessor->getMix() * 100.0);
    mixSlider->addListener(this);
    mixSlider->setTextValueSuffix("%");
    mixSlider->setNumDecimalPlacesToDisplay(0);
    mixSlider->setTextBoxStyle(Slider::TextBoxRight, false, 45, 18);
    addAndMakeVisible(mixSlider.get());

    mixLabel = std::make_unique<Label>("mixLabel", "MIX");
    mixLabel->setJustificationType(Justification::centredRight);
    mixLabel->setColour(Label::textColourId, palette.textDim);
    mixLabel->setFont(fonts.getBadgeFont());
    addAndMakeVisible(mixLabel.get());

    // Low cut slider
    lowCutSlider = std::make_unique<Slider>(Slider::LinearHorizontal, Slider::TextBoxRight);
    lowCutSlider->setRange(20.0, 500.0, 1.0);
    lowCutSlider->setValue(irProcessor->getLowCut());
    lowCutSlider->addListener(this);
    lowCutSlider->setTextValueSuffix(" Hz");
    lowCutSlider->setSkewFactorFromMidPoint(100.0);
    lowCutSlider->setTextBoxStyle(Slider::TextBoxRight, false, 55, 18);
    addAndMakeVisible(lowCutSlider.get());

    lowCutLabel = std::make_unique<Label>("lowCutLabel", "LO CUT");
    lowCutLabel->setJustificationType(Justification::centredRight);
    lowCutLabel->setColour(Label::textColourId, palette.textDim);
    lowCutLabel->setFont(fonts.getBadgeFont());
    addAndMakeVisible(lowCutLabel.get());

    // High cut slider
    highCutSlider = std::make_unique<Slider>(Slider::LinearHorizontal, Slider::TextBoxRight);
    highCutSlider->setRange(2000.0, 20000.0, 100.0);
    highCutSlider->setValue(irProcessor->getHighCut());
    highCutSlider->addListener(this);
    highCutSlider->setTextValueSuffix(" Hz");
    highCutSlider->setSkewFactorFromMidPoint(6000.0);
    highCutSlider->setTextBoxStyle(Slider::TextBoxRight, false, 55, 18);
    addAndMakeVisible(highCutSlider.get());

    highCutLabel = std::make_unique<Label>("highCutLabel", "HI CUT");
    highCutLabel->setJustificationType(Justification::centredRight);
    highCutLabel->setColour(Label::textColourId, palette.textDim);
    highCutLabel->setFont(fonts.getBadgeFont());
    addAndMakeVisible(highCutLabel.get());

    // Update display
    updateIRDisplay();
}

IRLoaderControl::~IRLoaderControl()
{
    setLookAndFeel(nullptr);
}

bool IRLoaderControl::isEmbeddedInGraphNode() const
{
    return findParentComponentOfClass<PluginComponent>() != nullptr;
}

void IRLoaderControl::paintEmbeddedGraphNode(Graphics& g, Rectangle<int> bounds)
{
    const auto palette = makeIRLoaderPalette();
    auto& fonts = FontManager::getInstance();
    auto area = bounds.reduced(8, 6);

    auto drawSectionHeader = [&](Rectangle<int> header, const String& title, Colour accent)
    {
        auto dot = Rectangle<float>(5.0f, 5.0f).withCentre({(float)header.getX() + 2.5f, (float)header.getCentreY()});
        g.setColour(accent.withAlpha(0.78f));
        g.fillEllipse(dot);
        g.setColour(accent.withAlpha(0.18f));
        g.fillEllipse(dot.expanded(3.0f));
        g.setFont(fonts.getBadgeFont().withHeight(9.2f));
        g.setColour(palette.textDim.withAlpha(0.80f));
        g.drawText(title.toUpperCase(), header.withTrimmedLeft(12), Justification::centredLeft, true);
    };

    auto drawEmbeddedStatusPill = [&](Rectangle<int> pillBounds, const String& text, bool active, Colour accent)
    {
        auto pill = pillBounds.toFloat();
        const auto fill = active ? palette.inset.interpolatedWith(accent, 0.14f) : palette.inset.darker(0.08f);
        g.setColour(fill.withAlpha(0.92f));
        g.fillRoundedRectangle(pill, 5.0f);
        g.setColour((active ? accent : palette.edge).withAlpha(active ? 0.54f : 0.34f));
        g.drawRoundedRectangle(pill.reduced(0.5f), 5.0f, 0.85f);

        auto dot = Rectangle<float>(5.5f, 5.5f).withCentre({pill.getX() + 9.0f, pill.getCentreY()});
        drawIRLoaderLed(g, dot, active ? palette.led : palette.textDim.withAlpha(0.42f), active);

        g.setFont(fonts.getBadgeFont().withHeight(8.5f));
        g.setColour((active ? palette.text : palette.textDim).withAlpha(active ? 0.86f : 0.56f));
        g.drawText(text, pillBounds.withTrimmedLeft(17), Justification::centredLeft, true);
    };

    auto drawEmbeddedIRWaveform = [&](Rectangle<int> waveBounds, bool active, float seed)
    {
        auto wave = waveBounds.toFloat();
        g.setColour(palette.inset.withAlpha(0.88f));
        g.fillRoundedRectangle(wave, 5.0f);
        g.setColour(palette.edge.withAlpha(0.38f));
        g.drawRoundedRectangle(wave.reduced(0.5f), 5.0f, 0.75f);

        if (!active)
            return;

        auto graph = wave.reduced(6.0f, 3.0f);
        Path line;
        const int points = 18;
        for (int i = 0; i < points; ++i)
        {
            const auto t = (float)i / (float)(points - 1);
            const auto x = graph.getX() + graph.getWidth() * t;
            const auto y = graph.getCentreY() +
                           std::sin((t * 7.0f + seed) * MathConstants<float>::pi) * graph.getHeight() * 0.28f +
                           std::sin((t * 19.0f + seed * 0.4f) * MathConstants<float>::pi) * graph.getHeight() * 0.13f;
            if (i == 0)
                line.startNewSubPath(x, y);
            else
                line.lineTo(x, y);
        }

        g.setColour(palette.accent2.withAlpha(0.20f));
        g.strokePath(line, PathStrokeType(4.0f, PathStrokeType::curved, PathStrokeType::rounded));
        g.setColour(palette.accent2.withAlpha(0.88f));
        g.strokePath(line, PathStrokeType(1.25f, PathStrokeType::curved, PathStrokeType::rounded));
    };

    auto drawEmbeddedSlotCard = [&](Rectangle<int> slotBounds, const String& label, const String& value,
                                    const String& badge, bool loaded, float seed)
    {
        auto slot = slotBounds.toFloat();
        const auto base = palette.inset.interpolatedWith(palette.face, 0.42f)
                              .interpolatedWith(palette.accent2, loaded ? 0.075f : 0.018f);
        g.setColour(palette.bottom.darker(0.18f).withAlpha(0.30f));
        g.fillRoundedRectangle(slot.translated(0.0f, 1.0f), 8.0f);
        ColourGradient fill(base.brighter(0.055f), slot.getX(), slot.getY(), base.darker(0.15f), slot.getX(),
                            slot.getBottom(), false);
        g.setGradientFill(fill);
        g.fillRoundedRectangle(slot, 8.0f);
        g.setColour((loaded ? palette.accent2 : palette.edge).withAlpha(loaded ? 0.50f : 0.34f));
        g.drawRoundedRectangle(slot.reduced(0.5f), 8.0f, 0.9f);
        if (loaded)
        {
            g.setColour(palette.accent.withAlpha(0.55f));
            g.fillRoundedRectangle(slot.getX() + 1.0f, slot.getY() + 5.0f, 2.5f, slot.getHeight() - 10.0f, 1.2f);
        }

        g.setFont(fonts.getBadgeFont().withHeight(9.0f));
        g.setColour(palette.textDim.withAlpha(0.70f));
        g.drawText(label.toUpperCase(), slotBounds.reduced(9, 0).removeFromTop(20), Justification::centredLeft, true);

        drawEmbeddedStatusPill(slotBounds.withTrimmedRight(7).withY(slotBounds.getY() + 5).withHeight(18)
                                   .removeFromRight(48),
                               badge, loaded, palette.accent2);

        ignoreUnused(value);

        drawEmbeddedIRWaveform(slotBounds.reduced(10, 0).withY(slotBounds.getBottom() - 26).withHeight(20), loaded,
                               seed);
    };

    auto drawControlRail = [&](Rectangle<int> row, Colour accent)
    {
        auto rail = row.reduced(48, 4).toFloat();
        g.setColour(palette.inset.withAlpha(0.72f));
        g.fillRoundedRectangle(rail, 5.0f);
        g.setColour(palette.edge.withAlpha(0.24f));
        g.drawRoundedRectangle(rail.reduced(0.5f), 5.0f, 0.7f);
        g.setColour(accent.withAlpha(0.06f));
        g.fillRoundedRectangle(rail.withHeight(rail.getHeight() * 0.42f), 5.0f);
    };

    auto drawEmbeddedXfadeReadout = [&](Rectangle<int> readoutBounds)
    {
        auto areaF = readoutBounds.toFloat();
        g.setColour(palette.inset.withAlpha(0.82f));
        g.fillRoundedRectangle(areaF, 6.0f);
        g.setColour(palette.edge.withAlpha(0.40f));
        g.drawRoundedRectangle(areaF.reduced(0.5f), 6.0f, 0.75f);
        g.setFont(fonts.getBadgeFont().withHeight(9.0f));
        g.setColour(palette.accent);
        g.drawText("A", readoutBounds.withWidth(18), Justification::centred, true);
        g.setColour(palette.accent2);
        g.drawText("B", readoutBounds.removeFromRight(18), Justification::centred, true);
        const auto blend = jlimit(0.0f, 1.0f, irProcessor->getBlend());
        const auto text = String(roundToInt((1.0f - blend) * 100.0f)) + "%  /  " + String(roundToInt(blend * 100.0f)) + "%";
        g.setFont(fonts.getMonoFont(10.0f));
        g.setColour(palette.textDim.withAlpha(0.84f));
        g.drawText(text, readoutBounds, Justification::centred, true);
    };

    auto drawEmbeddedFilterChip = [&](Rectangle<int> chipBounds, const String& label, const String& value)
    {
        auto chip = chipBounds.toFloat();
        g.setColour(palette.inset.withAlpha(0.88f));
        g.fillRoundedRectangle(chip, 6.0f);
        g.setColour(palette.edge.withAlpha(0.42f));
        g.drawRoundedRectangle(chip.reduced(0.5f), 6.0f, 0.8f);
        g.setFont(fonts.getBadgeFont().withHeight(8.5f));
        g.setColour(palette.textDim.withAlpha(0.68f));
        g.drawText(label, chipBounds.withWidth(22), Justification::centred, true);
        g.setFont(fonts.getMonoFont(10.0f));
        g.setColour(palette.text.withAlpha(0.82f));
        g.drawText(value, chipBounds.withTrimmedLeft(22), Justification::centredLeft, true);
    };

    auto drawEmbeddedFilterCurve = [&](Rectangle<int> curveBounds)
    {
        auto curve = curveBounds.toFloat();
        g.setColour(palette.inset.withAlpha(0.86f));
        g.fillRoundedRectangle(curve, 7.0f);
        g.setColour(palette.edge.withAlpha(0.38f));
        g.drawRoundedRectangle(curve.reduced(0.5f), 7.0f, 0.75f);

        auto graph = curve.reduced(9.0f, 7.0f);
        g.setColour(palette.textDim.withAlpha(0.16f));
        g.drawHorizontalLine(roundToInt(graph.getCentreY()), graph.getX(), graph.getRight());

        const auto normLog = [](float value, float lo, float hi)
        {
            value = jlimit(lo, hi, value);
            return (std::log10(value) - std::log10(lo)) / (std::log10(hi) - std::log10(lo));
        };

        const float loX = graph.getX() + graph.getWidth() * normLog(irProcessor->getLowCut(), 20.0f, 20000.0f);
        const float hiX = graph.getX() + graph.getWidth() * normLog(irProcessor->getHighCut(), 20.0f, 20000.0f);
        const float y = graph.getCentreY();

        Path response;
        response.startNewSubPath(graph.getX(), graph.getBottom() - 4.0f);
        response.cubicTo(loX - 28.0f, graph.getBottom() - 4.0f, loX - 16.0f, y, loX, y);
        response.lineTo(hiX, y);
        response.cubicTo(hiX + 16.0f, y, hiX + 28.0f, graph.getBottom() - 4.0f, graph.getRight(),
                         graph.getBottom() - 4.0f);

        g.setColour(palette.accent2.withAlpha(0.18f));
        g.strokePath(response, PathStrokeType(5.0f, PathStrokeType::curved, PathStrokeType::rounded));
        g.setColour(palette.accent2.withAlpha(0.86f));
        g.strokePath(response, PathStrokeType(1.45f, PathStrokeType::curved, PathStrokeType::rounded));

        const std::pair<float, Colour> markers[] = {{loX, palette.accent}, {hiX, palette.accent2}};
        for (const auto& marker : markers)
        {
            auto dot = Rectangle<float>(7.0f, 7.0f).withCentre({marker.first, y});
            g.setColour(marker.second.withAlpha(0.22f));
            g.fillEllipse(dot.expanded(4.0f));
            g.setColour(marker.second.withAlpha(0.90f));
            g.fillEllipse(dot);
            g.setColour(palette.edgeHi.withAlpha(0.58f));
            g.drawEllipse(dot, 0.75f);
        }
    };

    auto irSection = area.removeFromTop(190);
    drawSectionHeader(irSection.removeFromTop(16), "Impulse Responses", palette.accent2);
    irSection.removeFromTop(5);
    drawEmbeddedSlotCard(irSection.removeFromTop(82), "PRIMARY IR",
                         irProcessor->isIRLoaded() ? irProcessor->getIRName() : "No IR Loaded", "IR",
                         irProcessor->isIRLoaded(), 1.4f);
    irSection.removeFromTop(5);
    drawEmbeddedSlotCard(irSection.removeFromTop(82), "SECONDARY IR",
                         irProcessor->isIR2Loaded() ? irProcessor->getIR2Name() : "No IR 2 Loaded", "IR2",
                         irProcessor->isIR2Loaded(), 4.7f);

    area.removeFromTop(7);
    auto mixSection = area.removeFromTop(90);
    drawSectionHeader(mixSection.removeFromTop(16), "Blend", palette.accent);
    mixSection.removeFromTop(6);
    drawControlRail(mixSection.removeFromTop(18), palette.accent);
    mixSection.removeFromTop(5);
    drawControlRail(mixSection.removeFromTop(18), palette.accent2);
    mixSection.removeFromTop(5);
    drawEmbeddedXfadeReadout(mixSection.removeFromTop(24));

    area.removeFromTop(7);
    auto filterSection = area;
    drawSectionHeader(filterSection.removeFromTop(16), "Filter", palette.accent2);
    filterSection.removeFromTop(6);
    auto curve = filterSection.removeFromTop(44);
    drawEmbeddedFilterCurve(curve);
    filterSection.removeFromTop(6);
    auto chips = filterSection.removeFromTop(24);
    const auto chipW = (chips.getWidth() - 6) / 2;
    drawEmbeddedFilterChip(chips.removeFromLeft(chipW), "LO", String(roundToInt(irProcessor->getLowCut())) + " Hz");
    chips.removeFromLeft(6);
    drawEmbeddedFilterChip(chips, "HI", String(irProcessor->getHighCut() / 1000.0f, 1) + " kHz");
    filterSection.removeFromTop(6);
    drawControlRail(filterSection.removeFromTop(18), palette.accent);
    filterSection.removeFromTop(4);
    drawControlRail(filterSection.removeFromTop(18), palette.accent2);

    const bool anyLoaded = irProcessor->isIRLoaded() || irProcessor->isIR2Loaded();
    auto footer = bounds.reduced(8, 0).removeFromBottom(13);
    g.setFont(fonts.getBadgeFont());
    g.setColour((anyLoaded ? palette.accent2 : palette.textDim).withAlpha(anyLoaded ? 0.72f : 0.48f));
    g.drawText(anyLoaded ? "CABINET ACTIVE" : "NO CABINET", footer, Justification::centredRight, true);
}

//==============================================================================
void IRLoaderControl::paint(Graphics& g)
{
    irLookAndFeel.refreshColours();

    const auto palette = makeIRLoaderPalette();
    auto& fonts = FontManager::getInstance();
    auto bounds = getLocalBounds().toFloat();
    const bool embeddedInGraphNode = isEmbeddedInGraphNode();

    auto refreshChildColours = [&]()
    {
        auto styleName = [&](Label& label, bool loaded)
        {
            label.setFont(fonts.getCaptionFont());
            label.setColour(Label::textColourId, loaded ? palette.text : palette.textDim.withAlpha(0.72f));
            label.setColour(Label::backgroundColourId, Colours::transparentBlack);
            label.setColour(Label::outlineColourId, Colours::transparentBlack);
        };

        styleName(*irNameLabel, irProcessor->isIRLoaded());
        styleName(*irName2Label, irProcessor->isIR2Loaded());

        for (auto* label : {blendLabel.get(), mixLabel.get(), lowCutLabel.get(), highCutLabel.get()})
        {
            label->setFont(fonts.getBadgeFont());
            label->setColour(Label::textColourId, palette.textDim.withAlpha(0.82f));
        }

        for (auto* slider : {blendSlider.get(), mixSlider.get(), lowCutSlider.get(), highCutSlider.get()})
        {
            slider->setColour(Slider::textBoxTextColourId, palette.text.withAlpha(0.9f));
            slider->setColour(Slider::textBoxBackgroundColourId, palette.inset.withAlpha(0.9f));
            slider->setColour(Slider::textBoxOutlineColourId, palette.edge.withAlpha(0.72f));
            slider->setColour(Slider::textBoxHighlightColourId, palette.accent2.withAlpha(0.24f));
        }
    };
    refreshChildColours();

    if (embeddedInGraphNode)
    {
        paintEmbeddedGraphNode(g, getLocalBounds());
        return;
    }

    if (!embeddedInGraphNode)
    {
        // Main background with mockup-style amp faceplate depth.
        ColourGradient bgGradient(palette.top, 0, 0, palette.bottom, 0, bounds.getHeight(), false);
        bgGradient.addColour(0.45, palette.face);
        g.setGradientFill(bgGradient);
        g.fillRoundedRectangle(bounds, 6.0f);

        // Outer border with bevel effect
        g.setColour(Colours::black.withAlpha(0.46f));
        g.drawRoundedRectangle(bounds.reduced(0.5f), 6.0f, 1.0f);
        g.setColour(palette.edgeHi.withAlpha(0.52f));
        g.drawRoundedRectangle(bounds.reduced(1.5f), 5.0f, 1.0f);

        for (auto screwCentre : {Point<float>{7.0f, 7.0f},
                                 Point<float>{bounds.getRight() - 7.0f, 7.0f},
                                 Point<float>{7.0f, bounds.getBottom() - 7.0f},
                                 Point<float>{bounds.getRight() - 7.0f, bounds.getBottom() - 7.0f}})
        {
            auto screw = Rectangle<float>(5.0f, 5.0f).withCentre(screwCentre);
            g.setColour(Colours::black.withAlpha(0.3f));
            g.fillEllipse(screw.translated(0.0f, 0.7f));
            g.setColour(palette.edgeHi.withAlpha(0.46f));
            g.drawEllipse(screw, 0.8f);
        }

        // Header bar with rounded top corners
        Rectangle<float> headerBounds(2, 2, bounds.getWidth() - 4, 27);
        Path headerPath;
        headerPath.addRoundedRectangle(headerBounds.getX(), headerBounds.getY(), headerBounds.getWidth(),
                                       headerBounds.getHeight(), 5.0f, 5.0f, true, true, false, false);
        ColourGradient headerGradient(palette.face2.brighter(0.1f), 0, 2, palette.face.darker(0.12f), 0, 29, false);
        g.setGradientFill(headerGradient);
        g.fillPath(headerPath);

        // Header bottom line
        g.setColour(Colours::black.withAlpha(0.38f));
        g.drawHorizontalLine(29, 2, bounds.getWidth() - 2);
        g.setColour(palette.accent.withAlpha(0.26f));
        g.drawLine(38.0f, 28.0f, jmin(bounds.getWidth() - 52.0f, 148.0f), 28.0f, 1.0f);

        // Cabinet icon and title text.
        drawIRLoaderCabinetGlyph(g, Rectangle<float>(10.0f, 6.0f, 19.0f, 18.0f), palette,
                                 irProcessor->isIRLoaded() || irProcessor->isIR2Loaded());

        g.setFont(fonts.getBadgeFont());
        g.setColour(palette.textDim.withAlpha(0.58f));
        g.drawText("CABINET", Rectangle<float>(36.0f, 4.0f, 70.0f, 9.0f), Justification::centredLeft);
        g.setColour(palette.text);
        g.setFont(fonts.getSubheadingFont().withHeight(13.0f));
        g.drawText("IR LOADER", Rectangle<float>(36.0f, 11.0f, 116.0f, 16.0f), Justification::centredLeft);

        // Status LED (IR loaded indicator)
        const float ledSize = 8.0f;
        const float ledX = bounds.getWidth() - 18.0f;
        const float ledY = (28 - ledSize) * 0.5f;
        const bool anyLoaded = irProcessor->isIRLoaded() || irProcessor->isIR2Loaded();
        drawIRLoaderLed(g, Rectangle<float>(ledX, ledY, ledSize, ledSize), anyLoaded ? palette.led : palette.textDim,
                        anyLoaded);
    }

    auto drawSlotWell = [&](Rectangle<float> slotBounds, const String& label, bool loaded)
    {
        g.setColour(Colours::black.withAlpha(0.25f));
        g.fillRoundedRectangle(slotBounds.translated(0.0f, 1.0f), 6.0f);
        ColourGradient slotGrad(palette.face2.withAlpha(0.96f), slotBounds.getX(), slotBounds.getY(),
                                palette.inset.withAlpha(0.96f), slotBounds.getX(), slotBounds.getBottom(), false);
        g.setGradientFill(slotGrad);
        g.fillRoundedRectangle(slotBounds, 6.0f);
        g.setColour(loaded ? palette.accent2.withAlpha(0.48f) : palette.edge.withAlpha(0.42f));
        g.drawRoundedRectangle(slotBounds.reduced(0.5f), 6.0f, 1.0f);

        auto rightRail = slotBounds.removeFromRight(48.0f).reduced(5.0f, 4.0f);
        auto led = Rectangle<float>(7.5f, 7.5f).withCentre({rightRail.getX() + 8.0f, rightRail.getCentreY()});
        drawIRLoaderLed(g, led, loaded ? palette.led : palette.textDim.withAlpha(0.42f), loaded);

        g.setFont(fonts.getBadgeFont());
        g.setColour((loaded ? palette.accent2 : palette.textDim).withAlpha(loaded ? 0.88f : 0.52f));
        g.drawText(label, rightRail.withTrimmedLeft(15.0f), Justification::centredLeft, true);
    };

    const bool anyLoaded = irProcessor->isIRLoaded() || irProcessor->isIR2Loaded();
    const float slotTop = 36.0f;
    drawSlotWell(Rectangle<float>(8.0f, slotTop, bounds.getWidth() - 16.0f, 22.0f), "IR 1", irProcessor->isIRLoaded());
    drawSlotWell(Rectangle<float>(8.0f, slotTop + 26.0f, bounds.getWidth() - 16.0f, 22.0f), "IR 2",
                 irProcessor->isIR2Loaded());

    // Subtle section separator above sliders
    float separatorY = 88.0f;
    g.setColour(Colours::black.withAlpha(0.34f));
    g.drawHorizontalLine(static_cast<int>(separatorY), 8, bounds.getWidth() - 8);
    g.setColour(palette.edgeHi.withAlpha(0.22f));
    g.drawHorizontalLine(static_cast<int>(separatorY) + 1, 8, bounds.getWidth() - 8);

    const String status = irProcessor->isIRLoaded() && irProcessor->isIR2Loaded()
                              ? "DUAL CAB BLEND"
                              : irProcessor->isIRLoaded() ? "IR 1 ACTIVE"
                              : irProcessor->isIR2Loaded() ? "IR 2 ACTIVE"
                                                          : "NO CABINET LOADED";
    auto footer = bounds.removeFromBottom(18.0f).reduced(10.0f, 0.0f);
    g.setFont(fonts.getBadgeFont());
    g.setColour((anyLoaded ? palette.accent2 : palette.textDim).withAlpha(anyLoaded ? 0.72f : 0.48f));
    g.drawText(status, footer, Justification::centredRight, true);
}

void IRLoaderControl::resizedEmbeddedGraphNode(Rectangle<int> bounds)
{
    auto area = bounds.reduced(8, 6);
    constexpr int gap = 3;

    for (auto* slider : {blendSlider.get(), mixSlider.get(), lowCutSlider.get(), highCutSlider.get()})
        slider->setTextBoxStyle(Slider::TextBoxRight, false, 42, 14);
    highCutSlider->setTextBoxStyle(Slider::TextBoxRight, false, 58, 14);

    auto irSection = area.removeFromTop(190);
    irSection.removeFromTop(16);
    irSection.removeFromTop(5);

    auto row1Slot = irSection.removeFromTop(82).reduced(8, 6);
    row1Slot.removeFromTop(17);
    auto row1 = row1Slot.removeFromTop(22);
    loadButton->setBounds(row1.removeFromLeft(45));
    row1.removeFromLeft(gap);
    browseButton->setBounds(row1.removeFromLeft(55));
    row1.removeFromLeft(gap);
    clearButton->setBounds(row1.removeFromLeft(22));
    row1.removeFromLeft(gap);
    irNameLabel->setBounds(row1.removeFromTop(22));

    irSection.removeFromTop(5);
    auto row2Slot = irSection.removeFromTop(82).reduced(8, 6);
    row2Slot.removeFromTop(17);
    auto row2 = row2Slot.removeFromTop(22);
    loadButton2->setBounds(row2.removeFromLeft(45));
    row2.removeFromLeft(gap);
    browseButton2->setBounds(row2.removeFromLeft(55));
    row2.removeFromLeft(gap);
    clearButton2->setBounds(row2.removeFromLeft(22));
    row2.removeFromLeft(gap);
    irName2Label->setBounds(row2.removeFromTop(22));

    area.removeFromTop(7);
    auto mixArea = area.removeFromTop(90);
    mixArea.removeFromTop(16);
    mixArea.removeFromTop(6);

    auto blendRow = mixArea.removeFromTop(18);
    blendLabel->setBounds(blendRow.removeFromLeft(46));
    blendRow.removeFromLeft(gap);
    blendSlider->setBounds(blendRow);

    mixArea.removeFromTop(5);
    auto mixRow = mixArea.removeFromTop(18);
    mixLabel->setBounds(mixRow.removeFromLeft(46));
    mixRow.removeFromLeft(gap);
    mixSlider->setBounds(mixRow);

    area.removeFromTop(7);
    auto filterArea = area;
    filterArea.removeFromTop(16);
    filterArea.removeFromTop(6);
    filterArea.removeFromTop(44);
    filterArea.removeFromTop(6);
    filterArea.removeFromTop(24);
    filterArea.removeFromTop(6);

    auto lowRow = filterArea.removeFromTop(18);
    lowCutLabel->setBounds(lowRow.removeFromLeft(46));
    lowRow.removeFromLeft(gap);
    lowCutSlider->setBounds(lowRow);

    filterArea.removeFromTop(4);
    auto highRow = filterArea.removeFromTop(18);
    highCutLabel->setBounds(highRow.removeFromLeft(46));
    highRow.removeFromLeft(gap);
    highCutSlider->setBounds(highRow);
}

void IRLoaderControl::resized()
{
    auto bounds = getLocalBounds();
    const bool embeddedInGraphNode = isEmbeddedInGraphNode();
    if (embeddedInGraphNode)
    {
        resizedEmbeddedGraphNode(bounds);
        return;
    }

    bounds.removeFromTop(32); // Header space
    bounds = bounds.reduced(8, 4);

    const int rowHeight = 22;
    const int labelWidth = 45;
    const int clearButtonWidth = 22;
    const int spacing = 4;

    // Row 1: IR1 - Load + Browse + Clear + IR name
    auto row1 = bounds.removeFromTop(rowHeight);
    loadButton->setBounds(row1.removeFromLeft(45));
    row1.removeFromLeft(spacing);
    browseButton->setBounds(row1.removeFromLeft(55));
    row1.removeFromLeft(spacing);
    clearButton->setBounds(row1.removeFromLeft(clearButtonWidth));
    row1.removeFromLeft(spacing);
    irNameLabel->setBounds(row1);

    bounds.removeFromTop(spacing);

    // Row 2: IR2 - Load + Browse + Clear + IR name
    auto row2 = bounds.removeFromTop(rowHeight);
    loadButton2->setBounds(row2.removeFromLeft(45));
    row2.removeFromLeft(spacing);
    browseButton2->setBounds(row2.removeFromLeft(55));
    row2.removeFromLeft(spacing);
    clearButton2->setBounds(row2.removeFromLeft(clearButtonWidth));
    row2.removeFromLeft(spacing);
    irName2Label->setBounds(row2);

    bounds.removeFromTop(spacing + 2);

    // Row 3: Blend slider
    auto row3 = bounds.removeFromTop(rowHeight);
    blendLabel->setBounds(row3.removeFromLeft(labelWidth));
    row3.removeFromLeft(spacing);
    blendSlider->setBounds(row3);

    bounds.removeFromTop(spacing);

    // Row 4: Mix slider
    auto row4 = bounds.removeFromTop(rowHeight);
    mixLabel->setBounds(row4.removeFromLeft(labelWidth));
    row4.removeFromLeft(spacing);
    mixSlider->setBounds(row4);

    bounds.removeFromTop(spacing);

    // Row 5: Low cut slider
    auto row5 = bounds.removeFromTop(rowHeight);
    lowCutLabel->setBounds(row5.removeFromLeft(labelWidth));
    row5.removeFromLeft(spacing);
    lowCutSlider->setBounds(row5);

    bounds.removeFromTop(spacing);

    // Row 6: High cut slider
    auto row6 = bounds.removeFromTop(rowHeight);
    highCutLabel->setBounds(row6.removeFromLeft(labelWidth));
    row6.removeFromLeft(spacing);
    highCutSlider->setBounds(row6);
}

//==============================================================================
void IRLoaderControl::buttonClicked(Button* button)
{
    if (button == loadButton.get())
    {
        fileChooser = std::make_unique<FileChooser>("Select Impulse Response",
                                                    File::getSpecialLocation(File::userDocumentsDirectory),
                                                    "*.wav;*.aiff;*.aif", true);

        auto chooserFlags = FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles;

        fileChooser->launchAsync(chooserFlags,
                                 [this](const FileChooser& fc)
                                 {
                                     auto result = fc.getResult();
                                     if (result.existsAsFile())
                                     {
                                         irProcessor->loadIRFile(result);
                                         updateIRDisplay();
                                         repaint();
                                     }
                                 });
    }
    else if (button == loadButton2.get())
    {
        fileChooser = std::make_unique<FileChooser>("Select Impulse Response (Slot 2)",
                                                    File::getSpecialLocation(File::userDocumentsDirectory),
                                                    "*.wav;*.aiff;*.aif", true);

        auto chooserFlags = FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles;

        fileChooser->launchAsync(chooserFlags,
                                 [this](const FileChooser& fc)
                                 {
                                     auto result = fc.getResult();
                                     if (result.existsAsFile())
                                     {
                                         irProcessor->loadIRFile2(result);
                                         updateIRDisplay();
                                         repaint();
                                     }
                                 });
    }
    else if (button == browseButton.get())
    {
        IRBrowser::showWindow(
            [this](const File& irFile)
            {
                irProcessor->loadIRFile(irFile);
                updateIRDisplay();
                repaint();
            });
    }
    else if (button == browseButton2.get())
    {
        IRBrowser::showWindow(
            [this](const File& irFile)
            {
                irProcessor->loadIRFile2(irFile);
                updateIRDisplay();
                repaint();
            });
    }
    else if (button == clearButton.get())
    {
        irProcessor->loadIRFile(File());
        updateIRDisplay();
        repaint();
    }
    else if (button == clearButton2.get())
    {
        irProcessor->clearIR2();
        updateIRDisplay();
        repaint();
    }
}

void IRLoaderControl::sliderValueChanged(Slider* slider)
{
    if (slider == mixSlider.get())
    {
        irProcessor->setMix(static_cast<float>(slider->getValue() / 100.0));
    }
    else if (slider == lowCutSlider.get())
    {
        irProcessor->setLowCut(static_cast<float>(slider->getValue()));
    }
    else if (slider == highCutSlider.get())
    {
        irProcessor->setHighCut(static_cast<float>(slider->getValue()));
    }
    else if (slider == blendSlider.get())
    {
        irProcessor->setBlend(static_cast<float>(slider->getValue() / 100.0));
    }
}

//==============================================================================
void IRLoaderControl::updateIRDisplay()
{
    const auto palette = makeIRLoaderPalette();

    if (irProcessor->isIRLoaded())
    {
        irNameLabel->setText(irProcessor->getIRName(), dontSendNotification);
        irNameLabel->setColour(Label::textColourId, palette.text);
    }
    else
    {
        irNameLabel->setText("No IR Loaded", dontSendNotification);
        irNameLabel->setColour(Label::textColourId, palette.textDim.withAlpha(0.72f));
    }

    if (irProcessor->isIR2Loaded())
    {
        irName2Label->setText(irProcessor->getIR2Name(), dontSendNotification);
        irName2Label->setColour(Label::textColourId, palette.text);
    }
    else
    {
        irName2Label->setText("No IR 2 Loaded", dontSendNotification);
        irName2Label->setColour(Label::textColourId, palette.textDim.withAlpha(0.72f));
    }
}
