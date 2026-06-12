//  PluginSearchOverlay.cpp - Floating search window for plugin selection
//  ----------------------------------------------------------------------------
//  This file is part of Pedalboard3, an audio plugin host.
//  Copyright (c) 2026 Pedalboard3 Project.
//  ----------------------------------------------------------------------------

#include "PluginSearchOverlay.h"

#include "ColourScheme.h"
#include "FontManager.h"
#include "IconManager.h"
#include "InternalFilters.h"

// ==============================================================================
// BrowserWindowLookAndFeel is already defined in NAMModelBrowser.cpp, so we
// create a minimal version here for the search window. Because both are in a
// single translation unit scope, we use a distinct name.
namespace
{

void paintSearchGlyph(Graphics& g, Rectangle<float> area, Colour colour)
{
    const auto size = jmin(area.getWidth(), area.getHeight()) * 0.5f;
    auto circle = Rectangle<float>(area.getCentreX() - size * 0.55f, area.getCentreY() - size * 0.62f, size, size);
    g.setColour(colour);
    g.drawEllipse(circle, 1.5f);
    g.drawLine(circle.getRight() - 1.0f, circle.getBottom() - 1.0f, circle.getRight() + size * 0.38f,
               circle.getBottom() + size * 0.38f, 1.5f);
}

class SearchWindowLookAndFeel : public LookAndFeel_V4
{
  public:
    SearchWindowLookAndFeel()
    {
        auto& palette = ::ColourScheme::getInstance().colours;
        auto bg = palette["Window Background"];
        auto text = palette["Text Colour"];

        setColour(DocumentWindow::backgroundColourId, bg);
        setColour(DocumentWindow::textColourId, text);
        setColour(ResizableWindow::backgroundColourId, bg);
    }

    void drawDocumentWindowTitleBar(DocumentWindow& window, Graphics& g, int w, int h, int titleSpaceX, int titleSpaceW,
                                    const Image* /*icon*/, bool /*drawTitleTextOnLeft*/) override
    {
        auto& palette = ::ColourScheme::getInstance().colours;
        auto bg = palette["Window Background"].darker(0.12f);
        auto text = palette["Text Colour"];

        float cr = 10.0f;
        Path titlePath;
        titlePath.addRoundedRectangle(0.0f, 0.0f, (float)w, (float)h + cr, cr, cr, true, true, false, false);
        ColourGradient titleGradient(bg.brighter(0.08f), 0.0f, 0.0f, bg.darker(0.12f), 0.0f, (float)h, false);
        g.setGradientFill(titleGradient);
        g.fillPath(titlePath);

        g.setColour(text.withAlpha(0.08f));
        g.drawHorizontalLine(h - 1, 0.0f, (float)w);
        g.setColour(palette["Accent Colour"].withAlpha(0.34f));
        g.drawLine((float)titleSpaceX, (float)h - 2.0f, (float)jmin(w - 34, titleSpaceX + titleSpaceW / 2),
                   (float)h - 2.0f, 1.0f);

        auto lamp = Rectangle<float>((float)titleSpaceX, h * 0.5f - 5.0f, 10.0f, 10.0f);
        g.setColour(palette["Accent Colour"].withAlpha(0.16f));
        g.fillRoundedRectangle(lamp.expanded(4.0f), 6.0f);
        ColourGradient lampGradient(palette["Accent Colour"].brighter(0.35f), lamp.getX(), lamp.getY(),
                                    palette["Accent Colour"].darker(0.35f), lamp.getRight(), lamp.getBottom(), false);
        g.setGradientFill(lampGradient);
        g.fillRoundedRectangle(lamp, 3.0f);

        g.setColour(text.withAlpha(0.9f));
        g.setFont(FontManager::getInstance().getSubheadingFont());
        g.drawText(window.getName(), titleSpaceX + 18, 0, titleSpaceW - 18, h, Justification::centredLeft, true);
    }

