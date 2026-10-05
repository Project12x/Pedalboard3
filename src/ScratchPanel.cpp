#include "ScratchPanel.h"

#include "ColourScheme.h"
#include "FontManager.h"
#include "MainPanel.h"
#include "ScratchPanelLayout.h"
#include "ScratchPanelPresentation.h"

#include <cmath>

namespace
{
juce::Colour getColour(const char* name, juce::Colour fallback)
{
    auto& colours = ColourScheme::getInstance().colours;
    const auto found = colours.find(name);
    return found != colours.end() ? found->second : fallback;
}

juce::String formatDuration(double seconds)
{
    const int roundedSeconds = juce::roundToInt(seconds);
    return juce::String::formatted("%02d:%02d", roundedSeconds / 60, roundedSeconds % 60);
}

juce::String formatSampleRate(double sampleRate)
{
    if (sampleRate >= 1000.0)
        return juce::String(sampleRate / 1000.0, 1) + " kHz";

    return juce::String(sampleRate, 0) + " Hz";
}

juce::String formatGain(double gainDb)
{
    juce::String sign = gainDb >= 0.0 ? "+" : "";
    return sign + juce::String(gainDb, 1) + " dB";
}

juce::String formatPatchName(const ScratchTake& take)
{
    juce::String patch = take.patchName.isNotEmpty() ? take.patchName : "<untitled>";
    if (take.patchIndex > 0)
        patch << "  |  " << take.patchIndex;

    return patch;
}

juce::String getCurrentDateLabel()
{
    return juce::Time::getCurrentTime().formatted("%Y-%m-%d");
}

int countTakesOnDate(const std::vector<ScratchTake>& takes, const juce::String& dateLabel)
{
    int count = 0;
    for (const auto& take : takes)
        if (take.displayDateLabel() == dateLabel)
            ++count;

    return count;
}

juce::String getRecentCountLabel(const std::vector<ScratchTake>& takes)
{
    const auto todayCount = countTakesOnDate(takes, getCurrentDateLabel());
    return juce::String(todayCount) + " today  |  " + juce::String(static_cast<int>(takes.size())) + " total";
}

void drawLedDot(juce::Graphics& g, juce::Point<float> centre, float radius, juce::Colour colour, bool bright)
{
    if (bright)
    {
        g.setColour(colour.withAlpha(0.22f));
        g.fillEllipse(juce::Rectangle<float>(radius * 5.0f, radius * 5.0f).withCentre(centre));
    }

    g.setColour(colour.withAlpha(bright ? 1.0f : 0.42f));
    g.fillEllipse(juce::Rectangle<float>(radius * 2.0f, radius * 2.0f).withCentre(centre));
}

void fillPanel(juce::Graphics& g, juce::Rectangle<float> bounds, juce::Colour fill, juce::Colour stroke,
               float cornerRadius)
{
    g.setColour(fill);
    g.fillRoundedRectangle(bounds, cornerRadius);
    g.setColour(stroke);
    g.drawRoundedRectangle(bounds.reduced(0.5f), cornerRadius, 1.0f);
}

void drawChip(juce::Graphics& g, juce::Rectangle<float> bounds, const juce::String& label,
              juce::Colour fill, juce::Colour stroke, juce::Colour text)
{
    g.setColour(fill);
    g.fillRoundedRectangle(bounds, 8.0f);
    g.setColour(stroke);
    g.drawRoundedRectangle(bounds.reduced(0.5f), 8.0f, 1.0f);

    auto& fonts = FontManager::getInstance();
    g.setFont(fonts.getBadgeFont());
    g.setColour(text);
    g.drawFittedText(label, bounds.toNearestInt().reduced(8, 1), juce::Justification::centred, 1);
}

void styleScratchTextButton(juce::TextButton& button, juce::Colour accent, juce::Colour text, bool primary)
{
    button.setColour(juce::TextButton::buttonColourId, accent.withAlpha(primary ? 0.28f : 0.12f));
    button.setColour(juce::TextButton::buttonOnColourId, accent.withAlpha(primary ? 0.38f : 0.18f));
    button.setColour(juce::TextButton::textColourOffId, text.withAlpha(primary ? 0.96f : 0.84f));
    button.setColour(juce::TextButton::textColourOnId, text);
}

juce::String stateLabel(ScratchRecorderState state)
{
    switch (state)
    {
    case ScratchRecorderState::Ready:
        return "Ready";
    case ScratchRecorderState::Preparing:
        return "Preparing";
    case ScratchRecorderState::Recording:
        return "Recording";
    case ScratchRecorderState::Saving:
        return "Saving";
    case ScratchRecorderState::Saved:
        return "Saved";
    case ScratchRecorderState::Failed:
        return "Failed";
    }

    return "Ready";
}
} // namespace

