/*
  ==============================================================================

    NAMOnlineBrowser.cpp
    Online browser component for TONE3000 NAM models

  ==============================================================================
*/

#include "NAMOnlineBrowser.h"
#include "NAMProcessor.h"
#include "Tone3000Client.h"
#include "Tone3000Auth.h"
#include "ColourScheme.h"

#include <melatonin_blur/melatonin_blur.h>
#include <spdlog/spdlog.h>

//==============================================================================
// Tone3000ResultsListModel
//==============================================================================

Tone3000ResultsListModel::Tone3000ResultsListModel()
{
}

void Tone3000ResultsListModel::setResults(const std::vector<Tone3000::ToneInfo>& results)
{
    tones = results;

    // Check cache status for each tone
    auto& downloadManager = Tone3000DownloadManager::getInstance();
    for (auto& tone : tones)
    {
        if (downloadManager.isCached(juce::String(tone.id)))
        {
            auto cachedFile = downloadManager.getCachedFile(juce::String(tone.id));
            tone.localPath = cachedFile.getFullPathName().toStdString();
        }
    }
}

void Tone3000ResultsListModel::clear()
{
    tones.clear();
    downloadProgress.clear();
}

int Tone3000ResultsListModel::getNumRows()
{
    return static_cast<int>(tones.size());
}

void Tone3000ResultsListModel::paintListBoxItem(int rowNumber, juce::Graphics& g,
                                                  int width, int height, bool rowIsSelected)
{
    if (rowNumber < 0 || rowNumber >= static_cast<int>(tones.size()))
        return;

    auto& colours = ColourScheme::getInstance().colours;
    const auto& tone = tones[rowNumber];

    // Background: selection > hover > alternating
    if (rowIsSelected)
        g.fillAll(colours["Accent Colour"].withAlpha(0.3f));
    else if (rowNumber == hoveredRow)
        g.fillAll(colours["Accent Colour"].withAlpha(0.12f));
    else if (rowNumber % 2 == 1)
        g.fillAll(colours["Dialog Inner Background"].withAlpha(0.5f));

    // Text color
    g.setColour(colours["Text Colour"]);
    g.setFont(juce::Font(13.0f));

    // Draw name
    juce::String displayName = juce::String(tone.name);
    g.drawText(displayName, 8, 2, width - 80, height / 2, juce::Justification::centredLeft, true);

    // Draw author in smaller font
    g.setFont(juce::Font(11.0f));
    g.setColour(colours["Text Colour"].withAlpha(0.7f));
    juce::String authorText = "by " + juce::String(tone.authorName);
    g.drawText(authorText, 8, height / 2, width - 80, height / 2 - 2, juce::Justification::centredLeft, true);

    // Draw status indicator on right side
    juce::Rectangle<int> statusArea(width - 70, 4, 62, height - 8);

    auto progressIt = downloadProgress.find(tone.id);
    float progress = progressIt != downloadProgress.end() ? progressIt->second : -1.0f;

    if (tone.isCached())
    {
        // Cached - show checkmark
        g.setColour(juce::Colours::green);
        g.setFont(juce::Font(11.0f));
        g.drawText("Cached", statusArea, juce::Justification::centred);
    }
    else if (progress >= 0.0f && progress <= 1.0f)
    {
        // Downloading - show progress bar
        g.setColour(colours["Dialog Inner Background"]);
        g.fillRoundedRectangle(statusArea.toFloat(), 3.0f);

        g.setColour(colours["Accent Colour"]);
        auto progressWidth = statusArea.getWidth() * progress;
        g.fillRoundedRectangle(statusArea.getX(), statusArea.getY(),
                               progressWidth, statusArea.getHeight(), 3.0f);

        g.setColour(colours["Text Colour"]);
        g.setFont(juce::Font(10.0f));
        juce::String percentText = juce::String(static_cast<int>(progress * 100)) + "%";
        g.drawText(percentText, statusArea, juce::Justification::centred);
    }
    else if (progress > 1.5f)
    {
        // Complete
        g.setColour(juce::Colours::green);
        g.setFont(juce::Font(11.0f));
        g.drawText("Done", statusArea, juce::Justification::centred);
    }
    else if (progress < -1.5f)
    {
        // Failed
        g.setColour(juce::Colours::red);
        g.setFont(juce::Font(11.0f));
        g.drawText("Failed", statusArea, juce::Justification::centred);
    }
    else
    {
        // Not downloaded - show size
        g.setColour(colours["Text Colour"].withAlpha(0.5f));
        g.setFont(juce::Font(10.0f));
        if (tone.fileSize > 0)
        {
            juce::String sizeText;
            if (tone.fileSize > 1024 * 1024)
                sizeText = juce::String(tone.fileSize / (1024 * 1024)) + " MB";
            else if (tone.fileSize > 1024)
                sizeText = juce::String(tone.fileSize / 1024) + " KB";
            else
                sizeText = juce::String(tone.fileSize) + " B";
            g.drawText(sizeText, statusArea, juce::Justification::centred);
        }
    }

    // Bottom separator
    g.setColour(colours["Text Colour"].withAlpha(0.1f));
    g.drawHorizontalLine(height - 1, 0, width);
}

