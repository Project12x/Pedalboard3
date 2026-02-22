/*
  ==============================================================================

    NAMOnlineBrowser.cpp
    Online browser component for TONE3000 NAM models

  ==============================================================================
*/

#include "NAMOnlineBrowser.h"

#include "ColourScheme.h"
#include "FontManager.h"
#include "NAMProcessor.h"
#include "Tone3000Auth.h"
#include "Tone3000Client.h"

#include <melatonin_blur/melatonin_blur.h>
#include <spdlog/spdlog.h>

//==============================================================================
// Tone3000ResultsListModel
//==============================================================================

Tone3000ResultsListModel::Tone3000ResultsListModel() {}

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

void Tone3000ResultsListModel::paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height,
                                                bool rowIsSelected)
{
    if (rowNumber < 0 || rowNumber >= static_cast<int>(tones.size()))
        return;

    auto& colours = ColourScheme::getInstance().colours;
    const auto& tone = tones[rowNumber];

    const int margin = 6;
    const float cornerRadius = 6.0f;
    juce::Rectangle<float> itemBounds(static_cast<float>(margin), 2.0f, static_cast<float>(width - margin * 2),
                                      static_cast<float>(height - 4));

    // Background: selection > hover > none
    if (rowIsSelected)
    {
        g.setColour(colours["Accent Colour"].withAlpha(0.18f));
        g.fillRoundedRectangle(itemBounds, cornerRadius);
        g.setColour(colours["Accent Colour"].withAlpha(0.5f));
        g.drawRoundedRectangle(itemBounds, cornerRadius, 1.0f);

        // Left-edge accent stripe (DAW-style selection indicator)
        juce::Rectangle<float> stripe(itemBounds.getX(), itemBounds.getY() + 2.0f, 3.0f, itemBounds.getHeight() - 4.0f);
        g.setColour(colours["Accent Colour"]);
        g.fillRoundedRectangle(stripe, 1.5f);
    }
    else if (rowNumber == hoveredRow)
    {
        g.setColour(colours["Text Colour"].withAlpha(0.05f));
        g.fillRoundedRectangle(itemBounds, cornerRadius);
        g.setColour(colours["Accent Colour"].withAlpha(0.2f));
        g.drawRoundedRectangle(itemBounds, cornerRadius, 1.0f);
    }

    const int textX = margin + 10;

    // Badge layout - rightmost side
    const int badgeHeight = 16;
    int badgeX = width - margin - 6;

    // Gear type badge with bright fixed colors visible on dark backgrounds
    juce::String gearText;
    juce::Colour badgeCol;
    if (tone.gearType == "amp")
    {
        gearText = "Amp";
        badgeCol = juce::Colour(0xFFE8A838); // warm orange-gold
    }
    else if (tone.gearType == "pedal")
    {
        gearText = "Pedal";
        badgeCol = juce::Colour(0xFF38C8E8); // bright cyan
    }
    else if (tone.gearType == "full_rig")
    {
        gearText = "Full Rig";
        badgeCol = juce::Colour(0xFF58D868); // bright green
    }
    else
    {
        gearText = juce::String(tone.gearType);
        badgeCol = juce::Colour(0xFFB088E8); // lavender
    }

    if (gearText.isNotEmpty())
    {
        auto& fm = FontManager::getInstance();
        g.setFont(fm.getBadgeFont());
        int badgeW = static_cast<int>(fm.getBadgeFont().getStringWidthFloat(gearText)) + 12;
        badgeX -= badgeW;

        juce::Rectangle<float> badgeBounds(static_cast<float>(badgeX), (height - badgeHeight) / 2.0f,
                                           static_cast<float>(badgeW), static_cast<float>(badgeHeight));
        g.setColour(badgeCol.withAlpha(0.15f));
        g.fillRoundedRectangle(badgeBounds, badgeHeight / 2.0f);
        g.setColour(badgeCol.withAlpha(0.6f));
        g.drawRoundedRectangle(badgeBounds, badgeHeight / 2.0f, 1.0f);
        g.setColour(badgeCol.withAlpha(0.8f));
        g.drawText(gearText, badgeBounds, juce::Justification::centred, true);

        badgeX -= 4; // spacing
    }

    // Status indicator on right side
    juce::Rectangle<int> statusArea(badgeX - 55, 4, 50, height - 8);

    auto progressIt = downloadProgress.find(tone.id);
    float progress = progressIt != downloadProgress.end() ? progressIt->second : -1.0f;

    if (tone.isCached())
    {
        g.setColour(ColourScheme::getInstance().colours["Success Colour"]);
        g.setFont(FontManager::getInstance().getCaptionFont());
        g.drawText("Cached", statusArea, juce::Justification::centredRight);
    }
    else if (progress >= 0.0f && progress <= 1.0f)
    {
        auto barBounds = statusArea.toFloat();
        g.setColour(colours["Dialog Inner Background"]);
        g.fillRoundedRectangle(barBounds, 3.0f);
        g.setColour(colours["Accent Colour"]);
        g.fillRoundedRectangle(barBounds.getX(), barBounds.getY(), barBounds.getWidth() * progress,
                               barBounds.getHeight(), 3.0f);
        g.setColour(colours["Text Colour"]);
        g.setFont(FontManager::getInstance().getMonoFont(9.0f));
        g.drawText(juce::String(static_cast<int>(progress * 100)) + "%", statusArea, juce::Justification::centred);
    }
    else if (progress > 1.5f)
    {
        g.setColour(ColourScheme::getInstance().colours["Success Colour"]);
        g.setFont(FontManager::getInstance().getCaptionFont());
        g.drawText("Done", statusArea, juce::Justification::centredRight);
    }
    else if (progress < -1.5f)
    {
        g.setColour(ColourScheme::getInstance().colours["Danger Colour"]);
        g.setFont(FontManager::getInstance().getCaptionFont());
        g.drawText("Failed", statusArea, juce::Justification::centredRight);
    }
    else if (tone.fileSize > 0)
    {
        g.setColour(colours["Text Colour"].withAlpha(0.4f));
        g.setFont(FontManager::getInstance().getMonoFont(9.0f));
        juce::String sizeText;
        if (tone.fileSize > 1024 * 1024)
            sizeText = juce::String(tone.fileSize / (1024 * 1024)) + " MB";
        else if (tone.fileSize > 1024)
            sizeText = juce::String(tone.fileSize / 1024) + " KB";
        else
            sizeText = juce::String(tone.fileSize) + " B";
        g.drawText(sizeText, statusArea, juce::Justification::centredRight);
    }

    // Name (primary text)
    int textRight = statusArea.getX() - 4;
    g.setColour(colours["Text Colour"]);
    g.setFont(FontManager::getInstance().getBodyBoldFont());
    g.drawText(juce::String(tone.name), textX, 2, textRight - textX, height / 2, juce::Justification::centredLeft,
               true);

    // Author (secondary text)
    g.setFont(FontManager::getInstance().getCaptionFont());
    g.setColour(colours["Text Colour"].withAlpha(0.5f));
    g.drawText("by " + juce::String(tone.authorName), textX, height / 2, textRight - textX, height / 2 - 2,
               juce::Justification::centredLeft, true);
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
    downloadProgress[toneId.toStdString()] = 2.0f; // > 1 means complete
}

