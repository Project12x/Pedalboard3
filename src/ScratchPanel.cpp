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

void drawActionButton(juce::Graphics& g, juce::Rectangle<int> bounds, const juce::String& label,
                      bool enabled, juce::Colour accent, juce::Colour text)
{
    const auto alpha = enabled ? 1.0f : 0.34f;
    g.setColour(accent.withAlpha(enabled ? 0.18f : 0.08f));
    g.fillRoundedRectangle(bounds.toFloat(), 7.0f);
    g.setColour(accent.withAlpha(0.62f * alpha));
    g.drawRoundedRectangle(bounds.toFloat().reduced(0.5f), 7.0f, 1.0f);
    g.setFont(FontManager::getInstance().getBadgeFont());
    g.setColour(text.withAlpha(0.9f * alpha));
    g.drawFittedText(label, bounds.reduced(6, 1), juce::Justification::centred, 1);
}

void styleScratchTextButton(juce::TextButton& button, juce::Colour accent, juce::Colour text, bool primary)
{
    button.setColour(juce::TextButton::buttonColourId, accent.withAlpha(primary ? 0.28f : 0.12f));
    button.setColour(juce::TextButton::buttonOnColourId, accent.withAlpha(primary ? 0.38f : 0.18f));
    button.setColour(juce::TextButton::textColourOffId, text.withAlpha(primary ? 0.96f : 0.84f));
    button.setColour(juce::TextButton::textColourOnId, text);
}

void drawLane(juce::Graphics& g, juce::Rectangle<float> bounds, const juce::String& label,
              int channels, juce::Colour accent, juce::Colour text, juce::Colour field)
{
    auto labelArea = bounds.removeFromLeft(72.0f);

    auto& fonts = FontManager::getInstance();
    g.setFont(fonts.getBadgeFont());
    g.setColour(text.withAlpha(0.72f));
    g.drawFittedText(label, labelArea.toNearestInt(), juce::Justification::centredLeft, 1);

    g.setColour(field.withAlpha(0.88f));
    g.fillRoundedRectangle(bounds, 7.0f);

    const auto laneCount = juce::jlimit(0, 2, channels);
    if (laneCount == 0)
    {
        g.setColour(accent.withAlpha(0.28f));
        g.drawHorizontalLine(juce::roundToInt(bounds.getCentreY()), bounds.getX() + 10.0f,
                             bounds.getRight() - 10.0f);
        return;
    }

    for (int lane = 0; lane < laneCount; ++lane)
    {
        const float y = bounds.getY() + bounds.getHeight() * (0.34f + 0.24f * static_cast<float>(lane));
        juce::Path trace;
        trace.startNewSubPath(bounds.getX() + 10.0f, y);
        trace.cubicTo(bounds.getX() + bounds.getWidth() * 0.28f, y - 8.0f,
                      bounds.getX() + bounds.getWidth() * 0.52f, y + 9.0f,
                      bounds.getX() + bounds.getWidth() - 10.0f, y - 2.0f);

        g.setColour(accent.withAlpha(lane == 0 ? 0.9f : 0.48f));
        g.strokePath(trace, juce::PathStrokeType(1.6f));
    }
}

void drawThumbnailTrace(juce::Graphics& g, juce::Rectangle<float> bounds, const ScratchTake& take,
                        juce::Colour accent, juce::Colour field)
{
    g.setColour(field.withAlpha(0.78f));
    g.fillRoundedRectangle(bounds, 7.0f);

    juce::Path trace;
    const auto seed = juce::jmax(1, std::abs(take.takeId.hashCode()));
    const int points = 18;
    for (int i = 0; i < points; ++i)
    {
        const auto normX = static_cast<float>(i) / static_cast<float>(points - 1);
        const auto wave = static_cast<float>(((seed >> (i % 13)) + i * 17) % 100) / 100.0f;
        const auto y = bounds.getCentreY() + (wave - 0.5f) * bounds.getHeight() * 0.52f;
        const auto x = bounds.getX() + normX * bounds.getWidth();

        if (i == 0)
            trace.startNewSubPath(x, y);
        else
            trace.lineTo(x, y);
    }

    g.setColour(accent.withAlpha(take.complete ? 0.88f : 0.38f));
    g.strokePath(trace, juce::PathStrokeType(1.6f));
}

