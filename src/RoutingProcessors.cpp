/*
  ==============================================================================

    RoutingProcessors.cpp
    Created: 27 Jan 2026

    Processors for A/B routing (Splitter and Mixer).

  ==============================================================================
*/

#include "RoutingProcessors.h"

#include "ColourScheme.h"
#include "Images.h"
#include "PedalboardProcessorEditors.h"
#include "Vectors.h"

//==============================================================================
// Controls for Splitter
class SplitterControl : public Component, public Button::Listener
{
  public:
    SplitterControl(SplitterProcessor* proc) : processor(proc)
    {
        addAndMakeVisible(muteA);
        muteA.setButtonText("A");
        muteA.setClickingTogglesState(true);
        muteA.setColour(TextButton::buttonOnColourId, Colours::red);
        muteA.addListener(this);

        addAndMakeVisible(muteB);
        muteB.setButtonText("B");
        muteB.setClickingTogglesState(true);
        muteB.setColour(TextButton::buttonOnColourId, Colours::red);
        muteB.addListener(this);

        // Update state
        muteA.setToggleState(processor->getOutputMute(0), dontSendNotification);
        muteB.setToggleState(processor->getOutputMute(1), dontSendNotification);
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced(4);
        auto top = area.removeFromTop(area.getHeight() / 2);
        muteA.setBounds(top.reduced(2));
        muteB.setBounds(area.reduced(2));
    }

    void buttonClicked(Button* b) override
    {
        if (b == &muteA)
            processor->setOutputMute(0, muteA.getToggleState());
        else if (b == &muteB)
            processor->setOutputMute(1, muteB.getToggleState());
    }

    void paint(Graphics& g) override
    {
        g.fillAll(ColourScheme::getInstance().colours["Plugin Background"]);
        g.setColour(ColourScheme::getInstance().colours["Plugin Border"]);
        g.drawRect(getLocalBounds(), 1);
    }

  private:
    SplitterProcessor* processor;
    TextButton muteA;
    TextButton muteB;
};

//==============================================================================
// Channel strip editor for MixerProcessor
class MixerEditor : public AudioProcessorEditor, private Timer
{
  public:
    MixerEditor(MixerProcessor& proc) : AudioProcessorEditor(proc), mixer(proc)
    {
        setSize(230, 340);

        for (int ch = 0; ch < MixerProcessor::NumChannels; ++ch)
        {
            // Gain fader (vertical, dB scale)
            auto& f = faders[ch];
            f.setSliderStyle(Slider::LinearVertical);
            f.setTextBoxStyle(Slider::TextBoxBelow, false, 50, 16);
            f.setRange(-60.0, 12.0, 0.1);
            f.setValue(mixer.getChannelGainDb(ch), dontSendNotification);
            f.onValueChange = [this, ch]() { mixer.setChannelGainDb(ch, static_cast<float>(faders[ch].getValue())); };
            addAndMakeVisible(f);

            // Pan knob (rotary)
            auto& p = panKnobs[ch];
            p.setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
            p.setTextBoxStyle(Slider::NoTextBox, false, 0, 0);
            p.setRange(-1.0, 1.0, 0.01);
            p.setDoubleClickReturnValue(true, 0.0);
            p.setValue(mixer.getChannelPan(ch), dontSendNotification);
            p.onValueChange = [this, ch]() { mixer.setChannelPan(ch, static_cast<float>(panKnobs[ch].getValue())); };
            addAndMakeVisible(p);

            // Mute button
            auto& m = muteButtons[ch];
            m.setButtonText("M");
            m.setClickingTogglesState(true);
            m.setToggleState(mixer.getChannelMute(ch), dontSendNotification);
            m.onClick = [this, ch]() { mixer.setChannelMute(ch, muteButtons[ch].getToggleState()); };
            addAndMakeVisible(m);

            // Solo button
            auto& s = soloButtons[ch];
            s.setButtonText("S");
            s.setClickingTogglesState(true);
            s.setToggleState(mixer.getChannelSolo(ch), dontSendNotification);
            s.onClick = [this, ch]() { mixer.setChannelSolo(ch, soloButtons[ch].getToggleState()); };
            addAndMakeVisible(s);

            // Phase invert button
            auto& ph = phaseButtons[ch];
            ph.setButtonText(CharPointer_UTF8("\xc3\x98")); // "O-slash" as phase symbol
            ph.setClickingTogglesState(true);
            ph.setToggleState(mixer.getChannelPhaseInvert(ch), dontSendNotification);
            ph.onClick = [this, ch]() { mixer.setChannelPhaseInvert(ch, phaseButtons[ch].getToggleState()); };
            addAndMakeVisible(ph);
        }

        startTimerHz(30);
    }

