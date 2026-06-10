/*
  ==============================================================================

    IRLoaderControl.h
    UI control for the IR Loader processor

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

class IRLoaderProcessor;

//==============================================================================
/**
    Custom LookAndFeel for IR Loader controls.
    Matches the NAM Loader styling.
*/
class IRLoaderLookAndFeel : public LookAndFeel_V4
{
  public:
    IRLoaderLookAndFeel();

    void drawLinearSlider(Graphics& g, int x, int y, int width, int height, float sliderPos, float minSliderPos,
                          float maxSliderPos, const Slider::SliderStyle style, Slider& slider) override;

    void drawButtonBackground(Graphics& g, Button& button, const Colour& backgroundColour,
                              bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;
    void drawButtonText(Graphics& g, TextButton& button, bool shouldDrawButtonAsHighlighted,
                        bool shouldDrawButtonAsDown) override;
    void refreshColours();
};

//==============================================================================
/**
    Control component for IRLoaderProcessor.
    Shows IR file name, load button, and mix/filter controls.
    Uses inline styling matching the NAM Loader aesthetic.
*/
class IRLoaderControl : public Component, public Button::Listener, public Slider::Listener
{
  public:
    IRLoaderControl(IRLoaderProcessor* processor);
    ~IRLoaderControl() override;

    void paint(Graphics& g) override;
    void resized() override;
    void buttonClicked(Button* button) override;
    void sliderValueChanged(Slider* slider) override;

  private:
    void updateIRDisplay();

    IRLoaderProcessor* irProcessor;
    IRLoaderLookAndFeel irLookAndFeel;

    // File loading - IR1
    std::unique_ptr<TextButton> loadButton;
    std::unique_ptr<TextButton> browseButton;
    std::unique_ptr<TextButton> clearButton;
    std::unique_ptr<Label> irNameLabel;

    // File loading - IR2
    std::unique_ptr<TextButton> loadButton2;
    std::unique_ptr<TextButton> browseButton2;
    std::unique_ptr<TextButton> clearButton2;
    std::unique_ptr<Label> irName2Label;

    // Parameter controls
    std::unique_ptr<Slider> blendSlider;
    std::unique_ptr<Label> blendLabel;

    std::unique_ptr<Slider> mixSlider;
    std::unique_ptr<Label> mixLabel;

    std::unique_ptr<Slider> lowCutSlider;
    std::unique_ptr<Label> lowCutLabel;

    std::unique_ptr<Slider> highCutSlider;
    std::unique_ptr<Label> highCutLabel;

    // File chooser (kept alive for async operation)
    std::unique_ptr<FileChooser> fileChooser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(IRLoaderControl)
};
