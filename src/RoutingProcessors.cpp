/*
  ==============================================================================

    RoutingProcessors.cpp
    Created: 27 Jan 2026

    Processors for routing (Splitter and Mixer).

  ==============================================================================
*/

#include "RoutingProcessors.h"

#include "ColourScheme.h"
#include "PluginComponent.h"

#include <cmath>

static Colour getRoutingNodeAccent()
{
    return ColourScheme::getInstance().colours["Graph Category Dynamics"];
}

static String getRoutingVisualLabel(int index)
{
    return String(index + 1);
}

static float normaliseRoutingMeter(float peak)
{
    const float dbVal = Decibels::gainToDecibels(jmax(0.0f, peak), -60.0f);
    return jlimit(0.0f, 1.0f, (dbVal + 60.0f) / 72.0f);
}

static constexpr int kMixerNodeWidth = 320;
static constexpr int kMixerStripRowHeight = 52;
static constexpr int kMixerMasterRowHeight = 54;

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
                                colours["VU Meter Upper Colour"].withAlpha(muted ? 0.10f : 0.56f), fill.getRight(),
                                fill.getY(), false);
    fillGradient.addColour(0.70, colours["VU Meter Lower Colour"].withAlpha(muted ? 0.12f : 0.64f));
    g.setGradientFill(fillGradient);
    g.fillRoundedRectangle(fill, 3.0f);

    g.setColour(accent.withAlpha(muted ? 0.14f : 0.28f));
    g.drawRoundedRectangle(track.reduced(0.5f), 3.0f, 0.8f);
}

static void paintRoutingRow(Graphics& g, Rectangle<float> bounds, Colour accent, bool muted, bool input)
{
    if (bounds.isEmpty())
        return;

    auto& colours = ColourScheme::getInstance().colours;
    const auto base = input ? colours["Plugin Background"].brighter(0.08f)
                            : colours["Plugin Background"].brighter(muted ? 0.00f : 0.04f);
    ColourGradient fill(base.brighter(input ? 0.10f : 0.05f), bounds.getX(), bounds.getY(),
                        base.darker(input ? 0.18f : 0.12f), bounds.getX(), bounds.getBottom(), false);
    fill.addColour(0.50, base);
    g.setGradientFill(fill);
    g.fillRoundedRectangle(bounds, 7.0f);

    g.setColour((input ? accent : colours["Plugin Border"].interpolatedWith(accent, muted ? 0.08f : 0.24f))
                    .withAlpha(input ? 0.40f : 0.34f));
    g.drawRoundedRectangle(bounds.reduced(0.5f), 7.0f, 0.9f);
}

static void paintRoutingFanout(Graphics& g, Rectangle<float> bounds, Colour accent, bool muteA, bool muteB,
                               int routeCount = 2)
{
    if (bounds.isEmpty())
        return;

    const float startX = bounds.getX() + 30.0f;
    const float startY = bounds.getCentreY();
    const float endX = bounds.getRight() - 30.0f;
    const int routes = jmax(1, routeCount);

    for (int i = 0; i < routes; ++i)
    {
        const float t = routes == 1 ? 0.5f : static_cast<float>(i) / static_cast<float>(routes - 1);
        const float endY = jmap(t, bounds.getY() + 5.0f, bounds.getBottom() - 5.0f);
        const bool muted = i < 2 ? muteA : muteB;

        Path route;
        route.startNewSubPath(startX, startY);
        route.cubicTo(startX + 52.0f, startY, endX - 62.0f, endY, endX, endY);
        g.setColour(accent.withAlpha(muted ? 0.16f : 0.42f));
        g.strokePath(route, PathStrokeType(muted ? 1.1f : 1.8f, PathStrokeType::curved, PathStrokeType::rounded));
    }

    g.setColour(accent.withAlpha(0.78f));
    g.fillEllipse(startX - 3.0f, startY - 3.0f, 6.0f, 6.0f);
}

static void paintMixerStripDeck(Graphics& g, Rectangle<float> bounds, Colour accent, const String& label)
{
    if (bounds.isEmpty())
        return;

    auto& colours = ColourScheme::getInstance().colours;
    const auto deck = bounds.reduced(0.5f, 0.0f);
    ColourGradient fill(colours["Plugin Background"].brighter(0.025f), deck.getX(), deck.getY(),
                        colours["Plugin Background"].darker(0.08f), deck.getX(), deck.getBottom(), false);
    fill.addColour(0.46, colours["Plugin Background"].brighter(0.005f));
    g.setGradientFill(fill);
    g.fillRect(deck);

    g.setColour(colours["Plugin Border"].interpolatedWith(accent, 0.30f).withAlpha(0.48f));
    g.drawVerticalLine(roundToInt(bounds.getRight()), bounds.getY() + 9.0f, bounds.getBottom() - 9.0f);
    g.setColour(Colours::white.withAlpha(0.045f));
    g.drawLine(bounds.getX() + 7.0f, bounds.getY() + 1.0f, bounds.getRight() - 7.0f, bounds.getY() + 1.0f, 0.7f);

    ignoreUnused(label);
}

