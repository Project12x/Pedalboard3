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

    // Waveform buttons
    sineBtn = std::make_unique<TextButton>("SIN");
    sineBtn->addListener(this);
    sineBtn->setTooltip("Sine wave - pure tone for tuner testing");
    addAndMakeVisible(sineBtn.get());

    sawBtn = std::make_unique<TextButton>("SAW");
    sawBtn->addListener(this);
    sawBtn->setTooltip("Sawtooth wave - harmonic-rich for plugin testing");
    addAndMakeVisible(sawBtn.get());

    squareBtn = std::make_unique<TextButton>("SQR");
    squareBtn->addListener(this);
    squareBtn->setTooltip("Square wave - digital edge cases");
    addAndMakeVisible(squareBtn.get());

    noiseBtn = std::make_unique<TextButton>("NOISE");
    noiseBtn->addListener(this);
    noiseBtn->setTooltip("White noise - stress testing");
    addAndMakeVisible(noiseBtn.get());

    // Frequency slider (log scale)
    frequencySlider = std::make_unique<Slider>(Slider::LinearHorizontal, Slider::TextBoxRight);
    frequencySlider->setRange(20.0, 2000.0, 0.1);
    frequencySlider->setSkewFactorFromMidPoint(440.0);
    frequencySlider->setValue(processor->getFrequency(), dontSendNotification);
    frequencySlider->addListener(this);
    frequencySlider->setTextValueSuffix(" Hz");
    addAndMakeVisible(frequencySlider.get());

    // Detune slider
    detuneSlider = std::make_unique<Slider>(Slider::LinearHorizontal, Slider::TextBoxRight);
    detuneSlider->setRange(-100.0, 100.0, 0.1);
    detuneSlider->setValue(processor->getDetuneCents(), dontSendNotification);
    detuneSlider->addListener(this);
    detuneSlider->setTextValueSuffix(String::fromUTF8(" \xC2\xA2")); // Cent sign (U+00A2)
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
    addAndMakeVisible(amplitudeSlider.get());

    // Play button
    playButton = std::make_unique<TextButton>("PLAY");
    playButton->addListener(this);
    playButton->setColour(TextButton::buttonColourId, colours["Success Colour"].darker(0.3f));
    addAndMakeVisible(playButton.get());

    // Update display at 30fps
    startTimerHz(30);

    setSize(280, 180);
}

ToneGeneratorControl::~ToneGeneratorControl()
{
    stopTimer();
}

//==============================================================================
void ToneGeneratorControl::buttonClicked(Button* button)
{
    auto& colours = ColourScheme::getInstance().colours;
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
        playButton->setButtonText(nowPlaying ? "STOP" : "PLAY");
        playButton->setColour(TextButton::buttonColourId, nowPlaying ? colours["Danger Colour"].darker(0.3f)
                                                                     : colours["Success Colour"].darker(0.3f));
    }

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
}

//==============================================================================
void ToneGeneratorControl::timerCallback()
{
    if (toneProcessor == nullptr)
        return;

    displayedFrequency = toneProcessor->getActualFrequency();
    int midiNote = ToneGeneratorProcessor::frequencyToMidiNote(displayedFrequency);
    displayedNote = getNoteName(midiNote);

    repaint();
}