const Tone3000::ToneInfo* Tone3000ResultsListModel::getToneAt(int index) const
{
    if (index >= 0 && index < static_cast<int>(tones.size()))
        return &tones[index];
    return nullptr;
}

void Tone3000ResultsListModel::setDownloadProgress(const juce::String& toneId, float progress)
{
    downloadProgress[toneId.toStdString()] = progress;
}

void Tone3000ResultsListModel::setDownloadComplete(const juce::String& toneId)
{
    downloadProgress[toneId.toStdString()] = 2.0f;  // > 1 means complete
}

void Tone3000ResultsListModel::setDownloadFailed(const juce::String& toneId)
{
    downloadProgress[toneId.toStdString()] = -2.0f;  // < -1 means failed
}

void Tone3000ResultsListModel::setCached(const juce::String& toneId, const juce::String& localPath)
{
    for (auto& tone : tones)
    {
        if (tone.id == toneId.toStdString())
        {
            tone.localPath = localPath.toStdString();
            break;
        }
    }
    downloadProgress.erase(toneId.toStdString());
}

//==============================================================================
// NAMOnlineBrowserComponent
//==============================================================================

NAMOnlineBrowserComponent::NAMOnlineBrowserComponent(NAMProcessor* processor,
                                                       std::function<void()> onModelLoaded)
    : namProcessor(processor)
    , onModelLoadedCallback(std::move(onModelLoaded))
{
    auto& colours = ColourScheme::getInstance().colours;

    // Search controls
    searchLabel = std::make_unique<juce::Label>("searchLabel", "Search:");
    searchLabel->setColour(juce::Label::textColourId, colours["Text Colour"]);
    addAndMakeVisible(searchLabel.get());

    searchBox = std::make_unique<juce::TextEditor>("searchBox");
    searchBox->setColour(juce::TextEditor::backgroundColourId, colours["Dialog Inner Background"]);
    searchBox->setColour(juce::TextEditor::textColourId, colours["Text Colour"]);
    searchBox->setColour(juce::TextEditor::outlineColourId, colours["Text Colour"].withAlpha(0.3f));
    searchBox->setTextToShowWhenEmpty("Search TONE3000...", colours["Text Colour"].withAlpha(0.5f));
    searchBox->addListener(this);
    addAndMakeVisible(searchBox.get());

    searchButton = std::make_unique<juce::TextButton>("Search");
    searchButton->addListener(this);
    addAndMakeVisible(searchButton.get());

    // Filter controls
    gearTypeLabel = std::make_unique<juce::Label>("gearLabel", "Type:");
    gearTypeLabel->setColour(juce::Label::textColourId, colours["Text Colour"]);
    addAndMakeVisible(gearTypeLabel.get());

    gearTypeCombo = std::make_unique<juce::ComboBox>("gearType");
    gearTypeCombo->addItem("All", 1);
    gearTypeCombo->addItem("Amp", 2);
    gearTypeCombo->addItem("Pedal", 3);
    gearTypeCombo->addItem("Full Rig", 4);
    gearTypeCombo->setSelectedId(1);
    gearTypeCombo->addListener(this);
    addAndMakeVisible(gearTypeCombo.get());

    sortLabel = std::make_unique<juce::Label>("sortLabel", "Sort:");
    sortLabel->setColour(juce::Label::textColourId, colours["Text Colour"]);
    addAndMakeVisible(sortLabel.get());

    sortCombo = std::make_unique<juce::ComboBox>("sort");
    sortCombo->addItem("Trending", 1);
    sortCombo->addItem("Newest", 2);
    sortCombo->addItem("Most Downloaded", 3);
    sortCombo->addItem("Name A-Z", 4);
    sortCombo->setSelectedId(1);
    sortCombo->addListener(this);
    addAndMakeVisible(sortCombo.get());

    // Results list - transparent background for custom rounded painting
    resultsList = std::make_unique<juce::ListBox>("results", &listModel);
    resultsList->setRowHeight(40);
    resultsList->setColour(juce::ListBox::backgroundColourId, juce::Colours::transparentBlack);
    resultsList->setColour(juce::ListBox::outlineColourId, juce::Colours::transparentBlack);
    resultsList->setOutlineThickness(0);
    resultsList->addMouseListener(this, true);
    addAndMakeVisible(resultsList.get());

    // Details panel
    detailsTitle = std::make_unique<juce::Label>("detailsTitle", "Details");
    detailsTitle->setFont(juce::Font(14.0f, juce::Font::bold));
    detailsTitle->setColour(juce::Label::textColourId, colours["Text Colour"]);
    addAndMakeVisible(detailsTitle.get());

    auto createDetailLabel = [&colours](const juce::String& text) {
        auto label = std::make_unique<juce::Label>();
        label->setText(text, juce::dontSendNotification);
        label->setFont(juce::Font(12.0f));
        label->setColour(juce::Label::textColourId, colours["Text Colour"].withAlpha(0.7f));
        return label;
    };

    auto createValueLabel = [&colours]() {
        auto label = std::make_unique<juce::Label>();
        label->setFont(juce::Font(12.0f));
        label->setColour(juce::Label::textColourId, colours["Text Colour"]);
        return label;
    };

    nameLabel = createDetailLabel("Name:");
    addAndMakeVisible(nameLabel.get());
    nameValue = createValueLabel();
    addAndMakeVisible(nameValue.get());

    authorLabel = createDetailLabel("Author:");
    addAndMakeVisible(authorLabel.get());
    authorValue = createValueLabel();
    addAndMakeVisible(authorValue.get());

    architectureLabel = createDetailLabel("Architecture:");
    addAndMakeVisible(architectureLabel.get());
    architectureValue = createValueLabel();
    addAndMakeVisible(architectureValue.get());

    downloadsLabel = createDetailLabel("Downloads:");
    addAndMakeVisible(downloadsLabel.get());
    downloadsValue = createValueLabel();
    addAndMakeVisible(downloadsValue.get());

    sizeLabel = createDetailLabel("Size:");
    addAndMakeVisible(sizeLabel.get());
    sizeValue = createValueLabel();
    addAndMakeVisible(sizeValue.get());

    gearLabel = createDetailLabel("Type:");
    addAndMakeVisible(gearLabel.get());
    gearValue = createValueLabel();
    addAndMakeVisible(gearValue.get());

    // Action buttons
    downloadButton = std::make_unique<juce::TextButton>("Download");
    downloadButton->addListener(this);
    downloadButton->setEnabled(false);
    addAndMakeVisible(downloadButton.get());

    loadButton = std::make_unique<juce::TextButton>("Load");
    loadButton->addListener(this);
    loadButton->setEnabled(false);
    addAndMakeVisible(loadButton.get());

    // Status bar
    statusLabel = std::make_unique<juce::Label>("status", "Not logged in");
    statusLabel->setFont(juce::Font(11.0f));
    statusLabel->setColour(juce::Label::textColourId, colours["Text Colour"].withAlpha(0.7f));
    addAndMakeVisible(statusLabel.get());

    loginButton = std::make_unique<juce::TextButton>("Login");
    loginButton->addListener(this);
    addAndMakeVisible(loginButton.get());

    logoutButton = std::make_unique<juce::TextButton>("Logout");
    logoutButton->addListener(this);
    logoutButton->setVisible(false);
    addAndMakeVisible(logoutButton.get());

    prevPageButton = std::make_unique<juce::TextButton>("<");
    prevPageButton->addListener(this);
    prevPageButton->setEnabled(false);
    addAndMakeVisible(prevPageButton.get());

    nextPageButton = std::make_unique<juce::TextButton>(">");
    nextPageButton->addListener(this);
    nextPageButton->setEnabled(false);
    addAndMakeVisible(nextPageButton.get());

    pageLabel = std::make_unique<juce::Label>("page", "");
    pageLabel->setFont(juce::Font(11.0f));
    pageLabel->setColour(juce::Label::textColourId, colours["Text Colour"]);
    pageLabel->setJustificationType(juce::Justification::centred);
    addAndMakeVisible(pageLabel.get());

    // Register as download listener
    Tone3000DownloadManager::getInstance().addListener(this);

    // Update auth state
    refreshAuthState();

    // Set up list selection callback
    resultsList->setMouseClickGrabsKeyboardFocus(true);

    spdlog::info("[NAMOnlineBrowser] Component initialized");
}