    ~MixerEditor() override { stopTimer(); }

    void resized() override
    {
        auto area = getLocalBounds().reduced(4);
        int stripW = area.getWidth() / 2;

        for (int ch = 0; ch < MixerProcessor::NumChannels; ++ch)
        {
            auto strip = area.removeFromLeft(stripW).reduced(2);

            // Phase button at top
            phaseButtons[ch].setBounds(strip.removeFromTop(22).reduced(2, 0));
            strip.removeFromTop(2);

            // VU meter area (reserved for paint)
            vuAreas[ch] = strip.removeFromTop(100);
            strip.removeFromTop(2);

            // Gain fader
            faders[ch].setBounds(strip.removeFromTop(110));
            strip.removeFromTop(2);

            // Pan knob
            panKnobs[ch].setBounds(strip.removeFromTop(42));
            strip.removeFromTop(2);

            // Mute + Solo in a row
            auto btnRow = strip.removeFromTop(24);
            int btnW = btnRow.getWidth() / 2;
            muteButtons[ch].setBounds(btnRow.removeFromLeft(btnW).reduced(2, 0));
            soloButtons[ch].setBounds(btnRow.reduced(2, 0));
        }
    }

    void paint(Graphics& g) override
    {
        auto& cs = ColourScheme::getInstance();
        g.fillAll(cs.colours["Plugin Background"]);
        g.setColour(cs.colours["Plugin Border"]);
        g.drawRect(getLocalBounds(), 1);

        // Draw channel labels
        g.setFont(12.0f);
        g.setColour(cs.colours["Text Colour"]);
        int stripW = (getWidth() - 8) / 2;
        g.drawText("A", 4, getHeight() - 18, stripW, 16, Justification::centred);
        g.drawText("B", 4 + stripW, getHeight() - 18, stripW, 16, Justification::centred);

        // Draw VU meters for each channel
        for (int ch = 0; ch < MixerProcessor::NumChannels; ++ch)
        {
            drawVuMeter(g, ch, cs);
        }
    }

  private:
    MixerProcessor& mixer;
    Slider faders[MixerProcessor::NumChannels];
    Slider panKnobs[MixerProcessor::NumChannels];
    TextButton muteButtons[MixerProcessor::NumChannels];
    TextButton soloButtons[MixerProcessor::NumChannels];
    TextButton phaseButtons[MixerProcessor::NumChannels];
    Rectangle<int> vuAreas[MixerProcessor::NumChannels];

    void timerCallback() override
    {
        // Sync button colors for mute/solo state
        for (int ch = 0; ch < MixerProcessor::NumChannels; ++ch)
        {
            // Color mute button red when active
            muteButtons[ch].setColour(TextButton::buttonOnColourId, Colours::red);
            // Color solo button yellow when active
            soloButtons[ch].setColour(TextButton::buttonOnColourId, Colour(0xFFCCAA00));
            // Color phase button orange when active
            phaseButtons[ch].setColour(TextButton::buttonOnColourId, Colour(0xFFFF8800));
        }
        repaint(); // Repaint VU meters
    }

