/*
  ==============================================================================

    TunerControl.cpp
    Chromatic tuner with needle, pitch-drift, and string-reference displays
    Uses project fonts (Space Grotesk + JetBrains Mono) and drawn graphics

  ==============================================================================
*/

#include "TunerControl.h"

#include "ColourScheme.h"
#include "FontManager.h"
#include "TunerProcessor.h"

namespace
{
class TunerModeButtonLookAndFeel final : public LookAndFeel_V4
{
  public:
    void drawButtonBackground(Graphics&, Button&, const Colour&, bool, bool) override {}

    void drawButtonText(Graphics& g, TextButton& button, bool, bool) override
    {
        const auto text = button.findColour(button.getToggleState() ? TextButton::textColourOnId
                                                                    : TextButton::textColourOffId);
        g.setColour(text);
        g.setFont(FontManager::getInstance().getBadgeFont().withHeight(9.4f));
        g.drawFittedText(button.getButtonText(), button.getLocalBounds().reduced(3, 1), Justification::centred, 1);
    }
};

TunerModeButtonLookAndFeel tunerModeButtonLookAndFeel;
} // namespace

//==============================================================================
TunerControl::TunerControl(TunerProcessor* processor) : tunerProcessor(processor)
{
    needleModeButton = std::make_unique<TextButton>("NEEDLE");
    needleModeButton->setLookAndFeel(&tunerModeButtonLookAndFeel);
    needleModeButton->setTooltip("Needle tuner view");
    needleModeButton->addListener(this);
    addAndMakeVisible(needleModeButton.get());

    driftModeButton = std::make_unique<TextButton>("DRIFT");
    driftModeButton->setLookAndFeel(&tunerModeButtonLookAndFeel);
    driftModeButton->setTooltip("Pitch drift view");
    driftModeButton->addListener(this);
    addAndMakeVisible(driftModeButton.get());

    sixStringModeButton = std::make_unique<TextButton>("STRINGS");
    sixStringModeButton->setLookAndFeel(&tunerModeButtonLookAndFeel);
    sixStringModeButton->setTooltip("Six-string guitar reference view");
    sixStringModeButton->addListener(this);
    addAndMakeVisible(sixStringModeButton.get());

    bypassButton = std::make_unique<TextButton>("BYPASS");
    bypassButton->setLookAndFeel(&tunerModeButtonLookAndFeel);
    bypassButton->setTooltip("Bypass tuner processing");
    bypassButton->setClickingTogglesState(true);
    bypassButton->addListener(this);
    addAndMakeVisible(bypassButton.get());
    updateModeButtons();

    // 60 fps for smooth animation
    startTimerHz(60);

    setSize(360, 276);
}

TunerControl::~TunerControl()
{
    if (needleModeButton != nullptr)
        needleModeButton->setLookAndFeel(nullptr);
    if (driftModeButton != nullptr)
        driftModeButton->setLookAndFeel(nullptr);
    if (sixStringModeButton != nullptr)
        sixStringModeButton->setLookAndFeel(nullptr);
    if (bypassButton != nullptr)
        bypassButton->setLookAndFeel(nullptr);
    stopTimer();
}

//==============================================================================
void TunerControl::setBypassController(std::function<bool()> stateGetter, std::function<void(bool)> stateSetter)
{
    getBypassState = std::move(stateGetter);
    setBypassState = std::move(stateSetter);
    if (bypassButton != nullptr && getBypassState)
        bypassButton->setToggleState(getBypassState(), dontSendNotification);
}

//==============================================================================
void TunerControl::buttonClicked(Button* button)
{
    if (button == needleModeButton.get())
    {
        currentMode = TunerMode::Needle;
        updateModeButtons();
        repaint();
    }
    else if (button == driftModeButton.get())
    {
        currentMode = TunerMode::PitchDrift;
        updateModeButtons();
        repaint();
    }
    else if (button == sixStringModeButton.get())
    {
        currentMode = TunerMode::SixString;
        updateModeButtons();
        repaint();
    }
    else if (button == bypassButton.get())
    {
        if (setBypassState)
            setBypassState(bypassButton->getToggleState());
        repaint();
    }
}

//==============================================================================
void TunerControl::timerCallback()
{
    if (tunerProcessor == nullptr)
        return;

    float targetCents = tunerProcessor->getCentsDeviation();
    displayedCents += (targetCents - displayedCents) * NEEDLE_SMOOTHING;

    float targetAngle = jlimit(-50.0f, 50.0f, displayedCents) * 0.9f;
    needleAngle += (targetAngle - needleAngle) * NEEDLE_SMOOTHING;

    float absCents = std::abs(displayedCents);
    float targetGlow = (absCents < 5.0f) ? 1.0f - (absCents / 5.0f) : 0.0f;
    glowIntensity += (targetGlow - glowIntensity) * GLOW_SMOOTHING;

    if (currentMode == TunerMode::PitchDrift)
    {
        driftRotation = tunerProcessor->getDriftPhase() * MathConstants<float>::twoPi * DRIFT_BANDS;
    }

    if (bypassButton != nullptr && getBypassState)
        bypassButton->setToggleState(getBypassState(), dontSendNotification);

    repaint();
}

//==============================================================================
void TunerControl::paint(Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    drawTunerGlassPanel(g, bounds);

    auto area = bounds.reduced(8, 6);

    auto headerArea = area.removeFromTop(34);
    drawTunerHeader(g, headerArea);

    area.removeFromTop(9);

    auto modeArea = area.removeFromTop(29);
    drawModeSegmentedControl(g, modeArea);

    area.removeFromTop(4);

    auto noteArea = area.removeFromTop(50);
    drawNoteDisplay(g, noteArea);

    auto coarseArea = area.removeFromTop(18);
    drawCoarseDeviationStrip(g, coarseArea);

    area.removeFromTop(2);

    auto meterArea = area.removeFromTop(66);
    if (currentMode == TunerMode::Needle)
        drawNeedleMeter(g, meterArea);
    else if (currentMode == TunerMode::SixString)
        drawSixStringDisplay(g, meterArea.expanded(0.0f, 10.0f));
    else
        drawPitchDriftDisc(g, meterArea);

    auto ledArea = area.removeFromTop(16);
    if (currentMode == TunerMode::Needle)
        drawLedIndicators(g, ledArea);

    auto statusArea = area;
    drawStatusBadge(g, statusArea);
}