NAMOnlineBrowserComponent::~NAMOnlineBrowserComponent()
{
    Tone3000DownloadManager::getInstance().removeListener(this);
}

void NAMOnlineBrowserComponent::paint(juce::Graphics& g)
{
    auto& colours = ColourScheme::getInstance().colours;
    auto bgColour = colours["Window Background"];

    // Gradient background
    juce::ColourGradient bgGradient(bgColour.brighter(0.06f), 0, 0,
                                     bgColour.darker(0.06f), 0, static_cast<float>(getHeight()), false);
    g.setGradientFill(bgGradient);
    g.fillAll();

    // Calculate panel areas
    auto bounds = getLocalBounds().reduced(8);
    bounds.removeFromTop(70);  // Search + filters
    bounds.removeFromBottom(32);  // Status bar

    int listWidth = static_cast<int>(bounds.getWidth() * 0.55f);
    auto listArea = bounds.removeFromLeft(listWidth);
    bounds.removeFromLeft(16);  // Gap

    // Draw rounded list background
    auto listBounds = listArea.toFloat();
    g.setColour(colours["Dialog Inner Background"]);
    g.fillRoundedRectangle(listBounds, 8.0f);
    g.setColour(colours["Text Colour"].withAlpha(0.2f));
    g.drawRoundedRectangle(listBounds.reduced(0.5f), 8.0f, 1.0f);

    // Draw card-style details panel with shadow
    auto detailsBounds = bounds.toFloat();
    juce::Path detailsPath;
    detailsPath.addRoundedRectangle(detailsBounds, 8.0f);

    // Drop shadow
    melatonin::DropShadow shadow(juce::Colours::black.withAlpha(0.2f), 8, {2, 2});
    shadow.render(g, detailsPath);

    // Card fill with subtle gradient
    juce::ColourGradient cardGrad(colours["Dialog Inner Background"].brighter(0.04f),
                                   detailsBounds.getX(), detailsBounds.getY(),
                                   colours["Dialog Inner Background"].darker(0.04f),
                                   detailsBounds.getX(), detailsBounds.getBottom(), false);
    g.setGradientFill(cardGrad);
    g.fillPath(detailsPath);

    // Card border
    g.setColour(colours["Text Colour"].withAlpha(0.15f));
    g.strokePath(detailsPath, juce::PathStrokeType(1.0f));
}

