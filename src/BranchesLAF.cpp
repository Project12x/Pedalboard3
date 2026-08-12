//	BranchesLAF.cpp - LookAndFeel class implementing some different buttons.
//	----------------------------------------------------------------------------
//	This file is part of Branches, a branching story editor.
//	Copyright (c) 2008 Niall Moody.
//
//	This program is free software: you can redistribute it and/or modify
//	it under the terms of the GNU General Public License as published by
//	the Free Software Foundation, either version 3 of the License, or
//	(at your option) any later version.
//
//	This program is distributed in the hope that it will be useful,
//	but WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//	GNU General Public License for more details.
//
//	You should have received a copy of the GNU General Public License
//	along with this program.  If not, see <http://www.gnu.org/licenses/>.
//	----------------------------------------------------------------------------

#include "BranchesLAF.h"

#include "ColourScheme.h"
#include "FontManager.h"
#include "LookAndFeelImages.h"

#include <melatonin_blur/melatonin_blur.h>

using namespace std;

//------------------------------------------------------------------------------
BranchesLAF::BranchesLAF() : LookAndFeel_V4()
{
    refreshColours();
}

//------------------------------------------------------------------------------
BranchesLAF::~BranchesLAF() {}

//------------------------------------------------------------------------------
void BranchesLAF::refreshColours()
{
    auto& themeColours = ::ColourScheme::getInstance().colours;

    for (const auto& spec : ::ColourScheme::getLookAndFeelColourSpecs())
    {
        const auto role = String(spec.role);
        const auto colour = themeColours.find(role);
        if (colour != themeColours.end())
            setColour(spec.colourId, colour->second.withMultipliedAlpha(spec.alpha));
    }

    setColour(AlertWindow::backgroundColourId, themeColours["Window Background"]);
}

//------------------------------------------------------------------------------
void BranchesLAF::drawButtonBackground(Graphics& g, Button& button, const Colour& backgroundColour,
                                       bool isMouseOverButton, bool isButtonDown)
{
    auto& colours = ::ColourScheme::getInstance().colours;
    const auto defaultButtonCol = colours["Button Colour"];
    auto buttonCol =
        (backgroundColour != defaultButtonCol && backgroundColour != Colour()) ? backgroundColour : defaultButtonCol;
    const auto accentCol = colours["Accent Colour"];
    const auto borderCol = colours["Plugin Border"];
    const auto textCol = colours["Text Colour"];

    auto bounds = button.getLocalBounds().toFloat().reduced(1.0f);
    const float cornerRadius = jmin(8.0f, jmax(4.0f, bounds.getHeight() * 0.28f));
    const bool toggled = button.getToggleState();
    const bool enabled = button.isEnabled();

    if (isButtonDown)
        bounds = bounds.translated(0.0f, 0.8f);

    if (toggled)
        buttonCol = buttonCol.interpolatedWith(accentCol, 0.22f);
    if (isMouseOverButton && enabled)
        buttonCol = buttonCol.brighter(toggled ? 0.10f : 0.07f);
    if (!enabled)
        buttonCol = buttonCol.withMultipliedAlpha(0.42f);

    g.setColour(colours["Window Background"].darker(0.58f).withAlpha(isButtonDown ? 0.13f : 0.28f));
    g.fillRoundedRectangle(bounds.translated(0.0f, isButtonDown ? 0.6f : 1.8f), cornerRadius);

    ColourGradient fill(buttonCol.brighter(0.20f), bounds.getX(), bounds.getY(), buttonCol.darker(0.18f),
                        bounds.getX(), bounds.getBottom(), false);
    fill.addColour(0.45, buttonCol.brighter(0.07f));
    fill.addColour(0.78, buttonCol.darker(0.08f));
    g.setGradientFill(fill);
    g.fillRoundedRectangle(bounds, cornerRadius);

    auto topSheen = bounds.withHeight(jmax(3.0f, bounds.getHeight() * 0.42f)).reduced(2.0f, 1.0f);
    ColourGradient sheen(textCol.withAlpha(enabled ? 0.13f : 0.05f), topSheen.getX(), topSheen.getY(),
                         textCol.withAlpha(0.0f), topSheen.getX(), topSheen.getBottom(), false);
    g.setGradientFill(sheen);
    g.fillRoundedRectangle(topSheen, jmax(2.0f, cornerRadius - 2.0f));

    if ((isMouseOverButton || toggled) && enabled)
    {
        Path glowShape;
        glowShape.addRoundedRectangle(bounds.expanded(0.5f), cornerRadius + 0.5f);
        melatonin::DropShadow buttonGlow{accentCol.withAlpha(toggled ? 0.24f : 0.16f), toggled ? 9 : 6, {0, 0}};
        buttonGlow.render(g, glowShape);
    }

    const auto borderMix = toggled ? 0.58f : (isMouseOverButton && enabled ? 0.42f : 0.20f);
    g.setColour(borderCol.interpolatedWith(accentCol, borderMix).withAlpha(enabled ? 0.95f : 0.42f));
    g.drawRoundedRectangle(bounds, cornerRadius, toggled || isButtonDown ? 1.45f : 1.0f);

    g.setColour(textCol.withAlpha(enabled ? 0.09f : 0.03f));
    g.drawHorizontalLine(roundToInt(bounds.getY() + 1.0f), bounds.getX() + 3.0f, bounds.getRight() - 3.0f);
}

