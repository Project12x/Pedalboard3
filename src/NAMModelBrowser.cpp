/*
  ==============================================================================

    NAMModelBrowser.cpp
    Browser window for selecting and loading NAM model files

  ==============================================================================
*/

#include "NAMModelBrowser.h"
#include "NAMOnlineBrowser.h"
#include "NAMProcessor.h"
#include "ColourScheme.h"
#include "IconManager.h"

#include <melatonin_blur/melatonin_blur.h>
#include <spdlog/spdlog.h>

//==============================================================================
// NAMModelListModel
//==============================================================================

void NAMModelListModel::setModels(const std::vector<NAMModelInfo>& newModels)
{
    allModels = newModels;
    rebuildFilteredList();
}

void NAMModelListModel::setFilter(const String& filter)
{
    currentFilter = filter.toLowerCase();
    rebuildFilteredList();
}

int NAMModelListModel::getNumRows()
{
    return static_cast<int>(filteredIndices.size());
}

void NAMModelListModel::paintListBoxItem(int rowNumber, Graphics& g, int width, int height, bool rowIsSelected)
{
    auto& colours = ColourScheme::getInstance().colours;

    // Background: selection > hover > alternating
    if (rowIsSelected)
        g.fillAll(colours["List Selection"]);
    else if (rowNumber == hoveredRow)
        g.fillAll(colours["List Selection"].withAlpha(0.15f));
    else if (rowNumber % 2 == 0)
        g.fillAll(colours["Dialog Inner Background"]);
    else
        g.fillAll(colours["Dialog Inner Background"].darker(0.05f));

    if (rowNumber >= 0 && rowNumber < static_cast<int>(filteredIndices.size()))
    {
        const auto& model = allModels[filteredIndices[rowNumber]];
        int textX = 10;
        int halfHeight = height / 2;

        // Name on top line
        g.setColour(colours["Text Colour"]);
        g.setFont(13.0f);
        g.drawText(String(model.name), textX, 2, width - 20, halfHeight,
                   Justification::centredLeft, true);

        // Architecture on bottom line (dimmed)
        g.setColour(colours["Text Colour"].withAlpha(0.6f));
        g.setFont(11.0f);
        g.drawText(String(model.architecture), textX, halfHeight, width - 20, halfHeight - 2,
                   Justification::centredLeft, true);
    }

    // Bottom separator
    g.setColour(colours["Text Colour"].withAlpha(0.1f));
    g.drawHorizontalLine(height - 1, 0, width);
}

const NAMModelInfo* NAMModelListModel::getModelAt(int index) const
{
    if (index >= 0 && index < static_cast<int>(filteredIndices.size()))
    {
        return &allModels[filteredIndices[index]];
    }
    return nullptr;
}

void NAMModelListModel::rebuildFilteredList()
{
    filteredIndices.clear();

    for (size_t i = 0; i < allModels.size(); ++i)
    {
        if (currentFilter.isEmpty())
        {
            filteredIndices.push_back(i);
        }
        else
        {
            String name = String(allModels[i].name).toLowerCase();
            String arch = String(allModels[i].architecture).toLowerCase();

            if (name.contains(currentFilter) || arch.contains(currentFilter))
            {
                filteredIndices.push_back(i);
            }
        }
    }
}

//==============================================================================
// PillTabLookAndFeel - Custom look for pill-style tab buttons
//==============================================================================

class PillTabLookAndFeel : public LookAndFeel_V4
{
  public:
    void drawButtonBackground(Graphics& g, Button& button, const Colour& /*backgroundColour*/,
                               bool isMouseOverButton, bool isButtonDown) override
    {
        auto& colours = ::ColourScheme::getInstance().colours;
        auto bounds = button.getLocalBounds().toFloat().reduced(2);
        float cornerRadius = bounds.getHeight() / 2;

        if (button.getToggleState())
        {
            // Active: filled pill with accent color
            auto fillColour = colours["Accent Colour"];
            if (isButtonDown)
                fillColour = fillColour.darker(0.1f);
            else if (isMouseOverButton)
                fillColour = fillColour.brighter(0.1f);

            g.setColour(fillColour);
            g.fillRoundedRectangle(bounds, cornerRadius);
        }
        else
        {
            // Inactive: subtle hover effect only
            if (isMouseOverButton || isButtonDown)
            {
                g.setColour(colours["Text Colour"].withAlpha(0.1f));
                g.fillRoundedRectangle(bounds, cornerRadius);
            }
        }
    }

