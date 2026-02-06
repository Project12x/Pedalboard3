/*
  ==============================================================================

    NAMModelBrowser.h
    Browser window for selecting and loading NAM model files

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "NAMCore.h"

#include <functional>
#include <vector>

class NAMProcessor;
class NAMOnlineBrowserComponent;

//==============================================================================
/**
    ListBox model for displaying NAM models with filtering support.
*/
class NAMModelListModel : public ListBoxModel
{
  public:
    NAMModelListModel() = default;

    void setModels(const std::vector<NAMModelInfo>& models);
    void setFilter(const String& filter);

    int getNumRows() override;
    void paintListBoxItem(int rowNumber, Graphics& g, int width, int height, bool rowIsSelected) override;

    const NAMModelInfo* getModelAt(int index) const;
    int getFilteredCount() const { return static_cast<int>(filteredIndices.size()); }

  private:
    void rebuildFilteredList();

    std::vector<NAMModelInfo> allModels;
    std::vector<size_t> filteredIndices;
    String currentFilter;
};

//==============================================================================
/**
    Main component for the NAM model browser.
    Contains a model list, search box, and details panel.
*/
class NAMModelBrowserComponent : public Component,
                                  public Button::Listener,
                                  public TextEditor::Listener
{
  public:
    NAMModelBrowserComponent(NAMProcessor* processor, std::function<void()> onModelLoaded);
    ~NAMModelBrowserComponent() override;

    void paint(Graphics& g) override;
    void resized() override;
    void buttonClicked(Button* button) override;
    void textEditorTextChanged(TextEditor& editor) override;

    void scanDirectory(const File& directory);
    void refreshModelList();

  private:
    void updateDetailsPanel(const NAMModelInfo* model);
    void loadSelectedModel();
    void onListSelectionChanged();
    void switchToTab(int tabIndex);

    NAMProcessor* namProcessor;
    std::function<void()> onModelLoadedCallback;

    // Tab buttons
    std::unique_ptr<TextButton> localTabButton;
    std::unique_ptr<TextButton> onlineTabButton;
    int currentTab = 0;  // 0 = Local, 1 = Online

    // Online browser component
    std::unique_ptr<NAMOnlineBrowserComponent> onlineBrowser;

    NAMModelListModel listModel;

    std::unique_ptr<Label> titleLabel;
    std::unique_ptr<TextEditor> searchBox;
    std::unique_ptr<TextButton> refreshButton;
    std::unique_ptr<TextButton> browseFolderButton;
    std::unique_ptr<TextButton> loadButton;
    std::unique_ptr<TextButton> closeButton;
    std::unique_ptr<ListBox> modelList;

    // Details panel labels
    std::unique_ptr<Label> detailsTitle;
    std::unique_ptr<Label> nameLabel;
    std::unique_ptr<Label> nameValue;
    std::unique_ptr<Label> architectureLabel;
    std::unique_ptr<Label> architectureValue;
    std::unique_ptr<Label> sampleRateLabel;
    std::unique_ptr<Label> sampleRateValue;
    std::unique_ptr<Label> loudnessLabel;
    std::unique_ptr<Label> loudnessValue;
    std::unique_ptr<Label> metadataLabel;
    std::unique_ptr<TextEditor> metadataDisplay;

    File currentDirectory;
    std::vector<NAMModelInfo> models;

    std::unique_ptr<FileChooser> folderChooser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NAMModelBrowserComponent)
};

//==============================================================================
/**
    Document window for the NAM model browser.
    Uses singleton pattern for single instance.
*/
class NAMModelBrowser : public DocumentWindow
{
  public:
    NAMModelBrowser(NAMProcessor* processor, std::function<void()> onModelLoaded);
    ~NAMModelBrowser() override = default;

    void closeButtonPressed() override { setVisible(false); }

    static void showWindow(NAMProcessor* processor, std::function<void()> onModelLoaded);

  private:
    static std::unique_ptr<NAMModelBrowser> instance;
    static NAMProcessor* currentProcessor;
    static std::function<void()> currentCallback;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NAMModelBrowser)
};