ScratchPanel::RecordButton::RecordButton() : juce::Button("recordButton")
{
    setWantsKeyboardFocus(true);
    setTriggeredOnMouseDown(false);
}

void ScratchPanel::RecordButton::setRecordingState(bool shouldShowRecording, bool shouldShowSaving)
{
    if (recording == shouldShowRecording && saving == shouldShowSaving)
        return;

    recording = shouldShowRecording;
    saving = shouldShowSaving;
    repaint();
}

void ScratchPanel::RecordButton::paintButton(juce::Graphics& g, bool shouldDrawButtonAsHighlighted,
                                             bool shouldDrawButtonAsDown)
{
    const auto danger = getColour("Danger Colour", juce::Colour(0xffdc2626));
    const auto warning = getColour("Warning Colour", juce::Colour(0xfff59e0b));
    const auto textColour = getColour("Text Colour", juce::Colours::white);
    const auto field = getColour("Field Background", juce::Colour(0xff111827));

    auto bounds = getLocalBounds().toFloat().reduced(4.0f);
    const auto diameter = juce::jmin(bounds.getWidth(), bounds.getHeight());
    auto circle = juce::Rectangle<float>(diameter, diameter).withCentre(bounds.getCentre());

    auto fill = saving ? warning : danger;
    if (shouldDrawButtonAsDown)
        fill = fill.darker(0.22f);
    else if (shouldDrawButtonAsHighlighted)
        fill = fill.brighter(0.12f);

    if (recording || saving)
    {
        g.setColour(fill.withAlpha(0.24f));
        g.fillEllipse(circle.expanded(8.0f));
    }

    g.setColour(field.withAlpha(0.72f));
    g.fillEllipse(circle.expanded(3.0f));
    g.setColour(fill);
    g.fillEllipse(circle);
    g.setColour(textColour.withAlpha(0.92f));
    g.drawEllipse(circle.reduced(1.0f), 2.0f);

    auto& fonts = FontManager::getInstance();
    if (recording)
    {
        const auto stopSize = circle.getWidth() * 0.28f;
        g.setColour(textColour);
        g.fillRoundedRectangle(juce::Rectangle<float>(stopSize, stopSize).withCentre(circle.getCentre()), 4.0f);
    }
    else if (saving)
    {
        g.setFont(fonts.getBadgeFont());
        g.setColour(textColour);
        g.drawFittedText("SAVE", circle.toNearestInt().reduced(16), juce::Justification::centred, 1);
    }
    else
    {
        g.setFont(fonts.getBodyBoldFont().withHeight(18.0f));
        g.setColour(textColour);
        g.drawFittedText("REC", circle.toNearestInt().reduced(14), juce::Justification::centred, 1);
    }
}