    Button* createDocumentWindowButton(int buttonType) override
    {
        if (buttonType == DocumentWindow::closeButton)
        {
            // Simple close button — circle with X
            auto* btn = new TextButton("X");
            btn->setColour(TextButton::buttonColourId, Colours::transparentBlack);
            btn->setColour(TextButton::buttonOnColourId, Colour(0xFFCC4444));
            btn->setColour(TextButton::textColourOffId,
                           ::ColourScheme::getInstance().colours["Text Colour"].withAlpha(0.6f));
            btn->setColour(TextButton::textColourOnId, Colours::white);
            return btn;
        }
        return LookAndFeel_V4::createDocumentWindowButton(buttonType);
    }

    void drawResizableWindowBorder(Graphics& /*g*/, int /*w*/, int /*h*/, const BorderSize<int>& /*border*/,
                                   ResizableWindow& /*window*/) override
    {
    }

    void fillTextEditorBackground(Graphics& g, int width, int height, TextEditor& /*editor*/) override
    {
        auto& palette = ::ColourScheme::getInstance().colours;
        auto bounds = Rectangle<float>(0.0f, 0.0f, (float)width, (float)height);
        ColourGradient fill(palette["Dialog Inner Background"].darker(0.12f), bounds.getX(), bounds.getY(),
                            palette["Dialog Inner Background"].brighter(0.03f), bounds.getX(), bounds.getBottom(),
                            false);
        g.setGradientFill(fill);
        g.fillRoundedRectangle(bounds, 8.0f);
    }

    void drawTextEditorOutline(Graphics& g, int width, int height, TextEditor& editor) override
    {
        auto& palette = ::ColourScheme::getInstance().colours;
        auto bounds = Rectangle<float>(0.0f, 0.0f, (float)width, (float)height).reduced(0.5f);
        const float alpha = editor.hasKeyboardFocus(true) ? 0.46f : 0.16f;
        g.setColour((editor.hasKeyboardFocus(true) ? palette["Accent Colour"] : palette["Text Colour"]).withAlpha(alpha));
        g.drawRoundedRectangle(bounds, 8.0f, 1.0f);
    }
};

class SearchCategoryLookAndFeel : public LookAndFeel_V4
{
  public:
    void drawButtonBackground(Graphics& g, Button& button, const Colour& /*backgroundColour*/,
                              bool isMouseOverButton, bool isButtonDown) override
    {
        auto& palette = ::ColourScheme::getInstance().colours;
        auto bounds = button.getLocalBounds().toFloat().reduced(0.75f);
        const bool selected = button.getToggleState();

        auto* textButton = dynamic_cast<TextButton*>(&button);
        const auto label = textButton != nullptr ? textButton->getButtonText() : button.getName();
        auto accent = label == "Internal" ? Colour(0xFF44AA66) : palette["Accent Colour"];

        if (selected)
        {
            ColourGradient selectedFill(accent.brighter(0.12f), bounds.getX(), bounds.getY(), accent.darker(0.28f),
                                        bounds.getX(), bounds.getBottom(), false);
            g.setGradientFill(selectedFill);
            g.fillRoundedRectangle(bounds, 7.0f);
            g.setColour(accent.brighter(0.2f).withAlpha(0.72f));
            g.drawRoundedRectangle(bounds, 7.0f, 1.0f);
        }
        else
        {
            auto fill = palette["Dialog Inner Background"].darker(isMouseOverButton ? 0.02f : 0.12f);
            if (isButtonDown)
                fill = fill.brighter(0.06f);

            g.setColour(fill);
            g.fillRoundedRectangle(bounds, 7.0f);
            g.setColour(
                (isMouseOverButton ? accent : palette["Text Colour"]).withAlpha(isMouseOverButton ? 0.32f : 0.12f));
            g.drawRoundedRectangle(bounds, 7.0f, 1.0f);
        }
    }