//==============================================================================
void TunerControl::drawTunerGlassPanel(Graphics& g, Rectangle<float> bounds)
{
    auto& colours = ColourScheme::getInstance().colours;
    auto tunerAccent = colours["Tuner Active Colour"];
    const auto pluginBase = colours["Plugin Background"];
    const auto fieldBase = colours["Field Background"];

    ColourGradient panelFill(pluginBase.interpolatedWith(tunerAccent, 0.10f).brighter(0.06f), bounds.getX(),
                             bounds.getY(), pluginBase.interpolatedWith(fieldBase, 0.20f).darker(0.16f),
                             bounds.getX(), bounds.getBottom(), false);
    panelFill.addColour(0.38, pluginBase.interpolatedWith(tunerAccent, 0.055f));
    panelFill.addColour(0.72, pluginBase.interpolatedWith(fieldBase, 0.12f).darker(0.06f));
    g.setGradientFill(panelFill);
    g.fillRoundedRectangle(bounds, 8.0f);

    g.setColour(colours["Window Background"].darker(0.35f).withAlpha(0.18f));
    g.fillRoundedRectangle(bounds.reduced(4.0f).translated(0.0f, 1.5f), 7.0f);

    g.setColour(colours["Text Colour"].withAlpha(0.055f));
    g.drawLine(bounds.getX() + 9.0f, bounds.getY() + 3.0f, bounds.getRight() - 9.0f, bounds.getY() + 3.0f,
               1.0f);

    g.setColour(tunerAccent.withAlpha(0.38f));
    g.drawRoundedRectangle(bounds.reduced(0.5f), 8.0f, 1.1f);
    g.setColour(colours["Plugin Border"].withAlpha(0.44f));
    g.drawRoundedRectangle(bounds.reduced(2.0f), 6.5f, 0.65f);
}

void TunerControl::drawTunerHeader(Graphics& g, Rectangle<float> bounds)
{
    auto& colours = ColourScheme::getInstance().colours;
    auto& fonts = FontManager::getInstance();
    const auto tunerAccent = colours["Tuner Active Colour"];
    const bool bypassed = bypassButton != nullptr && bypassButton->getToggleState();
    const bool detected = tunerProcessor != nullptr && tunerProcessor->isPitchDetected();
    const bool showStatePill = bypassed || detected;

    ColourGradient headerFill(colours["Plugin Background"].interpolatedWith(tunerAccent, 0.20f).brighter(0.06f),
                              bounds.getX(), bounds.getY(),
                              colours["Plugin Background"].interpolatedWith(colours["Field Background"], 0.16f),
                              bounds.getX(), bounds.getBottom(), false);
    headerFill.addColour(0.52, colours["Plugin Background"].interpolatedWith(tunerAccent, 0.11f));
    g.setGradientFill(headerFill);
    g.fillRoundedRectangle(bounds, 7.0f);

    g.setColour(colours["Text Colour"].withAlpha(0.06f));
    for (float y = bounds.getY() + 5.0f; y < bounds.getBottom() - 3.0f; y += 4.0f)
        g.drawHorizontalLine(roundToInt(y), bounds.getX() + 10.0f, bounds.getRight() - 10.0f);

    g.setColour(tunerAccent.withAlpha(0.38f));
    g.drawRoundedRectangle(bounds.reduced(0.5f), 7.0f, 0.9f);
    g.setColour(tunerAccent.withAlpha(0.72f));
    g.fillRoundedRectangle(bounds.withHeight(2.0f).withY(bounds.getBottom() - 2.0f).reduced(9.0f, 0.0f), 1.0f);

    auto icon = bounds.withWidth(24.0f).reduced(5.0f);
    g.setColour(tunerAccent.withAlpha(0.16f));
    g.fillEllipse(icon);
    g.setColour(tunerAccent.withAlpha(0.88f));
    g.fillEllipse(icon.reduced(4.5f));

    auto textArea = bounds.withTrimmedLeft(31.0f).withTrimmedRight(showStatePill ? 98.0f : 10.0f).reduced(0.0f, 4.0f);
    g.setColour(tunerAccent.brighter(0.18f));
    g.setFont(fonts.getBadgeFont().withHeight(10.5f));
    g.drawText("TUNER", textArea.removeFromTop(12.0f), Justification::centredLeft, true);

    const String statusText = bypassed ? "Bypassed" : detected ? getNoteName(tunerProcessor->getDetectedNote()) : "Waiting";
    g.setColour(colours["Text Colour"].withAlpha(0.88f));
    g.setFont(fonts.getSubheadingFont().withHeight(15.5f));
    g.drawText(statusText, textArea, Justification::centredLeft, true);

    if (bypassed || detected)
    {
        auto statePill = bounds.removeFromRight(88.0f).reduced(6.0f, 7.0f);
        const auto stateColour = bypassed ? colours["Danger Colour"] : tunerAccent;
        g.setColour(colours["Field Background"].interpolatedWith(stateColour, 0.13f));
        g.fillRoundedRectangle(statePill, 9.0f);
        g.setColour(stateColour.withAlpha(0.52f));
        g.drawRoundedRectangle(statePill.reduced(0.5f), 9.0f, 0.8f);

        auto led = Rectangle<float>(7.0f, 7.0f).withCentre({statePill.getX() + 12.0f, statePill.getCentreY()});
        g.setColour(stateColour.withAlpha(0.24f));
        g.fillEllipse(led.expanded(3.0f));
        g.setColour(stateColour);
        g.fillEllipse(led);

        g.setColour(colours["Field Background"].interpolatedWith(stateColour, 0.13f).contrasting(0.90f).withAlpha(0.72f));
        g.setFont(fonts.getBadgeFont().withHeight(10.0f));
        g.drawText(bypassed ? "BYPASS" : "ACTIVE", statePill.withTrimmedLeft(23.0f).withTrimmedRight(6.0f),
                   Justification::centredLeft, true);
    }
}

