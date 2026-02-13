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
    /*setColour(TextButton::buttonColourId, Colour(0xFFEEECE1));
    setColour(TextButton::buttonOnColourId, Colour(0xFFEEECE1));
    setColour(PopupMenu::highlightedBackgroundColourId, Colour(0x40000000));
    setColour(PopupMenu::backgroundColourId, Colour(0xFFEEECE1));
    setColour(AlertWindow::backgroundColourId, Colour(0xFFEEECE1));
    setColour(ComboBox::buttonColourId, Colour(0xFFEEECE1));
    setColour(TextEditor::highlightColourId, Colour(0xB0B0B0FF));
    setColour(TextEditor::focusedOutlineColourId, Colour(0x40000000));
    setColour(DirectoryContentsDisplayComponent::highlightColourId, Colour(0xFFD7D1B5));
    setColour(ProgressBar::backgroundColourId, Colour(0xFFD7D1B5));
    setColour(ProgressBar::foregroundColourId, Colour(0xB0B0B0FF));*/

    map<String, Colour>& colours = ::ColourScheme::getInstance().colours;

    setColour(TextButton::buttonColourId, colours["Button Colour"]);
    setColour(TextButton::buttonOnColourId, colours["Button Colour"]);
    setColour(PopupMenu::highlightedBackgroundColourId, colours["Menu Selection Colour"]);
    setColour(PopupMenu::backgroundColourId, colours["Window Background"]);
    setColour(AlertWindow::backgroundColourId, colours["Window Background"]);
    setColour(ComboBox::buttonColourId, colours["Button Colour"]);
    setColour(TextEditor::highlightColourId, colours["Button Highlight"]);
    setColour(TextEditor::focusedOutlineColourId, colours["Menu Selection Colour"]);
    setColour(DirectoryContentsDisplayComponent::highlightColourId, colours["List Selected Colour"]);
    setColour(ProgressBar::backgroundColourId, colours["Window Background"]);
    setColour(ProgressBar::foregroundColourId, colours["CPU Meter Colour"]);

    // Fix for "pale on pale" text (Menu Visibility):
    setColour(PopupMenu::textColourId, colours["Text Colour"]);
    setColour(PopupMenu::highlightedTextColourId, colours["Text Colour"]);
    setColour(TextButton::textColourOnId, colours["Text Colour"]);
    setColour(TextButton::textColourOffId, colours["Text Colour"]);
    setColour(ComboBox::textColourId, colours["Text Colour"]);
    setColour(Label::textColourId, colours["Text Colour"]);

    // ToggleButton colors (needed for JUCE 8 compatibility)
    setColour(ToggleButton::textColourId, colours["Text Colour"]);
    setColour(ToggleButton::tickColourId, colours["Vector Colour"]);
    setColour(ToggleButton::tickDisabledColourId, colours["Tick Box Colour"]);
}

//------------------------------------------------------------------------------
BranchesLAF::~BranchesLAF() {}

//------------------------------------------------------------------------------
void BranchesLAF::refreshColours()
{
    map<String, Colour>& colours = ::ColourScheme::getInstance().colours;

    setColour(TextButton::buttonColourId, colours["Button Colour"]);
    setColour(TextButton::buttonOnColourId, colours["Button Colour"]);
    setColour(PopupMenu::highlightedBackgroundColourId, colours["Menu Selection Colour"]);
    setColour(PopupMenu::backgroundColourId, colours["Window Background"]);
    setColour(AlertWindow::backgroundColourId, colours["Window Background"]);
    setColour(ComboBox::buttonColourId, colours["Button Colour"]);
    setColour(TextEditor::highlightColourId, colours["Button Highlight"]);
    setColour(TextEditor::focusedOutlineColourId, colours["Menu Selection Colour"]);
    setColour(ProgressBar::backgroundColourId, colours["Window Background"]);
    setColour(ProgressBar::foregroundColourId, colours["CPU Meter Colour"]);

    setColour(PopupMenu::textColourId, colours["Text Colour"]);
    setColour(PopupMenu::highlightedTextColourId, colours["Text Colour"]);
    setColour(TextButton::textColourOnId, colours["Text Colour"]);
    setColour(TextButton::textColourOffId, colours["Text Colour"]);
    setColour(ComboBox::textColourId, colours["Text Colour"]);
    setColour(Label::textColourId, colours["Text Colour"]);

    // ToggleButton colors (needed for JUCE 8 compatibility)
    setColour(ToggleButton::textColourId, colours["Text Colour"]);
    setColour(ToggleButton::tickColourId, colours["Vector Colour"]);
    setColour(ToggleButton::tickDisabledColourId, colours["Tick Box Colour"]);
}