    void drawVuMeter(Graphics& g, int ch, ColourScheme& cs)
    {
        auto area = vuAreas[ch];
        if (area.isEmpty())
            return;

        // Background
        g.setColour(Colour(0xFF0A0A14));
        g.fillRect(area);

        // Draw L and R bars
        int barW = (area.getWidth() - 6) / 2;
        auto leftBar = area.withWidth(barW).translated(2, 0).reduced(0, 2);
        auto rightBar = leftBar.translated(barW + 2, 0);

        float vuL = mixer.channels[ch].vuLevelL.load(std::memory_order_relaxed);
        float vuR = mixer.channels[ch].vuLevelR.load(std::memory_order_relaxed);
        float peakL = mixer.channels[ch].peakL.load(std::memory_order_relaxed);
        float peakR = mixer.channels[ch].peakR.load(std::memory_order_relaxed);

        drawSingleBar(g, leftBar, vuL, peakL, cs);
        drawSingleBar(g, rightBar, vuR, peakR, cs);

        // Draw dB scale ticks
        g.setFont(9.0f);
        g.setColour(cs.colours["Text Colour"].withAlpha(0.5f));
        const float dbMarks[] = {0.0f, -6.0f, -12.0f, -24.0f, -48.0f};
        for (float db : dbMarks)
        {
            float norm = jlimit(0.0f, 1.0f, (db + 60.0f) / 72.0f); // -60 to +12 range
            int y = area.getBottom() - static_cast<int>(norm * area.getHeight());
            g.drawHorizontalLine(y, static_cast<float>(area.getX()), static_cast<float>(area.getX() + 3));
            g.drawHorizontalLine(y, static_cast<float>(area.getRight() - 3), static_cast<float>(area.getRight()));
        }

        // Border
        g.setColour(cs.colours["Plugin Border"]);
        g.drawRect(area, 1);
    }

    void drawSingleBar(Graphics& g, Rectangle<int> bar, float vuLevel, float peakLevel, ColourScheme& cs)
    {
        // Convert to dB and normalize to 0-1 range (-60 to +12)
        float vuDb = Decibels::gainToDecibels(vuLevel, -60.0f);
        float norm = jlimit(0.0f, 1.0f, (vuDb + 60.0f) / 72.0f);
        int fillH = static_cast<int>(norm * bar.getHeight());

        // Gradient fill: green -> yellow -> red
        float hFull = static_cast<float>(bar.getHeight());
        float yellowThreshold = 48.0f / 72.0f; // -12 dB
        float redThreshold = 60.0f / 72.0f;    // 0 dB

        for (int y = bar.getBottom() - fillH; y < bar.getBottom(); ++y)
        {
            float frac = 1.0f - static_cast<float>(y - bar.getY()) / hFull;
            Colour barCol;
            if (frac >= redThreshold)
                barCol = cs.colours["VU Meter Over Colour"];
            else if (frac >= yellowThreshold)
                barCol = cs.colours["VU Meter Upper Colour"];
            else
                barCol = cs.colours["VU Meter Lower Colour"];
            g.setColour(barCol);
            g.drawHorizontalLine(y, static_cast<float>(bar.getX()), static_cast<float>(bar.getRight()));
        }

        // Peak hold indicator
        float peakDb = Decibels::gainToDecibels(peakLevel, -60.0f);
        float peakNorm = jlimit(0.0f, 1.0f, (peakDb + 60.0f) / 72.0f);
        if (peakNorm > 0.001f)
        {
            int peakY = bar.getBottom() - static_cast<int>(peakNorm * bar.getHeight());
            g.setColour(Colours::white);
            g.drawHorizontalLine(peakY, static_cast<float>(bar.getX()), static_cast<float>(bar.getRight()));
        }
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MixerEditor)
};

//==============================================================================
// SplitterProcessor Implementation
//==============================================================================

SplitterProcessor::SplitterProcessor()
{
    // 2 inputs (Stereo), 4 outputs (Stereo A, Stereo B)
    setPlayConfigDetails(2, 4, 0, 0);
}

SplitterProcessor::~SplitterProcessor() {}

Component* SplitterProcessor::getControls()
{
    return new SplitterControl(this);
}