void TunerControl::drawModeSegmentedControl(Graphics& g, Rectangle<float> bounds)
{
    auto& colours = ColourScheme::getInstance().colours;
    auto tunerAccent = colours["Tuner Active Colour"];
    auto bypassPlate = bounds.removeFromRight(82.0f).reduced(1.0f);
    bounds.removeFromRight(4.0f);
    const auto plate = bounds.reduced(1.0f);

    ColourGradient plateFill(colours["Field Background"].interpolatedWith(tunerAccent, 0.055f).brighter(0.02f),
                             plate.getX(), plate.getY(),
                             colours["Field Background"].interpolatedWith(colours["Plugin Background"], 0.58f)
                                 .darker(0.16f),
                             plate.getX(), plate.getBottom(), false);
    plateFill.addColour(0.50, colours["Field Background"].interpolatedWith(colours["Plugin Background"], 0.45f));
    g.setGradientFill(plateFill);
    g.fillRoundedRectangle(plate, 7.0f);
    g.setColour(colours["Plugin Border"].interpolatedWith(tunerAccent, 0.14f).withAlpha(0.64f));
    g.drawRoundedRectangle(plate.reduced(0.5f), 7.0f, 0.9f);
    g.setColour(colours["Text Colour"].withAlpha(0.055f));
    g.drawLine(plate.getX() + 7.0f, plate.getY() + 1.0f, plate.getRight() - 7.0f, plate.getY() + 1.0f, 0.7f);

    const float segmentW = plate.getWidth() / 3.0f;
    for (int i = 1; i < 3; ++i)
    {
        const auto separatorX = plate.getX() + segmentW * static_cast<float>(i);
        g.setColour(colours["Plugin Border"].interpolatedWith(tunerAccent, 0.18f).withAlpha(0.42f));
        g.drawLine(separatorX, plate.getY() + 5.0f, separatorX, plate.getBottom() - 5.0f, 0.8f);
    }

    const int modeIndex = currentMode == TunerMode::Needle ? 0 : currentMode == TunerMode::PitchDrift ? 1 : 2;
    const auto selected = Rectangle<float>(plate.getX() + segmentW * (float)modeIndex, plate.getY(), segmentW,
                                           plate.getHeight())
                              .reduced(2.0f);
    ColourGradient selectedFill(tunerAccent.withAlpha(0.24f), selected.getX(), selected.getY(),
                                tunerAccent.darker(0.42f).withAlpha(0.28f), selected.getX(),
                                selected.getBottom(), false);
    g.setGradientFill(selectedFill);
    g.fillRoundedRectangle(selected, 5.0f);
    g.setColour(tunerAccent.withAlpha(0.42f));
    g.drawRoundedRectangle(selected.reduced(0.5f), 5.0f, 0.75f);
    g.setColour(tunerAccent.brighter(0.20f).withAlpha(0.38f));
    g.drawLine(selected.getX() + 5.0f, selected.getY() + 1.0f, selected.getRight() - 5.0f,
               selected.getY() + 1.0f, 0.7f);

    drawBypassPill(g, bypassPlate);
}

void TunerControl::drawBypassPill(Graphics& g, Rectangle<float> bounds)
{
    auto& colours = ColourScheme::getInstance().colours;
    const bool bypassed = bypassButton != nullptr && bypassButton->getToggleState();
    const auto accent = bypassed ? colours["Danger Colour"] : colours["Plugin Border"];
    const auto fillBase = colours["Field Background"].interpolatedWith(accent, bypassed ? 0.18f : 0.08f);

    g.setColour(fillBase.darker(0.08f));
    g.fillRoundedRectangle(bounds, 7.0f);
    g.setColour(accent.withAlpha(bypassed ? 0.58f : 0.42f));
    g.drawRoundedRectangle(bounds.reduced(0.5f), 7.0f, 0.9f);
}

void TunerControl::drawSixStringDisplay(Graphics& g, Rectangle<float> bounds)
{
    auto& colours = ColourScheme::getInstance().colours;
    auto& fonts = FontManager::getInstance();
    const StringArray strings{"E2", "A2", "D3", "G3", "B3", "E4"};

    const bool detected = tunerProcessor != nullptr && tunerProcessor->isPitchDetected();
    const int tunedMask = tunerProcessor != nullptr ? tunerProcessor->getGuitarStringInTuneMask() : 0;
    const int currentString = detected && tunerProcessor != nullptr ? tunerProcessor->getCurrentGuitarStringIndex() : -1;
    const float stringCents = detected && tunerProcessor != nullptr ? tunerProcessor->getCurrentGuitarStringCents() : 0.0f;
    const bool allStringsReady = (tunedMask & 0x3f) == 0x3f;
    int readyCount = 0;
    for (int i = 0; i < 6; ++i)
        if ((tunedMask & (1 << i)) != 0)
            ++readyCount;

    auto panel = bounds.reduced(2.0f, 0.0f);
    g.setColour(colours["Field Background"].withAlpha(0.74f));
    g.fillRoundedRectangle(panel, 8.0f);
    g.setColour(colours["Plugin Border"].withAlpha(0.48f));
    g.drawRoundedRectangle(panel.reduced(0.5f), 8.0f, 0.8f);

    auto row = panel.reduced(8.0f, 6.0f);
    auto statusRow = row.removeFromTop(16.0f);
    row.removeFromTop(3.0f);
    const auto statusAccent = allStringsReady ? colours["Tuner Active Colour"] : colours["Plugin Border"];
    const auto statusText = allStringsReady ? String("ALL STRINGS READY") : String(readyCount) + "/6 READY";
    auto statusPill = statusRow.withSizeKeepingCentre(jmin(132.0f, statusRow.getWidth()), statusRow.getHeight());
    g.setColour(colours["Field Background"].interpolatedWith(statusAccent, allStringsReady ? 0.18f : 0.08f));
    g.fillRoundedRectangle(statusPill, 6.0f);
    g.setColour(statusAccent.withAlpha(allStringsReady ? 0.58f : 0.34f));
    g.drawRoundedRectangle(statusPill.reduced(0.5f), 6.0f, 0.8f);
    g.setColour(colours["Text Colour"].withAlpha(allStringsReady ? 0.88f : 0.58f));
    g.setFont(fonts.getBadgeFont().withHeight(9.2f));
    g.drawText(statusText, statusPill.reduced(5.0f, 0.0f), Justification::centred, true);

    const float slotW = row.getWidth() / 6.0f;
    for (int i = 0; i < 6; ++i)
    {
        auto slot = row.removeFromLeft(slotW).reduced(2.0f, 0.0f);
        const bool isCurrentString = currentString == i;
        const bool isReadyString = (tunedMask & (1 << i)) != 0;
        const auto currentColour = getTuningColour(stringCents);
        const auto colour = isCurrentString ? currentColour
                                            : isReadyString ? colours["Tuner Active Colour"]
                                                            : colours["Text Colour"].withAlpha(0.30f);

        auto track = Rectangle<float>(16.0f, jmin(64.0f, slot.getHeight() - 17.0f))
                         .withCentre({slot.getCentreX(), slot.getCentreY() - 4.0f});
        g.setColour(colours["Field Background"].interpolatedWith(colour, isCurrentString ? 0.14f : isReadyString ? 0.10f : 0.04f));
        g.fillRoundedRectangle(track, 5.0f);
        g.setColour(colours["Plugin Border"].interpolatedWith(colour, isCurrentString ? 0.36f : isReadyString ? 0.24f : 0.08f).withAlpha(0.70f));
        g.drawRoundedRectangle(track.reduced(0.5f), 5.0f, 0.8f);

        auto zone = Rectangle<float>(track.getX(), track.getCentreY() - 6.5f, track.getWidth(), 13.0f);
        g.setColour(colour.withAlpha(isCurrentString ? 0.24f : isReadyString ? 0.18f : 0.06f));
        g.fillRoundedRectangle(zone, 2.5f);
        g.setColour(colour.withAlpha(isCurrentString ? 0.48f : isReadyString ? 0.32f : 0.13f));
        g.drawLine(zone.getX() + 1.0f, zone.getY(), zone.getRight() - 1.0f, zone.getY(), 0.7f);
        g.drawLine(zone.getX() + 1.0f, zone.getBottom(), zone.getRight() - 1.0f, zone.getBottom(), 0.7f);

        if (isReadyString && !isCurrentString)
        {
            auto readyDot = Rectangle<float>(8.0f, 8.0f).withCentre(track.getCentre());
            g.setColour(colour.withAlpha(0.18f));
            g.fillRoundedRectangle(readyDot.expanded(3.0f), 5.0f);
            g.setColour(colour.withAlpha(0.82f));
            g.fillRoundedRectangle(readyDot, 3.5f);
        }

        if (isCurrentString)
        {
            const float y = jmap(jlimit(-50.0f, 50.0f, stringCents), -50.0f, 50.0f, track.getBottom() - 4.0f,
                                 track.getY() + 4.0f);
            auto dot = Rectangle<float>(12.0f, 6.0f).withCentre({track.getCentreX(), y});
            g.setColour(currentColour.withAlpha(0.24f));
            g.fillRoundedRectangle(dot.expanded(4.0f, 3.0f), 5.0f);
            g.setColour(currentColour);
            g.fillRoundedRectangle(dot, 3.0f);
        }

        g.setColour(colour.withAlpha(isCurrentString ? 0.92f : isReadyString ? 0.78f : 0.48f));
        g.setFont(fonts.getMonoFont(9.5f));
        g.drawText(strings[i], slot.removeFromBottom(14.0f), Justification::centred, true);
    }
}

