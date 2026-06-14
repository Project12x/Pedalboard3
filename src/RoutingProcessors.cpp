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

static Colour getRoutingNodeAccent()
{
    return ColourScheme::getInstance().colours["Graph Category Dynamics"];
}

static String getRoutingVisualLabel(int index)
{
    return String(index + 1);
}

static void paintRoutingNodeShell(Graphics& g, Rectangle<float> bounds, Colour accent, const String& title)
{
    auto& colours = ColourScheme::getInstance().colours;
    const auto outer = bounds;
    const auto base = colours["Plugin Background"];

    ColourGradient body(base.brighter(0.10f), bounds.getX(), bounds.getY(), base.darker(0.18f), bounds.getX(),
                        bounds.getBottom(), false);
    body.addColour(0.36, base.brighter(0.02f));
    body.addColour(0.78, base.darker(0.08f));
    g.setGradientFill(body);
    g.fillRoundedRectangle(outer.reduced(0.5f), 8.0f);

    auto header = bounds.removeFromTop(25.0f).reduced(5.0f, 4.0f);
    ColourGradient headerFill(accent.withAlpha(0.20f), header.getX(), header.getY(), base.darker(0.06f),
                              header.getX(), header.getBottom(), false);
    headerFill.addColour(0.48, base.interpolatedWith(accent, 0.10f));
    g.setGradientFill(headerFill);
    g.fillRoundedRectangle(header, 6.0f);

    g.setColour(accent.withAlpha(0.48f));
    g.drawRoundedRectangle(outer.reduced(0.5f), 8.0f, 1.15f);
    g.setColour(Colours::white.withAlpha(0.06f));
    g.drawRoundedRectangle(outer.reduced(1.5f), 7.0f, 0.8f);

    if (title.isNotEmpty())
    {
        g.setColour(accent.withAlpha(0.88f));
        g.setFont(Font(11.0f, Font::bold));
        g.drawText(title, header.reduced(8.0f, 0.0f).toNearestInt(), Justification::centredLeft, false);
    }
}

static void paintRoutingBadge(Graphics& g, Rectangle<float> bounds, const String& text, Colour accent, bool primary)
{
    auto& colours = ColourScheme::getInstance().colours;
    const auto base = primary ? accent.withAlpha(0.20f) : colours["Plugin Background"].brighter(0.08f);
    ColourGradient fill(base.brighter(0.12f), bounds.getX(), bounds.getY(), base.darker(0.24f), bounds.getX(),
                        bounds.getBottom(), false);
    fill.addColour(0.50, base);
    g.setGradientFill(fill);
    g.fillRoundedRectangle(bounds, 5.0f);

    g.setColour(accent.withAlpha(primary ? 0.72f : 0.38f));
    g.drawRoundedRectangle(bounds.reduced(0.5f), 5.0f, 1.0f);

    g.setColour((primary ? accent : colours["Text Colour"]).withAlpha(primary ? 0.96f : 0.70f));
    g.setFont(Font(10.5f, Font::bold));
    g.drawText(text, bounds.toNearestInt(), Justification::centred, false);
}

static void paintRoutingMeterTrack(Graphics& g, Rectangle<float> bounds, float level, Colour accent, bool muted)
{
    if (bounds.isEmpty())
        return;

    auto& colours = ColourScheme::getInstance().colours;
    const auto track = bounds.reduced(0.0f, juce::jmax(0.0f, (bounds.getHeight() - 5.0f) * 0.5f));

    g.setColour(colours["Plugin Background"].darker(0.34f));
    g.fillRoundedRectangle(track, 3.0f);

    auto fill = track.withWidth(track.getWidth() * jlimit(0.0f, 1.0f, level));
    ColourGradient fillGradient(accent.withAlpha(muted ? 0.18f : 0.72f), fill.getX(), fill.getY(),
                                colours["VU Meter Upper Colour"].withAlpha(muted ? 0.10f : 0.56f),
                                fill.getRight(), fill.getY(), false);
    fillGradient.addColour(0.70, colours["VU Meter Lower Colour"].withAlpha(muted ? 0.12f : 0.64f));
    g.setGradientFill(fillGradient);
    g.fillRoundedRectangle(fill, 3.0f);

    g.setColour(accent.withAlpha(muted ? 0.14f : 0.28f));
    g.drawRoundedRectangle(track.reduced(0.5f), 3.0f, 0.8f);
}

