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

#include <array>

namespace
{
constexpr float kStagePitchTraceRangeCents = 50.0f;
constexpr float kStagePitchTraceConnectBreakCents = 35.0f;
constexpr int kStageStringCount = 6;
constexpr std::array<int, kStageStringCount> kStageStringMidiNotes{40, 45, 50, 55, 59, 64};
constexpr std::array<const char*, kStageStringCount> kStageStringLabels{"E2", "A2", "D3", "G3", "B3", "E4"};

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
            const auto panicTextColour = palette["Danger Colour"].contrasting(0.96f);
            ColourGradient panicFill(palette["Danger Colour"].brighter(isMouseOverButton ? 0.18f : 0.08f),
                                     bounds.getX(), bounds.getY(), palette["Danger Colour"].darker(0.18f),
                                     bounds.getX(), bounds.getBottom(), false);
            g.setGradientFill(panicFill);
            g.fillRoundedRectangle(bounds, radius);
            g.setColour(palette["Danger Colour"].brighter(0.35f).withAlpha(0.78f));
            g.drawRoundedRectangle(bounds.reduced(0.5f), radius, 1.4f);
            g.setColour(panicTextColour.withAlpha(0.20f));
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
        auto& palette = ::ColourScheme::getInstance().colours;
        const bool active = button.getToggleState();
        const bool panic = button.getName().containsIgnoreCase("panic");
        const float fontHeight = juce::jlimit(11.0f, 16.5f, button.getHeight() * 0.31f);
        auto textColour = panic ? palette["Danger Colour"].contrasting(0.96f)
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
    stagePitchTraceNote.fill(-1);

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
        const bool detected = tunerProcessor->isPitchDetected();
        float targetCents = detected ? tunerProcessor->getCentsDeviation() : 0.0f;
        displayedCents += (targetCents - displayedCents) * NEEDLE_SMOOTHING;

        float targetAngle = jlimit(-50.0f, 50.0f, displayedCents) * 0.9f;
        needleAngle += (targetAngle - needleAngle) * NEEDLE_SMOOTHING;

        const float targetConfidence = detected ? tunerProcessor->getDetectedConfidence() : 0.0f;
        displayedConfidence += (targetConfidence - displayedConfidence) * 0.18f;

        detectedNote = detected ? tunerProcessor->getDetectedNote() : -1;
        if (++stagePitchTraceFrameCounter >= 2)
        {
            stagePitchTraceFrameCounter = 0;
            pushStageTunerTraceSample();
        }

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

    g.setColour(colours["Stage Panel Background"].withAlpha(0.40f));
    g.fillRect(bounds);

    g.setColour(colours["Plugin Border"].withAlpha(0.48f));
    g.drawHorizontalLine(juce::roundToInt(bounds.getBottom()) - 1, bounds.getX(), bounds.getRight());

    auto leftArea = bounds.reduced((float)metrics.margin, 0.0f).withWidth((float)metrics.stageBrandWidth);
    const auto dotSize = metrics.liveDotSize;
    const auto dotArea =
        Rectangle<float>(leftArea.getX(), leftArea.getCentreY() - dotSize * 0.5f, dotSize, dotSize);

    g.setColour(colours["Accent Colour"].withAlpha(0.15f));
    g.fillEllipse(dotArea.expanded(dotSize * 0.85f));
    g.setColour(colours["Accent Colour"]);
    g.fillEllipse(dotArea);

    auto brandText = leftArea.withTrimmedLeft(dotSize + 12.0f);
    g.setColour(colours["Text Colour"].withAlpha(0.62f));
    g.setFont(fonts.getDisplayFont(metrics.statusFontHeight));
    g.drawText("STAGE MODE", brandText, Justification::centredLeft);

    if (themeSwitcher != nullptr && themeSwitcher->isVisible())
    {
        auto switcherRail = themeSwitcher->getBounds().toFloat().expanded(5.0f, 4.0f);
        g.setColour(colours["Stage Panel Background"].withAlpha(0.46f));
        g.fillRoundedRectangle(switcherRail, 11.0f);
        g.setColour(colours["Plugin Border"].withAlpha(0.34f));
        g.drawRoundedRectangle(switcherRail.reduced(0.5f), 11.0f, 1.0f);
    }