//------------------------------------------------------------------------------
void BranchesLAF::drawButtonBackground(Graphics& g, Button& button, const Colour& backgroundColour,
                                       bool isMouseOverButton, bool isButtonDown)
{
    auto& colours = ::ColourScheme::getInstance().colours;
    Colour buttonCol = colours["Button Colour"];
    Colour accentCol = colours["Audio Connection"];

    float w = (float)button.getWidth();
    float h = (float)button.getHeight();
    float cornerRadius = 6.0f;

    Rectangle<float> bounds(1.0f, 1.0f, w - 2.0f, h - 2.0f);

    // === Main fill ===
    Colour fillCol;
    if (isButtonDown)
        fillCol = buttonCol.darker(0.3f);
    else if (isMouseOverButton)
        fillCol = buttonCol.brighter(0.15f);
    else
        fillCol = buttonCol;

    // Strong top-to-bottom gradient
    ColourGradient mainGrad(fillCol.brighter(0.25f), 0.0f, bounds.getY(), fillCol.darker(0.2f), 0.0f,
                            bounds.getBottom(), false);
    g.setGradientFill(mainGrad);
    g.fillRoundedRectangle(bounds, cornerRadius);

    // === Glossy top highlight (very visible) ===
    if (!isButtonDown)
    {
        Rectangle<float> glossArea(bounds.getX(), bounds.getY(), bounds.getWidth(), bounds.getHeight() * 0.45f);
        ColourGradient glossGrad(Colours::white.withAlpha(0.35f), 0.0f, glossArea.getY(),
                                 Colours::white.withAlpha(0.0f), 0.0f, glossArea.getBottom(), false);
        g.setGradientFill(glossGrad);
        g.fillRoundedRectangle(glossArea.reduced(2.0f, 0.0f), cornerRadius - 1.0f);
    }

    // === Border ===
    bool isToggled = button.getToggleState();

    if (isMouseOverButton && !isButtonDown)
    {
        // Glowing accent border on hover
        g.setColour(accentCol.withAlpha(0.8f));
        g.drawRoundedRectangle(bounds, cornerRadius, 2.0f);

        // Outer glow
        g.setColour(accentCol.withAlpha(0.3f));
        g.drawRoundedRectangle(bounds.expanded(1.0f), cornerRadius + 1.0f, 2.0f);
    }
    else if (isButtonDown || isToggled)
    {
        // Bright accent border when pressed or toggled on
        g.setColour(accentCol);
        g.drawRoundedRectangle(bounds, cornerRadius, 2.0f);
    }
    else
    {
        // Normal border
        g.setColour(Colour(0x60000000));
        g.drawRoundedRectangle(bounds, cornerRadius, 1.0f);
    }
}

//------------------------------------------------------------------------------
void BranchesLAF::drawButtonText(Graphics& g, TextButton& button, bool isMouseOverButton, bool isButtonDown)
{
    int inc;

    g.setFont(FontManager::getInstance().getUIFont(15.0f));
    g.setColour(
        ::ColourScheme::getInstance().colours["Text Colour"].withMultipliedAlpha(button.isEnabled() ? 1.0f : 0.5f));

    const int yIndent = jmin(4, button.proportionOfHeight(0.3f));
    const int cornerSize = jmin(button.getHeight(), button.getWidth()) / 2;

    const int fontHeight = roundFloatToInt(FontManager::getInstance().getUIFont(15.0f).getHeight() * 0.6f);
    const int leftIndent = jmin(fontHeight, 2 + cornerSize / (button.isConnectedOnLeft() ? 4 : 2));
    const int rightIndent = jmin(fontHeight, 2 + cornerSize / (button.isConnectedOnRight() ? 4 : 2));

    if (isButtonDown)
        inc = 1;
    else
        inc = 0;

    g.drawFittedText(button.getButtonText(), leftIndent + inc, yIndent + inc,
                     button.getWidth() - leftIndent - rightIndent, button.getHeight() - yIndent * 2,
                     Justification::centred, 2);
}

