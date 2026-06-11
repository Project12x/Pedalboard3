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
#include "StageLayout.h"
#include "ThemeSwitcherComponent.h"
#include "TunerProcessor.h"

namespace
{
class StageButtonLookAndFeel : public LookAndFeel_V4
{
  public:
    void drawButtonBackground(Graphics& g, Button& button, const Colour& backgroundColour, bool isMouseOverButton,
                              bool isButtonDown) override
    {
        auto& palette = ::ColourScheme::getInstance().colours;
        auto bounds = button.getLocalBounds().toFloat().reduced(2.0f);
        const bool active = button.getToggleState();
        const bool panic = button.getName().containsIgnoreCase("panic");
        const bool nav = button.getButtonText().contains("PREV") || button.getButtonText().contains("NEXT");
        const bool utility = button.getButtonText().equalsIgnoreCase("EXIT") ||
                             button.getButtonText().equalsIgnoreCase("TUNER");

        if (isButtonDown)
            bounds = bounds.translated(0.0f, 1.0f);

        const auto radius = juce::jlimit(9.0f, 15.0f, bounds.getHeight() * 0.28f);
        const auto shadow = palette["Window Background"].darker(0.72f).withAlpha(isButtonDown ? 0.10f : 0.30f);
        g.setColour(shadow);
        g.fillRoundedRectangle(bounds.translated(0.0f, isButtonDown ? 1.0f : 2.5f), radius);

        if (active && !panic)
        {
            g.setColour(palette["Accent Colour"].withAlpha(0.14f));
            g.fillRoundedRectangle(bounds.expanded(2.0f), radius + 2.0f);
        }

        if (panic)
        {
            ColourGradient panicFill(palette["Danger Colour"].brighter(isMouseOverButton ? 0.18f : 0.08f),
                                     bounds.getX(), bounds.getY(), palette["Danger Colour"].darker(0.18f),
                                     bounds.getX(), bounds.getBottom(), false);
            g.setGradientFill(panicFill);
            g.fillRoundedRectangle(bounds, radius);
            g.setColour(palette["Danger Colour"].brighter(0.35f).withAlpha(0.78f));
            g.drawRoundedRectangle(bounds.reduced(0.5f), radius, 1.4f);
            g.setColour(juce::Colours::white.withAlpha(0.20f));
            g.drawLine(bounds.getX() + 8.0f, bounds.getY() + 3.0f, bounds.getRight() - 8.0f,
                       bounds.getY() + 3.0f, 1.0f);
            return;
        }

        auto base = active ? palette["Accent Colour"].withAlpha(0.22f)
                           : palette["Stage Panel Background"].withAlpha(utility ? 0.58f : 0.48f);
        if (nav)
            base = palette["Plugin Border"].withAlpha(active ? 0.52f : 0.42f);
        if (isMouseOverButton)
            base = base.brighter(0.10f);

        ColourGradient fill(base.brighter(active ? 0.18f : 0.10f), bounds.getX(), bounds.getY(),
                            base.darker(0.16f), bounds.getX(), bounds.getBottom(), false);
        g.setGradientFill(fill);
        g.fillRoundedRectangle(bounds, radius);

        g.setColour(palette["Text Colour"].withAlpha(0.10f));
        g.drawLine(bounds.getX() + 7.0f, bounds.getY() + 2.0f, bounds.getRight() - 7.0f, bounds.getY() + 2.0f, 1.0f);
        g.setColour((active ? palette["Accent Colour"] : palette["Plugin Border"]).withAlpha(active ? 0.82f : 0.46f));
        g.drawRoundedRectangle(bounds.reduced(0.5f), radius, active ? 1.5f : 1.0f);
    }

    void drawButtonText(Graphics& g, TextButton& button, bool /*isMouseOverButton*/, bool /*isButtonDown*/) override
    {
        const bool active = button.getToggleState();
        const bool panic = button.getName().containsIgnoreCase("panic");
        const float fontHeight = juce::jlimit(11.0f, 16.5f, button.getHeight() * 0.31f);
        auto textColour = panic ? juce::Colours::white
                                : button.findColour(active ? TextButton::textColourOnId : TextButton::textColourOffId);

        g.setFont(::FontManager::getInstance().getDisplayFont(fontHeight));
        g.setColour(textColour.withAlpha(button.isEnabled() ? (active || panic ? 0.96f : 0.78f) : 0.36f));
        g.drawFittedText(button.getButtonText(), button.getLocalBounds().reduced(9, 2), Justification::centred, 1);
    }
};

struct StagePatchLabel
{
    String title;
    String tone;
};

StagePatchLabel splitPatchLabel(const String& rawName)
{
    StagePatchLabel result;
    result.title = rawName.trim();

    const int dash = result.title.indexOf(" - ");
    if (dash > 0)
    {
        result.tone = result.title.substring(dash + 3).trim();
        result.title = result.title.substring(0, dash).trim();
    }

    return result;
}

StageButtonLookAndFeel stageButtonLookAndFeel;
} // namespace

//==============================================================================
StageView::StageView(MainPanel* panel) : mainPanel(panel)
{
    auto& colours = ColourScheme::getInstance().colours;

    // Ensure this component is opaque (draws its entire area)
    setOpaque(true);

    // Navigation buttons - using ASCII-safe labels
    prevButton = std::make_unique<TextButton>("<< PREV");
    prevButton->addListener(this);
    prevButton->setLookAndFeel(&stageButtonLookAndFeel);
    prevButton->setColour(TextButton::buttonColourId, colours["Plugin Border"].darker(0.2f));
    prevButton->setColour(TextButton::textColourOffId, colours["Text Colour"]);
    addAndMakeVisible(prevButton.get());

    nextButton = std::make_unique<TextButton>("NEXT >>");
    nextButton->addListener(this);
    nextButton->setLookAndFeel(&stageButtonLookAndFeel);
    nextButton->setColour(TextButton::buttonColourId, colours["Plugin Border"].darker(0.2f));
    nextButton->setColour(TextButton::textColourOffId, colours["Text Colour"]);
    addAndMakeVisible(nextButton.get());

    // Panic button
    panicButton = std::make_unique<TextButton>("PANIC");
    panicButton->addListener(this);
    panicButton->setLookAndFeel(&stageButtonLookAndFeel);
    panicButton->setColour(TextButton::buttonColourId, colours["Danger Colour"].darker(0.2f));
    panicButton->setColour(TextButton::textColourOffId, colours["Text Colour"]);
    addAndMakeVisible(panicButton.get());

    // Exit button
    exitButton = std::make_unique<TextButton>("EXIT");
    exitButton->addListener(this);
    exitButton->setLookAndFeel(&stageButtonLookAndFeel);
    exitButton->setColour(TextButton::buttonColourId, colours["Plugin Border"].darker(0.3f));
    exitButton->setColour(TextButton::textColourOffId, colours["Text Colour"].withAlpha(0.8f));
    addAndMakeVisible(exitButton.get());

    // Tuner toggle
    tunerToggleButton = std::make_unique<TextButton>("TUNER");
    tunerToggleButton->addListener(this);
    tunerToggleButton->setClickingTogglesState(true);
    tunerToggleButton->setToggleState(true, dontSendNotification);
    tunerToggleButton->setLookAndFeel(&stageButtonLookAndFeel);
    tunerToggleButton->setColour(TextButton::buttonColourId, colours["Plugin Border"].darker(0.2f));
    tunerToggleButton->setColour(TextButton::buttonOnColourId, colours["Tuner Active Colour"]);
    tunerToggleButton->setColour(TextButton::textColourOffId, colours["Text Colour"].withAlpha(0.7f));
    tunerToggleButton->setColour(TextButton::textColourOnId, colours["Text Colour"]);
    addAndMakeVisible(tunerToggleButton.get());

    auto makeViewButton = [&](const String& text)
    {
        auto button = std::make_unique<TextButton>(text);
        button->setClickingTogglesState(true);
        button->setRadioGroupId(37);
        button->setLookAndFeel(&stageButtonLookAndFeel);
        button->setColour(TextButton::buttonColourId, colours["Plugin Border"].darker(0.24f));
        button->setColour(TextButton::buttonOnColourId, colours["Accent Colour"].withAlpha(0.82f));
        button->setColour(TextButton::textColourOffId, colours["Text Colour"].withAlpha(0.72f));
        button->setColour(TextButton::textColourOnId, colours["Text Colour"]);
        button->addListener(this);
        addAndMakeVisible(button.get());
        return button;
    };

    patchViewButton = makeViewButton("HERO");
    queueViewButton = makeViewButton("SETLIST");
    gridViewButton = makeViewButton("GRID");
    tunerViewButton = makeViewButton("TUNE");
    syncViewButtons();

    themeSwitcher = std::make_unique<ThemeSwitcherComponent>(
        [this](const String& presetName)
        {
            if (mainPanel != nullptr)
                mainPanel->applyColourSchemePreset(presetName);
        });
    addAndMakeVisible(themeSwitcher.get());

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
    prevButton->setLookAndFeel(nullptr);
    nextButton->setLookAndFeel(nullptr);
    panicButton->setLookAndFeel(nullptr);
    exitButton->setLookAndFeel(nullptr);
    tunerToggleButton->setLookAndFeel(nullptr);
    patchViewButton->setLookAndFeel(nullptr);
    queueViewButton->setLookAndFeel(nullptr);
    gridViewButton->setLookAndFeel(nullptr);
    tunerViewButton->setLookAndFeel(nullptr);
}