void drawEmptyTakeState(juce::Graphics& g, juce::Rectangle<float> bounds, juce::Colour accent,
                        juce::Colour audio, juce::Colour field, juce::Colour border, juce::Colour text)
{
    auto& fonts = FontManager::getInstance();
    fillPanel(g, bounds, field.withAlpha(0.48f), border.withAlpha(0.42f), 12.0f);

    auto card = bounds.withSizeKeepingCentre(juce::jmin(bounds.getWidth() - 28.0f, 520.0f),
                                             juce::jmin(bounds.getHeight() - 22.0f, 150.0f));
    fillPanel(g, card, field.brighter(0.04f).withAlpha(0.66f), border.withAlpha(0.52f), 14.0f);

    auto content = card.reduced(18.0f, 14.0f);
    auto waveform = content.removeFromTop(38.0f);
    g.setColour(field.darker(0.18f).withAlpha(0.76f));
    g.fillRoundedRectangle(waveform, 8.0f);

    juce::Path rawTrace;
    juce::Path wetTrace;
    for (int i = 0; i < 26; ++i)
    {
        const auto x = waveform.getX() + 14.0f + static_cast<float>(i) / 25.0f * (waveform.getWidth() - 28.0f);
        const auto rawY = waveform.getCentreY() - 5.0f + std::sin(static_cast<float>(i) * 0.82f) * 5.0f;
        const auto wetY = waveform.getCentreY() + 5.0f + std::cos(static_cast<float>(i) * 0.73f) * 5.0f;

        if (i == 0)
        {
            rawTrace.startNewSubPath(x, rawY);
            wetTrace.startNewSubPath(x, wetY);
        }
        else
        {
            rawTrace.lineTo(x, rawY);
            wetTrace.lineTo(x, wetY);
        }
    }

    g.setColour(audio.withAlpha(0.58f));
    g.strokePath(rawTrace, juce::PathStrokeType(1.3f));
    g.setColour(accent.withAlpha(0.74f));
    g.strokePath(wetTrace, juce::PathStrokeType(1.5f));

    content.removeFromTop(12.0f);
    g.setFont(fonts.getBodyBoldFont().withHeight(14.5f));
    g.setColour(text.withAlpha(0.9f));
    g.drawFittedText("Armed for first scratch take", content.removeFromTop(20.0f).toNearestInt(),
                     juce::Justification::centred, 1);

    g.setFont(fonts.getCaptionFont());
    g.setColour(text.withAlpha(0.58f));
    g.drawFittedText("RAW DI and WET print will appear here together",
                     content.removeFromTop(20.0f).toNearestInt(), juce::Justification::centred, 1);

    content.removeFromTop(7.0f);
    auto chips = content.removeFromTop(24.0f).withSizeKeepingCentre(170.0f, 24.0f);
    drawChip(g, chips.removeFromLeft(78.0f), "RAW DI", audio.withAlpha(0.12f),
             audio.withAlpha(0.45f), text.withAlpha(0.78f));
    chips.removeFromLeft(10.0f);
    drawChip(g, chips.removeFromLeft(82.0f), "WET OUT", accent.withAlpha(0.12f),
             accent.withAlpha(0.45f), text.withAlpha(0.78f));
}

