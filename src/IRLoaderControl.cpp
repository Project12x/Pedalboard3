/*
  ==============================================================================

    IRLoaderControl.cpp
    UI control for the IR Loader processor
    Professional styling matching NAM Loader aesthetic

  ==============================================================================
*/

#include "IRLoaderControl.h"

#include "IRLoaderProcessor.h"
#include "NAMModelBrowser.h"

//==============================================================================
// IRLoaderLookAndFeel Implementation
//==============================================================================
IRLoaderLookAndFeel::IRLoaderLookAndFeel()
{
    // Set dark color scheme
    setColour(Slider::backgroundColourId, Colour(0xff1a1a1a));
    setColour(Slider::trackColourId, Colour(0xff4a90d9));
    setColour(Slider::thumbColourId, Colour(0xffe0e0e0));
    setColour(TextButton::buttonColourId, Colour(0xff3a3a3a));
    setColour(TextButton::textColourOffId, Colour(0xffe0e0e0));
    setColour(Label::textColourId, Colour(0xffe0e0e0));
}

void IRLoaderLookAndFeel::drawLinearSlider(Graphics& g, int x, int y, int width, int height, float sliderPos,
                                           float minSliderPos, float maxSliderPos, const Slider::SliderStyle style,
                                           Slider& slider)
{
    const bool isHorizontal = (style == Slider::LinearHorizontal || style == Slider::LinearBar);
    const float trackThickness = 4.0f;

    Rectangle<float> track;
    if (isHorizontal)
    {
        track = Rectangle<float>(static_cast<float>(x), y + (height - trackThickness) * 0.5f, static_cast<float>(width),
                                 trackThickness);
    }
    else
    {
        track = Rectangle<float>(x + (width - trackThickness) * 0.5f, static_cast<float>(y), trackThickness,
                                 static_cast<float>(height));
    }

    // Track background (inset effect)
    g.setColour(Colour(0xff101010));
    g.fillRoundedRectangle(track, 2.0f);
    g.setColour(Colour(0xff080808));
    g.drawRoundedRectangle(track, 2.0f, 1.0f);

    // Filled portion
    Rectangle<float> filledTrack;
    if (isHorizontal)
    {
        const float fillWidth = sliderPos - x;
        filledTrack = Rectangle<float>(static_cast<float>(x), track.getY(), fillWidth, trackThickness);
    }
    else
    {
        const float fillHeight = (y + height) - sliderPos;
        filledTrack = Rectangle<float>(track.getX(), sliderPos, trackThickness, fillHeight);
    }

    ColourGradient fillGradient(Colour(0xff4a90d9), filledTrack.getX(), filledTrack.getY(), Colour(0xff3070a0),
                                filledTrack.getRight(), filledTrack.getBottom(), false);
    g.setGradientFill(fillGradient);
    g.fillRoundedRectangle(filledTrack, 2.0f);

    // Thumb
    const float thumbSize = 14.0f;
    float thumbX, thumbY;
    if (isHorizontal)
    {
        thumbX = sliderPos - thumbSize * 0.5f;
        thumbY = y + (height - thumbSize) * 0.5f;
    }
    else
    {
        thumbX = x + (width - thumbSize) * 0.5f;
        thumbY = sliderPos - thumbSize * 0.5f;
    }

    // Thumb shadow
    g.setColour(Colours::black.withAlpha(0.3f));
    g.fillEllipse(thumbX + 1, thumbY + 1, thumbSize, thumbSize);

    // Thumb body
    ColourGradient thumbGradient(Colour(0xff505050), thumbX, thumbY, Colour(0xff303030), thumbX, thumbY + thumbSize,
                                 false);
    g.setGradientFill(thumbGradient);
    g.fillEllipse(thumbX, thumbY, thumbSize, thumbSize);

    // Thumb highlight
    g.setColour(Colour(0xff606060));
    g.drawEllipse(thumbX, thumbY, thumbSize, thumbSize, 1.0f);
}