    void drawButtonText(Graphics& g, TextButton& button, bool isMouseOverButton, bool /*isButtonDown*/) override
    {
        auto& palette = ::ColourScheme::getInstance().colours;
        const bool selected = button.getToggleState();
        g.setFont(FontManager::getInstance().getBodyBoldFont());
        g.setColour(selected ? Colours::white : palette["Text Colour"].withAlpha(isMouseOverButton ? 0.86f : 0.62f));
        g.drawText(button.getButtonText(), button.getLocalBounds().reduced(8, 2), Justification::centred, true);
    }
};

} // namespace

// ==============================================================================
// PluginSearchContent
// ==============================================================================

PluginSearchContent::PluginSearchContent(KnownPluginList& list) : pluginList(list)
{
    setWantsKeyboardFocus(true);

    // Search bar setup — pill-shaped with larger font
    auto& colours = ColourScheme::getInstance().colours;
    searchBar.setTextToShowWhenEmpty("Search plug-ins, makers, formats...", colours["Text Colour"].withAlpha(0.4f));
    searchBar.setFont(FontManager::getInstance().getSubheadingFont());
    searchBar.setJustification(Justification::centredLeft);
    searchBar.setIndents(34, 6); // Leave space for magnifier icon, vertically centered
    searchBar.setColour(TextEditor::backgroundColourId, colours["Dialog Inner Background"].darker(0.1f));
    searchBar.setColour(TextEditor::outlineColourId, Colours::transparentBlack);
    searchBar.setColour(TextEditor::focusedOutlineColourId, colours["Accent Colour"].withAlpha(0.45f));
    searchBar.setColour(TextEditor::textColourId, colours["Text Colour"]);
    searchBar.setColour(TextEditor::highlightColourId, colours["Accent Colour"].withAlpha(0.35f));
    searchBar.addListener(this);
    addAndMakeVisible(searchBar);

    // Results list setup
    resultsList.setModel(this);
    resultsList.setRowHeight(resultRowHeight);
    resultsList.setMultipleSelectionEnabled(false);
    resultsList.setColour(ListBox::backgroundColourId, Colours::transparentBlack);
    resultsList.setColour(ListBox::outlineColourId, Colours::transparentBlack);
    addAndMakeVisible(resultsList);

    // Category tab buttons
    struct TabDef
    {
        String label;
        Category cat;
    };
    TabDef tabs[] = {{"All", Category::All},
                     {"Effects", Category::Effects},
                     {"Instruments", Category::Instruments},
                     {"Internal", Category::Internal}};

    categoryButtonLookAndFeel = std::make_unique<SearchCategoryLookAndFeel>();

    for (auto& tab : tabs)
    {
        auto* btn = categoryButtons.add(new TextButton(tab.label));
        btn->setLookAndFeel(categoryButtonLookAndFeel.get());
        btn->setClickingTogglesState(true);
        btn->setRadioGroupId(1);
        btn->setToggleState(tab.cat == Category::All, dontSendNotification);
        btn->onClick = [this, cat = tab.cat]()
        {
            currentCategory = cat;
            updateResults();
        };
        addAndMakeVisible(btn);
    }
}

PluginSearchContent::~PluginSearchContent()
{
    for (auto* btn : categoryButtons)
        btn->setLookAndFeel(nullptr);
}

// ==============================================================================
void PluginSearchContent::activate()
{
    searchBar.clear();
    currentCategory = Category::All;
    for (auto* btn : categoryButtons)
        btn->setToggleState(btn->getButtonText() == "All", dontSendNotification);

    updateResults();
    searchBar.grabKeyboardFocus();
}