//------------------------------------------------------------------------------
void BranchesLAF::drawButtonText(Graphics& g, TextButton& button, bool isMouseOverButton, bool isButtonDown)
{
    const int inc = isButtonDown ? 1 : 0;
    auto& colours = ::ColourScheme::getInstance().colours;

    auto font = FontManager::getInstance().getBodyFont();
    if (button.getHeight() <= 22)
        font = font.withHeight(jmin(font.getHeight(), 12.0f));

    auto textColour = colours["Text Colour"];
    if (button.getToggleState())
        textColour = textColour.interpolatedWith(colours["Accent Colour"], 0.20f);
    if (isMouseOverButton && button.isEnabled())
        textColour = textColour.brighter(0.10f);
    if (!button.isEnabled())
        textColour = textColour.withAlpha(0.42f);

    g.setFont(font);

    const int yIndent = jmin(4, button.proportionOfHeight(0.3f));
    const int cornerSize = jmin(button.getHeight(), button.getWidth()) / 2;

    const int fontHeight = roundFloatToInt(font.getHeight() * 0.65f);
    const int leftIndent = jmin(fontHeight, 2 + cornerSize / (button.isConnectedOnLeft() ? 4 : 2));
    const int rightIndent = jmin(fontHeight, 2 + cornerSize / (button.isConnectedOnRight() ? 4 : 2));

    g.setColour(Colours::black.withAlpha(button.isEnabled() ? 0.32f : 0.10f));
    g.drawFittedText(button.getButtonText(), leftIndent + inc, yIndent + inc + 1,
                     button.getWidth() - leftIndent - rightIndent, button.getHeight() - yIndent * 2,
                     Justification::centred, 2);

    g.setColour(textColour);
    g.drawFittedText(button.getButtonText(), leftIndent + inc, yIndent + inc,
                     button.getWidth() - leftIndent - rightIndent, button.getHeight() - yIndent * 2,
                     Justification::centred, 2);
}

