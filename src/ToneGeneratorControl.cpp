/*
  ==============================================================================

    ToneGeneratorControl.cpp
    UI for the tone generator test tool

  ==============================================================================
*/

#include "ToneGeneratorControl.h"

#include "ColourScheme.h"
#include "FontManager.h"
#include "ToneGeneratorProcessor.h"

//==============================================================================
ToneGeneratorControl::ToneGeneratorControl(ToneGeneratorProcessor* processor) : toneProcessor(processor)
{
    auto& colours = ColourScheme::getInstance().colours;

    const int waveformRadioGroup = 1000;

    // Waveform buttons
    sineBtn = std::make_unique<TextButton>("SIN");
    sineBtn->addListener(this);
    sineBtn->setClickingTogglesState(true);
    sineBtn->setRadioGroupId(waveformRadioGroup);
    sineBtn->setTooltip("Sine wave - pure tone for tuner testing");
    addAndMakeVisible(sineBtn.get());

    sawBtn = std::make_unique<TextButton>("SAW");
    sawBtn->addListener(this);
    sawBtn->setClickingTogglesState(true);
    sawBtn->setRadioGroupId(waveformRadioGroup);
    sawBtn->setTooltip("Sawtooth wave - harmonic-rich for plugin testing");
    addAndMakeVisible(sawBtn.get());

    squareBtn = std::make_unique<TextButton>("SQR");
    squareBtn->addListener(this);
    squareBtn->setClickingTogglesState(true);
    squareBtn->setRadioGroupId(waveformRadioGroup);
    squareBtn->setTooltip("Square wave - digital edge cases");
    addAndMakeVisible(squareBtn.get());

    noiseBtn = std::make_unique<TextButton>("NOISE");
    noiseBtn->addListener(this);
    noiseBtn->setClickingTogglesState(true);
    noiseBtn->setRadioGroupId(waveformRadioGroup);
    noiseBtn->setTooltip("White noise - stress testing");
    addAndMakeVisible(noiseBtn.get());

    // Frequency slider (log scale)
    frequencySlider = std::make_unique<Slider>(Slider::LinearHorizontal, Slider::TextBoxRight);
    frequencySlider->setRange(20.0, 2000.0, 0.1);
    frequencySlider->setSkewFactorFromMidPoint(440.0);
    frequencySlider->setValue(processor->getFrequency(), dontSendNotification);
    frequencySlider->addListener(this);
    frequencySlider->setTextBoxStyle(Slider::TextBoxRight, false, 68, 18);
    styleEditableSlider(*frequencySlider, Colour(0xFF39D3E6), " Hz", 68);
    addAndMakeVisible(frequencySlider.get());

    // Detune slider
    detuneSlider = std::make_unique<Slider>(Slider::LinearHorizontal, Slider::TextBoxRight);
    detuneSlider->setRange(-100.0, 100.0, 0.1);
    detuneSlider->setValue(processor->getDetuneCents(), dontSendNotification);
    detuneSlider->addListener(this);
    detuneSlider->setTextBoxStyle(Slider::TextBoxRight, false, 54, 18);
    styleEditableSlider(*detuneSlider, colours["Warning Colour"], String::fromUTF8(" \xC2\xA2"), 54);
    addAndMakeVisible(detuneSlider.get());

    // Detune preset buttons (boundary testing)
    detune1Btn = std::make_unique<TextButton>("+1");
    detune1Btn->addListener(this);
    detune1Btn->setTooltip("+1 cent - just noticeable difference");
    addAndMakeVisible(detune1Btn.get());

    detune5Btn = std::make_unique<TextButton>("+5");
    detune5Btn->addListener(this);
    detune5Btn->setTooltip("+5 cents - typical 'in tune' threshold");
    addAndMakeVisible(detune5Btn.get());

    detune50Btn = std::make_unique<TextButton>("+50");
    detune50Btn->addListener(this);
    detune50Btn->setTooltip("+50 cents - quarter tone");
    addAndMakeVisible(detune50Btn.get());

    detune99Btn = std::make_unique<TextButton>("+99");
    detune99Btn->addListener(this);
    detune99Btn->setTooltip("+99 cents - near semitone BOUNDARY");
    addAndMakeVisible(detune99Btn.get());

    // Test mode buttons (mutually exclusive radio group)
    const int testModeRadioGroup = 1001;

    staticBtn = std::make_unique<TextButton>("STATIC");
    staticBtn->addListener(this);
    staticBtn->setClickingTogglesState(true);
    staticBtn->setRadioGroupId(testModeRadioGroup);
    staticBtn->setToggleState(true, dontSendNotification); // Default mode
    addAndMakeVisible(staticBtn.get());

    sweepBtn = std::make_unique<TextButton>("SWEEP");
    sweepBtn->addListener(this);
    sweepBtn->setClickingTogglesState(true);
    sweepBtn->setRadioGroupId(testModeRadioGroup);
    sweepBtn->setTooltip("Continuous frequency sweep");
    addAndMakeVisible(sweepBtn.get());

    driftBtn = std::make_unique<TextButton>("DRIFT");
    driftBtn->addListener(this);
    driftBtn->setClickingTogglesState(true);
    driftBtn->setRadioGroupId(testModeRadioGroup);
    driftBtn->setTooltip("Slow ±5 cent drift - tests tuner stability");
    addAndMakeVisible(driftBtn.get());

    // Amplitude slider
    amplitudeSlider = std::make_unique<Slider>(Slider::LinearHorizontal, Slider::TextBoxRight);
    amplitudeSlider->setRange(0.0, 1.0, 0.01);
    amplitudeSlider->setValue(processor->getAmplitude(), dontSendNotification);
    amplitudeSlider->addListener(this);
    amplitudeSlider->setTextBoxStyle(Slider::TextBoxRight, false, 46, 18);
    styleEditableSlider(*amplitudeSlider, colours["Success Colour"], {}, 46);
    addAndMakeVisible(amplitudeSlider.get());

    // Play button
    playButton = std::make_unique<TextButton>("PLAY");
    playButton->addListener(this);
    playButton->setColour(TextButton::buttonColourId, colours["Success Colour"].darker(0.3f));
    addAndMakeVisible(playButton.get());

    updateDisplay();
    syncButtonStates();

    // Update display at 30fps
    startTimerHz(30);

    setSize(340, 220);
}

