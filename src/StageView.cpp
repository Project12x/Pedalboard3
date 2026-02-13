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
#include "MasterGainState.h"
#include "SafetyLimiter.h"
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

    // Master gain sliders (larger for live use)
    inputGainSlider = std::make_unique<Slider>("stageInputGain");
    inputGainSlider->setSliderStyle(Slider::LinearBar);
    inputGainSlider->setRange(-60.0, 12.0, 0.1);
    inputGainSlider->setTextValueSuffix(" dB");
    inputGainSlider->setDoubleClickReturnValue(true, 0.0);
    inputGainSlider->setTooltip("Master Input Gain");
    inputGainSlider->textFromValueFunction = [](double v) { return "IN " + String(v, 1) + " dB"; };
    inputGainSlider->addListener(this);
    addAndMakeVisible(inputGainSlider.get());

    outputGainSlider = std::make_unique<Slider>("stageOutputGain");
    outputGainSlider->setSliderStyle(Slider::LinearBar);
    outputGainSlider->setRange(-60.0, 12.0, 0.1);
    outputGainSlider->setTextValueSuffix(" dB");
    outputGainSlider->setDoubleClickReturnValue(true, 0.0);
    outputGainSlider->setTooltip("Master Output Gain");
    outputGainSlider->textFromValueFunction = [](double v) { return "OUT " + String(v, 1) + " dB"; };
    outputGainSlider->addListener(this);
    addAndMakeVisible(outputGainSlider.get());

    // Sync initial values from MasterGainState
    {
        auto& gs = MasterGainState::getInstance();
        inputGainSlider->setValue(gs.masterInputGainDb.load(std::memory_order_relaxed), dontSendNotification);
        outputGainSlider->setValue(gs.masterOutputGainDb.load(std::memory_order_relaxed), dontSendNotification);
    }

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
void StageView::updatePatchInfo(const String& patchName, const String& nextPatchName, int currentIndex,
                                int totalPatches)
{
    this->currentPatchName = patchName;
    this->nextPatchName = nextPatchName;
    this->currentPatchIndex = currentIndex;
    this->totalPatchCount = totalPatches;
    repaint();
}

void StageView::setTunerProcessor(TunerProcessor* tuner)
{
    tunerProcessor = tuner;
}