// ==============================================================================
void PluginSearchContent::paint(Graphics& g)
{
    auto& colours = ColourScheme::getInstance().colours;

    // Content background — fill with rounded bottom corners only
    auto bounds = getLocalBounds().toFloat();
    float cr = 10.0f;
    Path bgPath;
    bgPath.addRoundedRectangle(bounds.getX(), bounds.getY(), bounds.getWidth(), bounds.getHeight(), cr, cr, false,
                               false, true, true);
    ColourGradient bg(colours["Window Background"].brighter(0.04f), 0.0f, 0.0f,
                      colours["Window Background"].darker(0.06f), 0.0f, bounds.getHeight(), false);
    g.setGradientFill(bg);
    g.fillPath(bgPath);

    // Subtle dot grid for cohesion with the main graph/dialog family.
    g.setColour(colours["Text Colour"].withAlpha(0.045f));
    for (int gy = 0; gy < getHeight(); gy += 16)
        for (int gx = 0; gx < getWidth(); gx += 16)
            g.fillEllipse((float)gx, (float)gy, 2.0f, 2.0f);

    auto headerPanel = searchBar.getBounds();
    for (auto* btn : categoryButtons)
        headerPanel = headerPanel.getUnion(btn->getBounds());

    if (!headerPanel.isEmpty())
    {
        auto panelBounds = headerPanel.expanded(8, 7).toFloat();
        ColourGradient panelFill(colours["Dialog Inner Background"].brighter(0.09f), panelBounds.getX(),
                                 panelBounds.getY(), colours["Dialog Inner Background"].darker(0.1f),
                                 panelBounds.getX(), panelBounds.getBottom(), false);
        g.setGradientFill(panelFill);
        g.fillRoundedRectangle(panelBounds, 9.0f);
        g.setColour(Colours::black.withAlpha(0.18f));
        g.drawRoundedRectangle(panelBounds.reduced(2.0f), 7.0f, 1.0f);
        g.setColour(colours["Text Colour"].withAlpha(0.13f));
        g.drawRoundedRectangle(panelBounds.reduced(0.5f), 9.0f, 1.0f);
        g.setColour(colours["Accent Colour"].withAlpha(0.58f));
        g.drawLine(panelBounds.getX() + 14.0f, panelBounds.getBottom() - 6.0f,
                   panelBounds.getX() + jmin(230.0f, panelBounds.getWidth() * 0.52f), panelBounds.getBottom() - 6.0f,
                   1.6f);
    }

    auto searchRow = getLocalBounds().reduced(contentPadding).removeFromTop(searchBarHeight);
    const int statusWidth = jlimit(92, 118, searchRow.getWidth() / 4);
    auto statusPill = searchRow.removeFromRight(statusWidth).reduced(1, 6).toFloat();
    g.setColour(colours["Dialog Inner Background"].darker(0.22f));
    g.fillRoundedRectangle(statusPill, 8.0f);
    g.setColour(colours["Text Colour"].withAlpha(0.12f));
    g.drawRoundedRectangle(statusPill.reduced(0.5f), 8.0f, 1.0f);

    auto statusLed = statusPill.removeFromLeft(22.0f);
    g.setColour(colours["VU Meter Lower Colour"].withAlpha(results.empty() ? 0.12f : 0.26f));
    g.fillEllipse(statusLed.withSizeKeepingCentre(8.0f, 8.0f));
    g.setColour(results.empty() ? colours["Text Colour"].withAlpha(0.42f) : colours["VU Meter Lower Colour"]);
    g.setFont(FontManager::getInstance().getBadgeFont());
    const auto resultText = String((int)results.size()) + (results.size() == 1 ? " RESULT" : " RESULTS");
    g.drawText(resultText, statusPill.reduced(1, 0), Justification::centredLeft, true);

    auto tabBounds = getLocalBounds().reduced(contentPadding);
    tabBounds.removeFromTop(searchBarHeight + 4);
    auto tabArea = tabBounds.removeFromTop(tabRowHeight).toFloat().expanded(2.0f, 1.0f);
    g.setColour(colours["Dialog Inner Background"].darker(0.1f));
    g.fillRoundedRectangle(tabArea, 8.0f);
    g.setColour(colours["Text Colour"].withAlpha(0.1f));
    g.drawRoundedRectangle(tabArea.reduced(0.5f), 8.0f, 1.0f);

    auto listWell = resultsList.getBounds().toFloat().expanded(1.0f);
    g.setColour(colours["Dialog Inner Background"].darker(0.13f));
    g.fillRoundedRectangle(listWell, 8.0f);
    g.setColour(colours["Text Colour"].withAlpha(0.12f));
    g.drawRoundedRectangle(listWell.reduced(0.5f), 8.0f, 1.0f);

    if (results.empty())
    {
        g.setFont(FontManager::getInstance().getBodyFont());
        g.setColour(colours["Text Colour"].withAlpha(0.48f));
        const auto query = searchBar.getText().trim();
        const String message = query.isNotEmpty() ? "No plug-ins match \"" + query + "\"" : "No plug-ins in this category";
        g.drawFittedText(message, resultsList.getBounds().reduced(18), Justification::centred, 2);
    }

    // Footer hint
    auto footer = getLocalBounds().reduced(contentPadding).removeFromBottom(20);
    g.setColour(colours["Text Colour"].withAlpha(0.35f));
    g.setFont(FontManager::getInstance().getCaptionFont());
    String footerText = getCurrentCategoryLabel();
    footerText += "   Up/Down Navigate   Enter Select   Esc Close";
    g.drawText(footerText, footer, Justification::centred);
}