//==============================================================================
void StageView::updatePatchInfo(const String& patchName, const String& previousPatchNameToUse,
                                const String& nextPatchNameToUse, int currentIndex, int totalPatches,
                                const StringArray& patchNamesToUse)
{
    this->currentPatchName = patchName;
    this->previousPatchName = previousPatchNameToUse;
    this->nextPatchName = nextPatchNameToUse;
    this->currentPatchIndex = currentIndex;
    this->totalPatchCount = totalPatches;
    this->patchNames = patchNamesToUse;
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
    const bool tunerFocus = viewMode == ViewMode::Tuner;
    const bool reserveTunerStrip =
        StageLayout::shouldReserveTunerStrip(showTuner, tunerFocus, viewMode == ViewMode::Patch);
    const auto metrics = StageLayout::calculateMetrics(getWidth(), getHeight(), reserveTunerStrip);

    // Dark background with subtle gradient
    g.setGradientFill(ColourGradient(colours["Stage Background Top"], 0, 0, colours["Stage Background Bottom"], 0,
                                     bounds.getHeight(), false));
    g.fillAll();

    g.setColour(colours["Plugin Border"].withAlpha(0.045f));
    for (int x = metrics.gridSpacing; x < getWidth(); x += metrics.gridSpacing)
        g.drawVerticalLine(x, 0.0f, (float)getHeight());
    for (int y = metrics.gridSpacing; y < getHeight(); y += metrics.gridSpacing)
        g.drawHorizontalLine(y, 0.0f, (float)getWidth());

    // Layout areas
    auto headerArea = bounds.removeFromTop((float)metrics.headerHeight);
    auto footerArea = bounds.removeFromBottom((float)metrics.footerHeight);
    auto mainArea = bounds;

    // Draw sections
    drawStatusBar(g, headerArea);
    if (tunerFocus)
    {
        drawTunerDisplay(g, mainArea);
    }
    else
    {
        Rectangle<float> tunerArea;
        if (reserveTunerStrip)
            tunerArea = mainArea.removeFromBottom((float)metrics.tunerHeight);

        if (viewMode == ViewMode::Queue)
            drawQueueFocus(g, mainArea);
        else if (viewMode == ViewMode::Grid)
            drawGridView(g, mainArea);
        else
            drawPatchDisplay(g, mainArea);

        if (reserveTunerStrip)
            drawTunerDisplay(g, tunerArea);
    }

    drawSafetyBar(g, footerArea);

    // Draw VU meters in footer area
    {
        auto& fonts = FontManager::getInstance();
        const float footerY = (float)getHeight() - (float)metrics.footerHeight;
        const float meterH = metrics.meterHeight;
        const float meterW = metrics.meterWidth;
        const float labelW = metrics.meterLabelWidth;
        const float startX = metrics.meterStartX;

        const Colour colGreen = colours["VU Meter Lower Colour"].withAlpha(1.0f);
        const Colour colYellow = colours["VU Meter Upper Colour"].withAlpha(1.0f);
        const Colour colRed = colours["VU Meter Over Colour"].withAlpha(1.0f);

        // Helper lambda to draw a stereo VU meter with gradient, peak hold, glow, and tick marks
        auto drawVU = [&](float x, float y, const String& label, float level0, float level1, const float* peakHold,
                          const int* peakCounters)
        {
            g.setColour(colours["Text Colour"].withAlpha(0.6f));
            g.setFont(fonts.getDisplayFont(metrics.statusFontHeight));
            g.drawText(label, x, y, labelW, metrics.footerHeight * 0.42f, Justification::centredRight);

            for (int ch = 0; ch < 2; ++ch)
            {
                float level = (ch == 0) ? level0 : level1;
                float my = y + ch * (meterH + metrics.meterChannelGap) + metrics.meterChannelGap;
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
                        Colour glowCol = (level >= 1.0f) ? colRed.withAlpha(glowAlpha)
                                                         : colours["Warning Colour"].withAlpha(glowAlpha * 0.7f);
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
                g.setColour(colours["Text Colour"].withAlpha(0.12f));
                const float dbMarks[] = {-48.0f, -24.0f, -12.0f, -6.0f, -3.0f, 0.0f};
                for (float db : dbMarks)
                {
                    float tickNorm = (db + 60.0f) / 60.0f;
                    float tickX = mx + meterW * tickNorm;
                    g.drawVerticalLine((int)tickX, my, my + meterH);
                }
            }
        };

        drawVU(startX, footerY + metrics.meterTopOffset, "IN", cachedInputLevels[0], cachedInputLevels[1], peakHoldInput,
               peakHoldInputCounters);
        drawVU(startX + labelW + meterW + metrics.meterSpacing + metrics.panicButtonWidth,
               footerY + metrics.meterTopOffset, "OUT", cachedOutputLevels[0], cachedOutputLevels[1], peakHoldOutput,
               peakHoldOutputCounters);
    }
}

void StageView::drawStatusBar(Graphics& g, Rectangle<float> bounds)
{
    auto& fonts = FontManager::getInstance();
    auto& colours = ColourScheme::getInstance().colours;
    const auto metrics = StageLayout::calculateMetrics(getWidth(), getHeight(), showTuner);

    g.setColour(colours["Stage Panel Background"].withAlpha(0.34f));
    g.fillRect(bounds);

    g.setColour(colours["Plugin Border"].withAlpha(0.45f));
    g.drawHorizontalLine(juce::roundToInt(bounds.getBottom()) - 1, bounds.getX(), bounds.getRight());

    auto leftArea = bounds.reduced((float)metrics.margin, 0.0f)
                        .withWidth(juce::jmax(220.0f, bounds.getWidth() * 0.24f));
    const auto dotSize = metrics.liveDotSize;
    const auto dotArea =
        Rectangle<float>(leftArea.getX(), leftArea.getCentreY() - dotSize * 0.5f, dotSize, dotSize);

    g.setColour(colours["Accent Colour"].withAlpha(0.18f));
    g.fillEllipse(dotArea.expanded(dotSize * 0.65f));
    g.setColour(colours["Accent Colour"]);
    g.fillEllipse(dotArea);

    g.setColour(colours["Text Colour"].withAlpha(0.58f));
    g.setFont(fonts.getDisplayFont(metrics.statusFontHeight));
    g.drawText("STAGE MODE", leftArea.withTrimmedLeft(dotSize + 12.0f), Justification::centredLeft);

    if (patchViewButton != nullptr && queueViewButton != nullptr && gridViewButton != nullptr &&
        tunerViewButton != nullptr)
    {
        auto modeRail = patchViewButton->getBounds()
                            .getUnion(queueViewButton->getBounds())
                            .getUnion(gridViewButton->getBounds())
                            .getUnion(tunerViewButton->getBounds())
                            .expanded(4, 3)
                            .toFloat();
        g.setColour(colours["Stage Panel Background"].withAlpha(0.42f));
        g.fillRoundedRectangle(modeRail, 12.0f);
        g.setColour(colours["Window Background"].darker(0.55f).withAlpha(0.18f));
        g.drawRoundedRectangle(modeRail.reduced(1.0f), 11.0f, 1.0f);
        g.setColour(colours["Plugin Border"].withAlpha(0.44f));
        g.drawRoundedRectangle(modeRail.reduced(0.5f), 12.0f, 1.0f);
    }

    Time now = Time::getCurrentTime();
    String timeStr = now.formatted("%H:%M");
    auto timeArea = bounds.reduced((float)metrics.margin, 0.0f);
    timeArea.removeFromRight((float)(metrics.utilityButtonWidth * 2 + metrics.margin * 2));
    g.setFont(fonts.getMonoDisplayFont(metrics.timeFontHeight));
    g.setColour(colours["Text Colour"].withAlpha(0.58f));
    g.drawText(timeStr, timeArea, Justification::centredRight);
}

