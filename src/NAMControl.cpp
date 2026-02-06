/*
  ==============================================================================

    NAMControl.cpp
    UI control for the NAM (Neural Amp Modeler) processor
    Professional amp-style interface

  ==============================================================================
*/

#include "NAMControl.h"

#include "NAMModelBrowser.h"
#include "NAMProcessor.h"
#include "SubGraphProcessor.h"

//==============================================================================
// NAMLookAndFeel Implementation
//==============================================================================
NAMLookAndFeel::NAMLookAndFeel()
{
    // Set dark color scheme
    setColour(Slider::backgroundColourId, Colour(0xff1a1a1a));
    setColour(Slider::trackColourId, Colour(0xff4a90d9));
    setColour(Slider::thumbColourId, Colour(0xffe0e0e0));
    setColour(TextButton::buttonColourId, Colour(0xff3a3a3a));
    setColour(TextButton::textColourOffId, Colour(0xffe0e0e0));
    setColour(ToggleButton::textColourId, Colour(0xffe0e0e0));
    setColour(Label::textColourId, Colour(0xffe0e0e0));
}

void NAMLookAndFeel::drawRotarySlider(Graphics& g, int x, int y, int width, int height,
                                       float sliderPos, float rotaryStartAngle,
                                       float rotaryEndAngle, Slider& slider)
{
    const float radius = jmin(width / 2.0f, height / 2.0f) - 4.0f;
    const float centreX = x + width * 0.5f;
    const float centreY = y + height * 0.5f;
    const float rx = centreX - radius;
    const float ry = centreY - radius;
    const float rw = radius * 2.0f;
    const float angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

    // Outer ring (metallic effect)
    ColourGradient outerGradient(Colour(0xff505050), centreX, centreY - radius,
                                  Colour(0xff202020), centreX, centreY + radius, false);
    g.setGradientFill(outerGradient);
    g.fillEllipse(rx - 2, ry - 2, rw + 4, rw + 4);

    // Main knob body
    ColourGradient knobGradient(Colour(0xff404040), centreX, centreY - radius,
                                 Colour(0xff1a1a1a), centreX, centreY + radius, false);
    g.setGradientFill(knobGradient);
    g.fillEllipse(rx, ry, rw, rw);

    // Inner circle (darker)
    const float innerRadius = radius * 0.7f;
    ColourGradient innerGradient(Colour(0xff2a2a2a), centreX, centreY - innerRadius,
                                  Colour(0xff151515), centreX, centreY + innerRadius, false);
    g.setGradientFill(innerGradient);
    g.fillEllipse(centreX - innerRadius, centreY - innerRadius,
                  innerRadius * 2.0f, innerRadius * 2.0f);

    // Pointer/indicator line
    Path p;
    const float pointerLength = radius * 0.6f;
    const float pointerThickness = 3.0f;
    p.addRectangle(-pointerThickness * 0.5f, -pointerLength, pointerThickness, pointerLength);
    p.applyTransform(AffineTransform::rotation(angle).translated(centreX, centreY));

    // Glow effect for pointer
    g.setColour(Colour(0xffff8c00).withAlpha(0.3f));
    g.fillPath(p);
    g.setColour(Colour(0xffff8c00));
    g.fillPath(p);

    // Tick marks around the knob
    g.setColour(Colour(0xff606060));
    const int numTicks = 11;
    for (int i = 0; i < numTicks; ++i)
    {
        const float tickAngle = rotaryStartAngle + (float)i / (numTicks - 1) * (rotaryEndAngle - rotaryStartAngle);
        const float tickInnerRadius = radius + 4.0f;
        const float tickOuterRadius = radius + 8.0f;

        Point<float> innerPoint(centreX + tickInnerRadius * std::sin(tickAngle),
                                 centreY - tickInnerRadius * std::cos(tickAngle));
        Point<float> outerPoint(centreX + tickOuterRadius * std::sin(tickAngle),
                                 centreY - tickOuterRadius * std::cos(tickAngle));

        g.drawLine(innerPoint.x, innerPoint.y, outerPoint.x, outerPoint.y, 1.0f);
    }
}