    if (patchViewButton != nullptr && queueViewButton != nullptr && gridViewButton != nullptr &&
        tunerViewButton != nullptr)
    {
        auto modeRail = patchViewButton->getBounds()
                            .getUnion(queueViewButton->getBounds())
                            .getUnion(gridViewButton->getBounds())
                            .getUnion(tunerViewButton->getBounds())
                            .expanded(5, 4)
                            .toFloat();
        g.setColour(colours["Window Background"].darker(0.36f).withAlpha(0.34f));
        g.fillRoundedRectangle(modeRail, 13.0f);
        g.setColour(colours["Stage Panel Background"].withAlpha(0.52f));
        g.fillRoundedRectangle(modeRail.reduced(1.0f), 12.0f);
        g.setColour(colours["Plugin Border"].withAlpha(0.52f));
        g.drawRoundedRectangle(modeRail.reduced(0.5f), 13.0f, 1.0f);
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
    if (prevButton != nullptr && nextButton != nullptr)
    {
        const auto navBounds = prevButton->getBounds().getUnion(nextButton->getBounds()).toFloat().expanded(14.0f, 0.0f);
        if (hintArea.intersects(navBounds))
            hintArea.setRight(juce::jmax(hintArea.getX(), navBounds.getX() - 10.0f));
    }
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

    const auto rangePrefix = patchCount > 0
                                 ? String(bankStart + 1).paddedLeft('0', 2) + "-" +
                                       String(bankEnd).paddedLeft('0', 2) + " of " + String(patchCount)
                                 : String("No patches loaded");
    const auto rangeText = patchCount > 0 && hintArea.getWidth() >= 300.0f
                               ? rangePrefix + " patches - tap a bank or tile to switch"
                               : rangePrefix;
    if (hintArea.getWidth() >= 96.0f)
    {
        g.setFont(fonts.getDisplayFont(14.0f));
        g.setColour(colours["Text Colour"].withAlpha(0.42f));
        g.drawText(rangeText, hintArea, Justification::centredRight);
    }

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

        Path tileClip;
        tileClip.addRoundedRectangle(tile, 16.0f);
        g.saveState();
        g.reduceClipRegion(tileClip);
        g.setColour(accent.withAlpha(isActive ? 0.78f : 0.45f));
        g.fillRect(tile.getX(), tile.getBottom() - 4.0f, tile.getWidth(), 4.0f);
        g.restoreState();
    }
}