void StageView::drawPatchDisplay(Graphics& g, Rectangle<float> bounds)
{
    auto& fonts = FontManager::getInstance();
    auto& colours = ColourScheme::getInstance().colours;
    const auto metrics = StageLayout::calculateMetrics(getWidth(), getHeight(), showTuner);

    auto content = bounds.reduced((float)metrics.heroHorizontalInset, (float)metrics.margin * 0.45f);
    if (content.getWidth() < 260.0f)
        content = bounds.reduced((float)metrics.margin * 2.0f, (float)metrics.margin * 0.45f);

    const auto label = splitPatchLabel(currentPatchName.isNotEmpty() ? currentPatchName : String("No Patch"));
    const auto titleText = StageLayout::elideLabel(label.title, metrics.patchNameMaxChars);
    const auto toneText = StageLayout::elideLabel(label.tone, metrics.nextPatchMaxChars);

    auto eyebrowArea = content.removeFromTop((float)metrics.heroEyebrowHeight);
    auto eyebrowPill = eyebrowArea.withSizeKeepingCentre(juce::jmin(eyebrowArea.getWidth() * 0.7f, 520.0f),
                                                         eyebrowArea.getHeight() - 4.0f);
    g.setColour(colours["Text Colour"].withAlpha(0.11f));
    g.fillRoundedRectangle(eyebrowPill, 10.0f);

    const auto positionText = totalPatchCount > 0 ? String(currentPatchIndex + 1) + " / " + String(totalPatchCount)
                                                  : String("0 / 0");
    auto nowArea = eyebrowPill.reduced(16.0f, 0.0f);
    auto posArea = nowArea.removeFromRight(juce::jmax(92.0f, metrics.positionFontHeight * 5.2f));
    nowArea.removeFromRight(18.0f);

    g.setColour(colours["Accent Colour"]);
    g.setFont(fonts.getDisplayFont(metrics.eyebrowFontHeight));
    g.drawText("NOW PLAYING", nowArea, Justification::centredRight);

    g.setColour(colours["Text Colour"].withAlpha(0.72f));
    g.setFont(fonts.getMonoDisplayFont(metrics.positionFontHeight));
    g.drawText(positionText, posArea, Justification::centredLeft);

    auto lowerArea = content.removeFromBottom((float)metrics.heroNextCueHeight + metrics.progressDotSize * 3.0f);
    auto titleStack = content.reduced(0.0f, juce::jmax(0.0f, content.getHeight() * 0.08f));
    auto toneArea = titleStack.removeFromBottom(label.tone.isNotEmpty() ? metrics.nextPatchFontHeight * 1.25f : 0.0f);

    g.setColour(colours["Text Colour"]);
    g.setFont(fonts.getDisplayFont(metrics.patchNameFontHeight));
    g.drawFittedText(titleText, titleStack.toNearestInt(), Justification::centred, 2);

    if (label.tone.isNotEmpty())
    {
        g.setColour(colours["Accent Colour"].withAlpha(0.9f));
        g.setFont(fonts.getDisplayFont(metrics.nextPatchFontHeight));
        g.drawText(toneText, toneArea, Justification::centred);
    }

    auto nextCueArea = lowerArea.removeFromTop((float)metrics.heroNextCueHeight).reduced(24.0f, 4.0f);

    if (nextPatchName.isNotEmpty())
    {
        g.setColour(colours["Warning Colour"].withAlpha(0.12f));
        g.fillRoundedRectangle(nextCueArea, 12.0f);
        g.setColour(colours["Warning Colour"].withAlpha(0.5f));
        g.drawRoundedRectangle(nextCueArea, 12.0f, 1.0f);
        g.setColour(colours["Text Colour"].withAlpha(0.72f));
        g.setFont(fonts.getDisplayFont(metrics.nextPatchFontHeight));
        g.drawText("NEXT  " + StageLayout::elideLabel(nextPatchName, metrics.nextPatchMaxChars), nextCueArea,
                   Justification::centred);
    }
    else
    {
        g.setColour(colours["Stage Panel Background"].withAlpha(0.28f));
        g.fillRoundedRectangle(nextCueArea, 12.0f);
        g.setColour(colours["Text Colour"].withAlpha(0.38f));
        g.setFont(fonts.getDisplayFont(metrics.nextPatchFontHeight));
        g.drawText("END OF SET", nextCueArea, Justification::centred);
    }

    drawPatchProgress(g, lowerArea);

    if (bounds.getWidth() >= 980.0f)
    {
        auto drawPeek = [&](Rectangle<float> peek, const String& meta, const String& name, bool rightAligned)
        {
            if (name.isEmpty() || peek.getWidth() < 130.0f)
                return;

            g.setColour(colours["Stage Panel Background"].withAlpha(0.18f));
            g.fillRoundedRectangle(peek, 14.0f);
            g.setColour(colours["Plugin Border"].withAlpha(0.28f));
            g.drawRoundedRectangle(peek.reduced(0.5f), 14.0f, 1.0f);

            auto body = peek.reduced(16.0f, 10.0f);
            g.setColour(colours["Accent Colour"].withAlpha(0.62f));
            g.setFont(fonts.getDisplayFont(metrics.queueLabelFontHeight));
            g.drawText(meta, body.removeFromTop(metrics.queueLabelFontHeight + 6.0f),
                       rightAligned ? Justification::centredRight : Justification::centredLeft);

            const auto parsed = splitPatchLabel(name);
            const int maxChars = juce::jlimit(12, 24, juce::roundToInt(body.getWidth() / 11.0f));
            g.setColour(colours["Text Colour"].withAlpha(0.58f));
            g.setFont(fonts.getDisplayFont(metrics.nextPatchFontHeight));
            g.drawFittedText(StageLayout::elideLabel(parsed.title, maxChars), body.toNearestInt(),
                             rightAligned ? Justification::centredRight : Justification::centredLeft, 2);
        };

        const auto peekWidth = juce::jlimit(150.0f, 300.0f, bounds.getWidth() * 0.16f);
        const auto peekHeight = juce::jlimit(84.0f, 128.0f, bounds.getHeight() * 0.18f);
        const auto peekY = bounds.getCentreY() - peekHeight * 0.5f;
        drawPeek({bounds.getX() + (float)metrics.margin * 1.4f, peekY, peekWidth, peekHeight}, "PREV",
                 previousPatchName, false);
        drawPeek({bounds.getRight() - peekWidth - (float)metrics.margin * 1.4f, peekY, peekWidth, peekHeight}, "NEXT",
                 nextPatchName, true);
    }
}

