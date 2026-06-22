/*
  ==============================================================================

    TunerControl.h
    Chromatic tuner with needle, pitch-drift, and string-reference displays

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

#include <array>

class TunerProcessor;

//==============================================================================
/**
    Tuner display with three views:
    - NEEDLE: Large analog-style needle meter
    - DRIFT: animated pitch-drift view driven by detected pitch error
    - SIX STRING: guitar-string reference view driven by the same monophonic detector
*/
class TunerControl : public Component, private Timer, public Button::Listener
{
  public:
    /// Tuner display modes
    enum class TunerMode
    {
        Needle,
        PitchDrift,
        SixString
    };

    TunerControl(TunerProcessor* processor);
    ~TunerControl() override;

    void setBypassController(std::function<bool()> stateGetter, std::function<void(bool)> stateSetter);
    void paint(Graphics& g) override;
    void resized() override;
    void buttonClicked(Button* button) override;

  private:
    void timerCallback() override;

    void drawTunerGlassPanel(Graphics& g, Rectangle<float> bounds);
    void drawTunerHeader(Graphics& g, Rectangle<float> bounds);
    void drawNoteGlyph(Graphics& g, Rectangle<float> bounds, const String& noteName, Colour noteColour);
    void drawNeedleArcBackdrop(Graphics& g, Point<float> centre, float radius);
    void drawModeSegmentedControl(Graphics& g, Rectangle<float> bounds);
    void drawBypassPill(Graphics& g, Rectangle<float> bounds);
    void drawCoarseDeviationStrip(Graphics& g, Rectangle<float> bounds);
    void drawStatusBadge(Graphics& g, Rectangle<float> bounds);
    void drawSignalConfidenceStrip(Graphics& g, Rectangle<float> bounds);
    void drawPitchTrace(Graphics& g, Rectangle<float> bounds);
    void drawReferenceResponseRail(Graphics& g, Rectangle<float> bounds);
    void pushPitchTraceSample();

    // Drawing methods - Needle mode
    void drawNeedleMeter(Graphics& g, Rectangle<float> bounds);
    void drawLedIndicators(Graphics& g, Rectangle<float> bounds);

    // Drawing methods - pitch-drift mode
    void drawPitchDriftDisc(Graphics& g, Rectangle<float> bounds);
    void drawSixStringDisplay(Graphics& g, Rectangle<float> bounds);

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
    void updateModeButtons();
    bool isPitchDriftMode() const { return currentMode == TunerMode::PitchDrift; }

    TunerProcessor* tunerProcessor;

    // Current mode
    TunerMode currentMode = TunerMode::Needle;
    std::unique_ptr<TextButton> needleModeButton;
    std::unique_ptr<TextButton> driftModeButton;
    std::unique_ptr<TextButton> sixStringModeButton;
    std::unique_ptr<TextButton> bypassButton;
    std::function<bool()> getBypassState;
    std::function<void(bool)> setBypassState;

    // Display values with smoothing
    float displayedCents = 0.0f;
    float needleAngle = 0.0f;    // Smoothed angle for needle
    float driftRotation = 0.0f; // For pitch-drift animation
    float displayedConfidence = 0.0f;

    // Animation state
    float glowIntensity = 0.0f; // For in-tune glow effect

    static constexpr int kPitchTraceSize = 80;
    std::array<float, kPitchTraceSize> pitchTraceCents{};
    std::array<float, kPitchTraceSize> pitchTraceConfidence{};
    std::array<int, kPitchTraceSize> pitchTraceNote{};
    int pitchTraceWriteIndex = 0;
    int pitchTraceFrameCounter = 0;

    // Visual constants
    static constexpr float NEEDLE_SMOOTHING = 0.15f;
    static constexpr float GLOW_SMOOTHING = 0.1f;
    static constexpr int NUM_LEDS = 11;    // -50 to +50 cents
    static constexpr int DRIFT_BANDS = 8; // Number of drift bands

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TunerControl)
};