//------------------------------------------------------------------------------
void BranchesLAF::drawLinearSlider(Graphics& g, int x, int y, int width, int height, float sliderPos,
                                   float minSliderPos, float maxSliderPos, Slider::SliderStyle style, Slider& slider)
{
    if (style != Slider::LinearBar && style != Slider::LinearBarVertical && style != Slider::LinearHorizontal &&
        style != Slider::LinearVertical)
    {
        LookAndFeel_V4::drawLinearSlider(g, x, y, width, height, sliderPos, minSliderPos, maxSliderPos, style, slider);
        return;
    }

    auto& colours = ::ColourScheme::getInstance().colours;
    const bool vertical = style == Slider::LinearVertical || style == Slider::LinearBarVertical;
    const bool barOnly = style == Slider::LinearBar || style == Slider::LinearBarVertical;
    auto bounds = Rectangle<float>((float)x, (float)y, (float)width, (float)height).reduced(1.0f);
    const auto trackThickness = vertical ? jmin(8.0f, bounds.getWidth() * 0.45f) : jmin(8.0f, bounds.getHeight() * 0.45f);
    auto track = vertical
                     ? Rectangle<float>(bounds.getCentreX() - trackThickness * 0.5f, bounds.getY(), trackThickness,
                                        bounds.getHeight())
                     : Rectangle<float>(bounds.getX(), bounds.getCentreY() - trackThickness * 0.5f, bounds.getWidth(),
                                        trackThickness);
    const float radius = trackThickness * 0.5f;
    const bool enabled = slider.isEnabled();

    auto trackBase = colours["Window Background"].darker(0.25f);
    auto accent = colours["Accent Colour"];
    if (slider.isColourSpecified(Slider::trackColourId))
        accent = slider.findColour(Slider::trackColourId);

    g.setColour(Colours::black.withAlpha(enabled ? 0.28f : 0.12f));
    g.fillRoundedRectangle(track.translated(0.0f, 1.0f), radius);

    ColourGradient trough(trackBase.darker(0.18f), track.getX(), track.getY(), trackBase.brighter(0.10f),
                          track.getX(), track.getBottom(), false);
    g.setGradientFill(trough);
    g.fillRoundedRectangle(track, radius);

    const auto clampedPos = vertical ? jlimit(track.getY(), track.getBottom(), sliderPos)
                                     : jlimit(track.getX(), track.getRight(), sliderPos);
    Rectangle<float> fill = vertical ? Rectangle<float>(track.getX(), clampedPos, track.getWidth(),
                                                        track.getBottom() - clampedPos)
                                     : Rectangle<float>(track.getX(), track.getY(), clampedPos - track.getX(),
                                                        track.getHeight());

    if (!fill.isEmpty())
    {
        ColourGradient fillGrad(accent.brighter(0.32f), fill.getX(), fill.getY(), accent.darker(0.14f),
                                vertical ? fill.getX() : fill.getRight(),
                                vertical ? fill.getBottom() : fill.getY(), false);
        fillGrad.addColour(0.52, accent);
        g.setGradientFill(fillGrad);
        g.fillRoundedRectangle(fill, radius);

        g.setColour(accent.withAlpha(enabled ? 0.38f : 0.14f));
        if (vertical)
            g.drawHorizontalLine(roundToInt(fill.getY()), fill.getX() + 1.0f, fill.getRight() - 1.0f);
        else
            g.drawVerticalLine(roundToInt(fill.getRight()) - 1, fill.getY() + 1.0f, fill.getBottom() - 1.0f);
    }

    g.setColour(colours["Plugin Border"].interpolatedWith(accent, slider.isMouseOverOrDragging() ? 0.34f : 0.14f)
                    .withAlpha(enabled ? 0.85f : 0.34f));
    g.drawRoundedRectangle(track, radius, 1.0f);

    if (!barOnly)
    {
        const float thumbSize = vertical ? jmin(bounds.getWidth(), 14.0f) : jmin(bounds.getHeight(), 14.0f);
        Rectangle<float> thumb = vertical
                                     ? Rectangle<float>(bounds.getCentreX() - thumbSize * 0.5f,
                                                        clampedPos - thumbSize * 0.5f, thumbSize, thumbSize)
                                     : Rectangle<float>(clampedPos - thumbSize * 0.5f,
                                                        bounds.getCentreY() - thumbSize * 0.5f, thumbSize, thumbSize);

        g.setColour(accent.withAlpha(slider.isMouseOverOrDragging() ? 0.18f : 0.10f));
        g.fillEllipse(thumb.expanded(3.0f));

        ColourGradient thumbGrad(colours["Button Colour"].brighter(0.25f), thumb.getX(), thumb.getY(),
                                 colours["Button Colour"].darker(0.18f), thumb.getRight(), thumb.getBottom(), true);
        g.setGradientFill(thumbGrad);
        g.fillEllipse(thumb);

        g.setColour(accent.withAlpha(enabled ? 0.75f : 0.28f));
        g.drawEllipse(thumb, 1.0f);
    }
}

//------------------------------------------------------------------------------
void BranchesLAF::drawScrollbarButton(Graphics& g, ScrollBar& /*scrollbar*/, int /*width*/, int /*height*/,
                                      int /*buttonDirection*/, bool /*isScrollbarVertical*/, bool /*isMouseOverButton*/,
                                      bool /*isButtonDown*/)
{
    // Modern scrollbars have no arrow buttons — intentionally empty
}

