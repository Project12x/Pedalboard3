//  PluginSearchOverlay.h - Floating search window for plugin selection
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

/// @brief Content component for the plugin search window.
///
/// Contains a search bar with live filtering, category tabs, and a scrollable
/// results list with keyboard navigation and fuzzy matching.
class PluginSearchContent : public Component, private TextEditor::Listener, private ListBoxModel
{
  public:
    PluginSearchContent(KnownPluginList& pluginList);
    ~PluginSearchContent() override = default;

    /// @brief Reset search state and grab keyboard focus.
    void activate();

    /// @brief Callback when a plugin is selected. Passes the type index (1-based).
    std::function<void(int typeIndex)> onPluginSelected;

    /// @brief Callback when the user requests to close (Esc key).
    std::function<void()> onCloseRequested;

    // Component overrides
    void paint(Graphics& g) override;
    void resized() override;
    bool keyPressed(const KeyPress& key) override;

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
        int typeIndex = -1;
        String name;
        String manufacturer;
        String formatName;
        String category;
        int score = 0;
        bool isInternal = false;
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

    // Layout constants
    static constexpr int searchBarHeight = 40;
    static constexpr int tabRowHeight = 32;
    static constexpr int resultRowHeight = 48;
    static constexpr int contentPadding = 12;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginSearchContent)
};

/// @brief A floating DocumentWindow that hosts the PluginSearchContent.
///
/// Uses the same custom LookAndFeel as the NAM/IR browser windows for
/// consistent dark-themed title bar, rounded corners, and close button.
class PluginSearchWindow : public DocumentWindow
{
  public:
    PluginSearchWindow(KnownPluginList& pluginList);
    ~PluginSearchWindow() override;

    /// @brief Show the window centered on screen.
    void showCentred();

    /// @brief Access the content for setting callbacks.
    PluginSearchContent* getSearchContent() { return content; }

    void closeButtonPressed() override;

  private:
    PluginSearchContent* content = nullptr; // Owned by setContentOwned
    LookAndFeel* windowLAF = nullptr;       // Custom LAF, owned

    static constexpr int windowWidth = 480;
    static constexpr int windowHeight = 460;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginSearchWindow)
};

#endif
