//  PluginSearchOverlay.h - Floating search overlay for plugin selection
//  ----------------------------------------------------------------------------
//  This file is part of Pedalboard3, an audio plugin host.
//  Copyright (c) 2026 Pedalboard3 Project.
//
//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//  ----------------------------------------------------------------------------

#ifndef PLUGINSEARCHOVERLAY_H_
#define PLUGINSEARCHOVERLAY_H_

#include <JuceHeader.h>
#include <functional>
#include <vector>

/// @brief A floating overlay for searching and selecting plugins with fuzzy matching.
///
/// Replaces the AlertWindow-based search dialog. Shows a search bar with live
/// filtering, category tabs, and a scrollable results list with keyboard navigation.
class PluginSearchOverlay : public Component, private TextEditor::Listener, private ListBoxModel
{
  public:
    /// @brief Construct the overlay.
    /// @param pluginList The known plugin list to search.
    PluginSearchOverlay(KnownPluginList& pluginList);
    ~PluginSearchOverlay() override = default;

    /// @brief Show the overlay at the given screen position.
    /// @param position Position on the parent component to center near.
    /// @param parentBounds Bounds of the parent to clamp within.
    void show(Point<int> position, Rectangle<int> parentBounds);

    /// @brief Hide the overlay.
    void hide();

    /// @brief Callback when a plugin is selected. Passes the type index (1-based, matching PopupMenu convention).
    std::function<void(int typeIndex)> onPluginSelected;

    // Component overrides
    void paint(Graphics& g) override;
    void resized() override;
    bool keyPressed(const KeyPress& key) override;

    /// @brief Dismiss when clicking outside the panel.
    void mouseDown(const MouseEvent& e) override;

  private:
    // Category filter enum
    enum class Category
    {
        All,
        Effects,
        Instruments,
        Internal
    };

    // A scored search result
    struct SearchResult
    {
        int typeIndex = -1;      // Index into KnownPluginList types
        String name;             // Plugin name
        String manufacturer;     // Manufacturer name
        String formatName;       // VST3, Internal, etc.
        String category;         // Plugin category
        int score = 0;           // Fuzzy match score (higher = better)
        bool isInternal = false; // Whether it's a built-in plugin
    };

    // TextEditor::Listener
    void textEditorTextChanged(TextEditor& editor) override;

    // ListBoxModel
    int getNumRows() override;
    void paintListBoxItem(int rowNumber, Graphics& g, int width, int height, bool rowIsSelected) override;
    void listBoxItemClicked(int row, const MouseEvent& e) override;
    void listBoxItemDoubleClicked(int row, const MouseEvent& e) override;

    // Internal methods
    void updateResults();
    int fuzzyScore(const String& query, const String& target) const;
    bool matchesCategory(const PluginDescription& type) const;
    void selectPlugin(int resultIndex);
    void paintFormatBadge(Graphics& g, const String& format, Rectangle<int> bounds) const;

    // Data
    KnownPluginList& pluginList;
    std::vector<SearchResult> results;

    // UI components
    TextEditor searchBar;
    ListBox resultsList;

    // Category tab buttons
    OwnedArray<TextButton> categoryButtons;
    Category currentCategory = Category::All;

    // Panel dimensions
    static constexpr int panelWidth = 480;
    static constexpr int panelHeight = 420;
    static constexpr int searchBarHeight = 40;
    static constexpr int tabRowHeight = 32;
    static constexpr int resultRowHeight = 48;
    static constexpr int cornerRadius = 12;
    static constexpr int panelPadding = 12;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginSearchOverlay)
};

#endif