static void paintMixerMasterDeck(Graphics& g, Rectangle<float> bounds, Colour accent)
{
    auto& colours = ColourScheme::getInstance().colours;
    const auto deck = bounds.reduced(1.0f, 0.0f);
    ColourGradient fill(colours["Plugin Background"].interpolatedWith(accent, 0.08f).brighter(0.03f), deck.getX(),
                        deck.getY(), colours["Plugin Background"].interpolatedWith(accent, 0.10f).darker(0.10f),
                        deck.getX(), deck.getBottom(), false);
    fill.addColour(0.52, colours["Plugin Background"].interpolatedWith(accent, 0.06f));
    g.setGradientFill(fill);
    g.fillRect(deck);

    g.setColour(colours["Plugin Border"].interpolatedWith(accent, 0.52f).withAlpha(0.72f));
    g.drawVerticalLine(roundToInt(deck.getX()), deck.getY() + 7.0f, deck.getBottom() - 7.0f);
    g.setColour(Colours::white.withAlpha(0.055f));
    g.drawLine(deck.getX() + 7.0f, deck.getY() + 1.0f, deck.getRight() - 7.0f, deck.getY() + 1.0f, 0.7f);
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

static void notifyParentPins(Component& component)
{
    if (auto* pc = component.findParentComponentOfClass<PluginComponent>())
        pc->refreshPins();
}

//==============================================================================
// Controls for Splitter
class SplitterControl : public Component, public Button::Listener, private Timer
{
  public:
    SplitterControl(SplitterProcessor* proc) : processor(proc)
    {
        addStripButton.setButtonText("+");
        addStripButton.addListener(this);
        styleRoutingButton(addStripButton, getRoutingNodeAccent());
        addAndMakeVisible(addStripButton);

        removeStripButton.setButtonText("-");
        removeStripButton.addListener(this);
        styleRoutingButton(removeStripButton, getRoutingNodeAccent());
        addAndMakeVisible(removeStripButton);

        for (int index = 0; index < SplitterProcessor::MaxStrips; ++index)
        {
            auto& f = faders[index];
            f.setSliderStyle(Slider::LinearHorizontal);
            f.setTextBoxStyle(Slider::NoTextBox, false, 0, 0);
            f.setRange(SplitterProcessor::MinGainDb, SplitterProcessor::MaxGainDb, 0.1);
            f.onValueChange = [this, index]()
            { processor->setOutputGainDb(index, static_cast<float>(faders[index].getValue())); };
            f.setAlpha(0.01f);
            addAndMakeVisible(f);

            auto& p = panSliders[index];
            p.setSliderStyle(Slider::LinearHorizontal);
            p.setTextBoxStyle(Slider::NoTextBox, false, 0, 0);
            p.setRange(-1.0, 1.0, 0.01);
            p.setDoubleClickReturnValue(true, 0.0);
            p.onValueChange = [this, index]()
            { processor->setOutputPan(index, static_cast<float>(panSliders[index].getValue())); };
            p.setAlpha(0.01f);
            addAndMakeVisible(p);

            muteButtons[index].setButtonText("M");
            auto& m = muteButtons[index];
            m.setClickingTogglesState(true);
            styleRoutingButton(m, ColourScheme::getInstance().colours["Danger Colour"]);
            m.onClick = [this, index]() { processor->setOutputMute(index, muteButtons[index].getToggleState()); };
            addAndMakeVisible(m);

            auto& s = soloButtons[index];
            s.setButtonText("S");
            s.setClickingTogglesState(true);
            styleRoutingButton(s, Colour(0xFFCCAA00));
            s.onClick = [this, index]() { processor->setOutputSolo(index, soloButtons[index].getToggleState()); };
            addAndMakeVisible(s);

            stereoButtons[index].setButtonText("ST");
            auto& st = stereoButtons[index];
            st.setClickingTogglesState(true);
            styleRoutingButton(st, getRoutingNodeAccent());
            st.onClick = [this, index]()
            {
                processor->setOutputStereo(index, stereoButtons[index].getToggleState());
                refreshTopology();
            };
            addAndMakeVisible(st);

            auto& ph = phaseButtons[index];
            ph.setButtonText(CharPointer_UTF8("\xc3\x98"));
            ph.setClickingTogglesState(true);
            styleRoutingButton(ph, Colour(0xFFFF8800));
            ph.onClick = [this, index]()
            { processor->setOutputPhaseInvert(index, phaseButtons[index].getToggleState()); };
            addAndMakeVisible(ph);
        }

        syncControlsFromProcessor();
        startTimerHz(24);
    }

    ~SplitterControl() override
    {
        addStripButton.removeListener(this);
        removeStripButton.removeListener(this);
        stopTimer();
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced(10, 8);
        auto header = area.removeFromTop(20);
        removeStripButton.setBounds(header.removeFromRight(22).reduced(1, 2));
        addStripButton.setBounds(header.removeFromRight(22).reduced(1, 2));
        area.removeFromTop(2);

        auto inputRow = area.removeFromTop(26);
        inputRowArea = inputRow.reduced(0, 1);
        inBadge = inputRow.removeFromLeft(36).reduced(1, 4);
        inputMeter = inputRow.reduced(8, 10);

        fanoutArea = area.removeFromTop(24).reduced(0, 2);
        area.removeFromTop(2);

        for (int i = 0; i < SplitterProcessor::MaxStrips; ++i)
        {
            outRows[i] = {};
            outBadges[i] = {};
            outLanes[i] = {};
            outDbAreas[i] = {};
        }

        for (int i = 0; i < processor->getNumStrips(); ++i)
        {
            auto output = area.removeFromTop(44);
            outRows[i] = output.reduced(0, 1);
            outBadges[i] = output.removeFromLeft(28).reduced(1, 3);

            phaseButtons[i].setBounds(output.removeFromRight(20).reduced(1, 13));
            stereoButtons[i].setBounds(output.removeFromRight(24).reduced(1, 13));
            soloButtons[i].setBounds(output.removeFromRight(20).reduced(1, 13));
            muteButtons[i].setBounds(output.removeFromRight(20).reduced(1, 13));

            outDbAreas[i] = output.removeFromRight(36).reduced(0, 12);
            outLanes[i] = output.reduced(7, 16);
            faders[i].setBounds(outLanes[i].expanded(8, 8));
            panSliders[i].setBounds(outLanes[i].withHeight(jmax(8, outLanes[i].getHeight())).expanded(4, 4));
            area.removeFromTop(2);
        }
    }

    void buttonClicked(Button* button) override
    {
        if (button == &addStripButton)
            addStripClicked();
        else if (button == &removeStripButton)
            removeStripClicked();
    }

    void paint(Graphics& g) override
    {
        auto accent = getRoutingNodeAccent();
        paintRoutingRow(g, inputRowArea.toFloat(), accent, false, true);
        paintRoutingBadge(g, inBadge.toFloat(), "IN", accent, true);
        paintRoutingFanout(g, fanoutArea.toFloat(), getRoutingNodeAccent(), false, false,
                           processor->getNumStrips());

        const float inputLevel =
            normaliseRoutingMeter(jmax(processor->inputPeakL.load(std::memory_order_relaxed),
                                       processor->inputPeakR.load(std::memory_order_relaxed)));
        paintRoutingMeterTrack(g, inputMeter.toFloat(), inputLevel, accent, false);

        for (int i = 0; i < processor->getNumStrips(); ++i)
        {
            auto* strip = processor->getStrip(i);
            if (strip == nullptr)
                continue;

            const bool muted = strip->mute.load(std::memory_order_relaxed);
            const bool soloed = strip->solo.load(std::memory_order_relaxed);
            const bool stereo = strip->stereo.load(std::memory_order_relaxed);
            const float peak = jmax(strip->peakL.load(std::memory_order_relaxed),
                                    strip->peakR.load(std::memory_order_relaxed));

            paintRoutingRow(g, outRows[i].toFloat(), accent, muted, false);
            paintRoutingBadge(g, outBadges[i].toFloat(), getRoutingVisualLabel(i), accent, !muted || soloed);
            paintRoutingMeterTrack(g, outLanes[i].toFloat(), normaliseRoutingMeter(peak), accent, muted);

            g.setColour(ColourScheme::getInstance().colours["Text Colour"].withAlpha(muted ? 0.30f : 0.60f));
            g.setFont(Font(9.5f, Font::bold));
            g.drawText(String(processor->getOutputGainDb(i), 1), outDbAreas[i], Justification::centredRight, true);

            g.setFont(Font(7.5f, Font::bold));
            g.setColour(accent.withAlpha(stereo ? 0.62f : 0.28f));
            const auto modeArea = outRows[i].withWidth(20).withX(outRows[i].getRight() - 20);
            g.drawText(stereo ? "ST" : "M", modeArea, Justification::centred, true);
        }
    }

  private:
    SplitterProcessor* processor;
    TextButton addStripButton;
    TextButton removeStripButton;
    std::array<Slider, SplitterProcessor::MaxStrips> faders;
    std::array<Slider, SplitterProcessor::MaxStrips> panSliders;
    std::array<TextButton, SplitterProcessor::MaxStrips> muteButtons;
    std::array<TextButton, SplitterProcessor::MaxStrips> soloButtons;
    std::array<TextButton, SplitterProcessor::MaxStrips> stereoButtons;
    std::array<TextButton, SplitterProcessor::MaxStrips> phaseButtons;
    Rectangle<int> inputRowArea;
    Rectangle<int> inBadge;
    Rectangle<int> inputMeter;
    std::array<Rectangle<int>, SplitterProcessor::MaxStrips> outRows;
    std::array<Rectangle<int>, SplitterProcessor::MaxStrips> outBadges;
    std::array<Rectangle<int>, SplitterProcessor::MaxStrips> outLanes;
    std::array<Rectangle<int>, SplitterProcessor::MaxStrips> outDbAreas;
    Rectangle<int> fanoutArea;

    void addStripClicked()
    {
        processor->addStrip();
        refreshTopology();
    }

    void removeStripClicked()
    {
        processor->removeStrip();
        refreshTopology();
    }

    void refreshTopology()
    {
        const auto newSize = processor->getSize();
        setSize(newSize.getX(), newSize.getY());
        syncControlsFromProcessor();
        resized();
        notifyParentPins(*this);
    }

    void syncControlsFromProcessor()
    {
        const int active = processor->getNumStrips();
        for (int i = 0; i < SplitterProcessor::MaxStrips; ++i)
        {
            const bool visible = i < active;
            faders[i].setVisible(visible);
            panSliders[i].setVisible(visible);
            muteButtons[i].setVisible(visible);
            soloButtons[i].setVisible(visible);
            stereoButtons[i].setVisible(visible);
            phaseButtons[i].setVisible(visible);

            if (visible)
            {
                faders[i].setValue(processor->getOutputGainDb(i), dontSendNotification);
                panSliders[i].setValue(processor->getOutputPan(i), dontSendNotification);
                muteButtons[i].setToggleState(processor->getOutputMute(i), dontSendNotification);
                soloButtons[i].setToggleState(processor->getOutputSolo(i), dontSendNotification);
                stereoButtons[i].setToggleState(processor->getOutputStereo(i), dontSendNotification);
                phaseButtons[i].setToggleState(processor->getOutputPhaseInvert(i), dontSendNotification);
            }
        }
    }

    void timerCallback() override
    {
        repaint();
    }
};

//==============================================================================
// SplitterProcessor Implementation
//==============================================================================

SplitterProcessor::SplitterProcessor()
{
    for (int i = 0; i < DefaultStrips; ++i)
        strips_[static_cast<size_t>(i)].resetDefaults(i);

    numStrips_.store(DefaultStrips, std::memory_order_release);
    updateChannelConfig();
}

SplitterProcessor::~SplitterProcessor() {}

Component* SplitterProcessor::getControls()
{
    return new SplitterControl(this);
}

Point<int> SplitterProcessor::getSize()
{
    return Point<int>(260, jmax(176, 92 + getNumStrips() * 46));
}

int SplitterProcessor::countTotalOutputChannels() const
{
    int total = 0;
    const int n = numStrips_.load(std::memory_order_acquire);
    for (int i = 0; i < n; ++i)
        total += strips_[static_cast<size_t>(i)].stereo.load(std::memory_order_relaxed) ? 2 : 1;
    return total;
}

void SplitterProcessor::updateChannelConfig()
{
    setPlayConfigDetails(2, countTotalOutputChannels(), getSampleRate(), getBlockSize());
}

void SplitterProcessor::addStrip()
{
    const int n = numStrips_.load(std::memory_order_acquire);
    if (n >= MaxStrips)
        return;

    strips_[static_cast<size_t>(n)].resetDefaults(n);
    if (currentSampleRate_ > 0.0)
        stripDsp_[static_cast<size_t>(n)].init(currentSampleRate_);

    numStrips_.store(n + 1, std::memory_order_release);
    updateChannelConfig();
}

void SplitterProcessor::removeStrip()
{
    const int n = numStrips_.load(std::memory_order_acquire);
    if (n <= 1)
        return;

    numStrips_.store(n - 1, std::memory_order_release);
    updateChannelConfig();
}

SplitterProcessor::StripState* SplitterProcessor::getStrip(int index)
{
    if (index >= 0 && index < numStrips_.load(std::memory_order_acquire))
        return &strips_[static_cast<size_t>(index)];
    return nullptr;
}

const SplitterProcessor::StripState* SplitterProcessor::getStrip(int index) const
{
    if (index >= 0 && index < numStrips_.load(std::memory_order_acquire))
        return &strips_[static_cast<size_t>(index)];
    return nullptr;
}

void SplitterProcessor::computeVuDecay(double sampleRate)
{
    currentSampleRate_ = sampleRate;
    const double samplesFor300ms = sampleRate * 0.3;
    peakDecay_ = static_cast<float>(std::pow(0.001, 1.0 / samplesFor300ms));
}

void SplitterProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    computeVuDecay(sampleRate);

    const int active = numStrips_.load(std::memory_order_acquire);
    for (int i = 0; i < MaxStrips; ++i)
    {
        auto& dsp = stripDsp_[static_cast<size_t>(i)];
        dsp.init(sampleRate);
        if (i < active)
        {
            const float gain =
                Decibels::decibelsToGain(strips_[static_cast<size_t>(i)].gainDb.load(std::memory_order_relaxed));
            dsp.smoothedGain.setCurrentAndTargetValue(gain);
        }
    }

    inputVuDspL_.init(static_cast<float>(sampleRate));
    inputVuDspR_.init(static_cast<float>(sampleRate));
    inputSnapshot_.setSize(2, samplesPerBlock, false, true, true);
}

