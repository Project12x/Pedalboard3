/*
  ==============================================================================

    TunerControl.cpp
    Professional chromatic tuner with analog needle and strobe display
    Uses project fonts (Space Grotesk + JetBrains Mono) and drawn graphics

  ==============================================================================
*/

#include "TunerControl.h"

#include "ColourScheme.h"
#include "FontManager.h"
#include "TunerProcessor.h"

//==============================================================================
TunerControl::TunerControl(TunerProcessor* processor) : tunerProcessor(processor)
{
    // Mode toggle button
    modeButton = std::make_unique<TextButton>("NEEDLE");
    modeButton->setTooltip("Toggle between Needle and Strobe tuner modes");
    modeButton->addListener(this);
    addAndMakeVisible(modeButton.get());

    // 60 fps for smooth animation
    startTimerHz(60);

    setSize(300, 200);
}

TunerControl::~TunerControl()
{
    stopTimer();
}

//==============================================================================
void TunerControl::buttonClicked(Button* button)
{
    if (button == modeButton.get())
    {
        if (currentMode == TunerMode::Needle)
        {
            currentMode = TunerMode::Strobe;
            modeButton->setButtonText("STROBE");
        }
        else
        {
            currentMode = TunerMode::Needle;
            modeButton->setButtonText("NEEDLE");
        }
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

    if (currentMode == TunerMode::Strobe)
    {
        strobeRotation = tunerProcessor->getStrobePhase() * MathConstants<float>::twoPi * STROBE_BANDS;
    }

    repaint();
}

//==============================================================================
void TunerControl::paint(Graphics& g)
{
    auto& colours = ColourScheme::getInstance().colours;
    auto bounds = getLocalBounds().toFloat();

    // Gradient background
    Colour bgTop = colours["Plugin Background"].brighter(0.05f);
    Colour bgBot = colours["Plugin Background"].darker(0.08f);
    g.setGradientFill(ColourGradient(bgTop, 0, 0, bgBot, 0, bounds.getHeight(), false));
    g.fillAll();

    // Subtle frame
    g.setColour(colours["Plugin Border"].withAlpha(0.3f));
    g.drawRect(bounds.reduced(1), 1.0f);

    auto area = bounds.reduced(6);

    auto noteArea = area.removeFromTop(52);
    drawNoteDisplay(g, noteArea);

    auto meterArea = area.removeFromTop(90);
    if (currentMode == TunerMode::Needle)
        drawNeedleMeter(g, meterArea);
    else
        drawStrobeDisc(g, meterArea);

    auto ledArea = area.removeFromTop(16);
    if (currentMode == TunerMode::Needle)
        drawLedIndicators(g, ledArea);

    drawFrequencyDisplay(g, area);
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
    g.setFont(fonts.getDisplayFont(50.0f));

    // Shadow
    g.setColour(Colours::black.withAlpha(0.4f));
    g.drawText(noteName, bounds.translated(1.5f, 1.5f), Justification::centred);

    // Main text
    g.setColour(noteCol);
    g.drawText(noteName, bounds, Justification::centred);
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

    // Tick marks with color zones
    for (int i = -5; i <= 5; ++i)
    {
        float tickAngle = degreesToRadians(-90.0f + (i * 10.0f));
        float innerR = meterRadius - 8;
        float outerR = meterRadius + 2;

        Colour tickCol;
        if (i == 0)
        {
            tickCol = Colours::white;
            innerR = meterRadius - 14;
        }
        else if (std::abs(i) <= 1)
            tickCol = colours["Success Colour"].withAlpha(0.8f);
        else if (std::abs(i) <= 2)
            tickCol = Colours::yellow.withAlpha(0.7f);
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
        g.setColour(Colours::black.withAlpha(0.35f));
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
void TunerControl::drawStrobeDisc(Graphics& g, Rectangle<float> bounds)
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

    // Strobe bands
    float bandAngle = MathConstants<float>::twoPi / STROBE_BANDS;
    Colour brightCol = getTuningColour(displayedCents);
    Colour darkCol = colours["Plugin Background"].darker(0.4f);

    for (int i = 0; i < STROBE_BANDS; ++i)
    {
        float startAngle = (i * bandAngle) + strobeRotation;

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
    auto& fonts = FontManager::getInstance();

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

    auto bounds = getLocalBounds().reduced(6);
    modeButton->setBounds(bounds.getRight() - 55, bounds.getBottom() - 14, 50, 13);
    modeButton->setColour(TextButton::buttonColourId, colours["Plugin Border"].darker(0.1f));
    modeButton->setColour(TextButton::textColourOffId, colours["Text Colour"].withAlpha(0.8f));
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
    float absCents = std::abs(cents);

    if (absCents < 2.0f)
        return Colour(0xFF00E676); // Bright green
    else if (absCents < 8.0f)
        return Colour(0xFF76FF03); // Lime
    else if (absCents < 18.0f)
        return Colour(0xFFFFEB3B); // Yellow
    else if (absCents < 32.0f)
        return Colour(0xFFFF9800); // Orange
    else
        return Colour(0xFFFF5252); // Red
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