//------------------------------------------------------------------------------
void BranchesLAF::drawScrollbarButton(Graphics& g, ScrollBar& scrollbar, int width, int height, int buttonDirection,
                                      bool isScrollbarVertical, bool isMouseOverButton, bool isButtonDown)
{
    float inc;
    Path highlight, shadow, tri;
    map<String, Colour>& colours = ::ColourScheme::getInstance().colours;

    if (!isScrollbarVertical)
    {
        // Background behind the button.
        // GradientBrush grad(Colour(0xFFDADCC6), 0.0f, 0.0f, Colour(0xFFEEECE1), 0.0f, (float)height, false);
        ColourGradient grad(colours["Window Background"].darker(0.25f), 0.0f, 0.0f, colours["Window Background"], 0.0f,
                            (float)height, false);

        if (buttonDirection == 3)
        {
            // g.setBrush(&grad);
            g.setGradientFill(grad);
            g.fillRect((float)3, (float)0, (float)width, (float)height);

            g.setColour(Colour(0x30000000));
            g.fillRect((float)width - 3.0f, (float)0, 3.0f, 1.0f);
            g.fillRect((float)width - 3.0f, (float)height - 1.0f, 3.0f, 1.0f);
        }
        else if (buttonDirection == 1)
        {
            // g.setBrush(&grad);
            g.setGradientFill(grad);
            g.fillRect((float)0, (float)0, (float)width - 3.0f, (float)height);

            g.setColour(Colour(0x30000000));
            g.fillRect((float)0, (float)0, 3.0f, 1.0f);
            g.fillRect((float)0, (float)height - 1.0f, 3.0f, 1.0f);
        }

        g.setColour(Colour(0x80000000));
        g.drawRoundedRectangle(1.0f, 1.0f, (float)(width - 2), (float)(height - 2), 2.0f, 1.0f);
        g.setColour(colours["Button Colour"]);
        g.fillRoundedRectangle(1.0f, 1.0f, (float)(width - 2), (float)(height - 2), 2.0f);

        g.setColour(Colour(0x08000000));
        g.fillRoundedRectangle(1.0f, (float)(height / 2), (float)(width - 2), (float)(height - 2), 2.0f);

        if (isMouseOverButton)
        {
            // Draw highlight.
            highlight.startNewSubPath(2.0f, (float)(height - 3));
            highlight.lineTo(2.0f, 4.0f);
            highlight.quadraticTo(2.0f, 2.0f, 4.0f, 2.0f);
            highlight.lineTo((float)(width - 3), 2.0f);
            if (!isButtonDown)
            {
                g.setColour(Colour(0xB0FFFFFF));
                g.strokePath(highlight, PathStrokeType(2.0f, PathStrokeType::curved, PathStrokeType::rounded));
            }
            else
            {
                g.setColour(Colour(0x20000000));
                g.strokePath(highlight, PathStrokeType(1.0f, PathStrokeType::curved, PathStrokeType::rounded));
            }

            // Draw shadow.
            shadow.startNewSubPath(3.0f, (float)(height - 2));
            shadow.lineTo((float)(width - 4), (float)(height - 2));
            shadow.quadraticTo((float)(width - 2), (float)(height - 2), (float)(width - 2), (float)(height - 4));
            shadow.lineTo((float)(width - 2), 3.0f);
            if (!isButtonDown)
            {
                g.setColour(Colour(0x20000000));
                g.strokePath(shadow, PathStrokeType(1.0f, PathStrokeType::curved, PathStrokeType::rounded));
            }
            else
            {
                g.setColour(Colour(0xB0FFFFFF));
                g.strokePath(shadow, PathStrokeType(2.0f, PathStrokeType::curved, PathStrokeType::rounded));
            }
        }
    }
    else
    {
        // Background behind the button.
        // GradientBrush grad(Colour(0xFFDADCC6), 0.0f, 0.0f, Colour(0xFFEEECE1), (float)width, 0.0f, false);
        ColourGradient grad(colours["Window Background"].darker(0.25f), 0.0f, 0.0f, colours["Window Background"],
                            (float)width, 0.0f, false);

        if (buttonDirection == 2)
        {
            // g.setBrush(&grad);
            g.setGradientFill(grad);
            g.fillRect((float)0, (float)0, (float)width, (float)height - 3.0f);

            g.setColour(Colour(0x30000000));
            g.fillRect((float)0, (float)0, 1.0f, 3.0f);
            g.fillRect((float)width - 1.0f, 0.0f, 1.0f, 3.0f);
        }
        else if (buttonDirection == 0)
        {
            // g.setBrush(&grad);
            g.setGradientFill(grad);
            g.fillRect((float)0, (float)3, (float)width, (float)height);

            g.setColour(Colour(0x30000000));
            g.fillRect((float)0, (float)height - 3.0f, 1.0f, 3.0f);
            g.fillRect((float)width - 1.0f, (float)height - 3.0f, 1.0f, 3.0f);
        }

        g.setColour(Colour(0x80000000));
        g.drawRoundedRectangle(1.0f, 1.0f, (float)(width - 2), (float)(height - 2), 2.0f, 1.0f);
        g.setColour(colours["Button Colour"]);
        g.fillRoundedRectangle(1.0f, 1.0f, (float)(width - 2), (float)(height - 2), 2.0f);

        g.setColour(Colour(0x08000000));
        g.fillRoundedRectangle((float)(width / 2), 1.0f, (float)(width - 2), (float)(height - 2), 2.0f);

        if (isMouseOverButton)
        {
            // Draw highlight.
            highlight.startNewSubPath(2.0f, (float)(height - 3));
            highlight.lineTo(2.0f, 4.0f);
            highlight.quadraticTo(2.0f, 2.0f, 4.0f, 2.0f);
            highlight.lineTo((float)(width - 3), 2.0f);
            if (!isButtonDown)
            {
                g.setColour(Colour(0xB0FFFFFF));
                g.strokePath(highlight, PathStrokeType(2.0f, PathStrokeType::curved, PathStrokeType::rounded));
            }
            else
            {
                g.setColour(Colour(0x20000000));
                g.strokePath(highlight, PathStrokeType(1.0f, PathStrokeType::curved, PathStrokeType::rounded));
            }

            // Draw shadow.
            shadow.startNewSubPath(3.0f, (float)(height - 2));
            shadow.lineTo((float)(width - 4), (float)(height - 2));
            shadow.quadraticTo((float)(width - 2), (float)(height - 2), (float)(width - 2), (float)(height - 4));
            shadow.lineTo((float)(width - 2), 3.0f);
            if (!isButtonDown)
            {
                g.setColour(Colour(0x20000000));
                g.strokePath(shadow, PathStrokeType(1.0f, PathStrokeType::curved, PathStrokeType::rounded));
            }
            else
            {
                g.setColour(Colour(0xB0FFFFFF));
                g.strokePath(shadow, PathStrokeType(2.0f, PathStrokeType::curved, PathStrokeType::rounded));
            }
        }
    }

    // Draw triangle.
    if (isButtonDown)
        inc = 1.0f;
    else
        inc = 0.0f;
    switch (buttonDirection)
    {
    case 0:
        tri.startNewSubPath((float)(width / 2) - (float)(width / 4) + inc,
                            (float)(height / 2) + (float)(height / 8) + inc);
        tri.lineTo((float)(width / 2) + inc, (float)(height / 2) - (float)(height / 8) + inc);
        tri.lineTo((float)(width / 2) + (float)(width / 4) + inc, (float)(height / 2) + (float)(height / 8) + inc);
        break;
    case 1:
        tri.startNewSubPath((float)(width / 2) - (float)(width / 8) + inc,
                            (float)(height / 2) - (float)(height / 4) + inc);
        tri.lineTo((float)(width / 2) + (float)(width / 8) + inc, (float)(height / 2) + inc);
        tri.lineTo((float)(width / 2) - (float)(width / 8) + inc, (float)(height / 2) + (float)(height / 4) + inc);
        break;
    case 2:
        tri.startNewSubPath((float)(width / 2) - (float)(width / 4) + inc,
                            (float)(height / 2) - (float)(height / 8) + inc);
        tri.lineTo((float)(width / 2) + inc, (float)(height / 2) + (float)(height / 8) + inc);
        tri.lineTo((float)(width / 2) + (float)(width / 4) + inc, (float)(height / 2) - (float)(height / 8) + inc);
        break;
    case 3:
        /*tri.addTriangle((float)(width/2)+(float)(width/6),
                        (float)(height/2)+(float)(height/4),
                        (float)(width/2)-(float)(width/6),
                        (float)(height/2),
                        (float)(width/2)+(float)(width/6),
                        (float)(height/2)-(float)(height/4));*/

        tri.startNewSubPath((float)(width / 2) + (float)(width / 8) + inc,
                            (float)(height / 2) + (float)(height / 4) + inc);
        tri.lineTo((float)(width / 2) - (float)(width / 8) + inc, (float)(height / 2) + inc);
        tri.lineTo((float)(width / 2) + (float)(width / 8) + inc, (float)(height / 2) - (float)(height / 4) + inc);
        break;
    }
    g.setColour(colours["Vector Colour"]);
    g.strokePath(tri, PathStrokeType(2.0f));
}