void StageView::drawTunerDisplay(Graphics& g, Rectangle<float> bounds)
{
    auto& fonts = FontManager::getInstance();
    auto& colours = ColourScheme::getInstance().colours;
    const auto metrics = StageLayout::calculateMetrics(getWidth(), getHeight(), showTuner);

    const float insetX = juce::jlimit(12.0f, 42.0f, bounds.getWidth() * 0.032f);
    const float insetY = juce::jlimit(5.0f, 18.0f, bounds.getHeight() * 0.075f);
    auto panel = bounds.reduced(insetX, insetY);

    ColourGradient panelFill(colours["Stage Panel Background"].withAlpha(0.44f), panel.getX(), panel.getY(),
                             colours["Stage Panel Background"].interpolatedWith(colours["Field Background"], 0.24f)
                                 .withAlpha(0.52f),
                             panel.getX(), panel.getBottom(), false);
    panelFill.addColour(0.44, colours["Stage Panel Background"].withAlpha(0.34f));
    g.setGradientFill(panelFill);
    g.fillRoundedRectangle(panel, 14.0f);
    g.setColour(colours["Plugin Border"].withAlpha(0.35f));
    g.drawRoundedRectangle(panel, 14.0f, 1.0f);

    auto body = panel.reduced(14.0f, 9.0f);
    auto labelArea = body.removeFromTop(juce::jlimit(24.0f, 34.0f, panel.getHeight() * 0.16f));
    const bool detected = tunerProcessor != nullptr && tunerProcessor->isPitchDetected() && detectedNote >= 0;
    const auto noteCol = detected ? getTuningColour(displayedCents) : colours["Text Colour"].withAlpha(0.36f);

    g.setColour(colours["Tuner Active Colour"].withAlpha(0.82f));
    g.setFont(fonts.getDisplayFont(metrics.safetyFontHeight));
    g.drawText("TUNER ACTIVE", labelArea.reduced(14.0f, 0.0f), Justification::centredLeft);

    auto headerStatus = labelArea.removeFromRight(juce::jlimit(130.0f, 210.0f, labelArea.getWidth() * 0.24f))
                            .reduced(0.0f, 2.0f);
    {
        const String statusText = detected ? (std::abs(displayedCents) < 3.0f ? "IN TUNE" : "TRACKING") : "WAITING";
        const auto statusColour =
            detected ? (std::abs(displayedCents) < 3.0f ? colours["Tuner Active Colour"] : colours["Warning Colour"])
                     : colours["Text Colour"].withAlpha(0.44f);
        g.setColour(colours["Field Background"].interpolatedWith(statusColour, detected ? 0.16f : 0.05f));
        g.fillRoundedRectangle(headerStatus, 8.0f);
        g.setColour(statusColour.withAlpha(detected ? 0.60f : 0.24f));
        g.drawRoundedRectangle(headerStatus.reduced(0.5f), 8.0f, 0.9f);
        g.setColour(colours["Text Colour"].withAlpha(detected ? 0.88f : 0.48f));
        g.setFont(fonts.getMonoFont(juce::jlimit(9.0f, 13.0f, headerStatus.getHeight() * 0.44f)));
        g.drawText(statusText, headerStatus.reduced(8.0f, 0.0f), Justification::centred, true);
    }

    body.removeFromTop(juce::jlimit(4.0f, 8.0f, panel.getHeight() * 0.025f));

    auto railArea = body.removeFromBottom(juce::jlimit(24.0f, 34.0f, panel.getHeight() * 0.16f));
    body.removeFromBottom(juce::jlimit(4.0f, 8.0f, panel.getHeight() * 0.025f));

    const auto drawNoteBlock = [&](Rectangle<float> noteArea)
    {
        g.setColour(colours["Field Background"].withAlpha(0.54f));
        g.fillRoundedRectangle(noteArea, 12.0f);
        g.setColour(colours["Plugin Border"].interpolatedWith(noteCol, detected ? 0.18f : 0.02f).withAlpha(0.48f));
        g.drawRoundedRectangle(noteArea.reduced(0.5f), 12.0f, 0.9f);

        auto inner = noteArea.reduced(12.0f, 7.0f);
        auto noteLine = inner.removeFromTop(juce::jmax(36.0f, inner.getHeight() * 0.54f));
        g.setColour(noteCol);
        g.setFont(fonts.getDisplayFont(juce::jlimit(36.0f, metrics.tunerNoteFontHeight, noteLine.getHeight() * 0.82f)));
        g.drawText(detected ? getNoteName(detectedNote) : "---", noteLine, Justification::centred);

        const String centsStr = detected ? ((displayedCents >= 0 ? "+" : "") +
                                            String(static_cast<int>(std::round(displayedCents))) + " cents")
                                         : "no signal";
        g.setColour(detected ? colours["Text Colour"].withAlpha(0.84f) : colours["Text Colour"].withAlpha(0.42f));
        g.setFont(fonts.getMonoDisplayFont(juce::jlimit(15.0f, metrics.tunerCentsFontHeight, inner.getHeight() * 0.30f)));
        g.drawText(centsStr, inner.removeFromTop(juce::jmax(18.0f, inner.getHeight() * 0.48f)), Justification::centred);

        auto track = noteArea.reduced(18.0f, 0.0f).withY(noteArea.getBottom() - 13.0f).withHeight(7.0f);
        g.setColour(colours["Plugin Border"].withAlpha(0.36f));
        g.fillRoundedRectangle(track, 3.5f);
        g.setColour(colours["Text Colour"].withAlpha(0.34f));
        g.fillRect(track.getCentreX() - 1.0f, track.getY() - 3.0f, 2.0f, track.getHeight() + 6.0f);

        const float indicatorPos = detected ? jlimit(-1.0f, 1.0f, displayedCents / 50.0f) : 0.0f;
        const float indicatorX = track.getCentreX() + indicatorPos * (track.getWidth() * 0.5f - 8.0f);
        g.setColour(noteCol);
        g.fillEllipse(indicatorX - 5.0f, track.getCentreY() - 5.0f, 10.0f, 10.0f);
    };

    if (body.getWidth() >= 760.0f)
    {
        auto noteArea = body.removeFromLeft(juce::jlimit(180.0f, 288.0f, body.getWidth() * 0.25f));
        body.removeFromLeft(10.0f);
        auto stringArea = body.removeFromRight(juce::jlimit(214.0f, 330.0f, bounds.getWidth() * 0.23f));
        body.removeFromRight(10.0f);

        drawNoteBlock(noteArea);
        drawStageTunerTrace(g, body);
        drawStageStringChecklist(g, stringArea);
    }
    else
    {
        auto noteArea = body.removeFromTop(juce::jlimit(70.0f, 132.0f, body.getHeight() * 0.45f));
        body.removeFromTop(7.0f);
        auto stringArea = body.removeFromBottom(juce::jlimit(62.0f, 104.0f, body.getHeight() * 0.40f));
        body.removeFromBottom(7.0f);

        drawNoteBlock(noteArea);
        drawStageTunerTrace(g, body);
        drawStageStringChecklist(g, stringArea);
    }

    drawStageTunerRail(g, railArea);
}

