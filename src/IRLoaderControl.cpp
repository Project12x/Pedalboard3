/*
  ==============================================================================

    IRLoaderControl.cpp
    UI control for the IR Loader processor

  ==============================================================================
*/

#include "IRLoaderControl.h"

#include "IRLoaderProcessor.h"

//==============================================================================
IRLoaderControl::IRLoaderControl(IRLoaderProcessor* processor) : irProcessor(processor)
{
    // Load button
    loadButton = std::make_unique<TextButton>("Load IR");
    loadButton->addListener(this);
    addAndMakeVisible(loadButton.get());

    // IR name display
    irNameLabel = std::make_unique<Label>("irName", "No IR Loaded");
    irNameLabel->setJustificationType(Justification::centredLeft);
    irNameLabel->setColour(Label::textColourId, Colours::white);
    irNameLabel->setColour(Label::backgroundColourId, Colours::transparentBlack);
    addAndMakeVisible(irNameLabel.get());

    // Mix slider
    mixSlider = std::make_unique<Slider>(Slider::LinearHorizontal, Slider::TextBoxRight);
    mixSlider->setRange(0.0, 1.0, 0.01);
    mixSlider->setValue(irProcessor->getMix());
    mixSlider->addListener(this);
    mixSlider->setTextValueSuffix("%");
    mixSlider->setNumDecimalPlacesToDisplay(0);
    addAndMakeVisible(mixSlider.get());

    mixLabel = std::make_unique<Label>("mixLabel", "Mix");
    mixLabel->setJustificationType(Justification::centredRight);
    mixLabel->setColour(Label::textColourId, Colours::white);
    addAndMakeVisible(mixLabel.get());

    // Low cut slider
    lowCutSlider = std::make_unique<Slider>(Slider::LinearHorizontal, Slider::TextBoxRight);
    lowCutSlider->setRange(20.0, 500.0, 1.0);
    lowCutSlider->setValue(irProcessor->getLowCut());
    lowCutSlider->addListener(this);
    lowCutSlider->setTextValueSuffix(" Hz");
    lowCutSlider->setSkewFactorFromMidPoint(100.0);
    addAndMakeVisible(lowCutSlider.get());

    lowCutLabel = std::make_unique<Label>("lowCutLabel", "Lo Cut");
    lowCutLabel->setJustificationType(Justification::centredRight);
    lowCutLabel->setColour(Label::textColourId, Colours::white);
    addAndMakeVisible(lowCutLabel.get());

    // High cut slider
    highCutSlider = std::make_unique<Slider>(Slider::LinearHorizontal, Slider::TextBoxRight);
    highCutSlider->setRange(2000.0, 20000.0, 100.0);
    highCutSlider->setValue(irProcessor->getHighCut());
    highCutSlider->addListener(this);
    highCutSlider->setTextValueSuffix(" Hz");
    highCutSlider->setSkewFactorFromMidPoint(6000.0);
    addAndMakeVisible(highCutSlider.get());

    highCutLabel = std::make_unique<Label>("highCutLabel", "Hi Cut");
    highCutLabel->setJustificationType(Justification::centredRight);
    highCutLabel->setColour(Label::textColourId, Colours::white);
    addAndMakeVisible(highCutLabel.get());

    // Update display
    updateIRDisplay();
}

IRLoaderControl::~IRLoaderControl() {}

//==============================================================================
void IRLoaderControl::paint(Graphics& g)
{
    // Dark background
    g.fillAll(Colour(0xff2a2a2a));

    // Border
    g.setColour(Colour(0xff404040));
    g.drawRect(getLocalBounds(), 1);

    // Header area
    auto headerBounds = getLocalBounds().removeFromTop(28);
    g.setColour(Colour(0xff1a1a1a));
    g.fillRect(headerBounds);

    // Title
    g.setColour(Colours::white);
    g.setFont(14.0f);
    g.drawText("IR LOADER", headerBounds.reduced(8, 0), Justification::centredLeft);

    // Status indicator
    Colour statusColour = irProcessor->isIRLoaded() ? Colours::limegreen : Colours::grey;
    g.setColour(statusColour);
    g.fillEllipse(headerBounds.getRight() - 20.0f, headerBounds.getCentreY() - 5.0f, 10.0f, 10.0f);
}

void IRLoaderControl::resized()
{
    auto bounds = getLocalBounds().reduced(8);
    bounds.removeFromTop(28); // Header space

    const int rowHeight = 24;
    const int labelWidth = 50;
    const int buttonWidth = 70;
    const int spacing = 4;

    // Row 1: Load button + IR name
    auto row1 = bounds.removeFromTop(rowHeight);
    loadButton->setBounds(row1.removeFromLeft(buttonWidth));
    row1.removeFromLeft(spacing);
    irNameLabel->setBounds(row1);

    bounds.removeFromTop(spacing);

    // Row 2: Mix slider
    auto row2 = bounds.removeFromTop(rowHeight);
    mixLabel->setBounds(row2.removeFromLeft(labelWidth));
    row2.removeFromLeft(spacing);
    mixSlider->setBounds(row2);

    bounds.removeFromTop(spacing);

    // Row 3: Low cut slider
    auto row3 = bounds.removeFromTop(rowHeight);
    lowCutLabel->setBounds(row3.removeFromLeft(labelWidth));
    row3.removeFromLeft(spacing);
    lowCutSlider->setBounds(row3);

    bounds.removeFromTop(spacing);

    // Row 4: High cut slider
    auto row4 = bounds.removeFromTop(rowHeight);
    highCutLabel->setBounds(row4.removeFromLeft(labelWidth));
    row4.removeFromLeft(spacing);
    highCutSlider->setBounds(row4);
}

//==============================================================================
void IRLoaderControl::buttonClicked(Button* button)
{
    if (button == loadButton.get())
    {
        fileChooser = std::make_unique<FileChooser>("Select Impulse Response",
                                                    File::getSpecialLocation(File::userDocumentsDirectory),
                                                    "*.wav;*.aiff;*.aif", true);

        auto chooserFlags = FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles;

        fileChooser->launchAsync(chooserFlags,
                                 [this](const FileChooser& fc)
                                 {
                                     auto result = fc.getResult();
                                     if (result.existsAsFile())
                                     {
                                         irProcessor->loadIRFile(result);
                                         updateIRDisplay();
                                         repaint();
                                     }
                                 });
    }
}

void IRLoaderControl::sliderValueChanged(Slider* slider)
{
    if (slider == mixSlider.get())
    {
        irProcessor->setMix(static_cast<float>(slider->getValue()));
    }
    else if (slider == lowCutSlider.get())
    {
        irProcessor->setLowCut(static_cast<float>(slider->getValue()));
    }
    else if (slider == highCutSlider.get())
    {
        irProcessor->setHighCut(static_cast<float>(slider->getValue()));
    }
}

//==============================================================================
void IRLoaderControl::updateIRDisplay()
{
    if (irProcessor->isIRLoaded())
    {
        irNameLabel->setText(irProcessor->getIRName(), dontSendNotification);
    }
    else
    {
        irNameLabel->setText("No IR Loaded", dontSendNotification);
    }
}