//------------------------------------------------------------------------------
void BranchesLAF::drawScrollbar(Graphics& g, ScrollBar& /*scrollbar*/, int x, int y, int width, int height,
                                bool isScrollbarVertical, int thumbStartPosition, int thumbSize, bool isMouseOver,
                                bool isMouseDown)
{
    auto& colours = ::ColourScheme::getInstance().colours;

    // Transparent track — no background fill, just the thumb
    const float thumbInset = 1.0f;
    const float cornerRadius = 3.0f;

    // Thumb color: subtle when idle, accent-tinted on hover/drag
    Colour thumbCol;
    if (isMouseDown)
        thumbCol = colours["Menu Selection Colour"].withAlpha(0.7f);
    else if (isMouseOver)
        thumbCol = colours["Text Colour"].withAlpha(0.35f);
    else
        thumbCol = colours["Text Colour"].withAlpha(0.18f);

    if (thumbSize > 0)
    {
        Rectangle<float> thumbBounds;
        if (isScrollbarVertical)
        {
            thumbBounds =
                Rectangle<float>(static_cast<float>(x) + thumbInset, static_cast<float>(thumbStartPosition),
                                 static_cast<float>(width) - thumbInset * 2.0f, static_cast<float>(thumbSize));
        }
        else
        {
            thumbBounds =
                Rectangle<float>(static_cast<float>(thumbStartPosition), static_cast<float>(y) + thumbInset,
                                 static_cast<float>(thumbSize), static_cast<float>(height) - thumbInset * 2.0f);
        }

        g.setColour(thumbCol);
        g.fillRoundedRectangle(thumbBounds, cornerRadius);
    }
}

//------------------------------------------------------------------------------
void BranchesLAF::drawMenuBarBackground(Graphics& g, int width, int height, bool isMouseOverBar,
                                        MenuBarComponent& menuBar)
{
    auto& colours = ::ColourScheme::getInstance().colours;
    Colour bgCol = colours["Window Background"];

    // Subtle metallic gradient (refined, not extreme)
    ColourGradient grad(bgCol.brighter(0.08f), 0.0f, 0.0f, bgCol.darker(0.04f), 0.0f, (float)height, false);
    g.setGradientFill(grad);
    g.fillRect(0.0f, 0.0f, (float)width, (float)height);

    // Top edge highlight (metallic sheen)
    g.setColour(Colours::white.withAlpha(0.06f));
    g.drawHorizontalLine(0, 0.0f, (float)width);

    // Bottom edge shadow (separation line)
    g.setColour(Colour(0x35000000));
    g.drawHorizontalLine(height - 1, 0.0f, (float)width);
}

//------------------------------------------------------------------------------
Font BranchesLAF::getMenuBarFont(MenuBarComponent& menuBar, int itemIndex, const String& itemText)
{
    return FontManager::getInstance().getBodyFont();
}

//------------------------------------------------------------------------------
void BranchesLAF::drawMenuBarItem(Graphics& g, int width, int height, int itemIndex, const String& itemText,
                                  bool isMouseOverItem, bool isMenuOpen, bool isMouseOverBar, MenuBarComponent& menuBar)
{
    map<String, Colour>& colours = ::ColourScheme::getInstance().colours;

    if (!menuBar.isEnabled())
    {
        g.setColour(colours["Text Colour"].withMultipliedAlpha(0.5f));
    }
    else if (isMenuOpen || isMouseOverItem)
    {
        g.fillAll(colours["Menu Selection Colour"]);
        g.setColour(colours["Menu Selection Colour"].contrasting());
    }
    else
    {
        g.setColour(colours["Text Colour"]);
    }

    g.setFont(getMenuBarFont(menuBar, itemIndex, itemText));
    g.drawFittedText(itemText, 0, 0, width, height, Justification::centred, 1);
}

//------------------------------------------------------------------------------
int BranchesLAF::getMenuBarItemWidth(MenuBarComponent& menuBar, int itemIndex, const String& itemText)
{
    return roundToInt(GlyphArrangement::getStringWidth(getMenuBarFont(menuBar, itemIndex, itemText), itemText))
           + menuBar.getHeight() - 8;
}