void StageView::drawStageTunerTrace(Graphics& g, Rectangle<float> bounds)
{
    auto& fonts = FontManager::getInstance();
    auto& colours = ColourScheme::getInstance().colours;
    const auto tunerAccent = colours["Tuner Active Colour"];

    auto panel = bounds.reduced(1.0f);
    g.setColour(colours["Field Background"].withAlpha(0.52f));
    g.fillRoundedRectangle(panel, 12.0f);
    g.setColour(colours["Plugin Border"].interpolatedWith(tunerAccent, 0.08f).withAlpha(0.46f));
    g.drawRoundedRectangle(panel.reduced(0.5f), 12.0f, 0.8f);

    auto labelArea = panel.reduced(10.0f, 4.0f).removeFromTop(14.0f);
    g.setColour(colours["Text Colour"].withAlpha(0.50f));
    g.setFont(fonts.getMonoFont(9.2f));
    g.drawText("PITCH HISTORY", labelArea, Justification::centredLeft, true);

    auto trace = panel.reduced(12.0f, 16.0f).withTrimmedTop(8.0f);
    if (trace.getWidth() <= 8.0f || trace.getHeight() <= 8.0f)
        return;

    const auto centsToY = [trace](float cents)
    {
        return trace.getCentreY() - (jlimit(-kStagePitchTraceRangeCents, kStagePitchTraceRangeCents, cents) /
                                     kStagePitchTraceRangeCents) *
                                        trace.getHeight() * 0.5f;
    };

    const auto inTuneTop = centsToY(5.0f);
    const auto inTuneBottom = centsToY(-5.0f);
    g.setColour(tunerAccent.withAlpha(0.10f));
    g.fillRoundedRectangle(Rectangle<float>(trace.getX(), inTuneTop, trace.getWidth(), inTuneBottom - inTuneTop), 4.0f);

    const std::array<float, 7> guideCents{{-50.0f, -25.0f, -10.0f, 0.0f, 10.0f, 25.0f, 50.0f}};
    for (const auto cents : guideCents)
    {
        const auto y = centsToY(cents);
        g.setColour(cents == 0.0f ? colours["Text Colour"].withAlpha(0.30f)
                                  : colours["Plugin Border"].withAlpha(0.18f));
        g.drawLine(trace.getX(), y, trace.getRight(), y, cents == 0.0f ? 1.2f : 0.55f);
    }

    const float step = trace.getWidth() / static_cast<float>(kStagePitchTraceSize - 1);
    bool hasPrevious = false;
    Point<float> previous;
    float previousCents = 0.0f;
    float previousConfidence = 0.0f;
    int previousNote = -1;

    for (int logical = 0; logical < kStagePitchTraceSize; ++logical)
    {
        const int index = (stagePitchTraceWriteIndex + logical) % kStagePitchTraceSize;
        const auto confidence = stagePitchTraceConfidence[static_cast<size_t>(index)];
        const auto note = stagePitchTraceNote[static_cast<size_t>(index)];
        const auto cents = stagePitchTraceCents[static_cast<size_t>(index)];
        const bool active = confidence > 0.05f && note >= 0;
        const auto point = Point<float>(trace.getX() + step * static_cast<float>(logical), centsToY(cents));

        if (active && hasPrevious && previousNote == note &&
            std::abs(cents - previousCents) <= kStagePitchTraceConnectBreakCents)
        {
            const auto segmentConfidence = jmin(confidence, previousConfidence);
            g.setColour(getTuningColour((cents + previousCents) * 0.5f).withAlpha(0.20f + segmentConfidence * 0.70f));
            g.drawLine(previous.x, previous.y, point.x, point.y, 1.6f + segmentConfidence * 1.8f);
        }

        hasPrevious = active;
        previous = point;
        previousCents = cents;
        previousConfidence = confidence;
        previousNote = note;
    }

    g.setColour(colours["Text Colour"].withAlpha(0.38f));
    g.setFont(fonts.getMonoFont(8.2f));
    g.drawText("-50", trace.withWidth(28.0f).withY(trace.getBottom() - 11.0f), Justification::centredLeft, true);
    g.drawText("+50", trace.withWidth(28.0f).withX(trace.getRight() - 28.0f).withY(trace.getY()), Justification::centredRight, true);
}