ToneGeneratorControl::~ToneGeneratorControl()
{
    stopTimer();
}

//==============================================================================
void ToneGeneratorControl::buttonClicked(Button* button)
{
    if (toneProcessor == nullptr)
        return;

    // Waveform buttons
    if (button == sineBtn.get())
        toneProcessor->setWaveform(ToneGeneratorProcessor::Waveform::Sine);
    else if (button == sawBtn.get())
        toneProcessor->setWaveform(ToneGeneratorProcessor::Waveform::Saw);
    else if (button == squareBtn.get())
        toneProcessor->setWaveform(ToneGeneratorProcessor::Waveform::Square);
    else if (button == noiseBtn.get())
        toneProcessor->setWaveform(ToneGeneratorProcessor::Waveform::WhiteNoise);

    // Detune presets (toggle +/-)
    else if (button == detune1Btn.get())
    {
        float current = toneProcessor->getDetuneCents();
        toneProcessor->setDetuneCents(current == 1.0f ? -1.0f : 1.0f);
        detuneSlider->setValue(toneProcessor->getDetuneCents());
    }
    else if (button == detune5Btn.get())
    {
        float current = toneProcessor->getDetuneCents();
        toneProcessor->setDetuneCents(current == 5.0f ? -5.0f : 5.0f);
        detuneSlider->setValue(toneProcessor->getDetuneCents());
    }
    else if (button == detune50Btn.get())
    {
        float current = toneProcessor->getDetuneCents();
        toneProcessor->setDetuneCents(current == 50.0f ? -50.0f : 50.0f);
        detuneSlider->setValue(toneProcessor->getDetuneCents());
    }
    else if (button == detune99Btn.get())
    {
        float current = toneProcessor->getDetuneCents();
        toneProcessor->setDetuneCents(current == 99.0f ? -99.0f : 99.0f);
        detuneSlider->setValue(toneProcessor->getDetuneCents());
    }

    // Test mode buttons
    else if (button == staticBtn.get() || button == sweepBtn.get() || button == driftBtn.get())
    {
        if (button == staticBtn.get())
            toneProcessor->setTestMode(ToneGeneratorProcessor::TestMode::Static);
        else if (button == sweepBtn.get())
            toneProcessor->setTestMode(ToneGeneratorProcessor::TestMode::Sweep);
        else
            toneProcessor->setTestMode(ToneGeneratorProcessor::TestMode::Drift);
    }

    // Play/Stop
    else if (button == playButton.get())
    {
        bool nowPlaying = !toneProcessor->isPlaying();
        toneProcessor->setPlaying(nowPlaying);
    }

    syncButtonStates();
    repaint();
}