//------------------------------------------------------------------------------
void BranchesLAF::drawPopupMenuBackground(Graphics& g, int width, int height)
{
    auto& colours = ::ColourScheme::getInstance().colours;
    Colour bgCol = colours["Window Background"];

    // Rounded background with subtle gradient
    Rectangle<float> bounds(0.0f, 0.0f, (float)width, (float)height);
    float cornerRadius = 6.0f;

    ColourGradient grad(bgCol, 0.0f, 0.0f, bgCol.darker(0.05f), 0.0f, (float)height, false);
    g.setGradientFill(grad);
    g.fillRoundedRectangle(bounds, cornerRadius);

    // Inner glow at top
    g.setColour(Colours::white.withAlpha(0.06f));
    g.drawHorizontalLine(2, 4.0f, (float)width - 4.0f);

    // Crisp rounded border
    g.setColour(Colour(0x50000000));
    g.drawRoundedRectangle(bounds.reduced(0.5f), cornerRadius, 1.0f);
}

//------------------------------------------------------------------------------
const Drawable* BranchesLAF::getDefaultFolderImage()
{
    static DrawableImage im;

    if (im.getImage().isNull())
        im.setImage(ImageCache::getFromMemory(LookAndFeelImages::lookandfeelfolder_32_png,
                                              LookAndFeelImages::lookandfeelfolder_32_pngSize));

    return &im;
}

//------------------------------------------------------------------------------
void BranchesLAF::drawComboBox(Graphics& g, int width, int height, bool isButtonDown, int /*buttonX*/, int /*buttonY*/,
                               int /*buttonW*/, int /*buttonH*/, ComboBox& box)
{
    Rectangle<float> bounds(0.0f, 0.0f, (float)width, (float)height);
    float cornerRadius = (float)height * 0.3f;

    // Background fill with subtle gradient
    Colour bgCol = box.findColour(ComboBox::backgroundColourId);
    ColourGradient bgGrad(bgCol.brighter(0.04f), 0.0f, 0.0f, bgCol.darker(0.04f), 0.0f, (float)height, false);
    g.setGradientFill(bgGrad);
    g.fillRoundedRectangle(bounds, cornerRadius);

    // Press darkening
    if (isButtonDown)
    {
        g.setColour(Colours::black.withAlpha(0.1f));
        g.fillRoundedRectangle(bounds, cornerRadius);
    }

    // Border — accent when focused, subtle otherwise
    if (box.isEnabled() && box.hasKeyboardFocus(false))
    {
        g.setColour(box.findColour(ComboBox::focusedOutlineColourId).withAlpha(0.6f));
        g.drawRoundedRectangle(bounds.reduced(0.5f), cornerRadius, 1.5f);
    }
    else
    {
        g.setColour(box.findColour(ComboBox::outlineColourId));
        g.drawRoundedRectangle(bounds.reduced(0.5f), cornerRadius, 1.0f);
    }

    // Chevron arrow on right side
    if (box.isEnabled())
    {
        float arrowZone = 20.0f;
        float arrowX = (float)width - arrowZone;
        float arrowCentreY = (float)height * 0.5f;
        float arrowW = 7.0f;
        float arrowH = 4.0f;

        Path chevron;
        chevron.startNewSubPath(arrowX, arrowCentreY - arrowH * 0.5f);
        chevron.lineTo(arrowX + arrowW * 0.5f, arrowCentreY + arrowH * 0.5f);
        chevron.lineTo(arrowX + arrowW, arrowCentreY - arrowH * 0.5f);

        g.setColour(box.findColour(ComboBox::arrowColourId).withAlpha(0.5f));
        g.strokePath(chevron, PathStrokeType(1.5f, PathStrokeType::curved, PathStrokeType::rounded));
    }
}