    void drawButtonText(Graphics& g, TextButton& button, bool /*isMouseOverButton*/, bool /*isButtonDown*/) override
    {
        auto& colours = ::ColourScheme::getInstance().colours;
        auto bounds = button.getLocalBounds().toFloat();

        g.setFont(Font(13.0f, Font::bold));

        if (button.getToggleState())
            g.setColour(Colours::white);
        else
            g.setColour(colours["Text Colour"].withAlpha(0.7f));

        g.drawText(button.getButtonText(), bounds, Justification::centred);
    }
};

static PillTabLookAndFeel pillTabLookAndFeel;

//==============================================================================
// NAMModelBrowserComponent
//==============================================================================

NAMModelBrowserComponent::NAMModelBrowserComponent(NAMProcessor* processor, std::function<void()> onModelLoaded)
    : namProcessor(processor), onModelLoadedCallback(std::move(onModelLoaded))
{
    auto& colours = ColourScheme::getInstance().colours;

    // Title
    titleLabel = std::make_unique<Label>("title", "NAM Model Browser");
    titleLabel->setFont(Font(18.0f, Font::bold));
    titleLabel->setColour(Label::textColourId, colours["Text Colour"]);
    addAndMakeVisible(titleLabel.get());

    // Tab buttons with pill-style look
    localTabButton = std::make_unique<TextButton>("Local");
    localTabButton->setClickingTogglesState(true);
    localTabButton->setToggleState(true, dontSendNotification);
    localTabButton->setRadioGroupId(1);
    localTabButton->setLookAndFeel(&pillTabLookAndFeel);
    localTabButton->addListener(this);
    addAndMakeVisible(localTabButton.get());

    onlineTabButton = std::make_unique<TextButton>("Online");
    onlineTabButton->setClickingTogglesState(true);
    onlineTabButton->setRadioGroupId(1);
    onlineTabButton->setLookAndFeel(&pillTabLookAndFeel);
    onlineTabButton->addListener(this);
    addAndMakeVisible(onlineTabButton.get());

    // Online browser component (created but initially hidden)
    onlineBrowser = std::make_unique<NAMOnlineBrowserComponent>(processor, onModelLoaded);
    onlineBrowser->setVisible(false);
    addAndMakeVisible(onlineBrowser.get());

    // Search box
    searchBox = std::make_unique<TextEditor>("search");
    searchBox->setTextToShowWhenEmpty("Search models...", colours["Text Colour"].withAlpha(0.5f));
    searchBox->addListener(this);
    searchBox->setColour(TextEditor::backgroundColourId, colours["Dialog Inner Background"]);
    searchBox->setColour(TextEditor::textColourId, colours["Text Colour"]);
    searchBox->setColour(TextEditor::outlineColourId, colours["Text Colour"].withAlpha(0.3f));
    addAndMakeVisible(searchBox.get());

    // Buttons
    refreshButton = std::make_unique<TextButton>("Refresh");
    refreshButton->addListener(this);
    addAndMakeVisible(refreshButton.get());

    browseFolderButton = std::make_unique<TextButton>("Browse Folder...");
    browseFolderButton->addListener(this);
    addAndMakeVisible(browseFolderButton.get());

    loadButton = std::make_unique<TextButton>("Load Model");
    loadButton->addListener(this);
    addAndMakeVisible(loadButton.get());

    closeButton = std::make_unique<TextButton>("Close");
    closeButton->addListener(this);
    addAndMakeVisible(closeButton.get());

    // Model list - transparent background for custom rounded painting
    modelList = std::make_unique<ListBox>("models", &listModel);
    modelList->setRowHeight(36);
    modelList->setColour(ListBox::backgroundColourId, Colours::transparentBlack);
    modelList->setColour(ListBox::outlineColourId, Colours::transparentBlack);
    modelList->setOutlineThickness(0);
    modelList->setMultipleSelectionEnabled(false);
    modelList->addMouseListener(this, true);
    addAndMakeVisible(modelList.get());

    // Details panel
    detailsTitle = std::make_unique<Label>("detailsTitle", "Model Details");
    detailsTitle->setFont(Font(14.0f, Font::bold));
    detailsTitle->setColour(Label::textColourId, colours["Text Colour"]);
    addAndMakeVisible(detailsTitle.get());

    auto createLabelPair = [&colours, this](std::unique_ptr<Label>& labelPtr, std::unique_ptr<Label>& valuePtr,
                                             const String& labelText, const String& valueText) {
        labelPtr = std::make_unique<Label>("", labelText);
        labelPtr->setFont(Font(12.0f, Font::bold));
        labelPtr->setColour(Label::textColourId, colours["Text Colour"].withAlpha(0.7f));
        addAndMakeVisible(labelPtr.get());

        valuePtr = std::make_unique<Label>("", valueText);
        valuePtr->setFont(Font(12.0f));
        valuePtr->setColour(Label::textColourId, colours["Text Colour"]);
        addAndMakeVisible(valuePtr.get());
    };

    createLabelPair(nameLabel, nameValue, "Name:", "-");
    createLabelPair(architectureLabel, architectureValue, "Architecture:", "-");
    createLabelPair(sampleRateLabel, sampleRateValue, "Sample Rate:", "-");
    createLabelPair(loudnessLabel, loudnessValue, "Loudness:", "-");

    metadataLabel = std::make_unique<Label>("", "Metadata:");
    metadataLabel->setFont(Font(12.0f, Font::bold));
    metadataLabel->setColour(Label::textColourId, colours["Text Colour"].withAlpha(0.7f));
    addAndMakeVisible(metadataLabel.get());

    metadataDisplay = std::make_unique<TextEditor>("metadata");
    metadataDisplay->setMultiLine(true);
    metadataDisplay->setReadOnly(true);
    metadataDisplay->setColour(TextEditor::backgroundColourId, colours["Dialog Inner Background"]);
    metadataDisplay->setColour(TextEditor::textColourId, colours["Text Colour"]);
    metadataDisplay->setColour(TextEditor::outlineColourId, colours["Text Colour"].withAlpha(0.3f));
    metadataDisplay->setFont(Font(11.0f));
    addAndMakeVisible(metadataDisplay.get());

    // File path in details
    createLabelPair(filePathLabel, filePathValue, "File:", "-");
    filePathValue->setMinimumHorizontalScale(0.5f);

    // Status bar
    statusLabel = std::make_unique<Label>("status", "");
    statusLabel->setFont(Font(11.0f));
    statusLabel->setColour(Label::textColourId, colours["Text Colour"].withAlpha(0.6f));
    addAndMakeVisible(statusLabel.get());

    // Empty state message
    emptyStateLabel = std::make_unique<Label>("emptyState", "No NAM models found.\nUse 'Browse Folder...' to select a folder\nor download models from the Online tab.");
    emptyStateLabel->setFont(Font(13.0f));
    emptyStateLabel->setColour(Label::textColourId, colours["Text Colour"].withAlpha(0.5f));
    emptyStateLabel->setJustificationType(Justification::centred);
    emptyStateLabel->setVisible(false);
    addAndMakeVisible(emptyStateLabel.get());

    // Start with NAM download directory (same as Tone3000DownloadManager uses)
    currentDirectory = File::getSpecialLocation(File::userDocumentsDirectory)
        .getChildFile("Pedalboard3")
        .getChildFile("NAM Models");

    // If the directory doesn't exist yet, fall back to Documents
    if (!currentDirectory.isDirectory())
        currentDirectory = File::getSpecialLocation(File::userDocumentsDirectory);

    setSize(700, 500);

    // Auto-scan on creation
    scanDirectory(currentDirectory);
}