void ToneGeneratorControl::sliderValueChanged(Slider* slider)
{
    if (toneProcessor == nullptr)
        return;

    if (slider == frequencySlider.get())
        toneProcessor->setFrequency(static_cast<float>(slider->getValue()));
    else if (slider == detuneSlider.get())
        toneProcessor->setDetuneCents(static_cast<float>(slider->getValue()));
    else if (slider == amplitudeSlider.get())
        toneProcessor->setAmplitude(static_cast<float>(slider->getValue()));

    updateDisplay();
    repaint();
}

//==============================================================================
void ToneGeneratorControl::timerCallback()
{
    if (toneProcessor == nullptr)
        return;

    updateDisplay();

    repaint();
}

//==============================================================================
void ToneGeneratorControl::paint(Graphics& g)
{
    auto& colours = ColourScheme::getInstance().colours;
    auto& fonts = FontManager::getInstance();
    const auto accent = Colour(0xFF39D3E6);
    const auto warning = colours["Warning Colour"];
    const auto success = colours["Success Colour"];

    drawChromeShell(g, getLocalBounds().toFloat());
    drawDisplayPanel(g, displayPanel.toFloat());
    drawWaveformGlyph(g, waveformGlyphArea.toFloat());

    g.setColour(colours["Text Colour"].withAlpha(0.88f));
    g.setFont(fonts.getBadgeFont());
    g.drawText("PITCH", pitchPanel.toFloat().removeFromTop(12.0f), Justification::left);
    g.drawText("WAVE", waveformPanel.toFloat().removeFromTop(12.0f), Justification::left);
    g.drawText("OUTPUT", bottomPanel.toFloat().removeFromTop(12.0f), Justification::left);

    const auto baseFrequency = toneProcessor != nullptr ? toneProcessor->getFrequency() : 440.0f;
    const auto detune = toneProcessor != nullptr ? toneProcessor->getDetuneCents() : 0.0f;
    const auto amplitude = toneProcessor != nullptr ? toneProcessor->getAmplitude() : 0.0f;
    const auto freqNorm = jlimit(0.0f, 1.0f,
                                 (std::log10(jmax(20.0f, baseFrequency)) - std::log10(20.0f)) /
                                     (std::log10(2000.0f) - std::log10(20.0f)));
    const auto detuneNorm = jmap(jlimit(-100.0f, 100.0f, detune), -100.0f, 100.0f, 0.0f, 1.0f);

    drawSliderLane(g, frequencyRail.toFloat(), "FREQ", freqNorm, accent);
    drawSliderLane(g, detuneRail.toFloat(), "FINE", detuneNorm, warning);
    drawSliderLane(g, amplitudeRail.toFloat(), "LVL", jlimit(0.0f, 1.0f, amplitude), success);

    if (toneProcessor != nullptr && toneProcessor->isPlaying())
    {
        auto led = headerArea.toFloat().removeFromLeft(18.0f).reduced(2.0f).withSizeKeepingCentre(9.0f, 9.0f);
        g.setColour(success.withAlpha(0.28f));
        g.fillEllipse(led.expanded(5.0f));
        g.setColour(success);
        g.fillEllipse(led);
    }
}