class ScratchPanel::RecentTakesList::Row final : public juce::Component
{
public:
    Row(MainPanel& panel, ScratchWaveformCache& cache) : owner(&panel), waveform(cache, true)
    {
        addAndMakeVisible(waveform);
        waveform.setInterceptsMouseClicks(false, false);
        for (auto* button : {&play, &reamp, &reveal}) addAndMakeVisible(button);
        play.setTooltip("Audition the original WET capture without current effects");
        reamp.setTooltip("Audition RAW through the current rig without graph edits");
        reveal.setTooltip("Reveal this take folder");
        play.onClick = [this] { if (auto* panel = owner.getComponent()) panel->previewScratchWetTake(entry.take); };
        reamp.onClick = [this] { if (auto* panel = owner.getComponent()) panel->reampScratchRawTake(entry.take); };
        reveal.onClick = [this] { if (auto* panel = owner.getComponent()) panel->revealScratchTake(entry.take); };
    }
    void update(const ScratchTakeLoadResult& next, uint64_t generation, bool isSelected)
    {
        if (next.take.takeDirectory != entry.take.takeDirectory || generation != loadedGeneration)
        {
            waveform.setTake(next);
            loadedGeneration = generation;
        }
        entry = next;
        selected = isSelected;
        const auto actions = scratchTakeActions(entry);
        play.setEnabled(actions.play);
        reamp.setEnabled(actions.reamp);
        reveal.setEnabled(actions.reveal);
        const auto text = getColour("Text Colour", juce::Colours::white);
        const auto accent = getColour("Accent Colour", juce::Colours::cyan);
        for (auto* button : {&play, &reamp, &reveal}) styleScratchTextButton(*button, accent, text, false);
        repaint();
    }
    void resized() override
    {
        auto area = getLocalBounds().reduced(10);
        waveform.setBounds(area.removeFromLeft(84));
        auto actions = area.removeFromBottom(26);
        reveal.setBounds(actions.removeFromRight(62));
        actions.removeFromRight(6);
        reamp.setBounds(actions.removeFromRight(68));
        actions.removeFromRight(6);
        play.setBounds(actions.removeFromRight(58));
    }
    void paint(juce::Graphics& g) override
    {
        const auto text = getColour("Text Colour", juce::Colours::white);
        const auto accent = getColour("Accent Colour", juce::Colours::cyan);
        const auto field = getColour("Field Background", juce::Colour(0xff151a2d));
        const auto border = getColour("Plugin Border", juce::Colour(0xff3a4264));
        fillPanel(g, getLocalBounds().reduced(1, 3).toFloat(), selected ? field.interpolatedWith(accent, 0.12f) : field,
                  selected ? accent : border.withAlpha(0.5f), 8.0f);
        auto content = getLocalBounds().reduced(10).withTrimmedLeft(96);
        g.setColour(text);
        g.setFont(FontManager::getInstance().getBodyBoldFont());
        g.drawFittedText(formatPatchName(entry.take), content.removeFromTop(22), juce::Justification::centredLeft, 1);
        g.setFont(FontManager::getInstance().getCaptionFont());
        g.setColour(text.withAlpha(0.68f));
        g.drawFittedText(entry.take.displayDateLabel() + "  " + entry.take.displayTimeLabel() + "  |  "
                        + formatDuration(entry.take.durationSeconds()), content.removeFromTop(20),
                        juce::Justification::centredLeft, 1);
        if (entry.state != ScratchTakeLoadState::Ready)
        {
            g.setColour(getColour("Warning Colour", juce::Colours::orange));
            g.drawFittedText(entry.message, content.removeFromTop(18), juce::Justification::centredLeft, 1);
        }
    }
    void mouseDown(const juce::MouseEvent&) override
    {
        if (auto* listBox = findParentComponentOfClass<juce::ListBox>())
            listBox->selectRow(listBox->getRowNumberOfComponent(this));
    }
private:
    juce::Component::SafePointer<MainPanel> owner;
    ScratchTakeLoadResult entry;
    uint64_t loadedGeneration = 0;
    bool selected = false;
    ScratchWaveformView waveform;
    juce::TextButton play{"Play"}, reamp{"Reamp"}, reveal{"Reveal"};
};

ScratchPanel::RecentTakesList::RecentTakesList(MainPanel& panel, ScratchWaveformCache& cacheToUse)
    : owner(&panel), cache(cacheToUse)
{
    addAndMakeVisible(list);
    list.setRowHeight(112);
    list.setColour(juce::ListBox::backgroundColourId, juce::Colours::transparentBlack);
    list.setMultipleSelectionEnabled(false);
}
int ScratchPanel::RecentTakesList::getNumRows()
{
    return snapshot ? static_cast<int>(snapshot->entries.size()) : 0;
}
juce::Component* ScratchPanel::RecentTakesList::refreshComponentForRow(int row, bool selected,
                                                                     juce::Component* existing)
{
    if (row < 0 || row >= getNumRows() || owner == nullptr) { delete existing; return nullptr; }
    auto* item = dynamic_cast<Row*>(existing);
    if (!item) { delete existing; item = new Row(*owner.getComponent(), cache); }
    item->update(snapshot->entries[static_cast<size_t>(row)], snapshot->generation, selected);
    return item;
}
void ScratchPanel::RecentTakesList::setSnapshot(std::shared_ptr<const ScratchLibrarySnapshot> next,
                                               const juce::File& selection)
{
    snapshot = std::move(next);
    updating = true;
    list.updateContent();
    const auto row = snapshot ? findScratchTakeRow(*snapshot, selection) : -1;
    if (row >= 0) list.selectRow(row);
    else list.deselectAllRows();
    updating = false;
    repaint();
}
void ScratchPanel::RecentTakesList::selectedRowsChanged(int row)
{
    if (!updating && snapshot && row >= 0 && row < getNumRows() && onSelection)
        onSelection(snapshot->entries[static_cast<size_t>(row)].take.takeDirectory);
}
void ScratchPanel::RecentTakesList::paint(juce::Graphics& g)
{
    if (getNumRows() != 0) return;
    g.setColour(getColour("Text Colour", juce::Colours::white).withAlpha(0.55f));
    g.setFont(FontManager::getInstance().getBodyFont());
    g.drawFittedText(snapshot && snapshot->scanning ? "Finding saved takes..." : "Your next idea starts with REC.",
                    getLocalBounds().reduced(12), juce::Justification::centred, 2);
}

