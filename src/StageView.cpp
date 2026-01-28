/*
  ==============================================================================

    StageView.cpp
    Performance/Stage Mode - Fullscreen overlay for live use

  ==============================================================================
*/

#include "StageView.h"

#include "ColourScheme.h"
#include "FontManager.h"
#include "MainPanel.h"
#include "TunerProcessor.h"

//==============================================================================
StageView::StageView(MainPanel* panel) : mainPanel(panel)
{
    auto& colours = ColourScheme::getInstance().colours;

    // Ensure this component is opaque (draws its entire area)
    setOpaque(true);

    // Navigation buttons - using ASCII-safe labels
    prevButton = std::make_unique<TextButton>("<< PREV");
    prevButton->addListener(this);
    prevButton->setColour(TextButton::buttonColourId, colours["Plugin Border"].darker(0.2f));
    prevButton->setColour(TextButton::textColourOffId, Colours::white);
    addAndMakeVisible(prevButton.get());

    nextButton = std::make_unique<TextButton>("NEXT >>");
    nextButton->addListener(this);
    nextButton->setColour(TextButton::buttonColourId, colours["Plugin Border"].darker(0.2f));
    nextButton->setColour(TextButton::textColourOffId, Colours::white);
    addAndMakeVisible(nextButton.get());

    // Panic button
    panicButton = std::make_unique<TextButton>("PANIC");
    panicButton->addListener(this);
    panicButton->setColour(TextButton::buttonColourId, Colours::darkred);
    panicButton->setColour(TextButton::textColourOffId, Colours::white);
    addAndMakeVisible(panicButton.get());

    // Exit button
    exitButton = std::make_unique<TextButton>("EXIT");
    exitButton->addListener(this);
    exitButton->setColour(TextButton::buttonColourId, colours["Plugin Border"].darker(0.3f));
    exitButton->setColour(TextButton::textColourOffId, Colours::white.withAlpha(0.8f));
    addAndMakeVisible(exitButton.get());

    // Tuner toggle
    tunerToggleButton = std::make_unique<TextButton>("TUNER");
    tunerToggleButton->addListener(this);
    tunerToggleButton->setClickingTogglesState(true);
    tunerToggleButton->setToggleState(true, dontSendNotification);
    tunerToggleButton->setColour(TextButton::buttonColourId, colours["Plugin Border"].darker(0.2f));
    tunerToggleButton->setColour(TextButton::buttonOnColourId, Colour(0xFF00AA55));
    tunerToggleButton->setColour(TextButton::textColourOffId, Colours::white.withAlpha(0.7f));
    tunerToggleButton->setColour(TextButton::textColourOnId, Colours::white);
    addAndMakeVisible(tunerToggleButton.get());

    // Capture keyboard focus
    setWantsKeyboardFocus(true);

    // Timer for tuner updates (30 fps)
    startTimerHz(30);
}

StageView::~StageView()
{
    stopTimer();
}

//==============================================================================
void StageView::updatePatchInfo(const String& patchName, int currentIndex, int totalPatches)
{
    currentPatchName = patchName;
    currentPatchIndex = currentIndex;
    totalPatchCount = totalPatches;
    repaint();
}

void StageView::setTunerProcessor(TunerProcessor* tuner)
{
    tunerProcessor = tuner;
}

//==============================================================================
void StageView::timerCallback()
{
    if (tunerProcessor != nullptr && showTuner)
    {
        float targetCents = tunerProcessor->getCentsDeviation();
        displayedCents += (targetCents - displayedCents) * NEEDLE_SMOOTHING;

        float targetAngle = jlimit(-50.0f, 50.0f, displayedCents) * 0.9f;
        needleAngle += (targetAngle - needleAngle) * NEEDLE_SMOOTHING;

        detectedNote = tunerProcessor->getDetectedNote();
        repaint();
    }
}