void SplitterProcessor::processBlock(AudioBuffer<float>& buffer, MidiBuffer& midiMessages)
{
    // JUCE buffers might resize if number of channels changes?
    // In Pedalboard3 we assume fixed channels usually?
    // Check output channels.
    int numIn = getTotalNumInputChannels();
    int numOut = getTotalNumOutputChannels();

    if (numOut < 4 || numIn < 2)
        return;

    auto* inL = buffer.getReadPointer(0);
    auto* inR = buffer.getReadPointer(1);

    auto* outAL = buffer.getWritePointer(0);
    auto* outAR = buffer.getWritePointer(1);

    auto* outBL = buffer.getWritePointer(2);
    auto* outBR = buffer.getWritePointer(3);

    int numSamples = buffer.getNumSamples();

    bool mA = muteA.load();
    bool mB = muteB.load();

    for (int i = 0; i < numSamples; ++i)
    {
        float l = inL[i];
        float r = inR[i];

        // Path A (Outputs 0, 1)
        if (mA)
        {
            outAL[i] = 0.0f;
            outAR[i] = 0.0f;
        }
        else
        {
            outAL[i] = l;
            outAR[i] = r;
        }

        // Path B (Outputs 2, 3)
        if (mB)
        {
            outBL[i] = 0.0f;
            outBR[i] = 0.0f;
        }
        else
        {
            outBL[i] = l;
            outBR[i] = r;
        }
    }
}

AudioProcessorEditor* SplitterProcessor::createEditor()
{
    return new GenericAudioProcessorEditor(*this);
}

void SplitterProcessor::fillInPluginDescription(PluginDescription& description) const
{
    description.name = "Splitter";
    description.descriptiveName = "Splits stereo input to two stereo pairs (A and B).";
    description.pluginFormatName = "Internal";
    description.category = "Routing";
    description.manufacturerName = "Pedalboard3";
    description.version = "1.00";
    description.uniqueId = description.name.hashCode();
    description.isInstrument = false;
    description.numInputChannels = 2;
    description.numOutputChannels = 4;
}

bool SplitterProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainInputChannels() == 2 && layouts.getMainOutputChannels() == 4)
        return true;
    return false;
}

const String SplitterProcessor::getInputChannelName(int channelIndex) const
{
    if (channelIndex == 0)
        return "Input L";
    if (channelIndex == 1)
        return "Input R";
    return String();
}

const String SplitterProcessor::getOutputChannelName(int channelIndex) const
{
    if (channelIndex == 0)
        return "Out A L";
    if (channelIndex == 1)
        return "Out A R";
    if (channelIndex == 2)
        return "Out B L";
    if (channelIndex == 3)
        return "Out B R";
    return String();
}

void SplitterProcessor::setOutputMute(int outputIndex, bool shouldMute)
{
    if (outputIndex == 0)
        muteA.store(shouldMute);
    else
        muteB.store(shouldMute);
}

bool SplitterProcessor::getOutputMute(int outputIndex) const
{
    if (outputIndex == 0)
        return muteA.load();
    return muteB.load();
}

void SplitterProcessor::getStateInformation(MemoryBlock& destData)
{
    XmlElement xml("SplitterSettings");
    xml.setAttribute("muteA", muteA.load());
    xml.setAttribute("muteB", muteB.load());
    copyXmlToBinary(xml, destData);
}

void SplitterProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState != nullptr && xmlState->hasTagName("SplitterSettings"))
    {
        muteA.store(xmlState->getBoolAttribute("muteA"));
        muteB.store(xmlState->getBoolAttribute("muteB"));
    }
}

//==============================================================================
// MixerProcessor Implementation
//==============================================================================

MixerProcessor::MixerProcessor()
{
    // 4 inputs (Stereo A, Stereo B), 2 outputs (Stereo Mix)
    setPlayConfigDetails(4, 2, 0, 0);
}

MixerProcessor::~MixerProcessor() {}

void MixerProcessor::prepareToPlay(double sampleRate, int /*samplesPerBlock*/)
{
    // Init gain smoothing (50ms multiplicative ramp)
    for (int ch = 0; ch < NumChannels; ++ch)
    {
        smoothedGain[ch].reset(sampleRate, 0.05);
        float gainLin = Decibels::decibelsToGain(channels[ch].gainDb.load(std::memory_order_relaxed));
        smoothedGain[ch].setCurrentAndTargetValue(gainLin);
        // Init VU meters
        channels[ch].vuL.init(static_cast<float>(sampleRate));
        channels[ch].vuR.init(static_cast<float>(sampleRate));
    }
    // Peak meter decay: ~300ms from peak to -60dB
    double samplesFor300ms = sampleRate * 0.3;
    peakDecayCoeff = static_cast<float>(std::pow(0.001, 1.0 / samplesFor300ms));
}

