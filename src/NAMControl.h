/*
  ==============================================================================

    NAMControl.h
    UI control for the NAM (Neural Amp Modeler) processor
    Professional amp-style interface

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

class NAMProcessor;

//==============================================================================
/**
    Custom LookAndFeel for amp-style controls.
*/
class NAMLookAndFeel : public LookAndFeel_V4
{
  public:
    NAMLookAndFeel();

    void drawRotarySlider(Graphics& g, int x, int y, int width, int height,
                          float sliderPos, float rotaryStartAngle,
                          float rotaryEndAngle, Slider& slider) override;

    void drawLinearSlider(Graphics& g, int x, int y, int width, int height,
                          float sliderPos, float minSliderPos, float maxSliderPos,
                          const Slider::SliderStyle style, Slider& slider) override;

    void drawToggleButton(Graphics& g, ToggleButton& button,
                          bool shouldDrawButtonAsHighlighted,
                          bool shouldDrawButtonAsDown) override;

    void drawButtonBackground(Graphics& g, Button& button,
                              const Colour& backgroundColour,
                              bool shouldDrawButtonAsHighlighted,
                              bool shouldDrawButtonAsDown) override;
};

//==============================================================================
/**
    Control component for NAMProcessor.
    Professional amp-style interface with model/IR loading,
    gain controls, noise gate, and tone stack.
*/
class NAMControl : public Component, public Button::Listener, public Slider::Listener
{
  public:
    NAMControl(NAMProcessor* processor);
    ~NAMControl() override;

    void paint(Graphics& g) override;
    void resized() override;
    void buttonClicked(Button* button) override;
    void sliderValueChanged(Slider* slider) override;

  private:
    void updateModelDisplay();
    void updateIRDisplay();
    void drawSectionPanel(Graphics& g, const Rectangle<int>& bounds,
                          const String& title, Colour headerColour);

    NAMProcessor* namProcessor;
    NAMLookAndFeel namLookAndFeel;

    // Model loading
    std::unique_ptr<TextButton> loadModelButton;
    std::unique_ptr<TextButton> browseModelsButton;
    std::unique_ptr<TextButton> clearModelButton;
    std::unique_ptr<Label> modelNameLabel;

    // IR loading
    std::unique_ptr<TextButton> loadIRButton;
    std::unique_ptr<TextButton> clearIRButton;
    std::unique_ptr<Label> irNameLabel;
    std::unique_ptr<ToggleButton> irEnabledButton;

    // Effects loop
    std::unique_ptr<ToggleButton> fxLoopEnabledButton;
    std::unique_ptr<TextButton> editFxLoopButton;

    // Input/Output gain
    std::unique_ptr<Slider> inputGainSlider;
    std::unique_ptr<Label> inputGainLabel;
    std::unique_ptr<Slider> outputGainSlider;
    std::unique_ptr<Label> outputGainLabel;

    // Noise gate
    std::unique_ptr<Slider> noiseGateSlider;
    std::unique_ptr<Label> noiseGateLabel;

    // Tone stack
    std::unique_ptr<ToggleButton> toneStackEnabledButton;
    std::unique_ptr<Slider> bassSlider;
    std::unique_ptr<Label> bassLabel;
    std::unique_ptr<Slider> midSlider;
    std::unique_ptr<Label> midLabel;
    std::unique_ptr<Slider> trebleSlider;
    std::unique_ptr<Label> trebleLabel;

    // Normalize
    std::unique_ptr<ToggleButton> normalizeButton;

    // File choosers (kept alive for async operation)
    std::unique_ptr<FileChooser> modelFileChooser;
    std::unique_ptr<FileChooser> irFileChooser;

    // Color scheme
    static constexpr uint32 kBackgroundDark = 0xff1a1a1a;
    static constexpr uint32 kBackgroundMid = 0xff252525;
    static constexpr uint32 kPanelBackground = 0xff2d2d2d;
    static constexpr uint32 kHeaderAccent = 0xff3a3a3a;
    static constexpr uint32 kAccentOrange = 0xffff8c00;
    static constexpr uint32 kAccentBlue = 0xff4a90d9;
    static constexpr uint32 kTextBright = 0xffe0e0e0;
    static constexpr uint32 kTextDim = 0xff909090;
    static constexpr uint32 kLedOn = 0xff00ff66;
    static constexpr uint32 kLedOff = 0xff404040;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NAMControl)
};