NAMModelBrowserComponent::~NAMModelBrowserComponent()
{
    // Clear custom LookAndFeel before destruction
    localTabButton->setLookAndFeel(nullptr);
    onlineTabButton->setLookAndFeel(nullptr);
}

void NAMModelBrowserComponent::paint(Graphics& g)
{
    auto& colours = ColourScheme::getInstance().colours;
    auto bgColour = colours["Window Background"];

    // Gradient background
    ColourGradient bgGradient(bgColour.brighter(0.06f), 0, 0,
                               bgColour.darker(0.06f), 0, static_cast<float>(getHeight()), false);
    g.setGradientFill(bgGradient);
    g.fillAll();

    // Only draw panels if on Local tab
    if (currentTab == 0)
    {
        auto bounds = getLocalBounds().reduced(16);
        bounds.removeFromTop(30 + 8 + 28 + 8); // Title + tabs + search row spacing
        bounds.removeFromBottom(20 + 4 + 36 + 8); // Status + button row

        int listWidth = static_cast<int>(bounds.getWidth() * 0.55f);
        auto listArea = bounds.removeFromLeft(listWidth);
        bounds.removeFromLeft(16); // Gap

        // Draw rounded list background
        auto listBounds = listArea.toFloat();
        g.setColour(colours["Dialog Inner Background"]);
        g.fillRoundedRectangle(listBounds, 8.0f);
        g.setColour(colours["Text Colour"].withAlpha(0.2f));
        g.drawRoundedRectangle(listBounds.reduced(0.5f), 8.0f, 1.0f);

        // Draw card-style details panel with shadow
        auto detailsBounds = bounds.toFloat();
        Path detailsPath;
        detailsPath.addRoundedRectangle(detailsBounds, 8.0f);

        // Drop shadow
        melatonin::DropShadow shadow(Colours::black.withAlpha(0.2f), 8, {2, 2});
        shadow.render(g, detailsPath);

        // Card fill with subtle gradient
        ColourGradient cardGrad(colours["Dialog Inner Background"].brighter(0.04f),
                                 detailsBounds.getX(), detailsBounds.getY(),
                                 colours["Dialog Inner Background"].darker(0.04f),
                                 detailsBounds.getX(), detailsBounds.getBottom(), false);
        g.setGradientFill(cardGrad);
        g.fillPath(detailsPath);

        // Card border
        g.setColour(colours["Text Colour"].withAlpha(0.15f));
        g.strokePath(detailsPath, PathStrokeType(1.0f));
    }
}