void StageView::drawPatchProgress(Graphics& g, Rectangle<float> bounds)
{
    if (totalPatchCount <= 1)
        return;

    auto& colours = ColourScheme::getInstance().colours;
    const auto metrics = StageLayout::calculateMetrics(getWidth(), getHeight(), showTuner);
    const auto activeIndex = juce::jlimit(0, juce::jmax(0, totalPatchCount - 1), currentPatchIndex);

    auto progressArea = bounds.withHeight(metrics.progressDotSize * 2.4f).withCentre(bounds.getCentre());

    if (totalPatchCount > metrics.maxProgressDots)
    {
        const auto trackWidth = juce::jmin(progressArea.getWidth() * 0.42f, 280.0f);
        const auto trackHeight = juce::jmax(5.0f, metrics.progressDotSize * 0.42f);
        const auto track = Rectangle<float>(progressArea.getCentreX() - trackWidth * 0.5f,
                                            progressArea.getCentreY() - trackHeight * 0.5f, trackWidth, trackHeight);
        const auto progress = totalPatchCount > 1 ? (float)activeIndex / (float)(totalPatchCount - 1) : 0.0f;

        g.setColour(colours["Plugin Border"].withAlpha(0.28f));
        g.fillRoundedRectangle(track, trackHeight * 0.5f);
        g.setColour(colours["Accent Colour"]);
        g.fillRoundedRectangle(track.withWidth(track.getWidth() * progress), trackHeight * 0.5f);
        return;
    }

    float totalWidth = 0.0f;
    for (int i = 0; i < totalPatchCount; ++i)
        totalWidth += (i == activeIndex ? metrics.progressActiveWidth : metrics.progressDotSize);
    totalWidth += (float)(totalPatchCount - 1) * metrics.progressDotGap;

    auto x = progressArea.getCentreX() - totalWidth * 0.5f;
    const auto y = progressArea.getCentreY() - metrics.progressDotSize * 0.5f;

    for (int i = 0; i < totalPatchCount; ++i)
    {
        const auto isActive = i == activeIndex;
        const auto isPast = i < activeIndex;
        const auto dotWidth = isActive ? metrics.progressActiveWidth : metrics.progressDotSize;
        const auto dot = Rectangle<float>(x, y, dotWidth, metrics.progressDotSize);

        if (isActive)
        {
            g.setColour(colours["Accent Colour"]);
            g.fillRoundedRectangle(dot, metrics.progressDotSize * 0.5f);
        }
        else if (isPast)
        {
            g.setColour(colours["Text Colour"].withAlpha(0.32f));
            g.fillEllipse(dot.withWidth(metrics.progressDotSize));
        }
        else
        {
            g.setColour(colours["Plugin Border"].withAlpha(0.55f));
            g.drawEllipse(dot.withWidth(metrics.progressDotSize), 1.2f);
        }

        x += dotWidth + metrics.progressDotGap;
    }
}

void StageView::drawLiveQueue(Graphics& g, Rectangle<float> bounds)
{
    auto& fonts = FontManager::getInstance();
    auto& colours = ColourScheme::getInstance().colours;
    const auto metrics = StageLayout::calculateMetrics(getWidth(), getHeight(), showTuner);

    if (metrics.liveQueueRailWidth <= 0 || bounds.getWidth() < 180.0f)
        return;

    const auto wantedHeight = (float)(metrics.liveQueueHeaderHeight + metrics.liveQueueRowHeight * 3 + metrics.margin);
    auto panel = bounds.withSizeKeepingCentre(bounds.getWidth(), juce::jmin(bounds.getHeight(), wantedHeight));

    g.setColour(colours["Stage Panel Background"].withAlpha(0.38f));
    g.fillRoundedRectangle(panel, 16.0f);
    g.setColour(colours["Plugin Border"].withAlpha(0.38f));
    g.drawRoundedRectangle(panel.reduced(0.5f), 16.0f, 1.0f);

    auto content = panel.reduced(12.0f, 10.0f);
    auto header = content.removeFromTop((float)metrics.liveQueueHeaderHeight);
    g.setColour(colours["Text Colour"].withAlpha(0.46f));
    g.setFont(fonts.getDisplayFont(metrics.queueLabelFontHeight));
    g.drawText("LIVE QUEUE", header, Justification::centredLeft);

    const auto positionText = totalPatchCount > 0 ? String(currentPatchIndex + 1) + "/" + String(totalPatchCount)
                                                  : String("0/0");
    g.setColour(colours["Accent Colour"].withAlpha(0.84f));
    g.setFont(fonts.getMonoDisplayFont(metrics.queueLabelFontHeight));
    g.drawText(positionText, header, Justification::centredRight);

    auto drawRow = [&](Rectangle<float> row, const String& label, const String& name, const String& fallback,
                       Colour accent, bool active, bool muted)
    {
        row = row.reduced(0.0f, 4.0f);
        const auto fill = active ? accent.withAlpha(0.18f) : colours["Text Colour"].withAlpha(muted ? 0.055f : 0.075f);
        const auto stroke = active ? accent.withAlpha(0.62f) : colours["Plugin Border"].withAlpha(0.34f);

        g.setColour(fill);
        g.fillRoundedRectangle(row, 12.0f);
        g.setColour(stroke);
        g.drawRoundedRectangle(row.reduced(0.5f), 12.0f, active ? 1.4f : 1.0f);

        auto body = row.reduced(12.0f, 8.0f);
        auto tag = body.removeFromLeft(58.0f);
        body.removeFromLeft(8.0f);
        auto dot = Rectangle<float>(metrics.liveDotSize, metrics.liveDotSize)
                       .withCentre({tag.getX() + metrics.liveDotSize * 0.5f, tag.getCentreY()});
        g.setColour(accent.withAlpha(muted ? 0.28f : 0.92f));
        if (active)
            g.fillEllipse(dot.expanded(metrics.liveDotSize * 0.9f));
        g.fillEllipse(dot);

        g.setColour((active ? accent : colours["Text Colour"]).withAlpha(muted ? 0.36f : 0.78f));
        g.setFont(fonts.getDisplayFont(metrics.queueLabelFontHeight));
        g.drawText(label, tag.withTrimmedLeft(metrics.liveDotSize + 8.0f), Justification::centredLeft);

        const auto displayName = name.isNotEmpty() ? name : fallback;
        const auto maxChars = juce::jlimit(12, 30, juce::roundToInt(body.getWidth() / 8.4f));
        g.setColour(colours["Text Colour"].withAlpha(muted ? 0.42f : 0.9f));
        g.setFont(fonts.getDisplayFont(metrics.queueTitleFontHeight));
        g.drawFittedText(StageLayout::elideLabel(displayName, maxChars), body.toNearestInt(),
                         Justification::centredLeft, 1);
    };

    const auto rowH = (float)metrics.liveQueueRowHeight;
    drawRow(content.removeFromTop(rowH), "PREV", previousPatchName, "Start of set",
            colours["Text Colour"].withAlpha(0.62f), false, previousPatchName.isEmpty());
    drawRow(content.removeFromTop(rowH), "LIVE", currentPatchName, "No Patch", colours["Accent Colour"], true, false);
    drawRow(content.removeFromTop(rowH), "NEXT", nextPatchName, "End of set", colours["Warning Colour"], false,
            nextPatchName.isEmpty());
}