juce::String stateLabel(ScratchRecorderState state)
{
    switch (state)
    {
    case ScratchRecorderState::Ready:
        return "Ready";
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

ScratchPanel::RecentTakesList::RecentTakesList(MainPanel& ownerToUse) : owner(&ownerToUse)
{
    setWantsKeyboardFocus(false);
}

void ScratchPanel::RecentTakesList::setTakes(std::vector<ScratchTake> takesToShow)
{
    takes = std::move(takesToShow);
    repaint();
}

int ScratchPanel::RecentTakesList::getPreferredHeight() const
{
    if (takes.empty())
        return 116;

    return 10 + static_cast<int>(takes.size()) * 112;
}

juce::Rectangle<int> ScratchPanel::RecentTakesList::getRowBounds(int index) const
{
    return {0, 10 + index * 112, getWidth() - 2, 102};
}

juce::Rectangle<int> ScratchPanel::RecentTakesList::getActionBounds(int index, Action action) const
{
    const auto actions = ScratchPanelLayout::calculateTakeRowActions(getRowBounds(index));

    if (action == Action::Play)
        return actions.play;

    if (action == Action::Reamp)
        return actions.reamp;

    if (action == Action::Reveal)
        return actions.reveal;

    return {};
}

ScratchPanel::RecentTakesList::Hit ScratchPanel::RecentTakesList::hitTest(juce::Point<int> point) const
{
    for (int i = 0; i < static_cast<int>(takes.size()); ++i)
    {
        for (auto action : {Action::Play, Action::Reamp, Action::Reveal})
            if (getActionBounds(i, action).contains(point))
                return {i, action};
    }

    return {};
}

void ScratchPanel::RecentTakesList::paint(juce::Graphics& g)
{
    const auto window = getColour("Window Background", juce::Colour(0xff10121f));
    const auto field = getColour("Field Background", juce::Colour(0xff151a2d));
    const auto panel = getColour("Plugin Background", juce::Colour(0xff232941));
    const auto border = getColour("Plugin Border", juce::Colour(0xff3a4264));
    const auto text = getColour("Text Colour", juce::Colours::white);
    const auto accent = getColour("Accent Colour", juce::Colour(0xff00d9ff));
    const auto audio = getColour("Audio Connection", accent);
    const auto danger = getColour("Danger Colour", juce::Colour(0xffdc2626));

    g.fillAll(juce::Colours::transparentBlack);
    auto& fonts = FontManager::getInstance();

    if (takes.empty())
    {
        auto empty = getLocalBounds().reduced(2, 10).toFloat();
        drawEmptyTakeState(g, empty, accent, audio, field, border, text);
        return;
    }

    for (int i = 0; i < static_cast<int>(takes.size()); ++i)
    {
        const auto& take = takes[static_cast<size_t>(i)];
        auto row = getRowBounds(i);
        fillPanel(g, row.toFloat(), panel.withAlpha(0.72f), border.withAlpha(0.54f), 12.0f);

        auto content = row.reduced(10, 10);
        auto thumb = content.removeFromLeft(82).toFloat();
        drawThumbnailTrace(g, thumb, take, take.complete ? audio : danger, window);
        content.removeFromLeft(12);

        auto actionColumn = content.removeFromRight(230);
        auto dateRow = actionColumn.removeFromTop(26);
        drawChip(g, dateRow.removeFromRight(104).toFloat(), take.displayDateLabel(),
                 field.withAlpha(0.66f), border.withAlpha(0.48f), text.withAlpha(0.84f));
        actionColumn.removeFromTop(8);

        auto titleArea = content.removeFromTop(24);
        const auto patch = formatPatchName(take);
        g.setFont(fonts.getBodyBoldFont().withHeight(14.0f));
        g.setColour(text);
        g.drawFittedText(patch, titleArea, juce::Justification::centredLeft, 1);

        auto metaArea = content.removeFromTop(22);
        g.setFont(fonts.getCaptionFont());
        g.setColour(text.withAlpha(0.68f));
        juce::String meta;
        meta << take.displayTimeLabel() << "  |  "
             << formatDuration(take.durationSeconds()) << "  |  "
             << formatSampleRate(take.sampleRate) << "  |  "
             << take.rawChannelCount << "/" << take.wetChannelCount << " ch";
        g.drawFittedText(meta, metaArea, juce::Justification::centredLeft, 1);

        auto chipArea = content.removeFromTop(24);
        drawChip(g, chipArea.removeFromLeft(62).toFloat(), "RAW " + juce::String(take.rawChannelCount) + "ch",
                 audio.withAlpha(0.12f), audio.withAlpha(0.5f), text.withAlpha(0.86f));
        chipArea.removeFromLeft(7);
        drawChip(g, chipArea.removeFromLeft(62).toFloat(), "WET " + juce::String(take.wetChannelCount) + "ch",
                 accent.withAlpha(0.12f), accent.withAlpha(0.5f), text.withAlpha(0.86f));
        chipArea.removeFromLeft(7);
        drawChip(g, chipArea.removeFromLeft(take.complete ? 58 : 86).toFloat(),
                 take.complete ? "saved" : "incomplete",
                 (take.complete ? accent : danger).withAlpha(0.1f),
                 (take.complete ? accent : danger).withAlpha(0.48f),
                 text.withAlpha(0.84f));

        if (!take.complete && take.failureReason.isNotEmpty())
        {
            g.setFont(fonts.getCaptionFont());
            g.setColour(danger.withAlpha(0.82f));
            g.drawFittedText(take.failureReason, content.withTrimmedTop(2), juce::Justification::centredLeft, 1);
        }

        drawActionButton(g, getActionBounds(i, Action::Play), "Play", take.canPlayWetPreview(), accent, text);
        drawActionButton(g, getActionBounds(i, Action::Reamp), "Reamp", take.canReampRawCapture(), audio, text);
        drawActionButton(g, getActionBounds(i, Action::Reveal), "Reveal", take.canReveal(), border.brighter(0.28f), text);
    }
}

void ScratchPanel::RecentTakesList::mouseDown(const juce::MouseEvent& event)
{
    auto* mainPanel = owner.getComponent();
    if (mainPanel == nullptr)
        return;

    const auto hit = hitTest(event.position.toInt());
    if (hit.takeIndex < 0 || hit.takeIndex >= static_cast<int>(takes.size()))
        return;

    const auto& take = takes[static_cast<size_t>(hit.takeIndex)];
    if (hit.action == Action::Play && take.canPlayWetPreview())
        mainPanel->previewScratchWetTake(take);
    else if (hit.action == Action::Reamp && take.canReampRawCapture())
        mainPanel->reampScratchRawTake(take);
    else if (hit.action == Action::Reveal && take.canReveal())
        mainPanel->revealScratchTake(take);
}

ScratchPanel::ScratchPanel(MainPanel& ownerToUse) : owner(&ownerToUse), recentTakesList(ownerToUse)
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

    styleScratchTextButton(chooseButton, accent, text, false);
    styleScratchTextButton(resetButton, border.brighter(0.24f), text, false);
    styleScratchTextButton(revealButton, accent, text, false);

    for (auto* label : {&statusLabel, &elapsedLabel})
    {
        addAndMakeVisible(label);
        label->setJustificationType(juce::Justification::centredLeft);
        label->setInterceptsMouseClicks(false, false);
    }
    statusLabel.setFont(FontManager::getInstance().getBodyBoldFont());
    elapsedLabel.setFont(FontManager::getInstance().getMonoDisplayFont(36.0f));
    elapsedLabel.setMinimumHorizontalScale(0.65f);

    addAndMakeVisible(recentTakesViewport);
    recentTakesViewport.setViewedComponent(&recentTakesList, false);
    recentTakesViewport.setScrollBarsShown(true, false);
    recentTakesViewport.setScrollOnDragMode(juce::Viewport::ScrollOnDragMode::all);

    startTimerHz(10);
    refresh();
}

ScratchPanel::~ScratchPanel()
{
    stopTimer();
    recordButton.removeListener(this);
    chooseButton.removeListener(this);
    resetButton.removeListener(this);
    revealButton.removeListener(this);
    recentTakesViewport.setViewedComponent(nullptr, false);
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
    g.drawFittedText("Current RAW + WET session", layout.header.withTrimmedTop(41),
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
    auto scopeTag = layout.scope.reduced(12, 6);
    g.setFont(fonts.getBadgeFont());
    g.setColour(text.withAlpha(0.46f));
    g.drawFittedText("RAW + WET sample-locked", scopeTag.removeFromTop(14).removeFromRight(156),
                     juce::Justification::centredRight, 1);
    auto scopeContent = layout.scope.reduced(14, 10).withTrimmedTop(12).toFloat();
    auto rawLane = scopeContent.removeFromTop(24.0f);
    scopeContent.removeFromTop(8.0f);
    auto wetLane = scopeContent.removeFromTop(24.0f);
    drawLane(g, rawLane, "RAW", rawChannels, audio, text, window);
    drawLane(g, wetLane, "WET", wetChannels, accent, text, window);

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
    g.drawFittedText("Recent Takes", layout.recentHeader, juce::Justification::centredLeft, 1);
    g.setFont(fonts.getMonoFont(11.0f));
    g.setColour(text.withAlpha(0.48f));
    g.drawFittedText(getRecentCountLabel(lastStatus.recentTakes), layout.recentHeader,
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
    recentTakesViewport.setBounds(layout.recent);
    updateRecentTakesList();
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
    recordButton.setRecordingState(isRecording, isSaving);
    recordButton.setEnabled(status.state != ScratchRecorderState::Saving);
    chooseButton.setEnabled(!isRecording && !isSaving);
    resetButton.setEnabled(!isRecording && !isSaving);
    statusLabel.setText(ScratchPanelPresentation::formatStatusLine(status), juce::dontSendNotification);
    statusLabel.setColour(juce::Label::textColourId, getColour("Text Colour", juce::Colours::white).withAlpha(0.88f));
    elapsedLabel.setColour(juce::Label::textColourId, getColour("Text Colour", juce::Colours::white));

    elapsedLabel.setText(ScratchPanelPresentation::formatElapsedLabel(status), juce::dontSendNotification);

    updateRecentTakesList();
    repaint();
}

void ScratchPanel::updateRecentTakesList()
{
    recentTakesList.setTakes(lastStatus.recentTakes);
    const auto listWidth = juce::jmax(1, recentTakesViewport.getWidth() - 14);
    const auto listHeight = juce::jmax(recentTakesViewport.getHeight(), recentTakesList.getPreferredHeight());
    recentTakesList.setSize(listWidth, listHeight);
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
    bounds.removeFromTop(12);
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