void StageView::drawStageStringChecklist(Graphics& g, Rectangle<float> bounds)
{
    auto& fonts = FontManager::getInstance();
    auto& colours = ColourScheme::getInstance().colours;

    auto panel = bounds.reduced(1.0f);
    const int mask = tunerProcessor != nullptr ? tunerProcessor->getGuitarStringInTuneMask() : 0;
    const int currentIndex = tunerProcessor != nullptr ? tunerProcessor->getCurrentGuitarStringIndex() : -1;
    const float currentCents = tunerProcessor != nullptr ? tunerProcessor->getCurrentGuitarStringCents() : 0.0f;
    const bool allReady = (mask & ((1 << kStageStringCount) - 1)) == ((1 << kStageStringCount) - 1);
    const auto accent = allReady ? colours["Tuner Active Colour"] : colours["Plugin Border"];

    g.setColour(colours["Field Background"].interpolatedWith(accent, allReady ? 0.14f : 0.04f).withAlpha(0.54f));
    g.fillRoundedRectangle(panel, 12.0f);
    g.setColour(accent.withAlpha(allReady ? 0.62f : 0.38f));
    g.drawRoundedRectangle(panel.reduced(0.5f), 12.0f, 0.8f);

    auto inner = panel.reduced(9.0f, 6.0f);
    auto title = inner.removeFromTop(14.0f);
    g.setColour((allReady ? colours["Tuner Active Colour"] : colours["Text Colour"]).withAlpha(allReady ? 0.86f : 0.50f));
    g.setFont(fonts.getMonoFont(8.8f));
    g.drawText(allReady ? "ALL STRINGS READY" : "STRING CHECK", title, Justification::centredLeft, true);

    inner.removeFromTop(4.0f);
    const int columns = inner.getWidth() >= 230.0f ? 3 : 2;
    const int rows = (kStageStringCount + columns - 1) / columns;
    const float gap = 5.0f;
    const float tileW = (inner.getWidth() - gap * static_cast<float>(columns - 1)) / static_cast<float>(columns);
    const float tileH = (inner.getHeight() - gap * static_cast<float>(rows - 1)) / static_cast<float>(rows);

    for (int i = 0; i < kStageStringCount; ++i)
    {
        const int row = i / columns;
        const int col = i % columns;
        auto tile = Rectangle<float>(inner.getX() + static_cast<float>(col) * (tileW + gap),
                                     inner.getY() + static_cast<float>(row) * (tileH + gap), tileW, tileH);
        const bool isCurrent = i == currentIndex;
        const bool isReady = (mask & (1 << i)) != 0;
        const auto tileAccent = isCurrent ? getTuningColour(currentCents)
                                          : isReady ? colours["Tuner Active Colour"]
                                                    : colours["Text Colour"].withAlpha(0.32f);

        g.setColour(colours["Stage Panel Background"].interpolatedWith(tileAccent, isCurrent ? 0.18f : isReady ? 0.12f : 0.03f)
                        .withAlpha(0.72f));
        g.fillRoundedRectangle(tile, 8.0f);
        g.setColour(tileAccent.withAlpha(isCurrent ? 0.78f : isReady ? 0.58f : 0.26f));
        g.drawRoundedRectangle(tile.reduced(0.5f), 8.0f, isCurrent ? 1.2f : 0.8f);

        auto text = tile.reduced(6.0f, 2.0f);
        g.setColour(tileAccent.withAlpha(isCurrent || isReady ? 0.92f : 0.55f));
        g.setFont(fonts.getDisplayFont(juce::jlimit(10.0f, 15.0f, tile.getHeight() * 0.42f)));
        g.drawText(kStageStringLabels[static_cast<size_t>(i)], text.removeFromTop(tile.getHeight() * 0.50f),
                   Justification::centredLeft, true);

        String status = "--";
        if (isReady)
            status = "OK";
        if (isCurrent)
            status = (currentCents >= 0.0f ? "+" : "") + String(static_cast<int>(std::round(currentCents)));
        g.setFont(fonts.getMonoFont(juce::jlimit(8.0f, 11.5f, tile.getHeight() * 0.32f)));
        g.drawText(status, text, Justification::centredLeft, true);
    }
}

