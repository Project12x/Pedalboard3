/*
  ==============================================================================

    OscilloscopeControl.cpp
    Real-time waveform display control for OscilloscopeProcessor

  ==============================================================================
*/

#include "OscilloscopeControl.h"
#include "OscilloscopeProcessor.h"
#include "ColourScheme.h"
#include "FontManager.h"

#include <cmath>

//==============================================================================
OscilloscopeControl::OscilloscopeControl(OscilloscopeProcessor* processor)
    : oscilloscopeProcessor(processor)
{
    displayBuffer.fill(0.0f);
    startTimer(16); // ~60 FPS
    setSize(280, 154);
}

OscilloscopeControl::~OscilloscopeControl()
{
    stopTimer();
}

//==============================================================================
void OscilloscopeControl::timerCallback()
{
    std::array<float, OscilloscopeProcessor::DISPLAY_SAMPLES> buffer;
    oscilloscopeProcessor->getDisplayBuffer(buffer);
    displayBuffer = buffer;
    repaint();
}

void OscilloscopeControl::paint(Graphics& g)
{
    auto& colours = ColourScheme::getInstance().colours;
    auto& fonts = FontManager::getInstance();
    auto bounds = getLocalBounds().toFloat().reduced(1.5f);
    const auto accent = colours["Audio Connection"];

    ColourGradient shell(colours["Plugin Background"].interpolatedWith(accent, 0.12f).brighter(0.08f), bounds.getX(),
                         bounds.getY(), colours["Window Background"].darker(0.14f), bounds.getX(),
                         bounds.getBottom(), false);
    shell.addColour(0.46, colours["Plugin Background"].interpolatedWith(accent, 0.08f));
    g.setGradientFill(shell);
    g.fillRoundedRectangle(bounds, 8.0f);

    g.setColour(colours["Plugin Border"].interpolatedWith(accent, 0.28f).withAlpha(0.70f));
    g.drawRoundedRectangle(bounds.reduced(0.5f), 8.0f, 1.0f);
    g.setColour(colours["Text Colour"].withAlpha(0.055f));
    g.drawRoundedRectangle(bounds.reduced(2.0f), 6.5f, 0.7f);

    auto header = bounds.reduced(8.0f, 7.0f).removeFromTop(28.0f);
    ColourGradient headerFill(colours["Plugin Background"].interpolatedWith(accent, 0.28f).brighter(0.10f),
                              header.getX(), header.getY(),
                              colours["Plugin Background"].interpolatedWith(accent, 0.14f).darker(0.10f),
                              header.getX(), header.getBottom(), false);
    g.setGradientFill(headerFill);
    g.fillRoundedRectangle(header, 6.0f);
    g.setColour(accent.withAlpha(0.52f));
    g.drawRoundedRectangle(header.reduced(0.5f), 6.0f, 1.0f);

    auto led = Rectangle<float>(8.0f, 16.0f).withCentre({header.getX() + 13.0f, header.getCentreY()});
    g.setColour(accent.withAlpha(0.18f));
    g.fillRoundedRectangle(led.expanded(3.0f), 4.0f);
    g.setColour(accent.withAlpha(0.92f));
    g.fillRoundedRectangle(led, 4.0f);

    auto titleArea = header.withTrimmedLeft(31.0f).withTrimmedRight(78.0f);
    g.setFont(fonts.getBadgeFont().withHeight(10.5f));
    g.setColour(accent.brighter(0.12f).withAlpha(0.92f));
    g.drawText("OSCILLOSCOPE", titleArea.removeFromTop(12.0f), Justification::centredLeft, true);
    g.setFont(fonts.getCaptionFont().withHeight(10.0f));
    g.setColour(colours["Text Colour"].withAlpha(0.58f));
    g.drawText("stereo pass-through monitor", titleArea, Justification::centredLeft, true);

    auto status = header.removeFromRight(58.0f).reduced(2.0f, 5.0f);
    g.setColour(colours["Field Background"].withAlpha(0.78f));
    g.fillRoundedRectangle(status, 7.0f);
    g.setColour(accent.withAlpha(0.34f));
    g.drawRoundedRectangle(status.reduced(0.5f), 7.0f, 0.8f);
    g.setFont(fonts.getBadgeFont().withHeight(9.0f));
    g.setColour(accent.withAlpha(0.86f));
    g.drawText("LIVE", status, Justification::centred, true);

    auto display = bounds.reduced(10.0f, 9.0f).withTrimmedTop(35.0f).withTrimmedBottom(8.0f);
    ColourGradient displayFill(colours["Field Background"].darker(0.12f), display.getX(), display.getY(),
                               colours["Window Background"].darker(0.28f), display.getX(), display.getBottom(),
                               false);
    g.setGradientFill(displayFill);
    g.fillRoundedRectangle(display, 6.0f);
    g.setColour(accent.withAlpha(0.34f));
    g.drawRoundedRectangle(display.reduced(0.5f), 6.0f, 0.9f);

    // Grid lines
    const auto gridArea = display.reduced(8.0f, 7.0f);
    g.setColour(colours["Text Colour"].withAlpha(0.085f));
    float centerY = gridArea.getCentreY();
    g.drawHorizontalLine((int)centerY, gridArea.getX(), gridArea.getRight());

    // Vertical divisions
    for (int i = 1; i < 4; ++i)
    {
        float x = gridArea.getX() + (gridArea.getWidth() * i / 4.0f);
        g.drawVerticalLine((int)x, gridArea.getY(), gridArea.getBottom());
    }

    g.setColour(accent.withAlpha(0.18f));
    g.drawHorizontalLine((int)(gridArea.getY() + gridArea.getHeight() * 0.28f), gridArea.getX(), gridArea.getRight());
    g.drawHorizontalLine((int)(gridArea.getY() + gridArea.getHeight() * 0.72f), gridArea.getX(), gridArea.getRight());

    // Waveform path
    Path waveform;
    float xScale = gridArea.getWidth() / (float)DISPLAY_SAMPLES;
    float yScale = gridArea.getHeight() * 0.44f; // Leave margin

    bool started = false;
    bool hasSignal = false;
    for (int i = 0; i < DISPLAY_SAMPLES; ++i)
    {
        hasSignal = hasSignal || std::abs(displayBuffer[i]) > 0.0005f;
        float x = gridArea.getX() + i * xScale;
        float y = centerY - displayBuffer[i] * yScale;

        // Clamp to bounds
        y = juce::jlimit(gridArea.getY(), gridArea.getBottom(), y);

        if (!started)
        {
            waveform.startNewSubPath(x, y);
            started = true;
        }
        else
        {
            waveform.lineTo(x, y);
        }
    }

    // Draw waveform with glow effect
    g.setColour(colours["Audio Connection"].withAlpha(0.3f));
    g.strokePath(waveform, PathStrokeType(3.0f));

    g.setColour(colours["Audio Connection"]);
    g.strokePath(waveform, PathStrokeType(1.5f));

    if (!hasSignal)
    {
        g.setFont(fonts.getMonoDisplayFont(9.0f));
        g.setColour(colours["Text Colour"].withAlpha(0.33f));
        g.drawText("NO SIGNAL", gridArea.reduced(0.0f, 7.0f), Justification::centredBottom, true);
    }
}

void OscilloscopeControl::resized()
{
    // No child components
}