void NAMLookAndFeel::drawLinearSlider(Graphics& g, int x, int y, int width, int height,
                                       float sliderPos, float minSliderPos, float maxSliderPos,
                                       const Slider::SliderStyle style, Slider& slider)
{
    const bool isHorizontal = (style == Slider::LinearHorizontal || style == Slider::LinearBar);
    const float trackThickness = 4.0f;

    Rectangle<float> track;
    if (isHorizontal)
    {
        track = Rectangle<float>(x, y + (height - trackThickness) * 0.5f, width, trackThickness);
    }
    else
    {
        track = Rectangle<float>(x + (width - trackThickness) * 0.5f, y, trackThickness, height);
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
        filledTrack = Rectangle<float>(x, track.getY(), fillWidth, trackThickness);
    }
    else
    {
        const float fillHeight = (y + height) - sliderPos;
        filledTrack = Rectangle<float>(track.getX(), sliderPos, trackThickness, fillHeight);
    }

    ColourGradient fillGradient(Colour(0xff4a90d9), filledTrack.getX(), filledTrack.getY(),
                                 Colour(0xff3070a0), filledTrack.getRight(), filledTrack.getBottom(), false);
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
    ColourGradient thumbGradient(Colour(0xff505050), thumbX, thumbY,
                                  Colour(0xff303030), thumbX, thumbY + thumbSize, false);
    g.setGradientFill(thumbGradient);
    g.fillEllipse(thumbX, thumbY, thumbSize, thumbSize);

    // Thumb highlight
    g.setColour(Colour(0xff606060));
    g.drawEllipse(thumbX, thumbY, thumbSize, thumbSize, 1.0f);
}

void NAMLookAndFeel::drawToggleButton(Graphics& g, ToggleButton& button,
                                       bool shouldDrawButtonAsHighlighted,
                                       bool shouldDrawButtonAsDown)
{
    const int width = button.getWidth();
    const int height = button.getHeight();
    const float ledSize = 10.0f;
    const float ledX = 4.0f;
    const float ledY = (height - ledSize) * 0.5f;

    // LED indicator
    Colour ledColour = button.getToggleState() ? Colour(0xff00ff66) : Colour(0xff303030);

    // LED glow when on
    if (button.getToggleState())
    {
        g.setColour(ledColour.withAlpha(0.3f));
        g.fillEllipse(ledX - 2, ledY - 2, ledSize + 4, ledSize + 4);
    }

    // LED body
    ColourGradient ledGradient(ledColour.brighter(0.2f), ledX, ledY,
                                ledColour.darker(0.2f), ledX, ledY + ledSize, false);
    g.setGradientFill(ledGradient);
    g.fillEllipse(ledX, ledY, ledSize, ledSize);

    // LED rim
    g.setColour(Colour(0xff202020));
    g.drawEllipse(ledX, ledY, ledSize, ledSize, 1.0f);

    // Text
    g.setColour(button.getToggleState() ? Colour(0xffe0e0e0) : Colour(0xff808080));
    g.setFont(12.0f);
    g.drawText(button.getButtonText(),
               Rectangle<int>(ledX + ledSize + 4, 0, width - (int)(ledX + ledSize + 4), height),
               Justification::centredLeft);
}

void NAMLookAndFeel::drawButtonBackground(Graphics& g, Button& button,
                                           const Colour& backgroundColour,
                                           bool shouldDrawButtonAsHighlighted,
                                           bool shouldDrawButtonAsDown)
{
    auto bounds = button.getLocalBounds().toFloat().reduced(0.5f);

    Colour baseColour = shouldDrawButtonAsDown ? Colour(0xff252525)
                      : shouldDrawButtonAsHighlighted ? Colour(0xff454545)
                      : Colour(0xff353535);

    // Button shadow
    g.setColour(Colours::black.withAlpha(0.3f));
    g.fillRoundedRectangle(bounds.translated(0, 1), 4.0f);

    // Button body gradient
    ColourGradient buttonGradient(baseColour.brighter(0.1f), bounds.getX(), bounds.getY(),
                                   baseColour.darker(0.1f), bounds.getX(), bounds.getBottom(), false);
    g.setGradientFill(buttonGradient);
    g.fillRoundedRectangle(bounds, 4.0f);

    // Border
    g.setColour(Colour(0xff505050));
    g.drawRoundedRectangle(bounds, 4.0f, 1.0f);
}