void NAMOnlineBrowserComponent::resized()
{
    auto bounds = getLocalBounds().reduced(8);

    // Search row
    auto searchRow = bounds.removeFromTop(28);
    searchLabel->setBounds(searchRow.removeFromLeft(50));
    searchButton->setBounds(searchRow.removeFromRight(70));
    searchRow.removeFromRight(8);
    searchBox->setBounds(searchRow);

    bounds.removeFromTop(8);

    // Filter row
    auto filterRow = bounds.removeFromTop(28);
    gearTypeLabel->setBounds(filterRow.removeFromLeft(40));
    gearTypeCombo->setBounds(filterRow.removeFromLeft(90));
    filterRow.removeFromLeft(16);
    sortLabel->setBounds(filterRow.removeFromLeft(35));
    sortCombo->setBounds(filterRow.removeFromLeft(120));

    bounds.removeFromTop(8);

    // Status bar at bottom
    auto statusRow = bounds.removeFromBottom(28);
    statusLabel->setBounds(statusRow.removeFromLeft(200));
    loginButton->setBounds(statusRow.removeFromLeft(60));
    logoutButton->setBounds(statusRow.removeFromLeft(60));

    nextPageButton->setBounds(statusRow.removeFromRight(30));
    statusRow.removeFromRight(4);
    pageLabel->setBounds(statusRow.removeFromRight(60));
    statusRow.removeFromRight(4);
    prevPageButton->setBounds(statusRow.removeFromRight(30));

    bounds.removeFromBottom(4);

    // Split remaining area between list and details
    int listWidth = bounds.getWidth() * 0.55f;
    auto listArea = bounds.removeFromLeft(listWidth);
    resultsList->setBounds(listArea);

    bounds.removeFromLeft(16);  // Gap

    // Details panel
    auto detailsArea = bounds;
    detailsTitle->setBounds(detailsArea.removeFromTop(24));
    detailsArea.removeFromTop(8);

    int labelWidth = 80;
    int rowHeight = 20;

    auto detailRow = [&]() {
        auto row = detailsArea.removeFromTop(rowHeight);
        detailsArea.removeFromTop(4);
        return row;
    };

    auto row = detailRow();
    nameLabel->setBounds(row.removeFromLeft(labelWidth));
    nameValue->setBounds(row);

    row = detailRow();
    authorLabel->setBounds(row.removeFromLeft(labelWidth));
    authorValue->setBounds(row);

    row = detailRow();
    gearLabel->setBounds(row.removeFromLeft(labelWidth));
    gearValue->setBounds(row);

    row = detailRow();
    architectureLabel->setBounds(row.removeFromLeft(labelWidth));
    architectureValue->setBounds(row);

    row = detailRow();
    downloadsLabel->setBounds(row.removeFromLeft(labelWidth));
    downloadsValue->setBounds(row);

    row = detailRow();
    sizeLabel->setBounds(row.removeFromLeft(labelWidth));
    sizeValue->setBounds(row);

    detailsArea.removeFromTop(12);

    // Action buttons
    auto buttonRow = detailsArea.removeFromTop(28);
    downloadButton->setBounds(buttonRow.removeFromLeft(90));
    buttonRow.removeFromLeft(8);
    loadButton->setBounds(buttonRow.removeFromLeft(70));
}