void NAMModelBrowserComponent::resized()
{
    auto bounds = getLocalBounds().reduced(16);

    // Title row with tab buttons
    auto titleRow = bounds.removeFromTop(30);
    titleLabel->setBounds(titleRow.removeFromLeft(180));

    // Tab buttons on the right side of title
    titleRow.removeFromLeft(16);
    localTabButton->setBounds(titleRow.removeFromLeft(60));
    titleRow.removeFromLeft(4);
    onlineTabButton->setBounds(titleRow.removeFromLeft(60));

    bounds.removeFromTop(8);

    // Online browser takes the full remaining area (minus close button row)
    if (currentTab == 1)
    {
        // Button row at bottom for close button only
        auto buttonRow = bounds.removeFromBottom(36);
        bounds.removeFromBottom(8);
        closeButton->setBounds(buttonRow.removeFromRight(70));

        onlineBrowser->setBounds(bounds);

        // Hide local browser elements
        searchBox->setVisible(false);
        refreshButton->setVisible(false);
        browseFolderButton->setVisible(false);
        loadButton->setVisible(false);
        modelList->setVisible(false);
        detailsTitle->setVisible(false);
        nameLabel->setVisible(false);
        nameValue->setVisible(false);
        architectureLabel->setVisible(false);
        architectureValue->setVisible(false);
        sampleRateLabel->setVisible(false);
        sampleRateValue->setVisible(false);
        loudnessLabel->setVisible(false);
        loudnessValue->setVisible(false);
        metadataLabel->setVisible(false);
        metadataDisplay->setVisible(false);
        filePathLabel->setVisible(false);
        filePathValue->setVisible(false);
        statusLabel->setVisible(false);
        emptyStateLabel->setVisible(false);
        return;
    }

    // Show local browser elements
    searchBox->setVisible(true);
    refreshButton->setVisible(true);
    browseFolderButton->setVisible(true);
    loadButton->setVisible(true);
    detailsTitle->setVisible(true);
    nameLabel->setVisible(true);
    nameValue->setVisible(true);
    architectureLabel->setVisible(true);
    architectureValue->setVisible(true);
    sampleRateLabel->setVisible(true);
    sampleRateValue->setVisible(true);
    loudnessLabel->setVisible(true);
    loudnessValue->setVisible(true);
    metadataLabel->setVisible(true);
    metadataDisplay->setVisible(true);
    filePathLabel->setVisible(true);
    filePathValue->setVisible(true);
    statusLabel->setVisible(true);

    // Show list or empty state based on model count
    bool hasModels = listModel.getNumRows() > 0;
    modelList->setVisible(hasModels);
    emptyStateLabel->setVisible(!hasModels);

    // Search and refresh row
    auto searchRow = bounds.removeFromTop(28);
    refreshButton->setBounds(searchRow.removeFromRight(70));
    searchRow.removeFromRight(8);
    browseFolderButton->setBounds(searchRow.removeFromRight(110));
    searchRow.removeFromRight(8);
    searchBox->setBounds(searchRow);
    bounds.removeFromTop(8);

    // Status bar at bottom
    auto statusRow = bounds.removeFromBottom(20);
    statusLabel->setBounds(statusRow);
    bounds.removeFromBottom(4);

    // Button row at bottom
    auto buttonRow = bounds.removeFromBottom(36);
    bounds.removeFromBottom(8);

    closeButton->setBounds(buttonRow.removeFromRight(70));
    buttonRow.removeFromRight(8);
    loadButton->setBounds(buttonRow.removeFromRight(100));

    // Split remaining area: list (55%) and details (45%)
    int listWidth = bounds.getWidth() * 0.55f;
    auto listArea = bounds.removeFromLeft(listWidth);
    bounds.removeFromLeft(16); // Gap

    // Model list or empty state
    modelList->setBounds(listArea);
    emptyStateLabel->setBounds(listArea);

    // Details panel
    auto detailsArea = bounds;

    detailsTitle->setBounds(detailsArea.removeFromTop(24));
    detailsArea.removeFromTop(8);

    auto layoutLabelValue = [&detailsArea](Label* label, Label* value) {
        auto row = detailsArea.removeFromTop(20);
        label->setBounds(row.removeFromLeft(90));
        value->setBounds(row);
        detailsArea.removeFromTop(4);
    };

    layoutLabelValue(nameLabel.get(), nameValue.get());
    layoutLabelValue(architectureLabel.get(), architectureValue.get());
    layoutLabelValue(sampleRateLabel.get(), sampleRateValue.get());
    layoutLabelValue(loudnessLabel.get(), loudnessValue.get());

    detailsArea.removeFromTop(8);

    // File path row (wider layout)
    auto fileRow = detailsArea.removeFromTop(20);
    filePathLabel->setBounds(fileRow.removeFromLeft(40));
    filePathValue->setBounds(fileRow);
    detailsArea.removeFromTop(8);

    metadataLabel->setBounds(detailsArea.removeFromTop(20));
    detailsArea.removeFromTop(4);
    metadataDisplay->setBounds(detailsArea);
}