//==============================================================================
// NAMControl Implementation
//==============================================================================
NAMControl::NAMControl(NAMProcessor* processor) : namProcessor(processor)
{
    setLookAndFeel(&namLookAndFeel);

    // Model loading section
    loadModelButton = std::make_unique<TextButton>("Load Model");
    loadModelButton->addListener(this);
    addAndMakeVisible(loadModelButton.get());

    browseModelsButton = std::make_unique<TextButton>("Browse...");
    browseModelsButton->setTooltip("Browse NAM Models");
    browseModelsButton->addListener(this);
    addAndMakeVisible(browseModelsButton.get());

    clearModelButton = std::make_unique<TextButton>("X");
    clearModelButton->setTooltip("Clear Model");
    clearModelButton->addListener(this);
    addAndMakeVisible(clearModelButton.get());

    modelNameLabel = std::make_unique<Label>("modelName", "No Model Loaded");
    modelNameLabel->setJustificationType(Justification::centredLeft);
    modelNameLabel->setColour(Label::textColourId, Colour(kTextBright));
    modelNameLabel->setColour(Label::backgroundColourId, Colour(0xff151515));
    modelNameLabel->setColour(Label::outlineColourId, Colour(0xff080808));
    addAndMakeVisible(modelNameLabel.get());

    // IR loading section
    loadIRButton = std::make_unique<TextButton>("Load IR");
    loadIRButton->addListener(this);
    addAndMakeVisible(loadIRButton.get());

    clearIRButton = std::make_unique<TextButton>("X");
    clearIRButton->setTooltip("Clear IR");
    clearIRButton->addListener(this);
    addAndMakeVisible(clearIRButton.get());

    irNameLabel = std::make_unique<Label>("irName", "No IR Loaded");
    irNameLabel->setJustificationType(Justification::centredLeft);
    irNameLabel->setColour(Label::textColourId, Colour(kTextBright));
    irNameLabel->setColour(Label::backgroundColourId, Colour(0xff151515));
    irNameLabel->setColour(Label::outlineColourId, Colour(0xff080808));
    addAndMakeVisible(irNameLabel.get());

    irEnabledButton = std::make_unique<ToggleButton>("IR");
    irEnabledButton->setToggleState(namProcessor->isIREnabled(), dontSendNotification);
    irEnabledButton->addListener(this);
    addAndMakeVisible(irEnabledButton.get());

    // Effects loop controls
    fxLoopEnabledButton = std::make_unique<ToggleButton>("FX Loop");
    fxLoopEnabledButton->setToggleState(namProcessor->isEffectsLoopEnabled(), dontSendNotification);
    fxLoopEnabledButton->addListener(this);
    addAndMakeVisible(fxLoopEnabledButton.get());

    editFxLoopButton = std::make_unique<TextButton>("Edit FX...");
    editFxLoopButton->setTooltip("Edit Effects Loop");
    editFxLoopButton->addListener(this);
    addAndMakeVisible(editFxLoopButton.get());

    // Input gain slider
    inputGainSlider = std::make_unique<Slider>(Slider::LinearHorizontal, Slider::TextBoxRight);
    inputGainSlider->setRange(-20.0, 20.0, 0.1);
    inputGainSlider->setValue(namProcessor->getInputGain());
    inputGainSlider->addListener(this);
    inputGainSlider->setTextValueSuffix(" dB");
    inputGainSlider->setTextBoxStyle(Slider::TextBoxRight, false, 55, 18);
    addAndMakeVisible(inputGainSlider.get());

    inputGainLabel = std::make_unique<Label>("inputLabel", "INPUT");
    inputGainLabel->setJustificationType(Justification::centredRight);
    inputGainLabel->setColour(Label::textColourId, Colour(kTextDim));
    inputGainLabel->setFont(Font(11.0f, Font::bold));
    addAndMakeVisible(inputGainLabel.get());

    // Output gain slider
    outputGainSlider = std::make_unique<Slider>(Slider::LinearHorizontal, Slider::TextBoxRight);
    outputGainSlider->setRange(-40.0, 40.0, 0.1);
    outputGainSlider->setValue(namProcessor->getOutputGain());
    outputGainSlider->addListener(this);
    outputGainSlider->setTextValueSuffix(" dB");
    outputGainSlider->setTextBoxStyle(Slider::TextBoxRight, false, 55, 18);
    addAndMakeVisible(outputGainSlider.get());

    outputGainLabel = std::make_unique<Label>("outputLabel", "OUTPUT");
    outputGainLabel->setJustificationType(Justification::centredRight);
    outputGainLabel->setColour(Label::textColourId, Colour(kTextDim));
    outputGainLabel->setFont(Font(11.0f, Font::bold));
    addAndMakeVisible(outputGainLabel.get());

    // Noise gate slider
    noiseGateSlider = std::make_unique<Slider>(Slider::LinearHorizontal, Slider::TextBoxRight);
    noiseGateSlider->setRange(-101.0, 0.0, 1.0);
    noiseGateSlider->setValue(namProcessor->getNoiseGateThreshold());
    noiseGateSlider->addListener(this);
    noiseGateSlider->setTextValueSuffix(" dB");
    noiseGateSlider->setTextBoxStyle(Slider::TextBoxRight, false, 55, 18);
    addAndMakeVisible(noiseGateSlider.get());

    noiseGateLabel = std::make_unique<Label>("gateLabel", "GATE");
    noiseGateLabel->setJustificationType(Justification::centredRight);
    noiseGateLabel->setColour(Label::textColourId, Colour(kTextDim));
    noiseGateLabel->setFont(Font(11.0f, Font::bold));
    addAndMakeVisible(noiseGateLabel.get());

    // Tone stack
    toneStackEnabledButton = std::make_unique<ToggleButton>("EQ");
    toneStackEnabledButton->setToggleState(namProcessor->isToneStackEnabled(), dontSendNotification);
    toneStackEnabledButton->addListener(this);
    addAndMakeVisible(toneStackEnabledButton.get());

    // Bass knob
    bassSlider = std::make_unique<Slider>(Slider::RotaryHorizontalVerticalDrag, Slider::NoTextBox);
    bassSlider->setRange(0.0, 10.0, 0.1);
    bassSlider->setValue(namProcessor->getBass());
    bassSlider->addListener(this);
    bassSlider->setRotaryParameters(MathConstants<float>::pi * 1.25f,
                                     MathConstants<float>::pi * 2.75f, true);
    addAndMakeVisible(bassSlider.get());

    bassLabel = std::make_unique<Label>("bassLabel", "BASS");
    bassLabel->setJustificationType(Justification::centred);
    bassLabel->setColour(Label::textColourId, Colour(kTextDim));
    bassLabel->setFont(Font(10.0f, Font::bold));
    addAndMakeVisible(bassLabel.get());

    // Mid knob
    midSlider = std::make_unique<Slider>(Slider::RotaryHorizontalVerticalDrag, Slider::NoTextBox);
    midSlider->setRange(0.0, 10.0, 0.1);
    midSlider->setValue(namProcessor->getMid());
    midSlider->addListener(this);
    midSlider->setRotaryParameters(MathConstants<float>::pi * 1.25f,
                                    MathConstants<float>::pi * 2.75f, true);
    addAndMakeVisible(midSlider.get());

    midLabel = std::make_unique<Label>("midLabel", "MID");
    midLabel->setJustificationType(Justification::centred);
    midLabel->setColour(Label::textColourId, Colour(kTextDim));
    midLabel->setFont(Font(10.0f, Font::bold));
    addAndMakeVisible(midLabel.get());

    // Treble knob
    trebleSlider = std::make_unique<Slider>(Slider::RotaryHorizontalVerticalDrag, Slider::NoTextBox);
    trebleSlider->setRange(0.0, 10.0, 0.1);
    trebleSlider->setValue(namProcessor->getTreble());
    trebleSlider->addListener(this);
    trebleSlider->setRotaryParameters(MathConstants<float>::pi * 1.25f,
                                       MathConstants<float>::pi * 2.75f, true);
    addAndMakeVisible(trebleSlider.get());

    trebleLabel = std::make_unique<Label>("trebleLabel", "TREBLE");
    trebleLabel->setJustificationType(Justification::centred);
    trebleLabel->setColour(Label::textColourId, Colour(kTextDim));
    trebleLabel->setFont(Font(10.0f, Font::bold));
    addAndMakeVisible(trebleLabel.get());

    // Normalize button
    normalizeButton = std::make_unique<ToggleButton>("Normalize");
    normalizeButton->setToggleState(namProcessor->isNormalizeOutput(), dontSendNotification);
    normalizeButton->addListener(this);
    addAndMakeVisible(normalizeButton.get());

    // Update displays
    updateModelDisplay();
    updateIRDisplay();
}