void ToneGeneratorControl::resized()
{
    auto& colours = ColourScheme::getInstance().colours;
    auto bounds = getLocalBounds().reduced(8, 6);

    headerArea = bounds.removeFromTop(20);
    bounds.removeFromTop(4);

    displayPanel = bounds.removeFromTop(60);
    auto displayInner = displayPanel.reduced(10, 8);
    waveformGlyphArea = displayInner.removeFromRight(76);
    displayInner.removeFromRight(10);
    frequencyChipArea = displayInner.removeFromTop(24);
    noteChipArea = displayInner.removeFromTop(18).withWidth(62);

    bounds.removeFromTop(7);
    pitchPanel = bounds.removeFromTop(48);

    auto pitchInner = pitchPanel.reduced(10, 7);
    auto freqRow = pitchInner.removeFromTop(18);
    frequencyChipArea = frequencyChipArea.reduced(0, 1);
    auto frequencyControl = freqRow.withTrimmedLeft(35);
    frequencyRail = frequencyControl.withTrimmedRight(76).reduced(0, 4);
    frequencySlider->setBounds(frequencyControl.expanded(7, 1));

    pitchInner.removeFromTop(4);
    auto detuneRow = pitchInner.removeFromTop(18);
    auto presetArea = detuneRow.removeFromRight(114);
    auto detuneControl = detuneRow.withTrimmedLeft(35);
    detuneChipArea = detuneControl.removeFromRight(60);
    detuneRail = detuneControl.reduced(0, 4);
    detuneSlider->setBounds(detuneRow.withTrimmedLeft(35).expanded(7, 1));

    int btnW = 26;
    detune1Btn->setBounds(presetArea.removeFromLeft(btnW).reduced(0, 1));
    presetArea.removeFromLeft(2);
    detune5Btn->setBounds(presetArea.removeFromLeft(btnW).reduced(0, 1));
    presetArea.removeFromLeft(2);
    detune50Btn->setBounds(presetArea.removeFromLeft(btnW).reduced(0, 1));
    presetArea.removeFromLeft(2);
    detune99Btn->setBounds(presetArea.removeFromLeft(btnW).reduced(0, 1));

    bounds.removeFromTop(7);
    waveformPanel = bounds.removeFromTop(26);
    auto waveformButtons = waveformPanel.reduced(10, 3);
    const int wfBtnW = waveformButtons.getWidth() / 4;
    sineBtn->setBounds(waveformButtons.removeFromLeft(wfBtnW).reduced(1, 0));
    sawBtn->setBounds(waveformButtons.removeFromLeft(wfBtnW).reduced(1, 0));
    squareBtn->setBounds(waveformButtons.removeFromLeft(wfBtnW).reduced(1, 0));
    noiseBtn->setBounds(waveformButtons.reduced(1, 0));

    bounds.removeFromTop(6);
    bottomPanel = bounds.removeFromTop(32);
    auto bottomInner = bottomPanel.reduced(10, 5);
    auto levelArea = bottomInner.removeFromLeft(124);
    amplitudeChipArea = levelArea.removeFromRight(52);
    amplitudeRail = levelArea.withTrimmedLeft(30).reduced(0, 5);
    amplitudeSlider->setBounds(levelArea.withTrimmedLeft(30).expanded(7, 1));

    bottomInner.removeFromLeft(9);
    modePanel = bottomInner;
    const int modeBtnW = 42;
    staticBtn->setBounds(modePanel.removeFromLeft(modeBtnW).reduced(1, 0));
    modePanel.removeFromLeft(2);
    sweepBtn->setBounds(modePanel.removeFromLeft(modeBtnW).reduced(1, 0));
    modePanel.removeFromLeft(2);
    driftBtn->setBounds(modePanel.removeFromLeft(modeBtnW).reduced(1, 0));
    modePanel.removeFromLeft(6);
    playButton->setBounds(modePanel.reduced(0, -1));

    syncButtonStates();

    for (auto* btn : {detune1Btn.get(), detune5Btn.get(), detune50Btn.get(), detune99Btn.get()})
        styleButtonChrome(*btn, colours["Warning Colour"], false);
}

