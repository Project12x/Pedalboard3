/*
  ==============================================================================

    MidiUtilityControls.cpp
    Pedalboard3 - UI Controls for MIDI Utility Processors

  ==============================================================================
*/

#include "ColourScheme.h"
#include "MidiUtilityProcessors.h"


//==============================================================================
// Control for MIDI Transpose
//==============================================================================
class MidiTransposeControl : public Component, public Slider::Listener
{
  public:
    MidiTransposeControl(MidiTransposeProcessor* proc) : processor(proc)
    {
        addAndMakeVisible(transposeSlider);
        transposeSlider.setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
        transposeSlider.setTextBoxStyle(Slider::TextBoxBelow, false, 50, 18);
        transposeSlider.setRange(-48, 48, 1);
        transposeSlider.setValue(processor->getTranspose(), dontSendNotification);
        transposeSlider.addListener(this);

        addAndMakeVisible(label);
        label.setText("Semitones", dontSendNotification);
        label.setJustificationType(Justification::centred);
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced(4);
        label.setBounds(area.removeFromTop(16));
        transposeSlider.setBounds(area);
    }

    void sliderValueChanged(Slider* slider) override { processor->setTranspose(static_cast<int>(slider->getValue())); }

    void paint(Graphics& g) override
    {
        g.fillAll(ColourScheme::getInstance().colours["Plugin Background"]);
        g.setColour(ColourScheme::getInstance().colours["Plugin Border"]);
        g.drawRect(getLocalBounds(), 1);
    }

  private:
    MidiTransposeProcessor* processor;
    Slider transposeSlider;
    Label label;
};

//==============================================================================
// Control for MIDI Rechannelize
//==============================================================================
class MidiRechannelizeControl : public Component, public Slider::Listener
{
  public:
    MidiRechannelizeControl(MidiRechannelizeProcessor* proc) : processor(proc)
    {
        addAndMakeVisible(inputLabel);
        inputLabel.setText("From:", dontSendNotification);
        inputLabel.setJustificationType(Justification::centredLeft);

        addAndMakeVisible(inputSlider);
        inputSlider.setSliderStyle(Slider::IncDecButtons);
        inputSlider.setTextBoxStyle(Slider::TextBoxLeft, false, 40, 20);
        inputSlider.setRange(0, 16, 1);
        inputSlider.setValue(processor->getInputChannel(), dontSendNotification);
        inputSlider.textFromValueFunction = [](double v) { return v == 0 ? "All" : String(static_cast<int>(v)); };
        inputSlider.addListener(this);

        addAndMakeVisible(outputLabel);
        outputLabel.setText("To:", dontSendNotification);
        outputLabel.setJustificationType(Justification::centredLeft);

        addAndMakeVisible(outputSlider);
        outputSlider.setSliderStyle(Slider::IncDecButtons);
        outputSlider.setTextBoxStyle(Slider::TextBoxLeft, false, 40, 20);
        outputSlider.setRange(1, 16, 1);
        outputSlider.setValue(processor->getOutputChannel(), dontSendNotification);
        outputSlider.addListener(this);
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced(4);
        int rowH = area.getHeight() / 2;

        auto top = area.removeFromTop(rowH);
        inputLabel.setBounds(top.removeFromLeft(40));
        inputSlider.setBounds(top);

        outputLabel.setBounds(area.removeFromLeft(40));
        outputSlider.setBounds(area);
    }

    void sliderValueChanged(Slider* slider) override
    {
        if (slider == &inputSlider)
            processor->setInputChannel(static_cast<int>(slider->getValue()));
        else if (slider == &outputSlider)
            processor->setOutputChannel(static_cast<int>(slider->getValue()));
    }

    void paint(Graphics& g) override
    {
        g.fillAll(ColourScheme::getInstance().colours["Plugin Background"]);
        g.setColour(ColourScheme::getInstance().colours["Plugin Border"]);
        g.drawRect(getLocalBounds(), 1);
    }

  private:
    MidiRechannelizeProcessor* processor;
    Slider inputSlider, outputSlider;
    Label inputLabel, outputLabel;
};