ScratchPanel::ScratchPanel(MainPanel& ownerToUse) : owner(&ownerToUse), recentTakesList(ownerToUse, waveformCache)
{
    const auto text = getColour("Text Colour", juce::Colours::white);
    const auto accent = getColour("Accent Colour", juce::Colour(0xff00d9ff));
    const auto border = getColour("Plugin Border", juce::Colour(0xff3a4264));

    addAndMakeVisible(recordButton);
    recordButton.setTooltip("Start or stop scratch capture");
    recordButton.addListener(this);

    addAndMakeVisible(chooseButton);
    chooseButton.setButtonText("Choose");
    chooseButton.addListener(this);
    chooseButton.setTooltip("Choose scratch ideas destination");

    addAndMakeVisible(resetButton);
    resetButton.setButtonText("Reset");
    resetButton.addListener(this);
    resetButton.setTooltip("Reset scratch ideas destination to the default folder");

    addAndMakeVisible(revealButton);
    revealButton.setButtonText("Reveal");
    revealButton.addListener(this);
    revealButton.setTooltip("Reveal scratch ideas folder");
    addAndMakeVisible(refreshButton);
    refreshButton.setTooltip("Rescan saved take folders without modifying them");
    refreshButton.onClick = [this] { if (auto* panel = owner.getComponent()) panel->refreshScratchLibrary(); };

    styleScratchTextButton(chooseButton, accent, text, false);
    styleScratchTextButton(resetButton, border.brighter(0.24f), text, false);
    styleScratchTextButton(revealButton, accent, text, false);
    styleScratchTextButton(refreshButton, accent, text, false);

    for (auto* label : {&statusLabel, &elapsedLabel})
    {
        addAndMakeVisible(label);
        label->setJustificationType(juce::Justification::centredLeft);
        label->setInterceptsMouseClicks(false, false);
    }
    statusLabel.setFont(FontManager::getInstance().getBodyBoldFont());
    elapsedLabel.setFont(FontManager::getInstance().getMonoDisplayFont(36.0f));
    elapsedLabel.setMinimumHorizontalScale(0.65f);

    addAndMakeVisible(recentTakesList);
    addAndMakeVisible(waveformViewport);
    waveformViewport.setViewedComponent(&selectedWaveform, false);
    waveformViewport.setScrollBarsShown(true, false);
    recentTakesList.onSelection = [this](const juce::File& directory) {
        if (directory != selectedTakeDirectory) if (auto* panel = owner.getComponent()) panel->stopScratchPlayback();
        selectedTakeDirectory = directory;
        const int row = librarySnapshot ? findScratchTakeRow(*librarySnapshot, directory) : -1;
        if (row >= 0) selectedWaveform.setTake(librarySnapshot->entries[static_cast<size_t>(row)]);
        else selectedWaveform.clear();
    };

    for (auto* button : {&wetButton, &rawButton, &pauseButton, &stopButton})
    {
        addAndMakeVisible(button); styleScratchTextButton(*button, accent, text, false);
        button->setWantsKeyboardFocus(true);
    }
    wetButton.setTooltip("Play selected original WET capture; current rig and live input are excluded");
    rawButton.setTooltip("Reamp selected RAW through the current rig; live input is excluded");
    wetButton.onClick = [this] { playSelected(ScratchPlaybackMode::OriginalWet); };
    rawButton.onClick = [this] { playSelected(ScratchPlaybackMode::CurrentRigRaw); };
    pauseButton.onClick = [this] {
        if (auto* panel = owner.getComponent()) {
            if (panel->getScratchPlaybackStatus().state == ScratchPlaybackState::Paused) panel->resumeScratchPlayback();
            else panel->pauseScratchPlayback();
        }
    };
    stopButton.onClick = [this] { if (auto* panel = owner.getComponent()) panel->stopScratchPlayback(); };
    addAndMakeVisible(loopButton);
    loopButton.onClick = [this] { if (auto* panel = owner.getComponent()) panel->setScratchPlaybackLooping(loopButton.getToggleState()); };
    addAndMakeVisible(previewVolume);
    previewVolume.setSliderStyle(juce::Slider::LinearHorizontal);
    previewVolume.setTextBoxStyle(juce::Slider::TextBoxRight, false, 62, 22);
    previewVolume.setRange(-60, 0, 0.1); previewVolume.setValue(-12, juce::dontSendNotification);
    previewVolume.setTextValueSuffix(" dB");
    previewVolume.setName("Audition level");
    previewVolume.setTooltip("Audition output level only; -60 dB mutes. Does not change rig gains.");
    previewVolume.onValueChange = [this] { if (auto* panel = owner.getComponent()) panel->setScratchPlaybackVolumeDb(previewVolume.getValue()); };
    addAndMakeVisible(playbackLabel);
    playbackLabel.setFont(FontManager::getInstance().getCaptionFont());
    playbackLabel.setInterceptsMouseClicks(false, false);
    selectedWaveform.onSeek = [this](double seconds) {
        if (auto* panel = owner.getComponent())
            if (panel->getScratchPlaybackStatus().takeDirectory == selectedTakeDirectory) panel->seekScratchPlayback(seconds);
    };
    setWantsKeyboardFocus(true);
    startTimerHz(10);
    refresh();
}