void ToneGeneratorControl::updateDisplay()
{
    if (toneProcessor == nullptr)
        return;

    displayedFrequency = toneProcessor->getActualFrequency();
    const int midiNote = ToneGeneratorProcessor::frequencyToMidiNote(displayedFrequency);
    displayedNote = getNoteName(midiNote);
}

void ToneGeneratorControl::syncButtonStates()
{
    if (toneProcessor == nullptr)
        return;

    auto& colours = ColourScheme::getInstance().colours;
    const auto accent = Colour(0xFF39D3E6);
    const auto waveform = toneProcessor->getWaveform();
    const auto mode = toneProcessor->getTestMode();
    const auto playing = toneProcessor->isPlaying();

    sineBtn->setToggleState(waveform == ToneGeneratorProcessor::Waveform::Sine, dontSendNotification);
    sawBtn->setToggleState(waveform == ToneGeneratorProcessor::Waveform::Saw, dontSendNotification);
    squareBtn->setToggleState(waveform == ToneGeneratorProcessor::Waveform::Square, dontSendNotification);
    noiseBtn->setToggleState(waveform == ToneGeneratorProcessor::Waveform::WhiteNoise, dontSendNotification);

    staticBtn->setToggleState(mode == ToneGeneratorProcessor::TestMode::Static, dontSendNotification);
    sweepBtn->setToggleState(mode == ToneGeneratorProcessor::TestMode::Sweep, dontSendNotification);
    driftBtn->setToggleState(mode == ToneGeneratorProcessor::TestMode::Drift, dontSendNotification);

    styleButtonChrome(*sineBtn, accent, sineBtn->getToggleState());
    styleButtonChrome(*sawBtn, accent, sawBtn->getToggleState());
    styleButtonChrome(*squareBtn, accent, squareBtn->getToggleState());
    styleButtonChrome(*noiseBtn, accent, noiseBtn->getToggleState());
    styleButtonChrome(*staticBtn, colours["Button Highlight"], staticBtn->getToggleState());
    styleButtonChrome(*sweepBtn, colours["Button Highlight"], sweepBtn->getToggleState());
    styleButtonChrome(*driftBtn, colours["Button Highlight"], driftBtn->getToggleState());

    playButton->setButtonText(playing ? "STOP" : "PLAY");
    styleButtonChrome(*playButton, playing ? colours["Danger Colour"] : colours["Success Colour"], playing);
}

void ToneGeneratorControl::styleButtonChrome(TextButton& button, Colour accent, bool active)
{
    auto& colours = ColourScheme::getInstance().colours;
    button.setColour(TextButton::buttonColourId,
                     active ? accent.darker(0.45f).withMultipliedSaturation(1.25f)
                            : colours["Plugin Background"].interpolatedWith(accent, 0.12f).brighter(0.02f));
    button.setColour(TextButton::buttonOnColourId, accent.darker(0.35f));
    button.setColour(TextButton::textColourOffId, colours["Text Colour"].withAlpha(active ? 0.98f : 0.68f));
    button.setColour(TextButton::textColourOnId, Colours::white.withAlpha(0.96f));
}

