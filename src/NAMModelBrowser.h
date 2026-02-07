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

    void setHoveredRow(int row) { hoveredRow = row; }
    int getHoveredRow() const { return hoveredRow; }

  private:
    void rebuildFilteredList();

    std::vector<NAMModelInfo> allModels;
    std::vector<size_t> filteredIndices;
    String currentFilter;
    int hoveredRow = -1;
};

//==============================================================================
/**
    Info structure for IR files (impulse responses).
*/
struct IRFileInfo
{
    std::string name;
    std::string filePath;
    int64_t fileSize = 0;
    double durationSeconds = 0.0;
    double sampleRate = 0.0;
    int numChannels = 0;
};

//==============================================================================
/**
    ListBox model for displaying IR files with filtering support.
*/
class IRListModel : public ListBoxModel
{
  public:
    IRListModel() = default;

    void setFiles(const std::vector<IRFileInfo>& files);
    void setFilter(const String& filter);

    int getNumRows() override;
    void paintListBoxItem(int rowNumber, Graphics& g, int width, int height, bool rowIsSelected) override;

    const IRFileInfo* getFileAt(int index) const;
    int getFilteredCount() const { return static_cast<int>(filteredIndices.size()); }

    void setHoveredRow(int row) { hoveredRow = row; }
    int getHoveredRow() const { return hoveredRow; }

  private:
    void rebuildFilteredList();

    std::vector<IRFileInfo> allFiles;
    std::vector<size_t> filteredIndices;
    String currentFilter;
    int hoveredRow = -1;
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
    void mouseUp(const MouseEvent& event) override;
    void mouseDoubleClick(const MouseEvent& event) override;
    void mouseMove(const MouseEvent& event) override;
    void mouseExit(const MouseEvent& event) override;

    void scanDirectory(const File& directory);
    void refreshModelList();

  private:
    void updateDetailsPanel(const NAMModelInfo* model);
    void loadSelectedModel();
    void onListSelectionChanged();
    void switchToTab(int tabIndex);

    // IR browser methods
    void scanIRDirectory(const File& directory);
    void addIRFileInfo(const File& file);
    void updateIRDetailsPanel(const IRFileInfo* irInfo);
    void loadSelectedIR();
    void onIRListSelectionChanged();

    NAMProcessor* namProcessor;
    std::function<void()> onModelLoadedCallback;

    // Tab buttons
    std::unique_ptr<TextButton> localTabButton;
    std::unique_ptr<TextButton> onlineTabButton;
    std::unique_ptr<TextButton> irTabButton;
    int currentTab = 0;  // 0 = Local, 1 = Online, 2 = IRs

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
    std::unique_ptr<Label> filePathLabel;
    std::unique_ptr<Label> filePathValue;

    // Status bar
    std::unique_ptr<Label> statusLabel;

    // Empty state
    std::unique_ptr<Label> emptyStateLabel;

    File currentDirectory;
    std::vector<NAMModelInfo> models;

    std::unique_ptr<FileChooser> folderChooser;

    // IR browser components (separate directory from NAM models)
    IRListModel irListModel;
    std::unique_ptr<ListBox> irList;
    std::unique_ptr<TextButton> irBrowseFolderButton;
    std::unique_ptr<TextButton> irLoadButton;

    // IR details labels
    std::unique_ptr<Label> irDetailsTitle;
    std::unique_ptr<Label> irNameLabel;
    std::unique_ptr<Label> irNameValue;
    std::unique_ptr<Label> irDurationLabel;
    std::unique_ptr<Label> irDurationValue;
    std::unique_ptr<Label> irSampleRateLabel;
    std::unique_ptr<Label> irSampleRateValue;
    std::unique_ptr<Label> irChannelsLabel;
    std::unique_ptr<Label> irChannelsValue;
    std::unique_ptr<Label> irFileSizeLabel;
    std::unique_ptr<Label> irFileSizeValue;
    std::unique_ptr<Label> irFilePathLabel;
    std::unique_ptr<Label> irFilePathValue;

    // IR state (separate directory from NAM models)
    File irDirectory;
    std::vector<IRFileInfo> irFiles;
    std::unique_ptr<FileChooser> irFolderChooser;

    // Scanning state
    bool isScanning = false;

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