//==============================================================================
// Control for Keyboard Split
//==============================================================================
class KeyboardSplitControl : public Component, public Slider::Listener
{
  public:
    KeyboardSplitControl(KeyboardSplitProcessor* proc) : processor(proc)
    {
        addAndMakeVisible(splitLabel);
        splitLabel.setText("Split:", dontSendNotification);
        splitLabel.setJustificationType(Justification::centredLeft);

        addAndMakeVisible(splitSlider);
        splitSlider.setSliderStyle(Slider::IncDecButtons);
        splitSlider.setTextBoxStyle(Slider::TextBoxLeft, false, 50, 20);
        splitSlider.setRange(0, 127, 1);
        splitSlider.setValue(processor->getSplitPoint(), dontSendNotification);
        splitSlider.textFromValueFunction = [](double v)
        { return KeyboardSplitProcessor::getNoteNameFromMidi(static_cast<int>(v)); };
        splitSlider.addListener(this);

        addAndMakeVisible(lowerLabel);
        lowerLabel.setText("Lower Ch:", dontSendNotification);
        lowerLabel.setJustificationType(Justification::centredLeft);

        addAndMakeVisible(lowerSlider);
        lowerSlider.setSliderStyle(Slider::IncDecButtons);
        lowerSlider.setTextBoxStyle(Slider::TextBoxLeft, false, 30, 20);
        lowerSlider.setRange(1, 16, 1);
        lowerSlider.setValue(processor->getLowerChannel(), dontSendNotification);
        lowerSlider.addListener(this);

        addAndMakeVisible(upperLabel);
        upperLabel.setText("Upper Ch:", dontSendNotification);
        upperLabel.setJustificationType(Justification::centredLeft);

        addAndMakeVisible(upperSlider);
        upperSlider.setSliderStyle(Slider::IncDecButtons);
        upperSlider.setTextBoxStyle(Slider::TextBoxLeft, false, 30, 20);
        upperSlider.setRange(1, 16, 1);
        upperSlider.setValue(processor->getUpperChannel(), dontSendNotification);
        upperSlider.addListener(this);
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced(4);
        int rowH = area.getHeight() / 3;

        auto row1 = area.removeFromTop(rowH);
        splitLabel.setBounds(row1.removeFromLeft(50));
        splitSlider.setBounds(row1);

        auto row2 = area.removeFromTop(rowH);
        lowerLabel.setBounds(row2.removeFromLeft(65));
        lowerSlider.setBounds(row2);

        upperLabel.setBounds(area.removeFromLeft(65));
        upperSlider.setBounds(area);
    }

    void sliderValueChanged(Slider* slider) override
    {
        if (slider == &splitSlider)
            processor->setSplitPoint(static_cast<int>(slider->getValue()));
        else if (slider == &lowerSlider)
            processor->setLowerChannel(static_cast<int>(slider->getValue()));
        else if (slider == &upperSlider)
            processor->setUpperChannel(static_cast<int>(slider->getValue()));
    }

    void paint(Graphics& g) override
    {
        g.fillAll(ColourScheme::getInstance().colours["Plugin Background"]);
        g.setColour(ColourScheme::getInstance().colours["Plugin Border"]);
        g.drawRect(getLocalBounds(), 1);
    }

  private:
    KeyboardSplitProcessor* processor;
    Slider splitSlider, lowerSlider, upperSlider;
    Label splitLabel, lowerLabel, upperLabel;
};

//==============================================================================
// Factory methods for getControls()
//==============================================================================

// These are defined here and linked via the .cpp implementation

// MidiTransposeProcessor
Component* MidiTransposeProcessor::getControls()
{
    return new MidiTransposeControl(this);
}

AudioProcessorEditor* MidiTransposeProcessor::createEditor()
{
    return new GenericAudioProcessorEditor(*this);
}

// MidiRechannelizeProcessor
Component* MidiRechannelizeProcessor::getControls()
{
    return new MidiRechannelizeControl(this);
}

AudioProcessorEditor* MidiRechannelizeProcessor::createEditor()
{
    return new GenericAudioProcessorEditor(*this);
}

// KeyboardSplitProcessor
Component* KeyboardSplitProcessor::getControls()
{
    return new KeyboardSplitControl(this);
}

AudioProcessorEditor* KeyboardSplitProcessor::createEditor()
{
    return new GenericAudioProcessorEditor(*this);
}