//==============================================================================
void StageView::timerCallback()
{
    bool needsRepaint = false;

    if (tunerProcessor != nullptr && showTuner)
    {
        float targetCents = tunerProcessor->getCentsDeviation();
        displayedCents += (targetCents - displayedCents) * NEEDLE_SMOOTHING;

        float targetAngle = jlimit(-50.0f, 50.0f, displayedCents) * 0.9f;
        needleAngle += (targetAngle - needleAngle) * NEEDLE_SMOOTHING;

        detectedNote = tunerProcessor->getDetectedNote();
        needsRepaint = true;
    }

    // Update VU meter levels from SafetyLimiter
    if (auto* limiter = SafetyLimiterProcessor::getInstance())
    {
        for (int ch = 0; ch < 2; ++ch)
        {
            float inLevel = limiter->getInputLevel(ch);
            float outLevel = limiter->getOutputLevel(ch);
            if (std::abs(inLevel - cachedInputLevels[ch]) > 0.001f ||
                std::abs(outLevel - cachedOutputLevels[ch]) > 0.001f)
            {
                cachedInputLevels[ch] = inLevel;
                cachedOutputLevels[ch] = outLevel;
                needsRepaint = true;
            }
        }
    }

    // Sync master gain sliders from MasterGainState (when not being dragged)
    {
        auto& gs = MasterGainState::getInstance();
        if (inputGainSlider && !inputGainSlider->isMouseButtonDown())
        {
            float inDb = gs.masterInputGainDb.load(std::memory_order_relaxed);
            if (std::abs((float)inputGainSlider->getValue() - inDb) > 0.01f)
                inputGainSlider->setValue(inDb, dontSendNotification);
        }
        if (outputGainSlider && !outputGainSlider->isMouseButtonDown())
        {
            float outDb = gs.masterOutputGainDb.load(std::memory_order_relaxed);
            if (std::abs((float)outputGainSlider->getValue() - outDb) > 0.01f)
                outputGainSlider->setValue(outDb, dontSendNotification);
        }
    }

    if (needsRepaint)
        repaint();
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

    // Draw VU meters in footer area
    {
        auto& fonts = FontManager::getInstance();
        float footerY = (float)getHeight() - footerHeight;
        float meterH = 8.0f;
        float meterW = 140.0f;
        float labelW = 40.0f;
        float startX = 30.0f;

        // Helper lambda to draw a stereo VU meter
        auto drawVU = [&](float x, float y, const String& label, float level0, float level1)
        {
            g.setColour(Colours::white.withAlpha(0.6f));
            g.setFont(fonts.getUIFont(14.0f, true));
            g.drawText(label, x, y, labelW, 28.0f, Justification::centredRight);

            for (int ch = 0; ch < 2; ++ch)
            {
                float level = (ch == 0) ? level0 : level1;
                float my = y + ch * (meterH + 4.0f) + 4.0f;
                float mx = x + labelW + 6.0f;

                // Convert to dB scale
                float levelDb = (level > 0.001f) ? 20.0f * std::log10(level) : -60.0f;
                float normalized = jlimit(0.0f, 1.0f, (levelDb + 60.0f) / 60.0f);

                // Background
                g.setColour(Colour(0xFF2a2a3e));
                g.fillRoundedRectangle(mx, my, meterW, meterH, 3.0f);

                // Level bar
                if (normalized > 0.0f)
                {
                    Colour barColour;
                    if (level >= 1.0f)
                        barColour = Colour(0xFFFF5252);
                    else if (normalized > 0.75f)
                        barColour = Colour(0xFFFFEB3B);
                    else
                        barColour = Colour(0xFF00E676);

                    g.setColour(barColour);
                    g.fillRoundedRectangle(mx, my, meterW * normalized, meterH, 3.0f);
                }
            }
        };

        drawVU(startX, footerY + 4.0f, "IN", cachedInputLevels[0], cachedInputLevels[1]);
        drawVU(startX + labelW + meterW + 30.0f + 160.0f, footerY + 4.0f, "OUT",
               cachedOutputLevels[0], cachedOutputLevels[1]);
    }
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

    g.drawText(displayName, bounds.reduced(100, 0).withTrimmedBottom(40), Justification::centred);

    // Next Patch Preview
    if (nextPatchName.isNotEmpty())
    {
        g.setColour(Colours::white.withAlpha(0.5f));
        g.setFont(fonts.getUIFont(32.0f));
        g.drawText("NEXT: " + nextPatchName, bounds.removeFromBottom(140).withTrimmedBottom(60),
                   Justification::centredTop);
    }
    else
    {
        g.setColour(Colours::white.withAlpha(0.3f));
        g.setFont(fonts.getUIFont(24.0f));
        g.drawText("(End of Set)", bounds.removeFromBottom(140).withTrimmedBottom(60), Justification::centredTop);
    }

    // Patch position indicator
    if (totalPatchCount > 0)
    {
        g.setColour(Colours::white.withAlpha(0.5f));
        g.setFont(fonts.getMonoFont(24.0f));
        String posStr = String(currentPatchIndex + 1) + " / " + String(totalPatchCount);
        // Positioned slightly lower
        g.drawText(posStr, bounds.translated(0, 100), Justification::centred);
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

    // Master gain sliders in footer area (next to VU meters)
    {
        int footerY = bounds.getHeight() - 80;
        float labelW = 40.0f;
        float meterW = 140.0f;
        float startX = 30.0f;

        // Input slider below input VU
        int inSliderX = (int)(startX + labelW + 6.0f);
        inputGainSlider->setBounds(inSliderX, footerY + 30, (int)meterW, 28);

        // Output slider below output VU
        int outSliderX = (int)(startX + labelW + meterW + 30.0f + 160.0f + labelW + 6.0f);
        outputGainSlider->setBounds(outSliderX, footerY + 30, (int)meterW, 28);
    }
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

void StageView::sliderValueChanged(Slider* slider)
{
    auto& state = MasterGainState::getInstance();
    if (slider == inputGainSlider.get())
    {
        state.masterInputGainDb.store((float)slider->getValue(), std::memory_order_relaxed);
        state.saveToSettings();
    }
    else if (slider == outputGainSlider.get())
    {
        state.masterOutputGainDb.store((float)slider->getValue(), std::memory_order_relaxed);
        state.saveToSettings();
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
        // Fetch next patch name from MainPanel
        String nextName = "";
        int nextIndex = mainPanel->getCurrentPatch() + 1;
        if (nextIndex < mainPanel->getPatchCount())
        {
            // We need a way to get the name of a specific patch index
            // Since MainPanel doesn't expose this yet, we will rely on a new method or fetch it differently
            // For now, let's assume MainPanel handles the push, but here we are pulling...
            // Actually, updatePatchInfo is PUSHED by MainPanel.
            // But this method 'updateAfterPatchChange' is called by StageView itself on key press.
            // So we need to request an update from MainPanel.
            mainPanel->updateStageView();
        }
    }
}