void NAMModelBrowserComponent::buttonClicked(Button* button)
{
    if (button == localTabButton.get())
    {
        switchToTab(0);
    }
    else if (button == onlineTabButton.get())
    {
        switchToTab(1);
    }
    else if (button == refreshButton.get())
    {
        scanDirectory(currentDirectory);
    }
    else if (button == browseFolderButton.get())
    {
        folderChooser =
            std::make_unique<FileChooser>("Select NAM Models Folder", currentDirectory, "", true);

        auto chooserFlags = FileBrowserComponent::openMode | FileBrowserComponent::canSelectDirectories;

        folderChooser->launchAsync(chooserFlags, [this](const FileChooser& fc) {
            auto result = fc.getResult();
            if (result.isDirectory())
            {
                currentDirectory = result;
                scanDirectory(currentDirectory);
            }
        });
    }
    else if (button == loadButton.get())
    {
        loadSelectedModel();
    }
    else if (button == closeButton.get())
    {
        if (auto* window = findParentComponentOfClass<NAMModelBrowser>())
            window->closeButtonPressed();
    }
}

void NAMModelBrowserComponent::switchToTab(int tabIndex)
{
    if (currentTab == tabIndex)
        return;

    currentTab = tabIndex;

    // Update tab button states
    localTabButton->setToggleState(tabIndex == 0, dontSendNotification);
    onlineTabButton->setToggleState(tabIndex == 1, dontSendNotification);

    // Show/hide appropriate content
    onlineBrowser->setVisible(tabIndex == 1);

    // Refresh local list when switching to Local tab (to pick up new downloads)
    if (tabIndex == 0)
        scanDirectory(currentDirectory);

    // Trigger layout update
    resized();
    repaint();

    spdlog::info("[NAMModelBrowser] Switched to {} tab", tabIndex == 0 ? "Local" : "Online");
}

