/*
  ==============================================================================

    NAMControl.cpp
    UI control for the NAM (Neural Amp Modeler) processor
    Professional amp-style interface with theme-complementary colours

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
    refreshColours();
}

void NAMLookAndFeel::refreshColours()
{
    auto& cs = ::ColourScheme::getInstance();

    // Derive amp palette from theme tokens
    Colour pluginBg = cs.colours["Plugin Background"];
    Colour windowBg = cs.colours["Window Background"];
    Colour textCol = cs.colours["Text Colour"];
    Colour sliderCol = cs.colours["Slider Colour"];
    Colour warnCol = cs.colours["Warning Colour"];
    Colour buttonCol = cs.colours["Button Colour"];
    Colour buttonHi = cs.colours["Button Highlight"];
    Colour fieldBg = cs.colours["Field Background"];
    Colour successCol = cs.colours["Success Colour"];

    // Build the amp palette: darker and warmer than the base theme
    ampBackground = pluginBg.darker(0.6f);
    ampSurface = pluginBg.darker(0.25f);
    ampBorder = pluginBg.darker(0.85f).interpolatedWith(Colour(0xff606060), 0.15f);
    ampHeaderBg = pluginBg.darker(0.5f);
    ampAccent = warnCol;            // Warm orange/amber
    ampAccentSecondary = sliderCol; // Theme slider colour
    ampTextBright = textCol;
    ampTextDim = textCol.withAlpha(0.6f);
    ampLedOn = successCol.brighter(0.4f);
    ampLedOff = pluginBg.darker(0.5f);
    ampKnobBody = pluginBg.darker(0.15f);
    ampKnobRing = pluginBg.interpolatedWith(Colour(0xffa0a0a0), 0.35f);
    ampTrackBg = ampBackground.darker(0.4f);
    ampButtonBg = buttonCol.darker(0.35f);
    ampButtonHover = buttonHi.darker(0.15f);
    ampInsetBg = ampBackground.darker(0.5f);

    // Apply to JUCE colour IDs
    setColour(Slider::backgroundColourId, ampTrackBg);
    setColour(Slider::trackColourId, ampAccentSecondary); // Theme slider colour
    setColour(Slider::thumbColourId, ampTextBright);
    setColour(Slider::textBoxTextColourId, ampTextBright);
    setColour(Slider::textBoxBackgroundColourId, ampInsetBg);
    setColour(Slider::textBoxOutlineColourId, ampBorder);
    setColour(TextButton::buttonColourId, ampButtonBg);
    setColour(TextButton::textColourOffId, ampTextBright);
    setColour(TextButton::textColourOnId, ampAccent);
    setColour(ToggleButton::textColourId, ampTextBright);
    setColour(ToggleButton::tickColourId, ampAccent);
    setColour(Label::textColourId, ampTextBright);
    setColour(Label::backgroundColourId, Colours::transparentBlack);
}

void NAMLookAndFeel::drawRotarySlider(Graphics& g, int x, int y, int width, int height, float sliderPos,
                                      float rotaryStartAngle, float rotaryEndAngle, Slider& slider)
{
    const float radius = jmin(width / 2.0f, height / 2.0f) - 8.0f;
    const float centreX = x + width * 0.5f;
    const float centreY = y + height * 0.5f;
    const float rx = centreX - radius;
    const float ry = centreY - radius;
    const float rw = radius * 2.0f;
    const float angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

    // Drop shadow
    g.setColour(Colours::black.withAlpha(0.35f));
    g.fillEllipse(rx - 1, ry + 2, rw + 2, rw + 2);

    // Value arc background (full range, dimmed)
    Path bgArc;
    bgArc.addCentredArc(centreX, centreY, radius + 6.0f, radius + 6.0f, 0.0f, rotaryStartAngle, rotaryEndAngle, true);
    g.setColour(ampTrackBg.brighter(0.05f));
    g.strokePath(bgArc, PathStrokeType(3.5f, PathStrokeType::curved, PathStrokeType::rounded));

    // Value arc (filled segment showing current position)
    Path valueArc;
    valueArc.addCentredArc(centreX, centreY, radius + 6.0f, radius + 6.0f, 0.0f, rotaryStartAngle, angle, true);
    g.setColour(ampAccent);
    g.strokePath(valueArc, PathStrokeType(3.5f, PathStrokeType::curved, PathStrokeType::rounded));

    // Value arc glow
    g.setColour(ampAccent.withAlpha(0.2f));
    g.strokePath(valueArc, PathStrokeType(8.0f, PathStrokeType::curved, PathStrokeType::rounded));

    // Outer metallic ring
    ColourGradient outerGradient(ampKnobRing.brighter(0.25f), centreX, centreY - radius, ampKnobRing.darker(0.15f),
                                 centreX, centreY + radius, false);
    g.setGradientFill(outerGradient);
    g.fillEllipse(rx - 2.5f, ry - 2.5f, rw + 5, rw + 5);

    // Outer ring border
    g.setColour(ampBorder.darker(0.3f));
    g.drawEllipse(rx - 2.5f, ry - 2.5f, rw + 5, rw + 5, 0.75f);

    // Main knob body
    ColourGradient knobGradient(ampKnobBody.brighter(0.15f), centreX, centreY - radius, ampKnobBody.darker(0.35f),
                                centreX, centreY + radius, false);
    g.setGradientFill(knobGradient);
    g.fillEllipse(rx, ry, rw, rw);

    // Inner recess (concave look)
    const float innerRadius = radius * 0.6f;
    ColourGradient innerGradient(ampKnobBody.darker(0.25f), centreX, centreY - innerRadius, ampKnobBody.darker(0.55f),
                                 centreX, centreY + innerRadius, false);
    g.setGradientFill(innerGradient);
    g.fillEllipse(centreX - innerRadius, centreY - innerRadius, innerRadius * 2.0f, innerRadius * 2.0f);

    // Inner recess rim
    g.setColour(Colours::black.withAlpha(0.15f));
    g.drawEllipse(centreX - innerRadius, centreY - innerRadius, innerRadius * 2.0f, innerRadius * 2.0f, 0.5f);

    // Pointer indicator
    Path p;
    const float pointerLength = radius * 0.5f;
    const float pointerThickness = 3.5f;
    p.addRoundedRectangle(-pointerThickness * 0.5f, -pointerLength, pointerThickness, pointerLength, 1.5f);
    p.applyTransform(AffineTransform::rotation(angle).translated(centreX, centreY));

    // Pointer glow
    g.setColour(ampAccent.withAlpha(0.3f));
    Path pGlow;
    pGlow.addRoundedRectangle(-pointerThickness, -pointerLength - 1, pointerThickness * 2, pointerLength + 1, 2.0f);
    pGlow.applyTransform(AffineTransform::rotation(angle).translated(centreX, centreY));
    g.fillPath(pGlow);

    g.setColour(ampAccent);
    g.fillPath(p);

    // Tick marks
    g.setColour(ampTextDim.withAlpha(0.35f));
    const int numTicks = 11;
    for (int i = 0; i < numTicks; ++i)
    {
        const float tickAngle = rotaryStartAngle + (float)i / (numTicks - 1) * (rotaryEndAngle - rotaryStartAngle);
        const float tickInnerRadius = radius + 11.0f;
        const float tickOuterRadius = radius + 15.0f;

        Point<float> innerPoint(centreX + tickInnerRadius * std::sin(tickAngle),
                                centreY - tickInnerRadius * std::cos(tickAngle));
        Point<float> outerPoint(centreX + tickOuterRadius * std::sin(tickAngle),
                                centreY - tickOuterRadius * std::cos(tickAngle));

        g.drawLine(innerPoint.x, innerPoint.y, outerPoint.x, outerPoint.y, 1.0f);
    }
}

void NAMLookAndFeel::drawLinearSlider(Graphics& g, int x, int y, int width, int height, float sliderPos,
                                      float minSliderPos, float maxSliderPos, const Slider::SliderStyle style,
                                      Slider& slider)
{
    const bool isHorizontal = (style == Slider::LinearHorizontal || style == Slider::LinearBar);
    const float trackThickness = 6.0f;

    Rectangle<float> track;
    if (isHorizontal)
    {
        track = Rectangle<float>(x, y + (height - trackThickness) * 0.5f, width, trackThickness);
    }
    else
    {
        track = Rectangle<float>(x + (width - trackThickness) * 0.5f, y, trackThickness, height);
    }

    // Track background (inset with subtle inner shadow)
    g.setColour(ampTrackBg);
    g.fillRoundedRectangle(track, 3.0f);
    // Inner shadow on track
    ColourGradient trackShadow(Colours::black.withAlpha(0.15f), track.getX(), track.getY(), Colours::transparentBlack,
                               track.getX(), track.getY() + 3.0f, false);
    g.setGradientFill(trackShadow);
    g.fillRoundedRectangle(track, 3.0f);
    g.setColour(ampBorder.darker(0.2f));
    g.drawRoundedRectangle(track, 3.0f, 0.75f);

    // Filled portion with accent colour
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

    // Gradient fill using theme slider colour
    ColourGradient fillGradient(ampAccentSecondary, filledTrack.getX(), filledTrack.getY(),
                                ampAccentSecondary.darker(0.4f), filledTrack.getRight(), filledTrack.getBottom(),
                                false);
    g.setGradientFill(fillGradient);
    g.fillRoundedRectangle(filledTrack, 3.0f);

    // Filled track glow
    g.setColour(ampAccentSecondary.withAlpha(0.1f));
    g.fillRoundedRectangle(filledTrack.expanded(0, 2.0f), 3.0f);

    // Thumb
    const float thumbSize = 18.0f;
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

    // Thumb body with metallic gradient
    ColourGradient thumbGradient(ampKnobRing.brighter(0.2f), thumbX, thumbY, ampKnobRing.darker(0.15f), thumbX,
                                 thumbY + thumbSize, false);
    g.setGradientFill(thumbGradient);
    g.fillEllipse(thumbX, thumbY, thumbSize, thumbSize);

    // Thumb centre dot
    g.setColour(ampAccent.withAlpha(0.6f));
    const float dotSize = 4.0f;
    g.fillEllipse(thumbX + (thumbSize - dotSize) * 0.5f, thumbY + (thumbSize - dotSize) * 0.5f, dotSize, dotSize);

    // Thumb rim
    g.setColour(ampBorder.brighter(0.4f));
    g.drawEllipse(thumbX, thumbY, thumbSize, thumbSize, 1.0f);
}

void NAMLookAndFeel::drawToggleButton(Graphics& g, ToggleButton& button, bool shouldDrawButtonAsHighlighted,
                                      bool shouldDrawButtonAsDown)
{
    const int width = button.getWidth();
    const int height = button.getHeight();
    const float ledSize = 12.0f;
    const float ledX = 4.0f;
    const float ledY = (height - ledSize) * 0.5f;

    // LED glow when on
    Colour ledColour = button.getToggleState() ? ampLedOn : ampLedOff;

    if (button.getToggleState())
    {
        // Outer glow
        g.setColour(ledColour.withAlpha(0.2f));
        g.fillEllipse(ledX - 4, ledY - 4, ledSize + 8, ledSize + 8);
        g.setColour(ledColour.withAlpha(0.35f));
        g.fillEllipse(ledX - 2, ledY - 2, ledSize + 4, ledSize + 4);
    }

    // LED body
    ColourGradient ledGradient(ledColour.brighter(0.3f), ledX, ledY, ledColour.darker(0.2f), ledX, ledY + ledSize,
                               false);
    g.setGradientFill(ledGradient);
    g.fillEllipse(ledX, ledY, ledSize, ledSize);

    // LED specular highlight
    if (button.getToggleState())
    {
        g.setColour(Colours::white.withAlpha(0.2f));
        g.fillEllipse(ledX + 2, ledY + 1, ledSize * 0.4f, ledSize * 0.3f);
    }

    // LED rim
    g.setColour(ampBorder.darker(0.1f));
    g.drawEllipse(ledX, ledY, ledSize, ledSize, 1.0f);

    // Text
    auto& fm = FontManager::getInstance();
    g.setColour(button.getToggleState() ? ampTextBright : ampTextDim);
    g.setFont(fm.getUIFont(12.0f, true));
    g.drawText(button.getButtonText(),
               Rectangle<int>((int)(ledX + ledSize + 5), 0, width - (int)(ledX + ledSize + 5), height),
               Justification::centredLeft);
}

void NAMLookAndFeel::drawButtonBackground(Graphics& g, Button& button, const Colour& backgroundColour,
                                          bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
{
    auto bounds = button.getLocalBounds().toFloat().reduced(0.5f);

    Colour baseColour = shouldDrawButtonAsDown          ? ampButtonBg.darker(0.3f)
                        : shouldDrawButtonAsHighlighted ? ampButtonHover
                                                        : ampButtonBg;

    // Drop shadow
    g.setColour(Colours::black.withAlpha(0.25f));
    g.fillRoundedRectangle(bounds.translated(0, 1.5f), 4.0f);

    // Body gradient (more pronounced)
    ColourGradient buttonGradient(baseColour.brighter(0.15f), bounds.getX(), bounds.getY(), baseColour.darker(0.15f),
                                  bounds.getX(), bounds.getBottom(), false);
    g.setGradientFill(buttonGradient);
    g.fillRoundedRectangle(bounds, 4.0f);

    // Top highlight bevel
    g.setColour(Colours::white.withAlpha(0.06f));
    g.fillRoundedRectangle(bounds.removeFromTop(bounds.getHeight() * 0.45f), 4.0f);

    // Border -- accent on hover, otherwise subtle
    if (shouldDrawButtonAsHighlighted)
    {
        g.setColour(ampAccent.withAlpha(0.6f));
        g.drawRoundedRectangle(button.getLocalBounds().toFloat().reduced(0.5f), 4.0f, 1.5f);
    }
    else
    {
        g.setColour(ampBorder.brighter(0.25f));
        g.drawRoundedRectangle(button.getLocalBounds().toFloat().reduced(0.5f), 4.0f, 1.0f);
    }
}

Label* NAMLookAndFeel::createSliderTextBox(Slider& slider)
{
    auto* label = LookAndFeel_V4::createSliderTextBox(slider);
    auto& fm = FontManager::getInstance();
    label->setFont(fm.getMonoFont(12.0f));
    return label;
}

Font NAMLookAndFeel::getTextButtonFont(TextButton& /*button*/, int buttonHeight)
{
    auto& fm = FontManager::getInstance();
    return fm.getUIFont(jmin(13.0f, (float)buttonHeight * 0.55f));
}