void NAMOnlineBrowserComponent::buttonClicked(juce::Button* button)
{
    if (button == searchButton.get())
    {
        currentPage = 1;
        performSearch();
    }
    else if (button == loginButton.get())
    {
        showLoginDialog();
    }
    else if (button == logoutButton.get())
    {
        logout();
    }
    else if (button == downloadButton.get())
    {
        downloadSelectedModel();
    }
    else if (button == loadButton.get())
    {
        if (selectedTone && selectedTone->isCached())
        {
            loadCachedModel(juce::String(selectedTone->id));
        }
    }
    else if (button == prevPageButton.get())
    {
        if (currentPage > 1)
            goToPage(currentPage - 1);
    }
    else if (button == nextPageButton.get())
    {
        if (hasMorePages)
            goToPage(currentPage + 1);
    }
}

void NAMOnlineBrowserComponent::textEditorReturnKeyPressed(juce::TextEditor& editor)
{
    if (&editor == searchBox.get())
    {
        currentPage = 1;
        performSearch();
    }
}

void NAMOnlineBrowserComponent::mouseUp(const juce::MouseEvent& event)
{
    // Check if click was on the results list
    if (resultsList != nullptr && resultsList->isParentOf(event.eventComponent))
    {
        // Defer the selection check to allow JUCE to update the selection first
        juce::MessageManager::callAsync([this]() {
            onListSelectionChanged();
        });
    }
}

