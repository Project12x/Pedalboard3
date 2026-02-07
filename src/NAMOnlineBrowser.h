/*
  ==============================================================================

    NAMOnlineBrowser.h
    Online browser component for TONE3000 NAM models

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "Tone3000Types.h"
#include "Tone3000DownloadManager.h"

#include <functional>
#include <vector>

class NAMProcessor;

//==============================================================================
/**
    ListBox model for displaying TONE3000 search results.
*/
class Tone3000ResultsListModel : public juce::ListBoxModel
{
  public:
    Tone3000ResultsListModel();

    void setResults(const std::vector<Tone3000::ToneInfo>& results);
    void clear();

    int getNumRows() override;
    void paintListBoxItem(int rowNumber, juce::Graphics& g,
                          int width, int height, bool rowIsSelected) override;

    const Tone3000::ToneInfo* getToneAt(int index) const;

    // Download state tracking
    void setDownloadProgress(const juce::String& toneId, float progress);
    void setDownloadComplete(const juce::String& toneId);
    void setDownloadFailed(const juce::String& toneId);
    void setCached(const juce::String& toneId, const juce::String& localPath);

  private:
    std::vector<Tone3000::ToneInfo> tones;

    // Track download states per tone
    std::map<std::string, float> downloadProgress;  // -1 = not downloading, 0-1 = progress, 2 = complete, -2 = failed
};

//==============================================================================
/**
    Main component for browsing TONE3000 online models.
    Includes search, filters, results list, and details panel.
*/
class NAMOnlineBrowserComponent : public juce::Component,
                                   public juce::Button::Listener,
                                   public juce::TextEditor::Listener,
                                   public juce::ComboBox::Listener,
                                   public Tone3000DownloadManager::Listener
{
  public:
    NAMOnlineBrowserComponent(NAMProcessor* processor, std::function<void()> onModelLoaded);
    ~NAMOnlineBrowserComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    // Button::Listener
    void buttonClicked(juce::Button* button) override;

    // TextEditor::Listener
    void textEditorReturnKeyPressed(juce::TextEditor& editor) override;
    void textEditorTextChanged(juce::TextEditor&) override {}

    // ComboBox::Listener
    void comboBoxChanged(juce::ComboBox* comboBox) override;

    // Mouse handling for list selection
    void mouseUp(const juce::MouseEvent& event) override;

    // Tone3000DownloadManager::Listener
    void downloadQueued(const juce::String& toneId) override;
    void downloadStarted(const juce::String& toneId) override;
    void downloadProgress(const juce::String& toneId, float progress,
                          int64_t bytesDownloaded, int64_t totalBytes) override;
    void downloadCompleted(const juce::String& toneId, const juce::File& file) override;
    void downloadFailed(const juce::String& toneId, const juce::String& errorMessage) override;
    void downloadCancelled(const juce::String& toneId) override;

    void refreshAuthState();

  private:
    void performSearch();
    void updateDetailsPanel(const Tone3000::ToneInfo* tone);
    void onListSelectionChanged();
    void downloadSelectedModel();
    void loadCachedModel(const juce::String& toneId);
    void updateStatusLabel();
    void showLoginDialog();
    void logout();
    void goToPage(int page);

    NAMProcessor* namProcessor;
    std::function<void()> onModelLoadedCallback;

    Tone3000ResultsListModel listModel;

    // Search controls
    std::unique_ptr<juce::Label> searchLabel;
    std::unique_ptr<juce::TextEditor> searchBox;
    std::unique_ptr<juce::TextButton> searchButton;

    // Filter controls
    std::unique_ptr<juce::Label> gearTypeLabel;
    std::unique_ptr<juce::ComboBox> gearTypeCombo;
    std::unique_ptr<juce::Label> sortLabel;
    std::unique_ptr<juce::ComboBox> sortCombo;

    // Results list
    std::unique_ptr<juce::ListBox> resultsList;

    // Details panel
    std::unique_ptr<juce::Label> detailsTitle;
    std::unique_ptr<juce::Label> nameLabel;
    std::unique_ptr<juce::Label> nameValue;
    std::unique_ptr<juce::Label> authorLabel;
    std::unique_ptr<juce::Label> authorValue;
    std::unique_ptr<juce::Label> architectureLabel;
    std::unique_ptr<juce::Label> architectureValue;
    std::unique_ptr<juce::Label> downloadsLabel;
    std::unique_ptr<juce::Label> downloadsValue;
    std::unique_ptr<juce::Label> sizeLabel;
    std::unique_ptr<juce::Label> sizeValue;
    std::unique_ptr<juce::Label> gearLabel;
    std::unique_ptr<juce::Label> gearValue;

    // Action buttons
    std::unique_ptr<juce::TextButton> downloadButton;
    std::unique_ptr<juce::TextButton> loadButton;

    // Status bar
    std::unique_ptr<juce::Label> statusLabel;
    std::unique_ptr<juce::TextButton> loginButton;
    std::unique_ptr<juce::TextButton> logoutButton;
    std::unique_ptr<juce::TextButton> prevPageButton;
    std::unique_ptr<juce::TextButton> nextPageButton;
    std::unique_ptr<juce::Label> pageLabel;

    // Search state
    juce::String currentQuery;
    Tone3000::GearType currentGearType = Tone3000::GearType::All;
    Tone3000::SortOrder currentSortOrder = Tone3000::SortOrder::Trending;
    int currentPage = 1;
    int totalResults = 0;
    bool hasMorePages = false;
    bool isSearching = false;

    // Currently selected tone
    const Tone3000::ToneInfo* selectedTone = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NAMOnlineBrowserComponent)
};