//------------------------------------------------------------------------------
void BranchesLAF::drawProgressBar(Graphics& g, ProgressBar& progressBar, int width, int height, double progress,
                                  const String& textToShow)
{
    auto& colours = ::ColourScheme::getInstance().colours;
    float cornerRadius = 4.0f;
    Rectangle<float> bounds(0.0f, 0.0f, (float)width, (float)height);

    // === Track background (recessed look) ===
    Colour trackTop = colours["Window Background"].darker(0.3f);
    Colour trackBottom = colours["Window Background"].darker(0.15f);
    ColourGradient trackGrad(trackTop, 0.0f, 0.0f, trackBottom, 0.0f, (float)height, false);
    g.setGradientFill(trackGrad);
    g.fillRoundedRectangle(bounds, cornerRadius);

    // Inner shadow at top for recessed feel
    g.setColour(Colour(0x25000000));
    g.drawHorizontalLine(1, 2.0f, (float)width - 2.0f);

    // Border
    g.setColour(Colour(0x40000000));
    g.drawRoundedRectangle(bounds.reduced(0.5f), cornerRadius, 1.0f);

    // === Progress fill (LED glow style) ===
    if (progress > 0.0)
    {
        float fillWidth = jmax(cornerRadius * 2.0f, (float)(width - 2) * (float)progress);
        Rectangle<float> fillBounds(1.0f, 1.0f, fillWidth, (float)height - 2.0f);

        Colour meterCol = colours["CPU Meter Colour"];

        // Main fill gradient
        ColourGradient fillGrad(meterCol.brighter(0.2f), 0.0f, fillBounds.getY(), meterCol.darker(0.1f), 0.0f,
                                fillBounds.getBottom(), false);
        g.setGradientFill(fillGrad);
        g.fillRoundedRectangle(fillBounds, cornerRadius - 1.0f);

        // Sheen overlay (metallic shine)
        ColourGradient sheen(Colours::white.withAlpha(0.2f), 0.0f, fillBounds.getY(), Colours::white.withAlpha(0.0f),
                             0.0f, fillBounds.getCentreY(), false);
        g.setGradientFill(sheen);
        g.fillRoundedRectangle(fillBounds.reduced(1.0f), cornerRadius - 2.0f);

        // Glow at right edge (LED effect)
        g.setColour(meterCol.brighter(0.5f).withAlpha(0.6f));
        g.drawVerticalLine((int)fillBounds.getRight() - 1, fillBounds.getY() + 2.0f, fillBounds.getBottom() - 2.0f);
    }

    // === Text ===
    if (textToShow.isNotEmpty())
    {
        g.setColour(colours["Text Colour"]);
        g.drawText(textToShow, bounds.toNearestInt(), Justification::centred, true);
    }
}

//------------------------------------------------------------------------------
void BranchesLAF::drawKeymapChangeButton(Graphics& g, int width, int height, Button& button,
                                         const String& keyDescription)
{
    map<String, Colour>& colours = ::ColourScheme::getInstance().colours;

    if (keyDescription.isNotEmpty())
    {
        drawButtonBackground(g, button, colours["Button Colour"], button.isOver(), button.isDown());

        g.setColour(colours["Text Colour"]);
        g.setFont(Font(FontOptions().withHeight(height * 0.6f)));
        g.drawFittedText(keyDescription, 3, 0, width - 6, height, Justification::centred, 1);
    }
    else
    {
        const float thickness = 7.0f;
        const float indent = 22.0f;

        Path p;
        p.addEllipse(0.0f, 0.0f, 100.0f, 100.0f);
        p.addRectangle(indent, 50.0f - thickness, 100.0f - indent * 2.0f, thickness * 2.0f);
        p.addRectangle(50.0f - thickness, indent, thickness * 2.0f, 50.0f - indent - thickness);
        p.addRectangle(50.0f - thickness, 50.0f + thickness, thickness * 2.0f, 50.0f - indent - thickness);
        p.setUsingNonZeroWinding(false);

        g.setColour(colours["Text Colour"].withAlpha(button.isDown() ? 0.7f : (button.isOver() ? 0.5f : 0.3f)));
        g.fillPath(p, p.getTransformToScaleToFit(2.0f, 2.0f, width - 4.0f, height - 4.0f, true));
    }
}

//------------------------------------------------------------------------------
void BranchesLAF::drawLabel(Graphics& g, Label& label)
{
    g.fillAll(label.findColour(Label::backgroundColourId));

    if (!label.isBeingEdited())
    {
        const float alpha = label.isEnabled() ? 1.0f : 0.5f;

        g.setColour(::ColourScheme::getInstance().colours["Text Colour"]);
        g.setFont(label.getFont());
        g.drawFittedText(label.getText(), label.getBorderSize().getLeft(), label.getBorderSize().getTop(),
                         label.getWidth() - 2 * label.getBorderSize().getLeft(),
                         label.getHeight() - 2 * label.getBorderSize().getTop(), label.getJustificationType(),
                         jmax(1, (int)(label.getHeight() / label.getFont().getHeight())),
                         label.getMinimumHorizontalScale());

        g.setColour(label.findColour(Label::outlineColourId).withMultipliedAlpha(alpha));
        g.drawRect(0, 0, label.getWidth(), label.getHeight());
    }
    else if (label.isEnabled())
    {
        g.setColour(label.findColour(Label::outlineColourId));
        g.drawRect(0, 0, label.getWidth(), label.getHeight());
    }
}