NAMControl::~NAMControl()
{
    setLookAndFeel(nullptr);
}

//==============================================================================
void NAMControl::paint(Graphics& g)
{
    // Main background with subtle gradient
    ColourGradient bgGradient(Colour(kBackgroundMid), 0, 0,
                               Colour(kBackgroundDark), 0, (float)getHeight(), false);
    g.setGradientFill(bgGradient);
    g.fillAll();

    // Outer border with bevel effect
    g.setColour(Colour(0xff101010));
    g.drawRect(getLocalBounds(), 1);
    g.setColour(Colour(0xff404040));
    g.drawRect(getLocalBounds().reduced(1), 1);

    // Header bar
    Rectangle<int> headerBounds(0, 0, getWidth(), 32);
    ColourGradient headerGradient(Colour(kHeaderAccent), 0, 0,
                                   Colour(kBackgroundDark), 0, 32, false);
    g.setGradientFill(headerGradient);
    g.fillRect(headerBounds);

    // Header bottom line
    g.setColour(Colour(0xff101010));
    g.drawHorizontalLine(31, 0, (float)getWidth());
    g.setColour(Colour(0xff505050));
    g.drawHorizontalLine(32, 0, (float)getWidth());

    // Title text
    g.setColour(Colour(kTextBright));
    g.setFont(Font(15.0f, Font::bold));
    g.drawText("NAM LOADER", headerBounds.reduced(12, 0), Justification::centredLeft);

    // Status LED (model loaded indicator)
    const float ledSize = 12.0f;
    const float ledX = getWidth() - 24.0f;
    const float ledY = (32 - ledSize) * 0.5f;

    Colour ledColour = namProcessor->isModelLoaded() ? Colour(kLedOn) : Colour(kLedOff);

    // LED glow
    if (namProcessor->isModelLoaded())
    {
        g.setColour(ledColour.withAlpha(0.4f));
        g.fillEllipse(ledX - 3, ledY - 3, ledSize + 6, ledSize + 6);
    }

    // LED body
    ColourGradient ledGradient(ledColour.brighter(0.3f), ledX, ledY,
                                ledColour.darker(0.2f), ledX, ledY + ledSize, false);
    g.setGradientFill(ledGradient);
    g.fillEllipse(ledX, ledY, ledSize, ledSize);

    // LED rim
    g.setColour(Colour(0xff101010));
    g.drawEllipse(ledX, ledY, ledSize, ledSize, 1.5f);

    // Section dividers
    g.setColour(Colour(0xff101010));
    g.drawHorizontalLine(95, 8, getWidth() - 8.0f);
    g.setColour(Colour(0xff404040));
    g.drawHorizontalLine(96, 8, getWidth() - 8.0f);

    g.setColour(Colour(0xff101010));
    g.drawHorizontalLine(175, 8, getWidth() - 8.0f);
    g.setColour(Colour(0xff404040));
    g.drawHorizontalLine(176, 8, getWidth() - 8.0f);
}