void TunerControl::drawNoteGlyph(Graphics& g, Rectangle<float> bounds, const String& noteName, Colour noteColour)
{
    auto& colours = ColourScheme::getInstance().colours;
    auto& fonts = FontManager::getInstance();

    if (noteName == "---" || noteName.isEmpty())
    {
        g.setColour(colours["Text Colour"].withAlpha(0.25f));
        g.setFont(fonts.getDisplayFont(44.0f));
        g.drawText("---", bounds, Justification::centred);
        return;
    }

    String pitch = noteName;
    String octave;
    while (pitch.isNotEmpty() && CharacterFunctions::isDigit(pitch.getLastCharacter()))
    {
        octave = String::charToString(pitch.getLastCharacter()) + octave;
        pitch = pitch.dropLastCharacters(1);
    }

    String root = pitch.substring(0, 1);
    String accidental = pitch.substring(1);
    auto centre = bounds.getCentre();
    auto noteBounds = bounds.withSizeKeepingCentre(92.0f, bounds.getHeight());

    g.setColour(colours["Window Background"].darker(0.35f).withAlpha(0.38f));
    g.setFont(fonts.getDisplayFont(50.0f));
    g.drawText(root, noteBounds.translated(1.5f, 1.5f), Justification::centred);

    g.setColour(noteColour);
    g.drawText(root, noteBounds, Justification::centred);

    if (accidental.isNotEmpty())
    {
        auto accidentalBounds = Rectangle<float>(centre.x + 22.0f, bounds.getY() + 9.0f, 22.0f, 20.0f);
        g.setColour(noteColour.withAlpha(0.86f));
        g.setFont(fonts.getSubheadingFont().withHeight(18.0f));
        g.drawText(accidental, accidentalBounds, Justification::centredLeft, true);
    }

    if (octave.isNotEmpty())
    {
        auto octaveBadge = Rectangle<float>(centre.x + 25.0f, bounds.getBottom() - 23.0f, 24.0f, 17.0f);
        g.setColour(colours["Field Background"].interpolatedWith(noteColour, 0.10f));
        g.fillRoundedRectangle(octaveBadge, 6.0f);
        g.setColour(noteColour.withAlpha(0.42f));
        g.drawRoundedRectangle(octaveBadge.reduced(0.5f), 6.0f, 0.8f);
        g.setColour(colours["Text Colour"].withAlpha(0.78f));
        g.setFont(fonts.getMonoFont(10.0f));
        g.drawText(octave, octaveBadge, Justification::centred, true);
    }

    const bool detected = tunerProcessor != nullptr && tunerProcessor->isPitchDetected();
    if (detected && std::abs(displayedCents) >= 3.0f)
    {
        const bool flat = displayedCents < 0.0f;
        auto symbolArea = flat ? Rectangle<float>(bounds.getX() + 42.0f, centre.y - 11.0f, 24.0f, 24.0f)
                               : Rectangle<float>(bounds.getRight() - 66.0f, centre.y - 11.0f, 24.0f, 24.0f);
        g.setColour(noteColour.withAlpha(0.18f));
        g.fillEllipse(symbolArea.expanded(2.0f));
        if (flat)
            drawFlatSymbol(g, symbolArea.getCentreX(), symbolArea.getCentreY(), 15.0f, noteColour.withAlpha(0.88f));
        else
            drawSharpSymbol(g, symbolArea.getCentreX(), symbolArea.getCentreY(), 15.0f, noteColour.withAlpha(0.88f));
    }
}