// ==============================================================================
void PluginSearchContent::paintOverChildren(Graphics& g)
{
    auto& colours = ColourScheme::getInstance().colours;
    paintSearchGlyph(g, searchBar.getBounds().withWidth(34).translated(4, 0).toFloat(),
                     colours["Text Colour"].withAlpha(0.46f));
}

// ==============================================================================
void PluginSearchContent::resized()
{
    auto area = getLocalBounds().reduced(contentPadding);

    // Search bar
    auto searchRow = area.removeFromTop(searchBarHeight);
    const int statusWidth = jlimit(92, 118, searchRow.getWidth() / 4);
    searchRow.removeFromRight(statusWidth + 8);
    searchBar.setBounds(searchRow);
    area.removeFromTop(4);

    // Category tabs
    auto tabArea = area.removeFromTop(tabRowHeight);
    int tabWidth = tabArea.getWidth() / jmax(1, categoryButtons.size());
    for (auto* btn : categoryButtons)
    {
        btn->setBounds(tabArea.removeFromLeft(tabWidth).reduced(2, 2));
    }
    area.removeFromTop(4);

    // Footer
    area.removeFromBottom(24);

    // Results list takes remaining space
    resultsList.setBounds(area);
}

// ==============================================================================
bool PluginSearchContent::keyPressed(const KeyPress& key)
{
    if (key == KeyPress::escapeKey)
    {
        if (onCloseRequested)
            onCloseRequested();
        return true;
    }

    if (key == KeyPress::returnKey)
    {
        int selected = resultsList.getSelectedRow();
        if (selected >= 0)
            selectPlugin(selected);
        return true;
    }

    if (key == KeyPress::downKey)
    {
        int newRow = jmin(resultsList.getSelectedRow() + 1, (int)results.size() - 1);
        resultsList.selectRow(newRow);
        resultsList.scrollToEnsureRowIsOnscreen(newRow);
        return true;
    }

    if (key == KeyPress::upKey)
    {
        int newRow = jmax(resultsList.getSelectedRow() - 1, 0);
        resultsList.selectRow(newRow);
        resultsList.scrollToEnsureRowIsOnscreen(newRow);
        return true;
    }

    return false;
}

// ==============================================================================
void PluginSearchContent::textEditorTextChanged(TextEditor&)
{
    updateResults();
}

// ==============================================================================
int PluginSearchContent::getNumRows()
{
    return (int)results.size();
}