void SplitterProcessor::processBlock(AudioBuffer<float>& buffer, MidiBuffer& /*midiMessages*/)
{
    const int numSamples = buffer.getNumSamples();
    const int ns = numStrips_.load(std::memory_order_acquire);
    const int totalChannels = buffer.getNumChannels();

    if (ns == 0 || numSamples == 0 || totalChannels <= 0)
    {
        buffer.clear();
        return;
    }

    if (inputSnapshot_.getNumSamples() < numSamples || inputSnapshot_.getNumChannels() < 2)
    {
        buffer.clear();
        return;
    }

    FloatVectorOperations::copy(inputSnapshot_.getWritePointer(0), buffer.getReadPointer(0), numSamples);
    FloatVectorOperations::copy(inputSnapshot_.getWritePointer(1),
                                buffer.getReadPointer(totalChannels > 1 ? 1 : 0), numSamples);

    const float* inL = inputSnapshot_.getReadPointer(0);
    const float* inR = inputSnapshot_.getReadPointer(1);

    float inPkL = inputPeakL.load(std::memory_order_relaxed);
    float inPkR = inputPeakR.load(std::memory_order_relaxed);
    for (int i = 0; i < numSamples; ++i)
    {
        const float absL = std::abs(inL[i]);
        const float absR = std::abs(inR[i]);
        inPkL = (absL > inPkL) ? absL : inPkL * peakDecay_;
        inPkR = (absR > inPkR) ? absR : inPkR * peakDecay_;
    }
    if (inPkL < 1e-10f)
        inPkL = 0.0f;
    if (inPkR < 1e-10f)
        inPkR = 0.0f;
    inputPeakL.store(inPkL, std::memory_order_relaxed);
    inputPeakR.store(inPkR, std::memory_order_relaxed);
    inputVuL.store(inPkL, std::memory_order_relaxed);
    inputVuR.store(inPkR, std::memory_order_relaxed);

    bool anySolo = false;
    for (int s = 0; s < ns; ++s)
    {
        if (strips_[static_cast<size_t>(s)].solo.load(std::memory_order_relaxed))
        {
            anySolo = true;
            break;
        }
    }

    int currentOutputChannel = 0;
    for (int s = 0; s < ns; ++s)
    {
        auto& strip = strips_[static_cast<size_t>(s)];
        auto& dsp = stripDsp_[static_cast<size_t>(s)];

        const bool isStereo = strip.stereo.load(std::memory_order_relaxed);
        const int channelsNeeded = isStereo ? 2 : 1;
        if (currentOutputChannel + channelsNeeded > totalChannels)
            break;

        float* dstL = buffer.getWritePointer(currentOutputChannel);
        float* dstR = isStereo ? buffer.getWritePointer(currentOutputChannel + 1) : nullptr;
        currentOutputChannel += channelsNeeded;

        const bool effectiveMute = strip.mute.load(std::memory_order_relaxed) ||
                                   (anySolo && !strip.solo.load(std::memory_order_relaxed));
        const bool phaseInv = strip.phaseInvert.load(std::memory_order_relaxed);
        const float gainDb = strip.gainDb.load(std::memory_order_relaxed);
        const float pan = strip.pan.load(std::memory_order_relaxed);

        dsp.smoothedGain.setTargetValue(Decibels::decibelsToGain(gainDb));

        float panL = 1.0f;
        float panR = 1.0f;
        if (isStereo)
        {
            if (pan <= 0.0f)
                panR = 1.0f + pan;
            else
                panL = 1.0f - pan;
        }

        float peakL = strip.peakL.load(std::memory_order_relaxed);
        float peakR = strip.peakR.load(std::memory_order_relaxed);

        for (int i = 0; i < numSamples; ++i)
        {
            const float gain = dsp.smoothedGain.getNextValue();
            float outL = 0.0f;
            float outR = 0.0f;

            if (isStereo)
            {
                outL = inL[i];
                outR = inR[i];
                if (phaseInv)
                {
                    outL = -outL;
                    outR = -outR;
                }
                outL *= gain * panL;
                outR *= gain * panR;
            }
            else
            {
                outL = (inL[i] + inR[i]) * 0.5f;
                if (phaseInv)
                    outL = -outL;
                outL *= gain;
                outR = outL;
            }

            const float absL = std::abs(outL);
            const float absR = std::abs(outR);
            peakL = (absL > peakL) ? absL : peakL * peakDecay_;
            peakR = isStereo ? ((absR > peakR) ? absR : peakR * peakDecay_) : peakL;

            dstL[i] = effectiveMute ? 0.0f : outL;
            if (dstR != nullptr)
                dstR[i] = effectiveMute ? 0.0f : outR;
        }

        if (peakL < 1e-10f)
            peakL = 0.0f;
        if (peakR < 1e-10f)
            peakR = 0.0f;
        strip.peakL.store(peakL, std::memory_order_relaxed);
        strip.peakR.store(peakR, std::memory_order_relaxed);
        strip.vuL.store(peakL, std::memory_order_relaxed);
        strip.vuR.store(peakR, std::memory_order_relaxed);
    }

    for (int ch = currentOutputChannel; ch < totalChannels; ++ch)
        buffer.clear(ch, 0, numSamples);
}

void SplitterProcessor::fillInPluginDescription(PluginDescription& description) const
{
    description.name = "Splitter";
    description.descriptiveName = "Splits stereo input to dynamic mono or stereo outputs.";
    description.pluginFormatName = "Internal";
    description.category = "Routing";
    description.manufacturerName = "Pedalboard3";
    description.version = "3.00";
    description.uniqueId = description.name.hashCode();
    description.isInstrument = false;
    description.numInputChannels = 2;
    description.numOutputChannels = countTotalOutputChannels();
}

bool SplitterProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    return layouts.getMainInputChannels() == 2 && layouts.getMainOutputChannels() == countTotalOutputChannels();
}

const String SplitterProcessor::getInputChannelName(int channelIndex) const
{
    return channelIndex == 0 ? "Input L" : "Input R";
}

const String SplitterProcessor::getOutputChannelName(int channelIndex) const
{
    if (channelIndex < countTotalOutputChannels())
    {
        int currentCh = 0;
        const int n = numStrips_.load(std::memory_order_acquire);
        for (int i = 0; i < n; ++i)
        {
            const auto& strip = strips_[static_cast<size_t>(i)];
            const bool stereo = strip.stereo.load(std::memory_order_relaxed);
            const int channels = stereo ? 2 : 1;
            if (channelIndex < currentCh + channels)
                return stereo ? strip.name + (channelIndex == currentCh ? " L" : " R") : strip.name;
            currentCh += channels;
        }
    }
    return "Output " + String(channelIndex + 1);
}

void SplitterProcessor::setOutputMute(int outputIndex, bool shouldMute)
{
    if (auto* strip = getStrip(outputIndex))
        strip->mute.store(shouldMute, std::memory_order_relaxed);
}