void NAMControl::resized()
{
    auto bounds = getLocalBounds();
    bounds.removeFromTop(36); // Header space
    bounds = bounds.reduced(10, 4);

    const int rowHeight = 24;
    const int labelWidth = 50;
    const int buttonWidth = 75;
    const int clearButtonWidth = 24;
    const int spacing = 6;

    // MODEL section
    auto modelRow = bounds.removeFromTop(rowHeight);
    loadModelButton->setBounds(modelRow.removeFromLeft(buttonWidth));
    modelRow.removeFromLeft(spacing);
    browseModelsButton->setBounds(modelRow.removeFromLeft(60));
    modelRow.removeFromLeft(spacing);
    clearModelButton->setBounds(modelRow.removeFromLeft(clearButtonWidth));
    modelRow.removeFromLeft(spacing);
    modelNameLabel->setBounds(modelRow);

    bounds.removeFromTop(spacing);

    // IR section
    auto irRow = bounds.removeFromTop(rowHeight);
    loadIRButton->setBounds(irRow.removeFromLeft(buttonWidth));
    irRow.removeFromLeft(spacing);
    clearIRButton->setBounds(irRow.removeFromLeft(clearButtonWidth));
    irRow.removeFromLeft(spacing);
    irEnabledButton->setBounds(irRow.removeFromRight(45));
    irRow.removeFromRight(spacing);
    irNameLabel->setBounds(irRow);

    bounds.removeFromTop(spacing);

    // FX Loop section
    auto fxRow = bounds.removeFromTop(rowHeight);
    fxLoopEnabledButton->setBounds(fxRow.removeFromLeft(70));
    fxRow.removeFromLeft(spacing);
    editFxLoopButton->setBounds(fxRow.removeFromLeft(70));

    bounds.removeFromTop(12); // Section spacing

    // GAIN section
    auto inputRow = bounds.removeFromTop(rowHeight);
    inputGainLabel->setBounds(inputRow.removeFromLeft(labelWidth));
    inputRow.removeFromLeft(spacing);
    inputGainSlider->setBounds(inputRow);

    bounds.removeFromTop(spacing);

    auto outputRow = bounds.removeFromTop(rowHeight);
    outputGainLabel->setBounds(outputRow.removeFromLeft(labelWidth));
    outputRow.removeFromLeft(spacing);
    outputGainSlider->setBounds(outputRow);

    bounds.removeFromTop(spacing);

    auto gateRow = bounds.removeFromTop(rowHeight);
    noiseGateLabel->setBounds(gateRow.removeFromLeft(labelWidth));
    gateRow.removeFromLeft(spacing);
    noiseGateSlider->setBounds(gateRow);

    bounds.removeFromTop(12); // Section spacing

    // EQ KNOBS section
    auto eqHeaderRow = bounds.removeFromTop(20);
    toneStackEnabledButton->setBounds(eqHeaderRow.removeFromLeft(50));
    eqHeaderRow.removeFromLeft(spacing * 4);
    normalizeButton->setBounds(eqHeaderRow.removeFromLeft(90));

    bounds.removeFromTop(4);

    // Knobs row
    auto knobRow = bounds.removeFromTop(70);
    const int knobWidth = knobRow.getWidth() / 3;
    const int knobSize = 54;

    auto bassArea = knobRow.removeFromLeft(knobWidth);
    bassSlider->setBounds(bassArea.withSizeKeepingCentre(knobSize, knobSize).translated(0, -6));
    bassLabel->setBounds(bassArea.removeFromBottom(16));

    auto midArea = knobRow.removeFromLeft(knobWidth);
    midSlider->setBounds(midArea.withSizeKeepingCentre(knobSize, knobSize).translated(0, -6));
    midLabel->setBounds(midArea.removeFromBottom(16));

    auto trebleArea = knobRow;
    trebleSlider->setBounds(trebleArea.withSizeKeepingCentre(knobSize, knobSize).translated(0, -6));
    trebleLabel->setBounds(trebleArea.removeFromBottom(16));
}