// ==============================================================================
void PluginSearchContent::paintListBoxItem(int rowNumber, Graphics& g, int width, int height, bool rowIsSelected)
{
    if (rowNumber < 0 || rowNumber >= (int)results.size())
        return;

    auto& colours = ColourScheme::getInstance().colours;
    auto& result = results[(size_t)rowNumber];
    auto& fm = FontManager::getInstance();

    auto itemBounds = Rectangle<float>(6.0f, 4.0f, (float)width - 12.0f, (float)height - 8.0f);

    // Row background
    if (rowIsSelected)
    {
        ColourGradient selected(colours["Accent Colour"].withAlpha(0.26f), itemBounds.getX(), itemBounds.getY(),
                                colours["Dialog Inner Background"].brighter(0.08f), itemBounds.getX(),
                                itemBounds.getBottom(), false);
        g.setGradientFill(selected);
        g.fillRoundedRectangle(itemBounds, 8.0f);
        g.setColour(colours["Accent Colour"].withAlpha(0.6f));
        g.drawRoundedRectangle(itemBounds.reduced(0.5f), 8.0f, 1.0f);
        g.fillRoundedRectangle(itemBounds.withWidth(3.0f).reduced(0.0f, 5.0f), 1.5f);
    }
    else
    {
        g.setColour(colours["Text Colour"].withAlpha(0.03f));
        g.fillRoundedRectangle(itemBounds, 8.0f);
        g.setColour(colours["Text Colour"].withAlpha(0.055f));
        g.drawRoundedRectangle(itemBounds.reduced(0.5f), 8.0f, 1.0f);
    }

    // Format badge
    auto glyph = itemBounds.removeFromLeft(40.0f).reduced(7.0f);
    Colour glyphColour = result.isInternal ? Colour(0xFF44AA66) : colours["Accent Colour"];
    IconManager::getInstance().drawDomainGlyphTile(g, glyph, IconManager::DomainGlyph::Plugin, glyphColour,
                                                   rowIsSelected);

    auto badgeBounds = Rectangle<int>((int)itemBounds.getX() + 6, (height - 20) / 2, 58, 20);
    paintFormatBadge(g, result.formatName, badgeBounds);

    // Plugin name
    int textX = badgeBounds.getRight() + 10;
    g.setColour(colours["Text Colour"]);
    g.setFont(fm.getBodyBoldFont());
    g.drawText(result.name, textX, 4, width - textX - 40, height / 2 - 2, Justification::centredLeft);

    // Manufacturer
    g.setColour(colours["Text Colour"].withAlpha(0.5f));
    g.setFont(fm.getCaptionFont());
    g.drawText(result.manufacturer, textX, height / 2 - 2, width - textX - 40, height / 2, Justification::centredLeft);

    g.setColour(rowIsSelected ? colours["Accent Colour"].withAlpha(0.85f) : colours["Text Colour"].withAlpha(0.28f));
    Path chevron;
    const float cx = (float)width - 24.0f;
    const float cy = height * 0.5f;
    chevron.startNewSubPath(cx - 3.0f, cy - 5.0f);
    chevron.lineTo(cx + 3.0f, cy);
    chevron.lineTo(cx - 3.0f, cy + 5.0f);
    g.strokePath(chevron, PathStrokeType(1.4f, PathStrokeType::curved, PathStrokeType::rounded));
}

// ==============================================================================
void PluginSearchContent::paintFormatBadge(Graphics& g, const String& format, Rectangle<int> bounds) const
{
    auto& colours = ColourScheme::getInstance().colours;
    auto& fm = FontManager::getInstance();

    Colour badgeColour;
    if (format == "VST3")
        badgeColour = Colour(0xFF5588DD); // Blue
    else if (format == "Internal")
        badgeColour = Colour(0xFF44AA66); // Green
    else if (format == "LADSPA")
        badgeColour = Colour(0xFFAA6644); // Orange
    else
        badgeColour = colours["Text Colour"].withAlpha(0.3f);

    g.setColour(badgeColour.withAlpha(0.2f));
    g.fillRoundedRectangle(bounds.toFloat(), 4.0f);

    g.setColour(badgeColour);
    g.setFont(fm.getBadgeFont());
    g.drawText(format, bounds, Justification::centred);
}

