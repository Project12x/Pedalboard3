/*
  ==============================================================================

    OscilloscopeControl.cpp
    Real-time waveform display control for OscilloscopeProcessor

  ==============================================================================
*/

#include "OscilloscopeControl.h"
#include "OscilloscopeProcessor.h"
#include "ColourScheme.h"

//==============================================================================
OscilloscopeControl::OscilloscopeControl(OscilloscopeProcessor* processor)
    : oscilloscopeProcessor(processor)
{
    displayBuffer.fill(0.0f);
    startTimer(16); // ~60 FPS
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
    auto bounds = getLocalBounds().toFloat().reduced(4);

    // Background
    g.setColour(colours["Window Background"].darker(0.2f));
    g.fillRoundedRectangle(bounds, 6.0f);

    // Grid lines
    g.setColour(colours["Text Colour"].withAlpha(0.1f));
    float centerY = bounds.getCentreY();
    g.drawHorizontalLine((int)centerY, bounds.getX(), bounds.getRight());

    // Vertical divisions
    for (int i = 1; i < 4; ++i)
    {
        float x = bounds.getX() + (bounds.getWidth() * i / 4.0f);
        g.drawVerticalLine((int)x, bounds.getY(), bounds.getBottom());
    }

    // Waveform path
    Path waveform;
    float xScale = bounds.getWidth() / (float)DISPLAY_SAMPLES;
    float yScale = bounds.getHeight() * 0.45f; // Leave margin

    bool started = false;
    for (int i = 0; i < DISPLAY_SAMPLES; ++i)
    {
        float x = bounds.getX() + i * xScale;
        float y = centerY - displayBuffer[i] * yScale;

        // Clamp to bounds
        y = juce::jlimit(bounds.getY(), bounds.getBottom(), y);

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

    // Border
    g.setColour(colours["Text Colour"].withAlpha(0.3f));
    g.drawRoundedRectangle(bounds, 6.0f, 1.0f);
}

void OscilloscopeControl::resized()
{
    // No child components
}