void Tone3000ResultsListModel::setDownloadFailed(const juce::String& toneId)
{
    downloadProgress[toneId.toStdString()] = -2.0f; // < -1 means failed
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

NAMOnlineBrowserComponent::NAMOnlineBrowserComponent(NAMProcessor* processor, std::function<void()> onModelLoaded)
    : namProcessor(processor), onModelLoadedCallback(std::move(onModelLoaded))
{
    auto& colours = ColourScheme::getInstance().colours;

    // Search controls
    searchBox = std::make_unique<juce::TextEditor>("searchBox");
    searchBox->setColour(juce::TextEditor::backgroundColourId, juce::Colours::transparentBlack);
    searchBox->setColour(juce::TextEditor::textColourId, colours["Text Colour"]);
    searchBox->setColour(juce::TextEditor::outlineColourId, juce::Colours::transparentBlack);
    searchBox->setColour(juce::TextEditor::focusedOutlineColourId, juce::Colours::transparentBlack);
    searchBox->setTextToShowWhenEmpty("Search TONE3000...", colours["Text Colour"].withAlpha(0.5f));
    searchBox->setFont(FontManager::getInstance().getBodyFont());
    searchBox->setIndents(28, 0); // Left indent for magnifying glass icon
    searchBox->addListener(this);
    addAndMakeVisible(searchBox.get());

    searchButton = std::make_unique<juce::TextButton>("Search");
    searchButton->addListener(this);
    searchButton->setColour(juce::TextButton::buttonColourId, colours["Accent Colour"]);
    searchButton->setColour(juce::TextButton::buttonOnColourId, colours["Accent Colour"].brighter(0.15f));
    searchButton->setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    searchButton->setColour(juce::TextButton::textColourOnId, juce::Colours::white);
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
    gearTypeCombo->setColour(juce::ComboBox::backgroundColourId, colours["Dialog Inner Background"]);
    gearTypeCombo->setColour(juce::ComboBox::textColourId, colours["Text Colour"]);
    gearTypeCombo->setColour(juce::ComboBox::outlineColourId, colours["Text Colour"].withAlpha(0.2f));
    gearTypeCombo->setColour(juce::ComboBox::arrowColourId, colours["Text Colour"].withAlpha(0.6f));
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
    sortCombo->setColour(juce::ComboBox::backgroundColourId, colours["Dialog Inner Background"]);
    sortCombo->setColour(juce::ComboBox::textColourId, colours["Text Colour"]);
    sortCombo->setColour(juce::ComboBox::outlineColourId, colours["Text Colour"].withAlpha(0.2f));
    sortCombo->setColour(juce::ComboBox::arrowColourId, colours["Text Colour"].withAlpha(0.6f));
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
    detailsTitle->setFont(FontManager::getInstance().getSubheadingFont());
    detailsTitle->setColour(juce::Label::textColourId, colours["Text Colour"]);
    addAndMakeVisible(detailsTitle.get());

    auto createDetailLabel = [&colours](const juce::String& text)
    {
        auto label = std::make_unique<juce::Label>();
        label->setText(text, juce::dontSendNotification);
        label->setFont(FontManager::getInstance().getLabelFont());
        label->setColour(juce::Label::textColourId, colours["Text Colour"].withAlpha(0.7f));
        return label;
    };

    auto createValueLabel = [&colours]()
    {
        auto label = std::make_unique<juce::Label>();
        label->setFont(FontManager::getInstance().getLabelFont());
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
    downloadButton->setColour(juce::TextButton::buttonColourId, colours["Accent Colour"]);
    downloadButton->setColour(juce::TextButton::buttonOnColourId, colours["Accent Colour"].brighter(0.15f));
    downloadButton->setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    downloadButton->setColour(juce::TextButton::textColourOnId, juce::Colours::white);
    addAndMakeVisible(downloadButton.get());

    loadButton = std::make_unique<juce::TextButton>("Load");
    loadButton->addListener(this);
    loadButton->setEnabled(false);
    loadButton->setColour(juce::TextButton::buttonColourId, colours["Slider Colour"]);
    loadButton->setColour(juce::TextButton::buttonOnColourId, colours["Slider Colour"].brighter(0.2f));
    loadButton->setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    loadButton->setColour(juce::TextButton::textColourOnId, juce::Colours::white);
    addAndMakeVisible(loadButton.get());

    // Status bar
    statusLabel = std::make_unique<juce::Label>("status", "Not logged in");
    statusLabel->setFont(FontManager::getInstance().getCaptionFont());
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
    prevPageButton->setColour(juce::TextButton::buttonColourId, colours["Button Colour"]);
    prevPageButton->setColour(juce::TextButton::textColourOffId, colours["Text Colour"]);
    addAndMakeVisible(prevPageButton.get());

    nextPageButton = std::make_unique<juce::TextButton>(">");
    nextPageButton->addListener(this);
    nextPageButton->setEnabled(false);
    nextPageButton->setColour(juce::TextButton::buttonColourId, colours["Button Colour"]);
    nextPageButton->setColour(juce::TextButton::textColourOffId, colours["Text Colour"]);
    addAndMakeVisible(nextPageButton.get());

    pageLabel = std::make_unique<juce::Label>("page", "");
    pageLabel->setFont(FontManager::getInstance().getCaptionFont());
    pageLabel->setColour(juce::Label::textColourId, colours["Text Colour"]);
    pageLabel->setJustificationType(juce::Justification::centred);
    addAndMakeVisible(pageLabel.get());

    // Register as download listener
    Tone3000DownloadManager::getInstance().addListener(this);

    // Update auth state
    refreshAuthState();

    // Set up list selection callback
    resultsList->setMouseClickGrabsKeyboardFocus(true);

    spdlog::info("[NAMOnlineBrowser] Component initialized, this={}", (void*)this);
}

NAMOnlineBrowserComponent::~NAMOnlineBrowserComponent()
{
    spdlog::info("[NAMOnlineBrowser] Component destructor called, this={}", (void*)this);
    Tone3000DownloadManager::getInstance().removeListener(this);
    spdlog::debug("[NAMOnlineBrowser] Removed download listener");
}

void NAMOnlineBrowserComponent::paint(juce::Graphics& g)
{
    auto& colours = ColourScheme::getInstance().colours;
    auto bgColour = colours["Window Background"];

    // Gradient background
    juce::ColourGradient bgGradient(bgColour.brighter(0.06f), 0, 0, bgColour.darker(0.06f), 0,
                                    static_cast<float>(getHeight()), false);
    g.setGradientFill(bgGradient);
    g.fillAll();

    // Subtle dot-grid pattern on background for visual character
    {
        g.setColour(colours["Text Colour"].withAlpha(0.05f));
        const int gridStep = 16;
        for (int gy = 0; gy < getHeight(); gy += gridStep)
            for (int gx = 0; gx < getWidth(); gx += gridStep)
                g.fillEllipse((float)gx, (float)gy, 2.0f, 2.0f);
    }

    // Calculate panel areas
    auto bounds = getLocalBounds().reduced(8);
    bounds.removeFromTop(70);    // Search + filters
    bounds.removeFromBottom(32); // Status bar

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
    juce::Path detailsPath;
    detailsPath.addRoundedRectangle(detailsBounds, 8.0f);

    // Drop shadow
    melatonin::DropShadow shadow(juce::Colours::black.withAlpha(0.2f), 8, {2, 2});
    shadow.render(g, detailsPath);

    // Card fill with subtle gradient
    juce::ColourGradient cardGrad(colours["Dialog Inner Background"].brighter(0.04f), detailsBounds.getX(),
                                  detailsBounds.getY(), colours["Dialog Inner Background"].darker(0.04f),
                                  detailsBounds.getX(), detailsBounds.getBottom(), false);
    g.setGradientFill(cardGrad);
    g.fillPath(detailsPath);

    // Card border
    g.setColour(colours["Text Colour"].withAlpha(0.15f));
    g.strokePath(detailsPath, juce::PathStrokeType(1.0f));

    // Detail panel section separators
    if (nameLabel && nameValue)
    {
        int sepLeft = nameLabel->getX();
        int sepRight = nameValue->getRight();
        g.setColour(colours["Text Colour"].withAlpha(0.08f));

        // Separator after author row
        if (authorValue)
        {
            float sepY = static_cast<float>(authorValue->getBottom()) + 2.0f;
            g.drawLine(static_cast<float>(sepLeft), sepY, static_cast<float>(sepRight), sepY, 1.0f);
        }
        // Separator after size row (before buttons)
        if (sizeValue)
        {
            float sepY = static_cast<float>(sizeValue->getBottom()) + 2.0f;
            g.drawLine(static_cast<float>(sepLeft), sepY, static_cast<float>(sepRight), sepY, 1.0f);
        }
    }

    // Empty state — subtle text only, no oversized icon
    if (selectedTone == nullptr && listModel.getNumRows() == 0)
    {
        g.setColour(colours["Text Colour"].withAlpha(0.20f));
        g.setFont(FontManager::getInstance().getCaptionFont());
        g.drawText("Search to browse models", detailsBounds, juce::Justification::centred, true);
    }
    else if (selectedTone == nullptr)
    {
        // Have results but nothing selected
        g.setColour(colours["Text Colour"].withAlpha(0.25f));
        g.setFont(FontManager::getInstance().getLabelFont());
        g.drawText("Select a model", detailsBounds.withHeight(detailsBounds.getHeight()), juce::Justification::centred,
                   true);
    }

    // Draw search box background with rounded pill shape (matching local tab)
    auto searchBounds = searchBox->getBounds().toFloat();
    float cr = searchBounds.getHeight() * 0.5f; // Full pill capsule

    // Rounded background fill
    g.setColour(colours["Dialog Inner Background"]);
    g.fillRoundedRectangle(searchBounds, cr);

    // Border -- brighter when focused
    bool focused = searchBox->hasKeyboardFocus(false);
    g.setColour(focused ? colours["Accent Colour"].withAlpha(0.6f) : colours["Text Colour"].withAlpha(0.2f));
    g.drawRoundedRectangle(searchBounds.reduced(0.5f), cr, 1.0f);

    // Magnifying glass icon
    float iconSize = 14.0f;
    float iconX = searchBounds.getX() + 9.0f;
    float iconY = searchBounds.getCentreY() - iconSize * 0.4f;
    float radius = iconSize * 0.35f;

    g.setColour(colours["Text Colour"].withAlpha(0.45f));
    g.drawEllipse(iconX, iconY, radius * 2.0f, radius * 2.0f, 1.5f);
    float handleStart = iconX + radius * 1.4f + radius;
    float handleEnd = handleStart + radius * 0.9f;
    float handleY = iconY + radius * 1.4f + radius;
    g.drawLine(handleStart, handleY, handleEnd, handleY + radius * 0.9f, 1.5f);
}

void NAMOnlineBrowserComponent::resized()
{
    auto bounds = getLocalBounds().reduced(8);

    // Search row
    auto searchRow = bounds.removeFromTop(32);
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

    bounds.removeFromLeft(16); // Gap

    // Details panel
    auto detailsArea = bounds;
    detailsTitle->setBounds(detailsArea.removeFromTop(24));
    detailsArea.removeFromTop(8);

    int labelWidth = 80;
    int rowHeight = 20;

    auto detailRow = [&]()
    {
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
        spdlog::info("[NAMOnlineBrowser] Login button clicked");
        showLoginDialog();
    }
    else if (button == logoutButton.get())
    {
        spdlog::info("[NAMOnlineBrowser] Logout button clicked");
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
        juce::MessageManager::callAsync([this]() { onListSelectionChanged(); });
    }
}

void NAMOnlineBrowserComponent::mouseMove(const juce::MouseEvent& event)
{
    if (resultsList != nullptr && resultsList->isParentOf(event.eventComponent))
    {
        auto localPoint = resultsList->getLocalPoint(event.eventComponent, event.position);
        int row = resultsList->getRowContainingPosition(static_cast<int>(localPoint.x), static_cast<int>(localPoint.y));
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
        case 1:
            currentGearType = Tone3000::GearType::All;
            break;
        case 2:
            currentGearType = Tone3000::GearType::Amp;
            break;
        case 3:
            currentGearType = Tone3000::GearType::Pedal;
            break;
        case 4:
            currentGearType = Tone3000::GearType::FullRig;
            break;
        default:
            currentGearType = Tone3000::GearType::All;
            break;
        }
    }
    else if (comboBox == sortCombo.get())
    {
        int id = sortCombo->getSelectedId();
        switch (id)
        {
        case 1:
            currentSortOrder = Tone3000::SortOrder::Trending;
            break;
        case 2:
            currentSortOrder = Tone3000::SortOrder::Newest;
            break;
        case 3:
            currentSortOrder = Tone3000::SortOrder::DownloadsAllTime;
            break;
        case 4:
            currentSortOrder = Tone3000::SortOrder::BestMatch;
            break;
        default:
            currentSortOrder = Tone3000::SortOrder::Trending;
            break;
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
        [this](Tone3000::SearchResult result, Tone3000::ApiError error)
        {
            juce::MessageManager::callAsync(
                [this, result, error]()
                {
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
    spdlog::info("[NAMOnlineBrowser] showLoginDialog() called, this={}", (void*)this);

    // Create auth handler and start OAuth flow
    auto* auth = new Tone3000Auth();
    spdlog::debug("[NAMOnlineBrowser] Created Tone3000Auth object at {}", (void*)auth);

    // Use SafePointer to avoid crash if component is destroyed before callback
    juce::Component::SafePointer<NAMOnlineBrowserComponent> safeThis(this);

    auth->startAuthentication(
        [safeThis, auth](bool success, juce::String errorMessage)
        {
            spdlog::info("[NAMOnlineBrowser] Auth callback fired: success={}, error='{}', safeThis valid={}", success,
                         errorMessage.toStdString(), safeThis != nullptr);

            // Clean up auth object
            spdlog::debug("[NAMOnlineBrowser] Deleting auth object at {}", (void*)auth);
            delete auth;
            spdlog::debug("[NAMOnlineBrowser] Auth object deleted");

            if (success)
            {
                spdlog::info("[NAMOnlineBrowser] Authentication successful, queuing UI update");
                juce::MessageManager::callAsync(
                    [safeThis]()
                    {
                        spdlog::debug("[NAMOnlineBrowser] Success callAsync executing, safeThis valid={}",
                                      safeThis != nullptr);
                        if (safeThis == nullptr)
                        {
                            spdlog::warn("[NAMOnlineBrowser] Component destroyed before success callback could run");
                            return;
                        }
                        spdlog::debug("[NAMOnlineBrowser] Calling refreshAuthState()");
                        safeThis->refreshAuthState();
                        spdlog::debug("[NAMOnlineBrowser] refreshAuthState() complete");
                        if (safeThis->selectedTone != nullptr && !safeThis->selectedTone->isCached())
                        {
                            spdlog::debug("[NAMOnlineBrowser] Enabling download button");
                            safeThis->downloadButton->setEnabled(true);
                        }
                        spdlog::info("[NAMOnlineBrowser] UI update complete after successful auth");
                    });
            }
            else
            {
                spdlog::warn("[NAMOnlineBrowser] OAuth failed ({}), queuing manual dialog", errorMessage.toStdString());

                // Fall back to manual dialog
                juce::MessageManager::callAsync(
                    [safeThis]()
                    {
                        spdlog::debug("[NAMOnlineBrowser] Failure callAsync executing, safeThis valid={}",
                                      safeThis != nullptr);
                        if (safeThis == nullptr)
                        {
                            spdlog::warn("[NAMOnlineBrowser] Component destroyed before failure callback could run");
                            return;
                        }

                        spdlog::info("[NAMOnlineBrowser] Launching manual auth dialog");
                        auto* manualDialog = new Tone3000ManualAuthDialog(
                            [safeThis](bool manualSuccess)
                            {
                                spdlog::info("[NAMOnlineBrowser] Manual dialog callback: success={}", manualSuccess);
                                juce::MessageManager::callAsync(
                                    [safeThis, manualSuccess]()
                                    {
                                        spdlog::debug(
                                            "[NAMOnlineBrowser] Manual dialog callAsync executing, safeThis valid={}",
                                            safeThis != nullptr);
                                        if (safeThis == nullptr)
                                        {
                                            spdlog::warn("[NAMOnlineBrowser] Component destroyed before manual dialog "
                                                         "callback could run");
                                            return;
                                        }
                                        spdlog::debug(
                                            "[NAMOnlineBrowser] Calling refreshAuthState() after manual auth");
                                        safeThis->refreshAuthState();
                                        if (manualSuccess && safeThis->selectedTone != nullptr &&
                                            !safeThis->selectedTone->isCached())
                                        {
                                            spdlog::debug(
                                                "[NAMOnlineBrowser] Enabling download button after manual auth");
                                            safeThis->downloadButton->setEnabled(true);
                                        }
                                        spdlog::info("[NAMOnlineBrowser] UI update complete after manual auth");
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
                        spdlog::debug("[NAMOnlineBrowser] Manual auth dialog launched");
                    });
            }
        });
    spdlog::debug("[NAMOnlineBrowser] startAuthentication() called, waiting for callback");
}

void NAMOnlineBrowserComponent::logout()
{
    spdlog::info("[NAMOnlineBrowser] logout() called, this={}", (void*)this);
    Tone3000Client::getInstance().logout();
    spdlog::debug("[NAMOnlineBrowser] Tone3000Client logout complete");
    refreshAuthState();
    downloadButton->setEnabled(false);
    spdlog::info("[NAMOnlineBrowser] Logout complete");
}

void NAMOnlineBrowserComponent::goToPage(int page)
{
    currentPage = page;
    performSearch();
}

void NAMOnlineBrowserComponent::refreshAuthState()
{
    spdlog::debug("[NAMOnlineBrowser] refreshAuthState() called, this={}", (void*)this);
    bool authenticated = Tone3000Client::getInstance().isAuthenticated();
    spdlog::info("[NAMOnlineBrowser] Auth state: authenticated={}", authenticated);

    loginButton->setVisible(!authenticated);
    logoutButton->setVisible(authenticated);

    updateStatusLabel();
    spdlog::debug("[NAMOnlineBrowser] refreshAuthState() complete");
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
    listModel.setDownloadProgress(toneId, -1.0f); // Reset state
    resultsList->repaint();

    if (selectedTone && selectedTone->id == toneId.toStdString())
    {
        downloadButton->setButtonText("Download");
        downloadButton->setEnabled(true);
    }
}
