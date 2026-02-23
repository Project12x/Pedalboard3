//  PluginSearchOverlay.cpp - Floating search overlay for plugin selection
//  ----------------------------------------------------------------------------
//  This file is part of Pedalboard3, an audio plugin host.
//  Copyright (c) 2026 Pedalboard3 Project.
//  ----------------------------------------------------------------------------

#include "PluginSearchOverlay.h"

#include "ColourScheme.h"
#include "FontManager.h"
#include "InternalFilters.h"

// ==============================================================================
PluginSearchOverlay::PluginSearchOverlay(KnownPluginList& list) : pluginList(list)
{
    setWantsKeyboardFocus(true);
    // NOTE: do NOT call setAlwaysOnTop here — this is a child component, not a
    // top-level window. setAlwaysOnTop requires a native peer and will crash.

    // Search bar setup
    searchBar.setTextToShowWhenEmpty("Search plugins...",
                                     ColourScheme::getInstance().colours["Text Colour"].withAlpha(0.4f));
    searchBar.setFont(FontManager::getInstance().getBodyFont());
    searchBar.setJustification(Justification::centredLeft);
    searchBar.setIndents(32, 0); // Leave space for magnifier icon
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

    setVisible(false);
}

// ==============================================================================
void PluginSearchOverlay::show(Point<int> position, Rectangle<int> parentBounds)
{
    int w = panelWidth;
    int h = panelHeight;

    // Center on click position, clamped to parent bounds
    int x = position.x - w / 2;
    int y = position.y - h / 2;

    // Clamp — guard against inverted bounds (canvas smaller than panel)
    int minX = parentBounds.getX() + 8;
    int maxX = parentBounds.getRight() - w - 8;
    if (maxX < minX)
        maxX = minX;
    int minY = parentBounds.getY() + 8;
    int maxY = parentBounds.getBottom() - h - 8;
    if (maxY < minY)
        maxY = minY;
    x = jlimit(minX, maxX, x);
    y = jlimit(minY, maxY, y);
    setBounds(x, y, w, h);
    setVisible(true);

    // Reset state
    searchBar.clear();
    currentCategory = Category::All;
    for (auto* btn : categoryButtons)
        btn->setToggleState(btn->getButtonText() == "All", dontSendNotification);

    updateResults();

    searchBar.grabKeyboardFocus();
    toFront(true);
}

// ==============================================================================
void PluginSearchOverlay::hide()
{
    setVisible(false);
    searchBar.clear();

    if (auto* parent = getParentComponent())
        parent->grabKeyboardFocus();
}

// ==============================================================================
void PluginSearchOverlay::paint(Graphics& g)
{
    auto& colours = ColourScheme::getInstance().colours;
    auto bounds = getLocalBounds().toFloat();

    // Drop shadow
    DropShadow shadow(Colours::black.withAlpha(0.5f), 20, {0, 4});
    shadow.drawForRectangle(g, getLocalBounds());

    // Panel background with rounded corners
    auto bgColour = colours["Window Background"].withAlpha(0.97f);
    g.setColour(bgColour);
    g.fillRoundedRectangle(bounds, (float)cornerRadius);

    // Border
    g.setColour(colours["Text Colour"].withAlpha(0.15f));
    g.drawRoundedRectangle(bounds.reduced(0.5f), (float)cornerRadius, 1.0f);

    // Magnifier icon in search bar area
    auto searchBounds = getLocalBounds().reduced(panelPadding).removeFromTop(searchBarHeight);
    g.setColour(colours["Text Colour"].withAlpha(0.5f));
    auto& fm = FontManager::getInstance();
    g.setFont(fm.getBodyFont());
    g.drawText(CharPointer_UTF8("\xf0\x9f\x94\x8d"), // magnifier emoji
               searchBounds.withWidth(30).translated(4, 0), Justification::centred);

    // Footer hint
    auto footer = getLocalBounds().reduced(panelPadding).removeFromBottom(20);
    g.setColour(colours["Text Colour"].withAlpha(0.35f));
    g.setFont(fm.getCaptionFont());
    g.drawText(CharPointer_UTF8("\xe2\x86\x91\xe2\x86\x93 Navigate   \xe2\x86\xb5 Select   Esc Close"), footer,
               Justification::centred);
}

// ==============================================================================
void PluginSearchOverlay::resized()
{
    auto area = getLocalBounds().reduced(panelPadding);

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
bool PluginSearchOverlay::keyPressed(const KeyPress& key)
{
    if (key == KeyPress::escapeKey)
    {
        hide();
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
void PluginSearchOverlay::mouseDown(const MouseEvent& e)
{
    // Clicking outside the panel area dismisses
    if (!getLocalBounds().reduced(panelPadding).contains(e.getPosition()))
    {
        hide();
    }
}

// ==============================================================================
void PluginSearchOverlay::textEditorTextChanged(TextEditor&)
{
    updateResults();
}

// ==============================================================================
int PluginSearchOverlay::getNumRows()
{
    return (int)results.size();
}

// ==============================================================================
void PluginSearchOverlay::paintListBoxItem(int rowNumber, Graphics& g, int width, int height, bool rowIsSelected)
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
void PluginSearchOverlay::paintFormatBadge(Graphics& g, const String& format, Rectangle<int> bounds) const
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
void PluginSearchOverlay::listBoxItemClicked(int row, const MouseEvent&)
{
    // Single click selects
    resultsList.selectRow(row);
}

// ==============================================================================
void PluginSearchOverlay::listBoxItemDoubleClicked(int row, const MouseEvent&)
{
    selectPlugin(row);
}

// ==============================================================================
void PluginSearchOverlay::selectPlugin(int resultIndex)
{
    if (resultIndex < 0 || resultIndex >= (int)results.size())
        return;

    int typeIndex = results[(size_t)resultIndex].typeIndex;

    hide();

    if (onPluginSelected)
        onPluginSelected(typeIndex + 1); // 1-based to match PopupMenu convention
}

// ==============================================================================
void PluginSearchOverlay::updateResults()
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

        // Category filter
        if (!matchesCategory(type))
            continue;

        // Score against search query
        int score = 0;
        if (query.isEmpty())
        {
            score = 100; // Show all when no query
        }
        else
        {
            score = fuzzyScore(query, type.name);
            // Also check manufacturer
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

    // Sort by score descending, then alphabetically
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
int PluginSearchOverlay::fuzzyScore(const String& query, const String& target) const
{
    if (query.isEmpty() || target.isEmpty())
        return 0;

    String lowerTarget = target.toLowerCase();
    String lowerQuery = query;

    // Exact match
    if (lowerTarget == lowerQuery)
        return 1000;

    // Prefix match (e.g. "fab" matches "FabFilter")
    if (lowerTarget.startsWith(lowerQuery))
        return 800;

    // Substring match (e.g. "filter" matches "FabFilter Pro-Q 3")
    if (lowerTarget.contains(lowerQuery))
        return 600;

    // Word-start / initials match (e.g. "fpq" matches "FabFilter Pro-Q 3")
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

    // Character-order match with gaps (e.g. "srum" loosely matches "Serum")
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
            // All query chars found in order — score based on density
            int density = (matched * 100) / lowerTarget.length();
            return 100 + density;
        }
    }

    return 0; // No match
}

// ==============================================================================
bool PluginSearchOverlay::matchesCategory(const PluginDescription& type) const
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