Component* MixerProcessor::getControls()
{
    return nullptr; // Use createEditor() instead
}

void MixerProcessor::processBlock(AudioBuffer<float>& buffer, MidiBuffer& /*midiMessages*/)
{
    int numIn = getTotalNumInputChannels();
    int numOut = getTotalNumOutputChannels();

    if (numIn < 4 || numOut < 2)
        return;

    const int numSamples = buffer.getNumSamples();

    // Read channel inputs
    const float* inAL = buffer.getReadPointer(0);
    const float* inAR = buffer.getReadPointer(1);
    const float* inBL = buffer.getReadPointer(2);
    const float* inBR = buffer.getReadPointer(3);

    float* outL = buffer.getWritePointer(0);
    float* outR = buffer.getWritePointer(1);

    // Read per-channel state atomics (once per block)
    struct ChBlock
    {
        float panL, panR;
        bool mute, phaseInv;
    };
    ChBlock chb[NumChannels];

    // Solo logic: if any channel is soloed, mute all non-soloed channels
    bool anySolo = false;
    for (int ch = 0; ch < NumChannels; ++ch)
    {
        if (channels[ch].solo.load(std::memory_order_relaxed))
            anySolo = true;
    }

    for (int ch = 0; ch < NumChannels; ++ch)
    {
        float pan = channels[ch].pan.load(std::memory_order_relaxed);
        // Equal-power pan law (-3 dB at center)
        chb[ch].panL = std::sqrt(0.5f * (1.0f - pan));
        chb[ch].panR = std::sqrt(0.5f * (1.0f + pan));
        chb[ch].mute = channels[ch].mute.load(std::memory_order_relaxed);
        chb[ch].phaseInv = channels[ch].phaseInvert.load(std::memory_order_relaxed);

        // If any channel is soloed and this one isn't, treat as muted
        if (anySolo && !channels[ch].solo.load(std::memory_order_relaxed))
            chb[ch].mute = true;

        // Update smoothed gain target
        float gainLin = Decibels::decibelsToGain(channels[ch].gainDb.load(std::memory_order_relaxed));
        smoothedGain[ch].setTargetValue(gainLin);
    }

    // Temp buffers for VU metering (per channel, L and R post-processing)
    float vuBufA_L[8192], vuBufA_R[8192];
    float vuBufB_L[8192], vuBufB_R[8192];
    jassert(numSamples <= 8192);

    for (int i = 0; i < numSamples; ++i)
    {
        float sumL = 0.0f, sumR = 0.0f;

        // Channel A (inputs 0, 1)
        {
            float gain = smoothedGain[0].getNextValue();
            float l = inAL[i];
            float r = inAR[i];
            if (chb[0].phaseInv)
            {
                l = -l;
                r = -r;
            }
            l *= gain;
            r *= gain;
            // Store for VU metering before mute (so VU shows signal even when muted)
            vuBufA_L[i] = l;
            vuBufA_R[i] = r;
            if (!chb[0].mute)
            {
                sumL += l * chb[0].panL;
                sumR += r * chb[0].panR;
            }
        }

        // Channel B (inputs 2, 3)
        {
            float gain = smoothedGain[1].getNextValue();
            float l = inBL[i];
            float r = inBR[i];
            if (chb[1].phaseInv)
            {
                l = -l;
                r = -r;
            }
            l *= gain;
            r *= gain;
            vuBufB_L[i] = l;
            vuBufB_R[i] = r;
            if (!chb[1].mute)
            {
                sumL += l * chb[1].panL;
                sumR += r * chb[1].panR;
            }
        }

        outL[i] = sumL;
        outR[i] = sumR;
    }

    // Feed VU meters (post-gain, pre-mute — shows signal level regardless of mute)
    channels[0].vuL.process(vuBufA_L, numSamples);
    channels[0].vuR.process(vuBufA_R, numSamples);
    channels[0].vuLevelL.store(channels[0].vuL.read(), std::memory_order_relaxed);
    channels[0].vuLevelR.store(channels[0].vuR.read(), std::memory_order_relaxed);

    channels[1].vuL.process(vuBufB_L, numSamples);
    channels[1].vuR.process(vuBufB_R, numSamples);
    channels[1].vuLevelL.store(channels[1].vuL.read(), std::memory_order_relaxed);
    channels[1].vuLevelR.store(channels[1].vuR.read(), std::memory_order_relaxed);

    // Peak metering with decay
    for (int ch = 0; ch < NumChannels; ++ch)
    {
        const float* vuL = (ch == 0) ? vuBufA_L : vuBufB_L;
        const float* vuR = (ch == 0) ? vuBufA_R : vuBufB_R;
        float peakL = channels[ch].peakL.load(std::memory_order_relaxed);
        float peakR = channels[ch].peakR.load(std::memory_order_relaxed);
        for (int i = 0; i < numSamples; ++i)
        {
            float absL = std::abs(vuL[i]);
            float absR = std::abs(vuR[i]);
            peakL = (absL > peakL) ? absL : peakL * peakDecayCoeff;
            peakR = (absR > peakR) ? absR : peakR * peakDecayCoeff;
        }
        if (peakL < 1e-10f)
            peakL = 0.0f;
        if (peakR < 1e-10f)
            peakR = 0.0f;
        channels[ch].peakL.store(peakL, std::memory_order_relaxed);
        channels[ch].peakR.store(peakR, std::memory_order_relaxed);
    }
}