ScratchPanel::~ScratchPanel()
{
    if (auto* panel = owner.getComponent()) panel->stopScratchPlayback();
    stopTimer();
    recordButton.removeListener(this);
    chooseButton.removeListener(this);
    resetButton.removeListener(this);
    revealButton.removeListener(this);
    waveformViewport.setViewedComponent(nullptr, false);
}

void ScratchPanel::paint(juce::Graphics& g)
{
    const auto window = getColour("Window Background", juce::Colour(0xff10121f));
    const auto field = getColour("Field Background", juce::Colour(0xff151a2d));
    const auto panel = getColour("Plugin Background", juce::Colour(0xff232941));
    const auto border = getColour("Plugin Border", juce::Colour(0xff3a4264));
    const auto text = getColour("Text Colour", juce::Colours::white);
    const auto accent = getColour("Accent Colour", juce::Colour(0xff00d9ff));
    const auto audio = getColour("Audio Connection", accent);
    const auto success = getColour("Success Colour", juce::Colour(0xff16a34a));
    const auto danger = getColour("Danger Colour", juce::Colour(0xffdc2626));

    const auto layout = calculateLayout();

    g.setGradientFill(juce::ColourGradient(window.brighter(0.05f), 0.0f, 0.0f, window.darker(0.34f), 0.0f,
                                           static_cast<float>(getHeight()), false));
    g.fillAll();

    auto& fonts = FontManager::getInstance();

    auto headerTitle = layout.header;
    g.setFont(fonts.getBadgeFont());
    g.setColour(danger);
    g.drawFittedText("PEDALBOARD 3  |  INSTANT CAPTURE", headerTitle.removeFromTop(16),
                     juce::Justification::centredLeft, 1);
    g.setFont(fonts.getHeadingFont().withHeight(22.0f));
    g.setColour(text);
    g.drawFittedText("Scratch Mode", headerTitle.removeFromTop(26), juce::Justification::centredLeft, 1);
    g.setFont(fonts.getCaptionFont());
    g.setColour(text.withAlpha(0.64f));
    g.drawFittedText("Capture ideas. Revisit the sound. Try a new rig.", layout.header.withTrimmedTop(41),
                     juce::Justification::centredLeft, 1);

    fillPanel(g, layout.hero.toFloat(), panel.withAlpha(0.78f), border.withAlpha(0.78f), 15.0f);
    const auto heroText = layout.hero.withTrimmedLeft(layout.record.getWidth() + 24).reduced(4, 12);
    const auto recording = lastStatus.state == ScratchRecorderState::Recording;
    const auto saving = lastStatus.state == ScratchRecorderState::Saving;
    const auto stateColour = recording ? danger : (saving ? getColour("Warning Colour", juce::Colour(0xfff59e0b)) : success);
    const auto stateChip = layout.status.withTrimmedLeft(juce::jmax(0, layout.status.getWidth() - 110));
    drawChip(g, stateChip.toFloat(), stateLabel(lastStatus.state),
             stateColour.withAlpha(0.18f), stateColour.withAlpha(0.72f), text);
    const auto statusLedArea = layout.status.withTrimmedRight(120).toFloat();
    drawLedDot(g, {statusLedArea.getX() + 6.0f, statusLedArea.getCentreY()}, 4.0f, stateColour,
               lastStatus.state != ScratchRecorderState::Failed);

    auto armRow = heroText.withTrimmedTop(76);
    auto rawChip = armRow.removeFromLeft(116).toFloat();
    armRow.removeFromLeft(8);
    auto wetChip = armRow.removeFromLeft(116).toFloat();
    const auto* take = ScratchPanelPresentation::getDisplayTake(lastStatus);
    const auto* armedContext = ScratchPanelPresentation::getDisplayContext(lastStatus);
    const int rawChannels = take != nullptr ? take->rawChannelCount
                                            : (armedContext != nullptr ? armedContext->rawChannelCount : 0);
    const int wetChannels = take != nullptr ? take->wetChannelCount
                                            : (armedContext != nullptr ? armedContext->wetChannelCount : 0);
    drawChip(g, rawChip, "RAW " + juce::String(rawChannels) + "ch", audio.withAlpha(0.14f),
             audio.withAlpha(0.62f), text);
    drawChip(g, wetChip, "WET " + juce::String(wetChannels) + "ch", accent.withAlpha(0.14f),
             accent.withAlpha(0.62f), text);

    fillPanel(g, layout.scope.toFloat(), field.withAlpha(0.72f), border.withAlpha(0.58f), 13.0f);
    fillPanel(g, layout.context.toFloat(), panel.withAlpha(0.62f), border.withAlpha(0.55f), 13.0f);
    auto context = layout.context.reduced(12, 10);
    auto contextHeader = context.removeFromTop(14);
    g.setFont(fonts.getBadgeFont());
    g.setColour(text.withAlpha(0.48f));
    g.drawFittedText("CAPTURED WITH EVERY TAKE", contextHeader, juce::Justification::centredLeft, 1);
    context.removeFromTop(6);
    const auto patch = take != nullptr && take->patchName.isNotEmpty()
                           ? take->patchName
                           : (armedContext != nullptr && armedContext->patchName.isNotEmpty() ? armedContext->patchName
                                                                                               : "Untitled patch");
    const auto device = take != nullptr && take->deviceName.isNotEmpty()
                            ? take->deviceName
                            : (armedContext != nullptr && armedContext->deviceName.isNotEmpty() ? armedContext->deviceName
                                                                                                 : "Audio device");
    const auto rate = take != nullptr ? formatSampleRate(take->sampleRate)
                                      : (armedContext != nullptr ? formatSampleRate(armedContext->sampleRate)
                                                                 : "Sample rate");
    const auto inputGain = take != nullptr ? "IN " + formatGain(take->masterInputGainDb)
                                           : (armedContext != nullptr
                                                  ? "IN " + formatGain(armedContext->masterInputGainDb)
                                                  : "IN");
    const auto outputGain = take != nullptr ? "OUT " + formatGain(take->masterOutputGainDb)
                                            : (armedContext != nullptr
                                                   ? "OUT " + formatGain(armedContext->masterOutputGainDb)
                                                   : "OUT");
    juce::StringArray chips;
    chips.add(patch);
    chips.add(device);
    chips.add(rate);
    chips.add(inputGain);
    chips.add(outputGain);
    for (int i = 0; i < chips.size(); ++i)
    {
        const auto chip = chips[i];
        const int chipW = juce::jlimit(72, 148, chip.length() * 7 + 24);
        if (context.getWidth() < chipW)
            break;

        drawChip(g, context.removeFromLeft(chipW).toFloat(), chip, field.withAlpha(0.62f),
                 border.withAlpha(0.46f), text.withAlpha(0.86f));
        context.removeFromLeft(8);
    }

    fillPanel(g, layout.destination.toFloat(), field.withAlpha(0.58f), border.withAlpha(0.56f), 12.0f);
    auto destinationText = layout.destinationText;
    g.setFont(fonts.getCaptionFont());
    g.setColour(text.withAlpha(0.58f));
    g.drawFittedText("Destination  |  day folder", destinationText.removeFromTop(16), juce::Justification::centredLeft, 1);
    g.setFont(fonts.getLabelFont());
    g.setColour(text.withAlpha(0.86f));
    const auto root = lastStatus.scratchRoot;
    juce::String destination;
    if (take != nullptr)
        destination = take->takeDirectory.getParentDirectory().getFullPathName();
    else if (root.getFullPathName().isNotEmpty())
        destination = root.getChildFile(getCurrentDateLabel()).getFullPathName();

    const auto destinationLabel = destination.isNotEmpty() ? destination : "Default scratch folder";
    const auto compactedDestination = ScratchPanelLayout::compactDestinationPathForDisplay(
        destinationLabel, juce::jmax(16, destinationText.getWidth() / 7));
    g.drawFittedText(compactedDestination, destinationText,
                     juce::Justification::centredLeft, 1);

    g.setFont(fonts.getSubheadingFont());
    g.setColour(text);
    g.drawFittedText("Takes", layout.recentHeader, juce::Justification::centredLeft, 1);
    g.setFont(fonts.getMonoFont(11.0f));
    g.setColour(text.withAlpha(0.48f));
    const auto libraryLabel = librarySnapshot && librarySnapshot->message.isNotEmpty()
        ? librarySnapshot->message : getRecentCountLabel(libraryTakes);
    g.drawFittedText(libraryLabel, layout.recentHeader.withTrimmedLeft(70).withTrimmedRight(80),
                     juce::Justification::centredRight, 1);
}