void NAMOnlineBrowserComponent::mouseMove(const juce::MouseEvent& event)
{
    if (resultsList != nullptr && resultsList->isParentOf(event.eventComponent))
    {
        auto localPoint = resultsList->getLocalPoint(event.eventComponent, event.position);
        int row = resultsList->getRowContainingPosition(static_cast<int>(localPoint.x),
                                                         static_cast<int>(localPoint.y));
        if (row != listModel.getHoveredRow())
        {
            listModel.setHoveredRow(row);
            resultsList->repaint();
        }
    }
}

void NAMOnlineBrowserComponent::mouseExit(const juce::MouseEvent& /*event*/)
{
    if (listModel.getHoveredRow() != -1)
    {
        listModel.setHoveredRow(-1);
        resultsList->repaint();
    }
}

void NAMOnlineBrowserComponent::comboBoxChanged(juce::ComboBox* comboBox)
{
    if (comboBox == gearTypeCombo.get())
    {
        int id = gearTypeCombo->getSelectedId();
        switch (id)
        {
            case 1: currentGearType = Tone3000::GearType::All; break;
            case 2: currentGearType = Tone3000::GearType::Amp; break;
            case 3: currentGearType = Tone3000::GearType::Pedal; break;
            case 4: currentGearType = Tone3000::GearType::FullRig; break;
            default: currentGearType = Tone3000::GearType::All; break;
        }
    }
    else if (comboBox == sortCombo.get())
    {
        int id = sortCombo->getSelectedId();
        switch (id)
        {
            case 1: currentSortOrder = Tone3000::SortOrder::Trending; break;
            case 2: currentSortOrder = Tone3000::SortOrder::Newest; break;
            case 3: currentSortOrder = Tone3000::SortOrder::DownloadsAllTime; break;
            case 4: currentSortOrder = Tone3000::SortOrder::BestMatch; break;
            default: currentSortOrder = Tone3000::SortOrder::Trending; break;
        }
    }

    // Re-search with new filters if we have a query
    if (currentQuery.isNotEmpty())
    {
        currentPage = 1;
        performSearch();
    }
}

void NAMOnlineBrowserComponent::performSearch()
{
    currentQuery = searchBox->getText().trim();

    if (isSearching)
    {
        spdlog::debug("[NAMOnlineBrowser] Search already in progress");
        return;
    }

    isSearching = true;
    searchButton->setEnabled(false);
    statusLabel->setText("Searching...", juce::dontSendNotification);

    spdlog::info("[NAMOnlineBrowser] Searching: '{}', page {}", currentQuery.toStdString(), currentPage);

    Tone3000Client::getInstance().search(
        currentQuery, currentGearType, currentSortOrder, currentPage,
        [this](Tone3000::SearchResult result, Tone3000::ApiError error) {
            juce::MessageManager::callAsync([this, result, error]() {
                isSearching = false;
                searchButton->setEnabled(true);

                if (error.isError())
                {
                    spdlog::error("[NAMOnlineBrowser] Search failed: {}", error.message);
                    statusLabel->setText("Search failed: " + juce::String(error.message),
                                         juce::dontSendNotification);
                    return;
                }

                listModel.setResults(result.tones);
                resultsList->updateContent();
                resultsList->deselectAllRows();

                totalResults = result.totalCount;
                hasMorePages = result.hasMore();
                currentPage = result.page;

                updateStatusLabel();
                updateDetailsPanel(nullptr);

                prevPageButton->setEnabled(currentPage > 1);
                nextPageButton->setEnabled(hasMorePages);

                juce::String pageText = "Page " + juce::String(currentPage);
                if (totalResults > 0)
                    pageText += " (" + juce::String(totalResults) + " results)";
                pageLabel->setText(pageText, juce::dontSendNotification);

                spdlog::info("[NAMOnlineBrowser] Found {} results", result.tones.size());
            });
        });
}