//==============================================================================
void StageView::paint(Graphics& g)
{
    auto& colours = ColourScheme::getInstance().colours;
    auto bounds = getLocalBounds().toFloat();

    // Dark background with subtle gradient
    Colour bgTop = Colour(0xFF1a1a2e);
    Colour bgBot = Colour(0xFF0f0f1a);
    g.setGradientFill(ColourGradient(bgTop, 0, 0, bgBot, 0, bounds.getHeight(), false));
    g.fillAll();

    // Layout areas
    float headerHeight = 50.0f;
    float footerHeight = 80.0f;
    float tunerHeight = showTuner ? 180.0f : 0.0f;

    auto headerArea = bounds.removeFromTop(headerHeight);
    auto footerArea = bounds.removeFromBottom(footerHeight);
    auto tunerArea = bounds.removeFromBottom(tunerHeight);
    auto patchArea = bounds;

    // Draw sections
    drawStatusBar(g, headerArea);
    drawPatchDisplay(g, patchArea);

    if (showTuner && tunerProcessor != nullptr)
        drawTunerDisplay(g, tunerArea);
}

void StageView::drawStatusBar(Graphics& g, Rectangle<float> bounds)
{
    auto& fonts = FontManager::getInstance();

    // "STAGE MODE" title
    g.setColour(Colours::white.withAlpha(0.5f));
    g.setFont(fonts.getUIFont(16.0f, true));
    g.drawText("STAGE MODE", bounds.reduced(20, 0), Justification::centredLeft);

    // Time display (optional)
    Time now = Time::getCurrentTime();
    String timeStr = now.formatted("%H:%M");
    g.setFont(fonts.getMonoFont(14.0f));
    g.drawText(timeStr, bounds.reduced(80, 0), Justification::centredRight);
}

void StageView::drawPatchDisplay(Graphics& g, Rectangle<float> bounds)
{
    auto& fonts = FontManager::getInstance();
    auto centre = bounds.getCentre();

    // Large patch name
    g.setColour(Colours::white);
    g.setFont(fonts.getUIFont(72.0f, true));

    // Truncate long names
    String displayName = currentPatchName;
    if (displayName.length() > 25)
        displayName = displayName.substring(0, 22) + "...";

    g.drawText(displayName, bounds.reduced(100, 0), Justification::centred);

    // Patch position indicator
    if (totalPatchCount > 0)
    {
        g.setColour(Colours::white.withAlpha(0.5f));
        g.setFont(fonts.getMonoFont(24.0f));
        String posStr = String(currentPatchIndex + 1) + " / " + String(totalPatchCount);
        g.drawText(posStr, bounds.translated(0, 60), Justification::centred);
    }
}

void StageView::drawTunerDisplay(Graphics& g, Rectangle<float> bounds)
{
    auto& fonts = FontManager::getInstance();
    auto& colours = ColourScheme::getInstance().colours;

    // Separator line
    g.setColour(colours["Plugin Border"].withAlpha(0.3f));
    g.drawHorizontalLine(static_cast<int>(bounds.getY()), bounds.getX() + 40, bounds.getRight() - 40);

    auto centreX = bounds.getCentreX();
    auto centreY = bounds.getCentreY();

    if (tunerProcessor == nullptr || !tunerProcessor->isPitchDetected())
    {
        g.setColour(Colours::white.withAlpha(0.25f));
        g.setFont(fonts.getUIFont(32.0f));
        g.drawText("Waiting for signal...", bounds, Justification::centred);
        return;
    }

    // Note display
    String noteName = getNoteName(detectedNote);
    Colour noteCol = getTuningColour(displayedCents);

    g.setColour(noteCol);
    g.setFont(fonts.getUIFont(64.0f, true));
    g.drawText(noteName, bounds.withTrimmedBottom(60), Justification::centred);

    // Cents display
    g.setFont(fonts.getMonoFont(28.0f));
    String centsStr = (displayedCents >= 0 ? "+" : "") + String(static_cast<int>(displayedCents)) + " cents";
    g.drawText(centsStr, bounds.withTrimmedTop(80), Justification::centred);

    // Simple bar indicator
    float barWidth = 400.0f;
    float barHeight = 12.0f;
    float barX = centreX - barWidth / 2;
    float barY = centreY + 50;

    // Background bar
    g.setColour(colours["Plugin Border"].darker(0.3f));
    g.fillRoundedRectangle(barX, barY, barWidth, barHeight, 6.0f);

    // Center marker
    g.setColour(Colours::white.withAlpha(0.5f));
    g.fillRect(centreX - 1.5f, barY - 4, 3.0f, barHeight + 8);

    // Indicator position
    float indicatorPos = jlimit(-1.0f, 1.0f, displayedCents / 50.0f);
    float indicatorX = centreX + indicatorPos * (barWidth / 2 - 10);

    g.setColour(noteCol);
    g.fillEllipse(indicatorX - 8, barY - 2, 16, barHeight + 4);
}