void StageView::drawQueueFocus(Graphics& g, Rectangle<float> bounds)
{
    auto& fonts = FontManager::getInstance();
    auto& colours = ColourScheme::getInstance().colours;
    const auto metrics = StageLayout::calculateMetrics(getWidth(), getHeight(), showTuner);

    setlistRowHitboxes.clear();

    const int patchCount = patchNames.isEmpty() ? totalPatchCount : patchNames.size();
    const int activeIndex = juce::jlimit(0, juce::jmax(0, patchCount - 1), currentPatchIndex);
    auto content = bounds.reduced((float)metrics.margin * 1.25f, (float)metrics.margin * 0.9f);
    const bool twoPane = content.getWidth() >= 980.0f && content.getHeight() >= 340.0f;

    Rectangle<float> rail;
    if (twoPane)
    {
        const auto railW = juce::jlimit(330.0f, 452.0f, content.getWidth() * 0.34f);
        rail = content.removeFromRight(railW);
        content.removeFromRight((float)metrics.margin);
    }

    auto heroPanel = content.reduced(0.0f, twoPane ? 0.0f : content.getHeight() * 0.06f);
    g.setColour(colours["Stage Panel Background"].withAlpha(0.18f));
    g.fillRoundedRectangle(heroPanel, 20.0f);
    g.setColour(colours["Plugin Border"].withAlpha(0.26f));
    g.drawRoundedRectangle(heroPanel.reduced(0.5f), 20.0f, 1.0f);

    auto heroInner = heroPanel.reduced(28.0f, 24.0f);
    auto heroHeader = heroInner.removeFromTop(44.0f);
    const float queueNavReserve =
        heroHeader.getWidth() > 560.0f
            ? (float)(juce::jmax(92, metrics.navButtonWidth - 22) * 2 + 8 + metrics.margin)
            : 0.0f;
    auto heroHeaderText = heroHeader;
    if (queueNavReserve > 0.0f)
        heroHeaderText.removeFromRight(juce::jmin(queueNavReserve, heroHeaderText.getWidth() * 0.42f));
    const auto positionText = patchCount > 0 ? String(activeIndex + 1).paddedLeft('0', 2) + " / " + String(patchCount)
                                             : String("00 / 00");

    g.setColour(colours["Accent Colour"].withAlpha(0.88f));
    g.setFont(fonts.getDisplayFont(metrics.eyebrowFontHeight));
    g.drawText("SETLIST LIVE", heroHeaderText, Justification::centredLeft);
    g.setColour(colours["Text Colour"].withAlpha(0.54f));
    g.setFont(fonts.getMonoDisplayFont(metrics.positionFontHeight));
    g.drawText(positionText, heroHeaderText, Justification::centredRight);

    const auto currentLabel = splitPatchLabel(currentPatchName.isNotEmpty() ? currentPatchName : String("No Patch"));
    auto titleArea = heroInner;
    auto bottomStack = titleArea.removeFromBottom(juce::jlimit(96.0f, 138.0f, heroPanel.getHeight() * 0.24f));

    g.setColour(colours["Text Colour"]);
    g.setFont(fonts.getDisplayFont(juce::jlimit(42.0f, 88.0f, heroPanel.getHeight() * 0.18f)));
    g.drawFittedText(StageLayout::elideLabel(currentLabel.title, metrics.patchNameMaxChars), titleArea.toNearestInt(),
                     Justification::centredLeft, 2);

    if (currentLabel.tone.isNotEmpty())
    {
        auto toneLine = bottomStack.removeFromTop(juce::jlimit(28.0f, 44.0f, bottomStack.getHeight() * 0.36f));
        g.setColour(colours["Accent Colour"].withAlpha(0.9f));
        g.setFont(fonts.getDisplayFont(metrics.nextPatchFontHeight));
        g.drawText(StageLayout::elideLabel(currentLabel.tone, metrics.nextPatchMaxChars), toneLine,
                   Justification::centredLeft);
    }

    auto cueRow = bottomStack.removeFromTop(juce::jlimit(40.0f, 54.0f, bottomStack.getHeight() * 0.48f));
    if (nextPatchName.isNotEmpty())
    {
        g.setColour(colours["Warning Colour"].withAlpha(0.11f));
        g.fillRoundedRectangle(cueRow, 12.0f);
        g.setColour(colours["Warning Colour"].withAlpha(0.48f));
        g.drawRoundedRectangle(cueRow.reduced(0.5f), 12.0f, 1.0f);
        g.setColour(colours["Text Colour"].withAlpha(0.72f));
        g.setFont(fonts.getDisplayFont(metrics.nextPatchFontHeight));
        g.drawText("NEXT  " + StageLayout::elideLabel(nextPatchName, metrics.nextPatchMaxChars), cueRow.reduced(16.0f, 0.0f),
                   Justification::centredLeft);
    }

    if (patchCount <= 0)
        return;

    if (rail.isEmpty())
        rail = bounds.reduced((float)metrics.margin * 1.4f, (float)metrics.margin).withTop(bounds.getCentreY());

    g.setColour(colours["Stage Panel Background"].withAlpha(0.34f));
    g.fillRoundedRectangle(rail, 18.0f);
    g.setColour(colours["Plugin Border"].withAlpha(0.34f));
    g.drawRoundedRectangle(rail.reduced(0.5f), 18.0f, 1.0f);

    auto railInner = rail.reduced(16.0f, 14.0f);
    auto railHeader = railInner.removeFromTop(42.0f);
    g.setColour(colours["Text Colour"].withAlpha(0.7f));
    g.setFont(fonts.getDisplayFont(17.0f));
    g.drawText("SETLIST", railHeader, Justification::centredLeft);
    g.setColour(colours["Text Colour"].withAlpha(0.38f));
    g.setFont(fonts.getMonoDisplayFont(13.0f));
    g.drawText(String(activeIndex + 1) + "/" + String(patchCount), railHeader, Justification::centredRight);

    railInner.removeFromTop(8.0f);
    const auto rowGap = 7.0f;
    const int maxRows = juce::jmax(1, juce::roundToInt((railInner.getHeight() + rowGap) / 56.0f));
    const int visibleRows = juce::jlimit(1, patchCount, maxRows);
    const int startIndex = juce::jlimit(0, juce::jmax(0, patchCount - visibleRows), activeIndex - visibleRows / 2);
    const auto rowHeight = (railInner.getHeight() - rowGap * (float)(visibleRows - 1)) / (float)visibleRows;

    for (int row = 0; row < visibleRows; ++row)
    {
        const int patchIndex = startIndex + row;
        auto item = Rectangle<float>(railInner.getX(), railInner.getY() + (rowHeight + rowGap) * (float)row,
                                     railInner.getWidth(), rowHeight);
        auto paintBounds = item.reduced(0.0f, 2.0f);
        setlistRowHitboxes.push_back({paintBounds, patchIndex});

        const bool active = patchIndex == activeIndex;
        const bool next = patchIndex == activeIndex + 1;
        const bool past = patchIndex < activeIndex;
        const auto accent = active   ? colours["Accent Colour"]
                            : next  ? colours["Warning Colour"]
                            : past  ? colours["Text Colour"].withAlpha(0.44f)
                                    : colours["Plugin Border"].brighter(0.45f);

        ColourGradient fill(active ? colours["Accent Colour"].withAlpha(0.2f)
                                   : colours["Stage Panel Background"].withAlpha(past ? 0.28f : 0.5f),
                            paintBounds.getX(), paintBounds.getY(),
                            active ? colours["Stage Panel Background"].withAlpha(0.58f)
                                   : colours["Window Background"].withAlpha(0.16f),
                            paintBounds.getX(), paintBounds.getBottom(), false);
        g.setGradientFill(fill);
        g.fillRoundedRectangle(paintBounds, active ? 14.0f : 12.0f);

        g.setColour((active || next ? accent : colours["Plugin Border"]).withAlpha(active ? 0.74f : 0.36f));
        g.drawRoundedRectangle(paintBounds.reduced(0.5f), active ? 14.0f : 12.0f, active ? 1.5f : 1.0f);

        auto body = paintBounds.reduced(14.0f, 8.0f);
        auto numberArea = body.removeFromLeft(42.0f);
        g.setFont(fonts.getMonoDisplayFont(active ? 19.0f : 17.0f));
        g.setColour(active ? colours["Accent Colour"] : colours["Text Colour"].withAlpha(past ? 0.34f : 0.48f));
        g.drawText(String(patchIndex + 1).paddedLeft('0', 2), numberArea, Justification::centredLeft);

        auto tagArea = body.removeFromRight(active ? 62.0f : (next ? 60.0f : 0.0f));
        if (active || next)
        {
            tagArea = tagArea.reduced(0.0f, 5.0f);
            g.setColour(active ? colours["Accent Colour"] : colours["Warning Colour"].withAlpha(0.13f));
            g.fillRoundedRectangle(tagArea, 8.0f);
            auto tagText = tagArea;
            if (active)
            {
                auto led = Rectangle<float>(6.0f, 6.0f).withCentre({tagArea.getX() + 11.0f, tagArea.getCentreY()});
                g.setColour(colours["Window Background"].darker(0.45f).withAlpha(0.18f));
                g.fillEllipse(led.expanded(4.0f));
                g.setColour(colours["Window Background"].darker(0.45f));
                g.fillEllipse(led);
                tagText.removeFromLeft(12.0f);
            }
            g.setColour(active ? colours["Window Background"].darker(0.4f) : colours["Warning Colour"]);
            g.setFont(fonts.getDisplayFont(11.0f));
            g.drawText(active ? "LIVE" : "NEXT", tagText, Justification::centred);
        }

        body.removeFromRight(8.0f);
        const String storedName = patchIndex >= 0 && patchIndex < patchNames.size() ? patchNames[patchIndex] : String();
        const auto parsed = splitPatchLabel(storedName.isNotEmpty()
                                                ? storedName
                                                : (patchIndex == activeIndex ? currentPatchName
                                                                             : "Patch " + String(patchIndex + 1)));
        auto titleLine = body.removeFromTop(body.getHeight() * (parsed.tone.isNotEmpty() ? 0.58f : 1.0f));
        const int titleChars = juce::jlimit(14, 34, juce::roundToInt(titleLine.getWidth() / 9.2f));
        g.setFont(fonts.getDisplayFont(active ? 19.0f : 17.0f));
        g.setColour(colours["Text Colour"].withAlpha(past ? 0.52f : 0.92f));
        g.drawText(StageLayout::elideLabel(parsed.title, titleChars), titleLine, Justification::centredLeft);

        if (parsed.tone.isNotEmpty())
        {
            const int toneChars = juce::jlimit(12, 36, juce::roundToInt(body.getWidth() / 8.0f));
            g.setFont(fonts.getDisplayFont(13.0f));
            g.setColour(colours["Text Colour"].withAlpha(past ? 0.34f : 0.52f));
            g.drawText(StageLayout::elideLabel(parsed.tone, toneChars), body, Justification::centredLeft);
        }
    }
}