void TunerControl::drawNeedleArcBackdrop(Graphics& g, Point<float> centre, float radius)
{
    auto& colours = ColourScheme::getInstance().colours;
    auto tunerAccent = colours["Tuner Active Colour"];

    auto makeArc = [&](float arcRadius)
    {
        Path arc;
        for (int d = -140; d <= -40; d += 4)
        {
            const float radians = degreesToRadians((float)d);
            const float x = centre.x + std::cos(radians) * arcRadius;
            const float y = centre.y + std::sin(radians) * arcRadius;
            if (d == -140)
                arc.startNewSubPath(x, y);
            else
                arc.lineTo(x, y);
        }
        return arc;
    };

    const auto arc = makeArc(radius - 2.0f);
    g.setColour(colours["Field Background"].darker(0.20f).withAlpha(0.82f));
    g.strokePath(arc, PathStrokeType(5.0f, PathStrokeType::curved, PathStrokeType::rounded));
    g.setColour(colours["Plugin Border"].interpolatedWith(tunerAccent, 0.16f).withAlpha(0.54f));
    g.strokePath(arc, PathStrokeType(1.2f, PathStrokeType::curved, PathStrokeType::rounded));

    auto centreMark = makeArc(radius - 17.0f);
    g.setColour(tunerAccent.withAlpha(0.11f));
    g.strokePath(centreMark, PathStrokeType(1.0f, PathStrokeType::curved, PathStrokeType::rounded));
}

void TunerControl::drawCoarseDeviationStrip(Graphics& g, Rectangle<float> bounds)
{
    auto& colours = ColourScheme::getInstance().colours;
    auto tunerAccent = colours["Tuner Active Colour"];
    auto track = bounds.reduced(25.0f, 5.0f);

    g.setColour(colours["Field Background"].darker(0.18f));
    g.fillRoundedRectangle(track, 3.0f);
    g.setColour(colours["Plugin Border"].withAlpha(0.55f));
    g.drawRoundedRectangle(track.reduced(0.5f), 3.0f, 0.75f);

    const float cents = tunerProcessor != nullptr && tunerProcessor->isPitchDetected() ? displayedCents : 0.0f;
    const bool inTune = tunerProcessor != nullptr && tunerProcessor->isPitchDetected() && std::abs(cents) < 3.0f;
    const bool close = tunerProcessor != nullptr && tunerProcessor->isPitchDetected() && std::abs(cents) < 14.0f;
    const float normalized = jlimit(0.0f, 1.0f, (jlimit(-50.0f, 50.0f, cents) + 50.0f) / 100.0f);
    const float dotX = track.getX() + track.getWidth() * normalized;
    const auto dotColour = tunerProcessor != nullptr && tunerProcessor->isPitchDetected()
                               ? (inTune ? tunerAccent : close ? colours["Warning Colour"] : colours["Danger Colour"])
                               : colours["Text Colour"].withAlpha(0.34f);

    g.setColour(colours["Text Colour"].withAlpha(0.34f));
    g.fillRoundedRectangle(track.getCentreX() - 0.75f, track.getY() - 3.0f, 1.5f, track.getHeight() + 6.0f, 0.75f);

    g.setColour(dotColour.withAlpha(inTune ? 0.30f : 0.16f));
    g.fillEllipse(dotX - 7.0f, track.getCentreY() - 7.0f, 14.0f, 14.0f);
    g.setColour(dotColour);
    g.fillEllipse(dotX - 4.5f, track.getCentreY() - 4.5f, 9.0f, 9.0f);

    g.setFont(FontManager::getInstance().getMonoFont(8.0f));
    g.setColour(colours["Text Colour"].withAlpha(0.44f));
    g.drawText("b", bounds.withWidth(18.0f), Justification::centred);
    g.drawText("#", bounds.withX(bounds.getRight() - 18.0f).withWidth(18.0f), Justification::centred);
}

void TunerControl::drawStatusBadge(Graphics& g, Rectangle<float> bounds)
{
    auto& colours = ColourScheme::getInstance().colours;
    auto& fonts = FontManager::getInstance();
    auto tunerAccent = colours["Tuner Active Colour"];

    const bool detected = tunerProcessor != nullptr && tunerProcessor->isPitchDetected();
    const float cents = detected ? displayedCents : 0.0f;
    const bool inTune = detected && std::abs(cents) < 3.0f;
    const auto statusColour =
        detected ? (inTune ? tunerAccent : std::abs(cents) < 14.0f ? colours["Warning Colour"] : colours["Danger Colour"])
                 : colours["Text Colour"].withAlpha(0.42f);

    auto badge = bounds.withTrimmedRight(76.0f).reduced(0.0f, 1.0f);
    const String statusText = detected ? (inTune ? "In Tune"
                                                 : ((cents >= 0.0f ? "+" : "") + String(static_cast<int>(std::round(cents))) +
                                                    " cents"))
                                       : "Waiting for signal";

    g.setColour(colours["Field Background"].interpolatedWith(statusColour, detected ? 0.10f : 0.03f));
    g.fillRoundedRectangle(badge, 6.0f);
    g.setColour(statusColour.withAlpha(detected ? 0.48f : 0.20f));
    g.drawRoundedRectangle(badge.reduced(0.5f), 6.0f, 0.8f);

    auto dot = Rectangle<float>(6.0f, 6.0f).withCentre({badge.getX() + 11.0f, badge.getCentreY()});
    g.setColour(statusColour.withAlpha(inTune ? 0.24f : 0.10f));
    g.fillEllipse(dot.expanded(3.0f));
    g.setColour(statusColour);
    g.fillEllipse(dot);

    g.setColour(detected ? colours["Text Colour"].withAlpha(0.84f) : colours["Text Colour"].withAlpha(0.44f));
    g.setFont(fonts.getMonoFont(10.0f));
    g.drawText(statusText, badge.withTrimmedLeft(21.0f).withTrimmedRight(6.0f), Justification::centredLeft, true);

    g.setColour(colours["Text Colour"].withAlpha(0.46f));
    g.setFont(fonts.getMonoFont(9.0f));
    const auto referenceText =
        tunerProcessor != nullptr ? "A=" + String(tunerProcessor->getReferenceA4Hz(), 0) : String("A=440");
    g.drawText(referenceText, bounds.removeFromRight(70.0f), Justification::centredRight, true);
}