//==============================================================================
// NAMControl Implementation
//==============================================================================
NAMControl::NAMControl(NAMProcessor* processor) : namProcessor(processor)
{
    setLookAndFeel(&namLookAndFeel);

    auto& fm = FontManager::getInstance();

    // Model loading section
    loadModelButton = std::make_unique<TextButton>("Load Model");
    loadModelButton->addListener(this);
    addAndMakeVisible(loadModelButton.get());

    browseModelsButton = std::make_unique<TextButton>("Browse...");
    browseModelsButton->setTooltip("Browse NAM Models Online");
    browseModelsButton->addListener(this);
    addAndMakeVisible(browseModelsButton.get());

    clearModelButton = std::make_unique<TextButton>("X");
    clearModelButton->setTooltip("Clear Model");
    clearModelButton->addListener(this);
    addAndMakeVisible(clearModelButton.get());

    modelNameLabel = std::make_unique<Label>("modelName", "No Model Loaded");
    modelNameLabel->setJustificationType(Justification::centredLeft);
    modelNameLabel->setFont(fm.getUIFont(13.0f));
    addAndMakeVisible(modelNameLabel.get());

    // Architecture badge
    modelArchLabel = std::make_unique<Label>("modelArch", "");
    modelArchLabel->setJustificationType(Justification::centred);
    modelArchLabel->setFont(fm.getUIFont(9.0f, true));
    addAndMakeVisible(modelArchLabel.get());

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
    irNameLabel->setFont(fm.getUIFont(13.0f));
    addAndMakeVisible(irNameLabel.get());

    irEnabledButton = std::make_unique<ToggleButton>("IR");
    irEnabledButton->setToggleState(namProcessor->isIREnabled(), dontSendNotification);
    irEnabledButton->addListener(this);
    addAndMakeVisible(irEnabledButton.get());

    // IR filters
    irLowCutSlider = std::make_unique<Slider>(Slider::LinearHorizontal, Slider::TextBoxRight);
    irLowCutSlider->setRange(20.0, 500.0, 1.0);
    irLowCutSlider->setValue(namProcessor->getIRLowCut());
    irLowCutSlider->setSkewFactorFromMidPoint(100.0);
    irLowCutSlider->addListener(this);
    irLowCutSlider->setTextValueSuffix(" Hz");
    irLowCutSlider->setTextBoxStyle(Slider::TextBoxRight, false, 60, 20);
    addAndMakeVisible(irLowCutSlider.get());

    irLowCutLabel = std::make_unique<Label>("lowCutLabel", "LO CUT");
    irLowCutLabel->setJustificationType(Justification::centredRight);
    irLowCutLabel->setFont(fm.getUIFont(10.0f, true));
    addAndMakeVisible(irLowCutLabel.get());

    irHighCutSlider = std::make_unique<Slider>(Slider::LinearHorizontal, Slider::TextBoxRight);
    irHighCutSlider->setRange(2000.0, 20000.0, 100.0);
    irHighCutSlider->setValue(namProcessor->getIRHighCut());
    irHighCutSlider->setSkewFactorFromMidPoint(8000.0);
    irHighCutSlider->addListener(this);
    irHighCutSlider->setTextValueSuffix(" Hz");
    irHighCutSlider->setTextBoxStyle(Slider::TextBoxRight, false, 70, 20);
    addAndMakeVisible(irHighCutSlider.get());

    irHighCutLabel = std::make_unique<Label>("highCutLabel", "HI CUT");
    irHighCutLabel->setJustificationType(Justification::centredRight);
    irHighCutLabel->setFont(fm.getUIFont(10.0f, true));
    addAndMakeVisible(irHighCutLabel.get());

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
    inputGainSlider->setTextBoxStyle(Slider::TextBoxRight, false, 65, 20);
    addAndMakeVisible(inputGainSlider.get());

    inputGainLabel = std::make_unique<Label>("inputLabel", "INPUT");
    inputGainLabel->setJustificationType(Justification::centredRight);
    inputGainLabel->setFont(fm.getUIFont(11.0f, true));
    addAndMakeVisible(inputGainLabel.get());

    // Output gain slider
    outputGainSlider = std::make_unique<Slider>(Slider::LinearHorizontal, Slider::TextBoxRight);
    outputGainSlider->setRange(-40.0, 40.0, 0.1);
    outputGainSlider->setValue(namProcessor->getOutputGain());
    outputGainSlider->addListener(this);
    outputGainSlider->setTextValueSuffix(" dB");
    outputGainSlider->setTextBoxStyle(Slider::TextBoxRight, false, 65, 20);
    addAndMakeVisible(outputGainSlider.get());

    outputGainLabel = std::make_unique<Label>("outputLabel", "OUTPUT");
    outputGainLabel->setJustificationType(Justification::centredRight);
    outputGainLabel->setFont(fm.getUIFont(11.0f, true));
    addAndMakeVisible(outputGainLabel.get());

    // Noise gate slider
    noiseGateSlider = std::make_unique<Slider>(Slider::LinearHorizontal, Slider::TextBoxRight);
    noiseGateSlider->setRange(-101.0, 0.0, 1.0);
    noiseGateSlider->setValue(namProcessor->getNoiseGateThreshold());
    noiseGateSlider->addListener(this);
    noiseGateSlider->setTextValueSuffix(" dB");
    noiseGateSlider->setTextBoxStyle(Slider::TextBoxRight, false, 65, 20);
    addAndMakeVisible(noiseGateSlider.get());

    noiseGateLabel = std::make_unique<Label>("gateLabel", "GATE");
    noiseGateLabel->setJustificationType(Justification::centredRight);
    noiseGateLabel->setFont(fm.getUIFont(11.0f, true));
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
    bassSlider->setRotaryParameters(MathConstants<float>::pi * 1.25f, MathConstants<float>::pi * 2.75f, true);
    addAndMakeVisible(bassSlider.get());

    bassLabel = std::make_unique<Label>("bassLabel", "BASS");
    bassLabel->setJustificationType(Justification::centred);
    bassLabel->setFont(fm.getUIFont(10.0f, true));
    addAndMakeVisible(bassLabel.get());

    // Mid knob
    midSlider = std::make_unique<Slider>(Slider::RotaryHorizontalVerticalDrag, Slider::NoTextBox);
    midSlider->setRange(0.0, 10.0, 0.1);
    midSlider->setValue(namProcessor->getMid());
    midSlider->addListener(this);
    midSlider->setRotaryParameters(MathConstants<float>::pi * 1.25f, MathConstants<float>::pi * 2.75f, true);
    addAndMakeVisible(midSlider.get());

    midLabel = std::make_unique<Label>("midLabel", "MID");
    midLabel->setJustificationType(Justification::centred);
    midLabel->setFont(fm.getUIFont(10.0f, true));
    addAndMakeVisible(midLabel.get());

    // Treble knob
    trebleSlider = std::make_unique<Slider>(Slider::RotaryHorizontalVerticalDrag, Slider::NoTextBox);
    trebleSlider->setRange(0.0, 10.0, 0.1);
    trebleSlider->setValue(namProcessor->getTreble());
    trebleSlider->addListener(this);
    trebleSlider->setRotaryParameters(MathConstants<float>::pi * 1.25f, MathConstants<float>::pi * 2.75f, true);
    addAndMakeVisible(trebleSlider.get());

    trebleLabel = std::make_unique<Label>("trebleLabel", "TREBLE");
    trebleLabel->setJustificationType(Justification::centred);
    trebleLabel->setFont(fm.getUIFont(10.0f, true));
    addAndMakeVisible(trebleLabel.get());

    // Normalize button
    normalizeButton = std::make_unique<ToggleButton>("Normalize");
    normalizeButton->setToggleState(namProcessor->isNormalizeOutput(), dontSendNotification);
    normalizeButton->addListener(this);
    addAndMakeVisible(normalizeButton.get());

    // Apply theme colours and update displays
    refreshColours();
    updateModelDisplay();
    updateIRDisplay();

    // Start LED pulse timer (30fps)
    startTimerHz(30);
}

