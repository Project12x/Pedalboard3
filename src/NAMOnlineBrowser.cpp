/*
  ==============================================================================

    NAMOnlineBrowser.cpp
    Online browser component for TONE3000 NAM models

  ==============================================================================
*/

#include "NAMOnlineBrowser.h"

#include "ColourScheme.h"
#include "FontManager.h"
#include "IconManager.h"
#include "NAMProcessor.h"
#include "Tone3000Auth.h"
#include "Tone3000Client.h"

#include <melatonin_blur/melatonin_blur.h>
#include <spdlog/spdlog.h>

namespace
{
juce::Colour getBrowserRoleColour(const juce::String& role, juce::Colour fallback)
{
    auto& colours = ColourScheme::getInstance().colours;
    auto found = colours.find(role);
    return found != colours.end() ? found->second : fallback;
}

juce::Colour getGearAccentColour(const std::string& gearType)
{
    auto& colours = ColourScheme::getInstance().colours;
    if (gearType == "amp")
        return getBrowserRoleColour("Graph Category Amp", colours["Warning Colour"]);
    if (gearType == "pedal")
        return getBrowserRoleColour("Graph Category Modulation", colours["Audio Connection"]);
    if (gearType == "full_rig")
        return colours["Success Colour"];
    return colours["Parameter Connection"];
}

juce::String getGearDisplayText(const std::string& gearType)
{
    if (gearType == "amp")
        return "Amp";
    if (gearType == "pedal")
        return "Pedal";
    if (gearType == "full_rig")
        return "Full Rig";
    return juce::String(gearType);
}

IconManager::DomainGlyph getGearGlyph(const std::string& gearType)
{
    if (gearType == "pedal")
        return IconManager::DomainGlyph::Pedal;
    if (gearType == "full_rig")
        return IconManager::DomainGlyph::FullRig;
    return IconManager::DomainGlyph::Amp;
}

juce::String formatToneFileSize(juce::int64 fileSize)
{
    if (fileSize <= 0)
        return {};
    if (fileSize > 1024 * 1024)
        return juce::String(fileSize / (1024 * 1024)) + " MB";
    if (fileSize > 1024)
        return juce::String(fileSize / 1024) + " KB";
    return juce::String(fileSize) + " B";
}

struct BrowserPalette
{
    juce::Colour top;
    juce::Colour bottom;
    juce::Colour accent;
    juce::Colour accent2;
    juce::Colour led;
    juce::Colour text;
    juce::Colour face;
    juce::Colour face2;
    juce::Colour inset;
    juce::Colour edge;
    juce::Colour edgeHi;
};

// Keep this local palette helper in visual sync with makeBrowserPalette() in NAMModelBrowser.cpp
// without sharing online browser search/auth/download behavior.
BrowserPalette makeOnlineBrowserPalette()
{
    auto& colours = ::ColourScheme::getInstance().colours;
    const auto preset = ::ColourScheme::getInstance().presetName;

    auto palette = [](juce::uint32 top, juce::uint32 bottom, juce::uint32 face, juce::uint32 face2,
                      juce::uint32 inset, juce::uint32 edge, juce::uint32 edgeHi, juce::uint32 accent,
                      juce::uint32 accent2, juce::uint32 led, juce::uint32 text)
    {
        return BrowserPalette{juce::Colour(top),    juce::Colour(bottom), juce::Colour(accent),
                              juce::Colour(accent2), juce::Colour(led),    juce::Colour(text),
                              juce::Colour(face),   juce::Colour(face2),  juce::Colour(inset),
                              juce::Colour(edge),   juce::Colour(edgeHi)};
    };

    if (preset == "Midnight")
        return palette(0xFF211A2B, 0xFF140F1B, 0xFF271F33, 0xFF30273D, 0xFF0E0A14, 0xFF473A57, 0xFF5B4C6E,
                       0xFFFFB020, 0xFF36C8FF, 0xFF3DDC84, 0xFFF4ECDD);
    if (preset == "Deep Ocean")
        return palette(0xFF102029, 0xFF08131B, 0xFF142A36, 0xFF1B3543, 0xFF07121A, 0xFF2C5563, 0xFF3C6B7A,
                       0xFFFF9E3D, 0xFF2BD4FF, 0xFF00E0AD, 0xFFEAF3F1);
    if (preset == "Synthwave")
        return palette(0xFF1E0A28, 0xFF0F0518, 0xFF2A1139, 0xFF351747, 0xFF0C0414, 0xFF5A2D72, 0xFF76439A,
                       0xFFFF8A3D, 0xFFFF45FF, 0xFF1FFFA0, 0xFFF6EBFF);
    if (preset == "Forest")
        return palette(0xFF1C1D13, 0xFF10110A, 0xFF26281A, 0xFF2F3120, 0xFF0E0F08, 0xFF4A4D2E, 0xFF5F633D,
                       0xFFE6AD36, 0xFF79D479, 0xFF7CE87C, 0xFFF1EEDA);
    if (preset == "Daylight")
        return palette(0xFF3B332A, 0xFF2B241C, 0xFF473E33, 0xFF52483B, 0xFF241F18, 0xFF615648, 0xFF796B58,
                       0xFFFFB43A, 0xFF3AA6EC, 0xFF4DDC84, 0xFFF5EDDE);

    const auto accent = colours["Warning Colour"];
    const auto accent2 = colours["Audio Connection"];
    const auto top = colours["Window Background"].interpolatedWith(accent, 0.08f);
    const auto bottom = colours["Window Background"].darker(0.16f).interpolatedWith(accent, 0.04f);
    const auto face = colours["Dialog Inner Background"].interpolatedWith(accent, 0.055f);
    const auto face2 = colours["Dialog Inner Background"].brighter(0.08f).interpolatedWith(accent, 0.075f);
    const auto inset = colours["Window Background"].darker(0.06f).interpolatedWith(accent, 0.025f);
    const auto edge = colours["Plugin Border"].interpolatedWith(accent, 0.12f);

    return {top, bottom, accent, accent2, colours["Success Colour"], colours["Text Colour"], face, face2, inset, edge,
            edge.brighter(0.2f)};
}

class OnlineBrowserActionButtonLookAndFeel : public juce::LookAndFeel_V4
{
  public:
    void drawButtonBackground(juce::Graphics& g, juce::Button& button, const juce::Colour& /*backgroundColour*/,
                              bool isMouseOverButton, bool isButtonDown) override
    {
        const auto palette = makeOnlineBrowserPalette();
        auto bounds = button.getLocalBounds().toFloat().reduced(1.0f);
        const auto baseColour = button.findColour(juce::TextButton::buttonColourId);
        const auto label = button.getButtonText();
        const bool warmPrimary = label == "Search" || label == "Download" || label.startsWith("Downloading");
        const bool coolAudition = label == "Load";
        const auto actionAccent = coolAudition ? palette.accent2 : warmPrimary ? palette.accent : baseColour;
        auto base = palette.face2.interpolatedWith(actionAccent, button.isEnabled() ? 0.16f : 0.05f);

        if (!button.isEnabled())
            base = palette.face.withMultipliedSaturation(0.45f).withAlpha(0.62f);
        else if (isButtonDown)
            base = base.darker(0.10f);
        else if (isMouseOverButton)
            base = base.interpolatedWith(actionAccent, warmPrimary || coolAudition ? 0.12f : 0.06f);

        if (button.isEnabled() && warmPrimary)
            base = palette.inset.interpolatedWith(palette.accent, isMouseOverButton ? 0.24f : 0.16f);
        else if (button.isEnabled() && coolAudition)
            base = palette.inset.interpolatedWith(palette.accent2, isMouseOverButton ? 0.22f : 0.14f);

        const auto radius = juce::jmin(9.0f, bounds.getHeight() * 0.30f);
        if (button.isEnabled())
        {
            g.setColour(juce::Colours::black.withAlpha(isButtonDown ? 0.08f : 0.18f));
            g.fillRoundedRectangle(bounds.translated(0.0f, isButtonDown ? 0.6f : 1.4f), radius);
        }

        juce::ColourGradient fill(base.brighter(warmPrimary || coolAudition ? 0.06f : 0.12f),
                                  bounds.getX(), bounds.getY(),
                                  base.darker(warmPrimary || coolAudition ? 0.03f : 0.08f),
                                  bounds.getX(), bounds.getBottom(), false);
        fill.addColour(0.45, base.brighter(0.02f));
        g.setGradientFill(fill);
        g.fillRoundedRectangle(bounds, radius);

        g.setColour(palette.text.withAlpha(button.isEnabled() ? 0.08f : 0.04f));
        g.drawLine(bounds.getX() + 6.0f, bounds.getY() + 1.5f, bounds.getRight() - 6.0f, bounds.getY() + 1.5f, 1.0f);
        g.setColour((warmPrimary || coolAudition ? actionAccent : isMouseOverButton ? baseColour : palette.edge)
                        .withAlpha(button.isEnabled() ? 0.72f : 0.18f));
        g.drawRoundedRectangle(bounds.reduced(0.5f), radius, button.isEnabled() ? 1.0f : 0.8f);
    }