void StageView::drawStageTunerRail(Graphics& g, Rectangle<float> bounds)
{
    auto& fonts = FontManager::getInstance();
    auto& colours = ColourScheme::getInstance().colours;
    const bool detected = tunerProcessor != nullptr && tunerProcessor->isPitchDetected() && detectedNote >= 0;
    const bool inTune = detected && std::abs(displayedCents) < 3.0f;
    const auto statusColour =
        detected ? (inTune ? colours["Tuner Active Colour"] : std::abs(displayedCents) < 14.0f
                                                        ? colours["Warning Colour"]
                                                        : colours["Danger Colour"])
                 : colours["Text Colour"].withAlpha(0.42f);
    const String statusText = detected ? (inTune ? "IN TUNE"
                                                 : ((displayedCents >= 0.0f ? "+" : "") +
                                                    String(static_cast<int>(std::round(displayedCents))) + " CENTS"))
                                       : "WAITING";

    auto rail = bounds.reduced(1.0f);
    g.setColour(colours["Field Background"].withAlpha(0.48f));
    g.fillRoundedRectangle(rail, 10.0f);
    g.setColour(colours["Plugin Border"].withAlpha(0.32f));
    g.drawRoundedRectangle(rail.reduced(0.5f), 10.0f, 0.75f);

    auto inner = rail.reduced(8.0f, 4.0f);
    const auto drawPill = [&](Rectangle<float> pill, const String& text, Colour accent, float blend)
    {
        g.setColour(colours["Stage Panel Background"].interpolatedWith(accent, blend).withAlpha(0.82f));
        g.fillRoundedRectangle(pill, 7.0f);
        g.setColour(accent.withAlpha(0.48f));
        g.drawRoundedRectangle(pill.reduced(0.5f), 7.0f, 0.75f);
        g.setColour(colours["Text Colour"].withAlpha(0.82f));
        g.setFont(fonts.getMonoFont(juce::jlimit(8.4f, 11.5f, pill.getHeight() * 0.44f)));
        g.drawText(text, pill.reduced(6.0f, 0.0f), Justification::centred, true);
    };

    if (inner.getWidth() < 320.0f)
    {
        drawPill(inner, statusText, statusColour, detected ? 0.16f : 0.04f);
        return;
    }

    auto status = inner.removeFromLeft(juce::jlimit(138.0f, 230.0f, inner.getWidth() * 0.28f));
    inner.removeFromLeft(6.0f);
    auto signal = inner.removeFromLeft(juce::jlimit(142.0f, 235.0f, inner.getWidth() * 0.36f));
    inner.removeFromLeft(6.0f);
    auto reference = inner.removeFromLeft(juce::jlimit(72.0f, 118.0f, inner.getWidth() * 0.38f));
    inner.removeFromLeft(6.0f);
    auto response = inner;

    drawPill(status, statusText, statusColour, detected ? 0.16f : 0.04f);

    const auto confidence = juce::jlimit(0.0f, 1.0f, displayedConfidence);
    g.setColour(colours["Stage Panel Background"].withAlpha(0.80f));
    g.fillRoundedRectangle(signal, 7.0f);
    g.setColour(colours["Plugin Border"].withAlpha(0.36f));
    g.drawRoundedRectangle(signal.reduced(0.5f), 7.0f, 0.75f);
    auto signalInner = signal.reduced(8.0f, 0.0f);
    auto signalLabel = signalInner.removeFromLeft(54.0f);
    auto signalValue = signalInner.removeFromRight(38.0f);
    auto track = signalInner.reduced(0.0f, signalInner.getHeight() * 0.34f);
    g.setColour(colours["Plugin Border"].withAlpha(0.30f));
    g.fillRoundedRectangle(track, 3.0f);
    g.setColour(colours["Tuner Active Colour"].interpolatedWith(colours["Warning Colour"], 1.0f - confidence).withAlpha(0.76f));
    g.fillRoundedRectangle(track.withWidth(track.getWidth() * confidence), 3.0f);
    g.setColour(colours["Text Colour"].withAlpha(0.58f));
    g.setFont(fonts.getMonoFont(9.0f));
    g.drawText("SIGNAL", signalLabel, Justification::centredLeft, true);
    g.drawText(String(roundToInt(confidence * 100.0f)) + "%", signalValue, Justification::centredRight, true);

    const auto referenceText =
        tunerProcessor != nullptr ? "A=" + String(tunerProcessor->getReferenceA4Hz(), 0) : String("A=440");
    const auto responseMode =
        tunerProcessor != nullptr ? tunerProcessor->getResponseMode() : TunerProcessor::ResponseMode::Stable;
    drawPill(reference, referenceText, colours["Plugin Border"], 0.07f);
    drawPill(response, responseMode == TunerProcessor::ResponseMode::Fast ? "FAST" : "STABLE",
             colours["Tuner Active Colour"], responseMode == TunerProcessor::ResponseMode::Fast ? 0.18f : 0.10f);
}