//==============================================================================
void ToneGeneratorControl::paint(Graphics& g)
{
    auto& colours = ColourScheme::getInstance().colours;
    auto& fonts = FontManager::getInstance();
    auto bounds = getLocalBounds().toFloat();

    // Background
    g.setColour(colours["Plugin Background"]);
    g.fillAll();

    // Border
    g.setColour(colours["Plugin Border"].withAlpha(0.3f));
    g.drawRect(bounds.reduced(1), 1.0f);

    // Title
    g.setColour(colours["Text Colour"]);
    g.setFont(fonts.getUIFont(12.0f, true));
    g.drawText("TONE GENERATOR", bounds.removeFromTop(18).reduced(4, 0), Justification::left);

    // Frequency display
    auto displayArea = bounds.removeFromTop(22).reduced(4, 0);
    g.setFont(fonts.getMonoFont(16.0f));
    g.setColour(Colour(0xFF00E676));
    g.drawText(String(displayedFrequency, 1) + " Hz  " + displayedNote, displayArea, Justification::left);

    // Labels
    g.setColour(colours["Text Colour"].withAlpha(0.6f));
    g.setFont(fonts.getUIFont(9.0f));
    g.drawText("Freq:", Rectangle<float>(4, 44, 30, 14), Justification::left);
    g.drawText("Detune:", Rectangle<float>(4, 66, 40, 14), Justification::left);
    g.drawText("Level:", Rectangle<float>(4, 108, 30, 14), Justification::left);
}

void ToneGeneratorControl::resized()
{
    auto& colours = ColourScheme::getInstance().colours;
    auto bounds = getLocalBounds().reduced(4);

    bounds.removeFromTop(18); // Title
    bounds.removeFromTop(22); // Frequency display

    // Frequency slider row
    auto freqRow = bounds.removeFromTop(20);
    freqRow.removeFromLeft(32); // Label space
    frequencySlider->setBounds(freqRow);

    bounds.removeFromTop(2);

    // Detune row
    auto detuneRow = bounds.removeFromTop(20);
    detuneRow.removeFromLeft(44); // Label space

    // Detune slider takes most of the row
    auto presetArea = detuneRow.removeFromRight(116);
    detuneSlider->setBounds(detuneRow);

    // Preset buttons (wider with spacing for text visibility)
    int btnW = 28;
    detune1Btn->setBounds(presetArea.removeFromLeft(btnW));
    presetArea.removeFromLeft(1);
    detune5Btn->setBounds(presetArea.removeFromLeft(btnW));
    presetArea.removeFromLeft(1);
    detune50Btn->setBounds(presetArea.removeFromLeft(btnW));
    presetArea.removeFromLeft(1);
    detune99Btn->setBounds(presetArea.removeFromLeft(btnW));

    bounds.removeFromTop(4);

    // Waveform buttons row
    auto waveformRow = bounds.removeFromTop(18);
    int wfBtnW = waveformRow.getWidth() / 4;
    sineBtn->setBounds(waveformRow.removeFromLeft(wfBtnW));
    sawBtn->setBounds(waveformRow.removeFromLeft(wfBtnW));
    squareBtn->setBounds(waveformRow.removeFromLeft(wfBtnW));
    noiseBtn->setBounds(waveformRow);

    bounds.removeFromTop(2);

    // Amplitude slider row
    auto ampRow = bounds.removeFromTop(20);
    ampRow.removeFromLeft(32); // Label space
    amplitudeSlider->setBounds(ampRow);

    bounds.removeFromTop(4);

    // Test mode + Play row
    auto modeRow = bounds.removeFromTop(22);
    int modeBtnW = 50;
    staticBtn->setBounds(modeRow.removeFromLeft(modeBtnW));
    modeRow.removeFromLeft(2);
    sweepBtn->setBounds(modeRow.removeFromLeft(modeBtnW));
    modeRow.removeFromLeft(2);
    driftBtn->setBounds(modeRow.removeFromLeft(modeBtnW));
    modeRow.removeFromLeft(8);
    playButton->setBounds(modeRow);

    // Style all buttons
    for (auto* btn : {sineBtn.get(), sawBtn.get(), squareBtn.get(), noiseBtn.get(), detune1Btn.get(), detune5Btn.get(),
                      detune50Btn.get(), detune99Btn.get(), staticBtn.get(), sweepBtn.get(), driftBtn.get()})
    {
        btn->setColour(TextButton::buttonColourId, colours["Plugin Border"].darker(0.1f));
        btn->setColour(TextButton::textColourOffId, colours["Text Colour"].withAlpha(0.8f));
    }
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