//------------------------------------------------------------------------------
void BranchesLAF::drawToggleButton(Graphics& g, ToggleButton& button, bool isMouseOverButton, bool isButtonDown)
{
    if (button.hasKeyboardFocus(true))
    {
        g.setColour(::ColourScheme::getInstance().colours["List Selected Colour"]);
        g.drawRect(0, 0, button.getWidth(), button.getHeight());
    }

    float fontSize = jmin(15.0f, button.getHeight() * 0.75f);
    const float tickWidth = fontSize * 1.1f;

    drawTickBox(g, button, 4.0f, (button.getHeight() - tickWidth) * 0.5f, tickWidth, tickWidth, button.getToggleState(),
                button.isEnabled(), isMouseOverButton, isButtonDown);

    // JUCE 8: setColour MUST come before setFont
    g.setColour(::ColourScheme::getInstance().colours["Text Colour"]);
    g.setFont(Font(FontOptions().withHeight(fontSize)));

    if (!button.isEnabled())
        g.setOpacity(0.5f);

    const int textX = (int)tickWidth + 5;

    g.drawFittedText(button.getButtonText(), textX, 0, button.getWidth() - textX - 2, button.getHeight(),
                     Justification::centredLeft, 10);
}

//------------------------------------------------------------------------------
void BranchesLAF::drawTickBox(Graphics& g, Component& component, float x, float y, float w, float h, bool ticked,
                              bool isEnabled, bool isMouseOverButton, bool isButtonDown)
{
    auto& colours = ::ColourScheme::getInstance().colours;
    float boxSize = w * 0.75f;
    float boxX = x;
    float boxY = y + (h - boxSize) * 0.5f;
    float cornerRadius = 4.0f;

    Rectangle<float> boxBounds(boxX, boxY, boxSize, boxSize);

    // === Box background (recessed) ===
    Colour boxCol = colours["Tick Box Colour"];
    if (!isEnabled)
        boxCol = boxCol.withAlpha(0.5f);

    // Gradient fill for depth
    ColourGradient boxGrad(boxCol.brighter(0.1f), boxX, boxY, boxCol.darker(0.15f), boxX, boxY + boxSize, false);
    g.setGradientFill(boxGrad);
    g.fillRoundedRectangle(boxBounds, cornerRadius);

    // Border
    g.setColour(Colour(0x50000000));
    g.drawRoundedRectangle(boxBounds, cornerRadius, 1.0f);

    // Hover glow
    if (isMouseOverButton && isEnabled)
    {
        g.setColour(colours["Button Highlight"].withAlpha(0.4f));
        g.drawRoundedRectangle(boxBounds.reduced(0.5f), cornerRadius - 0.5f, 1.5f);
    }

    // === Checkmark ===
    if (ticked)
    {
        Colour tickCol = colours["Audio Connection"]; // Bright accent color for visibility
        if (!isEnabled)
            tickCol = tickCol.withAlpha(0.4f);

        // Draw a clean, bold checkmark
        Path tick;
        float cx = boxX + boxSize * 0.5f;
        float cy = boxY + boxSize * 0.5f;
        float scale = boxSize * 0.35f;

        tick.startNewSubPath(cx - scale * 0.7f, cy);
        tick.lineTo(cx - scale * 0.15f, cy + scale * 0.55f);
        tick.lineTo(cx + scale * 0.7f, cy - scale * 0.5f);

        g.setColour(tickCol);
        g.strokePath(tick, PathStrokeType(2.5f, PathStrokeType::curved, PathStrokeType::rounded));
    }
}

//------------------------------------------------------------------------------
void BranchesLAF::fillTextEditorBackground(Graphics& g, int width, int height, TextEditor& textEditor)
{
    auto bg = textEditor.findColour(TextEditor::backgroundColourId);
    float cr = std::min(height * 0.5f, 14.0f);
    g.setColour(bg);
    g.fillRoundedRectangle(0.0f, 0.0f, (float)width, (float)height, cr);
}

//------------------------------------------------------------------------------
void BranchesLAF::drawTextEditorOutline(Graphics& g, int width, int height, TextEditor& textEditor)
{
    float cr = std::min(height * 0.5f, 14.0f);
    bool focused = textEditor.hasKeyboardFocus(true);
    auto outline = textEditor.findColour(focused ? TextEditor::focusedOutlineColourId : TextEditor::outlineColourId);

    if (!outline.isTransparent())
    {
        g.setColour(focused ? outline.withAlpha(0.7f) : outline);
        g.drawRoundedRectangle(0.5f, 0.5f, (float)width - 1.0f, (float)height - 1.0f, cr, 1.0f);
    }
}