NAMControl::~NAMControl()
{
    stopTimer();
    setLookAndFeel(nullptr);
}

void NAMControl::refreshColours()
{
    namLookAndFeel.refreshColours();
    auto& laf = namLookAndFeel;

    // Apply to model/IR display labels
    modelNameLabel->setColour(Label::backgroundColourId, laf.ampInsetBg);
    modelNameLabel->setColour(Label::outlineColourId, laf.ampBorder);
    irNameLabel->setColour(Label::backgroundColourId, laf.ampInsetBg);
    irNameLabel->setColour(Label::outlineColourId, laf.ampBorder);

    // Architecture badge
    modelArchLabel->setColour(Label::backgroundColourId, laf.ampAccent.withAlpha(0.15f));
    modelArchLabel->setColour(Label::textColourId, laf.ampAccent);

    // Dim labels
    irLowCutLabel->setColour(Label::textColourId, laf.ampTextDim);
    irHighCutLabel->setColour(Label::textColourId, laf.ampTextDim);
    inputGainLabel->setColour(Label::textColourId, laf.ampTextDim);
    outputGainLabel->setColour(Label::textColourId, laf.ampTextDim);
    noiseGateLabel->setColour(Label::textColourId, laf.ampTextDim);
    bassLabel->setColour(Label::textColourId, laf.ampTextDim);
    midLabel->setColour(Label::textColourId, laf.ampTextDim);
    trebleLabel->setColour(Label::textColourId, laf.ampTextDim);

    repaint();
}

