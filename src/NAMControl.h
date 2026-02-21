/*
  ==============================================================================

    NAMControl.h
    UI control for the NAM (Neural Amp Modeler) processor
    Professional amp-style interface with theme-complementary colours

  ==============================================================================
*/

#pragma once

#include "ColourScheme.h"
#include "FontManager.h"

#include <JuceHeader.h>

class NAMProcessor;

//==============================================================================
/**
    Custom LookAndFeel for amp-style controls.
    Derives its palette from the active ColourScheme for theme consistency.
*/
class NAMLookAndFeel : public LookAndFeel_V4
{
  public:
    NAMLookAndFeel();

    void drawRotarySlider(Graphics& g, int x, int y, int width, int height, float sliderPos, float rotaryStartAngle,
                          float rotaryEndAngle, Slider& slider) override;

    void drawLinearSlider(Graphics& g, int x, int y, int width, int height, float sliderPos, float minSliderPos,
                          float maxSliderPos, const Slider::SliderStyle style, Slider& slider) override;

    void drawToggleButton(Graphics& g, ToggleButton& button, bool shouldDrawButtonAsHighlighted,
                          bool shouldDrawButtonAsDown) override;

    void drawButtonBackground(Graphics& g, Button& button, const Colour& backgroundColour,
                              bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;

    void drawComboBox(Graphics& g, int width, int height, bool isButtonDown, int buttonX, int buttonY, int buttonW,
                      int buttonH, ComboBox& box) override;

    // F2: Typography overrides -- JetBrains Mono for numeric readouts, Inter for buttons
    Label* createSliderTextBox(Slider& slider) override;
    Font getTextButtonFont(TextButton& button, int buttonHeight) override;
    void drawLabel(Graphics& g, Label& label) override;

    // Refresh colours from ColourScheme
    void refreshColours();

    // Theme-derived palette (public for NAMControl::paint to use)
    Colour ampBackground;      // Darkened plugin background
    Colour ampSurface;         // Slightly lighter surface for panels
    Colour ampBorder;          // Panel borders
    Colour ampHeaderBg;        // Header bar background
    Colour ampAccent;          // Primary accent (warm orange from Warning Colour)
    Colour ampAccentSecondary; // Secondary accent (from Slider Colour)
    Colour ampTextBright;      // Primary text
    Colour ampTextDim;         // Secondary/label text
    Colour ampLedOn;           // Active LED colour
    Colour ampLedOff;          // Inactive LED colour
    Colour ampKnobBody;        // Rotary knob body
    Colour ampKnobRing;        // Knob outer ring
    Colour ampTrackBg;         // Slider track background
    Colour ampButtonBg;        // Button background
    Colour ampButtonHover;     // Button hover state
    Colour ampInsetBg;         // Recessed display areas
};

//==============================================================================
/**
    Control component for NAMProcessor.
    Professional amp-style interface with model/IR loading,
    gain controls, noise gate, and tone stack.
    Colours are derived from the active theme for consistency.
*/
class NAMControl : public Component, public Button::Listener, public Slider::Listener, public Timer
{
  public:
    NAMControl(NAMProcessor* processor);
    ~NAMControl() override;

    void paint(Graphics& g) override;
    void resized() override;
    void buttonClicked(Button* button) override;
    void sliderValueChanged(Slider* slider) override;
    void timerCallback() override;
    void mouseDown(const MouseEvent& event) override;

    // Refresh theme colours
    void refreshColours();

    // F1: Collapsible editor
    void setCollapsed(bool collapsed);
    bool isCollapsed() const { return collapsed; }

  private:
    void updateModelDisplay();
    void updateIRDisplay();
    void drawSectionPanel(Graphics& g, const Rectangle<int>& bounds, const String& title);

    NAMProcessor* namProcessor;
    NAMLookAndFeel namLookAndFeel;
    bool collapsed = false;

    // LED animation state
    float ledPulsePhase = 0.0f;

    // Model loading
    std::unique_ptr<TextButton> loadModelButton;
    std::unique_ptr<TextButton> browseModelsButton;
    std::unique_ptr<TextButton> clearModelButton;
    std::unique_ptr<Label> modelNameLabel;
    std::unique_ptr<Label> modelArchLabel; // Architecture type badge

    // IR loading
    std::unique_ptr<TextButton> loadIRButton;
    std::unique_ptr<TextButton> clearIRButton;
    std::unique_ptr<Label> irNameLabel;
    std::unique_ptr<ToggleButton> irEnabledButton;

    // IR2 loading (second cabinet slot)
    std::unique_ptr<TextButton> loadIR2Button;
    std::unique_ptr<TextButton> clearIR2Button;
    std::unique_ptr<Label> ir2NameLabel;
    std::unique_ptr<ToggleButton> ir2EnabledButton;

    // IR blend
    std::unique_ptr<Slider> irBlendSlider;
    std::unique_ptr<Label> irBlendLabel;

    // IR filters
    std::unique_ptr<Slider> irLowCutSlider;
    std::unique_ptr<Label> irLowCutLabel;
    std::unique_ptr<Slider> irHighCutSlider;
    std::unique_ptr<Label> irHighCutLabel;

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
    std::unique_ptr<TextButton> toneStackPreButton;
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
    std::unique_ptr<FileChooser> ir2FileChooser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NAMControl)
};
