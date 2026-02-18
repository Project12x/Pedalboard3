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
    prevButton->setColour(TextButton::textColourOffId, colours["Text Colour"]);
    addAndMakeVisible(prevButton.get());

    nextButton = std::make_unique<TextButton>("NEXT >>");
    nextButton->addListener(this);
    nextButton->setColour(TextButton::buttonColourId, colours["Plugin Border"].darker(0.2f));
    nextButton->setColour(TextButton::textColourOffId, colours["Text Colour"]);
    addAndMakeVisible(nextButton.get());

    // Panic button
    panicButton = std::make_unique<TextButton>("PANIC");
    panicButton->addListener(this);
    panicButton->setColour(TextButton::buttonColourId, Colours::darkred);
    panicButton->setColour(TextButton::textColourOffId, colours["Text Colour"]);
    addAndMakeVisible(panicButton.get());

    // Exit button
    exitButton = std::make_unique<TextButton>("EXIT");
    exitButton->addListener(this);
    exitButton->setColour(TextButton::buttonColourId, colours["Plugin Border"].darker(0.3f));
    exitButton->setColour(TextButton::textColourOffId, colours["Text Colour"].withAlpha(0.8f));
    addAndMakeVisible(exitButton.get());

    // Tuner toggle
    tunerToggleButton = std::make_unique<TextButton>("TUNER");
    tunerToggleButton->addListener(this);
    tunerToggleButton->setClickingTogglesState(true);
    tunerToggleButton->setToggleState(true, dontSendNotification);
    tunerToggleButton->setColour(TextButton::buttonColourId, colours["Plugin Border"].darker(0.2f));
    tunerToggleButton->setColour(TextButton::buttonOnColourId, colours["Tuner Active Colour"]);
    tunerToggleButton->setColour(TextButton::textColourOffId, colours["Text Colour"].withAlpha(0.7f));
    tunerToggleButton->setColour(TextButton::textColourOnId, colours["Text Colour"]);
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
            // VU ballistic levels for smooth meter bar
            float inVuLevel = limiter->getInputVuLevel(ch);
            float outVuLevel = limiter->getOutputVuLevel(ch);
            if (std::abs(inVuLevel - cachedInputLevels[ch]) > 0.001f ||
                std::abs(outVuLevel - cachedOutputLevels[ch]) > 0.001f)
            {
                cachedInputLevels[ch] = inVuLevel;
                cachedOutputLevels[ch] = outVuLevel;
                needsRepaint = true;
            }

            // Peak levels for peak hold indicator (sharp, instantaneous)
            float inPeak = limiter->getInputLevel(ch);
            float outPeak = limiter->getOutputLevel(ch);
            cachedInputPeakLevels[ch] = inPeak;
            cachedOutputPeakLevels[ch] = outPeak;

            // Update peak hold for input (from peak, not VU)
            float inDb = (inPeak > 0.001f) ? 20.0f * std::log10(inPeak) : -60.0f;
            float inNorm = jlimit(0.0f, 1.0f, (inDb + 60.0f) / 60.0f);
            if (inNorm >= peakHoldInput[ch])
            {
                peakHoldInput[ch] = inNorm;
                peakHoldInputCounters[ch] = 60;
            }
            else if (peakHoldInputCounters[ch] > 0)
                --peakHoldInputCounters[ch];
            else
            {
                peakHoldInput[ch] *= 0.92f;
                if (peakHoldInput[ch] < 0.01f)
                    peakHoldInput[ch] = 0.0f;
            }

            // Update peak hold for output (from peak, not VU)
            float outDb = (outPeak > 0.001f) ? 20.0f * std::log10(outPeak) : -60.0f;
            float outNorm = jlimit(0.0f, 1.0f, (outDb + 60.0f) / 60.0f);
            if (outNorm >= peakHoldOutput[ch])
            {
                peakHoldOutput[ch] = outNorm;
                peakHoldOutputCounters[ch] = 60;
            }
            else if (peakHoldOutputCounters[ch] > 0)
                --peakHoldOutputCounters[ch];
            else
            {
                peakHoldOutput[ch] *= 0.92f;
                if (peakHoldOutput[ch] < 0.01f)
                    peakHoldOutput[ch] = 0.0f;
            }
        }
    }

    // Always repaint if peak hold indicators are active
    if (peakHoldInput[0] > 0.0f || peakHoldInput[1] > 0.0f || peakHoldOutput[0] > 0.0f || peakHoldOutput[1] > 0.0f)
        needsRepaint = true;

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
    g.setGradientFill(ColourGradient(colours["Stage Background Top"], 0, 0, colours["Stage Background Bottom"], 0,
                                     bounds.getHeight(), false));
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
        float meterH = 10.0f;
        float meterW = 140.0f;
        float labelW = 40.0f;
        float startX = 30.0f;

        const Colour colGreen = colours["VU Meter Lower Colour"].withAlpha(1.0f);
        const Colour colYellow = colours["VU Meter Upper Colour"].withAlpha(1.0f);
        const Colour colRed = colours["VU Meter Over Colour"].withAlpha(1.0f);

        // Helper lambda to draw a stereo VU meter with gradient, peak hold, glow, and tick marks
        auto drawVU = [&](float x, float y, const String& label, float level0, float level1, const float* peakHold,
                          const int* peakCounters)
        {
            g.setColour(colours["Text Colour"].withAlpha(0.6f));
            g.setFont(fonts.getUIFont(14.0f, true));
            g.drawText(label, x, y, labelW, 32.0f, Justification::centredRight);

            for (int ch = 0; ch < 2; ++ch)
            {
                float level = (ch == 0) ? level0 : level1;
                float my = y + ch * (meterH + 4.0f) + 4.0f;
                float mx = x + labelW + 6.0f;

                float levelDb = (level > 0.001f) ? 20.0f * std::log10(level) : -60.0f;
                float normalized = jlimit(0.0f, 1.0f, (levelDb + 60.0f) / 60.0f);

                // Background
                g.setColour(colours["Stage Panel Background"]);
                g.fillRoundedRectangle(mx, my, meterW, meterH, 3.0f);

                // Gradient-filled level bar
                if (normalized > 0.0f)
                {
                    float barWidth = meterW * normalized;

                    // Glow effect when hot
                    if (normalized > 0.9f)
                    {
                        float glowAlpha = (normalized - 0.9f) * 3.0f;
                        Colour glowCol =
                            (level >= 1.0f) ? colRed.withAlpha(glowAlpha) : Colours::orange.withAlpha(glowAlpha * 0.7f);
                        g.setColour(glowCol);
                        g.fillRoundedRectangle(mx - 1.0f, my - 1.0f, barWidth + 2.0f, meterH + 2.0f, 4.0f);
                    }

                    // Green-to-yellow-to-red gradient
                    ColourGradient gradient(colGreen, mx, my, colRed, mx + meterW, my, false);
                    gradient.addColour(0.65, colYellow);
                    g.setGradientFill(gradient);

                    g.saveState();
                    g.reduceClipRegion(Rectangle<int>((int)mx, (int)my, (int)(barWidth + 1.0f), (int)(meterH + 1.0f)));
                    g.fillRoundedRectangle(mx, my, meterW, meterH, 3.0f);
                    g.restoreState();
                }

                // Peak hold indicator
                if (peakHold[ch] > 0.01f)
                {
                    float peakX = mx + meterW * peakHold[ch];
                    Colour peakCol = (peakHold[ch] > 0.95f)   ? colRed
                                     : (peakHold[ch] > 0.65f) ? colYellow
                                                              : colGreen.brighter(0.3f);
                    float alpha = (peakCounters[ch] > 0) ? 1.0f : jmax(0.3f, peakHold[ch]);
                    g.setColour(peakCol.withAlpha(alpha));
                    g.fillRect(peakX - 1.5f, my, 3.0f, meterH);
                }

                // dB scale tick marks
                g.setColour(Colours::white.withAlpha(0.12f));
                const float dbMarks[] = {-48.0f, -24.0f, -12.0f, -6.0f, -3.0f, 0.0f};
                for (float db : dbMarks)
                {
                    float tickNorm = (db + 60.0f) / 60.0f;
                    float tickX = mx + meterW * tickNorm;
                    g.drawVerticalLine((int)tickX, my, my + meterH);
                }
            }
        };

        drawVU(startX, footerY + 4.0f, "IN", cachedInputLevels[0], cachedInputLevels[1], peakHoldInput,
               peakHoldInputCounters);
        drawVU(startX + labelW + meterW + 30.0f + 160.0f, footerY + 4.0f, "OUT", cachedOutputLevels[0],
               cachedOutputLevels[1], peakHoldOutput, peakHoldOutputCounters);
    }
}