void NAMModelBrowserComponent::textEditorTextChanged(TextEditor& editor)
{
    if (&editor == searchBox.get())
    {
        listModel.setFilter(searchBox->getText());
        modelList->updateContent();
        modelList->repaint();
    }
}

void NAMModelBrowserComponent::scanDirectory(const File& directory)
{
    models.clear();

    if (!directory.isDirectory())
    {
        listModel.setModels(models);
        modelList->updateContent();
        return;
    }

    spdlog::info("[NAMModelBrowser] Scanning directory: {}", directory.getFullPathName().toStdString());

    // Find all .nam files recursively
    auto namFiles = directory.findChildFiles(File::findFiles, true, "*.nam");

    for (const auto& file : namFiles)
    {
        NAMModelInfo info;
        if (NAMCore::getModelInfo(file.getFullPathName().toStdString(), info))
        {
            models.push_back(std::move(info));
        }
    }

    spdlog::info("[NAMModelBrowser] Found {} NAM models", models.size());

    // Sort by name
    std::sort(models.begin(), models.end(),
              [](const NAMModelInfo& a, const NAMModelInfo& b) { return a.name < b.name; });

    listModel.setModels(models);
    modelList->updateContent();
    modelList->repaint();

    // Update status bar
    String statusText = currentDirectory.getFullPathName();
    if (models.empty())
        statusText += " - No models found";
    else if (models.size() == 1)
        statusText += " - 1 model";
    else
        statusText += " - " + String(models.size()) + " models";
    statusLabel->setText(statusText, dontSendNotification);

    // Update empty state visibility
    bool hasModels = !models.empty();
    modelList->setVisible(hasModels);
    emptyStateLabel->setVisible(!hasModels);

    // Clear details
    updateDetailsPanel(nullptr);
}

void NAMModelBrowserComponent::refreshModelList()
{
    scanDirectory(currentDirectory);
}

void NAMModelBrowserComponent::updateDetailsPanel(const NAMModelInfo* model)
{
    if (model)
    {
        nameValue->setText(String(model->name), dontSendNotification);
        architectureValue->setText(String(model->architecture), dontSendNotification);

        if (model->expectedSampleRate > 0)
            sampleRateValue->setText(String(static_cast<int>(model->expectedSampleRate)) + " Hz", dontSendNotification);
        else
            sampleRateValue->setText("Unknown", dontSendNotification);

        if (model->hasLoudness)
            loudnessValue->setText(String(model->loudness, 1) + " dB", dontSendNotification);
        else
            loudnessValue->setText("N/A", dontSendNotification);

        // Show file path (just the filename, with tooltip for full path)
        File modelFile(model->filePath);
        filePathValue->setText(modelFile.getFileName(), dontSendNotification);
        filePathValue->setTooltip(String(model->filePath));

        metadataDisplay->setText(String(model->metadata), dontSendNotification);
    }
    else
    {
        nameValue->setText("-", dontSendNotification);
        architectureValue->setText("-", dontSendNotification);
        sampleRateValue->setText("-", dontSendNotification);
        loudnessValue->setText("-", dontSendNotification);
        filePathValue->setText("-", dontSendNotification);
        filePathValue->setTooltip("");
        metadataDisplay->setText("", dontSendNotification);
    }
}