void NAMControl::setCollapsed(bool shouldCollapse)
{
    if (collapsed == shouldCollapse)
        return;

    collapsed = shouldCollapse;
    namProcessor->setEditorCollapsed(collapsed);

    // Show/hide all child controls
    for (auto* child : getChildren())
        child->setVisible(!collapsed);

    // Resize self to match processor's reported size
    auto newSize = namProcessor->getSize();
    setSize(newSize.getX(), newSize.getY());

    // Notify parent PluginComponent to relayout
    if (auto* parent = getParentComponent())
    {
        parent->resized();
        if (auto* grandparent = parent->getParentComponent())
            grandparent->resized();
    }

    resized();
    repaint();
}

void NAMControl::timerCallback()
{
    if (namProcessor->isModelLoaded())
    {
        ledPulsePhase += 0.06f;
        if (ledPulsePhase > MathConstants<float>::twoPi)
            ledPulsePhase -= MathConstants<float>::twoPi;

        // Only repaint the header area for LED animation
        repaint(0, 0, getWidth(), 36);
    }
}

void NAMControl::mouseDown(const MouseEvent& event)
{
    // Click in header area toggles collapse
    if (event.y < 40)
        setCollapsed(!collapsed);
}

//==============================================================================
void NAMControl::paint(Graphics& g)
{
    auto& laf = namLookAndFeel;
    auto& fm = FontManager::getInstance();
    auto bounds = getLocalBounds();

    // Layout constants -- shared with resized()
    const int headerH = 34;
    const int panelMargin = 8;
    const int sectionGap = 6;
    const int signalH = 155;
    const int gainH = 115;

    // Main background gradient
    ColourGradient bgGradient(laf.ampSurface, 0, 0, laf.ampBackground, 0, (float)getHeight(), false);
    g.setGradientFill(bgGradient);
    g.fillRoundedRectangle(bounds.toFloat(), 4.0f);

    // Outer border (double-line bevel)
    g.setColour(laf.ampBorder.darker(0.3f));
    g.drawRoundedRectangle(bounds.toFloat().reduced(0.5f), 4.0f, 1.5f);
    g.setColour(laf.ampBorder.brighter(0.15f));
    g.drawRoundedRectangle(bounds.toFloat().reduced(2.0f), 3.0f, 0.5f);

    // Header bar -- shows current model name
    Rectangle<int> headerBounds(2, 2, getWidth() - 4, headerH);
    ColourGradient headerGradient(laf.ampHeaderBg.brighter(0.08f), 0, 2, laf.ampHeaderBg.darker(0.15f), 0,
                                  (float)headerH, false);
    g.setGradientFill(headerGradient);
    g.fillRoundedRectangle(headerBounds.toFloat(), 3.0f);

    // Header accent underline
    g.setColour(laf.ampAccent.withAlpha(0.7f));
    g.fillRect(2, headerH + 2, getWidth() - 4, 2);
    g.setColour(laf.ampBorder.darker(0.2f));
    g.fillRect(2, headerH + 4, getWidth() - 4, 1);

    // Model name in header
    String headerText = namProcessor->isModelLoaded() ? namProcessor->getModelName() : "No Model";
    g.setColour(namProcessor->isModelLoaded() ? laf.ampTextBright : laf.ampTextDim);
    g.setFont(fm.getUIFont(14.0f, true));
    g.drawText(headerText, headerBounds.reduced(12, 0).withTrimmedRight(30), Justification::centredLeft, true);

    // Status LED in header (right side)
    const float ledSize = 12.0f;
    const float ledX = (float)getWidth() - 22.0f;
    const float ledY = (headerH - ledSize) * 0.5f + 2;

    Colour ledColour = namProcessor->isModelLoaded() ? laf.ampLedOn : laf.ampLedOff;

    if (namProcessor->isModelLoaded())
    {
        float pulse = 0.2f + 0.12f * std::sin(ledPulsePhase);
        g.setColour(ledColour.withAlpha(pulse));
        g.fillEllipse(ledX - 4, ledY - 4, ledSize + 8, ledSize + 8);
    }

    ColourGradient ledGradient(ledColour.brighter(0.3f), ledX, ledY, ledColour.darker(0.2f), ledX, ledY + ledSize,
                               false);
    g.setGradientFill(ledGradient);
    g.fillEllipse(ledX, ledY, ledSize, ledSize);
    g.setColour(laf.ampBorder.darker(0.2f));
    g.drawEllipse(ledX, ledY, ledSize, ledSize, 1.0f);

    if (namProcessor->isModelLoaded())
    {
        g.setColour(Colours::white.withAlpha(0.2f));
        g.fillEllipse(ledX + 2, ledY + 1, ledSize * 0.35f, ledSize * 0.25f);
    }

    // Collapse chevron (right of LED)
    {
        const float chevX = (float)getWidth() - 40.0f;
        const float chevY = (headerH - 8.0f) * 0.5f + 2;
        Path chevron;
        if (collapsed)
        {
            chevron.addTriangle(chevX, chevY, chevX, chevY + 8.0f, chevX + 6.0f, chevY + 4.0f);
        }
        else
        {
            chevron.addTriangle(chevX, chevY, chevX + 8.0f, chevY, chevX + 4.0f, chevY + 6.0f);
        }
        g.setColour(laf.ampTextDim);
        g.fillPath(chevron);
    }

    // When collapsed, only draw header
    if (collapsed)
        return;

    // Section panels
    auto contentArea = bounds.reduced(panelMargin, 0);
    contentArea.removeFromTop(headerH + 7);

    auto signalBounds = contentArea.removeFromTop(signalH).reduced(0, 2);
    drawSectionPanel(g, signalBounds, "SIGNAL CHAIN");

    contentArea.removeFromTop(sectionGap);

    auto gainBounds = contentArea.removeFromTop(gainH).reduced(0, 2);
    drawSectionPanel(g, gainBounds, "GAIN");

    contentArea.removeFromTop(sectionGap);

    auto eqBounds = contentArea.reduced(0, 2);
    drawSectionPanel(g, eqBounds, "TONE");
}