static void paintRoutingFanout(Graphics& g, Rectangle<float> bounds, Colour accent, bool muteA, bool muteB)
{
    if (bounds.isEmpty())
        return;

    auto& colours = ColourScheme::getInstance().colours;
    auto well = bounds.reduced(6.0f, 2.0f);
    g.setColour(colours["Plugin Background"].darker(0.22f));
    g.fillRoundedRectangle(well, 6.0f);
    g.setColour(accent.withAlpha(0.16f));
    g.drawRoundedRectangle(well.reduced(0.5f), 6.0f, 0.8f);

    const float startX = bounds.getX() + 30.0f;
    const float startY = bounds.getCentreY();
    const float endX = bounds.getRight() - 30.0f;
    const float aY = bounds.getY() + 7.0f;
    const float bY = bounds.getBottom() - 7.0f;

    auto drawRoute = [&](float endY, bool muted)
    {
        Path route;
        route.startNewSubPath(startX, startY);
        route.cubicTo(startX + 52.0f, startY, endX - 62.0f, endY, endX, endY);
        g.setColour(accent.withAlpha(muted ? 0.16f : 0.42f));
        g.strokePath(route, PathStrokeType(muted ? 1.1f : 1.8f, PathStrokeType::curved, PathStrokeType::rounded));
    };

    drawRoute(aY, muteA);
    drawRoute(bY, muteB);

    g.setColour(accent.withAlpha(0.78f));
    g.fillEllipse(startX - 3.0f, startY - 3.0f, 6.0f, 6.0f);
}

static void paintMixerStripDeck(Graphics& g, Rectangle<float> bounds, Colour accent, const String& label)
{
    auto& colours = ColourScheme::getInstance().colours;
    ColourGradient deck(colours["Plugin Background"].brighter(0.07f), bounds.getX(), bounds.getY(),
                        colours["Plugin Background"].darker(0.15f), bounds.getX(), bounds.getBottom(), false);
    deck.addColour(0.54, colours["Plugin Background"].darker(0.03f));
    g.setGradientFill(deck);
    g.fillRoundedRectangle(bounds, 7.0f);

    g.setColour(accent.withAlpha(0.38f));
    g.drawRoundedRectangle(bounds.reduced(0.5f), 7.0f, 1.0f);
    g.setColour(Colours::white.withAlpha(0.045f));
    g.drawLine(bounds.getX() + 7.0f, bounds.getY() + 1.0f, bounds.getRight() - 7.0f, bounds.getY() + 1.0f);

    ignoreUnused(label);
}

static void paintMixerPanRail(Graphics& g, Rectangle<float> bounds, float pan, Colour accent)
{
    auto& colours = ColourScheme::getInstance().colours;
    if (bounds.isEmpty())
        return;

    g.setColour(colours["Plugin Background"].darker(0.34f));
    g.fillRoundedRectangle(bounds, 3.0f);
    g.setColour(accent.withAlpha(0.26f));
    g.drawRoundedRectangle(bounds.reduced(0.5f), 3.0f, 0.8f);

    const float dotX = jmap(jlimit(-1.0f, 1.0f, pan), -1.0f, 1.0f, bounds.getX() + 4.0f, bounds.getRight() - 4.0f);
    auto dot = Rectangle<float>(dotX - 3.0f, bounds.getCentreY() - 3.0f, 6.0f, 6.0f);
    g.setColour(accent.withAlpha(0.26f));
    g.fillEllipse(dot.expanded(3.0f));
    g.setColour(accent);
    g.fillEllipse(dot);
}