bool SplitterProcessor::getOutputMute(int outputIndex) const
{
    if (auto* strip = getStrip(outputIndex))
        return strip->mute.load(std::memory_order_relaxed);
    return false;
}

void SplitterProcessor::setOutputGainDb(int outputIndex, float db)
{
    if (auto* strip = getStrip(outputIndex))
        strip->gainDb.store(jlimit(MinGainDb, MaxGainDb, db), std::memory_order_relaxed);
}

float SplitterProcessor::getOutputGainDb(int outputIndex) const
{
    if (auto* strip = getStrip(outputIndex))
        return strip->gainDb.load(std::memory_order_relaxed);
    return 0.0f;
}

void SplitterProcessor::setOutputPan(int outputIndex, float pan)
{
    if (auto* strip = getStrip(outputIndex))
        strip->pan.store(jlimit(-1.0f, 1.0f, pan), std::memory_order_relaxed);
}

float SplitterProcessor::getOutputPan(int outputIndex) const
{
    if (auto* strip = getStrip(outputIndex))
        return strip->pan.load(std::memory_order_relaxed);
    return 0.0f;
}

void SplitterProcessor::setOutputSolo(int outputIndex, bool shouldSolo)
{
    if (auto* strip = getStrip(outputIndex))
        strip->solo.store(shouldSolo, std::memory_order_relaxed);
}

bool SplitterProcessor::getOutputSolo(int outputIndex) const
{
    if (auto* strip = getStrip(outputIndex))
        return strip->solo.load(std::memory_order_relaxed);
    return false;
}

void SplitterProcessor::setOutputStereo(int outputIndex, bool shouldBeStereo)
{
    if (auto* strip = getStrip(outputIndex))
    {
        const bool old = strip->stereo.exchange(shouldBeStereo, std::memory_order_acq_rel);
        if (old != shouldBeStereo)
            updateChannelConfig();
    }
}

bool SplitterProcessor::getOutputStereo(int outputIndex) const
{
    if (auto* strip = getStrip(outputIndex))
        return strip->stereo.load(std::memory_order_relaxed);
    return true;
}

void SplitterProcessor::setOutputPhaseInvert(int outputIndex, bool shouldInvert)
{
    if (auto* strip = getStrip(outputIndex))
        strip->phaseInvert.store(shouldInvert, std::memory_order_relaxed);
}

bool SplitterProcessor::getOutputPhaseInvert(int outputIndex) const
{
    if (auto* strip = getStrip(outputIndex))
        return strip->phaseInvert.load(std::memory_order_relaxed);
    return false;
}

bool SplitterProcessor::isOutputChannelStereoPair(int channelIndex) const
{
    int currentCh = 0;
    const int n = numStrips_.load(std::memory_order_acquire);
    for (int i = 0; i < n; ++i)
    {
        const bool stereo = strips_[static_cast<size_t>(i)].stereo.load(std::memory_order_relaxed);
        const int channels = stereo ? 2 : 1;
        if (channelIndex >= currentCh && channelIndex < currentCh + channels)
            return stereo;
        currentCh += channels;
    }
    return false;
}

void SplitterProcessor::getStateInformation(MemoryBlock& destData)
{
    XmlElement xml("SplitterSettings");
    xml.setAttribute("version", 4);
    xml.setAttribute("numStrips", getNumStrips());
    for (int i = 0; i < getNumStrips(); ++i)
    {
        const auto& strip = strips_[static_cast<size_t>(i)];
        const String prefix = "strip" + String(i) + "_";
        xml.setAttribute(prefix + "gainDb", static_cast<double>(strip.gainDb.load(std::memory_order_relaxed)));
        xml.setAttribute(prefix + "pan", static_cast<double>(strip.pan.load(std::memory_order_relaxed)));
        xml.setAttribute(prefix + "mute", strip.mute.load(std::memory_order_relaxed));
        xml.setAttribute(prefix + "solo", strip.solo.load(std::memory_order_relaxed));
        xml.setAttribute(prefix + "stereo", strip.stereo.load(std::memory_order_relaxed));
        xml.setAttribute(prefix + "phase", strip.phaseInvert.load(std::memory_order_relaxed));
        xml.setAttribute(prefix + "name", strip.name);
    }
    copyXmlToBinary(xml, destData);
}

void SplitterProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState == nullptr || !xmlState->hasTagName("SplitterSettings"))
        return;

    if (!xmlState->hasAttribute("version"))
    {
        setOutputMute(0, xmlState->getBoolAttribute("muteA"));
        setOutputMute(1, xmlState->getBoolAttribute("muteB"));
        return;
    }

    const int active = jlimit(1, MaxStrips, xmlState->getIntAttribute("numStrips", DefaultStrips));
    for (int i = 0; i < active; ++i)
    {
        auto& strip = strips_[static_cast<size_t>(i)];
        strip.resetDefaults(i);
        const String prefix = "strip" + String(i) + "_";
        strip.gainDb.store(static_cast<float>(xmlState->getDoubleAttribute(prefix + "gainDb", 0.0)),
                           std::memory_order_relaxed);
        strip.pan.store(static_cast<float>(xmlState->getDoubleAttribute(prefix + "pan", 0.0)),
                        std::memory_order_relaxed);
        strip.mute.store(xmlState->getBoolAttribute(prefix + "mute", false), std::memory_order_relaxed);
        strip.solo.store(xmlState->getBoolAttribute(prefix + "solo", false), std::memory_order_relaxed);
        strip.stereo.store(xmlState->getBoolAttribute(prefix + "stereo", true), std::memory_order_relaxed);
        strip.phaseInvert.store(xmlState->getBoolAttribute(prefix + "phase", false), std::memory_order_relaxed);
        strip.name = xmlState->getStringAttribute(prefix + "name", "Out " + String(i + 1));
    }
    numStrips_.store(active, std::memory_order_release);
    updateChannelConfig();
}

PedalboardProcessor::PinLayout SplitterProcessor::getInputPinLayout() const
{
    PinLayout layout;
    layout.pinY.push_back(54);
    layout.pinY.push_back(76);
    return layout;
}

PedalboardProcessor::PinLayout SplitterProcessor::getOutputPinLayout() const
{
    PinLayout layout;
    const int n = numStrips_.load(std::memory_order_acquire);
    const int firstRowTop = 26 + 82;
    for (int i = 0; i < n; ++i)
    {
        const int rowTop = firstRowTop + i * 46;
        const bool stereo = strips_[static_cast<size_t>(i)].stereo.load(std::memory_order_relaxed);
        if (stereo)
        {
            layout.pinY.push_back(rowTop + 6);
            layout.pinY.push_back(rowTop + 28);
        }
        else
        {
            layout.pinY.push_back(rowTop + 17);
        }
    }
    return layout;
}

//==============================================================================
// MixerProcessor Implementation
//==============================================================================

MixerProcessor::MixerProcessor()
{
    for (int i = 0; i < DefaultStrips; ++i)
        strips_[static_cast<size_t>(i)].resetDefaults(i);

    numStrips_.store(DefaultStrips, std::memory_order_release);
    updateChannelConfig();
}

MixerProcessor::~MixerProcessor() {}

void MixerProcessor::computeVuDecay(double sampleRate)
{
    currentSampleRate_ = sampleRate;
    const double samplesFor300ms = sampleRate * 0.3;
    peakDecay_ = static_cast<float>(std::pow(0.001, 1.0 / samplesFor300ms));
}

void MixerProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    computeVuDecay(sampleRate);

    const int active = numStrips_.load(std::memory_order_acquire);
    for (int i = 0; i < MaxStrips; ++i)
    {
        auto& dsp = stripDsp_[static_cast<size_t>(i)];
        dsp.init(sampleRate);
        if (i < active)
        {
            const float gain =
                Decibels::decibelsToGain(strips_[static_cast<size_t>(i)].gainDb.load(std::memory_order_relaxed));
            dsp.smoothedGain.setCurrentAndTargetValue(gain);
        }
    }

    smoothedMasterGain_.reset(sampleRate, GainRampSeconds);
    smoothedMasterGain_.setCurrentAndTargetValue(Decibels::decibelsToGain(masterGainDb.load(std::memory_order_relaxed)));
    masterVuDspL_.init(static_cast<float>(sampleRate));
    masterVuDspR_.init(static_cast<float>(sampleRate));
    tempBuffer_.setSize(2, samplesPerBlock, false, true, true);
}