//==============================================================================
void TunerControl::drawNoteDisplay(Graphics& g, Rectangle<float> bounds)
{
    auto& colours = ColourScheme::getInstance().colours;
    auto& fonts = FontManager::getInstance();

    if (tunerProcessor == nullptr)
        return;

    auto centre = bounds.getCentre();

    if (!tunerProcessor->isPitchDetected())
    {
        g.setColour(colours["Text Colour"].withAlpha(0.25f));
        g.setFont(fonts.getDisplayFont(44.0f));
        g.drawText("---", bounds, Justification::centred);
        return;
    }

    int midiNote = tunerProcessor->getDetectedNote();
    String noteName = getNoteName(midiNote);

    // Glowing halo when in tune
    if (glowIntensity > 0.02f)
    {
        float glowSize = 70 + glowIntensity * 25;
        for (int i = 3; i >= 0; --i)
        {
            float expand = i * 8.0f;
            float alpha = glowIntensity * (0.12f - i * 0.03f);
            g.setColour(colours["Success Colour"].withAlpha(alpha));
            g.fillEllipse(centre.x - (glowSize + expand) / 2, centre.y - (glowSize * 0.6f + expand) / 2 + 2,
                          glowSize + expand, glowSize * 0.6f + expand);
        }
    }

    // Note name with shadow
    Colour noteCol = getTuningColour(displayedCents);
    drawNoteGlyph(g, bounds, noteName, noteCol);
}

//==============================================================================
void TunerControl::drawNeedleMeter(Graphics& g, Rectangle<float> bounds)
{
    auto& colours = ColourScheme::getInstance().colours;
    auto& fonts = FontManager::getInstance();

    if (tunerProcessor == nullptr)
        return;

    float centreX = bounds.getCentreX();
    float bottomY = bounds.getBottom() + 10; // Reduced offset to raise meter
    float meterRadius = jmin(bounds.getWidth() * 0.38f, bounds.getHeight() * 0.95f);

    drawNeedleArcBackdrop(g, {centreX, bottomY}, meterRadius);

    // Tick marks with color zones
    for (int i = -5; i <= 5; ++i)
    {
        float tickAngle = degreesToRadians(-90.0f + (i * 10.0f));
        float innerR = meterRadius - 8;
        float outerR = meterRadius + 2;

        Colour tickCol;
        if (i == 0)
        {
            tickCol = colours["Text Colour"];
            innerR = meterRadius - 14;
        }
        else if (std::abs(i) <= 1)
            tickCol = colours["Success Colour"].withAlpha(0.8f);
        else if (std::abs(i) <= 2)
            tickCol = colours["Warning Colour"].withAlpha(0.7f);
        else
            tickCol = colours["Danger Colour"].withAlpha(0.6f);

        g.setColour(tickCol);
        float thickness = (i == 0) ? 3.0f : 2.0f;
        g.drawLine(centreX + std::cos(tickAngle) * innerR, bottomY + std::sin(tickAngle) * innerR,
                   centreX + std::cos(tickAngle) * outerR, bottomY + std::sin(tickAngle) * outerR, thickness);
    }

    // Cent labels
    g.setColour(colours["Text Colour"].withAlpha(0.5f));
    g.setFont(fonts.getMonoFont(9.0f));
    float lblRadius = meterRadius - 20;
    float leftAngle = degreesToRadians(-90.0f - 50.0f);
    float rightAngle = degreesToRadians(-90.0f + 50.0f);
    g.drawText("-50",
               Rectangle<float>(centreX + std::cos(leftAngle) * lblRadius - 12,
                                bottomY + std::sin(leftAngle) * lblRadius - 5, 24, 10),
               Justification::centred);
    g.drawText("+50",
               Rectangle<float>(centreX + std::cos(rightAngle) * lblRadius - 12,
                                bottomY + std::sin(rightAngle) * lblRadius - 5, 24, 10),
               Justification::centred);

    // Draw needle
    if (tunerProcessor->isPitchDetected())
    {
        float needleRad = degreesToRadians(-90.0f + needleAngle);
        float needleLen = meterRadius - 3;

        // Shadow
        g.setColour(colours["Window Background"].darker(0.35f).withAlpha(0.35f));
        Path shadowPath;
        shadowPath.startNewSubPath(centreX + 3, bottomY + 3);
        shadowPath.lineTo(centreX + std::cos(needleRad) * needleLen + 3, bottomY + std::sin(needleRad) * needleLen + 3);
        g.strokePath(shadowPath, PathStrokeType(4.0f, PathStrokeType::curved, PathStrokeType::rounded));

        // Needle
        Colour needleCol = getTuningColour(displayedCents);
        g.setColour(needleCol.darker(0.2f));
        Path needlePath;
        needlePath.startNewSubPath(centreX, bottomY);
        needlePath.lineTo(centreX + std::cos(needleRad) * needleLen, bottomY + std::sin(needleRad) * needleLen);
        g.strokePath(needlePath, PathStrokeType(3.5f, PathStrokeType::curved, PathStrokeType::rounded));

        // Highlight
        g.setColour(needleCol.brighter(0.2f));
        g.strokePath(needlePath, PathStrokeType(1.5f, PathStrokeType::curved, PathStrokeType::rounded));

        // Tip glow
        float tipX = centreX + std::cos(needleRad) * needleLen;
        float tipY = bottomY + std::sin(needleRad) * needleLen;
        g.setColour(needleCol.withAlpha(0.3f));
        g.fillEllipse(tipX - 6, tipY - 6, 12, 12);
        g.setColour(needleCol);
        g.fillEllipse(tipX - 4, tipY - 4, 8, 8);
    }

    // Pivot with gradient
    ColourGradient pivotGrad(colours["Plugin Border"].brighter(0.3f), centreX - 6, bottomY - 6,
                             colours["Plugin Border"].darker(0.2f), centreX + 6, bottomY + 6, true);
    g.setGradientFill(pivotGrad);
    g.fillEllipse(centreX - 10, bottomY - 10, 20, 20);
    g.setColour(colours["Text Colour"].withAlpha(0.2f));
    g.drawEllipse(centreX - 10, bottomY - 10, 20, 20, 1.0f);
}