static void styleRoutingButton(TextButton& button, Colour accent)
{
    auto& colours = ColourScheme::getInstance().colours;
    button.setColour(TextButton::buttonColourId, colours["Plugin Background"].brighter(0.06f));
    button.setColour(TextButton::buttonOnColourId, accent.withAlpha(0.82f));
    button.setColour(TextButton::textColourOffId, colours["Text Colour"].withAlpha(0.75f));
    button.setColour(TextButton::textColourOnId, Colours::black.withAlpha(0.90f));
}

//==============================================================================
// Controls for Splitter
class SplitterControl : public Component, public Button::Listener
{
  public:
    SplitterControl(SplitterProcessor* proc) : processor(proc)
    {
        addAndMakeVisible(muteA);
        muteA.setButtonText("1");
        muteA.setClickingTogglesState(true);
        muteA.setColour(TextButton::buttonOnColourId, ColourScheme::getInstance().colours["Danger Colour"]);
        styleRoutingButton(muteA, getRoutingNodeAccent());
        muteA.addListener(this);

        addAndMakeVisible(muteB);
        muteB.setButtonText("2");
        muteB.setClickingTogglesState(true);
        muteB.setColour(TextButton::buttonOnColourId, ColourScheme::getInstance().colours["Danger Colour"]);
        styleRoutingButton(muteB, getRoutingNodeAccent());
        muteB.addListener(this);

        // Update state
        muteA.setToggleState(processor->getOutputMute(0), dontSendNotification);
        muteB.setToggleState(processor->getOutputMute(1), dontSendNotification);
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced(10, 9);
        area.removeFromTop(28);

        auto inputRow = area.removeFromTop(28);
        inBadge = inputRow.removeFromLeft(36).reduced(1, 4);
        inputMeter = inputRow.reduced(8, 11);

        fanoutArea = area.removeFromTop(34);

        auto outputA = area.removeFromTop(28);
        outBadgeA = outputA.removeFromLeft(28).reduced(1, 4);
        muteA.setBounds(outputA.removeFromRight(42).reduced(2, 3));
        laneA = outputA.reduced(8, 8);

        area.removeFromTop(4);
        auto outputB = area.removeFromTop(28);
        outBadgeB = outputB.removeFromLeft(28).reduced(1, 4);
        muteB.setBounds(outputB.removeFromRight(42).reduced(2, 3));
        laneB = outputB.reduced(8, 8);
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
        paintRoutingNodeShell(g, getLocalBounds().toFloat(), getRoutingNodeAccent(), {});

        auto accent = getRoutingNodeAccent();
        paintRoutingBadge(g, inBadge.toFloat(), "IN", accent, true);
        paintRoutingFanout(g, fanoutArea.toFloat(), getRoutingNodeAccent(), muteA.getToggleState(), muteB.getToggleState());
        paintRoutingBadge(g, outBadgeA.toFloat(), getRoutingVisualLabel(0), accent, !muteA.getToggleState());
        paintRoutingBadge(g, outBadgeB.toFloat(), getRoutingVisualLabel(1), accent, !muteB.getToggleState());

        paintRoutingMeterTrack(g, inputMeter.toFloat(), 0.62f, accent, false);
        paintRoutingMeterTrack(g, laneA.toFloat(), 0.72f, accent, muteA.getToggleState());
        paintRoutingMeterTrack(g, laneB.toFloat(), 0.38f, accent, muteB.getToggleState());
    }