Component* MixerProcessor::getControls()
{
    class MixerControl : public Component, private Timer
    {
      public:
        MixerControl(MixerProcessor* proc) : processor(proc)
        {
            addStripButton.setButtonText("+");
            addStripButton.onClick = [this]() { addStripClicked(); };
            styleRoutingButton(addStripButton, getRoutingNodeAccent());
            addAndMakeVisible(addStripButton);

            removeStripButton.setButtonText("-");
            removeStripButton.onClick = [this]() { removeStripClicked(); };
            styleRoutingButton(removeStripButton, getRoutingNodeAccent());
            addAndMakeVisible(removeStripButton);

            for (int ch = 0; ch < MixerProcessor::MaxStrips; ++ch)
            {
                auto& f = faders[ch];
                f.setSliderStyle(Slider::LinearVertical);
                f.setTextBoxStyle(Slider::NoTextBox, false, 0, 0);
                f.setRange(MixerProcessor::MinGainDb, MixerProcessor::MaxGainDb, 0.1);
                f.setValue(processor->getChannelGainDb(ch), dontSendNotification);
                f.onValueChange = [this, ch]()
                { processor->setChannelGainDb(ch, static_cast<float>(faders[ch].getValue())); };
                f.setAlpha(0.01f);
                addAndMakeVisible(f);

                auto& p = panKnobs[ch];
                p.setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
                p.setTextBoxStyle(Slider::NoTextBox, false, 0, 0);
                p.setRange(-1.0, 1.0, 0.01);
                p.setDoubleClickReturnValue(true, 0.0);
                p.setValue(processor->getChannelPan(ch), dontSendNotification);
                p.onValueChange = [this, ch]()
                { processor->setChannelPan(ch, static_cast<float>(panKnobs[ch].getValue())); };
                p.setAlpha(0.01f);
                addAndMakeVisible(p);

                auto& m = muteButtons[ch];
                m.setButtonText("M");
                m.setClickingTogglesState(true);
                styleRoutingButton(m, ColourScheme::getInstance().colours["Danger Colour"]);
                m.setToggleState(processor->getChannelMute(ch), dontSendNotification);
                m.onClick = [this, ch]() { processor->setChannelMute(ch, muteButtons[ch].getToggleState()); };
                addAndMakeVisible(m);

                auto& s = soloButtons[ch];
                s.setButtonText("S");
                s.setClickingTogglesState(true);
                styleRoutingButton(s, Colour(0xFFCCAA00));
                s.setToggleState(processor->getChannelSolo(ch), dontSendNotification);
                s.onClick = [this, ch]() { processor->setChannelSolo(ch, soloButtons[ch].getToggleState()); };
                addAndMakeVisible(s);

                auto& st = stereoButtons[ch];
                st.setButtonText("ST");
                st.setClickingTogglesState(true);
                styleRoutingButton(st, getRoutingNodeAccent());
                st.setToggleState(processor->getChannelStereo(ch), dontSendNotification);
                st.onClick = [this, ch]()
                {
                    processor->setChannelStereo(ch, stereoButtons[ch].getToggleState());
                    refreshTopology();
                };
                addAndMakeVisible(st);

                auto& ph = phaseButtons[ch];
                ph.setButtonText(CharPointer_UTF8("\xc3\x98"));
                ph.setClickingTogglesState(true);
                styleRoutingButton(ph, Colour(0xFFFF8800));
                ph.setToggleState(processor->getChannelPhaseInvert(ch), dontSendNotification);
                ph.onClick = [this, ch]()
                { processor->setChannelPhaseInvert(ch, phaseButtons[ch].getToggleState()); };
                addAndMakeVisible(ph);
            }

            auto& mf = masterFader;
            mf.setSliderStyle(Slider::LinearVertical);
            mf.setTextBoxStyle(Slider::NoTextBox, false, 0, 0);
            mf.setRange(MixerProcessor::MinGainDb, MixerProcessor::MaxGainDb, 0.1);
            mf.setValue(processor->getMasterGainDb(), dontSendNotification);
            mf.onValueChange = [this]() { processor->setMasterGainDb(static_cast<float>(masterFader.getValue())); };
            mf.setAlpha(0.01f);
            addAndMakeVisible(mf);

            masterMuteButton.setButtonText("M");
            masterMuteButton.setClickingTogglesState(true);
            styleRoutingButton(masterMuteButton, ColourScheme::getInstance().colours["Danger Colour"]);
            masterMuteButton.setToggleState(processor->getMasterMute(), dontSendNotification);
            masterMuteButton.onClick = [this]() { processor->setMasterMute(masterMuteButton.getToggleState()); };
            addAndMakeVisible(masterMuteButton);

            syncControlsFromProcessor();
            startTimerHz(30);
        }

        ~MixerControl() override { stopTimer(); }

        void resized() override
        {
            auto area = getLocalBounds().reduced(8, 6);
            auto header = area.removeFromTop(20);
            removeStripButton.setBounds(header.removeFromRight(22).reduced(1, 2));
            addStripButton.setBounds(header.removeFromRight(22).reduced(1, 2));
            area.removeFromTop(5);

            const int activeStrips = processor->getNumStrips();

            for (int ch = 0; ch < MixerProcessor::MaxStrips; ++ch)
            {
                vuAreas[ch] = {};
                stripDecks[ch] = {};
                badgeAreas[ch] = {};
                panRails[ch] = {};
                valueAreas[ch] = {};
            }

            for (int ch = 0; ch < activeStrips; ++ch)
            {
                auto row = area.removeFromTop(kMixerStripRowHeight);
                stripDecks[ch] = row.reduced(0, 1);

                auto strip = stripDecks[ch].reduced(7, 5);
                badgeAreas[ch] = strip.removeFromLeft(28).reduced(1, 6);

                phaseButtons[ch].setBounds(strip.removeFromRight(20).withSizeKeepingCentre(18, 16));
                stereoButtons[ch].setBounds(strip.removeFromRight(24).withSizeKeepingCentre(22, 16));
                soloButtons[ch].setBounds(strip.removeFromRight(20).withSizeKeepingCentre(18, 16));
                muteButtons[ch].setBounds(strip.removeFromRight(20).withSizeKeepingCentre(18, 16));

                valueAreas[ch] = strip.removeFromRight(42).reduced(0, 13);
                vuAreas[ch] = strip.removeFromRight(30).reduced(5, 3).withWidth(18);
                faders[ch].setBounds(vuAreas[ch].expanded(14, 4));

                panRails[ch] = strip.removeFromTop(14).reduced(8, 4);
                panKnobs[ch].setBounds(panRails[ch].expanded(7, 7));
            }

            auto masterStrip = area.removeFromTop(kMixerMasterRowHeight);
            masterDeck = masterStrip.reduced(0, 1);
            masterStrip = masterDeck.reduced(7, 5);
            masterBadgeArea = masterStrip.removeFromLeft(28).reduced(1, 6);
            masterMuteButton.setBounds(masterStrip.removeFromRight(24).withSizeKeepingCentre(22, 16));
            masterValueArea = masterStrip.removeFromRight(44).reduced(0, 14);
            masterFaderArea = masterStrip.removeFromRight(30).reduced(5, 3).withWidth(18);
            masterFader.setBounds(masterFaderArea.expanded(14, 4));
            masterPanRail = masterStrip.removeFromTop(14).reduced(8, 4);
        }

        void timerCallback() override { repaint(); }

        void paint(Graphics& g) override
        {
            auto& cs = ColourScheme::getInstance();
            const auto accent = getRoutingNodeAccent();

            for (int ch = 0; ch < processor->getNumStrips(); ++ch)
            {
                paintMixerStripDeck(g, stripDecks[ch].toFloat(), accent, getRoutingVisualLabel(ch));
                paintRoutingBadge(g, badgeAreas[ch].toFloat(), getRoutingVisualLabel(ch), accent, true);
                paintMixerPanRail(g, panRails[ch].toFloat(), static_cast<float>(panKnobs[ch].getValue()), accent);
                drawVuMeter(g, ch, cs);
                g.setColour(cs.colours["Text Colour"].withAlpha(0.70f));
                g.setFont(Font(9.8f, Font::bold));
                g.drawText(String(processor->getChannelGainDb(ch), 1), valueAreas[ch], Justification::centred, true);
            }

            paintMixerMasterDeck(g, masterDeck.toFloat(), accent);
            paintRoutingBadge(g, masterBadgeArea.toFloat(), "M", accent, true);
            paintMixerPanRail(g, masterPanRail.toFloat(), 0.0f, accent);
            drawMasterFader(g, cs);
            g.setColour(cs.colours["Text Colour"].withAlpha(0.72f));
            g.setFont(Font(9.8f, Font::bold));
            g.drawText(String(processor->getMasterGainDb(), 1), masterValueArea, Justification::centred, true);
        }

      private:
        MixerProcessor* processor;
        TextButton addStripButton;
        TextButton removeStripButton;
        std::array<Slider, MixerProcessor::MaxStrips> faders;
        std::array<Slider, MixerProcessor::MaxStrips> panKnobs;
        std::array<TextButton, MixerProcessor::MaxStrips> muteButtons;
        std::array<TextButton, MixerProcessor::MaxStrips> soloButtons;
        std::array<TextButton, MixerProcessor::MaxStrips> stereoButtons;
        std::array<TextButton, MixerProcessor::MaxStrips> phaseButtons;
        Slider masterFader;
        TextButton masterMuteButton;
        std::array<Rectangle<int>, MixerProcessor::MaxStrips> vuAreas;
        std::array<Rectangle<int>, MixerProcessor::MaxStrips> stripDecks;
        std::array<Rectangle<int>, MixerProcessor::MaxStrips> badgeAreas;
        std::array<Rectangle<int>, MixerProcessor::MaxStrips> panRails;
        std::array<Rectangle<int>, MixerProcessor::MaxStrips> valueAreas;
        Rectangle<int> masterDeck;
        Rectangle<int> masterBadgeArea;
        Rectangle<int> masterPanRail;
        Rectangle<int> masterFaderArea;
        Rectangle<int> masterValueArea;

        void addStripClicked()
        {
            processor->addStrip();
            refreshTopology();
        }

        void removeStripClicked()
        {
            processor->removeStrip();
            refreshTopology();
        }

        void refreshTopology()
        {
            const auto newSize = processor->getSize();
            setSize(newSize.getX(), newSize.getY());
            syncControlsFromProcessor();
            resized();
            notifyParentPins(*this);
        }

        void syncControlsFromProcessor()
        {
            const int active = processor->getNumStrips();
            for (int ch = 0; ch < MixerProcessor::MaxStrips; ++ch)
            {
                const bool visible = ch < active;
                faders[ch].setVisible(visible);
                panKnobs[ch].setVisible(visible);
                muteButtons[ch].setVisible(visible);
                soloButtons[ch].setVisible(visible);
                stereoButtons[ch].setVisible(visible);
                phaseButtons[ch].setVisible(visible);
                if (visible)
                {
                    faders[ch].setValue(processor->getChannelGainDb(ch), dontSendNotification);
                    panKnobs[ch].setValue(processor->getChannelPan(ch), dontSendNotification);
                    muteButtons[ch].setToggleState(processor->getChannelMute(ch), dontSendNotification);
                    soloButtons[ch].setToggleState(processor->getChannelSolo(ch), dontSendNotification);
                    stereoButtons[ch].setToggleState(processor->getChannelStereo(ch), dontSendNotification);
                    phaseButtons[ch].setToggleState(processor->getChannelPhaseInvert(ch), dontSendNotification);
                }
            }
        }

        void drawMasterFader(Graphics& g, ColourScheme& cs)
        {
            auto area = masterFaderArea;
            if (area.isEmpty())
                return;

            g.setColour(cs.colours["Plugin Background"].darker(0.35f));
            g.fillRoundedRectangle(area.toFloat(), 4.0f);
            drawFaderFill(g, area, processor->getMasterGainDb(), cs);
            g.setColour(getRoutingNodeAccent().withAlpha(masterMuteButton.getToggleState() ? 0.12f : 0.30f));
            g.drawRoundedRectangle(area.toFloat().reduced(0.5f), 4.0f, 0.9f);
        }

        void drawVuMeter(Graphics& g, int ch, ColourScheme& cs)
        {
            auto area = vuAreas[ch];
            if (area.isEmpty())
                return;

            g.setColour(cs.colours["Plugin Background"].darker(0.35f));
            g.fillRoundedRectangle(area.toFloat(), 4.0f);

            int barW = (area.getWidth() - 6) / 2;
            auto leftBar = area.withWidth(barW).translated(2, 0).reduced(0, 2);
            auto rightBar = leftBar.translated(barW + 2, 0);

            auto* strip = processor->getStrip(ch);
            const float vuL = strip != nullptr ? strip->vuL.load(std::memory_order_relaxed) : 0.0f;
            const float vuR = strip != nullptr ? strip->vuR.load(std::memory_order_relaxed) : 0.0f;
            const float peakL = strip != nullptr ? strip->peakL.load(std::memory_order_relaxed) : 0.0f;
            const float peakR = strip != nullptr ? strip->peakR.load(std::memory_order_relaxed) : 0.0f;

            drawSingleBar(g, leftBar, vuL, peakL, cs);
            drawSingleBar(g, rightBar, vuR, peakR, cs);

            drawFaderFill(g, area, processor->getChannelGainDb(ch), cs);

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

        void drawFaderFill(Graphics& g, Rectangle<int> area, float gainDb, ColourScheme& cs)
        {
            const auto track = area.toFloat().reduced(1.0f);
            const auto accent = getRoutingNodeAccent();
            const float normalized = jlimit(0.0f, 1.0f, (gainDb + 60.0f) / 72.0f);
            auto fill = track.withTop(track.getBottom() - track.getHeight() * normalized);

            ColourGradient faderFill(accent.withAlpha(0.36f), fill.getCentreX(), fill.getBottom(),
                                     cs.colours["VU Meter Lower Colour"].withAlpha(0.50f), fill.getCentreX(),
                                     fill.getY(), false);
            faderFill.addColour(0.62,
                                 accent.interpolatedWith(cs.colours["VU Meter Upper Colour"], 0.35f).withAlpha(0.42f));
            g.setGradientFill(faderFill);
            g.fillRoundedRectangle(fill, 3.0f);

            const float thumbY = jlimit(track.getY() + 4.0f, track.getBottom() - 4.0f,
                                        track.getBottom() - track.getHeight() * normalized);
            auto thumb = Rectangle<float>(track.getCentreX() - 10.0f, thumbY - 3.0f, 20.0f, 6.0f);
            ColourGradient thumbFill(cs.colours["Button Highlight"], thumb.getX(), thumb.getY(),
                                     cs.colours["Button Colour"], thumb.getX(), thumb.getBottom(), false);
            thumbFill.addColour(0.50, cs.colours["Text Colour"].withAlpha(0.16f));
            g.setGradientFill(thumbFill);
            g.fillRoundedRectangle(thumb, 2.0f);
            g.setColour(accent.withAlpha(0.56f));
            g.drawRoundedRectangle(thumb.reduced(0.5f), 2.0f, 0.8f);
        }

        void drawSingleBar(Graphics& g, Rectangle<int> bar, float vuLevel, float peakLevel, ColourScheme& cs)
        {
            float vuDb = Decibels::gainToDecibels(vuLevel, -60.0f);
            float norm = jlimit(0.0f, 1.0f, (vuDb + 60.0f) / 72.0f);
            int fillH = static_cast<int>(norm * bar.getHeight());

            float hFull = static_cast<float>(bar.getHeight());
            float yellowThreshold = 48.0f / 72.0f;
            float redThreshold = 60.0f / 72.0f;

            for (int y = bar.getBottom() - fillH; y < bar.getBottom(); ++y)
            {
                float frac = 1.0f - static_cast<float>(y - bar.getY()) / hFull;
                Colour barCol;
                if (frac >= redThreshold)
                    barCol = cs.colours["VU Meter Over Colour"].withAlpha(0.62f);
                else if (frac >= yellowThreshold)
                    barCol = cs.colours["VU Meter Upper Colour"].withAlpha(0.58f);
                else
                    barCol = cs.colours["VU Meter Lower Colour"].withAlpha(0.52f);
                g.setColour(barCol);
                g.drawHorizontalLine(y, static_cast<float>(bar.getX()), static_cast<float>(bar.getRight()));
            }

            float peakDb = Decibels::gainToDecibels(peakLevel, -60.0f);
            float peakNorm = jlimit(0.0f, 1.0f, (peakDb + 60.0f) / 72.0f);
            if (peakNorm > 0.001f)
            {
                int peakY = bar.getBottom() - static_cast<int>(peakNorm * bar.getHeight());
                g.setColour(Colours::white.withAlpha(0.62f));
                g.drawHorizontalLine(peakY, static_cast<float>(bar.getX()), static_cast<float>(bar.getRight()));
            }
        }

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MixerControl)
    };

    return new MixerControl(this);
}

