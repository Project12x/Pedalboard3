/*
  ==============================================================================

    TunerControl.h
    Professional chromatic tuner with analog needle and strobe display

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

class TunerProcessor;

//==============================================================================
/**
    Professional tuner display with two modes:
    - NEEDLE: Large analog-style needle meter
    - STROBE: "Turbo Tuner" style strobe disc for ±0.1 cent accuracy
*/
class TunerControl : public Component, private Timer, public Button::Listener
{
  public:
    /// Tuner display modes
    enum class TunerMode
    {
        Needle,
        Strobe
    };

    TunerControl(TunerProcessor* processor);
    ~TunerControl() override;

    void paint(Graphics& g) override;
    void resized() override;
    void buttonClicked(Button* button) override;

  private:
    void timerCallback() override;

    // Drawing methods - Needle mode
    void drawNeedleMeter(Graphics& g, Rectangle<float> bounds);
    void drawLedIndicators(Graphics& g, Rectangle<float> bounds);

    // Drawing methods - Strobe mode
    void drawStrobeDisc(Graphics& g, Rectangle<float> bounds);

    // Common drawing methods
    void drawNoteDisplay(Graphics& g, Rectangle<float> bounds);
    void drawFrequencyDisplay(Graphics& g, Rectangle<float> bounds);
    void drawModeToggle(Graphics& g, Rectangle<float> bounds);

    // Get note name with sharp/flat
    String getNoteName(int midiNote) const;

    // Get color based on tuning accuracy
    Colour getTuningColour(float cents) const;

    // Draw musical notation symbols - professional look
    void drawFlatSymbol(Graphics& g, float x, float y, float size, Colour colour) const;
    void drawSharpSymbol(Graphics& g, float x, float y, float size, Colour colour) const;

    TunerProcessor* tunerProcessor;

    // Current mode
    TunerMode currentMode = TunerMode::Needle;
    std::unique_ptr<TextButton> modeButton;

    // Display values with smoothing
    float displayedCents = 0.0f;
    float needleAngle = 0.0f;    // Smoothed angle for needle
    float strobeRotation = 0.0f; // For strobe animation

    // Animation state
    float glowIntensity = 0.0f; // For in-tune glow effect

    // Visual constants
    static constexpr float NEEDLE_SMOOTHING = 0.15f;
    static constexpr float GLOW_SMOOTHING = 0.1f;
    static constexpr int NUM_LEDS = 11;    // -50 to +50 cents
    static constexpr int STROBE_BANDS = 8; // Number of strobe bands

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TunerControl)
};