void ToneGeneratorControl::styleEditableSlider(Slider& slider, Colour accent, const String& suffix, int textBoxWidth)
{
    auto& colours = ColourScheme::getInstance().colours;
    slider.setTextBoxStyle(Slider::TextBoxRight, false, textBoxWidth, 18);
    slider.setTextValueSuffix(suffix);
    slider.setAlpha(1.0f);
    slider.setColour(Slider::backgroundColourId, Colours::transparentBlack);
    slider.setColour(Slider::trackColourId, Colours::transparentBlack);
    slider.setColour(Slider::thumbColourId, Colours::transparentBlack);
    slider.setColour(Slider::textBoxTextColourId, colours["Text Colour"].withAlpha(0.90f));
    slider.setColour(Slider::textBoxBackgroundColourId, colours["Field Background"].darker(0.22f).withAlpha(0.80f));
    slider.setColour(Slider::textBoxOutlineColourId, accent.withAlpha(0.48f));
    slider.setColour(Slider::textBoxHighlightColourId, accent.withAlpha(0.35f));
}

void ToneGeneratorControl::drawChromeShell(Graphics& g, Rectangle<float> bounds)
{
    auto& colours = ColourScheme::getInstance().colours;
    auto& fonts = FontManager::getInstance();
    const auto accent = Colour(0xFF39D3E6);
    const auto shell = bounds.reduced(2.0f);

    ColourGradient body(colours["Plugin Background"].interpolatedWith(accent, 0.13f).brighter(0.05f), shell.getX(),
                        shell.getY(), colours["Plugin Background"].darker(0.18f), shell.getX(), shell.getBottom(),
                        false);
    body.addColour(0.42, colours["Plugin Background"].interpolatedWith(accent, 0.08f));
    body.addColour(0.74, colours["Plugin Background"].darker(0.07f));
    g.setGradientFill(body);
    g.fillRoundedRectangle(shell, 8.0f);

    g.setColour(accent.withAlpha(0.18f));
    g.drawRoundedRectangle(shell.reduced(0.5f), 8.0f, 1.4f);
    g.setColour(Colours::white.withAlpha(0.08f));
    g.drawLine(shell.getX() + 10.0f, shell.getY() + 1.0f, shell.getRight() - 10.0f, shell.getY() + 1.0f, 1.0f);
    g.setColour(Colours::black.withAlpha(0.24f));
    g.drawLine(shell.getX() + 8.0f, shell.getBottom() - 1.0f, shell.getRight() - 8.0f, shell.getBottom() - 1.0f, 1.0f);

    auto header = headerArea.toFloat();
    ColourGradient headerFill(accent.withAlpha(0.16f), header.getX(), header.getY(), Colours::black.withAlpha(0.18f),
                              header.getX(), header.getBottom(), false);
    g.setGradientFill(headerFill);
    g.fillRoundedRectangle(header.reduced(1.0f), 5.0f);

    auto led = header.removeFromLeft(17.0f).reduced(3.5f);
    g.setColour(accent.withAlpha(0.22f));
    g.fillEllipse(led.expanded(3.0f));
    g.setColour(accent);
    g.fillEllipse(led);
    g.setColour(Colours::white.withAlpha(0.62f));
    g.fillEllipse(led.withSizeKeepingCentre(3.0f, 3.0f).translated(-1.0f, -1.0f));

    g.setFont(fonts.getBadgeFont());
    g.setColour(colours["Text Colour"].withAlpha(0.90f));
    g.drawText("TONE GENERATOR", header.removeFromLeft(122.0f), Justification::centredLeft);
    g.setColour(accent.withAlpha(0.52f));
    g.drawText("CAL", header.removeFromRight(34.0f), Justification::centredRight);
}