Point<int> MixerProcessor::getSize()
{
    return Point<int>(kMixerNodeWidth, 34 + getNumStrips() * kMixerStripRowHeight + kMixerMasterRowHeight);
}

int MixerProcessor::countTotalInputChannels() const
{
    int total = 0;
    const int n = numStrips_.load(std::memory_order_acquire);
    for (int i = 0; i < n; ++i)
        total += strips_[static_cast<size_t>(i)].stereo.load(std::memory_order_relaxed) ? 2 : 1;
    return total;
}

void MixerProcessor::updateChannelConfig()
{
    setPlayConfigDetails(countTotalInputChannels(), 2, getSampleRate(), getBlockSize());
}

void MixerProcessor::addStrip()
{
    const int n = numStrips_.load(std::memory_order_acquire);
    if (n >= MaxStrips)
        return;

    strips_[static_cast<size_t>(n)].resetDefaults(n);
    if (currentSampleRate_ > 0.0)
        stripDsp_[static_cast<size_t>(n)].init(currentSampleRate_);

    numStrips_.store(n + 1, std::memory_order_release);
    updateChannelConfig();
}

void MixerProcessor::removeStrip()
{
    const int n = numStrips_.load(std::memory_order_acquire);
    if (n <= 1)
        return;

    numStrips_.store(n - 1, std::memory_order_release);
    updateChannelConfig();
}

