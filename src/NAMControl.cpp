/*
  ==============================================================================

    NAMControl.cpp
    UI control for the NAM (Neural Amp Modeler) processor
    Professional amp-style interface with theme-complementary colours

  ==============================================================================
*/

#include "NAMControl.h"

#include "NAMModelBrowser.h"
#include "NAMProcessor.h"
#include "PluginComponent.h"
#include "SubGraphProcessor.h"

namespace
{
constexpr int kEmbeddedParamEqSingleRowDeckHeight = 88;
constexpr int kEmbeddedParamEqDoubleRowDeckHeight = 128;
constexpr int kStandaloneParamEqSingleRowDeckHeight = 118;
constexpr int kStandaloneParamEqDoubleRowDeckHeight = 160;
}

//==============================================================================
// NAMLookAndFeel Implementation
//==============================================================================
NAMLookAndFeel::NAMLookAndFeel()
{
    refreshColours();
}

void NAMLookAndFeel::refreshColours()
{
    auto& cs = ::ColourScheme::getInstance();
    const auto preset = cs.presetName;

    auto setBrowserPalette = [&](uint32 top, uint32 bottom, uint32 face, uint32 face2, uint32 inset, uint32 edge,
                                 uint32 edge2, uint32 accent, uint32 accent2, uint32 led, uint32 text)
    {
        ampBackgroundTop = Colour(top);
        ampBackground = Colour(bottom);
        ampSurface = Colour(face);
        ampBorder = Colour(edge);
        ampBorderBright = Colour(edge2);
        ampHeaderBg = Colour(face2);
        ampAccent = Colour(accent);
        ampAccentSecondary = Colour(accent2);
        ampTextBright = Colour(text);
        ampTextDim = Colour(text).withAlpha(0.68f);
        ampLedOn = Colour(led);
        ampLedOff = Colour(text).withAlpha(0.42f);
        ampKnobBody = Colour(inset).brighter(0.08f);
        ampKnobRing = Colour(edge).brighter(0.42f);
        ampTrackBg = Colour(inset).darker(0.12f);
        ampButtonBg = Colour(face2);
        ampButtonHover = Colour(face2).brighter(0.14f);
        ampInsetBg = Colour(inset);
    };

    if (preset == "Midnight")
        setBrowserPalette(0xFF211A2B, 0xFF140F1B, 0xFF271F33, 0xFF30273D, 0xFF0E0A14, 0xFF473A57,
                          0xFF5B4C6E, 0xFFFFB020, 0xFF36C8FF, 0xFF3DDC84, 0xFFF4ECDD);
    else if (preset == "Deep Ocean")
        setBrowserPalette(0xFF102029, 0xFF08131B, 0xFF142A36, 0xFF1B3543, 0xFF07121A, 0xFF2C5563,
                          0xFF3C6B7A, 0xFFFF9E3D, 0xFF2BD4FF, 0xFF00E0AD, 0xFFEAF3F1);
    else if (preset == "Synthwave")
        setBrowserPalette(0xFF1E0A28, 0xFF0F0518, 0xFF2A1139, 0xFF351747, 0xFF0C0414, 0xFF5A2D72,
                          0xFF76439A, 0xFFFF8A3D, 0xFFFF45FF, 0xFF1FFFA0, 0xFFF6EBFF);
    else if (preset == "Forest")
        setBrowserPalette(0xFF1C1D13, 0xFF10110A, 0xFF26281A, 0xFF2F3120, 0xFF0E0F08, 0xFF4A4D2E,
                          0xFF5F633D, 0xFFE6AD36, 0xFF79D479, 0xFF7CE87C, 0xFFF1EEDA);
    else if (preset == "Daylight")
        setBrowserPalette(0xFF3B332A, 0xFF2B241C, 0xFF473E33, 0xFF52483B, 0xFF241F18, 0xFF615648,
                          0xFF796B58, 0xFFFFB43A, 0xFF3AA6EC, 0xFF4DDC84, 0xFFF5EDDE);
    else
    {
        Colour pluginBg = cs.colours["Plugin Background"];
        Colour pluginBorder = cs.colours["Plugin Border"];
        Colour textCol = cs.colours["Text Colour"];
        Colour paramCol = cs.colours["Audio Connection"];
        Colour warnCol = cs.colours["Warning Colour"];
        Colour fieldBg = cs.colours["Field Background"];

        ampBackgroundTop = pluginBg.interpolatedWith(warnCol, 0.08f);
        ampBackground = pluginBg.darker(0.35f);
        ampSurface = pluginBg.interpolatedWith(fieldBg, 0.22f);
        ampBorder = pluginBorder.interpolatedWith(warnCol, 0.16f);
        ampBorderBright = ampBorder.brighter(0.16f);
        ampHeaderBg = pluginBg.interpolatedWith(warnCol, 0.16f);
        ampAccent = warnCol;
        ampAccentSecondary = paramCol;
        ampTextBright = textCol;
        ampTextDim = textCol.withAlpha(0.6f);
        ampLedOn = cs.colours["Success Colour"].brighter(0.4f);
        ampLedOff = fieldBg.interpolatedWith(pluginBg, 0.55f).darker(0.25f);
        ampKnobBody = pluginBg.darker(0.15f);
        ampKnobRing = pluginBorder.interpolatedWith(textCol, 0.28f);
        ampTrackBg = ampBackground.darker(0.4f);
        ampButtonBg = cs.colours["Button Colour"];
        ampButtonHover = cs.colours["Button Highlight"];
        ampInsetBg = fieldBg.interpolatedWith(pluginBg, 0.35f);
    }

    // Apply to JUCE colour IDs
    setColour(Slider::backgroundColourId, ampTrackBg);
    setColour(Slider::trackColourId, ampAccentSecondary); // Theme slider colour
    setColour(Slider::thumbColourId, ampTextBright);
    setColour(Slider::textBoxTextColourId, ampTextBright);
    setColour(Slider::textBoxBackgroundColourId, ampInsetBg);
    setColour(Slider::textBoxOutlineColourId, ampBorder);
    setColour(TextButton::buttonColourId, ampButtonBg);
    setColour(TextButton::textColourOffId, ampTextBright);
    setColour(TextButton::textColourOnId, ampAccent);
    setColour(ToggleButton::textColourId, ampTextBright);
    setColour(ToggleButton::tickColourId, ampAccent);
    setColour(Label::textColourId, ampTextBright);
    setColour(Label::backgroundColourId, Colours::transparentBlack);
}

void NAMLookAndFeel::drawRotarySlider(Graphics& g, int x, int y, int width, int height, float sliderPos,
                                      float rotaryStartAngle, float rotaryEndAngle, Slider& slider)
{
    const float radius = jmin(width / 2.0f, height / 2.0f) - 8.0f;
    const float centreX = x + width * 0.5f;
    const float centreY = y + height * 0.5f;
    const float rx = centreX - radius;
    const float ry = centreY - radius;
    const float rw = radius * 2.0f;
    const float angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

    // Drop shadow
    g.setColour(Colours::black.withAlpha(0.35f));
    g.fillEllipse(rx - 1, ry + 2, rw + 2, rw + 2);

    // Value arc background (full range, dimmed)
    Path bgArc;
    bgArc.addCentredArc(centreX, centreY, radius + 6.0f, radius + 6.0f, 0.0f, rotaryStartAngle, rotaryEndAngle, true);
    g.setColour(ampTrackBg.brighter(0.05f));
    g.strokePath(bgArc, PathStrokeType(3.5f, PathStrokeType::curved, PathStrokeType::rounded));

    // Value arc (filled segment showing current position)
    Path valueArc;
    valueArc.addCentredArc(centreX, centreY, radius + 6.0f, radius + 6.0f, 0.0f, rotaryStartAngle, angle, true);
    g.setColour(ampAccent);
    g.strokePath(valueArc, PathStrokeType(3.5f, PathStrokeType::curved, PathStrokeType::rounded));

    // Value arc glow
    g.setColour(ampAccent.withAlpha(0.2f));
    g.strokePath(valueArc, PathStrokeType(8.0f, PathStrokeType::curved, PathStrokeType::rounded));

    // Outer metallic ring
    ColourGradient outerGradient(ampKnobRing.brighter(0.25f), centreX, centreY - radius, ampKnobRing.darker(0.15f),
                                 centreX, centreY + radius, false);
    g.setGradientFill(outerGradient);
    g.fillEllipse(rx - 2.5f, ry - 2.5f, rw + 5, rw + 5);

    // Outer ring border
    g.setColour(ampBorder.darker(0.3f));
    g.drawEllipse(rx - 2.5f, ry - 2.5f, rw + 5, rw + 5, 0.75f);

    // Main knob body
    ColourGradient knobGradient(ampKnobBody.brighter(0.15f), centreX, centreY - radius, ampKnobBody.darker(0.35f),
                                centreX, centreY + radius, false);
    g.setGradientFill(knobGradient);
    g.fillEllipse(rx, ry, rw, rw);

    // Inner recess (concave look)
    const float innerRadius = radius * 0.6f;
    ColourGradient innerGradient(ampKnobBody.darker(0.25f), centreX, centreY - innerRadius, ampKnobBody.darker(0.55f),
                                 centreX, centreY + innerRadius, false);
    g.setGradientFill(innerGradient);
    g.fillEllipse(centreX - innerRadius, centreY - innerRadius, innerRadius * 2.0f, innerRadius * 2.0f);

    // Pointer indicator
    Path p;
    const float pointerLength = radius * 0.5f;
    const float pointerThickness = 3.5f;
    p.addRoundedRectangle(-pointerThickness * 0.5f, -pointerLength, pointerThickness, pointerLength, 1.5f);
    p.applyTransform(AffineTransform::rotation(angle).translated(centreX, centreY));

    // Pointer glow
    g.setColour(ampAccent.withAlpha(0.3f));
    Path pGlow;
    pGlow.addRoundedRectangle(-pointerThickness, -pointerLength - 1, pointerThickness * 2, pointerLength + 1, 2.0f);
    pGlow.applyTransform(AffineTransform::rotation(angle).translated(centreX, centreY));
    g.fillPath(pGlow);

    g.setColour(ampAccent);
    g.fillPath(p);

    // Tick marks
    g.setColour(ampTextDim.withAlpha(0.35f));
    const int numTicks = 11;
    for (int i = 0; i < numTicks; ++i)
    {
        const float tickAngle = rotaryStartAngle + (float)i / (numTicks - 1) * (rotaryEndAngle - rotaryStartAngle);
        const float tickInnerRadius = radius + 11.0f;
        const float tickOuterRadius = radius + 15.0f;

        Point<float> innerPoint(centreX + tickInnerRadius * std::sin(tickAngle),
                                centreY - tickInnerRadius * std::cos(tickAngle));
        Point<float> outerPoint(centreX + tickOuterRadius * std::sin(tickAngle),
                                centreY - tickOuterRadius * std::cos(tickAngle));

        g.drawLine(innerPoint.x, innerPoint.y, outerPoint.x, outerPoint.y, 1.0f);
    }
}

void NAMLookAndFeel::drawLinearSlider(Graphics& g, int x, int y, int width, int height, float sliderPos,
                                      float minSliderPos, float maxSliderPos, const Slider::SliderStyle style,
                                      Slider& slider)
{
    const bool isHorizontal = (style == Slider::LinearHorizontal || style == Slider::LinearBar);
    const float trackThickness = 6.0f;

    Rectangle<float> track;
    if (isHorizontal)
    {
        track = Rectangle<float>(x, y + (height - trackThickness) * 0.5f, width, trackThickness);
    }
    else
    {
        track = Rectangle<float>(x + (width - trackThickness) * 0.5f, y, trackThickness, height);
    }

    // Track background (inset with subtle inner shadow)
    g.setColour(ampTrackBg);
    g.fillRoundedRectangle(track, 3.0f);
    // Inner shadow on track
    ColourGradient trackShadow(Colours::black.withAlpha(0.15f), track.getX(), track.getY(), Colours::transparentBlack,
                               track.getX(), track.getY() + 3.0f, false);
    g.setGradientFill(trackShadow);
    g.fillRoundedRectangle(track, 3.0f);
    g.setColour(ampBorder.darker(0.2f));
    g.drawRoundedRectangle(track, 3.0f, 0.75f);

    // Filled portion with accent colour
    Rectangle<float> filledTrack;
    if (isHorizontal)
    {
        const float fillWidth = sliderPos - x;
        filledTrack = Rectangle<float>(x, track.getY(), fillWidth, trackThickness);
    }
    else
    {
        const float fillHeight = (y + height) - sliderPos;
        filledTrack = Rectangle<float>(track.getX(), sliderPos, trackThickness, fillHeight);
    }

    // Warm-to-cool amp rail: yellow/orange attack into explicit blue edge glow.
    const auto railBlue = ampAccentSecondary.interpolatedWith(Colour(0xFF2BD4FF), 0.72f).brighter(0.10f);
    ColourGradient fillGradient(ampAccent, filledTrack.getX(), filledTrack.getY(), railBlue, filledTrack.getRight(),
                                filledTrack.getBottom(), false);
    fillGradient.addColour(0.58, ampAccent.interpolatedWith(railBlue, 0.36f).brighter(0.08f));
    g.setGradientFill(fillGradient);
    g.fillRoundedRectangle(filledTrack, 3.0f);

    // Filled track glow
    g.setColour(ampAccentSecondary.withAlpha(0.12f));
    g.fillRoundedRectangle(filledTrack.expanded(0, 2.0f), 3.0f);

    // Thumb
    const float thumbSize = 18.0f;
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

    // Thumb body with metallic gradient
    ColourGradient thumbGradient(ampKnobRing.brighter(0.2f), thumbX, thumbY, ampKnobRing.darker(0.15f), thumbX,
                                 thumbY + thumbSize, false);
    g.setGradientFill(thumbGradient);
    g.fillEllipse(thumbX, thumbY, thumbSize, thumbSize);

    // Thumb centre dot
    g.setColour(ampAccent.withAlpha(0.6f));
    const float dotSize = 4.0f;
    g.fillEllipse(thumbX + (thumbSize - dotSize) * 0.5f, thumbY + (thumbSize - dotSize) * 0.5f, dotSize, dotSize);

    // Thumb rim
    g.setColour(ampBorder.brighter(0.4f));
    g.drawEllipse(thumbX, thumbY, thumbSize, thumbSize, 1.0f);
}