void ToneGeneratorControl::drawDisplayPanel(Graphics& g, Rectangle<float> bounds)
{
    auto& colours = ColourScheme::getInstance().colours;
    auto& fonts = FontManager::getInstance();
    const auto accent = Colour(0xFF39D3E6);
    auto panel = bounds.reduced(1.0f);

    ColourGradient panelFill(colours["Field Background"].darker(0.20f), panel.getX(), panel.getY(),
                             colours["Field Background"].interpolatedWith(accent, 0.10f), panel.getX(),
                             panel.getBottom(), false);
    panelFill.addColour(0.35, colours["Plugin Background"].darker(0.18f));
    g.setGradientFill(panelFill);
    g.fillRoundedRectangle(panel, 7.0f);

    g.setColour(accent.withAlpha(0.32f));
    g.drawRoundedRectangle(panel, 7.0f, 1.1f);
    g.setColour(Colours::white.withAlpha(0.07f));
    g.drawLine(panel.getX() + 8.0f, panel.getY() + 1.0f, panel.getRight() - 8.0f, panel.getY() + 1.0f);

    g.setFont(fonts.getMonoDisplayFont(20.0f));
    g.setColour(accent.brighter(0.18f));
    g.drawText(String(displayedFrequency, 1) + " Hz", frequencyChipArea.toFloat().expanded(0.0f, 2.0f),
               Justification::centredLeft, true);

    drawValueChip(g, noteChipArea.toFloat(), displayedNote, accent);

    auto status = bounds.reduced(8.0f, 7.0f).removeFromRight(70.0f).removeFromBottom(16.0f);
    const bool playing = toneProcessor != nullptr && toneProcessor->isPlaying();
    drawValueChip(g, status, playing ? "ACTIVE" : "READY", playing ? colours["Success Colour"] : accent);
}

void ToneGeneratorControl::drawWaveformGlyph(Graphics& g, Rectangle<float> bounds)
{
    auto& colours = ColourScheme::getInstance().colours;
    auto& fonts = FontManager::getInstance();
    const auto accent = Colour(0xFF39D3E6);
    auto tile = bounds.reduced(1.0f);

    ColourGradient tileFill(Colours::black.withAlpha(0.22f), tile.getX(), tile.getY(),
                            accent.darker(0.70f).withAlpha(0.22f), tile.getX(), tile.getBottom(), false);
    g.setGradientFill(tileFill);
    g.fillRoundedRectangle(tile, 6.0f);
    g.setColour(accent.withAlpha(0.34f));
    g.drawRoundedRectangle(tile, 6.0f, 1.0f);

    auto graph = tile.reduced(9.0f, 9.0f).withTrimmedBottom(9.0f);
    g.setColour(accent.withAlpha(0.10f));
    for (int i = 1; i < 4; ++i)
    {
        const auto x = graph.getX() + graph.getWidth() * static_cast<float>(i) / 4.0f;
        g.drawVerticalLine(roundToInt(x), graph.getY(), graph.getBottom());
    }
    g.drawHorizontalLine(roundToInt(graph.getCentreY()), graph.getX(), graph.getRight());

    Path wavePath;
    const auto waveform = toneProcessor != nullptr ? toneProcessor->getWaveform() : ToneGeneratorProcessor::Waveform::Sine;
    for (int i = 0; i < 48; ++i)
    {
        const float phase = static_cast<float>(i) / 47.0f;
        float value = 0.0f;
        if (waveform == ToneGeneratorProcessor::Waveform::Sine)
            value = std::sin(phase * MathConstants<float>::twoPi);
        else if (waveform == ToneGeneratorProcessor::Waveform::Saw)
            value = 2.0f * phase - 1.0f;
        else if (waveform == ToneGeneratorProcessor::Waveform::Square)
            value = phase < 0.5f ? 0.82f : -0.82f;
        else
            value = std::sin(phase * 91.0f) * std::sin(phase * 37.0f);

        const auto x = graph.getX() + phase * graph.getWidth();
        const auto y = graph.getCentreY() - value * graph.getHeight() * 0.42f;
        if (i == 0)
            wavePath.startNewSubPath(x, y);
        else
            wavePath.lineTo(x, y);
    }

    g.setColour(accent.withAlpha(0.22f));
    g.strokePath(wavePath, PathStrokeType(4.0f, PathStrokeType::curved, PathStrokeType::rounded));
    g.setColour(accent.brighter(0.25f));
    g.strokePath(wavePath, PathStrokeType(1.6f, PathStrokeType::curved, PathStrokeType::rounded));

    g.setFont(fonts.getBadgeFont());
    g.setColour(colours["Text Colour"].withAlpha(0.58f));
    g.drawText("WAVE", tile.removeFromBottom(12.0f), Justification::centred);
}