AudioProcessorEditor* MixerProcessor::createEditor()
{
    return new MixerEditor(*this);
}

void MixerProcessor::fillInPluginDescription(PluginDescription& description) const
{
    description.name = "Mixer";
    description.descriptiveName = "Mixes two stereo pairs (A and B) to stereo with gain, pan, mute/solo.";
    description.pluginFormatName = "Internal";
    description.category = "Routing";
    description.manufacturerName = "Pedalboard3";
    description.version = "2.00";
    description.uniqueId = description.name.hashCode();
    description.isInstrument = false;
    description.numInputChannels = 4;
    description.numOutputChannels = 2;
}

bool MixerProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainInputChannels() == 4 && layouts.getMainOutputChannels() == 2)
        return true;
    return false;
}

const String MixerProcessor::getInputChannelName(int channelIndex) const
{
    if (channelIndex == 0)
        return "In A L";
    if (channelIndex == 1)
        return "In A R";
    if (channelIndex == 2)
        return "In B L";
    if (channelIndex == 3)
        return "In B R";
    return String();
}

const String MixerProcessor::getOutputChannelName(int channelIndex) const
{
    if (channelIndex == 0)
        return "Output L";
    if (channelIndex == 1)
        return "Output R";
    return String();
}

//==============================================================================
// Parameter interface (for MIDI mapping compatibility)

float MixerProcessor::getParameter(int parameterIndex)
{
    switch (parameterIndex)
    {
    case ParamGainA:
        return jmap(channels[0].gainDb.load(std::memory_order_relaxed), -60.0f, 12.0f, 0.0f, 1.0f);
    case ParamGainB:
        return jmap(channels[1].gainDb.load(std::memory_order_relaxed), -60.0f, 12.0f, 0.0f, 1.0f);
    case ParamPanA:
        return jmap(channels[0].pan.load(std::memory_order_relaxed), -1.0f, 1.0f, 0.0f, 1.0f);
    case ParamPanB:
        return jmap(channels[1].pan.load(std::memory_order_relaxed), -1.0f, 1.0f, 0.0f, 1.0f);
    default:
        return 0.0f;
    }
}