void ScratchPanel::resized()
{
    const auto layout = calculateLayout();
    recordButton.setBounds(layout.record);
    elapsedLabel.setBounds(layout.timer);
    statusLabel.setBounds(layout.status.withTrimmedLeft(18).withTrimmedRight(120));
    chooseButton.setBounds(layout.choose);
    resetButton.setBounds(layout.reset);
    revealButton.setBounds(layout.reveal);
    refreshButton.setBounds(layout.recentHeader.withTrimmedLeft(juce::jmax(0, layout.recentHeader.getWidth() - 72)));
    recentTakesList.setBounds(layout.recent);
    waveformViewport.setBounds(layout.scope.reduced(4));
    selectedWaveform.setSize(juce::jmax(1, waveformViewport.getWidth() - 14),
                            juce::jmax(waveformViewport.getHeight(), selectedWaveform.preferredHeight()));
    auto transport = layout.transport;
    playbackLabel.setBounds(transport.removeFromTop(22));
    auto controls = transport.removeFromTop(28);
    wetButton.setBounds(controls.removeFromLeft(102)); controls.removeFromLeft(4);
    rawButton.setBounds(controls.removeFromLeft(90)); controls.removeFromLeft(4);
    pauseButton.setBounds(controls.removeFromLeft(60)); controls.removeFromLeft(4);
    stopButton.setBounds(controls.removeFromLeft(48)); controls.removeFromLeft(4);
    loopButton.setBounds(controls.removeFromLeft(58)); controls.removeFromLeft(4);
    previewVolume.setBounds(controls);
    updateRecentTakesList();
}

