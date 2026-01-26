//	BranchesLAF.h - LookAndFeel class implementing some different buttons.
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

#ifndef BRANCHESLAF_H_
#define BRANCHESLAF_H_

#include <JuceHeader.h>

///	LookAndFeel class implementing some different buttons.
class BranchesLAF : public LookAndFeel_V4
{
  public:
    ///	Constructor.
    BranchesLAF();
    ///	Destructor.
    ~BranchesLAF() override;

    /// @brief Refresh LookAndFeel colors from ColourScheme.
    /// Call this after changing theme to update menu/button colors.
    void refreshColours();

    ///	Draws the buttons.
    void drawButtonBackground(Graphics& g, Button& button, const Colour& backgroundColour, bool isMouseOverButton,
                              bool isButtonDown) override;
    ///	Draws button text.
    void drawButtonText(Graphics& g, TextButton& button, bool isMouseOverButton, bool isButtonDown) override;

    ///	Draws the scrollbar buttons.
    void drawScrollbarButton(Graphics& g, ScrollBar& scrollbar, int width, int height, int buttonDirection,
                             bool isScrollbarVertical, bool isMouseOverButton, bool isButtonDown) override;
    ///	Draws the scrollbar.
    void drawScrollbar(Graphics& g, ScrollBar& scrollbar, int x, int y, int width, int height, bool isScrollbarVertical,
                       int thumbStartPosition, int thumbSize, bool isMouseOver, bool isMouseDown) override;

    ///	Draws the menubar.
    void drawMenuBarBackground(Graphics& g, int width, int height, bool isMouseOverBar,
                               MenuBarComponent& menuBar) override;
    ///	Returns the menubar font.
    Font getMenuBarFont(MenuBarComponent& menuBar, int itemIndex, const String& itemText) override;
    ///	Draws the menubar items.
    void drawMenuBarItem(Graphics& g, int width, int height, int itemIndex, const String& itemText,
                         bool isMouseOverItem, bool isMenuOpen, bool isMouseOverBar,
                         MenuBarComponent& menuBar) override;
    ///	The width of a menubar item.
    int getMenuBarItemWidth(MenuBarComponent& menuBar, int itemIndex, const String& itemText) override;
    ///	Returns the popup meun font.
    Font getPopupMenuFont() override { return Font(FontOptions().withHeight(15.0f)); };
    ///	Draws the popup menu background.
    void drawPopupMenuBackground(Graphics& g, int width, int height) override;
    ///	Cancels menus' drop shadow.
    int getMenuWindowFlags() override { return 0; };

    /// Use legacy font metrics to maintain JUCE 7 text sizing behavior
    TypefaceMetricsKind getDefaultMetricsKind() const override { return TypefaceMetricsKind::legacy; }

    ///	Returns the image of a folder for the file chooser.
    const Drawable* getDefaultFolderImage() override;
    ///	Draws a combobox (used in the file chooser).
    void drawComboBox(Graphics& g, int width, int height, bool isButtonDown, int buttonX, int buttonY, int buttonW,
                      int buttonH, ComboBox& box) override;

    ///	Draws the ProgressBar.
    void drawProgressBar(Graphics& g, ProgressBar& progressBar, int width, int height, double progress,
                         const String& textToShow) override;

    ///	Draws the KeymapChange button.
    void drawKeymapChangeButton(Graphics& g, int width, int height, Button& button,
                                const String& keyDescription) override;

    ///	Draws a Label.
    void drawLabel(Graphics& g, Label& label) override;

    ///	Draws a ToggleButton.
    void drawToggleButton(Graphics& g, ToggleButton& button, bool isMouseOverButton, bool isButtonDown) override;

    ///	Drwas a tick box.
    void drawTickBox(Graphics& g, Component& component, float x, float y, float w, float h, bool ticked, bool isEnabled,
                     bool isMouseOverButton, bool isButtonDown) override;

    ///	Fills in the TextEditor background.
    void fillTextEditorBackground(Graphics& g, int width, int height, TextEditor& textEditor) override;

    ///	Draws the callout box.
    void drawCallOutBoxBackground(CallOutBox& box, Graphics& g, const Path& path, Image& cachedImage) override;

    /// Draws an AlertWindow.
    void drawAlertBox(Graphics& g, AlertWindow& alert, const Rectangle<int>& textArea, TextLayout& textLayout) override;
};

#endif