void NAMModelBrowserComponent::loadSelectedModel()
{
    auto selectedRow = modelList->getSelectedRow();
    if (selectedRow >= 0)
    {
        const auto* model = listModel.getModelAt(selectedRow);
        if (model && namProcessor)
        {
            File modelFile(model->filePath);
            if (namProcessor->loadModel(modelFile))
            {
                spdlog::info("[NAMModelBrowser] Loaded model: {}", model->name);

                if (onModelLoadedCallback)
                    onModelLoadedCallback();
            }
            else
            {
                spdlog::error("[NAMModelBrowser] Failed to load model: {}", model->name);
            }
        }
    }
}

void NAMModelBrowserComponent::onListSelectionChanged()
{
    auto selectedRow = modelList->getSelectedRow();
    const auto* model = listModel.getModelAt(selectedRow);
    updateDetailsPanel(model);
}

void NAMModelBrowserComponent::mouseUp(const MouseEvent& event)
{
    // Handle clicks on the model list to update selection
    if (modelList != nullptr && modelList->isParentOf(event.eventComponent))
    {
        juce::MessageManager::callAsync([this]() {
            onListSelectionChanged();
        });
    }
}

void NAMModelBrowserComponent::mouseDoubleClick(const MouseEvent& event)
{
    // Double-click on list item loads the model
    if (modelList != nullptr && modelList->isParentOf(event.eventComponent))
    {
        juce::MessageManager::callAsync([this]() {
            loadSelectedModel();
        });
    }
}

void NAMModelBrowserComponent::mouseMove(const MouseEvent& event)
{
    if (modelList != nullptr && modelList->isParentOf(event.eventComponent))
    {
        auto localPoint = modelList->getLocalPoint(event.eventComponent, event.position);
        int row = modelList->getRowContainingPosition(static_cast<int>(localPoint.x),
                                                       static_cast<int>(localPoint.y));
        if (row != listModel.getHoveredRow())
        {
            listModel.setHoveredRow(row);
            modelList->repaint();
        }
    }
}

void NAMModelBrowserComponent::mouseExit(const MouseEvent& /*event*/)
{
    if (listModel.getHoveredRow() != -1)
    {
        listModel.setHoveredRow(-1);
        modelList->repaint();
    }
}

//==============================================================================
// NAMModelBrowser
//==============================================================================

std::unique_ptr<NAMModelBrowser> NAMModelBrowser::instance;
NAMProcessor* NAMModelBrowser::currentProcessor = nullptr;
std::function<void()> NAMModelBrowser::currentCallback;

NAMModelBrowser::NAMModelBrowser(NAMProcessor* processor, std::function<void()> onModelLoaded)
    : DocumentWindow("NAM Model Browser", ColourScheme::getInstance().colours["Window Background"],
                     DocumentWindow::closeButton)
{
    setContentOwned(new NAMModelBrowserComponent(processor, std::move(onModelLoaded)), true);
    setResizable(true, false);
    setUsingNativeTitleBar(true);
    centreWithSize(700, 500);
}

void NAMModelBrowser::showWindow(NAMProcessor* processor, std::function<void()> onModelLoaded)
{
    // If processor changed or window doesn't exist, recreate
    if (!instance || currentProcessor != processor)
    {
        currentProcessor = processor;
        currentCallback = std::move(onModelLoaded);
        instance = std::make_unique<NAMModelBrowser>(processor, currentCallback);
    }

    instance->setVisible(true);
    instance->toFront(true);
}
