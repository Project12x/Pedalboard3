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
    Control component for IRLoaderProcessor.
    Shows IR file name, load button, and mix/filter controls.
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

    // File loading
    std::unique_ptr<TextButton> loadButton;
    std::unique_ptr<Label> irNameLabel;

    // Parameter controls
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
