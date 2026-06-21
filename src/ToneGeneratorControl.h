/*
  ==============================================================================

    ToneGeneratorControl.h
    UI for the tone generator test tool

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

class ToneGeneratorProcessor;

//==============================================================================
/**
    UI for controlling the tone generator:
    - Waveform selection
    - Frequency / MIDI note
    - Detune (cents)
    - Test mode selection
    - Play/Stop
*/
class ToneGeneratorControl : public Component, private Timer, public Button::Listener, public Slider::Listener
{
  public:
    ToneGeneratorControl(ToneGeneratorProcessor* processor);
    ~ToneGeneratorControl() override;

    void paint(Graphics& g) override;
    void resized() override;
    void buttonClicked(Button* button) override;
    void sliderValueChanged(Slider* slider) override;

  private:
    void timerCallback() override;

    // UI helpers
    void updateDisplay();
    void syncButtonStates();
    void styleButtonChrome(TextButton& button, Colour accent, bool active);
    void styleEditableSlider(Slider& slider, Colour accent, const String& suffix, int textBoxWidth);
    void drawChromeShell(Graphics& g, Rectangle<float> bounds);
    void drawDisplayPanel(Graphics& g, Rectangle<float> bounds);
    void drawWaveformGlyph(Graphics& g, Rectangle<float> bounds);
    void drawSectionLabel(Graphics& g, Rectangle<float> bounds, const String& text);
    void drawOutputKnob(Graphics& g, Rectangle<float> bounds, float normalisedValue, Colour accent);
    void drawValueChip(Graphics& g, Rectangle<float> bounds, const String& text, Colour accent);
    void drawSliderLane(Graphics& g, Rectangle<float> bounds, const String& label, float normalisedValue, Colour accent);
    Colour getAccentColour() const;
    String getNoteName(int midiNote) const;

    ToneGeneratorProcessor* toneProcessor;

    // Waveform buttons
    std::unique_ptr<TextButton> sineBtn;
    std::unique_ptr<TextButton> sawBtn;
    std::unique_ptr<TextButton> squareBtn;
    std::unique_ptr<TextButton> noiseBtn;

    // Frequency controls
    std::unique_ptr<Slider> frequencySlider;
    std::unique_ptr<Slider> detuneSlider;

    // Detune preset buttons for boundary testing
    std::unique_ptr<TextButton> detune1Btn;  // ±1 cent
    std::unique_ptr<TextButton> detune5Btn;  // ±5 cents
    std::unique_ptr<TextButton> detune50Btn; // ±50 cents
    std::unique_ptr<TextButton> detune99Btn; // ±99 cents (boundary)

    // Test mode buttons
    std::unique_ptr<TextButton> staticBtn;
    std::unique_ptr<TextButton> sweepBtn;
    std::unique_ptr<TextButton> driftBtn;

    // Play/Stop
    std::unique_ptr<TextButton> playButton;

    // Amplitude
    std::unique_ptr<Slider> amplitudeSlider;

    // Display state
    float displayedFrequency = 440.0f;
    String displayedNote = "A4";

    Rectangle<int> headerArea;
    Rectangle<int> displayPanel;
    Rectangle<int> waveformGlyphArea;
    Rectangle<int> frequencyRail;
    Rectangle<int> frequencyChipArea;
    Rectangle<int> noteChipArea;
    Rectangle<int> pitchPanel;
    Rectangle<int> detuneRail;
    Rectangle<int> detuneChipArea;
    Rectangle<int> waveformPanel;
    Rectangle<int> bottomPanel;
    Rectangle<int> outputPanel;
    Rectangle<int> outputKnobArea;
    Rectangle<int> amplitudeRail;
    Rectangle<int> amplitudeChipArea;
    Rectangle<int> modePanel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ToneGeneratorControl)
};