    void drawButtonText(juce::Graphics& g, juce::TextButton& button, bool /*isMouseOverButton*/,
                        bool /*isButtonDown*/) override
    {
        const auto palette = makeOnlineBrowserPalette();
        auto text = button.findColour(button.getToggleState() ? juce::TextButton::textColourOnId
                                                              : juce::TextButton::textColourOffId);
        if (!button.isEnabled())
            text = palette.text.withAlpha(0.34f);

        g.setFont(FontManager::getInstance().getBodyBoldFont());
        g.setColour(text);
        g.drawFittedText(button.getButtonText(), button.getLocalBounds().reduced(8, 2), juce::Justification::centred,
                         1);
    }
};

static OnlineBrowserActionButtonLookAndFeel onlineBrowserActionButtonLookAndFeel;
} // namespace

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
        if (downloadManager.isCached(tone))
        {
            auto cachedFile = downloadManager.getCachedFile(tone);
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
    const auto palette = makeOnlineBrowserPalette();
    const auto& tone = tones[rowNumber];

    const int margin = 7;
    const float cornerRadius = 8.0f;
    juce::Rectangle<float> itemBounds(static_cast<float>(margin), 2.0f, static_cast<float>(width - margin * 2),
                                      static_cast<float>(height - 4));
    const auto gearAccent = getGearAccentColour(tone.gearType);
    const auto surface = palette.face.interpolatedWith(palette.inset, 0.18f);

    // Background: selection > hover > base card
    if (rowIsSelected)
    {
        juce::ColourGradient selectedFill(surface.interpolatedWith(palette.accent, 0.16f), itemBounds.getX(),
                                          itemBounds.getY(), surface.interpolatedWith(palette.accent, 0.08f),
                                          itemBounds.getX(), itemBounds.getBottom(), false);
        g.setGradientFill(selectedFill);
        g.fillRoundedRectangle(itemBounds, cornerRadius);
        g.setColour(palette.accent.withAlpha(0.52f));
        g.drawRoundedRectangle(itemBounds.reduced(0.5f), cornerRadius, 1.25f);
    }
    else if (rowNumber == hoveredRow)
    {
        g.setColour(surface.interpolatedWith(palette.text, 0.035f));
        g.fillRoundedRectangle(itemBounds, cornerRadius);
        g.setColour(palette.accent.withAlpha(0.26f));
        g.drawRoundedRectangle(itemBounds.reduced(0.5f), cornerRadius, 1.0f);
    }
    else
    {
        g.setColour(surface.withAlpha(0.88f));
        g.fillRoundedRectangle(itemBounds, cornerRadius);
        g.setColour(palette.edge.withAlpha(0.38f));
        g.drawRoundedRectangle(itemBounds.reduced(0.5f), cornerRadius, 1.0f);
    }

    int rightEdge = width - margin - 10;
    const int badgeHeight = 17;
    auto& fm = FontManager::getInstance();

    auto glyphTile = itemBounds.withWidth(31.0f).reduced(6.0f, 5.0f);
    IconManager::getInstance().drawDomainGlyphTile(g, glyphTile, getGearGlyph(tone.gearType), gearAccent,
                                                   rowIsSelected || rowNumber == hoveredRow);

    // Gear type badge.
    auto gearText = getGearDisplayText(tone.gearType);
    if (gearText.isNotEmpty())
    {
        g.setFont(fm.getBadgeFont());
        const int badgeW = static_cast<int>(fm.getBadgeFont().getStringWidthFloat(gearText)) + 13;
        rightEdge -= badgeW;

        juce::Rectangle<float> badgeBounds(static_cast<float>(rightEdge), (height - badgeHeight) / 2.0f,
                                           static_cast<float>(badgeW), static_cast<float>(badgeHeight));
        g.setColour(gearAccent.withAlpha(0.16f));
        g.fillRoundedRectangle(badgeBounds, 5.0f);
        g.setColour(gearAccent.withAlpha(0.50f));
        g.drawRoundedRectangle(badgeBounds.reduced(0.5f), 5.0f, 1.0f);
        g.setColour(gearAccent.withAlpha(0.92f));
        g.drawText(gearText, badgeBounds, juce::Justification::centred, true);

        rightEdge -= 6;
    }

    auto progressIt = downloadProgress.find(tone.id);
    float progress = progressIt != downloadProgress.end() ? progressIt->second : -1.0f;
    juce::String statusText;
    juce::Colour statusColour = colours["Text Colour"].withAlpha(0.40f);

    if (tone.isCached())
    {
        statusText = "Cached";
        statusColour = palette.led;
    }
    else if (progress >= 0.0f && progress <= 1.0f)
    {
        statusText = juce::String(static_cast<int>(progress * 100)) + "%";
        statusColour = palette.accent;
    }
    else if (progress > 1.5f)
    {
        statusText = "Done";
        statusColour = colours["Success Colour"];
    }
    else if (progress < -1.5f)
    {
        statusText = "Failed";
        statusColour = colours["Danger Colour"];
    }
    else if (tone.fileSize > 0)
    {
        statusText = formatToneFileSize(tone.fileSize);
        statusColour = palette.text.withAlpha(0.46f);
    }

    if (statusText.isNotEmpty())
    {
        g.setFont(progress >= 0.0f && progress <= 1.0f ? fm.getMonoFont(9.0f) : fm.getCaptionFont());
        const int statusW = static_cast<int>(g.getCurrentFont().getStringWidthFloat(statusText)) + 14;
        rightEdge -= statusW;
        juce::Rectangle<float> statusBounds(static_cast<float>(rightEdge), (height - badgeHeight) / 2.0f,
                                            static_cast<float>(statusW), static_cast<float>(badgeHeight));
        g.setColour(statusColour.withAlpha(statusColour.getFloatAlpha() >= 0.99f ? 0.18f : 0.12f));
        g.fillRoundedRectangle(statusBounds, 5.0f);

        if (progress >= 0.0f && progress <= 1.0f)
        {
            g.setColour(statusColour.withAlpha(0.38f));
            g.fillRoundedRectangle(statusBounds.getX(), statusBounds.getY(), statusBounds.getWidth() * progress,
                                   statusBounds.getHeight(), 5.0f);
        }

        g.setColour(statusColour.withAlpha(0.62f));
        g.drawRoundedRectangle(statusBounds.reduced(0.5f), 5.0f, 1.0f);
        g.setColour(statusColour.withAlpha(0.96f));
        g.drawText(statusText, statusBounds, juce::Justification::centred, true);
        rightEdge -= 6;
    }

    // Name (primary text)
    const int textX = margin + 46;
    int textRight = rightEdge - 4;
    textRight = juce::jmax(textX + 48, textRight);
    g.setColour(rowIsSelected ? palette.text : palette.text.withAlpha(0.95f));
    g.setFont(fm.getBodyBoldFont());
    g.drawText(juce::String(tone.name), textX, 2, textRight - textX, height / 2, juce::Justification::centredLeft,
               true);

    // Author (secondary text)
    g.setFont(fm.getCaptionFont());
    g.setColour(palette.text.withAlpha(rowIsSelected ? 0.62f : 0.48f));
    const auto author = juce::String(tone.authorName).isNotEmpty() ? juce::String(tone.authorName) : "unknown author";
    g.drawText("by " + author, textX, height / 2, textRight - textX, height / 2 - 2,
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
    const auto palette = makeOnlineBrowserPalette();

    // Search controls
    searchBox = std::make_unique<juce::TextEditor>("searchBox");
    searchBox->setColour(juce::TextEditor::backgroundColourId, juce::Colours::transparentBlack);
    searchBox->setColour(juce::TextEditor::textColourId, palette.text);
    searchBox->setColour(juce::TextEditor::outlineColourId, juce::Colours::transparentBlack);
    searchBox->setColour(juce::TextEditor::focusedOutlineColourId, juce::Colours::transparentBlack);
    searchBox->setTextToShowWhenEmpty("Search TONE3000...", palette.text.withAlpha(0.5f));
    searchBox->setFont(FontManager::getInstance().getSubheadingFont()); // 15px fills pill better
    searchBox->setIndents(28, 6); // Left indent for magnifying glass icon, top indent to center text
    searchBox->addListener(this);
    addAndMakeVisible(searchBox.get());

    searchButton = std::make_unique<juce::TextButton>("Search");
    searchButton->addListener(this);
    searchButton->setLookAndFeel(&onlineBrowserActionButtonLookAndFeel);
    const auto searchFill = palette.accent;
    searchButton->setColour(juce::TextButton::buttonColourId, searchFill);
    searchButton->setColour(juce::TextButton::buttonOnColourId, searchFill.brighter(0.15f));
    searchButton->setColour(juce::TextButton::textColourOffId, searchFill);
    searchButton->setColour(juce::TextButton::textColourOnId, searchFill.brighter(0.10f));
    addAndMakeVisible(searchButton.get());

    // Filter controls
    gearTypeLabel = std::make_unique<juce::Label>("gearLabel", "Type:");
    gearTypeLabel->setColour(juce::Label::textColourId, palette.text);
    addAndMakeVisible(gearTypeLabel.get());

    gearTypeCombo = std::make_unique<juce::ComboBox>("gearType");
    gearTypeCombo->addItem("All", 1);
    gearTypeCombo->addItem("Amp", 2);
    gearTypeCombo->addItem("Pedal", 3);
    gearTypeCombo->addItem("Full Rig", 4);
    gearTypeCombo->setSelectedId(1);
    gearTypeCombo->setColour(juce::ComboBox::backgroundColourId, palette.inset);
    gearTypeCombo->setColour(juce::ComboBox::textColourId, palette.text);
    gearTypeCombo->setColour(juce::ComboBox::outlineColourId, palette.edge);
    gearTypeCombo->setColour(juce::ComboBox::arrowColourId, palette.text.withAlpha(0.6f));
    gearTypeCombo->addListener(this);
    addAndMakeVisible(gearTypeCombo.get());

    sortLabel = std::make_unique<juce::Label>("sortLabel", "Sort:");
    sortLabel->setColour(juce::Label::textColourId, palette.text);
    addAndMakeVisible(sortLabel.get());

    sortCombo = std::make_unique<juce::ComboBox>("sort");
    sortCombo->addItem("Trending", 1);
    sortCombo->addItem("Newest", 2);
    sortCombo->addItem("Most Downloaded", 3);
    sortCombo->addItem("Name A-Z", 4);
    sortCombo->setSelectedId(1);
    sortCombo->setColour(juce::ComboBox::backgroundColourId, palette.inset);
    sortCombo->setColour(juce::ComboBox::textColourId, palette.text);
    sortCombo->setColour(juce::ComboBox::outlineColourId, palette.edge);
    sortCombo->setColour(juce::ComboBox::arrowColourId, palette.text.withAlpha(0.6f));
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
    detailsContent = std::make_unique<juce::Component>("tone3000DetailsContent");
    detailsViewport = std::make_unique<juce::Viewport>("tone3000DetailsViewport");
    detailsViewport->setViewedComponent(detailsContent.get(), false);
    detailsViewport->setScrollBarsShown(true, false);
    detailsViewport->setScrollOnDragMode(juce::Viewport::ScrollOnDragMode::all);
    addAndMakeVisible(detailsViewport.get());

    detailsTitle = std::make_unique<juce::Label>("detailsTitle", "Details");
    detailsTitle->setFont(FontManager::getInstance().getSubheadingFont());
    detailsTitle->setColour(juce::Label::textColourId, palette.text);
    detailsContent->addAndMakeVisible(detailsTitle.get());

    auto createDetailLabel = [&palette](const juce::String& text)
    {
        auto label = std::make_unique<juce::Label>();
        label->setText(text, juce::dontSendNotification);
        label->setFont(FontManager::getInstance().getLabelFont());
        label->setColour(juce::Label::textColourId, palette.text.withAlpha(0.7f));
        return label;
    };

    auto createValueLabel = [&palette]()
    {
        auto label = std::make_unique<juce::Label>();
        label->setFont(FontManager::getInstance().getLabelFont());
        label->setColour(juce::Label::textColourId, palette.text);
        label->setJustificationType(juce::Justification::centredLeft);
        label->setMinimumHorizontalScale(0.72f);
        return label;
    };

    nameLabel = createDetailLabel("Name:");
    detailsContent->addAndMakeVisible(nameLabel.get());
    nameValue = createValueLabel();
    detailsContent->addAndMakeVisible(nameValue.get());

    authorLabel = createDetailLabel("Author:");
    detailsContent->addAndMakeVisible(authorLabel.get());
    authorValue = createValueLabel();
    detailsContent->addAndMakeVisible(authorValue.get());

    architectureLabel = createDetailLabel("Architecture:");
    detailsContent->addAndMakeVisible(architectureLabel.get());
    architectureValue = createValueLabel();
    detailsContent->addAndMakeVisible(architectureValue.get());

    downloadsLabel = createDetailLabel("Downloads:");
    detailsContent->addAndMakeVisible(downloadsLabel.get());
    downloadsValue = createValueLabel();
    detailsContent->addAndMakeVisible(downloadsValue.get());

    sizeLabel = createDetailLabel("Size:");
    detailsContent->addAndMakeVisible(sizeLabel.get());
    sizeValue = createValueLabel();
    detailsContent->addAndMakeVisible(sizeValue.get());

    gearLabel = createDetailLabel("Type:");
    detailsContent->addAndMakeVisible(gearLabel.get());
    gearValue = createValueLabel();
    detailsContent->addAndMakeVisible(gearValue.get());

    // Action buttons
    downloadButton = std::make_unique<juce::TextButton>("Download");
    downloadButton->addListener(this);
    downloadButton->setLookAndFeel(&onlineBrowserActionButtonLookAndFeel);
    downloadButton->setEnabled(false);
    const auto downloadFill = palette.accent;
    downloadButton->setColour(juce::TextButton::buttonColourId, downloadFill);
    downloadButton->setColour(juce::TextButton::buttonOnColourId, downloadFill.brighter(0.15f));
    downloadButton->setColour(juce::TextButton::textColourOffId, downloadFill);
    downloadButton->setColour(juce::TextButton::textColourOnId, downloadFill.brighter(0.10f));
    detailsContent->addAndMakeVisible(downloadButton.get());

    loadButton = std::make_unique<juce::TextButton>("Load");
    loadButton->addListener(this);
    loadButton->setLookAndFeel(&onlineBrowserActionButtonLookAndFeel);
    loadButton->setEnabled(false);
    const auto loadFill = palette.accent2;
    loadButton->setColour(juce::TextButton::buttonColourId, loadFill);
    loadButton->setColour(juce::TextButton::buttonOnColourId, loadFill.brighter(0.2f));
    loadButton->setColour(juce::TextButton::textColourOffId, loadFill);
    loadButton->setColour(juce::TextButton::textColourOnId, loadFill.brighter(0.10f));
    detailsContent->addAndMakeVisible(loadButton.get());

    // Status bar
    statusLabel = std::make_unique<juce::Label>("status", "Not logged in");
    statusLabel->setFont(FontManager::getInstance().getCaptionFont());
    statusLabel->setColour(juce::Label::textColourId, colours["Text Colour"].withAlpha(0.7f));
    addAndMakeVisible(statusLabel.get());

    loginButton = std::make_unique<juce::TextButton>("Login");
    loginButton->addListener(this);
    loginButton->setLookAndFeel(&onlineBrowserActionButtonLookAndFeel);
    loginButton->setColour(juce::TextButton::buttonColourId, palette.face2);
    loginButton->setColour(juce::TextButton::buttonOnColourId, palette.face2.brighter(0.12f));
    loginButton->setColour(juce::TextButton::textColourOffId, palette.text.withAlpha(0.86f));
    loginButton->setColour(juce::TextButton::textColourOnId, palette.text);
    addAndMakeVisible(loginButton.get());

    logoutButton = std::make_unique<juce::TextButton>("Logout");
    logoutButton->addListener(this);
    logoutButton->setLookAndFeel(&onlineBrowserActionButtonLookAndFeel);
    logoutButton->setVisible(false);
    logoutButton->setColour(juce::TextButton::buttonColourId, palette.face2);
    logoutButton->setColour(juce::TextButton::buttonOnColourId, palette.face2.brighter(0.12f));
    logoutButton->setColour(juce::TextButton::textColourOffId, palette.text.withAlpha(0.86f));
    logoutButton->setColour(juce::TextButton::textColourOnId, palette.text);
    addAndMakeVisible(logoutButton.get());

    prevPageButton = std::make_unique<juce::TextButton>("<");
    prevPageButton->addListener(this);
    prevPageButton->setLookAndFeel(&onlineBrowserActionButtonLookAndFeel);
    prevPageButton->setEnabled(false);
    prevPageButton->setColour(juce::TextButton::buttonColourId, palette.face2);
    prevPageButton->setColour(juce::TextButton::buttonOnColourId, palette.face2.brighter(0.12f));
    prevPageButton->setColour(juce::TextButton::textColourOffId, palette.text.withAlpha(0.82f));
    prevPageButton->setColour(juce::TextButton::textColourOnId, palette.text);
    addAndMakeVisible(prevPageButton.get());

    nextPageButton = std::make_unique<juce::TextButton>(">");
    nextPageButton->addListener(this);
    nextPageButton->setLookAndFeel(&onlineBrowserActionButtonLookAndFeel);
    nextPageButton->setEnabled(false);
    nextPageButton->setColour(juce::TextButton::buttonColourId, palette.face2);
    nextPageButton->setColour(juce::TextButton::buttonOnColourId, palette.face2.brighter(0.12f));
    nextPageButton->setColour(juce::TextButton::textColourOffId, palette.text.withAlpha(0.82f));
    nextPageButton->setColour(juce::TextButton::textColourOnId, palette.text);
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
    searchButton->setLookAndFeel(nullptr);
    downloadButton->setLookAndFeel(nullptr);
    loadButton->setLookAndFeel(nullptr);
    loginButton->setLookAndFeel(nullptr);
    logoutButton->setLookAndFeel(nullptr);
    prevPageButton->setLookAndFeel(nullptr);
    nextPageButton->setLookAndFeel(nullptr);
    Tone3000DownloadManager::getInstance().removeListener(this);
    spdlog::debug("[NAMOnlineBrowser] Removed download listener");
}

bool NAMOnlineBrowserComponent::isCompactLayout() const
{
    return getWidth() < 780 || getHeight() < 620;
}

void NAMOnlineBrowserComponent::paint(juce::Graphics& g)
{
    const auto palette = makeOnlineBrowserPalette();
    const bool compactLayout = isCompactLayout();

    // Gradient background
    juce::ColourGradient bgGradient(palette.top, 0, 0, palette.bottom, 0, static_cast<float>(getHeight()), false);
    g.setGradientFill(bgGradient);
    g.fillAll();

    // Subtle dot-grid pattern on background for visual character
    {
        g.setColour(palette.text.withAlpha(0.05f));
        const int gridStep = 16;
        for (int gy = 0; gy < getHeight(); gy += gridStep)
            for (int gx = 0; gx < getWidth(); gx += gridStep)
                g.fillEllipse((float)gx, (float)gy, 2.0f, 2.0f);
    }

    auto outer = getLocalBounds().reduced(8);
    auto toolbarBounds = outer.removeFromTop(compactLayout ? 106 : 118).toFloat();
    auto footerArea = getLocalBounds().reduced(8);
    auto footerBounds = footerArea.removeFromBottom(28).toFloat();

    juce::ColourGradient toolbarFill(palette.face2, toolbarBounds.getX(),
                                     toolbarBounds.getY(), palette.face,
                                     toolbarBounds.getX(), toolbarBounds.getBottom(), false);
    g.setGradientFill(toolbarFill);
    g.fillRoundedRectangle(toolbarBounds, 8.0f);
    g.setColour(palette.edge.withAlpha(0.75f));
    g.drawRoundedRectangle(toolbarBounds.reduced(0.5f), 8.0f, 1.0f);

    if (searchBox != nullptr)
    {
        auto searchPill = searchBox->getBounds().toFloat();
        juce::ColourGradient searchFill(palette.inset.brighter(0.03f), searchPill.getX(),
                                        searchPill.getY(), palette.inset.darker(0.06f),
                                        searchPill.getX(), searchPill.getBottom(), false);
        g.setGradientFill(searchFill);
        g.fillRoundedRectangle(searchPill, 7.0f);
        g.setColour(palette.edge.withAlpha(0.70f));
        g.drawRoundedRectangle(searchPill.reduced(0.5f), 7.0f, 1.0f);
        g.setColour(palette.text.withAlpha(0.05f));
        g.drawLine(searchPill.getX() + 8.0f, searchPill.getY() + 2.0f, searchPill.getRight() - 8.0f,
                   searchPill.getY() + 2.0f, 1.0f);
    }

    g.setColour(palette.inset.withAlpha(0.74f));
    g.fillRoundedRectangle(footerBounds, 7.0f);
    g.setColour(palette.edge.withAlpha(0.64f));
    g.drawRoundedRectangle(footerBounds.reduced(0.5f), 7.0f, 1.0f);

    // Draw rounded list background
    auto listBounds = resultsList != nullptr ? resultsList->getBounds().toFloat() : juce::Rectangle<float>();
    juce::ColourGradient listFill(palette.inset.brighter(0.03f), listBounds.getX(),
                                  listBounds.getY(), palette.inset.darker(0.05f),
                                  listBounds.getX(), listBounds.getBottom(), false);
    g.setGradientFill(listFill);
    g.fillRoundedRectangle(listBounds, 8.0f);
    g.setColour(palette.accent.withAlpha(0.12f));
    g.drawLine(listBounds.getX() + 9.0f, listBounds.getY() + 2.0f, listBounds.getRight() - 9.0f,
               listBounds.getY() + 2.0f, 1.0f);
    g.setColour(palette.edge.withAlpha(0.70f));
    g.drawRoundedRectangle(listBounds.reduced(0.5f), 8.0f, 1.0f);

    // Draw card-style details panel with shadow
    auto detailsBounds = detailsViewport != nullptr ? detailsViewport->getBounds().toFloat() : juce::Rectangle<float>();
    juce::Path detailsPath;
    detailsPath.addRoundedRectangle(detailsBounds, 8.0f);

    melatonin::DropShadow shadow{juce::Colours::black.withAlpha(0.28f), 10, {0, 4}};
    shadow.render(g, detailsPath);

    juce::ColourGradient cardGrad(palette.face2, detailsBounds.getX(), detailsBounds.getY(),
                                  palette.face, detailsBounds.getX(), detailsBounds.getBottom(), false);
    g.setGradientFill(cardGrad);
    g.fillPath(detailsPath);

    g.setColour(palette.edgeHi.withAlpha(0.36f));
    g.strokePath(detailsPath, juce::PathStrokeType(1.0f));

    if (selectedTone != nullptr)
    {
        const auto gearAccent = getGearAccentColour(selectedTone->gearType);
        auto hero = detailsBounds.reduced(18.0f, 14.0f).removeFromTop(92.0f);
        auto glyph = hero.removeFromLeft(58.0f).reduced(2.0f, 10.0f);
        IconManager::getInstance().drawDomainGlyphTile(g, glyph, getGearGlyph(selectedTone->gearType), gearAccent,
                                                       true);

        auto heroText = hero.reduced(12.0f, 4.0f);
        auto chip = heroText.removeFromBottom(20.0f);
        const auto gearText = getGearDisplayText(selectedTone->gearType);
        const float chipWidth = juce::jlimit(52.0f, 108.0f,
                                             FontManager::getInstance().getCaptionFont().getStringWidthFloat(gearText) +
                                                 24.0f);
        auto chipBounds = chip.withWidth(chipWidth);
        g.setColour(gearAccent.withAlpha(0.16f));
        g.fillRoundedRectangle(chipBounds, 7.0f);
        g.setColour(gearAccent.withAlpha(0.58f));
        g.drawRoundedRectangle(chipBounds.reduced(0.5f), 7.0f, 1.0f);
        g.setFont(FontManager::getInstance().getCaptionFont());
        g.setColour(gearAccent.withAlpha(0.95f));
        g.drawText(gearText.toUpperCase(), chipBounds, juce::Justification::centred, true);

        g.setFont(FontManager::getInstance().getSubheadingFont());
        g.setColour(palette.text);
        g.drawText(juce::String(selectedTone->name), heroText.removeFromTop(28.0f),
                   juce::Justification::centredLeft, true);
        g.setFont(FontManager::getInstance().getCaptionFont());
        g.setColour(palette.text.withAlpha(0.62f));
        g.drawText("by " + juce::String(selectedTone->authorName), heroText.removeFromTop(20.0f),
                   juce::Justification::centredLeft, true);

        auto separator = detailsBounds.reduced(18.0f, 0.0f).withY(detailsBounds.getY() + 114.0f).withHeight(1.0f);
        g.setColour(gearAccent.withAlpha(0.24f));
        g.fillRect(separator);
    }

    // Detail panel section separators
    if (nameLabel && nameValue)
    {
        auto boundsInBrowser = [this](const juce::Component* component)
        {
            if (component == nullptr)
                return juce::Rectangle<int>();
            if (detailsContent != nullptr && detailsViewport != nullptr && component->getParentComponent() == detailsContent.get())
            {
                return component->getBounds().translated(detailsViewport->getX() - detailsViewport->getViewPositionX(),
                                                         detailsViewport->getY() - detailsViewport->getViewPositionY());
            }

            return component->getBounds();
        };
        int sepLeft = boundsInBrowser(nameLabel.get()).getX();
        int sepRight = boundsInBrowser(nameValue.get()).getRight();
        g.setColour(palette.text.withAlpha(0.08f));

        const juce::Label* values[] = {nameValue.get(), authorValue.get(), gearValue.get(), architectureValue.get(),
                                       downloadsValue.get(), sizeValue.get()};
        for (const auto* value : values)
        {
            if (value == nullptr)
                continue;
            float sepY = static_cast<float>(boundsInBrowser(value).getBottom()) + 2.0f;
            if (sepY < detailsBounds.getY() || sepY > detailsBounds.getBottom())
                continue;
            g.drawLine(static_cast<float>(sepLeft), sepY, static_cast<float>(sepRight), sepY, 1.0f);
        }
    }

    // Empty state — subtle text only, no oversized icon
    if (selectedTone == nullptr && listModel.getNumRows() == 0)
    {
        auto empty = detailsBounds.reduced(26.0f);
        auto icon = empty.withSizeKeepingCentre(56.0f, 56.0f).translated(0.0f, -28.0f);
        IconManager::getInstance().drawDomainGlyphTile(g, icon, IconManager::DomainGlyph::Amp,
                                                       palette.accent, false);
        g.setColour(palette.text.withAlpha(0.34f));
        g.setFont(FontManager::getInstance().getLabelFont());
        g.drawText("Search TONE3000 to browse NAM models", empty.translated(0.0f, 34.0f),
                   juce::Justification::centred, true);
    }
    else if (selectedTone == nullptr)
    {
        // Have results but nothing selected
        auto empty = detailsBounds.reduced(26.0f);
        auto icon = empty.withSizeKeepingCentre(56.0f, 56.0f).translated(0.0f, -28.0f);
        IconManager::getInstance().drawDomainGlyphTile(g, icon, IconManager::DomainGlyph::FullRig,
                                                       palette.accent, false);
        g.setColour(palette.text.withAlpha(0.36f));
        g.setFont(FontManager::getInstance().getLabelFont());
        g.drawText("Select a model for details", empty.translated(0.0f, 34.0f), juce::Justification::centred, true);
    }
}

void NAMOnlineBrowserComponent::paintOverChildren(juce::Graphics& g)
{
    // Draw magnifying glass icon centered in the search pill
    const auto palette = makeOnlineBrowserPalette();
    auto searchBounds = searchBox->getBounds().toFloat();

    float iconSize = 13.0f;
    float radius = iconSize * 0.35f;
    float iconX = searchBounds.getX() + 10.0f;
    float iconCentreY = searchBounds.getCentreY();

    g.setColour(palette.text.withAlpha(0.45f));
    // Circle part - centered vertically
    g.drawEllipse(iconX, iconCentreY - radius, radius * 2.0f, radius * 2.0f, 1.5f);
    // Handle
    float handleStartX = iconX + radius + radius * 0.7f;
    float handleStartY = iconCentreY + radius * 0.7f;
    g.drawLine(handleStartX, handleStartY, handleStartX + radius * 0.8f, handleStartY + radius * 0.8f, 1.5f);
}

void NAMOnlineBrowserComponent::resized()
{
    const bool compactLayout = isCompactLayout();
    auto bounds = getLocalBounds().reduced(8);

    // Search row
    bounds.removeFromTop(compactLayout ? 26 : 34);
    auto searchRow = bounds.removeFromTop(compactLayout ? 34 : 38);
    const int searchButtonWidth = compactLayout ? 76 : 86;
    const int searchButtonGap = compactLayout ? 10 : 12;
    searchButton->setBounds(searchRow.removeFromRight(juce::jmin(searchButtonWidth, searchRow.getWidth())));
    searchRow.removeFromRight(juce::jmin(searchButtonGap, searchRow.getWidth()));
    const int maxSearchBoxWidth = compactLayout ? 210 : 240;
    const int searchBoxWidth = juce::jmin(maxSearchBoxWidth, searchRow.getWidth());
    searchBox->setBounds(searchRow.removeFromLeft(searchBoxWidth));

    bounds.removeFromTop(compactLayout ? 10 : 12);

    // Filter row
    auto filterRow = bounds.removeFromTop(28);
    gearTypeLabel->setBounds(filterRow.removeFromLeft(40));
    gearTypeCombo->setBounds(filterRow.removeFromLeft(90));
    filterRow.removeFromLeft(16);
    sortLabel->setBounds(filterRow.removeFromLeft(35));
    sortCombo->setBounds(filterRow.removeFromLeft(120));

    bounds.removeFromTop(compactLayout ? 10 : 12);

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
    auto detailsViewportBounds = bounds;
    detailsViewport->setBounds(detailsViewportBounds);
    const int detailsContentWidth = juce::jmax(1, detailsViewportBounds.getWidth() - 12);
    auto detailsArea = juce::Rectangle<int>(0, 0, detailsContentWidth, 1);
    detailsTitle->setBounds(detailsArea.removeFromTop(24));
    detailsArea.removeFromTop(selectedTone != nullptr ? 92 : 8);

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
    auto buttonRow = detailsArea.removeFromTop(compactLayout ? 31 : 33);
    downloadButton->setBounds(buttonRow.removeFromLeft(compactLayout ? 106 : 124));
    buttonRow.removeFromLeft(compactLayout ? 8 : 10);
    loadButton->setBounds(buttonRow.removeFromLeft(compactLayout ? 82 : 92));

    detailsContent->setSize(detailsContentWidth,
                            juce::jmax(detailsViewportBounds.getHeight(), buttonRow.getBottom() + 16));
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
        if (selectedTone && Tone3000DownloadManager::getInstance().isCached(*selectedTone))
        {
            loadCachedModel(*selectedTone);
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
        resized();
        repaint();
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
    bool isCached = Tone3000DownloadManager::getInstance().isCached(*tone);
    bool isDownloading = Tone3000DownloadManager::getInstance().isDownloading(juce::String(tone->id));

    downloadButton->setEnabled(!isCached && !isDownloading && Tone3000Client::getInstance().isAuthenticated());
    downloadButton->setButtonText(isDownloading ? "Downloading..." : "Download");
    loadButton->setEnabled(isCached);
    resized();
    repaint();
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

void NAMOnlineBrowserComponent::loadCachedModel(const Tone3000::ToneInfo& tone)
{
    auto cachedFile = Tone3000DownloadManager::getInstance().getCachedFile(tone);

    if (!cachedFile.existsAsFile())
    {
        spdlog::error("[NAMOnlineBrowser] Cached file not found for {}", tone.id);
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
                        if (safeThis->selectedTone != nullptr &&
                            !Tone3000DownloadManager::getInstance().isCached(*safeThis->selectedTone))
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
                                            !Tone3000DownloadManager::getInstance().isCached(*safeThis->selectedTone))
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