void NAMControl::resized()
{
    if (collapsed)
        return;

    auto bounds = getLocalBounds();

    // Layout constants -- must match paint()
    const int headerH = 34;
    const int panelMargin = 8;
    const int sectionGap = 6;
    const int signalH = 155;
    const int gainH = 115;

    bounds.removeFromTop(headerH + 7); // header + accent + gap
    bounds = bounds.reduced(panelMargin, 0);

    const int rowHeight = 26;
    const int labelWidth = 60;
    const int buttonWidth = 80;
    const int clearButtonWidth = 26;
    const int spacing = 6;
    const int sectionHeaderH = 20;
    const int sectionPad = 8;

    // ===================== SIGNAL CHAIN section =====================
    auto signalArea = bounds.removeFromTop(signalH).reduced(sectionPad, 2);
    signalArea.removeFromTop(sectionHeaderH);

    // Model row
    auto modelRow = signalArea.removeFromTop(rowHeight);
    loadModelButton->setBounds(modelRow.removeFromLeft(buttonWidth));
    modelRow.removeFromLeft(spacing);
    browseModelsButton->setBounds(modelRow.removeFromLeft(64));
    modelRow.removeFromLeft(spacing);
    clearModelButton->setBounds(modelRow.removeFromLeft(clearButtonWidth));
    modelRow.removeFromLeft(spacing);

    if (modelArchLabel->getText().isNotEmpty())
    {
        modelArchLabel->setBounds(modelRow.removeFromRight(50));
        modelRow.removeFromRight(spacing);
    }
    else
    {
        modelArchLabel->setBounds(Rectangle<int>());
    }
    modelNameLabel->setBounds(modelRow);

    signalArea.removeFromTop(spacing);

    // IR row
    auto irRow = signalArea.removeFromTop(rowHeight);
    loadIRButton->setBounds(irRow.removeFromLeft(buttonWidth));
    irRow.removeFromLeft(spacing);
    clearIRButton->setBounds(irRow.removeFromLeft(clearButtonWidth));
    irRow.removeFromLeft(spacing);
    irEnabledButton->setBounds(irRow.removeFromRight(50));
    irRow.removeFromRight(spacing);
    irNameLabel->setBounds(irRow);

    signalArea.removeFromTop(spacing);

    // IR Filters row
    auto irFilterRow = signalArea.removeFromTop(rowHeight);
    const int halfWidth = (irFilterRow.getWidth() - spacing) / 2;

    auto lowCutArea = irFilterRow.removeFromLeft(halfWidth);
    irLowCutLabel->setBounds(lowCutArea.removeFromLeft(45));
    lowCutArea.removeFromLeft(2);
    irLowCutSlider->setBounds(lowCutArea);

    irFilterRow.removeFromLeft(spacing);

    auto highCutArea = irFilterRow;
    irHighCutLabel->setBounds(highCutArea.removeFromLeft(45));
    highCutArea.removeFromLeft(2);
    irHighCutSlider->setBounds(highCutArea);

    signalArea.removeFromTop(spacing);

    // FX Loop row
    auto fxRow = signalArea.removeFromTop(rowHeight);
    fxLoopEnabledButton->setBounds(fxRow.removeFromLeft(75));
    fxRow.removeFromLeft(spacing);
    editFxLoopButton->setBounds(fxRow.removeFromLeft(80));

    bounds.removeFromTop(sectionGap);

    // ===================== GAIN section =====================
    auto gainArea = bounds.removeFromTop(gainH).reduced(sectionPad, 2);
    gainArea.removeFromTop(sectionHeaderH);

    auto inputRow = gainArea.removeFromTop(rowHeight);
    inputGainLabel->setBounds(inputRow.removeFromLeft(labelWidth));
    inputRow.removeFromLeft(spacing);
    inputGainSlider->setBounds(inputRow);

    gainArea.removeFromTop(spacing);

    auto outputRow = gainArea.removeFromTop(rowHeight);
    outputGainLabel->setBounds(outputRow.removeFromLeft(labelWidth));
    outputRow.removeFromLeft(spacing);
    outputGainSlider->setBounds(outputRow);

    gainArea.removeFromTop(spacing);

    auto gateRow = gainArea.removeFromTop(rowHeight);
    noiseGateLabel->setBounds(gateRow.removeFromLeft(labelWidth));
    gateRow.removeFromLeft(spacing);
    noiseGateSlider->setBounds(gateRow);

    bounds.removeFromTop(sectionGap);

    // ===================== TONE section =====================
    auto eqArea = bounds.reduced(sectionPad, 2);
    eqArea.removeFromTop(sectionHeaderH);

    auto eqHeaderRow = eqArea.removeFromTop(24);
    toneStackEnabledButton->setBounds(eqHeaderRow.removeFromLeft(55));
    eqHeaderRow.removeFromLeft(spacing * 4);
    normalizeButton->setBounds(eqHeaderRow.removeFromLeft(100));

    eqArea.removeFromTop(6);

    // Knobs row -- use remaining space
    auto knobRow = eqArea;
    const int knobWidth = knobRow.getWidth() / 3;
    const int knobSize = 68;

    auto bassArea = knobRow.removeFromLeft(knobWidth);
    bassLabel->setBounds(bassArea.removeFromBottom(18));
    bassSlider->setBounds(bassArea.withSizeKeepingCentre(knobSize, knobSize));

    auto midArea = knobRow.removeFromLeft(knobWidth);
    midLabel->setBounds(midArea.removeFromBottom(18));
    midSlider->setBounds(midArea.withSizeKeepingCentre(knobSize, knobSize));

    auto trebleArea = knobRow;
    trebleLabel->setBounds(trebleArea.removeFromBottom(18));
    trebleSlider->setBounds(trebleArea.withSizeKeepingCentre(knobSize, knobSize));
}