// ==============================================================================
String PluginSearchContent::getCurrentCategoryLabel() const
{
    switch (currentCategory)
    {
    case Category::Effects:
        return "Effects";
    case Category::Instruments:
        return "Instruments";
    case Category::Internal:
        return "Internal";
    case Category::All:
    default:
        return "All plug-ins";
    }
}

// ==============================================================================
void PluginSearchContent::listBoxItemClicked(int row, const MouseEvent&)
{
    resultsList.selectRow(row);
}

// ==============================================================================
void PluginSearchContent::listBoxItemDoubleClicked(int row, const MouseEvent&)
{
    selectPlugin(row);
}

// ==============================================================================
void PluginSearchContent::selectPlugin(int resultIndex)
{
    if (resultIndex < 0 || resultIndex >= (int)results.size())
        return;

    int typeIndex = results[(size_t)resultIndex].typeIndex;

    if (onCloseRequested)
        onCloseRequested();

    if (onPluginSelected)
        onPluginSelected(typeIndex + 1); // 1-based to match PopupMenu convention
}

// ==============================================================================
void PluginSearchContent::updateResults()
{
    results.clear();

    auto types = pluginList.getTypes();

    // Add Effect Rack if available
    InternalPluginFormat internalFormat;
    if (auto* subGraphDesc = internalFormat.getDescriptionFor(InternalPluginFormat::subGraphProcFilter))
        types.add(*subGraphDesc);

    String query = searchBar.getText().trim().toLowerCase();

    for (int i = 0; i < types.size(); ++i)
    {
        const auto& type = types.getReference(i);

        if (!PluginSearchLogic::matchesCategory(type, currentCategory))
            continue;

        int score = PluginSearchLogic::scorePlugin(query, type);

        if (score > 0)
        {
            SearchResult sr;
            sr.typeIndex = i;
            sr.name = type.name;
            sr.manufacturer = type.manufacturerName.isEmpty() ? type.pluginFormatName : type.manufacturerName;
            sr.formatName = type.pluginFormatName;
            sr.category = type.category;
            sr.score = score;
            sr.isInternal = (type.pluginFormatName == "Internal" || type.category == "Built-in");
            results.push_back(sr);
        }
    }

    std::sort(results.begin(), results.end(),
              [](const SearchResult& a, const SearchResult& b)
              {
                  if (a.score != b.score)
                      return a.score > b.score;
                  return a.name.compareIgnoreCase(b.name) < 0;
              });

    resultsList.updateContent();
    if (!results.empty())
        resultsList.selectRow(0);
}

// PluginSearchWindow
// ==============================================================================

PluginSearchWindow::PluginSearchWindow(KnownPluginList& pluginList)
    : DocumentWindow("Add Plugin", ColourScheme::getInstance().colours["Window Background"],
                     DocumentWindow::closeButton)
{
    windowLAF = new SearchWindowLookAndFeel();
    setLookAndFeel(windowLAF);

    setUsingNativeTitleBar(false);
    setResizable(false, false);
    setDropShadowEnabled(true);
    setAlwaysOnTop(true);

    content = new PluginSearchContent(pluginList);
    content->onCloseRequested = [this]() { closeButtonPressed(); };
    setContentOwned(content, false);
    setSize(windowWidth, windowHeight);
}

PluginSearchWindow::~PluginSearchWindow()
{
    setLookAndFeel(nullptr);
    delete windowLAF;
}

void PluginSearchWindow::showCentred()
{
    centreWithSize(windowWidth, windowHeight);
    setVisible(true);
    toFront(true);

    if (content)
        content->activate();
}

void PluginSearchWindow::closeButtonPressed()
{
    setVisible(false);
}