//------------------------------------------------------------------------------
void BranchesLAF::drawScrollbar(Graphics& g, ScrollBar& scrollbar, int x, int y, int width, int height,
                                bool isScrollbarVertical, int thumbStartPosition, int thumbSize, bool isMouseOver,
                                bool isMouseDown)
{
    map<String, Colour>& colours = ::ColourScheme::getInstance().colours;

    if (!isScrollbarVertical)
    {
        // GradientBrush grad(Colour(0xFFDADCC6), 0.0f, (float)y, Colour(0xFFEEECE1), 0.0f, (float)height, false);
        ColourGradient grad(colours["Window Background"].darker(0.25f), 0.0f, (float)y, colours["Window Background"],
                            0.0f, (float)height, false);

        // g.setBrush(&grad);
        g.setGradientFill(grad);
        g.fillRect((float)x, (float)y, (float)width, (float)height);

        g.setColour(Colour(0x30000000));
        g.fillRect((float)x, (float)y, (float)width, 1.0f);
        g.fillRect((float)x, (float)height - 1.0f, (float)width, 1.0f);

        g.setColour(Colour(0x80000000));
        g.drawRoundedRectangle((float)(thumbStartPosition + 1), (float)(y + 1), (float)(thumbSize - 2),
                               (float)(height - 2), 2.0f, 1.0f);
        g.setColour(colours["Button Colour"]);
        g.fillRoundedRectangle((float)(thumbStartPosition + 1), (float)(y + 1), (float)(thumbSize - 2),
                               (float)(height - 2), 2.0f);

        g.setColour(Colour(0x08000000));
        g.fillRoundedRectangle((float)(thumbStartPosition + 1), (float)(height / 2), (float)(thumbSize - 2),
                               (float)(height - 2), 2.0f);
    }
    else
    {
        // GradientBrush grad(Colour(0xFFDADCC6), (float)x, (float)0.0f, Colour(0xFFEEECE1), (float)width, 0.0f, false);
        ColourGradient grad(colours["Window Background"].darker(0.25f), (float)x, (float)0.0f,
                            colours["Window Background"], (float)width, 0.0f, false);

        // g.setBrush(&grad);
        g.setGradientFill(grad);
        g.fillRect((float)x, (float)y, (float)width, (float)height);

        g.setColour(Colour(0x30000000));
        g.fillRect((float)x, (float)y, 1.0f, (float)height);
        g.fillRect((float)width - 1.0f, (float)y, 1.0f, (float)height);

        g.setColour(Colour(0x80000000));
        g.drawRoundedRectangle((float)(x + 1), (float)(thumbStartPosition + 1), (float)(width - 2),
                               (float)(thumbSize - 2), 2.0f, 1.0f);
        g.setColour(colours["Button Colour"]);
        g.fillRoundedRectangle((float)(x + 1), (float)(thumbStartPosition + 1), (float)(width - 2),
                               (float)(thumbSize - 2), 2.0f);

        g.setColour(Colour(0x08000000));
        g.fillRoundedRectangle((float)(width / 2), (float)(thumbStartPosition + 1), (float)(width - 2),
                               (float)(thumbSize - 2), 2.0f);
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
    return FontManager::getInstance().getUIFont(15.0f);
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
    return getMenuBarFont(menuBar, itemIndex, itemText).getStringWidth(itemText) + menuBar.getHeight() - 8;
}

//------------------------------------------------------------------------------
void BranchesLAF::drawPopupMenuBackground(Graphics& g, int width, int height)
{
    auto& colours = ::ColourScheme::getInstance().colours;
    Colour bgCol = colours["Window Background"];

    // Solid background with subtle gradient for depth
    ColourGradient grad(bgCol, 0.0f, 0.0f, bgCol.darker(0.05f), 0.0f, (float)height, false);
    g.setGradientFill(grad);
    g.fillRect(0, 0, width, height);

    // Inner glow at top (light source from above)
    g.setColour(Colours::white.withAlpha(0.08f));
    g.drawHorizontalLine(1, 1.0f, (float)width - 1.0f);

    // Crisp border
    g.setColour(Colour(0x50000000));
    g.drawRect(0, 0, width, height, 1);
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
void BranchesLAF::drawComboBox(Graphics& g, int width, int height, bool isButtonDown, int buttonX, int buttonY,
                               int buttonW, int buttonH, ComboBox& box)
{
    float inc;
    map<String, Colour>& colours = ::ColourScheme::getInstance().colours;

    g.setColour(colours["Text Editor Colour"]);
    g.fillRect(0, 0, width - buttonW + 3, height);

    if (box.isEnabled() && box.hasKeyboardFocus(false))
    {
        g.setColour(colours["Button Colour"]);
        g.drawRect(0, 0, width, height, 2);
    }
    else
    {
        g.setColour(box.findColour(ComboBox::outlineColourId));
        g.drawRect(0, 0, width - buttonW + 3, height);
    }

    g.setColour(Colour(0x80000000));
    g.drawRoundedRectangle(buttonX + 1.0f, buttonY + 1.0f, (float)(buttonW - 2), (float)(buttonH - 2), 2.0f, 1.0f);
    g.setColour(colours["Button Colour"]);
    g.fillRoundedRectangle(buttonX + 1.0f, buttonY + 1.0f, (float)(buttonW - 2), (float)(buttonH - 2), 2.0f);

    g.setColour(Colour(0x08000000));
    g.fillRoundedRectangle(buttonX + 1.0f, buttonY + (float)(buttonH / 2), (float)(buttonW - 2), (float)(buttonH - 2),
                           2.0f);

    if (isButtonDown)
        inc = 1.0f;
    else
        inc = 0.0f;

    if (box.isEnabled())
    {
        Path tri;

        tri.startNewSubPath(buttonX + (float)(buttonW / 2) - (float)(buttonW / 4) + inc,
                            buttonY + (float)(buttonH / 2) - (float)(buttonH / 8) + inc);
        tri.lineTo(buttonX + (float)(buttonW / 2) + inc, buttonY + (float)(buttonH / 2) + (float)(buttonH / 8) + inc);
        tri.lineTo(buttonX + (float)(buttonW / 2) + (float)(buttonW / 4) + inc,
                   buttonY + (float)(buttonH / 2) - (float)(buttonH / 8) + inc);

        g.setColour(colours["Vector Colour"]);
        g.strokePath(tri, PathStrokeType(2.0f));
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
void BranchesLAF::fillTextEditorBackground(Graphics& g, int /*width*/, int /*height*/, TextEditor& textEditor)
{
    g.fillAll(::ColourScheme::getInstance().colours["Text Editor Colour"]);
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
        uint32 colour;
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