MixerProcessor::StripState* MixerProcessor::getStrip(int index)
{
    if (index >= 0 && index < numStrips_.load(std::memory_order_acquire))
        return &strips_[static_cast<size_t>(index)];
    return nullptr;
}

const MixerProcessor::StripState* MixerProcessor::getStrip(int index) const
{
    if (index >= 0 && index < numStrips_.load(std::memory_order_acquire))
        return &strips_[static_cast<size_t>(index)];
    return nullptr;
}

void MixerProcessor::processBlock(AudioBuffer<float>& buffer, MidiBuffer& /*midiMessages*/)
{
    const int numSamples = buffer.getNumSamples();
    const int ns = numStrips_.load(std::memory_order_acquire);
    const int totalInputChannels = buffer.getNumChannels();

    if (ns == 0 || numSamples == 0)
    {
        buffer.clear();
        return;
    }

    if (tempBuffer_.getNumSamples() < numSamples || tempBuffer_.getNumChannels() < 2)
    {
        buffer.clear();
        return;
    }

    tempBuffer_.clear();

    bool anySolo = false;
    for (int s = 0; s < ns; ++s)
    {
        if (strips_[static_cast<size_t>(s)].solo.load(std::memory_order_relaxed))
        {
            anySolo = true;
            break;
        }
    }

    float* mixL = tempBuffer_.getWritePointer(0);
    float* mixR = tempBuffer_.getWritePointer(1);
    int currentInputChannel = 0;

    for (int s = 0; s < ns; ++s)
    {
        auto& strip = strips_[static_cast<size_t>(s)];
        auto& dsp = stripDsp_[static_cast<size_t>(s)];

        const bool isStereo = strip.stereo.load(std::memory_order_relaxed);
        const int channelsNeeded = isStereo ? 2 : 1;
        if (currentInputChannel + channelsNeeded > totalInputChannels)
            break;

        const float* srcL = buffer.getReadPointer(currentInputChannel);
        const float* srcR = isStereo ? buffer.getReadPointer(currentInputChannel + 1) : nullptr;
        currentInputChannel += channelsNeeded;

        const bool effectiveMute = strip.mute.load(std::memory_order_relaxed) ||
                                   (anySolo && !strip.solo.load(std::memory_order_relaxed));
        const bool phaseInv = strip.phaseInvert.load(std::memory_order_relaxed);
        const float pan = strip.pan.load(std::memory_order_relaxed);
        const float gainDb = strip.gainDb.load(std::memory_order_relaxed);

        dsp.smoothedGain.setTargetValue(Decibels::decibelsToGain(gainDb));

        float panL = 1.0f;
        float panR = 1.0f;
        if (isStereo)
        {
            if (pan <= 0.0f)
                panR = 1.0f + pan;
            else
                panL = 1.0f - pan;
        }
        else
        {
            panL = std::sqrt(0.5f * (1.0f - pan));
            panR = std::sqrt(0.5f * (1.0f + pan));
        }

        float peakL = strip.peakL.load(std::memory_order_relaxed);
        float peakR = strip.peakR.load(std::memory_order_relaxed);

        for (int i = 0; i < numSamples; ++i)
        {
            const float gain = dsp.smoothedGain.getNextValue();
            float l = srcL[i];
            float r = isStereo ? srcR[i] : l;

            if (phaseInv)
            {
                l = -l;
                r = -r;
            }

            l *= gain * panL;
            r *= gain * panR;

            const float absL = std::abs(l);
            const float absR = std::abs(r);
            peakL = (absL > peakL) ? absL : peakL * peakDecay_;
            peakR = (absR > peakR) ? absR : peakR * peakDecay_;

            if (!effectiveMute)
            {
                mixL[i] += l;
                mixR[i] += r;
            }
        }

        if (peakL < 1e-10f)
            peakL = 0.0f;
        if (peakR < 1e-10f)
            peakR = 0.0f;
        strip.peakL.store(peakL, std::memory_order_relaxed);
        strip.peakR.store(peakR, std::memory_order_relaxed);
        strip.vuL.store(peakL, std::memory_order_relaxed);
        strip.vuR.store(peakR, std::memory_order_relaxed);
    }

    smoothedMasterGain_.setTargetValue(Decibels::decibelsToGain(masterGainDb.load(std::memory_order_relaxed)));
    const bool masterMuted = masterMute.load(std::memory_order_relaxed);
    float masterPkL = masterPeakL.load(std::memory_order_relaxed);
    float masterPkR = masterPeakR.load(std::memory_order_relaxed);

    float* outL = buffer.getWritePointer(0);
    float* outR = buffer.getNumChannels() > 1 ? buffer.getWritePointer(1) : nullptr;

    for (int i = 0; i < numSamples; ++i)
    {
        const float gain = smoothedMasterGain_.getNextValue();
        float l = masterMuted ? 0.0f : mixL[i] * gain;
        float r = masterMuted ? 0.0f : mixR[i] * gain;

        outL[i] = l;
        if (outR != nullptr)
            outR[i] = r;

        const float absL = std::abs(l);
        const float absR = std::abs(r);
        masterPkL = (absL > masterPkL) ? absL : masterPkL * peakDecay_;
        masterPkR = (absR > masterPkR) ? absR : masterPkR * peakDecay_;
    }

    if (masterPkL < 1e-10f)
        masterPkL = 0.0f;
    if (masterPkR < 1e-10f)
        masterPkR = 0.0f;
    masterPeakL.store(masterPkL, std::memory_order_relaxed);
    masterPeakR.store(masterPkR, std::memory_order_relaxed);
    masterVuL.store(masterPkL, std::memory_order_relaxed);
    masterVuR.store(masterPkR, std::memory_order_relaxed);

    for (int ch = 2; ch < buffer.getNumChannels(); ++ch)
        buffer.clear(ch, 0, numSamples);
}

void MixerProcessor::fillInPluginDescription(PluginDescription& description) const
{
    description.name = "Mixer";
    description.descriptiveName = "Mixes dynamic mono or stereo strips to a stereo master.";
    description.pluginFormatName = "Internal";
    description.category = "Routing";
    description.manufacturerName = "Pedalboard3";
    description.version = "4.00";
    description.uniqueId = description.name.hashCode();
    description.isInstrument = false;
    description.numInputChannels = countTotalInputChannels();
    description.numOutputChannels = 2;
}

bool MixerProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    return layouts.getMainInputChannels() == countTotalInputChannels() && layouts.getMainOutputChannels() == 2;
}

const String MixerProcessor::getInputChannelName(int channelIndex) const
{
    if (channelIndex < countTotalInputChannels())
    {
        int currentCh = 0;
        const int n = numStrips_.load(std::memory_order_acquire);
        for (int i = 0; i < n; ++i)
        {
            const auto& strip = strips_[static_cast<size_t>(i)];
            const bool stereo = strip.stereo.load(std::memory_order_relaxed);
            const int channels = stereo ? 2 : 1;
            if (channelIndex < currentCh + channels)
                return stereo ? strip.name + (channelIndex == currentCh ? " L" : " R") : strip.name;
            currentCh += channels;
        }
    }
    return "Input " + String(channelIndex + 1);
}

const String MixerProcessor::getOutputChannelName(int channelIndex) const
{
    return channelIndex == 0 ? "Master L" : "Master R";
}

float MixerProcessor::getChannelGainDb(int ch) const
{
    if (auto* strip = getStrip(ch))
        return strip->gainDb.load(std::memory_order_relaxed);
    return 0.0f;
}

void MixerProcessor::setChannelGainDb(int ch, float db)
{
    if (auto* strip = getStrip(ch))
        strip->gainDb.store(jlimit(MinGainDb, MaxGainDb, db), std::memory_order_relaxed);
}

float MixerProcessor::getChannelPan(int ch) const
{
    if (auto* strip = getStrip(ch))
        return strip->pan.load(std::memory_order_relaxed);
    return 0.0f;
}

void MixerProcessor::setChannelPan(int ch, float p)
{
    if (auto* strip = getStrip(ch))
        strip->pan.store(jlimit(-1.0f, 1.0f, p), std::memory_order_relaxed);
}

bool MixerProcessor::getChannelMute(int ch) const
{
    if (auto* strip = getStrip(ch))
        return strip->mute.load(std::memory_order_relaxed);
    return false;
}

void MixerProcessor::setChannelMute(int ch, bool m)
{
    if (auto* strip = getStrip(ch))
        strip->mute.store(m, std::memory_order_relaxed);
}

bool MixerProcessor::getChannelSolo(int ch) const
{
    if (auto* strip = getStrip(ch))
        return strip->solo.load(std::memory_order_relaxed);
    return false;
}

void MixerProcessor::setChannelSolo(int ch, bool s)
{
    if (auto* strip = getStrip(ch))
        strip->solo.store(s, std::memory_order_relaxed);
}