//==============================================================================
void NAMControl::buttonClicked(Button* button)
{
    if (button == loadModelButton.get())
    {
        modelFileChooser = std::make_unique<FileChooser>(
            "Select NAM Model", File::getSpecialLocation(File::userDocumentsDirectory), "*.nam", true);

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
        NAMModelBrowser::showWindow(namProcessor,
                                    [this]()
                                    {
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
                    FXLoopWindow(const String& name, Colour bg) : DocumentWindow(name, bg, DocumentWindow::closeButton)
                    {
                    }
                    void closeButtonPressed() override { delete this; }
                };

                auto* window =
                    new FXLoopWindow("FX Loop - " + namProcessor->getModelName(), namLookAndFeel.ampBackground);
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
    else if (slider == irLowCutSlider.get())
    {
        namProcessor->setIRLowCut(static_cast<float>(slider->getValue()));
    }
    else if (slider == irHighCutSlider.get())
    {
        namProcessor->setIRHighCut(static_cast<float>(slider->getValue()));
    }
}

//==============================================================================
void NAMControl::updateModelDisplay()
{
    auto& laf = namLookAndFeel;

    if (namProcessor->isModelLoaded())
    {
        modelNameLabel->setText(namProcessor->getModelName(), dontSendNotification);
        modelNameLabel->setColour(Label::textColourId, laf.ampTextBright);

        // Show architecture badge
        modelArchLabel->setText("NAM", dontSendNotification);
    }
    else
    {
        modelNameLabel->setText("No Model Loaded", dontSendNotification);
        modelNameLabel->setColour(Label::textColourId, laf.ampTextDim);
        modelArchLabel->setText("", dontSendNotification);
    }

    // Relayout to show/hide architecture badge
    resized();
}

void NAMControl::updateIRDisplay()
{
    auto& laf = namLookAndFeel;

    if (namProcessor->isIRLoaded())
    {
        irNameLabel->setText(namProcessor->getIRName(), dontSendNotification);
        irNameLabel->setColour(Label::textColourId, laf.ampTextBright);
    }
    else
    {
        irNameLabel->setText("No IR Loaded", dontSendNotification);
        irNameLabel->setColour(Label::textColourId, laf.ampTextDim);
    }
}

void NAMControl::drawSectionPanel(Graphics& g, const Rectangle<int>& bounds, const String& title)
{
    auto& laf = namLookAndFeel;
    auto& fm = FontManager::getInstance();

    // Panel background (more visible contrast)
    g.setColour(laf.ampBackground.brighter(0.06f));
    g.fillRoundedRectangle(bounds.toFloat(), 5.0f);

    // Inner shadow effect (top edge darker for depth)
    ColourGradient shadowGrad(Colours::black.withAlpha(0.12f), (float)bounds.getX(), (float)bounds.getY(),
                              Colours::transparentBlack, (float)bounds.getX(), bounds.getY() + 10.0f, false);
    g.setGradientFill(shadowGrad);
    g.fillRoundedRectangle(bounds.toFloat(), 5.0f);

    // Panel border (stronger, more visible)
    g.setColour(laf.ampBorder.brighter(0.2f));
    g.drawRoundedRectangle(bounds.toFloat(), 5.0f, 1.25f);

    // Section title
    if (title.isNotEmpty())
    {
        // Accent dot
        g.setColour(laf.ampAccent);
        g.fillEllipse(bounds.getX() + 6.0f, bounds.getY() + 6.5f, 4.0f, 4.0f);

        // Title text
        g.setColour(laf.ampTextDim.brighter(0.15f));
        g.setFont(fm.getUIFont(10.0f, true));
        g.drawText(title, bounds.getX() + 14, bounds.getY() + 2, 100, 16, Justification::centredLeft);
    }
}
