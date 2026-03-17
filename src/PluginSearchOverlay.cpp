//  PluginSearchOverlay.cpp - Floating search window for plugin selection
//  ----------------------------------------------------------------------------
//  This file is part of Pedalboard3, an audio plugin host.
//  Copyright (c) 2026 Pedalboard3 Project.
//  ----------------------------------------------------------------------------

#include "PluginSearchOverlay.h"

#include "ColourScheme.h"
#include "FontManager.h"
#include "InternalFilters.h"

// ==============================================================================
// BrowserWindowLookAndFeel is already defined in NAMModelBrowser.cpp, so we
// create a minimal version here for the search window. Because both are in a
// single translation unit scope, we use a distinct name.
namespace
{

class SearchWindowLookAndFeel : public LookAndFeel_V4
{
  public:
    SearchWindowLookAndFeel()
    {
        auto& colours = ::ColourScheme::getInstance().colours;
        auto bg = colours["Window Background"];
        auto text = colours["Text Colour"];

        setColour(DocumentWindow::backgroundColourId, bg);
        setColour(DocumentWindow::textColourId, text);
        setColour(ResizableWindow::backgroundColourId, bg);
    }

    void drawDocumentWindowTitleBar(DocumentWindow& window, Graphics& g, int w, int h, int titleSpaceX, int titleSpaceW,
                                    const Image* /*icon*/, bool /*drawTitleTextOnLeft*/) override
    {
        auto& colours = ::ColourScheme::getInstance().colours;
        auto bg = colours["Window Background"].darker(0.15f);
        auto text = colours["Text Colour"];

        float cr = 10.0f;
        Path titlePath;
        titlePath.addRoundedRectangle(0.0f, 0.0f, (float)w, (float)h + cr, cr, cr, true, true, false, false);
        g.setColour(bg);
        g.fillPath(titlePath);

        g.setColour(text.withAlpha(0.1f));
        g.drawHorizontalLine(h - 1, 0.0f, (float)w);

        g.setColour(text.withAlpha(0.9f));
        g.setFont(FontManager::getInstance().getSubheadingFont());
        g.drawText(window.getName(), titleSpaceX, 0, titleSpaceW, h, Justification::centredLeft, true);
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
    searchBar.setTextToShowWhenEmpty("Search plugins...", colours["Text Colour"].withAlpha(0.4f));
    searchBar.setFont(FontManager::getInstance().getSubheadingFont());
    searchBar.setJustification(Justification::centredLeft);
    searchBar.setIndents(28, 6); // Leave space for magnifier icon, vertically centered
    searchBar.setColour(TextEditor::backgroundColourId, colours["Window Background"].brighter(0.08f));
    searchBar.setColour(TextEditor::outlineColourId, Colours::transparentBlack);
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

    for (auto& tab : tabs)
    {
        auto* btn = categoryButtons.add(new TextButton(tab.label));
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
    g.setColour(colours["Window Background"]);
    g.fillPath(bgPath);

    // Magnifier icon in search bar area
    auto searchBounds = getLocalBounds().reduced(contentPadding).removeFromTop(searchBarHeight);
    g.setColour(colours["Text Colour"].withAlpha(0.5f));
    g.setFont(FontManager::getInstance().getBodyFont());
    g.drawText(CharPointer_UTF8("\xf0\x9f\x94\x8d"), // magnifier emoji
               searchBounds.withWidth(30).translated(4, 0), Justification::centred);

    // Footer hint
    auto footer = getLocalBounds().reduced(contentPadding).removeFromBottom(20);
    g.setColour(colours["Text Colour"].withAlpha(0.35f));
    g.setFont(FontManager::getInstance().getCaptionFont());
    g.drawText(CharPointer_UTF8("\xe2\x86\x91\xe2\x86\x93 Navigate   \xe2\x86\xb5 Select   Esc Close"), footer,
               Justification::centred);
}

// ==============================================================================
void PluginSearchContent::resized()
{
    auto area = getLocalBounds().reduced(contentPadding);

    // Search bar
    searchBar.setBounds(area.removeFromTop(searchBarHeight));
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

    // Row background
    if (rowIsSelected)
    {
        g.setColour(colours["Accent Colour"].withAlpha(0.25f));
        g.fillRoundedRectangle(2.0f, 1.0f, (float)width - 4.0f, (float)height - 2.0f, 6.0f);
    }

    // Format badge
    auto badgeBounds = Rectangle<int>(8, (height - 20) / 2, 48, 20);
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

        if (!matchesCategory(type))
            continue;

        int score = 0;
        if (query.isEmpty())
        {
            score = 100;
        }
        else
        {
            score = fuzzyScore(query, type.name);
            int mfgScore = fuzzyScore(query, type.manufacturerName);
            score = jmax(score, mfgScore);
        }

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

// ==============================================================================
int PluginSearchContent::fuzzyScore(const String& query, const String& target) const
{
    if (query.isEmpty() || target.isEmpty())
        return 0;

    String lowerTarget = target.toLowerCase();
    String lowerQuery = query;

    if (lowerTarget == lowerQuery)
        return 1000;

    if (lowerTarget.startsWith(lowerQuery))
        return 800;

    if (lowerTarget.contains(lowerQuery))
        return 600;

    {
        String initials;
        bool nextIsStart = true;
        for (auto ch : lowerTarget)
        {
            if (ch == ' ' || ch == '-' || ch == '_')
            {
                nextIsStart = true;
            }
            else if (nextIsStart)
            {
                initials += ch;
                nextIsStart = false;
            }
        }
        if (initials.startsWith(lowerQuery))
            return 500;
        if (initials.contains(lowerQuery))
            return 400;
    }

    {
        int qi = 0;
        int matched = 0;
        for (int ti = 0; ti < lowerTarget.length() && qi < lowerQuery.length(); ++ti)
        {
            if (lowerTarget[ti] == lowerQuery[qi])
            {
                ++qi;
                ++matched;
            }
        }
        if (qi == lowerQuery.length())
        {
            int density = (matched * 100) / lowerTarget.length();
            return 100 + density;
        }
    }

    return 0;
}

// ==============================================================================
bool PluginSearchContent::matchesCategory(const PluginDescription& type) const
{
    switch (currentCategory)
    {
    case Category::All:
        return true;

    case Category::Effects:
        return type.pluginFormatName != "Internal" && type.category != "Built-in" && !type.isInstrument;

    case Category::Instruments:
        return type.isInstrument;

    case Category::Internal:
        return type.pluginFormatName == "Internal" || type.category == "Built-in";
    }
    return true;
}

// ==============================================================================
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