void IRLoaderLookAndFeel::drawButtonBackground(Graphics& g, Button& button, const Colour& backgroundColour,
                                               bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
{
    auto bounds = button.getLocalBounds().toFloat().reduced(0.5f);

    Colour baseColour = shouldDrawButtonAsDown          ? Colour(0xff252525)
                        : shouldDrawButtonAsHighlighted ? Colour(0xff454545)
                                                        : Colour(0xff353535);

    // Button shadow
    g.setColour(Colours::black.withAlpha(0.3f));
    g.fillRoundedRectangle(bounds.translated(0, 1), 4.0f);

    // Button body gradient
    ColourGradient buttonGradient(baseColour.brighter(0.1f), bounds.getX(), bounds.getY(), baseColour.darker(0.1f),
                                  bounds.getX(), bounds.getBottom(), false);
    g.setGradientFill(buttonGradient);
    g.fillRoundedRectangle(bounds, 4.0f);

    // Border
    g.setColour(Colour(0xff505050));
    g.drawRoundedRectangle(bounds, 4.0f, 1.0f);
}

//==============================================================================
// IRLoaderControl Implementation
//==============================================================================
IRLoaderControl::IRLoaderControl(IRLoaderProcessor* processor) : irProcessor(processor)
{
    setLookAndFeel(&irLookAndFeel);

    // Load button
    loadButton = std::make_unique<TextButton>("Load");
    loadButton->addListener(this);
    addAndMakeVisible(loadButton.get());

    // Browse button (opens IR browser window)
    browseButton = std::make_unique<TextButton>("Browse");
    browseButton->setTooltip("Browse IR Library");
    browseButton->addListener(this);
    addAndMakeVisible(browseButton.get());

    // Clear button
    clearButton = std::make_unique<TextButton>("X");
    clearButton->setTooltip("Clear IR");
    clearButton->addListener(this);
    addAndMakeVisible(clearButton.get());

    // IR name display
    irNameLabel = std::make_unique<Label>("irName", "No IR Loaded");
    irNameLabel->setJustificationType(Justification::centredLeft);
    irNameLabel->setColour(Label::textColourId, Colour(kTextBright));
    irNameLabel->setColour(Label::backgroundColourId, Colour(0xff151515));
    irNameLabel->setColour(Label::outlineColourId, Colour(0xff080808));
    addAndMakeVisible(irNameLabel.get());

    // IR2 controls
    loadButton2 = std::make_unique<TextButton>("Load");
    loadButton2->addListener(this);
    addAndMakeVisible(loadButton2.get());

    browseButton2 = std::make_unique<TextButton>("Browse");
    browseButton2->setTooltip("Browse IR Library (Slot 2)");
    browseButton2->addListener(this);
    addAndMakeVisible(browseButton2.get());

    clearButton2 = std::make_unique<TextButton>("X");
    clearButton2->setTooltip("Clear IR 2");
    clearButton2->addListener(this);
    addAndMakeVisible(clearButton2.get());

    irName2Label = std::make_unique<Label>("irName2", "No IR 2 Loaded");
    irName2Label->setJustificationType(Justification::centredLeft);
    irName2Label->setColour(Label::textColourId, Colour(kTextBright));
    irName2Label->setColour(Label::backgroundColourId, Colour(0xff151515));
    irName2Label->setColour(Label::outlineColourId, Colour(0xff080808));
    addAndMakeVisible(irName2Label.get());

    // Blend slider (0 = IR1 only, 100 = IR2 only)
    blendSlider = std::make_unique<Slider>(Slider::LinearHorizontal, Slider::TextBoxRight);
    blendSlider->setRange(0.0, 100.0, 1.0);
    blendSlider->setValue(irProcessor->getBlend() * 100.0);
    blendSlider->addListener(this);
    blendSlider->setTextValueSuffix("%");
    blendSlider->setNumDecimalPlacesToDisplay(0);
    blendSlider->setTextBoxStyle(Slider::TextBoxRight, false, 45, 18);
    addAndMakeVisible(blendSlider.get());

    blendLabel = std::make_unique<Label>("blendLabel", "BLEND");
    blendLabel->setJustificationType(Justification::centredRight);
    blendLabel->setColour(Label::textColourId, Colour(kTextDim));
    blendLabel->setFont(Font(11.0f, Font::bold));
    addAndMakeVisible(blendLabel.get());

    // Mix slider (0-100% display)
    mixSlider = std::make_unique<Slider>(Slider::LinearHorizontal, Slider::TextBoxRight);
    mixSlider->setRange(0.0, 100.0, 1.0);
    mixSlider->setValue(irProcessor->getMix() * 100.0);
    mixSlider->addListener(this);
    mixSlider->setTextValueSuffix("%");
    mixSlider->setNumDecimalPlacesToDisplay(0);
    mixSlider->setTextBoxStyle(Slider::TextBoxRight, false, 45, 18);
    addAndMakeVisible(mixSlider.get());

    mixLabel = std::make_unique<Label>("mixLabel", "MIX");
    mixLabel->setJustificationType(Justification::centredRight);
    mixLabel->setColour(Label::textColourId, Colour(kTextDim));
    mixLabel->setFont(Font(11.0f, Font::bold));
    addAndMakeVisible(mixLabel.get());

    // Low cut slider
    lowCutSlider = std::make_unique<Slider>(Slider::LinearHorizontal, Slider::TextBoxRight);
    lowCutSlider->setRange(20.0, 500.0, 1.0);
    lowCutSlider->setValue(irProcessor->getLowCut());
    lowCutSlider->addListener(this);
    lowCutSlider->setTextValueSuffix(" Hz");
    lowCutSlider->setSkewFactorFromMidPoint(100.0);
    lowCutSlider->setTextBoxStyle(Slider::TextBoxRight, false, 55, 18);
    addAndMakeVisible(lowCutSlider.get());

    lowCutLabel = std::make_unique<Label>("lowCutLabel", "LO CUT");
    lowCutLabel->setJustificationType(Justification::centredRight);
    lowCutLabel->setColour(Label::textColourId, Colour(kTextDim));
    lowCutLabel->setFont(Font(11.0f, Font::bold));
    addAndMakeVisible(lowCutLabel.get());

    // High cut slider
    highCutSlider = std::make_unique<Slider>(Slider::LinearHorizontal, Slider::TextBoxRight);
    highCutSlider->setRange(2000.0, 20000.0, 100.0);
    highCutSlider->setValue(irProcessor->getHighCut());
    highCutSlider->addListener(this);
    highCutSlider->setTextValueSuffix(" Hz");
    highCutSlider->setSkewFactorFromMidPoint(6000.0);
    highCutSlider->setTextBoxStyle(Slider::TextBoxRight, false, 55, 18);
    addAndMakeVisible(highCutSlider.get());

    highCutLabel = std::make_unique<Label>("highCutLabel", "HI CUT");
    highCutLabel->setJustificationType(Justification::centredRight);
    highCutLabel->setColour(Label::textColourId, Colour(kTextDim));
    highCutLabel->setFont(Font(11.0f, Font::bold));
    addAndMakeVisible(highCutLabel.get());

    // Update display
    updateIRDisplay();
}

IRLoaderControl::~IRLoaderControl()
{
    setLookAndFeel(nullptr);
}

//==============================================================================
void IRLoaderControl::paint(Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    // Main background with subtle gradient
    ColourGradient bgGradient(Colour(kBackgroundMid), 0, 0, Colour(kBackgroundDark), 0, bounds.getHeight(), false);
    g.setGradientFill(bgGradient);
    g.fillRoundedRectangle(bounds, 4.0f);

    // Outer border with bevel effect
    g.setColour(Colour(0xff101010));
    g.drawRoundedRectangle(bounds.reduced(0.5f), 4.0f, 1.0f);
    g.setColour(Colour(0xff404040));
    g.drawRoundedRectangle(bounds.reduced(1.5f), 3.0f, 1.0f);

    // Header bar with rounded top corners
    Rectangle<float> headerBounds(1, 1, bounds.getWidth() - 2, 27);
    Path headerPath;
    headerPath.addRoundedRectangle(headerBounds.getX(), headerBounds.getY(), headerBounds.getWidth(),
                                   headerBounds.getHeight(), 3.0f, 3.0f, true, true, false, false);
    ColourGradient headerGradient(Colour(kHeaderAccent).brighter(0.1f), 0, 0, Colour(kBackgroundDark), 0, 28, false);
    g.setGradientFill(headerGradient);
    g.fillPath(headerPath);

    // Header bottom line
    g.setColour(Colour(0xff101010));
    g.drawHorizontalLine(27, 1, bounds.getWidth() - 1);
    g.setColour(Colour(0xff505050).withAlpha(0.5f));
    g.drawHorizontalLine(28, 1, bounds.getWidth() - 1);

    // Cabinet icon (speaker cone representation)
    const float iconX = 10.0f;
    const float iconY = 6.0f;
    const float iconSize = 16.0f;

    // Outer ring
    g.setColour(Colour(kTextDim));
    g.drawEllipse(iconX, iconY, iconSize, iconSize, 1.5f);
    // Inner cone
    g.setColour(Colour(kTextBright).withAlpha(0.8f));
    g.fillEllipse(iconX + 5, iconY + 5, iconSize - 10, iconSize - 10);
    g.setColour(Colour(kTextDim));
    g.drawEllipse(iconX + 5, iconY + 5, iconSize - 10, iconSize - 10, 1.0f);

    // Title text
    g.setColour(Colour(kTextBright));
    g.setFont(Font(13.0f, Font::bold));
    g.drawText("IR LOADER", Rectangle<float>(iconX + iconSize + 6, 0, 100, 28), Justification::centredLeft);

    // Status LED (IR loaded indicator)
    const float ledSize = 8.0f;
    const float ledX = bounds.getWidth() - 18.0f;
    const float ledY = (28 - ledSize) * 0.5f;

    Colour ledColour = irProcessor->isIRLoaded() ? Colour(kLedOn) : Colour(kLedOff);

    // LED glow effect
    if (irProcessor->isIRLoaded())
    {
        g.setColour(ledColour.withAlpha(0.3f));
        g.fillEllipse(ledX - 4, ledY - 4, ledSize + 8, ledSize + 8);
        g.setColour(ledColour.withAlpha(0.15f));
        g.fillEllipse(ledX - 6, ledY - 6, ledSize + 12, ledSize + 12);
    }

    // LED body with gradient
    ColourGradient ledGradient(ledColour.brighter(0.4f), ledX, ledY, ledColour.darker(0.3f), ledX, ledY + ledSize,
                               false);
    g.setGradientFill(ledGradient);
    g.fillEllipse(ledX, ledY, ledSize, ledSize);

    // LED rim
    g.setColour(Colour(0xff101010));
    g.drawEllipse(ledX, ledY, ledSize, ledSize, 1.0f);

    // Subtle section separator above sliders
    float separatorY = 82.0f;
    g.setColour(Colour(0xff101010));
    g.drawHorizontalLine(static_cast<int>(separatorY), 8, bounds.getWidth() - 8);
    g.setColour(Colour(0xff353535));
    g.drawHorizontalLine(static_cast<int>(separatorY) + 1, 8, bounds.getWidth() - 8);
}

void IRLoaderControl::resized()
{
    auto bounds = getLocalBounds();
    bounds.removeFromTop(32); // Header space
    bounds = bounds.reduced(8, 4);

    const int rowHeight = 22;
    const int labelWidth = 45;
    const int clearButtonWidth = 22;
    const int spacing = 4;

    // Row 1: IR1 - Load + Browse + Clear + IR name
    auto row1 = bounds.removeFromTop(rowHeight);
    loadButton->setBounds(row1.removeFromLeft(45));
    row1.removeFromLeft(spacing);
    browseButton->setBounds(row1.removeFromLeft(55));
    row1.removeFromLeft(spacing);
    clearButton->setBounds(row1.removeFromLeft(clearButtonWidth));
    row1.removeFromLeft(spacing);
    irNameLabel->setBounds(row1);

    bounds.removeFromTop(spacing);

    // Row 2: IR2 - Load + Browse + Clear + IR name
    auto row2 = bounds.removeFromTop(rowHeight);
    loadButton2->setBounds(row2.removeFromLeft(45));
    row2.removeFromLeft(spacing);
    browseButton2->setBounds(row2.removeFromLeft(55));
    row2.removeFromLeft(spacing);
    clearButton2->setBounds(row2.removeFromLeft(clearButtonWidth));
    row2.removeFromLeft(spacing);
    irName2Label->setBounds(row2);

    bounds.removeFromTop(spacing + 2);

    // Row 3: Blend slider
    auto row3 = bounds.removeFromTop(rowHeight);
    blendLabel->setBounds(row3.removeFromLeft(labelWidth));
    row3.removeFromLeft(spacing);
    blendSlider->setBounds(row3);

    bounds.removeFromTop(spacing);

    // Row 4: Mix slider
    auto row4 = bounds.removeFromTop(rowHeight);
    mixLabel->setBounds(row4.removeFromLeft(labelWidth));
    row4.removeFromLeft(spacing);
    mixSlider->setBounds(row4);

    bounds.removeFromTop(spacing);

    // Row 5: Low cut slider
    auto row5 = bounds.removeFromTop(rowHeight);
    lowCutLabel->setBounds(row5.removeFromLeft(labelWidth));
    row5.removeFromLeft(spacing);
    lowCutSlider->setBounds(row5);

    bounds.removeFromTop(spacing);

    // Row 6: High cut slider
    auto row6 = bounds.removeFromTop(rowHeight);
    highCutLabel->setBounds(row6.removeFromLeft(labelWidth));
    row6.removeFromLeft(spacing);
    highCutSlider->setBounds(row6);
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
    else if (button == loadButton2.get())
    {
        fileChooser = std::make_unique<FileChooser>("Select Impulse Response (Slot 2)",
                                                    File::getSpecialLocation(File::userDocumentsDirectory),
                                                    "*.wav;*.aiff;*.aif", true);

        auto chooserFlags = FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles;

        fileChooser->launchAsync(chooserFlags,
                                 [this](const FileChooser& fc)
                                 {
                                     auto result = fc.getResult();
                                     if (result.existsAsFile())
                                     {
                                         irProcessor->loadIRFile2(result);
                                         updateIRDisplay();
                                         repaint();
                                     }
                                 });
    }
    else if (button == browseButton.get())
    {
        IRBrowser::showWindow(
            [this](const File& irFile)
            {
                irProcessor->loadIRFile(irFile);
                updateIRDisplay();
                repaint();
            });
    }
    else if (button == browseButton2.get())
    {
        IRBrowser::showWindow(
            [this](const File& irFile)
            {
                irProcessor->loadIRFile2(irFile);
                updateIRDisplay();
                repaint();
            });
    }
    else if (button == clearButton.get())
    {
        irProcessor->loadIRFile(File());
        updateIRDisplay();
        repaint();
    }
    else if (button == clearButton2.get())
    {
        irProcessor->clearIR2();
        updateIRDisplay();
        repaint();
    }
}

void IRLoaderControl::sliderValueChanged(Slider* slider)
{
    if (slider == mixSlider.get())
    {
        irProcessor->setMix(static_cast<float>(slider->getValue() / 100.0));
    }
    else if (slider == lowCutSlider.get())
    {
        irProcessor->setLowCut(static_cast<float>(slider->getValue()));
    }
    else if (slider == highCutSlider.get())
    {
        irProcessor->setHighCut(static_cast<float>(slider->getValue()));
    }
    else if (slider == blendSlider.get())
    {
        irProcessor->setBlend(static_cast<float>(slider->getValue() / 100.0));
    }
}

//==============================================================================
void IRLoaderControl::updateIRDisplay()
{
    if (irProcessor->isIRLoaded())
    {
        irNameLabel->setText(irProcessor->getIRName(), dontSendNotification);
        irNameLabel->setColour(Label::textColourId, Colour(kTextBright));
    }
    else
    {
        irNameLabel->setText("No IR Loaded", dontSendNotification);
        irNameLabel->setColour(Label::textColourId, Colour(kTextDim));
    }

    if (irProcessor->isIR2Loaded())
    {
        irName2Label->setText(irProcessor->getIR2Name(), dontSendNotification);
        irName2Label->setColour(Label::textColourId, Colour(kTextBright));
    }
    else
    {
        irName2Label->setText("No IR 2 Loaded", dontSendNotification);
        irName2Label->setColour(Label::textColourId, Colour(kTextDim));
    }
}