//==============================================================================
void NAMControl::buttonClicked(Button* button)
{
    if (button == loadModelButton.get())
    {
        modelFileChooser = std::make_unique<FileChooser>("Select NAM Model",
                                                          File::getSpecialLocation(File::userDocumentsDirectory),
                                                          "*.nam", true);

        auto chooserFlags = FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles;

        modelFileChooser->launchAsync(chooserFlags,
                                       [this](const FileChooser& fc)
                                       {
                                           auto result = fc.getResult();
                                           if (result.existsAsFile())
                                           {
                                               if (namProcessor->loadModel(result))
                                               {
                                                   updateModelDisplay();
                                                   repaint();
                                               }
                                           }
                                       });
    }
    else if (button == browseModelsButton.get())
    {
        NAMModelBrowser::showWindow(namProcessor, [this]() {
            updateModelDisplay();
            repaint();
        });
    }
    else if (button == clearModelButton.get())
    {
        namProcessor->clearModel();
        updateModelDisplay();
        repaint();
    }
    else if (button == loadIRButton.get())
    {
        irFileChooser = std::make_unique<FileChooser>("Select Impulse Response",
                                                       File::getSpecialLocation(File::userDocumentsDirectory),
                                                       "*.wav;*.aiff;*.aif", true);

        auto chooserFlags = FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles;

        irFileChooser->launchAsync(chooserFlags,
                                    [this](const FileChooser& fc)
                                    {
                                        auto result = fc.getResult();
                                        if (result.existsAsFile())
                                        {
                                            if (namProcessor->loadIR(result))
                                            {
                                                updateIRDisplay();
                                                repaint();
                                            }
                                        }
                                    });
    }
    else if (button == clearIRButton.get())
    {
        namProcessor->clearIR();
        updateIRDisplay();
        repaint();
    }
    else if (button == irEnabledButton.get())
    {
        namProcessor->setIREnabled(irEnabledButton->getToggleState());
    }
    else if (button == fxLoopEnabledButton.get())
    {
        namProcessor->setEffectsLoopEnabled(fxLoopEnabledButton->getToggleState());
    }
    else if (button == editFxLoopButton.get())
    {
        // Open the effects loop editor
        if (auto* fxLoop = namProcessor->getEffectsLoop())
        {
            if (auto* editor = fxLoop->createEditor())
            {
                // Create self-deleting window
                class FXLoopWindow : public DocumentWindow
                {
                public:
                    FXLoopWindow(const String& name, Colour bg)
                        : DocumentWindow(name, bg, DocumentWindow::closeButton) {}
                    void closeButtonPressed() override { delete this; }
                };

                auto* window = new FXLoopWindow("FX Loop - " + namProcessor->getModelName(),
                                                 Colour(0xff252525));
                window->setContentOwned(editor, true);
                window->setResizable(true, false);
                window->setUsingNativeTitleBar(true);
                window->centreWithSize(editor->getWidth(), editor->getHeight());
                window->setVisible(true);
            }
        }
    }
    else if (button == toneStackEnabledButton.get())
    {
        namProcessor->setToneStackEnabled(toneStackEnabledButton->getToggleState());
    }
    else if (button == normalizeButton.get())
    {
        namProcessor->setNormalizeOutput(normalizeButton->getToggleState());
    }
}

