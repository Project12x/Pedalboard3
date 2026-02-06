/*
  ==============================================================================

    NAMModelBrowser.cpp
    Browser window for selecting and loading NAM model files

  ==============================================================================
*/

#include "NAMModelBrowser.h"
#include "NAMProcessor.h"
#include "ColourScheme.h"

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

    if (rowIsSelected)
        g.fillAll(colours["List Selection"]);
    else if (rowNumber % 2 == 0)
        g.fillAll(colours["Dialog Inner Background"]);
    else
        g.fillAll(colours["Dialog Inner Background"].darker(0.05f));

    if (rowNumber >= 0 && rowNumber < static_cast<int>(filteredIndices.size()))
    {
        const auto& model = allModels[filteredIndices[rowNumber]];

        g.setColour(colours["Text Colour"]);
        g.setFont(13.0f);
        g.drawText(String(model.name), 8, 0, width - 120, height, Justification::centredLeft, true);

        // Show architecture in smaller text on the right
        g.setColour(colours["Text Colour"].withAlpha(0.6f));
        g.setFont(11.0f);
        g.drawText(String(model.architecture), width - 110, 0, 100, height, Justification::centredRight, true);
    }
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

    // Model list
    modelList = std::make_unique<ListBox>("models", &listModel);
    modelList->setRowHeight(24);
    modelList->setColour(ListBox::backgroundColourId, colours["Dialog Inner Background"]);
    modelList->setColour(ListBox::outlineColourId, colours["Text Colour"].withAlpha(0.3f));
    modelList->setOutlineThickness(1);
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

    // Start with user documents folder
    currentDirectory = File::getSpecialLocation(File::userDocumentsDirectory);

    setSize(700, 500);
}

NAMModelBrowserComponent::~NAMModelBrowserComponent() = default;

void NAMModelBrowserComponent::paint(Graphics& g)
{
    auto& colours = ColourScheme::getInstance().colours;
    g.fillAll(colours["Window Background"]);

    // Draw separator between list and details
    auto bounds = getLocalBounds().reduced(16);
    bounds.removeFromTop(30 + 8 + 28 + 8); // Title + search row
    bounds.removeFromBottom(36 + 8);        // Button row

    int listWidth = bounds.getWidth() * 0.55f;
    g.setColour(colours["Text Colour"].withAlpha(0.2f));
    g.drawVerticalLine(16 + listWidth + 8, bounds.getY(), bounds.getBottom());
}

void NAMModelBrowserComponent::resized()
{
    auto bounds = getLocalBounds().reduced(16);

    // Title
    titleLabel->setBounds(bounds.removeFromTop(30));
    bounds.removeFromTop(8);

    // Search and refresh row
    auto searchRow = bounds.removeFromTop(28);
    refreshButton->setBounds(searchRow.removeFromRight(70));
    searchRow.removeFromRight(8);
    browseFolderButton->setBounds(searchRow.removeFromRight(110));
    searchRow.removeFromRight(8);
    searchBox->setBounds(searchRow);
    bounds.removeFromTop(8);

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

    // Model list
    modelList->setBounds(listArea);

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
    metadataLabel->setBounds(detailsArea.removeFromTop(20));
    detailsArea.removeFromTop(4);
    metadataDisplay->setBounds(detailsArea);
}

void NAMModelBrowserComponent::buttonClicked(Button* button)
{
    if (button == refreshButton.get())
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

        metadataDisplay->setText(String(model->metadata), dontSendNotification);
    }
    else
    {
        nameValue->setText("-", dontSendNotification);
        architectureValue->setText("-", dontSendNotification);
        sampleRateValue->setText("-", dontSendNotification);
        loudnessValue->setText("-", dontSendNotification);
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