//==============================================================================
void TunerControl::drawPitchDriftDisc(Graphics& g, Rectangle<float> bounds)
{
    auto& colours = ColourScheme::getInstance().colours;
    auto& fonts = FontManager::getInstance();

    if (tunerProcessor == nullptr)
        return;

    float centreX = bounds.getCentreX();
    float centreY = bounds.getCentreY();
    float radius = jmin(bounds.getWidth(), bounds.getHeight()) * 0.42f;

    // Outer rings
    g.setColour(colours["Plugin Border"].darker(0.2f));
    g.drawEllipse(centreX - radius - 5, centreY - radius - 5, (radius + 5) * 2, (radius + 5) * 2, 3.0f);
    g.setColour(colours["Plugin Border"]);
    g.drawEllipse(centreX - radius - 3, centreY - radius - 3, (radius + 3) * 2, (radius + 3) * 2, 1.5f);

    if (!tunerProcessor->isPitchDetected())
    {
        g.setColour(colours["Text Colour"].withAlpha(0.1f));
        g.fillEllipse(centreX - radius, centreY - radius, radius * 2, radius * 2);

        g.setColour(colours["Text Colour"].withAlpha(0.4f));
        g.setFont(fonts.getBodyFont());
        g.drawText("No Signal", bounds, Justification::centred);
        return;
    }

    // Drift bands
    float bandAngle = MathConstants<float>::twoPi / DRIFT_BANDS;
    Colour brightCol = getTuningColour(displayedCents);
    Colour darkCol = colours["Plugin Background"].darker(0.4f);

    for (int i = 0; i < DRIFT_BANDS; ++i)
    {
        float startAngle = (i * bandAngle) + driftRotation;

        Path brightSeg;
        brightSeg.addPieSegment(centreX - radius, centreY - radius, radius * 2, radius * 2, startAngle,
                                startAngle + (bandAngle * 0.5f), 0.25f);
        g.setColour(brightCol);
        g.fillPath(brightSeg);

        Path darkSeg;
        darkSeg.addPieSegment(centreX - radius, centreY - radius, radius * 2, radius * 2,
                              startAngle + (bandAngle * 0.5f), startAngle + bandAngle, 0.25f);
        g.setColour(darkCol);
        g.fillPath(darkSeg);
    }

    // Center hub
    ColourGradient hubGrad(colours["Plugin Border"].brighter(0.2f), centreX - 5, centreY - 5,
                           colours["Plugin Border"].darker(0.3f), centreX + 5, centreY + 5, true);
    g.setGradientFill(hubGrad);
    g.fillEllipse(centreX - 14, centreY - 14, 28, 28);
    g.setColour(colours["Text Colour"].withAlpha(0.15f));
    g.drawEllipse(centreX - 14, centreY - 14, 28, 28, 1.0f);

    // In-tune glow
    if (glowIntensity > 0.1f)
    {
        g.setColour(colours["Success Colour"].withAlpha(glowIntensity * 0.15f));
        g.fillEllipse(centreX - radius, centreY - radius, radius * 2, radius * 2);
    }

    // Draw arrow indicator instead of unicode
    Colour arrowCol = getTuningColour(displayedCents);
    g.setColour(arrowCol);

    if (std::abs(displayedCents) < 1.0f)
    {
        // In tune - draw filled circle
        g.fillEllipse(centreX - 5, centreY - 5, 10, 10);
    }
    else
    {
        // Draw arrow pointing up (sharp) or down (flat)
        Path arrow;
        if (displayedCents > 0)
        {
            // Sharp - arrow up
            arrow.addTriangle(centreX, centreY - 6, centreX - 5, centreY + 4, centreX + 5, centreY + 4);
        }
        else
        {
            // Flat - arrow down
            arrow.addTriangle(centreX, centreY + 6, centreX - 5, centreY - 4, centreX + 5, centreY - 4);
        }
        g.fillPath(arrow);
    }
}

//==============================================================================
void TunerControl::drawLedIndicators(Graphics& g, Rectangle<float> bounds)
{
    auto& colours = ColourScheme::getInstance().colours;

    if (tunerProcessor == nullptr)
        return;

    float totalWidth = bounds.getWidth() - 50;
    float ledWidth = totalWidth / NUM_LEDS;
    float ledHeight = bounds.getHeight() * 0.7f;
    float ledY = bounds.getCentreY() - ledHeight / 2;
    float startX = bounds.getX() + 25;

    for (int i = 0; i < NUM_LEDS; ++i)
    {
        float ledX = startX + (i * ledWidth);
        Rectangle<float> ledBounds(ledX + 1, ledY, ledWidth - 2, ledHeight);

        int centsValue = (i - 5) * 10;
        bool isCenter = (i == 5);
        bool isLit = false;

        if (tunerProcessor->isPitchDetected())
        {
            float cents = displayedCents;
            if (isCenter && std::abs(cents) < 5.0f)
                isLit = true;
            else if (!isCenter && std::abs(cents - centsValue) < 10.0f)
                isLit = true;
        }

        Colour baseColour;
        if (isCenter)
            baseColour = colours["Success Colour"];
        else if (std::abs(i - 5) == 1)
            baseColour = colours["Success Colour"].brighter(0.2f);
        else if (std::abs(i - 5) == 2)
            baseColour = colours["Warning Colour"];
        else
            baseColour = colours["Danger Colour"];

        if (isLit)
        {
            g.setColour(baseColour.withAlpha(0.35f));
            g.fillRoundedRectangle(ledBounds.expanded(2), 3.0f);

            ColourGradient ledGrad(baseColour.brighter(0.3f), ledBounds.getX(), ledBounds.getY(),
                                   baseColour.darker(0.2f), ledBounds.getX(), ledBounds.getBottom(), false);
            g.setGradientFill(ledGrad);
            g.fillRoundedRectangle(ledBounds, 2.0f);
        }
        else
        {
            g.setColour(baseColour.withAlpha(0.1f));
            g.fillRoundedRectangle(ledBounds, 2.0f);
        }
    }

    // Draw flat symbol on left side
    float symbolY = bounds.getCentreY();
    drawFlatSymbol(g, bounds.getX() + 12, symbolY, 12.0f, colours["Text Colour"].withAlpha(0.6f));

    // Draw sharp symbol on right side
    drawSharpSymbol(g, bounds.getRight() - 12, symbolY, 12.0f, colours["Text Colour"].withAlpha(0.6f));
}

//==============================================================================
void TunerControl::drawFrequencyDisplay(Graphics& g, Rectangle<float> bounds)
{
    auto& colours = ColourScheme::getInstance().colours;
    auto& fonts = FontManager::getInstance();

    if (tunerProcessor == nullptr)
        return;

    String displayText;
    Colour textCol = colours["Text Colour"];

    if (tunerProcessor->isPitchDetected())
    {
        float freq = tunerProcessor->getDetectedFrequency();
        float cents = displayedCents;
        String centsStr = (cents >= 0 ? "+" : "") + String(static_cast<int>(cents));
        displayText = String(freq, 1) + " Hz  " + centsStr + " cents";
        textCol = textCol.withAlpha(0.85f);
    }
    else
    {
        displayText = "Waiting for signal...";
        textCol = textCol.withAlpha(0.4f);
    }

    g.setColour(textCol);
    g.setFont(fonts.getMonoFont(11.0f));
    g.drawText(displayText, bounds, Justification::centred);
}