  private:
    SplitterProcessor* processor;
    TextButton muteA;
    TextButton muteB;
    Rectangle<int> inBadge;
    Rectangle<int> outBadgeA;
    Rectangle<int> outBadgeB;
    Rectangle<int> inputMeter;
    Rectangle<int> laneA;
    Rectangle<int> laneB;
    Rectangle<int> fanoutArea;
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
    // Full channel strip control embedded in the node panel
    class MixerControl : public Component, private Timer
    {
      public:
        MixerControl(MixerProcessor* proc) : processor(proc)
        {
            for (int ch = 0; ch < MixerProcessor::NumChannels; ++ch)
            {
                // Gain fader (vertical, dB scale)
                auto& f = faders[ch];
                f.setSliderStyle(Slider::LinearVertical);
                f.setTextBoxStyle(Slider::TextBoxBelow, false, 50, 16);
                f.setRange(-60.0, 12.0, 0.1);
                f.setValue(processor->getChannelGainDb(ch), dontSendNotification);
                f.onValueChange = [this, ch]()
                { processor->setChannelGainDb(ch, static_cast<float>(faders[ch].getValue())); };
                addAndMakeVisible(f);

                // Pan knob (rotary)
                auto& p = panKnobs[ch];
                p.setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
                p.setTextBoxStyle(Slider::NoTextBox, false, 0, 0);
                p.setRange(-1.0, 1.0, 0.01);
                p.setDoubleClickReturnValue(true, 0.0);
                p.setValue(processor->getChannelPan(ch), dontSendNotification);
                p.onValueChange = [this, ch]()
                { processor->setChannelPan(ch, static_cast<float>(panKnobs[ch].getValue())); };
                addAndMakeVisible(p);

                // Mute button
                auto& m = muteButtons[ch];
                m.setButtonText("M");
                m.setClickingTogglesState(true);
                m.setColour(TextButton::buttonOnColourId, ColourScheme::getInstance().colours["Danger Colour"]);
                styleRoutingButton(m, ColourScheme::getInstance().colours["Danger Colour"]);
                m.setToggleState(processor->getChannelMute(ch), dontSendNotification);
                m.onClick = [this, ch]() { processor->setChannelMute(ch, muteButtons[ch].getToggleState()); };
                addAndMakeVisible(m);

                // Solo button
                auto& s = soloButtons[ch];
                s.setButtonText("S");
                s.setClickingTogglesState(true);
                s.setColour(TextButton::buttonOnColourId, Colour(0xFFCCAA00));
                styleRoutingButton(s, Colour(0xFFCCAA00));
                s.setToggleState(processor->getChannelSolo(ch), dontSendNotification);
                s.onClick = [this, ch]() { processor->setChannelSolo(ch, soloButtons[ch].getToggleState()); };
                addAndMakeVisible(s);

                // Phase invert button
                auto& ph = phaseButtons[ch];
                ph.setButtonText(CharPointer_UTF8("\xc3\x98")); // O-slash as phase symbol
                ph.setClickingTogglesState(true);
                ph.setColour(TextButton::buttonOnColourId, Colour(0xFFFF8800));
                styleRoutingButton(ph, Colour(0xFFFF8800));
                ph.setToggleState(processor->getChannelPhaseInvert(ch), dontSendNotification);
                ph.onClick = [this, ch]() { processor->setChannelPhaseInvert(ch, phaseButtons[ch].getToggleState()); };
                addAndMakeVisible(ph);
            }

            startTimerHz(30);
        }

        ~MixerControl() override { stopTimer(); }

        void resized() override
        {
            auto area = getLocalBounds().reduced(8, 8);
            area.removeFromTop(28);
            int stripW = area.getWidth() / 2;

            for (int ch = 0; ch < MixerProcessor::NumChannels; ++ch)
            {
                auto strip = area.removeFromLeft(stripW).reduced(4, 0);
                stripDecks[ch] = strip;

                auto top = strip.removeFromTop(22);
                badgeAreas[ch] = top.removeFromLeft(26).reduced(1, 3);
                phaseButtons[ch].setBounds(top.removeFromRight(28).reduced(2, 2));
                strip.removeFromTop(2);

                vuAreas[ch] = strip.removeFromTop(54).reduced(12, 2);
                strip.removeFromTop(2);

                faders[ch].setBounds(strip.removeFromTop(88).reduced(6, 0));
                strip.removeFromTop(2);

                panRails[ch] = strip.removeFromTop(12).reduced(12, 3);
                strip.removeFromTop(2);
                panKnobs[ch].setBounds(strip.removeFromTop(34).withSizeKeepingCentre(34, 34));

                strip.removeFromTop(2);
                auto btnRow = strip.removeFromTop(24);
                int btnW = btnRow.getWidth() / 2;
                muteButtons[ch].setBounds(btnRow.removeFromLeft(btnW).reduced(2, 0));
                soloButtons[ch].setBounds(btnRow.reduced(2, 0));
            }
        }