void StageView::drawStatusBar(Graphics& g, Rectangle<float> bounds)
{
    auto& fonts = FontManager::getInstance();
    auto& colours = ColourScheme::getInstance().colours;

    // "STAGE MODE" title
    g.setColour(colours["Text Colour"].withAlpha(0.5f));
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
    auto& colours = ColourScheme::getInstance().colours;
    auto centre = bounds.getCentre();

    // Large patch name
    g.setColour(colours["Text Colour"]);
    g.setFont(fonts.getUIFont(72.0f, true));

    // Truncate long names
    String displayName = currentPatchName;
    if (displayName.length() > 25)
        displayName = displayName.substring(0, 22) + "...";

    g.drawText(displayName, bounds.reduced(100, 0).withTrimmedBottom(40), Justification::centred);

    // Next Patch Preview
    if (nextPatchName.isNotEmpty())
    {
        g.setColour(colours["Text Colour"].withAlpha(0.5f));
        g.setFont(fonts.getUIFont(32.0f));
        g.drawText("NEXT: " + nextPatchName, bounds.removeFromBottom(140).withTrimmedBottom(60),
                   Justification::centredTop);
    }
    else
    {
        g.setColour(colours["Text Colour"].withAlpha(0.3f));
        g.setFont(fonts.getUIFont(24.0f));
        g.drawText("(End of Set)", bounds.removeFromBottom(140).withTrimmedBottom(60), Justification::centredTop);
    }

    // Patch position indicator
    if (totalPatchCount > 0)
    {
        g.setColour(colours["Text Colour"].withAlpha(0.5f));
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
        g.setColour(colours["Text Colour"].withAlpha(0.25f));
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
    g.setColour(colours["Text Colour"].withAlpha(0.5f));
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
    auto& colours = ColourScheme::getInstance().colours;
    float absCents = std::abs(cents);

    if (absCents < 5.0f)
        return colours["VU Meter Lower Colour"].withAlpha(1.0f);
    else if (absCents < 15.0f)
        return colours["VU Meter Upper Colour"].withAlpha(1.0f);
    else
        return colours["VU Meter Over Colour"].withAlpha(1.0f);
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