void NAMLookAndFeel::drawToggleButton(Graphics& g, ToggleButton& button, bool shouldDrawButtonAsHighlighted,
                                      bool shouldDrawButtonAsDown)
{
    const int width = button.getWidth();
    const int height = button.getHeight();
    const float ledSize = 12.0f;
    const float ledX = 4.0f;
    const float ledY = (height - ledSize) * 0.5f;

    // LED glow when on
    Colour ledColour = button.getToggleState() ? ampLedOn : ampLedOff;

    // Cast shadow beneath LED (always visible)
    g.setColour(Colours::black.withAlpha(0.2f));
    g.fillEllipse(ledX + 1.0f, ledY + 2.0f, ledSize, ledSize);

    if (button.getToggleState())
    {
        // Outer glow
        g.setColour(ledColour.withAlpha(0.15f));
        g.fillEllipse(ledX - 6, ledY - 6, ledSize + 12, ledSize + 12);
        g.setColour(ledColour.withAlpha(0.25f));
        g.fillEllipse(ledX - 3, ledY - 3, ledSize + 6, ledSize + 6);
        g.setColour(ledColour.withAlpha(0.4f));
        g.fillEllipse(ledX - 1, ledY - 1, ledSize + 2, ledSize + 2);
    }

    // LED body
    ColourGradient ledGradient(ledColour.brighter(0.3f), ledX, ledY, ledColour.darker(0.2f), ledX, ledY + ledSize,
                               false);
    g.setGradientFill(ledGradient);
    g.fillEllipse(ledX, ledY, ledSize, ledSize);

    // LED specular highlight
    if (button.getToggleState())
    {
        g.setColour(ledColour.contrasting(0.96f).withAlpha(0.2f));
        g.fillEllipse(ledX + 2, ledY + 1, ledSize * 0.4f, ledSize * 0.3f);
    }

    // LED rim
    g.setColour(ampBorder.darker(0.1f));
    g.drawEllipse(ledX, ledY, ledSize, ledSize, 1.0f);

    // Text
    auto& fm = FontManager::getInstance();
    g.setColour(button.getToggleState() ? ampTextBright : ampTextDim);
    g.setFont(fm.getLabelFont());
    g.drawText(button.getButtonText(),
               Rectangle<int>((int)(ledX + ledSize + 5), 0, width - (int)(ledX + ledSize + 5), height),
               Justification::centredLeft);
}