        void timerCallback() override { repaint(); }

        void paint(Graphics& g) override
        {
            auto& cs = ColourScheme::getInstance();
            const auto accent = getRoutingNodeAccent();
            paintRoutingNodeShell(g, getLocalBounds().toFloat(), getRoutingNodeAccent(), {});

            for (int ch = 0; ch < MixerProcessor::NumChannels; ++ch)
            {
                paintMixerStripDeck(g, stripDecks[ch].toFloat(), accent, getRoutingVisualLabel(ch));
                auto badge = badgeAreas[ch];
                paintRoutingBadge(g, badge.toFloat(), getRoutingVisualLabel(ch), accent, true);
                paintMixerPanRail(g, panRails[ch].toFloat(), static_cast<float>(panKnobs[ch].getValue()), accent);
                drawVuMeter(g, ch, cs);
            }
        }

      private:
        MixerProcessor* processor;
        Slider faders[MixerProcessor::NumChannels];
        Slider panKnobs[MixerProcessor::NumChannels];
        TextButton muteButtons[MixerProcessor::NumChannels];
        TextButton soloButtons[MixerProcessor::NumChannels];
        TextButton phaseButtons[MixerProcessor::NumChannels];
        Rectangle<int> vuAreas[MixerProcessor::NumChannels];
        Rectangle<int> stripDecks[MixerProcessor::NumChannels];
        Rectangle<int> badgeAreas[MixerProcessor::NumChannels];
        Rectangle<int> panRails[MixerProcessor::NumChannels];

        void drawVuMeter(Graphics& g, int ch, ColourScheme& cs)
        {
            auto area = vuAreas[ch];
            if (area.isEmpty())
                return;

            g.setColour(cs.colours["Plugin Background"].darker(0.35f));
            g.fillRoundedRectangle(area.toFloat(), 4.0f);

            // Draw L and R bars
            int barW = (area.getWidth() - 6) / 2;
            auto leftBar = area.withWidth(barW).translated(2, 0).reduced(0, 2);
            auto rightBar = leftBar.translated(barW + 2, 0);

            float vuL = processor->channels[ch].vuLevelL.load(std::memory_order_relaxed);
            float vuR = processor->channels[ch].vuLevelR.load(std::memory_order_relaxed);
            float peakL = processor->channels[ch].peakL.load(std::memory_order_relaxed);
            float peakR = processor->channels[ch].peakR.load(std::memory_order_relaxed);

            drawSingleBar(g, leftBar, vuL, peakL, cs);
            drawSingleBar(g, rightBar, vuR, peakR, cs);

            // Draw dB scale ticks
            g.setFont(9.0f);
            g.setColour(cs.colours["Text Colour"].withAlpha(0.5f));
            const float dbMarks[] = {0.0f, -6.0f, -12.0f, -24.0f, -48.0f};
            for (float db : dbMarks)
            {
                float norm = jlimit(0.0f, 1.0f, (db + 60.0f) / 72.0f);
                int y = area.getBottom() - static_cast<int>(norm * area.getHeight());
                g.drawHorizontalLine(y, static_cast<float>(area.getX()), static_cast<float>(area.getX() + 3));
                g.drawHorizontalLine(y, static_cast<float>(area.getRight() - 3), static_cast<float>(area.getRight()));
            }

            g.setColour(getRoutingNodeAccent().withAlpha(0.24f));
            g.drawRoundedRectangle(area.toFloat().reduced(0.5f), 4.0f, 0.9f);
        }

        void drawSingleBar(Graphics& g, Rectangle<int> bar, float vuLevel, float peakLevel, ColourScheme& cs)
        {
            float vuDb = Decibels::gainToDecibels(vuLevel, -60.0f);
            float norm = jlimit(0.0f, 1.0f, (vuDb + 60.0f) / 72.0f);
            int fillH = static_cast<int>(norm * bar.getHeight());

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

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MixerControl)
    };

    return new MixerControl(this);
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
    return nullptr; // Uses getControls() instead
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