void StageView::pushStageTunerTraceSample()
{
    if (tunerProcessor == nullptr)
        return;

    const auto index = static_cast<size_t>(stagePitchTraceWriteIndex);
    if (tunerProcessor->isPitchDetected())
    {
        stagePitchTraceCents[index] = tunerProcessor->getCentsDeviation();
        stagePitchTraceConfidence[index] = tunerProcessor->getDetectedConfidence();
        stagePitchTraceNote[index] = tunerProcessor->getDetectedNote();
    }
    else
    {
        stagePitchTraceCents[index] = 0.0f;
        stagePitchTraceConfidence[index] = 0.0f;
        stagePitchTraceNote[index] = -1;
    }

    stagePitchTraceWriteIndex = (stagePitchTraceWriteIndex + 1) % kStagePitchTraceSize;
}

void StageView::drawSafetyBar(Graphics& g, Rectangle<float> bounds)
{
    auto& fonts = FontManager::getInstance();
    auto& colours = ColourScheme::getInstance().colours;
    const auto metrics = StageLayout::calculateMetrics(getWidth(), getHeight(), showTuner);

    g.setColour(colours["Window Background"].darker(0.12f).withAlpha(0.78f));
    g.fillRect(bounds);
    g.setColour(colours["Plugin Border"].withAlpha(0.50f));
    g.drawHorizontalLine(juce::roundToInt(bounds.getY()), bounds.getX(), bounds.getRight());

    auto labelArea = bounds.reduced((float)metrics.margin, 0.0f).removeFromTop(metrics.meterTopOffset - 2.0f);
    g.setFont(fonts.getDisplayFont(metrics.safetyFontHeight));
    g.setColour(colours["Text Colour"].withAlpha(0.50f));
    g.drawText("SAFETY", labelArea.withWidth(70.0f), Justification::centredLeft);

    const auto meterGroupX = metrics.meterStartX + metrics.meterLabelWidth + 6.0f;
    auto meterLabelArea = Rectangle<float>(meterGroupX + 72.0f, labelArea.getY(),
                                           metrics.meterWidth * 2.0f + metrics.meterSpacing, labelArea.getHeight());
    g.setColour(colours["Accent Colour"].withAlpha(0.48f));
    g.drawText("MASTER BUS", meterLabelArea, Justification::centredLeft);

    auto meterBack = Rectangle<float>(metrics.meterStartX - 8.0f, bounds.getY() + metrics.meterTopOffset - 7.0f,
                                      metrics.meterLabelWidth * 2.0f + metrics.meterWidth * 2.0f +
                                          metrics.meterSpacing + metrics.panicButtonWidth + 28.0f,
                                      metrics.sliderTopOffset + metrics.sliderHeight - metrics.meterTopOffset + 16.0f);
    g.setColour(colours["Stage Panel Background"].withAlpha(0.26f));
    g.fillRoundedRectangle(meterBack, 12.0f);
    g.setColour(colours["Plugin Border"].withAlpha(0.24f));
    g.drawRoundedRectangle(meterBack.reduced(0.5f), 12.0f, 1.0f);

    auto panicGlow = panicButton != nullptr ? panicButton->getBounds().toFloat().expanded(5.0f)
                                            : Rectangle<float>();
    if (!panicGlow.isEmpty())
    {
        g.setColour(colours["Danger Colour"].withAlpha(0.16f));
        g.fillRoundedRectangle(panicGlow, 15.0f);
        g.setColour(colours["Danger Colour"].withAlpha(0.28f));
        g.drawRoundedRectangle(panicGlow.reduced(0.5f), 15.0f, 1.0f);
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
        const int switcherH = juce::jmin(32, utilityButtonHeight);
        const int switcherX = margin + metrics.stageBrandWidth + margin;
        themeSwitcher->setBounds(switcherX, headerButtonY + juce::roundToInt((utilityButtonHeight - switcherH) * 0.5f),
                                 metrics.stageThemeSwitcherWidth, switcherH);
        themeSwitcher->setVisible(metrics.showStageThemeSwitcher);
        themeSwitcher->toFront(false);
    }

    exitButton->setBounds(bounds.getWidth() - utilityButtonWidth - margin, headerButtonY, utilityButtonWidth,
                          utilityButtonHeight);
    tunerToggleButton->setBounds(bounds.getWidth() - utilityButtonWidth * 2 - margin * 2, headerButtonY,
                                 utilityButtonWidth, utilityButtonHeight);

    const int modeButtonW = metrics.modeButtonWidth;
    const int modeButtonGap = metrics.modeButtonGap;
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