void MixerProcessor::setParameter(int parameterIndex, float newValue)
{
    switch (parameterIndex)
    {
    case ParamGainA:
        channels[0].gainDb.store(jmap(newValue, 0.0f, 1.0f, -60.0f, 12.0f), std::memory_order_relaxed);
        break;
    case ParamGainB:
        channels[1].gainDb.store(jmap(newValue, 0.0f, 1.0f, -60.0f, 12.0f), std::memory_order_relaxed);
        break;
    case ParamPanA:
        channels[0].pan.store(jmap(newValue, 0.0f, 1.0f, -1.0f, 1.0f), std::memory_order_relaxed);
        break;
    case ParamPanB:
        channels[1].pan.store(jmap(newValue, 0.0f, 1.0f, -1.0f, 1.0f), std::memory_order_relaxed);
        break;
    default:
        break;
    }
}

const String MixerProcessor::getParameterName(int parameterIndex)
{
    switch (parameterIndex)
    {
    case ParamGainA:
        return "Gain A";
    case ParamGainB:
        return "Gain B";
    case ParamPanA:
        return "Pan A";
    case ParamPanB:
        return "Pan B";
    default:
        return "";
    }
}

const String MixerProcessor::getParameterText(int parameterIndex)
{
    switch (parameterIndex)
    {
    case ParamGainA:
        return String(channels[0].gainDb.load(std::memory_order_relaxed), 1) + " dB";
    case ParamGainB:
        return String(channels[1].gainDb.load(std::memory_order_relaxed), 1) + " dB";
    case ParamPanA:
        return String(channels[0].pan.load(std::memory_order_relaxed), 2);
    case ParamPanB:
        return String(channels[1].pan.load(std::memory_order_relaxed), 2);
    default:
        return "";
    }
}

//==============================================================================
// State serialization (backward-compatible)

void MixerProcessor::getStateInformation(MemoryBlock& destData)
{
    XmlElement xml("MixerSettings");
    xml.setAttribute("version", 2);
    for (int ch = 0; ch < NumChannels; ++ch)
    {
        String prefix = (ch == 0) ? "A" : "B";
        xml.setAttribute("gain" + prefix, static_cast<double>(channels[ch].gainDb.load(std::memory_order_relaxed)));
        xml.setAttribute("pan" + prefix, static_cast<double>(channels[ch].pan.load(std::memory_order_relaxed)));
        xml.setAttribute("mute" + prefix, channels[ch].mute.load(std::memory_order_relaxed));
        xml.setAttribute("solo" + prefix, channels[ch].solo.load(std::memory_order_relaxed));
        xml.setAttribute("phase" + prefix, channels[ch].phaseInvert.load(std::memory_order_relaxed));
    }
    copyXmlToBinary(xml, destData);
}

void MixerProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState == nullptr || !xmlState->hasTagName("MixerSettings"))
        return;

    int version = xmlState->getIntAttribute("version", 1);

    if (version >= 2)
    {
        // New format: per-channel gainDb, pan, mute, solo, phaseInvert
        for (int ch = 0; ch < NumChannels; ++ch)
        {
            String prefix = (ch == 0) ? "A" : "B";
            channels[ch].gainDb.store(static_cast<float>(xmlState->getDoubleAttribute("gain" + prefix, 0.0)),
                                      std::memory_order_relaxed);
            channels[ch].pan.store(static_cast<float>(xmlState->getDoubleAttribute("pan" + prefix, 0.0)),
                                   std::memory_order_relaxed);
            channels[ch].mute.store(xmlState->getBoolAttribute("mute" + prefix, false), std::memory_order_relaxed);
            channels[ch].solo.store(xmlState->getBoolAttribute("solo" + prefix, false), std::memory_order_relaxed);
            channels[ch].phaseInvert.store(xmlState->getBoolAttribute("phase" + prefix, false),
                                           std::memory_order_relaxed);
        }
    }
    else
    {
        // Legacy format: convert linear levelA/levelB to dB
        float levelA = static_cast<float>(xmlState->getDoubleAttribute("levelA", 0.707));
        float levelB = static_cast<float>(xmlState->getDoubleAttribute("levelB", 0.707));
        channels[0].gainDb.store(Decibels::gainToDecibels(levelA, -60.0f), std::memory_order_relaxed);
        channels[1].gainDb.store(Decibels::gainToDecibels(levelB, -60.0f), std::memory_order_relaxed);
    }
}
