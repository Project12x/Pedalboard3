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
#include "IRLoaderProcessor.h"
#include "NAMModelBrowser.h"

#include <array>

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
    g.setColour(palette.accent2.withAlpha(active ? 0.16f : 0.08f));
    g.fillRoundedRectangle(area, 5.0f);
    g.setColour(palette.edgeHi.withAlpha(0.54f));
    g.drawRoundedRectangle(area.reduced(0.5f), 5.0f, 1.0f);

    const auto coneSize = jmin(area.getWidth(), area.getHeight()) * 0.28f;
    const std::array<Point<float>, 4> centres = {
        Point<float>{area.getX() + area.getWidth() * 0.33f, area.getY() + area.getHeight() * 0.34f},
        Point<float>{area.getX() + area.getWidth() * 0.67f, area.getY() + area.getHeight() * 0.34f},
        Point<float>{area.getX() + area.getWidth() * 0.33f, area.getY() + area.getHeight() * 0.70f},
        Point<float>{area.getX() + area.getWidth() * 0.67f, area.getY() + area.getHeight() * 0.70f}};

    for (const auto& centre : centres)
    {
        const auto cone = Rectangle<float>(coneSize, coneSize).withCentre(centre);
        g.setColour(palette.accent2.withAlpha(active ? 0.84f : 0.54f));
        g.drawEllipse(cone, 1.1f);
        g.setColour(palette.accent2.withAlpha(active ? 0.18f : 0.08f));
        g.fillEllipse(cone.reduced(coneSize * 0.28f));
    }
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

//==============================================================================
void IRLoaderControl::paint(Graphics& g)
{
    irLookAndFeel.refreshColours();

    const auto palette = makeIRLoaderPalette();
    auto& fonts = FontManager::getInstance();
    auto bounds = getLocalBounds().toFloat();

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

    drawSlotWell(Rectangle<float>(8.0f, 36.0f, bounds.getWidth() - 16.0f, 22.0f), "IR 1", irProcessor->isIRLoaded());
    drawSlotWell(Rectangle<float>(8.0f, 62.0f, bounds.getWidth() - 16.0f, 22.0f), "IR 2", irProcessor->isIR2Loaded());

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

void IRLoaderControl::resized()
{
    auto bounds = getLocalBounds();
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