void ScratchPanel::playSelected(ScratchPlaybackMode mode)
{
    auto* panel = owner.getComponent();
    const int row = librarySnapshot ? findScratchTakeRow(*librarySnapshot, selectedTakeDirectory) : -1;
    if (!panel || row < 0) return;
    const auto& take = librarySnapshot->entries[static_cast<size_t>(row)].take;
    if (mode == ScratchPlaybackMode::OriginalWet) panel->previewScratchWetTake(take);
    else panel->reampScratchRawTake(take);
}

void ScratchPanel::buttonClicked(juce::Button* button)
{
    if (button == &recordButton)
    {
        if (auto* mainPanel = owner.getComponent())
            mainPanel->toggleScratchCapture();
    }
    else if (button == &revealButton)
    {
        if (auto* mainPanel = owner.getComponent())
            mainPanel->revealScratchFolder();
    }
    else if (button == &chooseButton)
    {
        if (auto* mainPanel = owner.getComponent())
            mainPanel->chooseScratchFolder();
    }
    else if (button == &resetButton)
    {
        if (auto* mainPanel = owner.getComponent())
            mainPanel->resetScratchFolderToDefault();
    }

    refresh();
}

void ScratchPanel::timerCallback()
{
    refresh();
}

void ScratchPanel::refresh()
{
    auto* mainPanel = owner.getComponent();
    if (mainPanel == nullptr)
    {
        stopTimer();
        recordButton.setEnabled(false);
        chooseButton.setEnabled(false);
        resetButton.setEnabled(false);
        revealButton.setEnabled(false);
        statusLabel.setText("Closed", juce::dontSendNotification);
        return;
    }

    const auto status = mainPanel->getScratchRecorderStatus();
    lastStatus = status;
    const bool isRecording = status.state == ScratchRecorderState::Recording;
    const bool isSaving = status.state == ScratchRecorderState::Saving;
    const bool isPreparing = status.state == ScratchRecorderState::Preparing;
    recordButton.setRecordingState(isRecording, isSaving);
    recordButton.setEnabled(status.state != ScratchRecorderState::Saving);
    chooseButton.setEnabled(!isRecording && !isSaving && !isPreparing);
    resetButton.setEnabled(!isRecording && !isSaving && !isPreparing);
    statusLabel.setText(ScratchPanelPresentation::formatStatusLine(status), juce::dontSendNotification);
    statusLabel.setColour(juce::Label::textColourId, getColour("Text Colour", juce::Colours::white).withAlpha(0.88f));
    elapsedLabel.setColour(juce::Label::textColourId, getColour("Text Colour", juce::Colours::white));

    elapsedLabel.setText(ScratchPanelPresentation::formatElapsedLabel(status), juce::dontSendNotification);

    updateRecentTakesList();
    const auto playback = mainPanel->getScratchPlaybackStatus();
    const int row = librarySnapshot ? findScratchTakeRow(*librarySnapshot, selectedTakeDirectory) : -1;
    const bool ready = row >= 0 && librarySnapshot->entries[static_cast<size_t>(row)].state == ScratchTakeLoadState::Ready;
    wetButton.setEnabled(ready && !isRecording && !isSaving && !isPreparing);
    rawButton.setEnabled(ready && !isRecording && !isSaving && !isPreparing);
    pauseButton.setEnabled(playback.state == ScratchPlaybackState::Playing || playback.state == ScratchPlaybackState::Paused);
    pauseButton.setButtonText(playback.state == ScratchPlaybackState::Paused ? "Resume" : "Pause");
    stopButton.setEnabled(playback.replacingLive);
    loopButton.setToggleState(playback.looping, juce::dontSendNotification);
    if (!previewVolume.isMouseButtonDown()) previewVolume.setValue(playback.volumeDb, juce::dontSendNotification);
    juce::String transportText = ready ? "Selected take ready - choose an audition mode" : "Select a ready take to audition";
    if (isPreparing || isRecording || isSaving) transportText = "Audition unavailable during preparation, capture or saving";
    else if (playback.replacingLive)
        transportText = (playback.mode == ScratchPlaybackMode::OriginalWet ? "ORIGINAL WET" : "RAW / CURRENT RIG")
            + juce::String("  |  Live input replaced  |  ")
            + (playback.state == ScratchPlaybackState::Paused ? "Paused" : playback.buffering ? "Loading..." : formatDuration(playback.positionSeconds));
    else if (playback.state == ScratchPlaybackState::Error) transportText = playback.message;
    playbackLabel.setText(transportText, juce::dontSendNotification);
    playbackLabel.setTooltip(transportText);
    playbackLabel.setColour(juce::Label::textColourId, getColour(playback.replacingLive ? "Warning Colour" : "Text Colour", juce::Colours::white));
    selectedWaveform.setPlayheadSeconds(playback.takeDirectory == selectedTakeDirectory ? playback.positionSeconds : 0);
    repaint();
}

