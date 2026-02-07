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
#include <nlohmann/json.hpp>

#include <set>

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
    const int margin = 6;
    const float cornerRadius = 6.0f;
    Rectangle<float> itemBounds(static_cast<float>(margin), 2.0f,
                                 static_cast<float>(width - margin * 2),
                                 static_cast<float>(height - 4));

    // Background with rounded corners
    if (rowIsSelected)
    {
        g.setColour(colours["Accent Colour"].withAlpha(0.3f));
        g.fillRoundedRectangle(itemBounds, cornerRadius);
        g.setColour(colours["Accent Colour"]);
        g.drawRoundedRectangle(itemBounds, cornerRadius, 1.5f);
    }
    else if (rowNumber == hoveredRow)
    {
        g.setColour(colours["Text Colour"].withAlpha(0.08f));
        g.fillRoundedRectangle(itemBounds, cornerRadius);
    }

    if (rowNumber >= 0 && rowNumber < static_cast<int>(filteredIndices.size()))
    {
        const auto& model = allModels[filteredIndices[rowNumber]];
        const int textX = margin + 10;

        // Extract rig type and model type from metadata
        String rigType;
        String modelType;
        if (!model.metadata.empty())
        {
            try
            {
                auto meta = nlohmann::json::parse(model.metadata);

                // Try to get amp/gear info in order of preference
                if (meta.contains("gear") && meta["gear"].is_object())
                {
                    const auto& gear = meta["gear"];
                    if (gear.contains("amp") && gear["amp"].is_string())
                        rigType = String(gear["amp"].get<std::string>());
                }
                if (rigType.isEmpty() && meta.contains("amp") && meta["amp"].is_string())
                    rigType = String(meta["amp"].get<std::string>());
                if (rigType.isEmpty() && meta.contains("gear") && meta["gear"].is_string())
                    rigType = String(meta["gear"].get<std::string>());
                if (rigType.isEmpty() && meta.contains("name") && meta["name"].is_string())
                    rigType = String(meta["name"].get<std::string>());

                // Try to get model type (preamp/amp/full chain)
                if (meta.contains("model_type") && meta["model_type"].is_string())
                    modelType = String(meta["model_type"].get<std::string>());
                else if (meta.contains("type") && meta["type"].is_string())
                    modelType = String(meta["type"].get<std::string>());
                else if (meta.contains("category") && meta["category"].is_string())
                    modelType = String(meta["category"].get<std::string>());
                else if (meta.contains("capture") && meta["capture"].is_string())
                    modelType = String(meta["capture"].get<std::string>());
                else if (meta.contains("gear_type") && meta["gear_type"].is_string())
                    modelType = String(meta["gear_type"].get<std::string>());
            }
            catch (const std::exception&)
            {
                // Ignore parse errors
            }
        }

        // Badge layout - rightmost is architecture, then model type
        const int badgeHeight = 16;
        const int badgeSpacing = 4;
        int badgeX = width - margin - 6;

        // Architecture badge (rightmost)
        const int archBadgeWidth = 50;
        badgeX -= archBadgeWidth;

        Colour archColour;
        String archShort(model.architecture);
        if (archShort.containsIgnoreCase("LSTM"))
            archColour = Colour(0xff4a9eff);  // Blue
        else if (archShort.containsIgnoreCase("WaveNet"))
            archColour = Colour(0xff9b59b6);  // Purple
        else if (archShort.containsIgnoreCase("ConvNet"))
            archColour = Colour(0xff2ecc71);  // Green
        else if (archShort.containsIgnoreCase("Linear"))
            archColour = Colour(0xffe67e22);  // Orange
        else
            archColour = colours["Text Colour"].withAlpha(0.4f);

        Rectangle<float> archBadgeBounds(static_cast<float>(badgeX),
                                          (height - badgeHeight) / 2.0f,
                                          static_cast<float>(archBadgeWidth),
                                          static_cast<float>(badgeHeight));
        g.setColour(archColour.withAlpha(0.15f));
        g.fillRoundedRectangle(archBadgeBounds, badgeHeight / 2.0f);
        g.setColour(archColour.withAlpha(0.6f));
        g.drawRoundedRectangle(archBadgeBounds, badgeHeight / 2.0f, 1.0f);

        g.setFont(Font(9.0f));
        g.setColour(archColour.withAlpha(0.8f));
        g.drawText(archShort, archBadgeBounds, Justification::centred, true);

        // Model type badge (left of architecture badge, if we have type info)
        if (modelType.isNotEmpty())
        {
            badgeX -= badgeSpacing;

            // Normalize model type display text
            String typeDisplay = modelType.toLowerCase();
            Colour typeColour;
            if (typeDisplay.contains("preamp") || typeDisplay.contains("pre-amp"))
            {
                typeDisplay = "Preamp";
                typeColour = Colour(0xfff39c12);  // Yellow/gold
            }
            else if (typeDisplay.contains("full") || typeDisplay.contains("chain") || typeDisplay.contains("rig"))
            {
                typeDisplay = "Full Rig";
                typeColour = Colour(0xff1abc9c);  // Teal
            }
            else if (typeDisplay.contains("pedal"))
            {
                typeDisplay = "Pedal";
                typeColour = Colour(0xffe74c3c);  // Red
            }
            else if (typeDisplay.contains("amp"))
            {
                typeDisplay = "Amp";
                typeColour = Colour(0xff3498db);  // Blue
            }
            else
            {
                typeDisplay = modelType.substring(0, 10);  // Truncate unknown types
                typeColour = colours["Text Colour"].withAlpha(0.5f);
            }

            int typeBadgeWidth = static_cast<int>(Font(9.0f).getStringWidthFloat(typeDisplay)) + 12;
            badgeX -= typeBadgeWidth;

            Rectangle<float> typeBadgeBounds(static_cast<float>(badgeX),
                                              (height - badgeHeight) / 2.0f,
                                              static_cast<float>(typeBadgeWidth),
                                              static_cast<float>(badgeHeight));
            g.setColour(typeColour.withAlpha(0.15f));
            g.fillRoundedRectangle(typeBadgeBounds, badgeHeight / 2.0f);
            g.setColour(typeColour.withAlpha(0.6f));
            g.drawRoundedRectangle(typeBadgeBounds, badgeHeight / 2.0f, 1.0f);

            g.setFont(Font(9.0f));
            g.setColour(typeColour.withAlpha(0.8f));
            g.drawText(typeDisplay, typeBadgeBounds, Justification::centred, true);
        }

        // Model name (top line) - adjust width to not overlap badges
        const int textEndX = badgeX - 8;
        g.setColour(rowIsSelected ? colours["Text Colour"] : colours["Text Colour"].withAlpha(0.95f));
        g.setFont(Font(13.0f, Font::bold));
        g.drawText(String(model.name), textX, 4, textEndX - textX, height / 2,
                   Justification::centredLeft, true);

        // Rig type and sample rate info on bottom line
        String infoLine;
        if (rigType.isNotEmpty())
        {
            // Truncate long rig names
            if (rigType.length() > 40)
                rigType = rigType.substring(0, 37) + "...";
            infoLine = rigType;
        }
        if (model.expectedSampleRate > 0)
        {
            if (infoLine.isNotEmpty()) infoLine += "  |  ";
            infoLine += String(static_cast<int>(model.expectedSampleRate)) + " Hz";
        }

        if (infoLine.isNotEmpty())
        {
            g.setColour(colours["Text Colour"].withAlpha(0.5f));
            g.setFont(11.0f);
            g.drawText(infoLine, textX, height / 2, textEndX - textX, height / 2 - 4,
                       Justification::centredLeft, true);
        }
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
// IRListModel
//==============================================================================

void IRListModel::setFiles(const std::vector<IRFileInfo>& newFiles)
{
    allFiles = newFiles;
    rebuildFilteredList();
}

void IRListModel::setFilter(const String& filter)
{
    currentFilter = filter.toLowerCase();
    rebuildFilteredList();
}

int IRListModel::getNumRows()
{
    return static_cast<int>(filteredIndices.size());
}

void IRListModel::paintListBoxItem(int rowNumber, Graphics& g, int width, int height, bool rowIsSelected)
{
    auto& colours = ColourScheme::getInstance().colours;
    const int margin = 6;
    const float cornerRadius = 6.0f;
    Rectangle<float> itemBounds(static_cast<float>(margin), 2.0f,
                                 static_cast<float>(width - margin * 2),
                                 static_cast<float>(height - 4));

    // Background with rounded corners
    if (rowIsSelected)
    {
        g.setColour(colours["Accent Colour"].withAlpha(0.3f));
        g.fillRoundedRectangle(itemBounds, cornerRadius);
        g.setColour(colours["Accent Colour"]);
        g.drawRoundedRectangle(itemBounds, cornerRadius, 1.5f);
    }
    else if (rowNumber == hoveredRow)
    {
        g.setColour(colours["Text Colour"].withAlpha(0.08f));
        g.fillRoundedRectangle(itemBounds, cornerRadius);
    }

    if (rowNumber >= 0 && rowNumber < static_cast<int>(filteredIndices.size()))
    {
        const auto& ir = allFiles[filteredIndices[rowNumber]];
        const int textX = margin + 10;
        const int badgeWidth = 50;
        const int badgeHeight = 18;
        const int badgeX = width - margin - badgeWidth - 8;

        // Duration badge
        String durationText;
        if (ir.durationSeconds > 0)
        {
            if (ir.durationSeconds >= 1.0)
                durationText = String(ir.durationSeconds, 2) + "s";
            else
                durationText = String(static_cast<int>(ir.durationSeconds * 1000)) + "ms";
        }

        if (durationText.isNotEmpty())
        {
            Rectangle<float> badgeBounds(static_cast<float>(badgeX),
                                          (height - badgeHeight) / 2.0f,
                                          static_cast<float>(badgeWidth),
                                          static_cast<float>(badgeHeight));
            Colour badgeColour = Colour(0xff3498db);  // Blue for duration
            g.setColour(badgeColour.withAlpha(0.2f));
            g.fillRoundedRectangle(badgeBounds, badgeHeight / 2.0f);
            g.setColour(badgeColour);
            g.drawRoundedRectangle(badgeBounds, badgeHeight / 2.0f, 1.0f);

            g.setFont(Font(10.0f, Font::bold));
            g.setColour(badgeColour);
            g.drawText(durationText, badgeBounds, Justification::centred, true);
        }

        // IR name
        g.setColour(rowIsSelected ? colours["Text Colour"] : colours["Text Colour"].withAlpha(0.95f));
        g.setFont(Font(13.0f, Font::bold));
        g.drawText(String(ir.name), textX, 4, badgeX - textX - 8, height / 2,
                   Justification::centredLeft, true);

        // Sample rate and channels on bottom line
        String details;
        if (ir.sampleRate > 0)
            details = String(static_cast<int>(ir.sampleRate / 1000)) + "kHz";
        if (ir.numChannels > 0)
        {
            if (details.isNotEmpty()) details += "  |  ";
            details += ir.numChannels == 1 ? "Mono" : (ir.numChannels == 2 ? "Stereo" : String(ir.numChannels) + "ch");
        }

        if (details.isNotEmpty())
        {
            g.setColour(colours["Text Colour"].withAlpha(0.5f));
            g.setFont(11.0f);
            g.drawText(details, textX, height / 2, badgeX - textX - 8, height / 2 - 4,
                       Justification::centredLeft, true);
        }
    }
}

const IRFileInfo* IRListModel::getFileAt(int index) const
{
    if (index >= 0 && index < static_cast<int>(filteredIndices.size()))
    {
        return &allFiles[filteredIndices[index]];
    }
    return nullptr;
}

void IRListModel::rebuildFilteredList()
{
    filteredIndices.clear();

    for (size_t i = 0; i < allFiles.size(); ++i)
    {
        if (currentFilter.isEmpty())
        {
            filteredIndices.push_back(i);
        }
        else
        {
            String name = String(allFiles[i].name).toLowerCase();

            if (name.contains(currentFilter))
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

    irTabButton = std::make_unique<TextButton>("IRs");
    irTabButton->setClickingTogglesState(true);
    irTabButton->setRadioGroupId(1);
    irTabButton->setLookAndFeel(&pillTabLookAndFeel);
    irTabButton->addListener(this);
    addAndMakeVisible(irTabButton.get());

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

    // Buttons with styled colors
    auto styleButton = [&colours](TextButton* btn, bool isPrimary = false) {
        if (isPrimary)
        {
            // Primary action button (accent color)
            btn->setColour(TextButton::buttonColourId, colours["Slider Colour"]);
            btn->setColour(TextButton::buttonOnColourId, colours["Slider Colour"].brighter(0.2f));
            btn->setColour(TextButton::textColourOffId, Colours::white);
            btn->setColour(TextButton::textColourOnId, Colours::white);
        }
        else
        {
            // Secondary button
            btn->setColour(TextButton::buttonColourId, colours["Button Colour"]);
            btn->setColour(TextButton::buttonOnColourId, colours["Button Highlight"]);
            btn->setColour(TextButton::textColourOffId, colours["Text Colour"]);
            btn->setColour(TextButton::textColourOnId, colours["Text Colour"]);
        }
    };

    refreshButton = std::make_unique<TextButton>("Refresh");
    refreshButton->addListener(this);
    styleButton(refreshButton.get());
    addAndMakeVisible(refreshButton.get());

    browseFolderButton = std::make_unique<TextButton>("Browse Folder...");
    browseFolderButton->addListener(this);
    styleButton(browseFolderButton.get());
    addAndMakeVisible(browseFolderButton.get());

    loadButton = std::make_unique<TextButton>("Load Model");
    loadButton->addListener(this);
    styleButton(loadButton.get(), true);  // Primary action
    addAndMakeVisible(loadButton.get());

    closeButton = std::make_unique<TextButton>("Close");
    closeButton->addListener(this);
    styleButton(closeButton.get());
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

    // IR browser components
    irList = std::make_unique<ListBox>("irs", &irListModel);
    irList->setRowHeight(36);
    irList->setColour(ListBox::backgroundColourId, Colours::transparentBlack);
    irList->setColour(ListBox::outlineColourId, Colours::transparentBlack);
    irList->setOutlineThickness(0);
    irList->setMultipleSelectionEnabled(false);
    irList->addMouseListener(this, true);
    irList->setVisible(false);
    addAndMakeVisible(irList.get());

    irBrowseFolderButton = std::make_unique<TextButton>("Browse IR Folder...");
    irBrowseFolderButton->addListener(this);
    irBrowseFolderButton->setColour(TextButton::buttonColourId, colours["Button Colour"]);
    irBrowseFolderButton->setColour(TextButton::buttonOnColourId, colours["Button Highlight"]);
    irBrowseFolderButton->setColour(TextButton::textColourOffId, colours["Text Colour"]);
    irBrowseFolderButton->setColour(TextButton::textColourOnId, colours["Text Colour"]);
    irBrowseFolderButton->setVisible(false);
    addAndMakeVisible(irBrowseFolderButton.get());

    irLoadButton = std::make_unique<TextButton>("Load IR");
    irLoadButton->addListener(this);
    irLoadButton->setColour(TextButton::buttonColourId, colours["Slider Colour"]);
    irLoadButton->setColour(TextButton::buttonOnColourId, colours["Slider Colour"].brighter(0.2f));
    irLoadButton->setColour(TextButton::textColourOffId, Colours::white);
    irLoadButton->setColour(TextButton::textColourOnId, Colours::white);
    irLoadButton->setVisible(false);
    addAndMakeVisible(irLoadButton.get());

    // IR details panel
    irDetailsTitle = std::make_unique<Label>("irDetailsTitle", "IR Details");
    irDetailsTitle->setFont(Font(14.0f, Font::bold));
    irDetailsTitle->setColour(Label::textColourId, colours["Text Colour"]);
    irDetailsTitle->setVisible(false);
    addAndMakeVisible(irDetailsTitle.get());

    auto createIRLabelPair = [&colours, this](std::unique_ptr<Label>& labelPtr, std::unique_ptr<Label>& valuePtr,
                                               const String& labelText, const String& valueText) {
        labelPtr = std::make_unique<Label>("", labelText);
        labelPtr->setFont(Font(12.0f, Font::bold));
        labelPtr->setColour(Label::textColourId, colours["Text Colour"].withAlpha(0.7f));
        labelPtr->setVisible(false);
        addAndMakeVisible(labelPtr.get());

        valuePtr = std::make_unique<Label>("", valueText);
        valuePtr->setFont(Font(12.0f));
        valuePtr->setColour(Label::textColourId, colours["Text Colour"]);
        valuePtr->setVisible(false);
        addAndMakeVisible(valuePtr.get());
    };

    createIRLabelPair(irNameLabel, irNameValue, "Name:", "-");
    createIRLabelPair(irDurationLabel, irDurationValue, "Duration:", "-");
    createIRLabelPair(irSampleRateLabel, irSampleRateValue, "Sample Rate:", "-");
    createIRLabelPair(irChannelsLabel, irChannelsValue, "Channels:", "-");
    createIRLabelPair(irFileSizeLabel, irFileSizeValue, "File Size:", "-");
    createIRLabelPair(irFilePathLabel, irFilePathValue, "File:", "-");
    irFilePathValue->setMinimumHorizontalScale(0.5f);

    // Start with NAM download directory (same as Tone3000DownloadManager uses)
    currentDirectory = File::getSpecialLocation(File::userDocumentsDirectory)
        .getChildFile("Pedalboard3")
        .getChildFile("NAM Models");

    // Create NAM Models directory if it doesn't exist
    if (!currentDirectory.isDirectory())
        currentDirectory.createDirectory();

    // IR directory (separate from NAM models)
    irDirectory = File::getSpecialLocation(File::userDocumentsDirectory)
        .getChildFile("Pedalboard3")
        .getChildFile("IR");

    // Create IR directory if it doesn't exist
    if (!irDirectory.isDirectory())
        irDirectory.createDirectory();

    setSize(700, 500);

    // Auto-scan on creation
    scanDirectory(currentDirectory);
}

NAMModelBrowserComponent::~NAMModelBrowserComponent()
{
    // Clear custom LookAndFeel before destruction
    localTabButton->setLookAndFeel(nullptr);
    onlineTabButton->setLookAndFeel(nullptr);
    irTabButton->setLookAndFeel(nullptr);
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

    // Draw panels for Local and IR tabs
    if (currentTab == 0 || currentTab == 2)
    {
        auto bounds = getLocalBounds().reduced(16);
        bounds.removeFromTop(30 + 8 + 28 + 8); // Title + tabs + search row spacing
        bounds.removeFromBottom(20 + 4 + 36 + 8); // Status + button row

        int listWidth = static_cast<int>(bounds.getWidth() * 0.55f);
        auto listArea = bounds.removeFromLeft(listWidth);
        bounds.removeFromLeft(16); // Gap

        // Draw rounded list background with subtle inner shadow
        auto listBounds = listArea.toFloat();
        g.setColour(colours["Dialog Inner Background"].darker(0.02f));
        g.fillRoundedRectangle(listBounds, 8.0f);
        g.setColour(colours["Text Colour"].withAlpha(0.15f));
        g.drawRoundedRectangle(listBounds.reduced(0.5f), 8.0f, 1.0f);

        // Draw card-style details panel with shadow
        auto detailsBounds = bounds.toFloat();
        Path detailsPath;
        detailsPath.addRoundedRectangle(detailsBounds, 8.0f);

        // Drop shadow
        melatonin::DropShadow shadow(Colours::black.withAlpha(0.25f), 10, {2, 3});
        shadow.render(g, detailsPath);

        // Card fill with subtle gradient
        ColourGradient cardGrad(colours["Dialog Inner Background"].brighter(0.05f),
                                 detailsBounds.getX(), detailsBounds.getY(),
                                 colours["Dialog Inner Background"].darker(0.03f),
                                 detailsBounds.getX(), detailsBounds.getBottom(), false);
        g.setGradientFill(cardGrad);
        g.fillPath(detailsPath);

        // Card border with glow effect
        g.setColour(colours["Accent Colour"].withAlpha(0.1f));
        g.strokePath(detailsPath, PathStrokeType(2.0f));
        g.setColour(colours["Text Colour"].withAlpha(0.12f));
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
    titleRow.removeFromLeft(4);
    irTabButton->setBounds(titleRow.removeFromLeft(50));

    bounds.removeFromTop(8);

    // Hide all IR components by default
    auto hideIRComponents = [this]() {
        irList->setVisible(false);
        irBrowseFolderButton->setVisible(false);
        irLoadButton->setVisible(false);
        irDetailsTitle->setVisible(false);
        irNameLabel->setVisible(false);
        irNameValue->setVisible(false);
        irDurationLabel->setVisible(false);
        irDurationValue->setVisible(false);
        irSampleRateLabel->setVisible(false);
        irSampleRateValue->setVisible(false);
        irChannelsLabel->setVisible(false);
        irChannelsValue->setVisible(false);
        irFileSizeLabel->setVisible(false);
        irFileSizeValue->setVisible(false);
        irFilePathLabel->setVisible(false);
        irFilePathValue->setVisible(false);
    };

    // Hide all local NAM components by default
    auto hideLocalComponents = [this]() {
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
    };

    // Online browser takes the full remaining area (minus close button row)
    if (currentTab == 1)
    {
        // Button row at bottom for close button only
        auto buttonRow = bounds.removeFromBottom(36);
        bounds.removeFromBottom(8);
        closeButton->setBounds(buttonRow.removeFromRight(70));

        onlineBrowser->setBounds(bounds);

        // Hide local and IR browser elements
        hideLocalComponents();
        hideIRComponents();
        return;
    }

    // IR browser tab
    if (currentTab == 2)
    {
        hideLocalComponents();
        onlineBrowser->setVisible(false);

        // Search row with IR-specific browse button
        auto searchRow = bounds.removeFromTop(28);
        refreshButton->setBounds(searchRow.removeFromRight(70));
        refreshButton->setVisible(true);
        searchRow.removeFromRight(8);
        irBrowseFolderButton->setBounds(searchRow.removeFromRight(120));
        irBrowseFolderButton->setVisible(true);
        searchRow.removeFromRight(8);
        searchBox->setBounds(searchRow);
        searchBox->setVisible(true);
        bounds.removeFromTop(8);

        // Status bar at bottom
        auto statusRow = bounds.removeFromBottom(20);
        statusLabel->setBounds(statusRow);
        statusLabel->setVisible(true);
        bounds.removeFromBottom(4);

        // Button row at bottom
        auto buttonRow = bounds.removeFromBottom(36);
        bounds.removeFromBottom(8);
        closeButton->setBounds(buttonRow.removeFromRight(70));
        buttonRow.removeFromRight(8);
        irLoadButton->setBounds(buttonRow.removeFromRight(80));
        irLoadButton->setVisible(true);

        // Split remaining area: list (55%) and details (45%)
        int listWidth = bounds.getWidth() * 0.55f;
        auto listArea = bounds.removeFromLeft(listWidth);
        bounds.removeFromLeft(16);

        // IR list
        irList->setBounds(listArea);
        irList->setVisible(true);

        // IR details panel
        auto detailsArea = bounds;

        irDetailsTitle->setBounds(detailsArea.removeFromTop(24));
        irDetailsTitle->setVisible(true);
        detailsArea.removeFromTop(8);

        auto layoutIRLabelValue = [&detailsArea](Label* label, Label* value) {
            auto row = detailsArea.removeFromTop(20);
            label->setBounds(row.removeFromLeft(90));
            label->setVisible(true);
            value->setBounds(row);
            value->setVisible(true);
            detailsArea.removeFromTop(4);
        };

        layoutIRLabelValue(irNameLabel.get(), irNameValue.get());
        layoutIRLabelValue(irDurationLabel.get(), irDurationValue.get());
        layoutIRLabelValue(irSampleRateLabel.get(), irSampleRateValue.get());
        layoutIRLabelValue(irChannelsLabel.get(), irChannelsValue.get());
        layoutIRLabelValue(irFileSizeLabel.get(), irFileSizeValue.get());

        detailsArea.removeFromTop(8);

        // File path row
        auto fileRow = detailsArea.removeFromTop(20);
        irFilePathLabel->setBounds(fileRow.removeFromLeft(40));
        irFilePathLabel->setVisible(true);
        irFilePathValue->setBounds(fileRow);
        irFilePathValue->setVisible(true);

        return;
    }

    // Local NAM tab - hide IR components and online browser
    hideIRComponents();
    onlineBrowser->setVisible(false);

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
    else if (button == irTabButton.get())
    {
        switchToTab(2);
    }
    else if (button == refreshButton.get())
    {
        if (currentTab == 0)
            scanDirectory(currentDirectory);
        else if (currentTab == 2)
            scanIRDirectory(irDirectory);
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
    else if (button == irBrowseFolderButton.get())
    {
        irFolderChooser =
            std::make_unique<FileChooser>("Select IR Folder", irDirectory, "", true);

        auto chooserFlags = FileBrowserComponent::openMode | FileBrowserComponent::canSelectDirectories;

        irFolderChooser->launchAsync(chooserFlags, [this](const FileChooser& fc) {
            auto result = fc.getResult();
            if (result.isDirectory())
            {
                irDirectory = result;
                scanIRDirectory(irDirectory);
            }
        });
    }
    else if (button == loadButton.get())
    {
        loadSelectedModel();
    }
    else if (button == irLoadButton.get())
    {
        loadSelectedIR();
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
    irTabButton->setToggleState(tabIndex == 2, dontSendNotification);

    // Show/hide appropriate content
    onlineBrowser->setVisible(tabIndex == 1);

    // Refresh content when switching tabs (each uses its own directory)
    if (tabIndex == 0)
        scanDirectory(currentDirectory);
    else if (tabIndex == 2)
        scanIRDirectory(irDirectory);

    // Trigger layout update
    resized();
    repaint();

    const char* tabNames[] = {"Local", "Online", "IRs"};
    spdlog::info("[NAMModelBrowser] Switched to {} tab", tabNames[tabIndex]);
}

void NAMModelBrowserComponent::textEditorTextChanged(TextEditor& editor)
{
    if (&editor == searchBox.get())
    {
        if (currentTab == 0)
        {
            listModel.setFilter(searchBox->getText());
            modelList->updateContent();
            modelList->repaint();
        }
        else if (currentTab == 2)
        {
            irListModel.setFilter(searchBox->getText());
            irList->updateContent();
            irList->repaint();
        }
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

    // Show scanning indicator
    isScanning = true;
    statusLabel->setText("Scanning for NAM models...", dontSendNotification);
    statusLabel->repaint();

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

    isScanning = false;
    spdlog::info("[NAMModelBrowser] Found {} NAM models", models.size());

    // Sort by name
    std::sort(models.begin(), models.end(),
              [](const NAMModelInfo& a, const NAMModelInfo& b) { return a.name < b.name; });

    listModel.setModels(models);
    modelList->updateContent();
    modelList->repaint();

    // Update status bar with result
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

        // Format metadata as user-friendly text instead of raw JSON
        String formattedMetadata;
        if (!model->metadata.empty())
        {
            try
            {
                auto meta = nlohmann::json::parse(model->metadata);

                // Extract and format common fields
                auto addField = [&](const char* label, const char* key)
                {
                    if (meta.contains(key) && !meta[key].is_null())
                    {
                        String value;
                        if (meta[key].is_string())
                            value = String(meta[key].get<std::string>());
                        else if (meta[key].is_number())
                            value = String(meta[key].get<double>());
                        else if (meta[key].is_boolean())
                            value = meta[key].get<bool>() ? "Yes" : "No";

                        if (value.isNotEmpty())
                            formattedMetadata += String(label) + ": " + value + "\n";
                    }
                };

                // Common NAM metadata fields
                addField("Author", "author");
                addField("Name", "name");
                addField("Modeled By", "modeled_by");
                addField("Date", "date");
                addField("Gear", "gear");
                addField("Amp", "amp");
                addField("Cab", "cab");
                addField("Mic", "mic");
                addField("Description", "description");
                addField("Notes", "notes");
                addField("License", "license");
                addField("Version", "version");

                // Handle nested gear object if present
                if (meta.contains("gear") && meta["gear"].is_object())
                {
                    const auto& gear = meta["gear"];
                    if (gear.contains("amp") && !gear["amp"].is_null())
                        formattedMetadata += "Amp: " + String(gear["amp"].get<std::string>()) + "\n";
                    if (gear.contains("cabinet") && !gear["cabinet"].is_null())
                        formattedMetadata += "Cabinet: " + String(gear["cabinet"].get<std::string>()) + "\n";
                    if (gear.contains("mic") && !gear["mic"].is_null())
                        formattedMetadata += "Mic: " + String(gear["mic"].get<std::string>()) + "\n";
                }
            }
            catch (const std::exception&)
            {
                // Fall back to raw metadata if parsing fails
                formattedMetadata = String(model->metadata);
            }
        }

        metadataDisplay->setText(formattedMetadata.trimEnd(), dontSendNotification);
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
    // Handle clicks on the IR list
    if (irList != nullptr && irList->isParentOf(event.eventComponent))
    {
        juce::MessageManager::callAsync([this]() {
            onIRListSelectionChanged();
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
    // Double-click on IR list item loads the IR
    if (irList != nullptr && irList->isParentOf(event.eventComponent))
    {
        juce::MessageManager::callAsync([this]() {
            loadSelectedIR();
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
    if (irList != nullptr && irList->isParentOf(event.eventComponent))
    {
        auto localPoint = irList->getLocalPoint(event.eventComponent, event.position);
        int row = irList->getRowContainingPosition(static_cast<int>(localPoint.x),
                                                    static_cast<int>(localPoint.y));
        if (row != irListModel.getHoveredRow())
        {
            irListModel.setHoveredRow(row);
            irList->repaint();
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
    if (irListModel.getHoveredRow() != -1)
    {
        irListModel.setHoveredRow(-1);
        irList->repaint();
    }
}

//==============================================================================
// IR Browser Methods
//==============================================================================

void NAMModelBrowserComponent::scanIRDirectory(const File& directory)
{
    irFiles.clear();

    // Show scanning indicator
    isScanning = true;
    statusLabel->setText("Scanning for IR files...", dontSendNotification);
    statusLabel->repaint();

    // Track seen file paths to avoid duplicates
    std::set<String> seenPaths;

    auto scanDir = [this, &seenPaths](const File& dir)
    {
        if (!dir.isDirectory())
            return;

        spdlog::info("[NAMModelBrowser] Scanning IR directory: {}", dir.getFullPathName().toStdString());

        // Find all IR files recursively (.wav, .aiff, .aif)
        auto wavFiles = dir.findChildFiles(File::findFiles, true, "*.wav");
        auto aiffFiles = dir.findChildFiles(File::findFiles, true, "*.aiff");
        auto aifFiles = dir.findChildFiles(File::findFiles, true, "*.aif");

        for (const auto& file : wavFiles)
        {
            if (seenPaths.find(file.getFullPathName()) == seenPaths.end())
            {
                seenPaths.insert(file.getFullPathName());
                addIRFileInfo(file);
            }
        }
        for (const auto& file : aiffFiles)
        {
            if (seenPaths.find(file.getFullPathName()) == seenPaths.end())
            {
                seenPaths.insert(file.getFullPathName());
                addIRFileInfo(file);
            }
        }
        for (const auto& file : aifFiles)
        {
            if (seenPaths.find(file.getFullPathName()) == seenPaths.end())
            {
                seenPaths.insert(file.getFullPathName());
                addIRFileInfo(file);
            }
        }
    };

    // Scan primary IR directory
    scanDir(directory);

    // Also scan NAM Models directory (TONE3000 downloads IRs there too)
    if (currentDirectory.isDirectory() && currentDirectory != directory)
    {
        scanDir(currentDirectory);
    }

    isScanning = false;
    spdlog::info("[NAMModelBrowser] Found {} IR files total", irFiles.size());

    // Sort by name
    std::sort(irFiles.begin(), irFiles.end(),
              [](const IRFileInfo& a, const IRFileInfo& b) { return a.name < b.name; });

    irListModel.setFiles(irFiles);
    irList->updateContent();
    irList->repaint();

    // Update status bar
    String statusText = irDirectory.getFullPathName();
    if (currentDirectory != irDirectory)
        statusText += " + " + currentDirectory.getFileName();
    if (irFiles.empty())
        statusText += " - No IR files found";
    else if (irFiles.size() == 1)
        statusText += " - 1 IR file";
    else
        statusText += " - " + String(irFiles.size()) + " IR files";
    statusLabel->setText(statusText, dontSendNotification);

    // Clear details
    updateIRDetailsPanel(nullptr);
}

void NAMModelBrowserComponent::addIRFileInfo(const File& file)
{
    IRFileInfo info;
    info.name = file.getFileNameWithoutExtension().toStdString();
    info.filePath = file.getFullPathName().toStdString();
    info.fileSize = file.getSize();

    // Read audio file metadata
    AudioFormatManager formatManager;
    formatManager.registerBasicFormats();

    std::unique_ptr<AudioFormatReader> reader(formatManager.createReaderFor(file));
    if (reader)
    {
        info.sampleRate = reader->sampleRate;
        info.numChannels = static_cast<int>(reader->numChannels);
        if (reader->sampleRate > 0)
            info.durationSeconds = static_cast<double>(reader->lengthInSamples) / reader->sampleRate;
    }

    irFiles.push_back(std::move(info));
}

void NAMModelBrowserComponent::updateIRDetailsPanel(const IRFileInfo* irInfo)
{
    if (irInfo)
    {
        irNameValue->setText(String(irInfo->name), dontSendNotification);

        // Duration
        if (irInfo->durationSeconds > 0)
        {
            if (irInfo->durationSeconds >= 1.0)
                irDurationValue->setText(String(irInfo->durationSeconds, 3) + " s", dontSendNotification);
            else
                irDurationValue->setText(String(static_cast<int>(irInfo->durationSeconds * 1000)) + " ms", dontSendNotification);
        }
        else
        {
            irDurationValue->setText("-", dontSendNotification);
        }

        // Sample rate
        if (irInfo->sampleRate > 0)
            irSampleRateValue->setText(String(static_cast<int>(irInfo->sampleRate)) + " Hz", dontSendNotification);
        else
            irSampleRateValue->setText("-", dontSendNotification);

        // Channels
        if (irInfo->numChannels > 0)
        {
            String chText = irInfo->numChannels == 1 ? "Mono" : (irInfo->numChannels == 2 ? "Stereo" : String(irInfo->numChannels) + " channels");
            irChannelsValue->setText(chText, dontSendNotification);
        }
        else
        {
            irChannelsValue->setText("-", dontSendNotification);
        }

        // File size
        if (irInfo->fileSize > 0)
        {
            String sizeText;
            if (irInfo->fileSize > 1024 * 1024)
                sizeText = String(irInfo->fileSize / (1024 * 1024)) + " MB";
            else if (irInfo->fileSize > 1024)
                sizeText = String(irInfo->fileSize / 1024) + " KB";
            else
                sizeText = String(irInfo->fileSize) + " bytes";
            irFileSizeValue->setText(sizeText, dontSendNotification);
        }
        else
        {
            irFileSizeValue->setText("-", dontSendNotification);
        }

        // File path
        File irFile(irInfo->filePath);
        irFilePathValue->setText(irFile.getFileName(), dontSendNotification);
        irFilePathValue->setTooltip(String(irInfo->filePath));
    }
    else
    {
        irNameValue->setText("-", dontSendNotification);
        irDurationValue->setText("-", dontSendNotification);
        irSampleRateValue->setText("-", dontSendNotification);
        irChannelsValue->setText("-", dontSendNotification);
        irFileSizeValue->setText("-", dontSendNotification);
        irFilePathValue->setText("-", dontSendNotification);
        irFilePathValue->setTooltip("");
    }
}

void NAMModelBrowserComponent::loadSelectedIR()
{
    auto selectedRow = irList->getSelectedRow();
    if (selectedRow >= 0)
    {
        const auto* irInfo = irListModel.getFileAt(selectedRow);
        if (irInfo && namProcessor)
        {
            File irFile(irInfo->filePath);
            if (namProcessor->loadIR(irFile))
            {
                spdlog::info("[NAMModelBrowser] Loaded IR: {}", irInfo->name);

                if (onModelLoadedCallback)
                    onModelLoadedCallback();
            }
            else
            {
                spdlog::error("[NAMModelBrowser] Failed to load IR: {}", irInfo->name);
            }
        }
    }
}

void NAMModelBrowserComponent::onIRListSelectionChanged()
{
    auto selectedRow = irList->getSelectedRow();
    const auto* irInfo = irListModel.getFileAt(selectedRow);
    updateIRDetailsPanel(irInfo);
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
