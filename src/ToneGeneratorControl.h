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

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ToneGeneratorControl)
};