//==============================================================================
void TunerControl::resized()
{
    auto& colours = ColourScheme::getInstance().colours;

    auto bounds = getLocalBounds().reduced(8, 6);
    bounds.removeFromTop(43);
    auto modeArea = bounds.removeFromTop(29);
    auto bypassArea = modeArea.removeFromRight(82).reduced(2, 2);
    modeArea.removeFromRight(4);
    const int thirdWidth = modeArea.getWidth() / 3;
    needleModeButton->setBounds(modeArea.removeFromLeft(thirdWidth).reduced(2, 2));
    driftModeButton->setBounds(modeArea.removeFromLeft(thirdWidth).reduced(2, 2));
    sixStringModeButton->setBounds(modeArea.reduced(2, 2));
    bypassButton->setBounds(bypassArea);

    for (auto* button : {needleModeButton.get(), driftModeButton.get(), sixStringModeButton.get()})
    {
        const auto plateBase = colours["Field Background"].interpolatedWith(colours["Plugin Background"], 0.45f)
                                   .darker(0.12f);
        button->setColour(TextButton::buttonColourId, Colours::transparentBlack);
        button->setColour(TextButton::buttonOnColourId, Colours::transparentBlack);
        button->setColour(TextButton::textColourOffId, plateBase.contrasting(0.88f).withAlpha(0.62f));
        button->setColour(TextButton::textColourOnId, colours["Tuner Active Colour"].brighter(0.18f));
    }
    const auto bypassBase = colours["Field Background"].interpolatedWith(colours["Plugin Border"], 0.08f);
    bypassButton->setColour(TextButton::buttonColourId, Colours::transparentBlack);
    bypassButton->setColour(TextButton::buttonOnColourId, Colours::transparentBlack);
    bypassButton->setColour(TextButton::textColourOffId, bypassBase.contrasting(0.88f).withAlpha(0.70f));
    bypassButton->setColour(TextButton::textColourOnId, colours["Danger Colour"].brighter(0.22f));

    updateModeButtons();
}

void TunerControl::updateModeButtons()
{
    if (needleModeButton != nullptr)
        needleModeButton->setToggleState(currentMode == TunerMode::Needle, dontSendNotification);
    if (driftModeButton != nullptr)
        driftModeButton->setToggleState(currentMode == TunerMode::PitchDrift, dontSendNotification);
    if (sixStringModeButton != nullptr)
        sixStringModeButton->setToggleState(currentMode == TunerMode::SixString, dontSendNotification);
}

//==============================================================================
String TunerControl::getNoteName(int midiNote) const
{
    if (midiNote < 0 || midiNote > 127)
        return "---";

    static const char* noteNames[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
    int octave = (midiNote / 12) - 1;
    int noteIndex = midiNote % 12;

    return String(noteNames[noteIndex]) + String(octave);
}

//==============================================================================
Colour TunerControl::getTuningColour(float cents) const
{
    auto& colours = ColourScheme::getInstance().colours;
    float absCents = std::abs(cents);

    if (absCents < 2.0f)
        return colours["Tuner Active Colour"].brighter(0.18f);
    else if (absCents < 8.0f)
        return colours["Success Colour"].interpolatedWith(colours["Tuner Active Colour"], 0.45f).brighter(0.10f);
    else if (absCents < 18.0f)
        return colours["Warning Colour"].brighter(0.12f);
    else if (absCents < 32.0f)
        return colours["Warning Colour"].interpolatedWith(colours["Danger Colour"], 0.32f).brighter(0.08f);
    else
        return colours["Danger Colour"].brighter(0.10f);
}

//==============================================================================
void TunerControl::drawFlatSymbol(Graphics& g, float x, float y, float size, Colour colour) const
{
    // Flat symbol (♭): vertical line with curved bottom loop
    g.setColour(colour);

    float scale = size / 16.0f; // Normalized to 16px base

    // Vertical stem
    Path stem;
    stem.startNewSubPath(x, y - size * 0.6f);
    stem.lineTo(x, y + size * 0.4f);
    g.strokePath(stem, PathStrokeType(scale * 2.0f, PathStrokeType::curved, PathStrokeType::rounded));

    // Curved loop (the belly of the flat)
    Path loop;
    loop.startNewSubPath(x, y);
    loop.cubicTo(x + size * 0.45f, y - size * 0.1f,  // control point 1
                 x + size * 0.45f, y + size * 0.35f, // control point 2
                 x, y + size * 0.4f);                // end point
    g.strokePath(loop, PathStrokeType(scale * 2.0f, PathStrokeType::curved, PathStrokeType::rounded));
}

//==============================================================================
void TunerControl::drawSharpSymbol(Graphics& g, float x, float y, float size, Colour colour) const
{
    // Sharp symbol (♯): two vertical lines crossed by two horizontal lines
    g.setColour(colour);

    float scale = size / 16.0f;
    float lineThickness = scale * 2.0f;

    // Vertical lines (slightly tilted for musical authenticity)
    float vOffset = size * 0.25f;
    float vHeight = size * 0.65f;

    Path verticals;
    // Left vertical
    verticals.startNewSubPath(x - size * 0.15f, y - vHeight + vOffset * 0.3f);
    verticals.lineTo(x - size * 0.15f, y + vHeight + vOffset * 0.3f);
    // Right vertical
    verticals.startNewSubPath(x + size * 0.15f, y - vHeight - vOffset * 0.3f);
    verticals.lineTo(x + size * 0.15f, y + vHeight - vOffset * 0.3f);
    g.strokePath(verticals, PathStrokeType(lineThickness, PathStrokeType::curved, PathStrokeType::rounded));

    // Horizontal lines (tilted slightly for natural appearance)
    float hWidth = size * 0.4f;
    float tilt = size * 0.08f;

    Path horizontals;
    // Top horizontal
    horizontals.startNewSubPath(x - hWidth, y - size * 0.2f + tilt);
    horizontals.lineTo(x + hWidth, y - size * 0.2f - tilt);
    // Bottom horizontal
    horizontals.startNewSubPath(x - hWidth, y + size * 0.2f + tilt);
    horizontals.lineTo(x + hWidth, y + size * 0.2f - tilt);
    g.strokePath(horizontals, PathStrokeType(lineThickness * 1.3f, PathStrokeType::curved, PathStrokeType::rounded));
}