bool MixerProcessor::getChannelStereo(int ch) const
{
    if (auto* strip = getStrip(ch))
        return strip->stereo.load(std::memory_order_relaxed);
    return true;
}

void MixerProcessor::setChannelStereo(int ch, bool s)
{
    if (auto* strip = getStrip(ch))
    {
        const bool old = strip->stereo.exchange(s, std::memory_order_acq_rel);
        if (old != s)
            updateChannelConfig();
    }
}

bool MixerProcessor::getChannelPhaseInvert(int ch) const
{
    if (auto* strip = getStrip(ch))
        return strip->phaseInvert.load(std::memory_order_relaxed);
    return false;
}

void MixerProcessor::setChannelPhaseInvert(int ch, bool p)
{
    if (auto* strip = getStrip(ch))
        strip->phaseInvert.store(p, std::memory_order_relaxed);
}

float MixerProcessor::getParameter(int parameterIndex)
{
    switch (parameterIndex)
    {
    case ParamGainA:
        return jmap(getChannelGainDb(0), MinGainDb, MaxGainDb, 0.0f, 1.0f);
    case ParamGainB:
        return jmap(getChannelGainDb(1), MinGainDb, MaxGainDb, 0.0f, 1.0f);
    case ParamPanA:
        return jmap(getChannelPan(0), -1.0f, 1.0f, 0.0f, 1.0f);
    case ParamPanB:
        return jmap(getChannelPan(1), -1.0f, 1.0f, 0.0f, 1.0f);
    default:
        return 0.0f;
    }
}

void MixerProcessor::setParameter(int parameterIndex, float newValue)
{
    switch (parameterIndex)
    {
    case ParamGainA:
        setChannelGainDb(0, jmap(newValue, 0.0f, 1.0f, MinGainDb, MaxGainDb));
        break;
    case ParamGainB:
        setChannelGainDb(1, jmap(newValue, 0.0f, 1.0f, MinGainDb, MaxGainDb));
        break;
    case ParamPanA:
        setChannelPan(0, jmap(newValue, 0.0f, 1.0f, -1.0f, 1.0f));
        break;
    case ParamPanB:
        setChannelPan(1, jmap(newValue, 0.0f, 1.0f, -1.0f, 1.0f));
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
        return "Gain 1";
    case ParamGainB:
        return "Gain 2";
    case ParamPanA:
        return "Pan 1";
    case ParamPanB:
        return "Pan 2";
    default:
        return "";
    }
}

const String MixerProcessor::getParameterText(int parameterIndex)
{
    switch (parameterIndex)
    {
    case ParamGainA:
        return String(getChannelGainDb(0), 1) + " dB";
    case ParamGainB:
        return String(getChannelGainDb(1), 1) + " dB";
    case ParamPanA:
        return String(getChannelPan(0), 2);
    case ParamPanB:
        return String(getChannelPan(1), 2);
    default:
        return "";
    }
}

bool MixerProcessor::isInputChannelStereoPair(int channelIndex) const
{
    int currentCh = 0;
    const int n = numStrips_.load(std::memory_order_acquire);
    for (int i = 0; i < n; ++i)
    {
        const bool stereo = strips_[static_cast<size_t>(i)].stereo.load(std::memory_order_relaxed);
        const int channels = stereo ? 2 : 1;
        if (channelIndex >= currentCh && channelIndex < currentCh + channels)
            return stereo;
        currentCh += channels;
    }
    return false;
}

void MixerProcessor::getStateInformation(MemoryBlock& destData)
{
    XmlElement xml("MixerSettings");
    xml.setAttribute("version", 4);
    xml.setAttribute("numStrips", getNumStrips());
    xml.setAttribute("masterGain", static_cast<double>(masterGainDb.load(std::memory_order_relaxed)));
    xml.setAttribute("masterMute", masterMute.load(std::memory_order_relaxed));
    for (int ch = 0; ch < getNumStrips(); ++ch)
    {
        const auto& strip = strips_[static_cast<size_t>(ch)];
        const String prefix = "strip" + String(ch) + "_";
        xml.setAttribute(prefix + "gainDb", static_cast<double>(strip.gainDb.load(std::memory_order_relaxed)));
        xml.setAttribute(prefix + "pan", static_cast<double>(strip.pan.load(std::memory_order_relaxed)));
        xml.setAttribute(prefix + "mute", strip.mute.load(std::memory_order_relaxed));
        xml.setAttribute(prefix + "solo", strip.solo.load(std::memory_order_relaxed));
        xml.setAttribute(prefix + "stereo", strip.stereo.load(std::memory_order_relaxed));
        xml.setAttribute(prefix + "phase", strip.phaseInvert.load(std::memory_order_relaxed));
        xml.setAttribute(prefix + "name", strip.name);
    }
    copyXmlToBinary(xml, destData);
}

void MixerProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState == nullptr || !xmlState->hasTagName("MixerSettings"))
        return;

    const int version = xmlState->getIntAttribute("version", 1);
    if (version < 4)
    {
        setChannelGainDb(0, static_cast<float>(xmlState->getDoubleAttribute("gainA", 0.0)));
        setChannelGainDb(1, static_cast<float>(xmlState->getDoubleAttribute("gainB", 0.0)));
        setChannelPan(0, static_cast<float>(xmlState->getDoubleAttribute("panA", 0.0)));
        setChannelPan(1, static_cast<float>(xmlState->getDoubleAttribute("panB", 0.0)));
        setChannelMute(0, xmlState->getBoolAttribute("muteA", false));
        setChannelMute(1, xmlState->getBoolAttribute("muteB", false));
        setChannelSolo(0, xmlState->getBoolAttribute("soloA", false));
        setChannelSolo(1, xmlState->getBoolAttribute("soloB", false));
        setChannelPhaseInvert(0, xmlState->getBoolAttribute("phaseA", false));
        setChannelPhaseInvert(1, xmlState->getBoolAttribute("phaseB", false));
        masterGainDb.store(static_cast<float>(xmlState->getDoubleAttribute("masterGain", 0.0)),
                           std::memory_order_relaxed);
        masterMute.store(xmlState->getBoolAttribute("masterMute", false), std::memory_order_relaxed);
        return;
    }

    const int active = jlimit(1, MaxStrips, xmlState->getIntAttribute("numStrips", DefaultStrips));
    for (int ch = 0; ch < active; ++ch)
    {
        auto& strip = strips_[static_cast<size_t>(ch)];
        strip.resetDefaults(ch);
        const String prefix = "strip" + String(ch) + "_";
        strip.gainDb.store(static_cast<float>(xmlState->getDoubleAttribute(prefix + "gainDb", 0.0)),
                           std::memory_order_relaxed);
        strip.pan.store(static_cast<float>(xmlState->getDoubleAttribute(prefix + "pan", 0.0)),
                        std::memory_order_relaxed);
        strip.mute.store(xmlState->getBoolAttribute(prefix + "mute", false), std::memory_order_relaxed);
        strip.solo.store(xmlState->getBoolAttribute(prefix + "solo", false), std::memory_order_relaxed);
        strip.stereo.store(xmlState->getBoolAttribute(prefix + "stereo", true), std::memory_order_relaxed);
        strip.phaseInvert.store(xmlState->getBoolAttribute(prefix + "phase", false), std::memory_order_relaxed);
        strip.name = xmlState->getStringAttribute(prefix + "name", "Ch " + String(ch + 1));
    }

    numStrips_.store(active, std::memory_order_release);
    masterGainDb.store(static_cast<float>(xmlState->getDoubleAttribute("masterGain", 0.0)),
                       std::memory_order_relaxed);
    masterMute.store(xmlState->getBoolAttribute("masterMute", false), std::memory_order_relaxed);
    updateChannelConfig();
}

PedalboardProcessor::PinLayout MixerProcessor::getInputPinLayout() const
{
    PinLayout layout;
    const int firstRowTop = 26 + 31;
    const int n = numStrips_.load(std::memory_order_acquire);
    for (int i = 0; i < n; ++i)
    {
        const int rowTop = firstRowTop + i * kMixerStripRowHeight;
        const bool stereo = strips_[static_cast<size_t>(i)].stereo.load(std::memory_order_relaxed);
        if (stereo)
        {
            layout.pinY.push_back(rowTop + 8);
            layout.pinY.push_back(rowTop + 30);
        }
        else
        {
            layout.pinY.push_back(rowTop + 19);
        }
    }
    return layout;
}

PedalboardProcessor::PinLayout MixerProcessor::getOutputPinLayout() const
{
    PinLayout layout;
    const int firstRowTop = 26 + 31;
    const int n = numStrips_.load(std::memory_order_acquire);
    const int masterRowTop = firstRowTop + n * kMixerStripRowHeight;
    layout.pinY.push_back(masterRowTop + 8);
    layout.pinY.push_back(masterRowTop + 30);
    return layout;
}