void StageView::drawGridView(Graphics& g, Rectangle<float> bounds)
{
    auto& fonts = FontManager::getInstance();
    auto& colours = ColourScheme::getInstance().colours;
    const auto metrics = StageLayout::calculateMetrics(getWidth(), getHeight(), showTuner);

    gridTileHitboxes.clear();
    gridBankHitboxes.clear();

    const int patchCount = patchNames.isEmpty() ? totalPatchCount : patchNames.size();
    auto content = bounds.reduced((float)metrics.margin * 1.35f, (float)metrics.margin * 0.9f);

    auto bankRow = content.removeFromTop(46.0f);
    content.removeFromTop(12.0f);

    const int bankSize = 8;
    const int activeIndex = juce::jlimit(0, juce::jmax(0, patchCount - 1), currentPatchIndex);
    const int bankStart = patchCount > 0 ? (activeIndex / bankSize) * bankSize : 0;
    const int bankEnd = juce::jmin(bankStart + bankSize, patchCount);
    const int bankNumber = patchCount > 0 ? bankStart / bankSize + 1 : 1;
    const int totalBanks = patchCount > 0 ? (patchCount + bankSize - 1) / bankSize : 1;
    const int activeBank = juce::jmax(0, bankNumber - 1);

    auto drawBankPill = [&](Rectangle<float> pill, int bankIndex, bool active, bool enabled)
    {
        const auto accent = active ? colours["Accent Colour"] : colours["Text Colour"].withAlpha(0.62f);
        g.setColour(active ? colours["Accent Colour"].withAlpha(0.17f)
                           : colours["Stage Panel Background"].withAlpha(enabled ? 0.46f : 0.26f));
        g.fillRoundedRectangle(pill, 11.0f);
        g.setColour((active ? colours["Accent Colour"] : colours["Plugin Border"]).withAlpha(active ? 0.74f : 0.40f));
        g.drawRoundedRectangle(pill.reduced(0.5f), 11.0f, active ? 1.5f : 1.0f);

        if (active)
        {
            auto led = Rectangle<float>(7.0f, 7.0f).withCentre({pill.getX() + 16.0f, pill.getCentreY()});
            g.setColour(colours["Accent Colour"].withAlpha(0.16f));
            g.fillEllipse(led.expanded(5.0f));
            g.setColour(colours["Accent Colour"]);
            g.fillEllipse(led);
        }

        g.setFont(fonts.getDisplayFont(14.0f));
        g.setColour(accent.withAlpha(enabled ? 0.92f : 0.34f));
        g.drawText(StageLayout::formatBankLabel(bankIndex).toUpperCase(),
                   pill.withTrimmedLeft(active ? 12.0f : 0.0f), Justification::centred);

        if (enabled && !active)
            gridBankHitboxes.push_back({pill, bankIndex * bankSize});
    };

    auto titlePill = bankRow.removeFromLeft(128.0f).reduced(0.0f, 3.0f);
    g.setColour(colours["Stage Panel Background"].withAlpha(0.38f));
    g.fillRoundedRectangle(titlePill, 11.0f);
    g.setColour(colours["Plugin Border"].withAlpha(0.38f));
    g.drawRoundedRectangle(titlePill.reduced(0.5f), 11.0f, 1.0f);
    g.setFont(fonts.getDisplayFont(13.0f));
    g.setColour(colours["Text Colour"].withAlpha(0.62f));
    g.drawText("PATCH GRID", titlePill, Justification::centred);
    bankRow.removeFromLeft(10.0f);

    const float hintWidth = juce::jmin(520.0f, bankRow.getWidth() * 0.42f);
    auto hintArea = bankRow.removeFromRight(hintWidth);
    const float bankPillWidth = juce::jlimit(94.0f, 118.0f, bankRow.getWidth() * 0.22f);
    const float bankPillGap = 8.0f;
    const int maxBankPills = juce::jmax(1, juce::roundToInt((bankRow.getWidth() + bankPillGap) /
                                                            (bankPillWidth + bankPillGap)));
    const auto visibleBanks = StageLayout::collectVisibleBankIndices(activeBank, totalBanks, maxBankPills);

    for (int bankIndex : visibleBanks)
    {
        auto bankPill = bankRow.removeFromLeft(bankPillWidth).reduced(0.0f, 3.0f);
        drawBankPill(bankPill, bankIndex, bankIndex == activeBank, patchCount > 0);
        bankRow.removeFromLeft(bankPillGap);
    }

    const auto rangeText = patchCount > 0
                               ? String(bankStart + 1).paddedLeft('0', 2) + "-" +
                                     String(bankEnd).paddedLeft('0', 2) + " of " + String(patchCount) +
                                     " patches - tap a bank or tile to switch"
                               : String("No patches loaded");
    g.setFont(fonts.getDisplayFont(14.0f));
    g.setColour(colours["Text Colour"].withAlpha(0.42f));
    g.drawText(rangeText, hintArea, Justification::centredRight);

    if (patchCount <= 0)
    {
        g.setColour(colours["Text Colour"].withAlpha(0.28f));
        g.setFont(fonts.getDisplayFont(28.0f));
        g.drawText("No patches loaded", content, Justification::centred);
        return;
    }

    const int visibleCount = juce::jmax(1, bankEnd - bankStart);
    const int columns = content.getWidth() < 780.0f ? 2 : 4;
    const int rows = juce::jmax(1, (visibleCount + columns - 1) / columns);
    const float gap = juce::jlimit(10.0f, 18.0f, content.getWidth() * 0.012f);
    const float tileW = (content.getWidth() - gap * (float)(columns - 1)) / (float)columns;
    const float tileH = (content.getHeight() - gap * (float)(rows - 1)) / (float)rows;

    for (int slot = 0; slot < visibleCount; ++slot)
    {
        const int patchIndex = bankStart + slot;
        const int col = slot % columns;
        const int row = slot / columns;
        auto tile = Rectangle<float>(content.getX() + (tileW + gap) * (float)col,
                                     content.getY() + (tileH + gap) * (float)row, tileW, tileH);

        if (tile.getHeight() < 68.0f)
            continue;

        gridTileHitboxes.push_back({tile, patchIndex});

        const bool isActive = patchIndex == activeIndex;
        const bool isNext = patchIndex == activeIndex + 1;
        const bool isPast = patchIndex < activeIndex;
        const auto accent = isActive   ? colours["Accent Colour"]
                            : isNext  ? colours["Warning Colour"]
                            : isPast  ? colours["Text Colour"].withAlpha(0.42f)
                                      : colours["Slider Colour"].withAlpha(0.72f);

        ColourGradient fill(isActive ? colours["Accent Colour"].withAlpha(0.22f)
                                     : colours["Stage Panel Background"].withAlpha(0.58f),
                            tile.getX(), tile.getY(),
                            isActive ? colours["Stage Panel Background"].withAlpha(0.72f)
                                     : colours["Stage Panel Background"].withAlpha(0.34f),
                            tile.getX(), tile.getBottom(), false);
        g.setGradientFill(fill);
        g.fillRoundedRectangle(tile, 16.0f);

        g.setColour((isActive || isNext ? accent : colours["Plugin Border"]).withAlpha(isActive ? 0.9f : 0.46f));
        g.drawRoundedRectangle(tile.reduced(0.5f), 16.0f, isActive ? 1.8f : 1.0f);

        auto inner = tile.reduced(18.0f, 14.0f);
        auto top = inner.removeFromTop(34.0f);
        const auto patchNumber = String(patchIndex + 1).paddedLeft('0', 2);
        g.setFont(fonts.getMonoDisplayFont(22.0f));
        g.setColour(isActive ? colours["Accent Colour"] : colours["Text Colour"].withAlpha(0.46f));
        g.drawText(patchNumber, top.removeFromLeft(52.0f), Justification::centredLeft);

        if (isActive || isNext)
        {
            auto tag = top.removeFromRight(isActive ? 76.0f : 70.0f).reduced(0.0f, 4.0f);
            g.setColour(isActive ? colours["Accent Colour"] : colours["Warning Colour"].withAlpha(0.12f));
            g.fillRoundedRectangle(tag, 8.0f);
            auto tagText = tag;
            if (isActive)
            {
                auto led = Rectangle<float>(6.0f, 6.0f).withCentre({tag.getX() + 12.0f, tag.getCentreY()});
                g.setColour(colours["Window Background"].darker(0.45f).withAlpha(0.18f));
                g.fillEllipse(led.expanded(4.0f));
                g.setColour(colours["Window Background"].darker(0.45f));
                g.fillEllipse(led);
                tagText.removeFromLeft(12.0f);
            }
            g.setColour(isActive ? colours["Window Background"].darker(0.4f) : colours["Warning Colour"]);
            g.setFont(fonts.getDisplayFont(12.0f));
            g.drawText(isActive ? "LIVE" : "NEXT", tagText, Justification::centred);
        }

        const String storedName = patchIndex >= 0 && patchIndex < patchNames.size() ? patchNames[patchIndex] : String();
        String rawName = storedName.isNotEmpty() ? storedName : "Patch " + patchNumber;
        String title = rawName;
        String tone;
        const int dash = rawName.indexOf(" - ");
        if (dash > 0)
        {
            title = rawName.substring(0, dash).trim();
            tone = rawName.substring(dash + 3).trim();
        }

        auto nameArea = inner.withTrimmedBottom(20.0f);
        const int titleChars = juce::jlimit(12, 30, juce::roundToInt(nameArea.getWidth() / 10.0f));
        g.setFont(fonts.getDisplayFont(juce::jlimit(18.0f, 28.0f, tileH * 0.15f)));
        g.setColour(colours["Text Colour"].withAlpha(isPast ? 0.58f : 0.96f));
        g.drawFittedText(StageLayout::elideLabel(title, titleChars), nameArea.toNearestInt(),
                         Justification::centredLeft, 2);

        if (tone.isNotEmpty())
        {
            auto toneArea = inner.removeFromBottom(22.0f);
            const int toneChars = juce::jlimit(12, 36, juce::roundToInt(toneArea.getWidth() / 8.0f));
            g.setFont(fonts.getDisplayFont(14.0f));
            g.setColour((isNext ? colours["Warning Colour"] : colours["Text Colour"]).withAlpha(isPast ? 0.42f : 0.62f));
            g.drawText(StageLayout::elideLabel(tone, toneChars), toneArea, Justification::centredLeft);
        }

        auto stripe = Rectangle<float>(tile.getX(), tile.getBottom() - 7.0f, tile.getWidth(), 7.0f);
        g.setColour(accent.withAlpha(isActive ? 0.98f : 0.72f));
        g.fillRoundedRectangle(stripe, 3.5f);
    }
}