//------------------------------------------------------------------------------
void BranchesLAF::drawCallOutBoxBackground(CallOutBox& box, Graphics& g, const Path& path, Image& cachedImage)
{
    Image content(Image::ARGB, box.getWidth(), box.getHeight(), true);

    {
        Graphics g2(content);

        g2.setColour(::ColourScheme::getInstance().colours["Window Background"].withAlpha(0.9f));
        g2.fillPath(path);

        g2.setColour(Colours::black.withAlpha(0.8f));
        g2.strokePath(path, PathStrokeType(2.0f));
    }

    DropShadowEffect shadow;
    DropShadow shad(Colours::black.withAlpha(0.5f), 5, Point<int>(2, 2));
    shadow.setShadowProperties(shad);
    shadow.applyEffect(content, g, 1.0f, 1.0f);
}

//------------------------------------------------------------------------------
void BranchesLAF::drawAlertBox(Graphics& g, AlertWindow& alert, const Rectangle<int>& textArea, TextLayout& textLayout)
{
    // Get colors from the colour scheme
    map<String, Colour>& colours = ::ColourScheme::getInstance().colours;

    // Draw background
    g.fillAll(colours["Window Background"]);

    // Draw a subtle border
    g.setColour(Colours::black.withAlpha(0.3f));
    g.drawRect(alert.getLocalBounds(), 1);

    // Get the icon component if present
    auto bounds = alert.getLocalBounds().reduced(1);
    int iconSpaceUsed = 0;

    // Check for icon - JUCE AlertWindows typically have an icon on the left
    auto iconType = alert.getAlertType();
    if (iconType != AlertWindow::NoIcon)
    {
        // Draw the icon
        Path icon;
        uint32 colour = 0x60AAAAAA;
        constexpr int iconSize = 80;

        if (iconType == AlertWindow::WarningIcon)
        {
            colour = 0x55ff5555;
            icon.addTriangle(iconSize * 0.5f, 0.0f, iconSize * 1.0f, iconSize * 0.866f, 0.0f, iconSize * 0.866f);
            icon.addEllipse(iconSize * 0.42f, iconSize * 0.6f, iconSize * 0.16f, iconSize * 0.16f);
            icon.addRectangle(iconSize * 0.45f, iconSize * 0.25f, iconSize * 0.1f, iconSize * 0.3f);
        }
        else if (iconType == AlertWindow::InfoIcon)
        {
            colour = 0x605555ff;
            icon.addEllipse(0.0f, 0.0f, iconSize, iconSize);
            icon.addRectangle(iconSize * 0.4f, iconSize * 0.25f, iconSize * 0.2f, iconSize * 0.15f);
            icon.addRectangle(iconSize * 0.4f, iconSize * 0.45f, iconSize * 0.2f, iconSize * 0.35f);
        }
        else if (iconType == AlertWindow::QuestionIcon)
        {
            colour = 0x60AAAAAA;
            icon.addEllipse(0.0f, 0.0f, iconSize, iconSize);
            icon.addEllipse(iconSize * 0.42f, iconSize * 0.72f, iconSize * 0.16f, iconSize * 0.16f);

            Path q;
            q.addEllipse(iconSize * 0.22f, iconSize * 0.13f, iconSize * 0.56f, iconSize * 0.42f);
            q.addRectangle(iconSize * 0.4f, iconSize * 0.45f, iconSize * 0.2f, iconSize * 0.2f);
            icon.addPath(q, AffineTransform::rotation(0.15f, iconSize * 0.5f, iconSize * 0.5f));
        }

        const Rectangle<int> iconRect(8, bounds.getY() + 8, iconSize, iconSize);
        icon.applyTransform(icon.getTransformToScaleToFit(iconRect.toFloat(), true));
        g.setColour(Colour(colour));
        g.fillPath(icon);

        iconSpaceUsed = iconRect.getRight();
    }

    // Draw the text layout - this replaces the duplicate drawing issue
    g.setColour(colours["Text Colour"]);
    textLayout.draw(g, textArea.toFloat());
}