void ToneGeneratorControl::drawValueChip(Graphics& g, Rectangle<float> bounds, const String& text, Colour accent)
{
    auto& colours = ColourScheme::getInstance().colours;
    auto& fonts = FontManager::getInstance();
    auto chip = bounds.reduced(0.5f);

    ColourGradient fill(Colours::black.withAlpha(0.24f), chip.getX(), chip.getY(),
                        accent.darker(0.60f).withAlpha(0.32f), chip.getX(), chip.getBottom(), false);
    g.setGradientFill(fill);
    g.fillRoundedRectangle(chip, 5.0f);
    g.setColour(accent.withAlpha(0.54f));
    g.drawRoundedRectangle(chip, 5.0f, 0.9f);
    g.setColour(colours["Text Colour"].withAlpha(0.88f));
    g.setFont(fonts.getMonoDisplayFont(10.6f));
    g.drawText(text, chip.reduced(5.0f, 0.0f), Justification::centred, true);
}

void ToneGeneratorControl::drawSliderLane(Graphics& g, Rectangle<float> bounds, const String& label, float normalisedValue,
                                          Colour accent)
{
    auto& colours = ColourScheme::getInstance().colours;
    auto& fonts = FontManager::getInstance();
    auto labelArea = bounds.removeFromLeft(30.0f);
    auto rail = bounds.reduced(1.0f, 3.0f);

    g.setFont(fonts.getBadgeFont());
    g.setColour(colours["Text Colour"].withAlpha(0.56f));
    g.drawText(label, labelArea, Justification::centredLeft);

    ColourGradient bed(Colours::black.withAlpha(0.34f), rail.getX(), rail.getY(),
                       colours["Plugin Background"].brighter(0.05f).withAlpha(0.38f), rail.getX(), rail.getBottom(),
                       false);
    g.setGradientFill(bed);
    g.fillRoundedRectangle(rail, 4.0f);
    g.setColour(Colours::black.withAlpha(0.38f));
    g.drawRoundedRectangle(rail, 4.0f, 0.8f);

    auto fill = rail.withWidth(rail.getWidth() * jlimit(0.0f, 1.0f, normalisedValue));
    ColourGradient fillGradient(accent.withAlpha(0.76f), fill.getX(), fill.getY(),
                                accent.brighter(0.22f).withAlpha(0.92f), fill.getRight(), fill.getBottom(), false);
    g.setGradientFill(fillGradient);
    g.fillRoundedRectangle(fill, 4.0f);

    const auto thumbX = rail.getX() + rail.getWidth() * jlimit(0.0f, 1.0f, normalisedValue);
    auto thumb = Rectangle<float>(thumbX - 3.5f, rail.getY() - 3.0f, 7.0f, rail.getHeight() + 6.0f);
    g.setColour(accent.withAlpha(0.30f));
    g.fillRoundedRectangle(thumb.expanded(2.0f, 1.0f), 4.0f);
    g.setColour(accent.brighter(0.18f));
    g.fillRoundedRectangle(thumb, 3.5f);
}

//==============================================================================
String ToneGeneratorControl::getNoteName(int midiNote) const
{
    if (midiNote < 0 || midiNote > 127)
        return "---";

    static const char* noteNames[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
    int octave = (midiNote / 12) - 1;
    int noteIndex = midiNote % 12;

    return String(noteNames[noteIndex]) + String(octave);
}