//==============================================================================
void StageView::resized()
{
    auto bounds = getLocalBounds();
    int margin = 20;
    int buttonHeight = 60;
    int buttonWidth = 120;

    // Header buttons (top right)
    exitButton->setBounds(bounds.getWidth() - buttonWidth - margin, margin, buttonWidth, 40);
    tunerToggleButton->setBounds(bounds.getWidth() - buttonWidth * 2 - margin * 2, margin, buttonWidth, 40);

    // Navigation buttons (sides, vertically centered)
    int navY = bounds.getCentreY() - buttonHeight / 2;
    prevButton->setBounds(margin, navY, buttonWidth + 20, buttonHeight);
    nextButton->setBounds(bounds.getWidth() - buttonWidth - 20 - margin, navY, buttonWidth + 20, buttonHeight);

    // Panic button (bottom right)
    panicButton->setBounds(bounds.getWidth() - 160 - margin, bounds.getHeight() - buttonHeight - margin, 160,
                           buttonHeight);
}

//==============================================================================
void StageView::buttonClicked(Button* button)
{
    if (mainPanel == nullptr)
        return;

    if (button == prevButton.get())
    {
        mainPanel->getApplicationCommandManager()->invokeDirectly(MainPanel::PatchPrevPatch, true);
    }
    else if (button == nextButton.get())
    {
        mainPanel->getApplicationCommandManager()->invokeDirectly(MainPanel::PatchNextPatch, true);
    }
    else if (button == panicButton.get())
    {
        mainPanel->getApplicationCommandManager()->invokeDirectly(MainPanel::EditPanic, true);
    }
    else if (button == exitButton.get())
    {
        mainPanel->toggleStageMode();
    }
    else if (button == tunerToggleButton.get())
    {
        showTuner = tunerToggleButton->getToggleState();
        resized();
        repaint();
    }
}

bool StageView::keyPressed(const KeyPress& key)
{
    if (mainPanel == nullptr)
        return false;

    // Exit stage mode
    if (key == KeyPress::escapeKey || key == KeyPress::F11Key)
    {
        mainPanel->toggleStageMode();
        return true;
    }
    // Previous patch - Up, Left, Page Up
    else if (key == KeyPress::upKey || key == KeyPress::leftKey || key == KeyPress::pageUpKey)
    {
        mainPanel->getApplicationCommandManager()->invokeDirectly(MainPanel::PatchPrevPatch, true);
        updateAfterPatchChange();
        return true;
    }
    // Next patch - Down, Right, Page Down
    else if (key == KeyPress::downKey || key == KeyPress::rightKey || key == KeyPress::pageDownKey)
    {
        mainPanel->getApplicationCommandManager()->invokeDirectly(MainPanel::PatchNextPatch, true);
        updateAfterPatchChange();
        return true;
    }
    // Panic
    else if (key == KeyPress(L'p') || key == KeyPress(L'P'))
    {
        mainPanel->getApplicationCommandManager()->invokeDirectly(MainPanel::EditPanic, true);
        return true;
    }
    // Toggle tuner
    else if (key == KeyPress::spaceKey)
    {
        tunerToggleButton->setToggleState(!tunerToggleButton->getToggleState(), sendNotification);
        return true;
    }

    return false;
}

//==============================================================================
String StageView::getNoteName(int midiNote) const
{
    if (midiNote < 0 || midiNote > 127)
        return "---";

    static const char* noteNames[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
    int octave = (midiNote / 12) - 1;
    int noteIndex = midiNote % 12;

    return String(noteNames[noteIndex]) + String(octave);
}

Colour StageView::getTuningColour(float cents) const
{
    float absCents = std::abs(cents);

    if (absCents < 5.0f)
        return Colour(0xFF00E676); // Green - in tune
    else if (absCents < 15.0f)
        return Colour(0xFFFFEB3B); // Yellow - close
    else
        return Colour(0xFFFF5252); // Red - out of tune
}

void StageView::updateAfterPatchChange()
{
    if (mainPanel != nullptr)
    {
        updatePatchInfo(mainPanel->getCurrentPatchName(), mainPanel->getCurrentPatch(), mainPanel->getPatchCount());
    }
}