void StageView::drawTunerDisplay(Graphics& g, Rectangle<float> bounds)
{
    auto& fonts = FontManager::getInstance();
    auto& colours = ColourScheme::getInstance().colours;
    const auto metrics = StageLayout::calculateMetrics(getWidth(), getHeight(), showTuner);

    auto panel = bounds.reduced((float)metrics.margin * 2.2f, (float)metrics.margin * 0.35f);
    g.setColour(colours["Stage Panel Background"].withAlpha(0.34f));
    g.fillRoundedRectangle(panel, 14.0f);
    g.setColour(colours["Plugin Border"].withAlpha(0.35f));
    g.drawRoundedRectangle(panel, 14.0f, 1.0f);

    auto labelArea = panel.removeFromTop(juce::jmax(24.0f, metrics.tunerCentsFontHeight * 0.95f));
    g.setColour(colours["Tuner Active Colour"].withAlpha(0.82f));
    g.setFont(fonts.getDisplayFont(metrics.safetyFontHeight));
    g.drawText("TUNER ACTIVE", labelArea.reduced(14.0f, 0.0f), Justification::centredLeft);

    auto centreX = panel.getCentreX();
    auto centreY = panel.getCentreY();

    if (tunerProcessor == nullptr || !tunerProcessor->isPitchDetected())
    {
        g.setColour(colours["Text Colour"].withAlpha(0.25f));
        g.setFont(fonts.getDisplayFont(metrics.tunerWaitingFontHeight));
        g.drawText("Waiting for signal...", panel, Justification::centred);
        return;
    }

    // Note display
    String noteName = getNoteName(detectedNote);
    Colour noteCol = getTuningColour(displayedCents);

    g.setColour(noteCol);
    g.setFont(fonts.getDisplayFont(metrics.tunerNoteFontHeight));
    g.drawText(noteName, panel.withTrimmedBottom(metrics.tunerCentsFontHeight * 1.8f), Justification::centred);

    // Cents display
    g.setFont(fonts.getMonoDisplayFont(metrics.tunerCentsFontHeight));
    String centsStr = (displayedCents >= 0 ? "+" : "") + String(static_cast<int>(displayedCents)) + " cents";
    g.drawText(centsStr, panel.withTrimmedTop(metrics.tunerNoteFontHeight * 1.15f), Justification::centred);

    // Simple bar indicator
    float barWidth = metrics.tunerBarWidth;
    float barHeight = metrics.tunerBarHeight;
    float barX = centreX - barWidth / 2;
    float barY = centreY + metrics.tunerCentsFontHeight * 1.6f;

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

void StageView::drawSafetyBar(Graphics& g, Rectangle<float> bounds)
{
    auto& fonts = FontManager::getInstance();
    auto& colours = ColourScheme::getInstance().colours;
    const auto metrics = StageLayout::calculateMetrics(getWidth(), getHeight(), showTuner);

    g.setColour(colours["Stage Panel Background"].withAlpha(0.42f));
    g.fillRect(bounds);
    g.setColour(colours["Plugin Border"].withAlpha(0.45f));
    g.drawHorizontalLine(juce::roundToInt(bounds.getY()), bounds.getX(), bounds.getRight());

    auto labelArea = bounds.reduced((float)metrics.margin, 0.0f).removeFromTop(metrics.meterTopOffset - 2.0f);
    g.setFont(fonts.getDisplayFont(metrics.safetyFontHeight));
    g.setColour(colours["Text Colour"].withAlpha(0.46f));
    g.drawText("SAFETY BAR", labelArea.withWidth(86.0f), Justification::centredLeft);

    const auto meterGroupX = metrics.meterStartX + metrics.meterLabelWidth + 6.0f;
    auto meterLabelArea = Rectangle<float>(meterGroupX + 72.0f, labelArea.getY(),
                                           metrics.meterWidth * 2.0f + metrics.meterSpacing, labelArea.getHeight());
    g.setColour(colours["Text Colour"].withAlpha(0.34f));
    g.drawText("MASTER BUS", meterLabelArea, Justification::centredLeft);

    auto panicGlow = panicButton != nullptr ? panicButton->getBounds().toFloat().expanded(4.0f)
                                            : Rectangle<float>();
    if (!panicGlow.isEmpty())
    {
        g.setColour(colours["Danger Colour"].withAlpha(0.13f));
        g.fillRoundedRectangle(panicGlow, 14.0f);
    }
}

//==============================================================================
void StageView::resized()
{
    auto bounds = getLocalBounds();
    const bool tunerFocus = viewMode == ViewMode::Tuner;
    const bool reserveTunerStrip =
        StageLayout::shouldReserveTunerStrip(showTuner, tunerFocus, viewMode == ViewMode::Patch);
    const auto metrics = StageLayout::calculateMetrics(bounds.getWidth(), bounds.getHeight(), reserveTunerStrip);
    const int margin = metrics.margin;
    const int utilityButtonHeight = metrics.utilityButtonHeight;
    const int utilityButtonWidth = metrics.utilityButtonWidth;

    // Header buttons (top right)
    const int headerButtonY = (metrics.headerHeight - utilityButtonHeight) / 2;
    if (themeSwitcher != nullptr)
    {
        constexpr int switcherW = 136;
        themeSwitcher->setBounds(margin + 176, headerButtonY + juce::roundToInt((utilityButtonHeight - 32) * 0.5f),
                                 switcherW, 32);
        themeSwitcher->setVisible(bounds.getWidth() >= 1120);
        themeSwitcher->toFront(false);
    }

    exitButton->setBounds(bounds.getWidth() - utilityButtonWidth - margin, headerButtonY, utilityButtonWidth,
                          utilityButtonHeight);
    tunerToggleButton->setBounds(bounds.getWidth() - utilityButtonWidth * 2 - margin * 2, headerButtonY,
                                 utilityButtonWidth, utilityButtonHeight);

    const int modeButtonW = juce::jlimit(72, 96, bounds.getWidth() / 24);
    const int modeButtonGap = 6;
    const int modeGroupW = modeButtonW * 4 + modeButtonGap * 3;
    const int modeX = (bounds.getWidth() - modeGroupW) / 2;
    patchViewButton->setBounds(modeX, headerButtonY, modeButtonW, utilityButtonHeight);
    queueViewButton->setBounds(modeX + modeButtonW + modeButtonGap, headerButtonY, modeButtonW, utilityButtonHeight);
    gridViewButton->setBounds(modeX + (modeButtonW + modeButtonGap) * 2, headerButtonY, modeButtonW,
                              utilityButtonHeight);
    tunerViewButton->setBounds(modeX + (modeButtonW + modeButtonGap) * 3, headerButtonY, modeButtonW,
                               utilityButtonHeight);

    auto performanceArea = bounds;
    performanceArea.removeFromTop(metrics.headerHeight);
    performanceArea.removeFromBottom(metrics.footerHeight);
    if (reserveTunerStrip)
        performanceArea.removeFromBottom(metrics.tunerHeight);

    // Navigation buttons
    const int navY = performanceArea.getCentreY() - metrics.navButtonHeight / 2;
    if (viewMode == ViewMode::Grid || viewMode == ViewMode::Patch)
    {
        const int gridNavY = metrics.headerHeight + margin + 6;
        const int gridNavW = juce::jmax(92, metrics.navButtonWidth - 22);
        nextButton->setBounds(bounds.getWidth() - gridNavW - margin, gridNavY, gridNavW, metrics.utilityButtonHeight);
        prevButton->setBounds(nextButton->getX() - gridNavW - 8, gridNavY, gridNavW, metrics.utilityButtonHeight);
    }
    else if (viewMode == ViewMode::Queue)
    {
        auto queueContent = bounds;
        queueContent.removeFromTop(metrics.headerHeight);
        queueContent.removeFromBottom(metrics.footerHeight);
        if (reserveTunerStrip)
            queueContent.removeFromBottom(metrics.tunerHeight);
        queueContent = queueContent.reduced(juce::roundToInt((float)margin * 1.25f),
                                            juce::roundToInt((float)margin * 0.9f));

        const bool twoPane = queueContent.getWidth() >= 980 && queueContent.getHeight() >= 340;
        int heroRight = queueContent.getRight();
        if (twoPane)
        {
            const int railW = juce::roundToInt(juce::jlimit(330.0f, 452.0f, (float)queueContent.getWidth() * 0.34f));
            heroRight = queueContent.getRight() - railW - margin;
        }

        const int compactNavW = juce::jmax(92, metrics.navButtonWidth - 22);
        const int compactNavH = metrics.utilityButtonHeight;
        const int navRight = heroRight - 28;
        const int queueNavY = queueContent.getY() + 24 + juce::jmax(0, (44 - compactNavH) / 2);
        nextButton->setBounds(navRight - compactNavW, queueNavY, compactNavW, compactNavH);
        prevButton->setBounds(navRight - compactNavW * 2 - 8, queueNavY, compactNavW, compactNavH);
    }
    else
    {
        prevButton->setBounds(margin, navY, metrics.navButtonWidth, metrics.navButtonHeight);
        const bool reserveLiveQueueRail = viewMode == ViewMode::Patch && metrics.liveQueueRailWidth > 0;
        const int nextRightInset = reserveLiveQueueRail ? metrics.liveQueueRailWidth + margin * 3 : margin;
        nextButton->setBounds(bounds.getWidth() - metrics.navButtonWidth - nextRightInset, navY, metrics.navButtonWidth,
                              metrics.navButtonHeight);
    }

    // Panic button (bottom right safety bar)
    panicButton->setBounds(bounds.getWidth() - metrics.panicButtonWidth - margin,
                           bounds.getHeight() - metrics.panicButtonHeight -
                               juce::roundToInt((metrics.footerHeight - metrics.panicButtonHeight) * 0.5f),
                           metrics.panicButtonWidth, metrics.panicButtonHeight);

    // Master gain sliders in footer area (next to VU meters)
    {
        const int footerY = bounds.getHeight() - metrics.footerHeight;
        const float labelW = metrics.meterLabelWidth;
        const float meterW = metrics.meterWidth;
        const float startX = metrics.meterStartX;

        // Input slider below input VU
        int inSliderX = (int)(startX + labelW + 6.0f);
        inputGainSlider->setBounds(inSliderX, footerY + juce::roundToInt(metrics.sliderTopOffset),
                                   (int)meterW, juce::roundToInt(metrics.sliderHeight));

        // Output slider below output VU
        int outSliderX = (int)(startX + labelW + meterW + metrics.meterSpacing + metrics.panicButtonWidth + labelW +
                               6.0f);
        outputGainSlider->setBounds(outSliderX, footerY + juce::roundToInt(metrics.sliderTopOffset),
                                    (int)meterW, juce::roundToInt(metrics.sliderHeight));
    }
}

//==============================================================================
void StageView::mouseDown(const MouseEvent& event)
{
    const auto point = event.position;
    if (viewMode != ViewMode::Grid && viewMode != ViewMode::Queue)
        return;

    if (viewMode == ViewMode::Grid)
    {
        for (const auto& hitbox : gridBankHitboxes)
        {
            if (hitbox.first.contains(point))
            {
                switchToPatchIndex(hitbox.second);
                return;
            }
        }
    }

    auto& hitboxes = viewMode == ViewMode::Grid ? gridTileHitboxes : setlistRowHitboxes;
    for (const auto& hitbox : hitboxes)
    {
        if (hitbox.first.contains(point))
        {
            switchToPatchIndex(hitbox.second);
            return;
        }
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
        if (!showTuner && viewMode == ViewMode::Tuner)
            setViewMode(ViewMode::Patch);
        resized();
        repaint();
    }
    else if (button == patchViewButton.get())
    {
        setViewMode(ViewMode::Patch);
    }
    else if (button == queueViewButton.get())
    {
        setViewMode(ViewMode::Queue);
    }
    else if (button == gridViewButton.get())
    {
        setViewMode(ViewMode::Grid);
    }
    else if (button == tunerViewButton.get())
    {
        setViewMode(ViewMode::Tuner);
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
    else if (key == KeyPress(L'1'))
    {
        setViewMode(ViewMode::Patch);
        return true;
    }
    else if (key == KeyPress(L'2'))
    {
        setViewMode(ViewMode::Queue);
        return true;
    }
    else if (key == KeyPress(L'3'))
    {
        setViewMode(ViewMode::Grid);
        return true;
    }
    else if (key == KeyPress(L'4') || key == KeyPress(L't') || key == KeyPress(L'T'))
    {
        setViewMode(ViewMode::Tuner);
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

void StageView::setViewMode(ViewMode mode)
{
    viewMode = mode;
    if (viewMode == ViewMode::Tuner)
    {
        showTuner = true;
        tunerToggleButton->setToggleState(true, dontSendNotification);
    }

    syncViewButtons();
    resized();
    repaint();
}

void StageView::setViewModeForVisualQa(int modeIndex)
{
    switch (modeIndex)
    {
    case 1:
        setViewMode(ViewMode::Queue);
        break;
    case 2:
        setViewMode(ViewMode::Grid);
        break;
    case 3:
        setViewMode(ViewMode::Tuner);
        break;
    default:
        setViewMode(ViewMode::Patch);
        break;
    }
}

void StageView::syncViewButtons()
{
    if (patchViewButton)
        patchViewButton->setToggleState(viewMode == ViewMode::Patch, dontSendNotification);
    if (queueViewButton)
        queueViewButton->setToggleState(viewMode == ViewMode::Queue, dontSendNotification);
    if (gridViewButton)
        gridViewButton->setToggleState(viewMode == ViewMode::Grid, dontSendNotification);
    if (tunerViewButton)
        tunerViewButton->setToggleState(viewMode == ViewMode::Tuner, dontSendNotification);
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

void StageView::switchToPatchIndex(int patchIndex)
{
    if (mainPanel == nullptr || patchIndex < 0 || patchIndex >= totalPatchCount)
        return;

    if (auto* patchBox = mainPanel->getPatchComboBox())
        patchBox->setSelectedItemIndex(patchIndex, sendNotification);
}