void ScratchPanel::updateRecentTakesList()
{
    if (auto* panel = owner.getComponent())
    {
        const auto next = panel->getScratchLibrarySnapshot();
        if (next != librarySnapshot)
        {
            librarySnapshot = next;
            libraryTakes.clear();
            libraryTakes.reserve(next->entries.size());
            for (const auto& entry : next->entries)
            {
                auto take = entry.take;
                take.complete = entry.state == ScratchTakeLoadState::Ready;
                if (entry.message.isNotEmpty()) take.failureReason = entry.message;
                libraryTakes.push_back(std::move(take));
            }
            selectedTakeDirectory = reconcileScratchSelection(*next, selectedTakeDirectory);
            const int row = findScratchTakeRow(*next, selectedTakeDirectory);
            if (row >= 0) selectedWaveform.setTake(next->entries[static_cast<size_t>(row)]);
            else selectedWaveform.clear();
            recentTakesList.setSnapshot(next, selectedTakeDirectory);
        }
    }
    selectedWaveform.setSize(juce::jmax(1, waveformViewport.getWidth() - 14),
                            juce::jmax(waveformViewport.getHeight(), selectedWaveform.preferredHeight()));
}

ScratchPanel::Layout ScratchPanel::calculateLayout() const
{
    Layout layout;
    auto bounds = ScratchPanelLayout::calculateContentBounds(getLocalBounds()).reduced(18);

    layout.header = bounds.removeFromTop(50);
    bounds.removeFromTop(6);

    layout.hero = bounds.removeFromTop(126);
    layout.record = layout.hero.withTrimmedRight(layout.hero.getWidth() - 118).reduced(10).withSizeKeepingCentre(104, 104);
    auto heroRight = layout.hero.withTrimmedLeft(134).reduced(4, 12);
    layout.timer = heroRight.removeFromTop(46);
    layout.status = heroRight.removeFromTop(30);

    bounds.removeFromTop(12);
    layout.scope = bounds.removeFromTop(86);
    bounds.removeFromTop(4);
    layout.transport = bounds.removeFromTop(54);
    bounds.removeFromTop(8);
    layout.context = bounds.removeFromTop(68);
    bounds.removeFromTop(12);
    layout.destination = bounds.removeFromTop(54);
    const auto destinationLayout = ScratchPanelLayout::calculateDestinationLayout(layout.destination);
    layout.destinationText = destinationLayout.text;
    layout.choose = destinationLayout.choose;
    layout.reset = destinationLayout.reset;
    layout.reveal = destinationLayout.reveal;
    bounds.removeFromTop(12);
    layout.recentHeader = bounds.removeFromTop(24);
    layout.recent = bounds;

    return layout;
}