void NAMLookAndFeel::drawButtonBackground(Graphics& g, Button& button, const Colour& backgroundColour,
                                          bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
{
    auto bounds = button.getLocalBounds().toFloat().reduced(0.5f);

    // Use per-button colour if explicitly set, otherwise fall back to amp default
    Colour btnCol = (backgroundColour != ampButtonBg && backgroundColour != Colour()) ? backgroundColour : ampButtonBg;

    Colour baseColour = shouldDrawButtonAsDown          ? btnCol.darker(0.3f)
                        : shouldDrawButtonAsHighlighted ? btnCol.brighter(0.15f)
                                                        : btnCol;
    const auto sheenColour = baseColour.contrasting(0.96f);

    // Drop shadow (deeper)
    g.setColour(Colours::black.withAlpha(0.15f));
    g.fillRoundedRectangle(bounds.translated(0, 2.5f), 5.0f);
    g.setColour(Colours::black.withAlpha(0.3f));
    g.fillRoundedRectangle(bounds.translated(0, 1.5f), 4.0f);

    // Body gradient (more pronounced)
    ColourGradient buttonGradient(baseColour.brighter(0.18f), bounds.getX(), bounds.getY(), baseColour.darker(0.18f),
                                  bounds.getX(), bounds.getBottom(), false);
    g.setGradientFill(buttonGradient);
    g.fillRoundedRectangle(bounds, 4.0f);

    // Top highlight bevel (stronger)
    g.setColour(sheenColour.withAlpha(0.09f));
    g.fillRoundedRectangle(bounds.removeFromTop(bounds.getHeight() * 0.42f), 4.0f);

    // Inner shadow at bottom (embossed inset effect)
    auto innerShadowBounds = button.getLocalBounds().toFloat().reduced(1.0f);
    ColourGradient innerShadow(Colours::transparentBlack, innerShadowBounds.getX(), innerShadowBounds.getY(),
                               Colours::black.withAlpha(0.08f), innerShadowBounds.getX(), innerShadowBounds.getBottom(),
                               false);
    g.setGradientFill(innerShadow);
    g.fillRoundedRectangle(innerShadowBounds, 4.0f);

    // Border -- accent on hover, otherwise subtle
    if (shouldDrawButtonAsHighlighted)
    {
        g.setColour(ampAccent.withAlpha(0.6f));
        g.drawRoundedRectangle(button.getLocalBounds().toFloat().reduced(0.5f), 4.0f, 1.5f);
    }
    else
    {
        g.setColour(ampBorder.brighter(0.25f));
        g.drawRoundedRectangle(button.getLocalBounds().toFloat().reduced(0.5f), 4.0f, 1.0f);
    }
}

void NAMLookAndFeel::drawComboBox(Graphics& g, int width, int height, bool isButtonDown, int buttonX, int buttonY,
                                  int buttonW, int buttonH, ComboBox& box)
{
    auto bounds = Rectangle<float>(0, 0, (float)width, (float)height);
    float cornerRadius = 4.0f;

    // Body fill — match button style
    Colour baseCol = isButtonDown ? ampButtonBg.darker(0.2f) : ampButtonBg;
    ColourGradient bodyGrad(baseCol.brighter(0.12f), 0, 0, baseCol.darker(0.1f), 0, (float)height, false);
    g.setGradientFill(bodyGrad);
    g.fillRoundedRectangle(bounds, cornerRadius);

    // Border
    bool focused = box.hasKeyboardFocus(false);
    g.setColour(focused ? ampAccent.withAlpha(0.6f) : ampBorder.brighter(0.15f));
    g.drawRoundedRectangle(bounds.reduced(0.5f), cornerRadius, 1.0f);

    // Dropdown arrow — small chevron on the right
    float arrowX = (float)(buttonX + buttonW / 2);
    float arrowY = (float)(height / 2);
    float arrowSize = 5.0f;
    float offset = isButtonDown ? 1.0f : 0.0f;

    Path arrow;
    arrow.startNewSubPath(arrowX - arrowSize + offset, arrowY - arrowSize * 0.4f + offset);
    arrow.lineTo(arrowX + offset, arrowY + arrowSize * 0.4f + offset);
    arrow.lineTo(arrowX + arrowSize + offset, arrowY - arrowSize * 0.4f + offset);

    g.setColour(ampTextDim);
    g.strokePath(arrow, PathStrokeType(1.5f, PathStrokeType::curved, PathStrokeType::rounded));
}

Label* NAMLookAndFeel::createSliderTextBox(Slider& slider)
{
    auto* label = LookAndFeel_V4::createSliderTextBox(slider);
    auto& fm = FontManager::getInstance();
    label->setFont(fm.getMonoFont(14.0f));

    // LCD-style colours: dark recessed background, bright accent text
    label->setColour(Label::backgroundColourId, ampInsetBg);
    label->setColour(Label::textColourId, ampAccent);
    label->setColour(Label::outlineColourId, Colours::transparentBlack);
    label->setColour(Label::textWhenEditingColourId, ampTextBright);
    label->setColour(Label::backgroundWhenEditingColourId, ampInsetBg.darker(0.15f));
    label->setColour(Label::outlineWhenEditingColourId, ampAccent.withAlpha(0.4f));
    label->setJustificationType(Justification::centred);

    return label;
}

Font NAMLookAndFeel::getTextButtonFont(TextButton& /*button*/, int buttonHeight)
{
    auto& fm = FontManager::getInstance();
    return fm.getUIFont(jmin(15.0f, (float)buttonHeight * 0.55f + 2.0f));
}

void NAMLookAndFeel::drawLabel(Graphics& g, Label& label)
{
    // Check if this label belongs to a slider (value display)
    bool isSliderTextBox = (dynamic_cast<Slider*>(label.getParentComponent()) != nullptr);
    const bool isModelChip = label.getName() == "modelName" || label.getName() == "irName" ||
                             label.getName() == "ir2Name";
    const bool isArchChip = label.getName() == "modelArch";

    if (isSliderTextBox)
    {
        auto bounds = label.getLocalBounds().toFloat();

        // Recessed inset background
        g.setColour(ampInsetBg);
        g.fillRoundedRectangle(bounds, 3.0f);

        // Top inner shadow (recessed depth)
        ColourGradient insetShadow(Colours::black.withAlpha(0.18f), bounds.getX(), bounds.getY(),
                                   Colours::transparentBlack, bounds.getX(), bounds.getY() + 4.0f, false);
        g.setGradientFill(insetShadow);
        g.fillRoundedRectangle(bounds, 3.0f);

        // Bottom edge accent glow (subtle LCD backlight feel)
        g.setColour(ampAccent.withAlpha(0.06f));
        g.fillRoundedRectangle(bounds.getX(), bounds.getBottom() - 2.0f, bounds.getWidth(), 2.0f, 1.0f);

        // Inset border
        g.setColour(Colours::black.withAlpha(0.3f));
        g.drawRoundedRectangle(bounds.reduced(0.5f), 3.0f, 1.0f);

        // Draw text
        if (!label.isBeingEdited())
        {
            auto textColour = label.findColour(Label::textColourId);
            g.setColour(textColour);
            g.setFont(label.getFont());
            g.drawText(label.getText(), bounds.reduced(2, 0), label.getJustificationType(), false);
        }
    }
    else if (isModelChip || isArchChip)
    {
        auto bounds = label.getLocalBounds().toFloat().reduced(0.5f);
        const bool loaded = !label.getText().startsWithIgnoreCase("No ") && label.getText().isNotEmpty();
        const auto accent = isArchChip ? ampAccent : (loaded ? ampAccent : ampBorder);
        const auto bg = loaded ? ampInsetBg.interpolatedWith(ampAccent, 0.08f) : ampInsetBg;

        ColourGradient chipFill(bg.brighter(0.05f), bounds.getX(), bounds.getY(), bg.darker(0.12f), bounds.getX(),
                                bounds.getBottom(), false);
        g.setGradientFill(chipFill);
        g.fillRoundedRectangle(bounds, 5.0f);

        g.setColour(Colours::black.withAlpha(0.14f));
        g.drawLine(bounds.getX() + 5.0f, bounds.getY() + 1.0f, bounds.getRight() - 5.0f, bounds.getY() + 1.0f, 1.0f);

        if (loaded)
        {
            g.setColour(accent.withAlpha(0.20f));
            g.fillRoundedRectangle(bounds.getX() + 1.5f, bounds.getY() + 4.0f, 3.0f, bounds.getHeight() - 8.0f,
                                   1.5f);
        }

        g.setColour(accent.withAlpha(loaded || isArchChip ? 0.58f : 0.28f));
        g.drawRoundedRectangle(bounds, 5.0f, loaded || isArchChip ? 1.1f : 0.8f);

        if (!label.isBeingEdited())
        {
            g.setColour(loaded || isArchChip ? ampTextBright : ampTextDim);
            g.setFont(label.getFont());
            if (isArchChip)
                g.drawText(label.getText(), bounds.reduced(2.0f, 0.0f), label.getJustificationType(), true);
            else
                g.drawText(label.getText(), bounds.reduced(8.0f, 0.0f), label.getJustificationType(), true);
        }
    }
    else
    {
        // Default label rendering for non-slider labels
        LookAndFeel_V4::drawLabel(g, label);
    }
}

//==============================================================================
// NAMControl Implementation
//==============================================================================
NAMControl::NAMControl(NAMProcessor* processor) : namProcessor(processor)
{
    setLookAndFeel(&namLookAndFeel);

    auto& fm = FontManager::getInstance();

    // Model loading section
    loadModelButton = std::make_unique<TextButton>("Load Model");
    loadModelButton->addListener(this);
    addAndMakeVisible(loadModelButton.get());

    browseModelsButton = std::make_unique<TextButton>("Browse...");
    browseModelsButton->setTooltip("Browse NAM Models Online");
    browseModelsButton->addListener(this);
    addAndMakeVisible(browseModelsButton.get());

    clearModelButton = std::make_unique<TextButton>("X");
    clearModelButton->setTooltip("Clear Model");
    clearModelButton->addListener(this);
    addAndMakeVisible(clearModelButton.get());

    modelNameLabel = std::make_unique<Label>("modelName", "No Model Loaded");
    modelNameLabel->setJustificationType(Justification::centredLeft);
    modelNameLabel->setFont(fm.getBodyFont().withHeight(fm.getBodyFont().getHeight() + 1.0f));
    addAndMakeVisible(modelNameLabel.get());

    // Architecture badge
    modelArchLabel = std::make_unique<Label>("modelArch", "");
    modelArchLabel->setJustificationType(Justification::centred);
    modelArchLabel->setFont(fm.getBadgeFont().withHeight(fm.getBadgeFont().getHeight() + 2.0f));
    addAndMakeVisible(modelArchLabel.get());

    // A2 slimmable model size. Hidden unless the loaded model supports it.
    slimmableSizeSlider = std::make_unique<Slider>(Slider::LinearHorizontal, Slider::TextBoxRight);
    slimmableSizeSlider->setRange(0.0, 1.0, 0.01);
    slimmableSizeSlider->setValue(namProcessor->getSlimmableSize(), dontSendNotification);
    slimmableSizeSlider->setTextBoxStyle(Slider::TextBoxRight, false, 45, 20);
    slimmableSizeSlider->setTooltip("NAM A2 model size");
    slimmableSizeSlider->addListener(this);
    addAndMakeVisible(slimmableSizeSlider.get());

    slimmableSizeLabel = std::make_unique<Label>("slimmableSizeLabel", "SIZE");
    slimmableSizeLabel->setJustificationType(Justification::centredRight);
    slimmableSizeLabel->setFont(fm.getCaptionFont().withHeight(fm.getCaptionFont().getHeight() + 2.0f));
    addAndMakeVisible(slimmableSizeLabel.get());

    // IR loading section
    cabinetIrCollapseButton = std::make_unique<TextButton>("IR -");
    cabinetIrCollapseButton->setTooltip("Collapse or expand the cabinet IR section");
    cabinetIrCollapseButton->addListener(this);
    cabinetIrCollapseButton->setVisible(false);
    addAndMakeVisible(cabinetIrCollapseButton.get());

    loadIRButton = std::make_unique<TextButton>("Load IR");
    loadIRButton->addListener(this);
    addAndMakeVisible(loadIRButton.get());

    clearIRButton = std::make_unique<TextButton>("X");
    clearIRButton->setTooltip("Clear IR");
    clearIRButton->addListener(this);
    addAndMakeVisible(clearIRButton.get());

    irNameLabel = std::make_unique<Label>("irName", "No IR Loaded");
    irNameLabel->setJustificationType(Justification::centredLeft);
    irNameLabel->setFont(fm.getBodyFont().withHeight(fm.getBodyFont().getHeight() + 1.0f));
    addAndMakeVisible(irNameLabel.get());

    irEnabledButton = std::make_unique<ToggleButton>("IR");
    irEnabledButton->setToggleState(namProcessor->isIREnabled(), dontSendNotification);
    irEnabledButton->addListener(this);
    addAndMakeVisible(irEnabledButton.get());

    // IR2 loading section (second cabinet slot)
    loadIR2Button = std::make_unique<TextButton>("Load IR2");
    loadIR2Button->addListener(this);
    addAndMakeVisible(loadIR2Button.get());

    clearIR2Button = std::make_unique<TextButton>("X");
    clearIR2Button->setTooltip("Clear IR2");
    clearIR2Button->addListener(this);
    addAndMakeVisible(clearIR2Button.get());

    ir2NameLabel = std::make_unique<Label>("ir2Name", "No IR2 Loaded");
    ir2NameLabel->setJustificationType(Justification::centredLeft);
    ir2NameLabel->setFont(fm.getBodyFont().withHeight(fm.getBodyFont().getHeight() + 1.0f));
    addAndMakeVisible(ir2NameLabel.get());

    ir2EnabledButton = std::make_unique<ToggleButton>("IR2");
    ir2EnabledButton->setToggleState(namProcessor->isIR2Enabled(), dontSendNotification);
    ir2EnabledButton->addListener(this);
    addAndMakeVisible(ir2EnabledButton.get());

    // IR blend slider
    irBlendSlider = std::make_unique<Slider>(Slider::LinearHorizontal, Slider::TextBoxRight);
    irBlendSlider->setRange(0.0, 1.0, 0.01);
    irBlendSlider->setValue(namProcessor->getIRBlend());
    irBlendSlider->addListener(this);
    irBlendSlider->setTextBoxStyle(Slider::TextBoxRight, false, 45, 20);
    addAndMakeVisible(irBlendSlider.get());

    irBlendLabel = std::make_unique<Label>("blendLabel", "BLEND");
    irBlendLabel->setJustificationType(Justification::centredRight);
    irBlendLabel->setFont(fm.getCaptionFont().withHeight(fm.getCaptionFont().getHeight() + 2.0f));
    addAndMakeVisible(irBlendLabel.get());

    // IR filters
    irLowCutSlider = std::make_unique<Slider>(Slider::LinearHorizontal, Slider::TextBoxRight);
    irLowCutSlider->setRange(20.0, 500.0, 1.0);
    irLowCutSlider->setValue(namProcessor->getIRLowCut());
    irLowCutSlider->setSkewFactorFromMidPoint(100.0);
    irLowCutSlider->addListener(this);
    irLowCutSlider->setTextValueSuffix(" Hz");
    irLowCutSlider->setTextBoxStyle(Slider::TextBoxRight, false, 60, 20);
    addAndMakeVisible(irLowCutSlider.get());

    irLowCutLabel = std::make_unique<Label>("lowCutLabel", "LO CUT");
    irLowCutLabel->setJustificationType(Justification::centredRight);
    irLowCutLabel->setFont(fm.getCaptionFont().withHeight(fm.getCaptionFont().getHeight() + 2.0f));
    addAndMakeVisible(irLowCutLabel.get());

    irHighCutSlider = std::make_unique<Slider>(Slider::LinearHorizontal, Slider::TextBoxRight);
    irHighCutSlider->setRange(2000.0, 20000.0, 100.0);
    irHighCutSlider->setValue(namProcessor->getIRHighCut());
    irHighCutSlider->setSkewFactorFromMidPoint(8000.0);
    irHighCutSlider->addListener(this);
    irHighCutSlider->setTextValueSuffix(" Hz");
    irHighCutSlider->setTextBoxStyle(Slider::TextBoxRight, false, 70, 20);
    addAndMakeVisible(irHighCutSlider.get());

    irHighCutLabel = std::make_unique<Label>("highCutLabel", "HI CUT");
    irHighCutLabel->setJustificationType(Justification::centredRight);
    irHighCutLabel->setFont(fm.getCaptionFont().withHeight(fm.getCaptionFont().getHeight() + 2.0f));
    addAndMakeVisible(irHighCutLabel.get());

    // Effects loop controls
    fxLoopEnabledButton = std::make_unique<ToggleButton>("FX Loop");
    fxLoopEnabledButton->setToggleState(namProcessor->isEffectsLoopEnabled(), dontSendNotification);
    fxLoopEnabledButton->addListener(this);
    addAndMakeVisible(fxLoopEnabledButton.get());

    editFxLoopButton = std::make_unique<TextButton>("Edit FX...");
    editFxLoopButton->setTooltip("Edit Effects Loop");
    editFxLoopButton->addListener(this);
    addAndMakeVisible(editFxLoopButton.get());

    // Input gain slider
    inputGainSlider = std::make_unique<Slider>(Slider::LinearHorizontal, Slider::TextBoxRight);
    inputGainSlider->setRange(-20.0, 20.0, 0.1);
    inputGainSlider->setValue(namProcessor->getInputGain());
    inputGainSlider->addListener(this);
    inputGainSlider->setTextValueSuffix(" dB");
    inputGainSlider->setTextBoxStyle(Slider::TextBoxRight, false, 65, 20);
    addAndMakeVisible(inputGainSlider.get());

    inputGainLabel = std::make_unique<Label>("inputLabel", "INPUT");
    inputGainLabel->setJustificationType(Justification::centredRight);
    inputGainLabel->setFont(fm.getCaptionFont().withHeight(fm.getCaptionFont().getHeight() + 2.0f));
    addAndMakeVisible(inputGainLabel.get());

    // Output gain slider
    outputGainSlider = std::make_unique<Slider>(Slider::LinearHorizontal, Slider::TextBoxRight);
    outputGainSlider->setRange(-40.0, 40.0, 0.1);
    outputGainSlider->setValue(namProcessor->getOutputGain());
    outputGainSlider->addListener(this);
    outputGainSlider->setTextValueSuffix(" dB");
    outputGainSlider->setTextBoxStyle(Slider::TextBoxRight, false, 65, 20);
    addAndMakeVisible(outputGainSlider.get());

    outputGainLabel = std::make_unique<Label>("outputLabel", "OUTPUT");
    outputGainLabel->setJustificationType(Justification::centredRight);
    outputGainLabel->setFont(fm.getCaptionFont().withHeight(fm.getCaptionFont().getHeight() + 2.0f));
    addAndMakeVisible(outputGainLabel.get());

    // Noise gate slider
    noiseGateSlider = std::make_unique<Slider>(Slider::LinearHorizontal, Slider::TextBoxRight);
    noiseGateSlider->setRange(-101.0, 0.0, 1.0);
    noiseGateSlider->setValue(namProcessor->getNoiseGateThreshold());
    noiseGateSlider->addListener(this);
    noiseGateSlider->setTextValueSuffix(" dB");
    noiseGateSlider->setTextBoxStyle(Slider::TextBoxRight, false, 65, 20);
    addAndMakeVisible(noiseGateSlider.get());

    noiseGateLabel = std::make_unique<Label>("gateLabel", "GATE");
    noiseGateLabel->setJustificationType(Justification::centredRight);
    noiseGateLabel->setFont(fm.getCaptionFont().withHeight(fm.getCaptionFont().getHeight() + 2.0f));
    addAndMakeVisible(noiseGateLabel.get());

    // Tone stack
    toneStackEnabledButton = std::make_unique<ToggleButton>("EQ");
    toneStackEnabledButton->setToggleState(namProcessor->isToneStackEnabled(), dontSendNotification);
    toneStackEnabledButton->addListener(this);
    addAndMakeVisible(toneStackEnabledButton.get());

    toneStackPreButton = std::make_unique<TextButton>(namProcessor->isToneStackPre() ? "PRE" : "POST");
    toneStackPreButton->setTooltip("EQ position: PRE (before amp model) / POST (after amp model)");
    toneStackPreButton->addListener(this);
    addAndMakeVisible(toneStackPreButton.get());

    toneEqModeStackButton = std::make_unique<TextButton>("STACK");
    toneEqModeStackButton->setTooltip("Use the original NAM bass/mid/treble tone stack");
    toneEqModeStackButton->setClickingTogglesState(false);
    toneEqModeStackButton->addListener(this);
    addAndMakeVisible(toneEqModeStackButton.get());

    toneEqModeParamButton = std::make_unique<TextButton>("PARAM");
    toneEqModeParamButton->setTooltip("Use the additive four-band parametric EQ");
    toneEqModeParamButton->setClickingTogglesState(false);
    toneEqModeParamButton->addListener(this);
    addAndMakeVisible(toneEqModeParamButton.get());

    paramEqBandCountButton = std::make_unique<TextButton>(String(namProcessor->getActiveParamEqBandCount()) + "B");
    paramEqBandCountButton->setTooltip("Cycle parametric EQ band count: 4, 8, 10, 12");
    paramEqBandCountButton->addListener(this);
    addAndMakeVisible(paramEqBandCountButton.get());

    // Bass knob
    bassSlider = std::make_unique<Slider>(Slider::RotaryHorizontalVerticalDrag, Slider::NoTextBox);
    bassSlider->setRange(0.0, 10.0, 0.1);
    bassSlider->setValue(namProcessor->getBass());
    bassSlider->addListener(this);
    bassSlider->setRotaryParameters(MathConstants<float>::pi * 1.25f, MathConstants<float>::pi * 2.75f, true);
    addAndMakeVisible(bassSlider.get());

    bassLabel = std::make_unique<Label>("bassLabel", "BASS");
    bassLabel->setJustificationType(Justification::centred);
    bassLabel->setFont(fm.getCaptionFont().withHeight(fm.getCaptionFont().getHeight() + 2.0f));
    addAndMakeVisible(bassLabel.get());

    // Mid knob
    midSlider = std::make_unique<Slider>(Slider::RotaryHorizontalVerticalDrag, Slider::NoTextBox);
    midSlider->setRange(0.0, 10.0, 0.1);
    midSlider->setValue(namProcessor->getMid());
    midSlider->addListener(this);
    midSlider->setRotaryParameters(MathConstants<float>::pi * 1.25f, MathConstants<float>::pi * 2.75f, true);
    addAndMakeVisible(midSlider.get());

    midLabel = std::make_unique<Label>("midLabel", "MID");
    midLabel->setJustificationType(Justification::centred);
    midLabel->setFont(fm.getCaptionFont().withHeight(fm.getCaptionFont().getHeight() + 2.0f));
    addAndMakeVisible(midLabel.get());

    // Treble knob
    trebleSlider = std::make_unique<Slider>(Slider::RotaryHorizontalVerticalDrag, Slider::NoTextBox);
    trebleSlider->setRange(0.0, 10.0, 0.1);
    trebleSlider->setValue(namProcessor->getTreble());
    trebleSlider->addListener(this);
    trebleSlider->setRotaryParameters(MathConstants<float>::pi * 1.25f, MathConstants<float>::pi * 2.75f, true);
    addAndMakeVisible(trebleSlider.get());

    trebleLabel = std::make_unique<Label>("trebleLabel", "TREBLE");
    trebleLabel->setJustificationType(Justification::centred);
    trebleLabel->setFont(fm.getCaptionFont().withHeight(fm.getCaptionFont().getHeight() + 2.0f));
    addAndMakeVisible(trebleLabel.get());

    for (int band = 0; band < NAMProcessor::kParamEqBandCount; ++band)
    {
        paramEqBandLabels[band] = std::make_unique<Label>("paramEqBand" + String(band + 1), "B" + String(band + 1));
        paramEqBandLabels[band]->setJustificationType(Justification::centredRight);
        paramEqBandLabels[band]->setFont(fm.getCaptionFont().withHeight(fm.getCaptionFont().getHeight() + 2.0f));
        addAndMakeVisible(paramEqBandLabels[band].get());

        paramEqFrequencySliders[band] = std::make_unique<Slider>(Slider::LinearVertical, Slider::TextBoxBelow);
        paramEqFrequencySliders[band]->setRange(20.0, 20000.0, 1.0);
        paramEqFrequencySliders[band]->setSkewFactorFromMidPoint(800.0);
        paramEqFrequencySliders[band]->setValue(namProcessor->getParamEqBandFrequency(band));
        paramEqFrequencySliders[band]->setTextValueSuffix(" Hz");
        paramEqFrequencySliders[band]->addListener(this);
        addAndMakeVisible(paramEqFrequencySliders[band].get());

        paramEqGainSliders[band] = std::make_unique<Slider>(Slider::LinearVertical, Slider::TextBoxBelow);
        paramEqGainSliders[band]->setRange(-18.0, 18.0, 0.1);
        paramEqGainSliders[band]->setValue(namProcessor->getParamEqBandGain(band));
        paramEqGainSliders[band]->setTextValueSuffix(" dB");
        paramEqGainSliders[band]->addListener(this);
        addAndMakeVisible(paramEqGainSliders[band].get());

        paramEqQSliders[band] = std::make_unique<Slider>(Slider::LinearVertical, Slider::TextBoxBelow);
        paramEqQSliders[band]->setRange(0.1, 10.0, 0.01);
        paramEqQSliders[band]->setSkewFactorFromMidPoint(1.0);
        paramEqQSliders[band]->setValue(namProcessor->getParamEqBandQ(band));
        paramEqQSliders[band]->addListener(this);
        addAndMakeVisible(paramEqQSliders[band].get());
    }

    // Normalize button
    normalizeButton = std::make_unique<ToggleButton>("Normalize");
    normalizeButton->setToggleState(namProcessor->isNormalizeOutput(), dontSendNotification);
    normalizeButton->addListener(this);
    addAndMakeVisible(normalizeButton.get());

    // Apply theme colours and update displays
    refreshColours();
    updateModelDisplay();
    updateIRDisplay();
    updateEqModeVisibility();
    updateSlimmableControlState();

    // Start LED pulse timer (30fps)
    startTimerHz(30);
}

NAMControl::~NAMControl()
{
    stopTimer();
    setLookAndFeel(nullptr);
}

bool NAMControl::isEmbeddedInGraphNode() const
{
    return findParentComponentOfClass<PluginComponent>() != nullptr;
}

void NAMControl::refreshColours()
{
    namLookAndFeel.refreshColours();
    auto& laf = namLookAndFeel;

    // Apply to model/IR display labels
    const auto loadedChipBg = laf.ampInsetBg.interpolatedWith(laf.ampAccent, 0.06f);
    const auto loadedChipOutline = laf.ampAccent.withAlpha(0.32f);
    const auto emptyChipOutline = laf.ampBorder.withAlpha(0.75f);

    modelNameLabel->setColour(Label::backgroundColourId, namProcessor->isModelLoaded() ? loadedChipBg : laf.ampInsetBg);
    modelNameLabel->setColour(Label::outlineColourId, namProcessor->isModelLoaded() ? loadedChipOutline : emptyChipOutline);
    irNameLabel->setColour(Label::backgroundColourId, namProcessor->isIRLoaded() ? loadedChipBg : laf.ampInsetBg);
    irNameLabel->setColour(Label::outlineColourId, namProcessor->isIRLoaded() ? loadedChipOutline : emptyChipOutline);
    ir2NameLabel->setColour(Label::backgroundColourId, namProcessor->isIR2Loaded() ? loadedChipBg : laf.ampInsetBg);
    ir2NameLabel->setColour(Label::outlineColourId, namProcessor->isIR2Loaded() ? loadedChipOutline : emptyChipOutline);

    // Architecture badge
    modelArchLabel->setColour(Label::backgroundColourId, laf.ampAccent.withAlpha(0.18f));
    modelArchLabel->setColour(Label::outlineColourId, laf.ampAccent.withAlpha(0.42f));
    modelArchLabel->setColour(Label::textColourId, laf.ampAccent);

    // Dim labels
    irLowCutLabel->setColour(Label::textColourId, laf.ampTextDim);
    irHighCutLabel->setColour(Label::textColourId, laf.ampTextDim);
    irBlendLabel->setColour(Label::textColourId, laf.ampTextDim);
    slimmableSizeLabel->setColour(Label::textColourId, laf.ampTextDim);
    inputGainLabel->setColour(Label::textColourId, laf.ampTextDim);
    outputGainLabel->setColour(Label::textColourId, laf.ampTextDim);
    noiseGateLabel->setColour(Label::textColourId, laf.ampTextDim);
    bassLabel->setColour(Label::textColourId, laf.ampTextDim);
    midLabel->setColour(Label::textColourId, laf.ampTextDim);
    trebleLabel->setColour(Label::textColourId, laf.ampTextDim);
    for (auto& label : paramEqBandLabels)
        label->setColour(Label::textColourId, laf.ampTextDim);

    repaint();
}

void NAMControl::updateEqModeVisibility()
{
    const bool controlsVisible = !collapsed;
    const bool parametric = controlsVisible && namProcessor->getToneEqMode() == NAMProcessor::ToneEqMode::Parametric;
    const bool stack = controlsVisible && !parametric;
    const bool embedded = isEmbeddedInGraphNode();

    toneEqModeStackButton->setToggleState(!parametric, dontSendNotification);
    toneEqModeParamButton->setToggleState(parametric, dontSendNotification);
    toneEqModeStackButton->setButtonText(parametric ? "PARAM" : "STACK");
    paramEqBandCountButton->setButtonText(String(namProcessor->getActiveParamEqBandCount()) + "B");

    toneEqModeStackButton->setVisible(controlsVisible);
    toneEqModeParamButton->setVisible(controlsVisible && !embedded);
    paramEqBandCountButton->setVisible(controlsVisible && parametric);

    bassSlider->setVisible(!parametric && stack);
    bassLabel->setVisible(!parametric && stack);
    midSlider->setVisible(!parametric && stack);
    midLabel->setVisible(!parametric && stack);
    trebleSlider->setVisible(!parametric && stack);
    trebleLabel->setVisible(!parametric && stack);

    for (int band = 0; band < NAMProcessor::kParamEqBandCount; ++band)
    {
        const bool visible = parametric && band < namProcessor->getActiveParamEqBandCount();
        paramEqBandLabels[band]->setVisible(visible);
        paramEqFrequencySliders[band]->setVisible(visible);
        paramEqGainSliders[band]->setVisible(visible);
        paramEqQSliders[band]->setVisible(visible);
    }
}

void NAMControl::setCollapsed(bool shouldCollapse)
{
    if (collapsed == shouldCollapse)
        return;

    collapsed = shouldCollapse;
    namProcessor->setEditorCollapsed(collapsed);

    // Show/hide all child controls
    for (auto* child : getChildren())
        child->setVisible(!collapsed);
    updateEqModeVisibility();
    updateSlimmableControlState();

    // Tell the parent PluginComponent to re-query getSize() and resize the node
    if (auto* pc = dynamic_cast<PluginComponent*>(getParentComponent()))
        pc->updateNodeSize();

    resized();
    repaint();
}

void NAMControl::timerCallback()
{
    if (namProcessor->isModelLoaded())
    {
        ledPulsePhase += 0.06f;
        if (ledPulsePhase > MathConstants<float>::twoPi)
            ledPulsePhase -= MathConstants<float>::twoPi;

        // Only repaint the header area for LED animation
        repaint(0, 0, getWidth(), 36);
    }
}

void NAMControl::mouseDown(const MouseEvent& event)
{
    // Click in header area toggles collapse
    if (!isEmbeddedInGraphNode() && event.y < 40)
        setCollapsed(!collapsed);
}

void NAMControl::paintEmbeddedGraphNode(Graphics& g, Rectangle<int> bounds)
{
    auto& laf = namLookAndFeel;
    auto& fm = FontManager::getInstance();
    cabinetIrCollapsed = namProcessor->isEmbeddedCabinetIrCollapsed();

    auto area = bounds.reduced(3, 2);

    auto drawSectionHeader = [&](Rectangle<int> header, const String& title, Colour accent)
    {
        auto dot = Rectangle<float>(7.0f, 7.0f).withCentre({(float)header.getX() + 3.5f, (float)header.getCentreY()});
        g.setColour(accent.withAlpha(0.24f));
        g.fillEllipse(dot.expanded(4.0f));
        g.setColour(accent.withAlpha(0.92f));
        g.fillEllipse(dot);

        g.setFont(fm.getBadgeFont().withHeight(12.5f));
        g.setColour(laf.ampTextDim.withAlpha(0.86f));
        g.drawText(title.toUpperCase(), header.withTrimmedLeft(14), Justification::centredLeft, true);
    };

    auto drawInsetField = [&](Rectangle<int> fieldBounds, bool active, Colour accent)
    {
        auto field = fieldBounds.toFloat();
        const auto fillBase = laf.ampInsetBg.interpolatedWith(accent, active ? 0.060f : 0.012f);
        ColourGradient fill(fillBase.brighter(0.035f), field.getX(), field.getY(), fillBase.darker(0.16f),
                            field.getX(), field.getBottom(), false);
        g.setGradientFill(fill);
        g.fillRoundedRectangle(field, 7.0f);
        g.setColour((active ? accent : laf.ampBorder).withAlpha(active ? 0.48f : 0.30f));
        g.drawRoundedRectangle(field.reduced(0.5f), 7.0f, active ? 1.0f : 0.75f);
        if (active)
        {
            g.setColour(accent.withAlpha(0.42f));
            g.fillRoundedRectangle(field.getX() + 1.0f, field.getY() + 4.0f, 2.4f, field.getHeight() - 8.0f, 1.2f);
        }
    };

    auto drawMockupPill = [&](Rectangle<int> pillBounds, const String& text, Colour accent, bool dot)
    {
        auto pill = pillBounds.toFloat();
        g.setColour(accent.withAlpha(0.16f));
        g.fillRoundedRectangle(pill, 5.0f);
        g.setColour(accent.withAlpha(0.58f));
        g.drawRoundedRectangle(pill.reduced(0.5f), 5.0f, 0.9f);
        auto textArea = pillBounds;
        if (dot)
        {
            auto led = Rectangle<float>(5.5f, 5.5f).withCentre({pill.getX() + 10.0f, pill.getCentreY()});
            g.setColour(accent.withAlpha(0.22f));
            g.fillEllipse(led.expanded(3.0f));
            g.setColour(accent.withAlpha(0.94f));
            g.fillEllipse(led);
            textArea.removeFromLeft(17);
        }
        g.setFont(fm.getBadgeFont().withHeight(10.6f));
        g.setColour(accent.brighter(0.05f));
        g.drawText(text, textArea, Justification::centred, true);
    };

    auto drawEmbeddedSlotCard = [&](Rectangle<int> slotBounds, const String& label, const String& value,
                                    const String& badge, bool active, Colour accent)
    {
        auto slot = slotBounds.toFloat();
        const auto fillBase = laf.ampInsetBg.interpolatedWith(laf.ampSurface, 0.40f)
                                  .interpolatedWith(accent, active ? 0.070f : 0.018f);
        g.setColour(laf.ampBackground.darker(0.18f).withAlpha(0.32f));
        g.fillRoundedRectangle(slot.translated(0.0f, 1.5f), 7.0f);
        ColourGradient fill(fillBase.brighter(0.055f), slot.getX(), slot.getY(), fillBase.darker(0.14f), slot.getX(),
                            slot.getBottom(), false);
        g.setGradientFill(fill);
        g.fillRoundedRectangle(slot, 9.0f);
        g.setColour((active ? accent : laf.ampBorder).withAlpha(active ? 0.50f : 0.34f));
        g.drawRoundedRectangle(slot.reduced(0.5f), 9.0f, 1.0f);
        g.setColour(laf.ampTextBright.withAlpha(0.04f));
        g.drawRoundedRectangle(slot.reduced(2.0f), 7.0f, 0.7f);
        if (active)
        {
            g.setColour(accent.withAlpha(0.52f));
            g.fillRoundedRectangle(slot.getX() + 1.5f, slot.getY() + 8.0f, 2.8f, slot.getHeight() - 16.0f, 1.4f);
        }

        g.setFont(fm.getBadgeFont().withHeight(11.0f));
        g.setColour(laf.ampTextDim.withAlpha(0.72f));
        g.drawText(label.toUpperCase(), slotBounds.reduced(10, 0).removeFromTop(20), Justification::centredLeft, true);
        ignoreUnused(value, badge);
    };

    auto drawControlRail = [&](Rectangle<int> row, Colour accent)
    {
        ignoreUnused(row, accent);
    };

    auto drawToneModeSegment = [&](Rectangle<int> segmentBounds)
    {
        auto segment = segmentBounds.toFloat();
        g.setColour(laf.ampInsetBg.withAlpha(0.84f));
        g.fillRoundedRectangle(segment, 6.0f);
        g.setColour(laf.ampBorder.withAlpha(0.46f));
        g.drawRoundedRectangle(segment.reduced(0.5f), 6.0f, 0.85f);

        const bool parametric = namProcessor->getToneEqMode() == NAMProcessor::ToneEqMode::Parametric;
        auto stack = segmentBounds.removeFromLeft(segmentBounds.getWidth() / 2);
        auto param = segmentBounds;
        auto selected = parametric ? param : stack;
        auto accent = parametric ? laf.ampAccentSecondary : laf.ampAccent;
        g.setColour(accent.withAlpha(0.18f));
        g.fillRoundedRectangle(selected.reduced(3, 3).toFloat(), 5.0f);
        g.setColour(accent.withAlpha(0.76f));
        g.drawRoundedRectangle(selected.reduced(3, 3).toFloat().reduced(0.5f), 5.0f, 1.0f);
        g.setColour(laf.ampBorder.withAlpha(0.48f));
        g.drawVerticalLine(stack.getRight(), (float)stack.getY() + 4.0f, (float)stack.getBottom() - 4.0f);
    };

    auto drawEmbeddedToneCurve = [&](Rectangle<int> curveBounds)
    {
        auto curve = curveBounds.toFloat();
        g.setColour(laf.ampInsetBg.withAlpha(0.88f));
        g.fillRoundedRectangle(curve, 7.0f);
        g.setColour(laf.ampBorder.withAlpha(0.42f));
        g.drawRoundedRectangle(curve.reduced(0.5f), 7.0f, 0.8f);

        auto graph = curve.reduced(9.0f, 7.0f);
        const float centreY = graph.getCentreY();
        const float amp = graph.getHeight() * 0.46f;
        const auto norm = [](float v) { return jlimit(0.0f, 1.0f, v / 10.0f); };
        const float bass = norm(namProcessor->getBass());
        const float mid = norm(namProcessor->getMid());
        const float treble = norm(namProcessor->getTreble());
        const float yBass = centreY - (bass - 0.5f) * amp;
        const float yMid = centreY - (mid - 0.5f) * amp;
        const float yTreble = centreY - (treble - 0.5f) * amp;

        Path fillPath;
        fillPath.startNewSubPath(graph.getX(), graph.getBottom());
        fillPath.lineTo(graph.getX(), yBass);
        fillPath.cubicTo(graph.getX() + graph.getWidth() * 0.22f, yBass,
                         graph.getX() + graph.getWidth() * 0.34f, yMid,
                         graph.getX() + graph.getWidth() * 0.50f, yMid);
        fillPath.cubicTo(graph.getX() + graph.getWidth() * 0.66f, yMid,
                         graph.getX() + graph.getWidth() * 0.78f, yTreble, graph.getRight(), yTreble);
        fillPath.lineTo(graph.getRight(), graph.getBottom());
        fillPath.closeSubPath();
        ColourGradient fill(laf.ampAccentSecondary.withAlpha(0.18f), graph.getCentreX(), graph.getY(),
                            Colours::transparentBlack, graph.getCentreX(), graph.getBottom(), false);
        g.setGradientFill(fill);
        g.fillPath(fillPath);

        Path line;
        line.startNewSubPath(graph.getX(), yBass);
        line.cubicTo(graph.getX() + graph.getWidth() * 0.22f, yBass,
                     graph.getX() + graph.getWidth() * 0.34f, yMid, graph.getX() + graph.getWidth() * 0.50f,
                     yMid);
        line.cubicTo(graph.getX() + graph.getWidth() * 0.66f, yMid,
                     graph.getX() + graph.getWidth() * 0.78f, yTreble, graph.getRight(), yTreble);
        g.setColour(laf.ampAccentSecondary.withAlpha(0.24f));
        g.strokePath(line, PathStrokeType(5.0f, PathStrokeType::curved, PathStrokeType::rounded));
        g.setColour(laf.ampAccentSecondary.withAlpha(0.92f));
        g.strokePath(line, PathStrokeType(1.6f, PathStrokeType::curved, PathStrokeType::rounded));
    };

    auto drawEmbeddedEqCurve = [&](Rectangle<int> curveBounds)
    {
        auto curve = curveBounds.toFloat();
        g.setColour(laf.ampInsetBg.withAlpha(0.90f));
        g.fillRoundedRectangle(curve, 7.0f);
        g.setColour(laf.ampBorder.withAlpha(0.42f));
        g.drawRoundedRectangle(curve.reduced(0.5f), 7.0f, 0.8f);

        auto graph = curve.reduced(9.0f, 7.0f);
        g.setColour(laf.ampTextDim.withAlpha(0.16f));
        g.drawHorizontalLine(roundToInt(graph.getCentreY()), graph.getX(), graph.getRight());

        Path eqPath;
        const int activeBandCount = namProcessor->getActiveParamEqBandCount();
        for (int band = 0; band < activeBandCount; ++band)
        {
            const auto x = graph.getX() + graph.getWidth() * ((float)band + 0.5f) / (float)activeBandCount;
            const auto y =
                jmap(jlimit(-12.0f, 12.0f, namProcessor->getParamEqBandGain(band)), -12.0f, 12.0f,
                     graph.getBottom(), graph.getY());
            if (band == 0)
                eqPath.startNewSubPath(x, y);
            else
                eqPath.lineTo(x, y);

            const auto bandDot = Rectangle<float>(7.0f, 7.0f).withCentre({x, y});
            g.setColour(laf.ampAccent
                            .interpolatedWith(laf.ampAccentSecondary,
                                              activeBandCount > 1 ? (float)band / (float)(activeBandCount - 1) : 0.0f)
                            .withAlpha(0.88f));
            g.fillEllipse(bandDot);
        }

        g.setColour(laf.ampAccent.withAlpha(0.18f));
        g.strokePath(eqPath, PathStrokeType(5.0f, PathStrokeType::curved, PathStrokeType::rounded));
        g.setColour(laf.ampAccent.withAlpha(0.88f));
        g.strokePath(eqPath, PathStrokeType(1.4f, PathStrokeType::curved, PathStrokeType::rounded));
    };

    auto drawParamEqDeck = [&](Rectangle<int> deckBounds)
    {
        const int activeBandCount = jlimit(1, NAMProcessor::kParamEqBandCount, namProcessor->getActiveParamEqBandCount());
        const int rows = activeBandCount > 4 ? 2 : 1;
        const int columns = jmax(1, (activeBandCount + rows - 1) / rows);
        const int gap = 4;
        const int rowGap = 5;
        const int bandW = jmax(1, (deckBounds.getWidth() - gap * (columns - 1)) / columns);
        const int bandH = jmax(1, (deckBounds.getHeight() - rowGap * (rows - 1)) / rows);

        for (int band = 0; band < activeBandCount; ++band)
        {
            const int row = band / columns;
            const int column = band % columns;
            auto strip = Rectangle<int>(deckBounds.getX() + column * (bandW + gap),
                                        deckBounds.getY() + row * (bandH + rowGap), bandW, bandH)
                             .reduced(1);

            const auto accent = laf.ampAccent.interpolatedWith(
                laf.ampAccentSecondary, activeBandCount > 1 ? (float)band / (float)(activeBandCount - 1) : 0.0f);
            ColourGradient fill(laf.ampInsetBg.withAlpha(0.78f), strip.getX(), strip.getY(),
                                laf.ampSurface.withAlpha(0.56f), strip.getX(), strip.getBottom(), false);
            g.setGradientFill(fill);
            g.fillRoundedRectangle(strip.toFloat(), 5.0f);
            g.setColour(accent.withAlpha(0.28f));
            g.drawRoundedRectangle(strip.toFloat().reduced(0.5f), 5.0f, 0.8f);
        }
    };

    auto captureSection = area.removeFromTop(66);
    drawSectionHeader(captureSection.removeFromTop(18), "Capture", laf.ampAccent);
    auto metaRow = captureSection.removeFromTop(22).reduced(6, 0);
    auto archPill = metaRow.removeFromRight(64).reduced(2, 2);
    metaRow.removeFromRight(4);
    auto namPill = metaRow.removeFromRight(44).reduced(2, 2);
    metaRow.removeFromRight(6);
    drawMockupPill(namPill, "NAM", laf.ampAccentSecondary, false);
    drawMockupPill(archPill, modelArchLabel->getText().isNotEmpty() ? modelArchLabel->getText() : "NAM",
                   laf.ampAccent, false);
    g.setFont(fm.getBadgeFont().withHeight(12.0f));
    g.setColour(laf.ampTextDim.withAlpha(0.72f));
    g.drawText(namProcessor->isModelLoaded() ? namProcessor->getModelName() : "No model selected",
               metaRow, Justification::centredLeft, true);
    captureSection.removeFromTop(27);

    area.removeFromTop(8);
    const int cabinetSectionHeight = cabinetIrCollapsed ? 34 : 224;
    auto cabinetSection = area.removeFromTop(cabinetSectionHeight);
    drawSectionHeader(cabinetSection.removeFromTop(18), "Cabinet IR", laf.ampAccentSecondary);
    if (cabinetIrCollapsed)
    {
        g.setFont(fm.getBadgeFont().withHeight(11.5f));
        g.setColour(laf.ampTextDim.withAlpha(0.70f));
        g.drawText("Cabinet controls collapsed", cabinetSection.reduced(16, 0), Justification::centredLeft, true);
    }
    else
    {
        cabinetSection.removeFromTop(8);
        auto slotGrid = cabinetSection.removeFromTop(98);
        const int slotGap = 12;
        const int slotW = (slotGrid.getWidth() - slotGap) / 2;
        auto ir1Slot = slotGrid.removeFromLeft(slotW);
        slotGrid.removeFromLeft(slotGap);
        auto ir2Slot = slotGrid;
        drawEmbeddedSlotCard(ir1Slot, "IR 1",
                             namProcessor->isIRLoaded() ? namProcessor->getIRName() : "No IR Loaded",
                             namProcessor->isIREnabled() ? "ON" : "OFF",
                             namProcessor->isIRLoaded() && namProcessor->isIREnabled(), laf.ampAccentSecondary);
        drawEmbeddedSlotCard(ir2Slot, "IR 2",
                             namProcessor->isIR2Loaded() ? namProcessor->getIR2Name() : "No IR 2 Loaded",
                             namProcessor->isIR2Enabled() ? "ON" : "OFF",
                             namProcessor->isIR2Loaded() && namProcessor->isIR2Enabled(), laf.ampAccentSecondary);

        cabinetSection.removeFromTop(10);
        drawControlRail(cabinetSection.removeFromTop(22), laf.ampAccentSecondary);
        cabinetSection.removeFromTop(10);
        drawControlRail(cabinetSection.removeFromTop(22), laf.ampAccentSecondary);
        cabinetSection.removeFromTop(10);
        drawControlRail(cabinetSection.removeFromTop(22), laf.ampAccentSecondary);
    }

    area.removeFromTop(8);
    auto gainSection = area.removeFromTop(109);
    drawSectionHeader(gainSection.removeFromTop(18), "Gain", laf.ampAccent);
    gainSection.removeFromTop(5);
    drawControlRail(gainSection.removeFromTop(22), laf.ampAccentSecondary);
    gainSection.removeFromTop(10);
    drawControlRail(gainSection.removeFromTop(22), laf.ampAccentSecondary);
    gainSection.removeFromTop(10);
    drawControlRail(gainSection.removeFromTop(22), laf.ampAccentSecondary);

    area.removeFromTop(8);
    auto toneSection = area;
    auto toneHeader = toneSection.removeFromTop(24);
    drawSectionHeader(toneHeader.withHeight(18), "Tone", laf.ampAccentSecondary);
    toneSection.removeFromTop(4);
    auto curveArea = toneSection.removeFromTop(namProcessor->getToneEqMode() == NAMProcessor::ToneEqMode::Parametric ? 58 : 42);
    if (namProcessor->getToneEqMode() == NAMProcessor::ToneEqMode::Parametric)
    {
        drawEmbeddedEqCurve(curveArea);
        toneSection.removeFromTop(6);
        drawParamEqDeck(toneSection.removeFromTop(getParamEqDeckHeight(true)));
    }
    else
    {
        drawEmbeddedToneCurve(curveArea);
        toneSection.removeFromTop(7);
        toneSection.removeFromTop(86);
    }

    toneSection.removeFromTop(8);
    drawSectionHeader(toneSection.removeFromTop(18), "FX Loop", laf.ampAccent);
}

//==============================================================================
void NAMControl::paint(Graphics& g)
{
    auto& laf = namLookAndFeel;
    auto& fm = FontManager::getInstance();
    auto bounds = getLocalBounds();
    const bool embeddedInGraphNode = isEmbeddedInGraphNode();

    if (embeddedInGraphNode)
    {
        paintEmbeddedGraphNode(g, bounds);
        return;
    }

    // Layout constants -- shared with resized()
    const int headerH = 34;
    const int panelMargin = 8;
    const int sectionGap = 6;
    const int signalH = 205;
    const int gainH = 100;

    // Main background gradient
    ColourGradient bgGradient(laf.ampSurface, 0, 0, laf.ampBackground, 0, (float)getHeight(), false);
    g.setGradientFill(bgGradient);
    g.fillRoundedRectangle(bounds.toFloat(), 4.0f);

    // Procedural noise texture (subtle grain for premium feel)
    {
        Random rng(42); // deterministic seed for consistency
        g.setColour(laf.ampTextBright.withAlpha(0.012f));
        const int step = 4;
        for (int ny = 0; ny < getHeight(); ny += step)
        {
            for (int nx = 0; nx < getWidth(); nx += step)
            {
                if (rng.nextFloat() > 0.5f)
                    g.fillRect(nx, ny, step, step);
            }
        }
    }

    // Outer border (double-line bevel)
    g.setColour(laf.ampBorder.darker(0.3f));
    g.drawRoundedRectangle(bounds.toFloat().reduced(0.5f), 4.0f, 1.5f);
    g.setColour(laf.ampBorder.brighter(0.15f));
    g.drawRoundedRectangle(bounds.toFloat().reduced(2.0f), 3.0f, 0.5f);

    // Header bar -- shows current model state with amp-category identity
    Rectangle<int> headerBounds(2, 2, getWidth() - 4, headerH);
    auto header = headerBounds.toFloat();
    ColourGradient headerGrad(namLookAndFeel.ampHeaderBg.brighter(0.13f), header.getX(), header.getY(),
                              namLookAndFeel.ampHeaderBg.darker(0.12f), header.getX(), header.getBottom(), false);
    g.setGradientFill(headerGrad);
    g.fillRoundedRectangle(header, 8.0f);
    g.setColour(namLookAndFeel.ampAccent.withAlpha(0.55f));
    g.fillRoundedRectangle(header.getX() + 8.0f, header.getBottom() - 3.0f, header.getWidth() - 16.0f, 2.0f,
                           1.0f);

    // Header top sheen
    g.setColour(laf.ampTextBright.withAlpha(0.07f));
    g.drawLine(8.0f, 4.0f, (float)getWidth() - 10.0f, 4.0f, 1.0f);

    String headerText = namProcessor->isModelLoaded() ? namProcessor->getModelName() : "No Model";
    auto headerTextArea = headerBounds.reduced(12, 0).withTrimmedRight(56);

    g.setColour(laf.ampAccent.withAlpha(namProcessor->isModelLoaded() ? 0.95f : 0.55f));
    g.fillRoundedRectangle((float)headerTextArea.getX(), (float)headerTextArea.getCentreY() - 4.0f, 8.0f, 8.0f,
                           2.5f);

    g.setColour(laf.ampTextDim.withAlpha(0.78f));
    g.setFont(fm.getCaptionFont());
    g.drawText("NAM LOADER", headerTextArea.withTrimmedLeft(16).withHeight(12), Justification::centredLeft, true);

    g.setColour(namProcessor->isModelLoaded() ? laf.ampTextBright : laf.ampTextDim);
    g.setFont(fm.getLabelFont());
    g.drawText(headerText, headerTextArea.withTrimmedLeft(16).withTrimmedTop(12), Justification::centredLeft, true);

    // Status LED in header (right side)
    const float ledSize = 12.0f;
    const float ledX = (float)getWidth() - 22.0f;
    const float ledY = (headerH - ledSize) * 0.5f + 2;

    Colour ledColour = namProcessor->isModelLoaded() ? laf.ampLedOn : laf.ampLedOff;

    if (namProcessor->isModelLoaded())
    {
        float pulse = 0.2f + 0.12f * std::sin(ledPulsePhase);
        g.setColour(ledColour.withAlpha(pulse));
        g.fillEllipse(ledX - 4, ledY - 4, ledSize + 8, ledSize + 8);
    }

    ColourGradient ledGradient(ledColour.brighter(0.3f), ledX, ledY, ledColour.darker(0.2f), ledX,
                               ledY + ledSize, false);
    g.setGradientFill(ledGradient);
    g.fillEllipse(ledX, ledY, ledSize, ledSize);
    g.setColour(laf.ampBorder.darker(0.2f));
    g.drawEllipse(ledX, ledY, ledSize, ledSize, 1.0f);

    if (namProcessor->isModelLoaded())
    {
        g.setColour(ledColour.contrasting(0.96f).withAlpha(0.2f));
        g.fillEllipse(ledX + 2, ledY + 1, ledSize * 0.35f, ledSize * 0.25f);
    }

    // Collapse chevron (right of LED)
    {
        const float chevX = (float)getWidth() - 40.0f;
        const float chevY = (headerH - 8.0f) * 0.5f + 2;
        Path chevron;
        if (collapsed)
        {
            chevron.addTriangle(chevX, chevY, chevX, chevY + 8.0f, chevX + 6.0f, chevY + 4.0f);
        }
        else
        {
            chevron.addTriangle(chevX, chevY, chevX + 8.0f, chevY, chevX + 4.0f, chevY + 6.0f);
        }
        g.setColour(laf.ampTextDim);
        g.fillPath(chevron);
    }

    // When collapsed, only draw header
    if (collapsed)
        return;

    // Section panels
    auto contentArea = bounds.reduced(panelMargin, 0);
    contentArea.removeFromTop(headerH + 7);

    auto signalBounds = contentArea.removeFromTop(signalH).reduced(0, 2);
    drawSectionPanel(g, signalBounds, "SIGNAL CHAIN");

    contentArea.removeFromTop(sectionGap);

    auto gainBounds = contentArea.removeFromTop(gainH).reduced(0, 2);
    drawSectionPanel(g, gainBounds, "GAIN");

    contentArea.removeFromTop(sectionGap);

    auto eqBounds = contentArea.reduced(0, 2);
    drawSectionPanel(g, eqBounds,
                     namProcessor->getToneEqMode() == NAMProcessor::ToneEqMode::Parametric ? "Parametric EQ"
                                                                                            : "TONE");
}

int NAMControl::getParamEqDeckHeight(bool embedded) const
{
    const bool doubleRow = namProcessor != nullptr && namProcessor->getActiveParamEqBandCount() > 4;
    if (embedded)
        return doubleRow ? kEmbeddedParamEqDoubleRowDeckHeight : kEmbeddedParamEqSingleRowDeckHeight;

    return doubleRow ? kStandaloneParamEqDoubleRowDeckHeight : kStandaloneParamEqSingleRowDeckHeight;
}

void NAMControl::configureParamEqSliderPresentation(bool embedded)
{
    for (int band = 0; band < NAMProcessor::kParamEqBandCount; ++band)
    {
        auto* frequency = paramEqFrequencySliders[band].get();
        auto* gain = paramEqGainSliders[band].get();
        auto* q = paramEqQSliders[band].get();

        for (auto* slider : {frequency, gain, q})
        {
            slider->setSliderStyle(Slider::LinearVertical);
            slider->setPopupDisplayEnabled(true, true, this);
        }

        if (embedded)
        {
            frequency->setTextBoxStyle(Slider::NoTextBox, false, 0, 0);
            gain->setTextBoxStyle(Slider::NoTextBox, false, 0, 0);
            q->setTextBoxStyle(Slider::NoTextBox, false, 0, 0);
        }
        else
        {
            frequency->setTextBoxStyle(Slider::TextBoxBelow, false, 54, 16);
            gain->setTextBoxStyle(Slider::TextBoxBelow, false, 46, 16);
            q->setTextBoxStyle(Slider::TextBoxBelow, false, 36, 16);
        }

        frequency->setTooltip("Band " + String(band + 1) + " frequency");
        gain->setTooltip("Band " + String(band + 1) + " gain");
        q->setTooltip("Band " + String(band + 1) + " Q");
    }
}

void NAMControl::layoutParamEqBandDeck(Rectangle<int> deckBounds, bool embedded)
{
    const int activeBandCount = jlimit(1, NAMProcessor::kParamEqBandCount, namProcessor->getActiveParamEqBandCount());
    const int rows = activeBandCount > 4 ? 2 : 1;
    const int columns = jmax(1, (activeBandCount + rows - 1) / rows);
    const int gap = embedded ? 4 : 6;
    const int rowGap = embedded ? 5 : 7;
    const int labelH = embedded ? 11 : 14;
    const int sliderGap = embedded ? 1 : 2;
    const int bandW = jmax(1, (deckBounds.getWidth() - gap * (columns - 1)) / columns);
    const int bandH = jmax(1, (deckBounds.getHeight() - rowGap * (rows - 1)) / rows);

    for (int band = 0; band < NAMProcessor::kParamEqBandCount; ++band)
    {
        if (band >= activeBandCount)
        {
            paramEqBandLabels[band]->setBounds(Rectangle<int>());
            paramEqFrequencySliders[band]->setBounds(Rectangle<int>());
            paramEqGainSliders[band]->setBounds(Rectangle<int>());
            paramEqQSliders[band]->setBounds(Rectangle<int>());
            continue;
        }

        const int row = band / columns;
        const int column = band % columns;
        auto bandArea = Rectangle<int>(deckBounds.getX() + column * (bandW + gap),
                                       deckBounds.getY() + row * (bandH + rowGap), bandW, bandH)
                            .reduced(2, embedded ? 2 : 3);

        auto labelArea = bandArea.removeFromTop(labelH);
        paramEqBandLabels[band]->setJustificationType(Justification::centred);
        paramEqBandLabels[band]->setBounds(labelArea);
        bandArea.removeFromTop(1);

        const int sliderW = jmax(6, (bandArea.getWidth() - sliderGap * 2) / 3);
        auto freqArea = bandArea.removeFromLeft(sliderW);
        bandArea.removeFromLeft(sliderGap);
        auto gainArea = bandArea.removeFromLeft(sliderW);
        bandArea.removeFromLeft(sliderGap);
        auto qArea = bandArea;

        paramEqFrequencySliders[band]->setBounds(freqArea);
        paramEqGainSliders[band]->setBounds(gainArea);
        paramEqQSliders[band]->setBounds(qArea);
    }
}

void NAMControl::resizedEmbeddedGraphNode(Rectangle<int> bounds)
{
    cabinetIrCollapsed = namProcessor->isEmbeddedCabinetIrCollapsed();

    auto area = bounds.reduced(3, 2);
    constexpr int gap = 4;
    constexpr int clearW = 23;

    for (auto* slider : {irBlendSlider.get(), irLowCutSlider.get(), irHighCutSlider.get()})
        slider->setTextBoxStyle(Slider::TextBoxRight, false, 42, 17);
    for (auto* slider : {inputGainSlider.get(), outputGainSlider.get(), noiseGateSlider.get()})
        slider->setTextBoxStyle(Slider::TextBoxRight, false, 46, 17);
    configureParamEqSliderPresentation(true);
    fxLoopEnabledButton->setButtonText("FX");
    normalizeButton->setButtonText("Norm");
    cabinetIrCollapseButton->setVisible(true);
    cabinetIrCollapseButton->setButtonText(cabinetIrCollapsed ? "Show" : "Hide");
    modelArchLabel->setBounds(Rectangle<int>());
    modelNameLabel->setBounds(Rectangle<int>());

    auto captureSection = area.removeFromTop(66);
    captureSection.removeFromTop(18);
    captureSection.removeFromTop(22);
    auto modelButtons = captureSection.removeFromTop(27).reduced(3, 2);
    loadModelButton->setBounds(modelButtons.removeFromLeft(92));
    modelButtons.removeFromLeft(gap);
    browseModelsButton->setBounds(modelButtons.removeFromLeft(86));
    modelButtons.removeFromLeft(gap);
    clearModelButton->setBounds(modelButtons.removeFromLeft(clearW));
    if (namProcessor->isCurrentModelSlimmable() && modelButtons.getWidth() > 68)
    {
        modelButtons.removeFromLeft(gap);
        slimmableSizeLabel->setBounds(modelButtons.removeFromLeft(34));
        modelButtons.removeFromLeft(2);
        slimmableSizeSlider->setBounds(modelButtons);
    }
    else
    {
        slimmableSizeLabel->setBounds(Rectangle<int>());
        slimmableSizeSlider->setBounds(Rectangle<int>());
    }

    area.removeFromTop(8);
    const int cabinetSectionHeight = cabinetIrCollapsed ? 34 : 224;
    auto cabinetSection = area.removeFromTop(cabinetSectionHeight);
    auto cabinetHeader = cabinetSection.removeFromTop(18);
    cabinetIrCollapseButton->setBounds(cabinetHeader.removeFromRight(54).reduced(0, 1));
    const bool showCabinetControls = !cabinetIrCollapsed;
    Component* cabinetControls[] = {loadIRButton.get(),     clearIRButton.get(),   irNameLabel.get(),
                                    irEnabledButton.get(),  loadIR2Button.get(),   clearIR2Button.get(),
                                    ir2NameLabel.get(),     ir2EnabledButton.get(), irBlendSlider.get(),
                                    irBlendLabel.get(),     irLowCutSlider.get(),  irLowCutLabel.get(),
                                    irHighCutSlider.get(),  irHighCutLabel.get()};
    for (auto* child : cabinetControls)
        child->setVisible(showCabinetControls);

    if (showCabinetControls)
    {
        cabinetSection.removeFromTop(8);
        auto irSlots = cabinetSection.removeFromTop(98);
        const int slotGap = 12;
        const int slotW = (irSlots.getWidth() - slotGap) / 2;
        auto ir1Slot = irSlots.removeFromLeft(slotW).reduced(8, 10);
        irSlots.removeFromLeft(slotGap);
        auto ir2Slot = irSlots.reduced(8, 10);

        ir1Slot.removeFromTop(24);
        auto ir1Controls = ir1Slot.removeFromTop(24);
        loadIRButton->setBounds(ir1Controls.removeFromLeft(62));
        ir1Controls.removeFromLeft(gap);
        clearIRButton->setBounds(ir1Controls.removeFromLeft(clearW));
        ir1Controls.removeFromLeft(gap);
        irEnabledButton->setBounds(Rectangle<int>(ir1Slot.getRight() - 51, ir1Slot.getY() - 24, 46, 20));
        ir1Slot.removeFromTop(8);
        irNameLabel->setBounds(ir1Slot.removeFromTop(25));

        ir2Slot.removeFromTop(24);
        auto ir2Controls = ir2Slot.removeFromTop(24);
        loadIR2Button->setBounds(ir2Controls.removeFromLeft(68));
        ir2Controls.removeFromLeft(gap);
        clearIR2Button->setBounds(ir2Controls.removeFromLeft(clearW));
        ir2Controls.removeFromLeft(gap);
        ir2EnabledButton->setBounds(Rectangle<int>(ir2Slot.getRight() - 51, ir2Slot.getY() - 24, 46, 20));
        ir2Slot.removeFromTop(8);
        ir2NameLabel->setBounds(ir2Slot.removeFromTop(25));

        cabinetSection.removeFromTop(10);
        auto blendArea = cabinetSection.removeFromTop(22);
        irBlendLabel->setBounds(blendArea.removeFromLeft(47));
        blendArea.removeFromLeft(gap);
        irBlendSlider->setBounds(blendArea);

        cabinetSection.removeFromTop(10);
        auto lowArea = cabinetSection.removeFromTop(22);
        irLowCutLabel->setBounds(lowArea.removeFromLeft(47));
        lowArea.removeFromLeft(gap);
        irLowCutSlider->setBounds(lowArea);

        cabinetSection.removeFromTop(10);
        auto highArea = cabinetSection.removeFromTop(22);
        irHighCutLabel->setBounds(highArea.removeFromLeft(47));
        highArea.removeFromLeft(gap);
        irHighCutSlider->setBounds(highArea);
    }

    area.removeFromTop(8);
    auto gainArea = area.removeFromTop(109);
    gainArea.removeFromTop(18);
    gainArea.removeFromTop(5);
    for (auto rowSpec : {std::pair<Label*, Slider*>{inputGainLabel.get(), inputGainSlider.get()},
                         std::pair<Label*, Slider*>{outputGainLabel.get(), outputGainSlider.get()},
                         std::pair<Label*, Slider*>{noiseGateLabel.get(), noiseGateSlider.get()}})
    {
        auto row = gainArea.removeFromTop(22);
        rowSpec.first->setBounds(row.removeFromLeft(46));
        row.removeFromLeft(gap);
        rowSpec.second->setBounds(row);
        gainArea.removeFromTop(10);
    }

    area.removeFromTop(8);
    auto toneArea = area;
    auto toneHeader = toneArea.removeFromTop(24);
    toneHeader.removeFromLeft(60);
    auto toneTools = toneHeader.reduced(2, 1);
    toneStackEnabledButton->setBounds(toneTools.removeFromLeft(38));
    toneTools.removeFromLeft(gap);
    toneStackPreButton->setBounds(toneTools.removeFromLeft(40));
    toneTools.removeFromLeft(gap);
    toneEqModeStackButton->setBounds(toneTools.removeFromLeft(62));
    toneTools.removeFromLeft(gap);
    toneEqModeParamButton->setBounds(Rectangle<int>());
    if (namProcessor->getToneEqMode() == NAMProcessor::ToneEqMode::Parametric)
    {
        paramEqBandCountButton->setBounds(toneTools.removeFromLeft(38));
        toneTools.removeFromLeft(gap);
    }
    else
    {
        paramEqBandCountButton->setBounds(Rectangle<int>());
    }
    normalizeButton->setBounds(toneTools.removeFromLeft(62));

    toneArea.removeFromTop(4);
    if (namProcessor->getToneEqMode() == NAMProcessor::ToneEqMode::Parametric)
    {
        toneArea.removeFromTop(58);
        toneArea.removeFromTop(6);
        layoutParamEqBandDeck(toneArea.removeFromTop(getParamEqDeckHeight(true)), true);
    }
    else
    {
        toneArea.removeFromTop(42);
        toneArea.removeFromTop(7);
        auto knobRow = toneArea.removeFromTop(86);
        const int knobW = knobRow.getWidth() / 3;
        const int knobSize = 68;

        auto bassArea = knobRow.removeFromLeft(knobW);
        bassLabel->setText("BASS " + String(bassSlider->getValue(), 1), dontSendNotification);
        bassLabel->setBounds(bassArea.removeFromBottom(15));
        bassSlider->setBounds(bassArea.withSizeKeepingCentre(knobSize, knobSize));

        auto midArea = knobRow.removeFromLeft(knobW);
        midLabel->setText("MID " + String(midSlider->getValue(), 1), dontSendNotification);
        midLabel->setBounds(midArea.removeFromBottom(15));
        midSlider->setBounds(midArea.withSizeKeepingCentre(knobSize, knobSize));

        auto trebleArea = knobRow;
        trebleLabel->setText("TREBLE " + String(trebleSlider->getValue(), 1), dontSendNotification);
        trebleLabel->setBounds(trebleArea.removeFromBottom(15));
        trebleSlider->setBounds(trebleArea.withSizeKeepingCentre(knobSize, knobSize));
    }

    toneArea.removeFromTop(8);
    toneArea.removeFromTop(18);
    toneArea.removeFromTop(3);
    auto fxTools = toneArea.removeFromTop(24);
    fxLoopEnabledButton->setBounds(fxTools.removeFromLeft(58));
    fxTools.removeFromLeft(gap);
    editFxLoopButton->setBounds(fxTools.removeFromLeft(82));

    updateEqModeVisibility();
}

void NAMControl::resized()
{
    const bool embeddedInGraphNode = isEmbeddedInGraphNode();
    auto bounds = getLocalBounds();

    if (embeddedInGraphNode)
    {
        resizedEmbeddedGraphNode(bounds);
        return;
    }

    if (collapsed)
        return;

    // Layout constants -- must match paint()
    const int headerH = 34;
    const int panelMargin = 8;
    const int sectionGap = 6;
    const int signalH = 205;
    const int gainH = 100;

    bounds.removeFromTop(headerH + 7); // header + accent + gap
    bounds = bounds.reduced(panelMargin, 0);

    const int rowHeight = 26;
    const int labelWidth = 60;
    const int buttonWidth = 80;
    const int clearButtonWidth = 26;
    const int spacing = 4;
    const int sectionHeaderH = 20;
    const int sectionPad = 8;

    fxLoopEnabledButton->setButtonText("FX Loop");
    normalizeButton->setButtonText("Normalize");
    cabinetIrCollapseButton->setVisible(false);
    modelNameLabel->setVisible(true);
    modelArchLabel->setVisible(true);

    configureParamEqSliderPresentation(false);

    // ===================== SIGNAL CHAIN section =====================
    auto signalArea = bounds.removeFromTop(signalH).reduced(sectionPad, 2);
    signalArea.removeFromTop(sectionHeaderH);

    // Model row
    auto modelRow = signalArea.removeFromTop(rowHeight);
    loadModelButton->setBounds(modelRow.removeFromLeft(buttonWidth));
    modelRow.removeFromLeft(spacing);
    browseModelsButton->setBounds(modelRow.removeFromLeft(64));
    modelRow.removeFromLeft(spacing);
    clearModelButton->setBounds(modelRow.removeFromLeft(clearButtonWidth));
    modelRow.removeFromLeft(spacing);

    if (modelArchLabel->getText().isNotEmpty())
    {
        modelArchLabel->setBounds(modelRow.removeFromRight(50));
        modelRow.removeFromRight(spacing);
    }
    else
    {
        modelArchLabel->setBounds(Rectangle<int>());
    }
    modelNameLabel->setBounds(modelRow);

    signalArea.removeFromTop(spacing);

    // IR row
    auto irRow = signalArea.removeFromTop(rowHeight);
    loadIRButton->setBounds(irRow.removeFromLeft(buttonWidth));
    irRow.removeFromLeft(spacing);
    clearIRButton->setBounds(irRow.removeFromLeft(clearButtonWidth));
    irRow.removeFromLeft(spacing);
    irEnabledButton->setBounds(irRow.removeFromRight(50));
    irRow.removeFromRight(spacing);
    irNameLabel->setBounds(irRow);

    signalArea.removeFromTop(spacing);

    // IR2 row
    auto ir2Row = signalArea.removeFromTop(rowHeight);
    loadIR2Button->setBounds(ir2Row.removeFromLeft(buttonWidth));
    ir2Row.removeFromLeft(spacing);
    clearIR2Button->setBounds(ir2Row.removeFromLeft(clearButtonWidth));
    ir2Row.removeFromLeft(spacing);
    ir2EnabledButton->setBounds(ir2Row.removeFromRight(50));
    ir2Row.removeFromRight(spacing);
    ir2NameLabel->setBounds(ir2Row);

    signalArea.removeFromTop(spacing);

    // IR Blend row
    auto blendRow = signalArea.removeFromTop(rowHeight);
    irBlendLabel->setBounds(blendRow.removeFromLeft(45));
    blendRow.removeFromLeft(2);
    irBlendSlider->setBounds(blendRow);

    signalArea.removeFromTop(spacing);

    // IR Filters row
    auto irFilterRow = signalArea.removeFromTop(rowHeight);
    const int halfWidth = (irFilterRow.getWidth() - spacing) / 2;

    auto lowCutArea = irFilterRow.removeFromLeft(halfWidth);
    irLowCutLabel->setBounds(lowCutArea.removeFromLeft(45));
    lowCutArea.removeFromLeft(2);
    irLowCutSlider->setBounds(lowCutArea);

    irFilterRow.removeFromLeft(spacing);

    auto highCutArea = irFilterRow;
    irHighCutLabel->setBounds(highCutArea.removeFromLeft(45));
    highCutArea.removeFromLeft(2);
    irHighCutSlider->setBounds(highCutArea);

    signalArea.removeFromTop(spacing);

    // FX Loop row
    auto fxRow = signalArea.removeFromTop(rowHeight);
    fxLoopEnabledButton->setBounds(fxRow.removeFromLeft(75));
    fxRow.removeFromLeft(spacing);
    editFxLoopButton->setBounds(fxRow.removeFromLeft(80));
    if (namProcessor->isCurrentModelSlimmable() && fxRow.getWidth() > 84)
    {
        fxRow.removeFromLeft(spacing);
        slimmableSizeLabel->setBounds(fxRow.removeFromLeft(45));
        fxRow.removeFromLeft(2);
        slimmableSizeSlider->setBounds(fxRow);
    }
    else
    {
        slimmableSizeLabel->setBounds(Rectangle<int>());
        slimmableSizeSlider->setBounds(Rectangle<int>());
    }

    bounds.removeFromTop(sectionGap);

    // ===================== GAIN section =====================
    auto gainArea = bounds.removeFromTop(gainH).reduced(sectionPad, 2);
    gainArea.removeFromTop(sectionHeaderH);

    auto inputRow = gainArea.removeFromTop(rowHeight);
    inputGainLabel->setBounds(inputRow.removeFromLeft(labelWidth));
    inputRow.removeFromLeft(spacing);
    inputGainSlider->setBounds(inputRow);

    gainArea.removeFromTop(spacing);

    auto outputRow = gainArea.removeFromTop(rowHeight);
    outputGainLabel->setBounds(outputRow.removeFromLeft(labelWidth));
    outputRow.removeFromLeft(spacing);
    outputGainSlider->setBounds(outputRow);

    gainArea.removeFromTop(spacing);

    auto gateRow = gainArea.removeFromTop(rowHeight);
    noiseGateLabel->setBounds(gateRow.removeFromLeft(labelWidth));
    gateRow.removeFromLeft(spacing);
    noiseGateSlider->setBounds(gateRow);

    bounds.removeFromTop(sectionGap);

    // ===================== TONE section =====================
    auto eqArea = bounds.reduced(sectionPad, 2);
    eqArea.removeFromTop(sectionHeaderH);

    auto eqHeaderRow = eqArea.removeFromTop(24);
    toneStackEnabledButton->setBounds(eqHeaderRow.removeFromLeft(55));
    eqHeaderRow.removeFromLeft(spacing);
    toneStackPreButton->setBounds(eqHeaderRow.removeFromLeft(50));
    eqHeaderRow.removeFromLeft(spacing);
    toneEqModeStackButton->setBounds(eqHeaderRow.removeFromLeft(58));
    eqHeaderRow.removeFromLeft(spacing);
    toneEqModeParamButton->setBounds(eqHeaderRow.removeFromLeft(58));
    eqHeaderRow.removeFromLeft(spacing);
    if (namProcessor->getToneEqMode() == NAMProcessor::ToneEqMode::Parametric)
    {
        paramEqBandCountButton->setBounds(eqHeaderRow.removeFromLeft(48));
        eqHeaderRow.removeFromLeft(spacing);
    }
    else
    {
        paramEqBandCountButton->setBounds(Rectangle<int>());
    }
    normalizeButton->setBounds(eqHeaderRow.removeFromLeft(96));

    eqArea.removeFromTop(6);

    if (namProcessor->getToneEqMode() == NAMProcessor::ToneEqMode::Parametric)
    {
        layoutParamEqBandDeck(eqArea.removeFromTop(getParamEqDeckHeight(false)), false);
    }
    else
    {
        // Knobs row -- use remaining space
        auto knobRow = eqArea;
        const int knobWidth = knobRow.getWidth() / 3;
        const int knobSize = 52;

        auto bassArea = knobRow.removeFromLeft(knobWidth);
        bassLabel->setBounds(bassArea.removeFromBottom(14));
        bassSlider->setBounds(bassArea.withSizeKeepingCentre(knobSize, knobSize));

        auto midArea = knobRow.removeFromLeft(knobWidth);
        midLabel->setBounds(midArea.removeFromBottom(14));
        midSlider->setBounds(midArea.withSizeKeepingCentre(knobSize, knobSize));

        auto trebleArea = knobRow;
        trebleLabel->setBounds(trebleArea.removeFromBottom(14));
        trebleSlider->setBounds(trebleArea.withSizeKeepingCentre(knobSize, knobSize));
    }

    updateEqModeVisibility();
}

//==============================================================================
void NAMControl::buttonClicked(Button* button)
{
    if (button == loadModelButton.get())
    {
        modelFileChooser = std::make_unique<FileChooser>(
            "Select NAM Model", File::getSpecialLocation(File::userDocumentsDirectory), "*.nam", true);

        auto chooserFlags = FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles;

        modelFileChooser->launchAsync(chooserFlags,
                                      [this](const FileChooser& fc)
                                      {
                                          auto result = fc.getResult();
                                          if (result.existsAsFile())
                                          {
                                              if (namProcessor->loadModel(result))
                                              {
                                                  updateModelDisplay();
                                                  repaint();
                                              }
                                          }
                                      });
    }
    else if (button == browseModelsButton.get())
    {
        NAMModelBrowser::showWindow(namProcessor,
                                    [this]()
                                    {
                                        updateModelDisplay();
                                        repaint();
                                    });
    }
    else if (button == clearModelButton.get())
    {
        namProcessor->clearModel();
        updateModelDisplay();
        repaint();
    }
    else if (button == loadIRButton.get())
    {
        irFileChooser = std::make_unique<FileChooser>("Select Impulse Response",
                                                      File::getSpecialLocation(File::userDocumentsDirectory),
                                                      "*.wav;*.aiff;*.aif", true);

        auto chooserFlags = FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles;

        irFileChooser->launchAsync(chooserFlags,
                                   [this](const FileChooser& fc)
                                   {
                                       auto result = fc.getResult();
                                       if (result.existsAsFile())
                                       {
                                           if (namProcessor->loadIR(result))
                                           {
                                               updateIRDisplay();
                                               repaint();
                                           }
                                       }
                                   });
    }
    else if (button == clearIRButton.get())
    {
        namProcessor->clearIR();
        updateIRDisplay();
        repaint();
    }
    else if (button == loadIR2Button.get())
    {
        ir2FileChooser = std::make_unique<FileChooser>("Select Impulse Response 2",
                                                       File::getSpecialLocation(File::userDocumentsDirectory),
                                                       "*.wav;*.aiff;*.aif", true);

        auto chooserFlags = FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles;

        ir2FileChooser->launchAsync(chooserFlags,
                                    [this](const FileChooser& fc)
                                    {
                                        auto result = fc.getResult();
                                        if (result.existsAsFile())
                                        {
                                            if (namProcessor->loadIR2(result))
                                            {
                                                updateIRDisplay();
                                                repaint();
                                            }
                                        }
                                    });
    }
    else if (button == clearIR2Button.get())
    {
        namProcessor->clearIR2();
        updateIRDisplay();
        repaint();
    }
    else if (button == irEnabledButton.get())
    {
        namProcessor->setIREnabled(irEnabledButton->getToggleState());
    }
    else if (button == ir2EnabledButton.get())
    {
        namProcessor->setIR2Enabled(ir2EnabledButton->getToggleState());
    }
    else if (button == fxLoopEnabledButton.get())
    {
        namProcessor->setEffectsLoopEnabled(fxLoopEnabledButton->getToggleState());
    }
    else if (button == editFxLoopButton.get())
    {
        // Open the effects loop editor
        if (auto* fxLoop = namProcessor->getEffectsLoop())
        {
            if (auto* editor = fxLoop->createEditor())
            {
                // Create self-deleting window
                class FXLoopWindow : public DocumentWindow
                {
                  public:
                    FXLoopWindow(const String& name, Colour bg) : DocumentWindow(name, bg, DocumentWindow::closeButton)
                    {
                    }
                    void closeButtonPressed() override { delete this; }
                };

                auto* window =
                    new FXLoopWindow("FX Loop - " + namProcessor->getModelName(), namLookAndFeel.ampBackground);
                window->setContentOwned(editor, true);
                window->setResizable(true, false);
                window->setUsingNativeTitleBar(true);
                window->centreWithSize(editor->getWidth(), editor->getHeight());
                window->setVisible(true);
            }
        }
    }
    else if (button == toneStackEnabledButton.get())
    {
        namProcessor->setToneStackEnabled(toneStackEnabledButton->getToggleState());
    }
    else if (button == toneStackPreButton.get())
    {
        bool newPre = !namProcessor->isToneStackPre();
        namProcessor->setToneStackPre(newPre);
        toneStackPreButton->setButtonText(newPre ? "PRE" : "POST");
        repaint();
    }
    else if (button == toneEqModeStackButton.get())
    {
        if (namProcessor->getToneEqMode() == NAMProcessor::ToneEqMode::Parametric)
            namProcessor->setToneEqMode(NAMProcessor::ToneEqMode::Stack);
        else
            namProcessor->setToneEqMode(NAMProcessor::ToneEqMode::Parametric);
        updateEqModeVisibility();
        resized();
        repaint();
    }
    else if (button == toneEqModeParamButton.get())
    {
        namProcessor->setToneEqMode(NAMProcessor::ToneEqMode::Parametric);
        updateEqModeVisibility();
        resized();
        repaint();
    }
    else if (button == paramEqBandCountButton.get())
    {
        const int current = namProcessor->getActiveParamEqBandCount();
        const int next = current == 4 ? 8 : (current == 8 ? 10 : (current == 10 ? 12 : 4));
        namProcessor->setActiveParamEqBandCount(next);
        updateEqModeVisibility();
        resized();
        repaint();
    }
    else if (button == cabinetIrCollapseButton.get())
    {
        cabinetIrCollapsed = !cabinetIrCollapsed;
        namProcessor->setEmbeddedCabinetIrCollapsed(cabinetIrCollapsed);
        cabinetIrCollapseButton->setButtonText(cabinetIrCollapsed ? "Show" : "Hide");
        resized();
        repaint();
        if (auto* pluginNode = findParentComponentOfClass<PluginComponent>())
            pluginNode->updateNodeSize();
    }
    else if (button == normalizeButton.get())
    {
        namProcessor->setNormalizeOutput(normalizeButton->getToggleState());
    }
}

void NAMControl::sliderValueChanged(Slider* slider)
{
    if (slider == inputGainSlider.get())
    {
        namProcessor->setInputGain(static_cast<float>(slider->getValue()));
    }
    else if (slider == outputGainSlider.get())
    {
        namProcessor->setOutputGain(static_cast<float>(slider->getValue()));
    }
    else if (slider == noiseGateSlider.get())
    {
        namProcessor->setNoiseGateThreshold(static_cast<float>(slider->getValue()));
    }
    else if (slider == bassSlider.get())
    {
        namProcessor->setBass(static_cast<float>(slider->getValue()));
        bassLabel->setText("BASS " + String(slider->getValue(), 1), dontSendNotification);
        repaint();
    }
    else if (slider == midSlider.get())
    {
        namProcessor->setMid(static_cast<float>(slider->getValue()));
        midLabel->setText("MID " + String(slider->getValue(), 1), dontSendNotification);
        repaint();
    }
    else if (slider == trebleSlider.get())
    {
        namProcessor->setTreble(static_cast<float>(slider->getValue()));
        trebleLabel->setText("TREBLE " + String(slider->getValue(), 1), dontSendNotification);
        repaint();
    }
    else if (slider == irLowCutSlider.get())
    {
        namProcessor->setIRLowCut(static_cast<float>(slider->getValue()));
    }
    else if (slider == irHighCutSlider.get())
    {
        namProcessor->setIRHighCut(static_cast<float>(slider->getValue()));
    }
    else if (slider == irBlendSlider.get())
    {
        namProcessor->setIRBlend(static_cast<float>(slider->getValue()));
    }
    else if (slider == slimmableSizeSlider.get())
    {
        namProcessor->setSlimmableSize(static_cast<float>(slider->getValue()));
    }

    for (int band = 0; band < NAMProcessor::kParamEqBandCount; ++band)
    {
        if (slider == paramEqFrequencySliders[band].get())
        {
            namProcessor->setParamEqBandFrequency(band, static_cast<float>(slider->getValue()));
            repaint();
            return;
        }
        if (slider == paramEqGainSliders[band].get())
        {
            namProcessor->setParamEqBandGain(band, static_cast<float>(slider->getValue()));
            repaint();
            return;
        }
        if (slider == paramEqQSliders[band].get())
        {
            namProcessor->setParamEqBandQ(band, static_cast<float>(slider->getValue()));
            repaint();
            return;
        }
    }
}

void NAMControl::updateSlimmableControlState()
{
    const bool visible = !collapsed && namProcessor->isCurrentModelSlimmable();
    slimmableSizeSlider->setVisible(visible);
    slimmableSizeLabel->setVisible(visible);
    slimmableSizeSlider->setEnabled(visible);
    slimmableSizeLabel->setEnabled(visible);
    slimmableSizeSlider->setValue(namProcessor->getSlimmableSize(), dontSendNotification);
}

//==============================================================================
void NAMControl::updateModelDisplay()
{
    auto& laf = namLookAndFeel;
    const auto loadedChipBg = laf.ampInsetBg.interpolatedWith(laf.ampAccent, 0.06f);
    const auto loadedChipOutline = laf.ampAccent.withAlpha(0.32f);
    const auto emptyChipOutline = laf.ampBorder.withAlpha(0.75f);

    if (namProcessor->isModelLoaded())
    {
        const auto architectureBadge = namProcessor->getModelArchitectureBadge();
        const auto badgeColour =
            architectureBadge.equalsIgnoreCase("A2") ? laf.ampAccentSecondary : laf.ampAccent;

        modelNameLabel->setText(namProcessor->getModelName(), dontSendNotification);
        modelNameLabel->setColour(Label::textColourId, laf.ampTextBright);
        modelNameLabel->setColour(Label::backgroundColourId, loadedChipBg);
        modelNameLabel->setColour(Label::outlineColourId, loadedChipOutline);

        modelArchLabel->setText(architectureBadge, dontSendNotification);
        modelArchLabel->setColour(Label::backgroundColourId, badgeColour.withAlpha(0.18f));
        modelArchLabel->setColour(Label::outlineColourId, badgeColour.withAlpha(0.42f));
        modelArchLabel->setColour(Label::textColourId, badgeColour);
    }
    else
    {
        modelNameLabel->setText("No Model Loaded", dontSendNotification);
        modelNameLabel->setColour(Label::textColourId, laf.ampTextDim);
        modelNameLabel->setColour(Label::backgroundColourId, laf.ampInsetBg);
        modelNameLabel->setColour(Label::outlineColourId, emptyChipOutline);
        modelArchLabel->setText("", dontSendNotification);
    }

    // Relayout to show/hide architecture badge
    updateSlimmableControlState();
    resized();
}

void NAMControl::updateIRDisplay()
{
    auto& laf = namLookAndFeel;
    const auto loadedChipBg = laf.ampInsetBg.interpolatedWith(laf.ampAccent, 0.06f);
    const auto loadedChipOutline = laf.ampAccent.withAlpha(0.32f);
    const auto emptyChipOutline = laf.ampBorder.withAlpha(0.75f);

    if (namProcessor->isIRLoaded())
    {
        irNameLabel->setText(namProcessor->getIRName(), dontSendNotification);
        irNameLabel->setColour(Label::textColourId, laf.ampTextBright);
        irNameLabel->setColour(Label::backgroundColourId, loadedChipBg);
        irNameLabel->setColour(Label::outlineColourId, loadedChipOutline);
    }
    else
    {
        irNameLabel->setText("No IR Loaded", dontSendNotification);
        irNameLabel->setColour(Label::textColourId, laf.ampTextDim);
        irNameLabel->setColour(Label::backgroundColourId, laf.ampInsetBg);
        irNameLabel->setColour(Label::outlineColourId, emptyChipOutline);
    }

    if (namProcessor->isIR2Loaded())
    {
        ir2NameLabel->setText(namProcessor->getIR2Name(), dontSendNotification);
        ir2NameLabel->setColour(Label::textColourId, laf.ampTextBright);
        ir2NameLabel->setColour(Label::backgroundColourId, loadedChipBg);
        ir2NameLabel->setColour(Label::outlineColourId, loadedChipOutline);
    }
    else
    {
        ir2NameLabel->setText("No IR2 Loaded", dontSendNotification);
        ir2NameLabel->setColour(Label::textColourId, laf.ampTextDim);
        ir2NameLabel->setColour(Label::backgroundColourId, laf.ampInsetBg);
        ir2NameLabel->setColour(Label::outlineColourId, emptyChipOutline);
    }
}

void NAMControl::drawSectionPanel(Graphics& g, const Rectangle<int>& bounds, const String& title)
{
    auto& laf = namLookAndFeel;
    auto& fm = FontManager::getInstance();

    auto panel = bounds.toFloat();
    const float radius = 6.0f;

    ColourGradient panelFill(laf.ampSurface.brighter(0.07f), panel.getX(), panel.getY(),
                             laf.ampSurface.darker(0.12f), panel.getX(), panel.getBottom(), false);
    g.setGradientFill(panelFill);
    g.fillRoundedRectangle(panel, radius);

    // Brushed-metal texture (subtle horizontal lines)
    {
        g.saveState();
        g.reduceClipRegion(bounds);
        for (int ly = bounds.getY(); ly < bounds.getBottom(); ly += 2)
        {
            float alpha = ((ly % 4) == 0) ? 0.025f : 0.012f;
            g.setColour(laf.ampTextBright.withAlpha(alpha));
            g.drawHorizontalLine(ly, static_cast<float>(bounds.getX()), static_cast<float>(bounds.getRight()));
        }
        g.restoreState();
    }

    // Inner shadow effect (top edge darker for depth)
    ColourGradient shadowGrad(Colours::black.withAlpha(0.12f), (float)bounds.getX(), (float)bounds.getY(),
                              Colours::transparentBlack, (float)bounds.getX(), bounds.getY() + 10.0f, false);
    g.setGradientFill(shadowGrad);
    g.fillRoundedRectangle(bounds.toFloat(), 5.0f);

    // Bottom highlight (convex bevel)
    ColourGradient bottomHighlight(Colours::transparentBlack, (float)bounds.getX(), bounds.getBottom() - 10.0f,
                                   laf.ampTextBright.withAlpha(0.03f), (float)bounds.getX(),
                                   (float)bounds.getBottom(), false);
    g.setGradientFill(bottomHighlight);
    g.fillRoundedRectangle(panel, radius);

    g.setColour(laf.ampTextBright.withAlpha(0.055f));
    g.drawLine(panel.getX() + 7.0f, panel.getY() + 2.0f, panel.getRight() - 7.0f, panel.getY() + 2.0f, 1.0f);

    g.setColour(laf.ampAccent.withAlpha(0.55f));
    g.fillRoundedRectangle(panel.getX() + 1.0f, panel.getY() + 7.0f, 3.0f, jmax(12.0f, panel.getHeight() - 14.0f),
                           1.5f);

    g.setColour(laf.ampBorder.brighter(0.14f));
    g.drawRoundedRectangle(panel.reduced(0.5f), radius, 1.15f);

    // Section title with accent dot glow
    if (title.isNotEmpty())
    {
        float dotX = bounds.getX() + 6.0f;
        float dotY = bounds.getY() + 6.5f;
        float dotSize = 4.0f;

        // Accent dot glow aura
        g.setColour(laf.ampAccent.withAlpha(0.15f));
        g.fillEllipse(dotX - 3.0f, dotY - 3.0f, dotSize + 6.0f, dotSize + 6.0f);
        g.setColour(laf.ampAccent.withAlpha(0.3f));
        g.fillEllipse(dotX - 1.0f, dotY - 1.0f, dotSize + 2.0f, dotSize + 2.0f);

        // Accent dot
        g.setColour(laf.ampAccent);
        g.fillEllipse(dotX, dotY, dotSize, dotSize);

        // Title text
        g.setColour(laf.ampTextDim.withAlpha(0.82f));
        g.setFont(fm.getCaptionFont());
        g.drawText(title, bounds.getX() + 14, bounds.getY() + 2, 100, 16, Justification::centredLeft);
    }
}