void NAMControl::sliderValueChanged(Slider* slider)
{
    if (slider == inputGainSlider.get())
    {
        namProcessor->setInputGain(static_cast<float>(slider->getValue()));
    }
    else if (slider == outputGainSlider.get())
    {
        namProcessor->setOutputGain(static_cast<float>(slider->getValue()));
    }
    else if (slider == noiseGateSlider.get())
    {
        namProcessor->setNoiseGateThreshold(static_cast<float>(slider->getValue()));
    }
    else if (slider == bassSlider.get())
    {
        namProcessor->setBass(static_cast<float>(slider->getValue()));
    }
    else if (slider == midSlider.get())
    {
        namProcessor->setMid(static_cast<float>(slider->getValue()));
    }
    else if (slider == trebleSlider.get())
    {
        namProcessor->setTreble(static_cast<float>(slider->getValue()));
    }
}

//==============================================================================
void NAMControl::updateModelDisplay()
{
    if (namProcessor->isModelLoaded())
    {
        modelNameLabel->setText(namProcessor->getModelName(), dontSendNotification);
        modelNameLabel->setColour(Label::textColourId, Colour(kTextBright));
    }
    else
    {
        modelNameLabel->setText("No Model Loaded", dontSendNotification);
        modelNameLabel->setColour(Label::textColourId, Colour(kTextDim));
    }
}

void NAMControl::updateIRDisplay()
{
    if (namProcessor->isIRLoaded())
    {
        irNameLabel->setText(namProcessor->getIRName(), dontSendNotification);
        irNameLabel->setColour(Label::textColourId, Colour(kTextBright));
    }
    else
    {
        irNameLabel->setText("No IR Loaded", dontSendNotification);
        irNameLabel->setColour(Label::textColourId, Colour(kTextDim));
    }
}

void NAMControl::drawSectionPanel(Graphics& g, const Rectangle<int>& bounds,
                                   const String& title, Colour headerColour)
{
    // Panel background
    g.setColour(Colour(kPanelBackground));
    g.fillRoundedRectangle(bounds.toFloat(), 4.0f);

    // Panel border
    g.setColour(Colour(0xff101010));
    g.drawRoundedRectangle(bounds.toFloat(), 4.0f, 1.0f);

    // Section title
    if (title.isNotEmpty())
    {
        auto titleBounds = bounds.withHeight(18);
        g.setColour(headerColour);
        g.setFont(Font(10.0f, Font::bold));
        g.drawText(title, titleBounds.reduced(6, 0), Justification::centredLeft);
    }
}