void NAMOnlineBrowserComponent::updateDetailsPanel(const Tone3000::ToneInfo* tone)
{
    selectedTone = tone;

    if (tone == nullptr)
    {
        nameValue->setText("", juce::dontSendNotification);
        authorValue->setText("", juce::dontSendNotification);
        architectureValue->setText("", juce::dontSendNotification);
        downloadsValue->setText("", juce::dontSendNotification);
        sizeValue->setText("", juce::dontSendNotification);
        gearValue->setText("", juce::dontSendNotification);

        downloadButton->setEnabled(false);
        loadButton->setEnabled(false);
        return;
    }

    nameValue->setText(juce::String(tone->name), juce::dontSendNotification);
    authorValue->setText(juce::String(tone->authorName), juce::dontSendNotification);
    architectureValue->setText(juce::String(tone->architecture), juce::dontSendNotification);
    downloadsValue->setText(juce::String(tone->downloads), juce::dontSendNotification);
    gearValue->setText(juce::String(tone->gearType), juce::dontSendNotification);

    if (tone->fileSize > 0)
    {
        juce::String sizeText;
        if (tone->fileSize > 1024 * 1024)
            sizeText = juce::String::formatted("%.1f MB", tone->fileSize / (1024.0 * 1024.0));
        else if (tone->fileSize > 1024)
            sizeText = juce::String::formatted("%.1f KB", tone->fileSize / 1024.0);
        else
            sizeText = juce::String(tone->fileSize) + " bytes";
        sizeValue->setText(sizeText, juce::dontSendNotification);
    }
    else
    {
        sizeValue->setText("Unknown", juce::dontSendNotification);
    }

    // Update button states
    bool isCached = tone->isCached();
    bool isDownloading = Tone3000DownloadManager::getInstance().isDownloading(juce::String(tone->id));

    downloadButton->setEnabled(!isCached && !isDownloading && Tone3000Client::getInstance().isAuthenticated());
    downloadButton->setButtonText(isDownloading ? "Downloading..." : "Download");
    loadButton->setEnabled(isCached);
}

void NAMOnlineBrowserComponent::onListSelectionChanged()
{
    int selectedRow = resultsList->getSelectedRow();
    const Tone3000::ToneInfo* tone = listModel.getToneAt(selectedRow);
    updateDetailsPanel(tone);
}

void NAMOnlineBrowserComponent::downloadSelectedModel()
{
    if (selectedTone == nullptr)
        return;

    if (!Tone3000Client::getInstance().isAuthenticated())
    {
        showLoginDialog();
        return;
    }

    spdlog::info("[NAMOnlineBrowser] Queueing download: {}", selectedTone->name);
    Tone3000DownloadManager::getInstance().queueDownload(*selectedTone);

    downloadButton->setEnabled(false);
    downloadButton->setButtonText("Downloading...");
}

void NAMOnlineBrowserComponent::loadCachedModel(const juce::String& toneId)
{
    auto cachedFile = Tone3000DownloadManager::getInstance().getCachedFile(toneId);

    if (!cachedFile.existsAsFile())
    {
        spdlog::error("[NAMOnlineBrowser] Cached file not found for {}", toneId.toStdString());
        return;
    }

    if (namProcessor != nullptr)
    {
        spdlog::info("[NAMOnlineBrowser] Loading model: {}", cachedFile.getFullPathName().toStdString());
        namProcessor->loadModel(cachedFile);

        if (onModelLoadedCallback)
            onModelLoadedCallback();
    }
}

void NAMOnlineBrowserComponent::updateStatusLabel()
{
    if (Tone3000Client::getInstance().isAuthenticated())
    {
        statusLabel->setText("Logged in", juce::dontSendNotification);
    }
    else
    {
        statusLabel->setText("Not logged in", juce::dontSendNotification);
    }
}

void NAMOnlineBrowserComponent::showLoginDialog()
{
    spdlog::info("[NAMOnlineBrowser] Starting authentication flow");

    // Create auth handler and start OAuth flow
    auto* auth = new Tone3000Auth();

    auth->startAuthentication([this, auth](bool success, juce::String errorMessage) {
        // Clean up auth object
        delete auth;

        if (success)
        {
            spdlog::info("[NAMOnlineBrowser] Authentication successful");
            juce::MessageManager::callAsync([this]() {
                refreshAuthState();
                if (selectedTone != nullptr && !selectedTone->isCached())
                    downloadButton->setEnabled(true);
            });
        }
        else
        {
            spdlog::warn("[NAMOnlineBrowser] OAuth failed ({}), showing manual dialog", errorMessage.toStdString());

            // Fall back to manual dialog
            juce::MessageManager::callAsync([this]() {
                auto* manualDialog = new Tone3000ManualAuthDialog([this](bool manualSuccess) {
                    juce::MessageManager::callAsync([this, manualSuccess]() {
                        refreshAuthState();
                        if (manualSuccess && selectedTone != nullptr && !selectedTone->isCached())
                            downloadButton->setEnabled(true);
                    });
                });

                juce::DialogWindow::LaunchOptions options;
                options.content.setOwned(manualDialog);
                options.dialogTitle = "TONE3000 Login";
                options.dialogBackgroundColour = ColourScheme::getInstance().colours["Window Background"];
                options.escapeKeyTriggersCloseButton = true;
                options.useNativeTitleBar = true;
                options.resizable = false;
                options.launchAsync();
            });
        }
    });
}

void NAMOnlineBrowserComponent::logout()
{
    spdlog::info("[NAMOnlineBrowser] Logging out");
    Tone3000Client::getInstance().logout();
    refreshAuthState();
    downloadButton->setEnabled(false);
}

void NAMOnlineBrowserComponent::goToPage(int page)
{
    currentPage = page;
    performSearch();
}

void NAMOnlineBrowserComponent::refreshAuthState()
{
    bool authenticated = Tone3000Client::getInstance().isAuthenticated();

    loginButton->setVisible(!authenticated);
    logoutButton->setVisible(authenticated);

    updateStatusLabel();
}

// Download listener callbacks
void NAMOnlineBrowserComponent::downloadQueued(const juce::String& toneId)
{
    listModel.setDownloadProgress(toneId, 0.0f);
    resultsList->repaint();
}

void NAMOnlineBrowserComponent::downloadStarted(const juce::String& toneId)
{
    listModel.setDownloadProgress(toneId, 0.0f);
    resultsList->repaint();
}

void NAMOnlineBrowserComponent::downloadProgress(const juce::String& toneId, float progress,
                                                   int64_t /*bytesDownloaded*/, int64_t /*totalBytes*/)
{
    listModel.setDownloadProgress(toneId, progress);
    resultsList->repaint();

    // Update button if this is the selected model
    if (selectedTone && selectedTone->id == toneId.toStdString())
    {
        downloadButton->setButtonText(juce::String::formatted("Downloading %.0f%%", progress * 100));
    }
}

void NAMOnlineBrowserComponent::downloadCompleted(const juce::String& toneId, const juce::File& file)
{
    listModel.setCached(toneId, file.getFullPathName());
    resultsList->repaint();

    // Update details panel if this is the selected model
    if (selectedTone && selectedTone->id == toneId.toStdString())
    {
        downloadButton->setButtonText("Download");
        downloadButton->setEnabled(false);
        loadButton->setEnabled(true);
    }

    spdlog::info("[NAMOnlineBrowser] Download completed: {}", toneId.toStdString());
}

void NAMOnlineBrowserComponent::downloadFailed(const juce::String& toneId, const juce::String& errorMessage)
{
    listModel.setDownloadFailed(toneId);
    resultsList->repaint();

    if (selectedTone && selectedTone->id == toneId.toStdString())
    {
        downloadButton->setButtonText("Download");
        downloadButton->setEnabled(true);
    }

    spdlog::error("[NAMOnlineBrowser] Download failed: {} - {}", toneId.toStdString(), errorMessage.toStdString());
}

void NAMOnlineBrowserComponent::downloadCancelled(const juce::String& toneId)
{
    listModel.setDownloadProgress(toneId, -1.0f);  // Reset state
    resultsList->repaint();

    if (selectedTone && selectedTone->id == toneId.toStdString())
    {
        downloadButton->setButtonText("Download");
        downloadButton->setEnabled(true);
    }
}

