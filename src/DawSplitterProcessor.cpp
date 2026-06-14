//  DawSplitterProcessor.cpp - DAW-style N-channel splitter node.
//  --------------------------------------------------------------------------
//  Mirror of DawMixerProcessor: 1 stereo input -> N mono outputs.

#include "DawSplitterProcessor.h"

#include "ColourScheme.h"
#include "PluginComponent.h"

#include <functional>

//==============================================================================
// DawSplitterProcessor
//==============================================================================

DawSplitterProcessor::DawSplitterProcessor()
{
    for (int i = 0; i < DefaultStrips; ++i)
        strips_[static_cast<size_t>(i)].resetDefaults(i);

    numStrips_.store(DefaultStrips, std::memory_order_release);
    updateChannelConfig();
}

DawSplitterProcessor::~DawSplitterProcessor() {}

// Stereo support helper
int DawSplitterProcessor::countTotalOutputChannels() const
{
    int total = 0;
    int n = numStrips_.load(std::memory_order_acquire);
    for (int i = 0; i < n; ++i)
    {
        bool isStereo = strips_[static_cast<size_t>(i)].stereo.load(std::memory_order_relaxed);
        total += isStereo ? 2 : 1;
    }
    return total;
}

void DawSplitterProcessor::updateChannelConfig()
{
    int numInputChannels = 2; // stereo input
    int numOutputChannels = countTotalOutputChannels();
    setPlayConfigDetails(numInputChannels, numOutputChannels, getSampleRate(), getBlockSize());
}

// Lock-free: just bump the atomic counter + init defaults. No allocation.
void DawSplitterProcessor::addStrip()
{
    int n = numStrips_.load(std::memory_order_acquire);
    if (n >= MaxStrips)
        return;

    strips_[static_cast<size_t>(n)].resetDefaults(n);

    if (currentSampleRate_ > 0.0)
        stripDsp_[static_cast<size_t>(n)].init(currentSampleRate_);

    numStrips_.store(n + 1, std::memory_order_release);
    updateChannelConfig();
}

// Lock-free: just decrement the atomic counter. No deallocation.
void DawSplitterProcessor::removeStrip()
{
    int n = numStrips_.load(std::memory_order_acquire);
    if (n <= 1)
        return;

    numStrips_.store(n - 1, std::memory_order_release);
    updateChannelConfig();
}

DawSplitterProcessor::StripState* DawSplitterProcessor::getStrip(int index)
{
    if (index >= 0 && index < numStrips_.load(std::memory_order_acquire))
        return &strips_[static_cast<size_t>(index)];
    return nullptr;
}

const DawSplitterProcessor::StripState* DawSplitterProcessor::getStrip(int index) const
{
    if (index >= 0 && index < numStrips_.load(std::memory_order_acquire))
        return &strips_[static_cast<size_t>(index)];
    return nullptr;
}

//==============================================================================
// DSP
//==============================================================================

void DawSplitterProcessor::computeVuDecay(double sampleRate)
{
    currentSampleRate_ = sampleRate;
    double samplesFor300ms = sampleRate * 0.3;
    peakDecay_ = static_cast<float>(std::pow(0.001, 1.0 / samplesFor300ms));
}

void DawSplitterProcessor::prepareToPlay(double sampleRate, int /*samplesPerBlock*/)
{
    computeVuDecay(sampleRate);

    // Init ALL MaxStrips DSP instances (cheap, avoids any runtime allocation)
    int n = numStrips_.load(std::memory_order_acquire);
    for (int i = 0; i < MaxStrips; ++i)
    {
        stripDsp_[static_cast<size_t>(i)].init(sampleRate);

        if (i < n)
        {
            float gainLin =
                Decibels::decibelsToGain(strips_[static_cast<size_t>(i)].gainDb.load(std::memory_order_relaxed));
            stripDsp_[static_cast<size_t>(i)].smoothedGain.setCurrentAndTargetValue(gainLin);
        }
    }

    // Input VU
    inputVuDspL_.init(static_cast<float>(sampleRate));
    inputVuDspR_.init(static_cast<float>(sampleRate));
}

void DawSplitterProcessor::processBlock(AudioBuffer<float>& buffer, MidiBuffer& /*midi*/)
{
    const int numSamples = buffer.getNumSamples();
    const int ns = numStrips_.load(std::memory_order_acquire);
    const int totalOutputChannels = buffer.getNumChannels();

    if (ns == 0 || numSamples == 0)
    {
        buffer.clear();
        return;
    }

    // Read the stereo input (channels 0-1)
    const float* inL = buffer.getReadPointer(0);
    const float* inR = buffer.getReadPointer(jmin(1, totalOutputChannels - 1));

    // Input VU metering
    float inPkL = inputPeakL.load(std::memory_order_relaxed);
    float inPkR = inputPeakR.load(std::memory_order_relaxed);
    for (int i = 0; i < numSamples; ++i)
    {
        float absL = std::abs(inL[i]);
        float absR = std::abs(inR[i]);
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

    // Solo detection
    bool anySolo = false;
    for (int s = 0; s < ns; ++s)
    {
        if (strips_[static_cast<size_t>(s)].solo.load(std::memory_order_relaxed))
        {
            anySolo = true;
            break;
        }
    }

    // Process each output strip
    int currentOutputChannel = 0;

    for (int s = 0; s < ns; ++s)
    {
        auto& strip = strips_[static_cast<size_t>(s)];
        auto& dsp = stripDsp_[static_cast<size_t>(s)];

        bool isStereo = strip.stereo.load(std::memory_order_relaxed);
        int channelsNeeded = isStereo ? 2 : 1;

        // Skip if we run out of output channels
        if (currentOutputChannel + channelsNeeded > totalOutputChannels)
            break;

        float* dstL = buffer.getWritePointer(currentOutputChannel);
        float* dstR = isStereo ? buffer.getWritePointer(currentOutputChannel + 1) : nullptr;
        currentOutputChannel += channelsNeeded;

        // Read atomic state once per block
        bool mute = strip.mute.load(std::memory_order_relaxed);
        bool solo = strip.solo.load(std::memory_order_relaxed);
        bool phaseInv = strip.phaseInvert.load(std::memory_order_relaxed);
        float gainDb = strip.gainDb.load(std::memory_order_relaxed);
        float pan = strip.pan.load(std::memory_order_relaxed);
        bool effectiveMute = mute || (anySolo && !solo);

        // Update smoothed gain target
        float gainLin = Decibels::decibelsToGain(gainDb);
        dsp.smoothedGain.setTargetValue(gainLin);

        // Pan/Balance calculation
        float panL, panR;
        if (isStereo)
        {
            // Stereo Balance
            if (pan <= 0.0f)
            {
                panL = 1.0f;
                panR = 1.0f + pan;
            }
            else
            {
                panL = 1.0f - pan;
                panR = 1.0f;
            }
        }
        else
        {
            // Mono splitter: sum L+R to single output. Pan not applicable
            // (downstream routing determines stereo placement).
            panL = 1.0f;
            panR = 1.0f;
        }

        // Peak metering
        float peakL = strip.peakL.load(std::memory_order_relaxed);
        float peakR = strip.peakR.load(std::memory_order_relaxed);

        for (int i = 0; i < numSamples; ++i)
        {
            float gain = dsp.smoothedGain.getNextValue();

            float outL, outR;

            if (isStereo)
            {
                // Stereo pass-through with balance
                float l = inL[i];
                float r = inR[i];

                if (phaseInv)
                {
                    l = -l;
                    r = -r;
                }

                outL = l * gain * panL;
                outR = r * gain * panR;
            }
            else
            {
                // Sum to mono
                float sample = (inL[i] + inR[i]) * 0.5f;
                if (phaseInv)
                    sample = -sample;
                outL = sample * gain;
                outR = outL; // internal var only
            }

            // Peak detection
            float absL = std::abs(outL);
            float absR = std::abs(outR);

            peakL = (absL > peakL) ? absL : peakL * peakDecay_;
            if (isStereo)
                peakR = (absR > peakR) ? absR : peakR * peakDecay_;
            else
                peakR = peakL;

            if (effectiveMute)
            {
                dstL[i] = 0.0f;
                if (dstR)
                    dstR[i] = 0.0f;
            }
            else
            {
                dstL[i] = outL;
                if (dstR)
                    dstR[i] = outR;
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

    // Clear unused output channels beyond what we wrote
    for (int ch = currentOutputChannel; ch < totalOutputChannels; ++ch)
        buffer.clear(ch, 0, numSamples);
}

//==============================================================================
// Shared VU painting helper (used by both SplitterStripRow and SplitterInputRow)
//==============================================================================
static void paintStereoVUHelper(Graphics& g, Rectangle<int> area, float peakL, float peakR)
{
    if (area.isEmpty())
        return;
    int halfH = area.getHeight() / 2;
    auto barL = area.removeFromTop(halfH).reduced(0, 1);
    auto barR = area.reduced(0, 1);

    auto drawBar = [&](Rectangle<int> bar, float peak)
    {
        float dbVal = Decibels::gainToDecibels(peak, -60.0f);
        float norm = jlimit(0.0f, 1.0f, (dbVal + 60.0f) / 72.0f);
        g.setColour(Colour(0xFF1A1A1A));
        g.fillRect(bar);
        int fillW = static_cast<int>(norm * bar.getWidth());
        auto filled = bar.withWidth(fillW);
        if (dbVal > 0.0f)
            g.setColour(ColourScheme::getInstance().colours["Danger Colour"]);
        else if (dbVal > -6.0f)
            g.setColour(ColourScheme::getInstance().colours["Warning Colour"]);
        else if (dbVal > -18.0f)
            g.setColour(Colour(0xFF00CC00));
        else
            g.setColour(Colour(0xFF008800));
        g.fillRect(filled);
    };

    drawBar(barL, peakL);
    drawBar(barR, peakR);
}

static Colour getRoutingAccent()
{
    return ColourScheme::getInstance().colours["Graph Category Dynamics"];
}

static float normalisePeakForMeter(float peak)
{
    const float dbVal = Decibels::gainToDecibels(jmax(0.0f, peak), -60.0f);
    return jlimit(0.0f, 1.0f, (dbVal + 60.0f) / 72.0f);
}

static void paintRoutingBadge(Graphics& g, Rectangle<float> bounds, const String& text, Colour accent, bool primary)
{
    auto& colours = ColourScheme::getInstance().colours;
    const auto bg = primary ? accent.withAlpha(0.20f) : colours["Plugin Background"].brighter(0.10f);
    ColourGradient fill(bg.brighter(0.12f), bounds.getX(), bounds.getY(), bg.darker(0.26f), bounds.getX(),
                        bounds.getBottom(), false);
    fill.addColour(0.48, bg);
    g.setGradientFill(fill);
    g.fillRoundedRectangle(bounds, 5.0f);

    g.setColour(accent.withAlpha(primary ? 0.72f : 0.42f));
    g.drawRoundedRectangle(bounds.reduced(0.5f), 5.0f, 1.0f);

    g.setColour((primary ? accent : colours["Text Colour"]).withAlpha(primary ? 0.95f : 0.72f));
    g.setFont(Font(10.5f, Font::bold));
    g.drawText(text, bounds.toNearestInt(), Justification::centred, false);
}

static void paintRoutingShell(Graphics& g, Rectangle<float> bounds, Colour accent, const String& title)
{
    auto& colours = ColourScheme::getInstance().colours;
    const auto outer = bounds;
    const auto base = colours["Plugin Background"];
    ColourGradient shell(base.brighter(0.10f), bounds.getX(), bounds.getY(), base.darker(0.18f), bounds.getX(),
                         bounds.getBottom(), false);
    shell.addColour(0.32, base.brighter(0.03f));
    shell.addColour(0.74, base.darker(0.08f));
    g.setGradientFill(shell);
    g.fillRoundedRectangle(bounds.reduced(0.5f), 8.0f);

    auto header = bounds.removeFromTop(25.0f).reduced(5.0f, 4.0f);
    ColourGradient headerFill(accent.withAlpha(0.20f), header.getX(), header.getY(), colours["Plugin Background"],
                              header.getX(), header.getBottom(), false);
    g.setGradientFill(headerFill);
    g.fillRoundedRectangle(header, 6.0f);

    g.setColour(accent.withAlpha(0.46f));
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

static void paintInsetMeterTrack(Graphics& g, Rectangle<float> bounds, float peak, Colour accent, bool stereo)
{
    auto& colours = ColourScheme::getInstance().colours;
    if (bounds.isEmpty())
        return;

    g.setColour(colours["Plugin Background"].darker(0.35f));
    g.fillRoundedRectangle(bounds, 4.0f);
    g.setColour(Colours::black.withAlpha(0.45f));
    g.drawRoundedRectangle(bounds.reduced(0.5f), 4.0f, 1.0f);

    const auto norm = normalisePeakForMeter(peak);
    auto fill = bounds.reduced(1.5f).withWidth(bounds.reduced(1.5f).getWidth() * norm);
    ColourGradient meter(accent.withAlpha(0.62f), fill.getX(), fill.getCentreY(),
                         ColourScheme::getInstance().colours["Warning Colour"].withAlpha(0.74f), fill.getRight(),
                         fill.getCentreY(), false);
    meter.addColour(0.76, accent.withAlpha(0.72f));
    g.setGradientFill(meter);
    g.fillRoundedRectangle(fill, 3.0f);

    if (stereo)
    {
        g.setColour(Colours::white.withAlpha(0.15f));
        g.drawLine(bounds.getX() + 2.0f, bounds.getCentreY(), bounds.getRight() - 2.0f, bounds.getCentreY(), 0.7f);
    }
}

static void paintSplitterFanout(Graphics& g, Rectangle<float> bounds, int outputs, Colour accent)
{
    if (bounds.isEmpty())
        return;

    auto& colours = ColourScheme::getInstance().colours;
    g.setColour(colours["Plugin Background"].darker(0.22f));
    g.fillRoundedRectangle(bounds.reduced(7.0f, 2.0f), 6.0f);

    const int count = jmax(1, outputs);
    const float startX = bounds.getX() + 34.0f;
    const float startY = bounds.getCentreY();
    const float endX = bounds.getRight() - 34.0f;
    Path fan;
    for (int i = 0; i < count; ++i)
    {
        const float t = count > 1 ? static_cast<float>(i) / static_cast<float>(count - 1) : 0.5f;
        const float endY = jmap(t, bounds.getY() + 5.0f, bounds.getBottom() - 5.0f);
        fan.startNewSubPath(startX, startY);
        fan.cubicTo(startX + 56.0f, startY, endX - 60.0f, endY, endX, endY);
    }

    g.setColour(accent.withAlpha(0.36f));
    g.strokePath(fan, PathStrokeType(1.8f, PathStrokeType::curved, PathStrokeType::rounded));
    g.setColour(accent.withAlpha(0.75f));
    g.fillEllipse(startX - 3.0f, startY - 3.0f, 6.0f, 6.0f);
}

//==============================================================================
// UI -- Horizontal Strip Row (same layout as DawMixerProcessor)
//==============================================================================

class SplitterStripRow : public Component
{
  public:
    SplitterStripRow(DawSplitterProcessor* proc, int stripIndex, std::function<void()> onLayoutChange)
        : processor(proc), index(stripIndex), layoutChangeCallback(std::move(onLayoutChange))
    {
        stereoBtn.setButtonText("ST");
        stereoBtn.setClickingTogglesState(true);
        stereoBtn.setColour(TextButton::buttonOnColourId, Colours::cyan);
        stereoBtn.setColour(TextButton::buttonColourId, Colour(0xFF505050)); // Explicit off-state grey
        stereoBtn.setColour(TextButton::textColourOffId, Colours::white);
        stereoBtn.setColour(TextButton::textColourOnId, Colours::black);
        stereoBtn.setTooltip("Toggle Stereo/Mono Strip");
        stereoBtn.onClick = [this]()
        {
            if (auto* s = processor->getStrip(index))
            {
                bool newState = stereoBtn.getToggleState();
                if (s->stereo.load(std::memory_order_relaxed) != newState)
                {
                    s->stereo.store(newState, std::memory_order_relaxed);
                    processor->updateChannelConfig();
                    if (layoutChangeCallback)
                        layoutChangeCallback();
                }
            }
        };
        addAndMakeVisible(stereoBtn);

        phaseBtn.setButtonText(CharPointer_UTF8("\xc3\x98"));
        phaseBtn.setClickingTogglesState(true);
        phaseBtn.setColour(TextButton::buttonOnColourId, Colour(0xFFFF8800));
        phaseBtn.onClick = [this]()
        {
            if (auto* s = processor->getStrip(index))
                s->phaseInvert.store(phaseBtn.getToggleState(), std::memory_order_relaxed);
        };
        addAndMakeVisible(phaseBtn);

        muteBtn.setButtonText("M");
        muteBtn.setClickingTogglesState(true);
        muteBtn.setColour(TextButton::buttonOnColourId, ColourScheme::getInstance().colours["Danger Colour"]);
        muteBtn.onClick = [this]()
        {
            if (auto* s = processor->getStrip(index))
                s->mute.store(muteBtn.getToggleState(), std::memory_order_relaxed);
        };
        addAndMakeVisible(muteBtn);

        soloBtn.setButtonText("S");
        soloBtn.setClickingTogglesState(true);
        soloBtn.setColour(TextButton::buttonOnColourId, Colour(0xFFCCAA00));
        soloBtn.onClick = [this]()
        {
            if (auto* s = processor->getStrip(index))
                s->solo.store(soloBtn.getToggleState(), std::memory_order_relaxed);
        };
        addAndMakeVisible(soloBtn);

        fader.setSliderStyle(Slider::LinearHorizontal);
        fader.setTextBoxStyle(Slider::TextBoxRight, false, 48, 18);
        fader.setRange(DawSplitterProcessor::MinGainDb, DawSplitterProcessor::MaxGainDb, 0.1);
        fader.setDoubleClickReturnValue(true, 0.0);
        fader.setSkewFactorFromMidPoint(-12.0);
        fader.onValueChange = [this]()
        {
            if (auto* s = processor->getStrip(index))
                s->gainDb.store(static_cast<float>(fader.getValue()), std::memory_order_relaxed);
        };
        addAndMakeVisible(fader);

        nameLabel.setFont(Font(11.0f));
        nameLabel.setJustificationType(Justification::centredLeft);
        nameLabel.setColour(Label::textColourId, Colours::white);
        addAndMakeVisible(nameLabel);

        syncFromProcessor();
    }

    void syncFromProcessor()
    {
        if (auto* s = processor->getStrip(index))
        {
            fader.setValue(s->gainDb.load(std::memory_order_relaxed), dontSendNotification);
            muteBtn.setToggleState(s->mute.load(std::memory_order_relaxed), dontSendNotification);
            soloBtn.setToggleState(s->solo.load(std::memory_order_relaxed), dontSendNotification);
            phaseBtn.setToggleState(s->phaseInvert.load(std::memory_order_relaxed), dontSendNotification);
            stereoBtn.setToggleState(s->stereo.load(std::memory_order_relaxed), dontSendNotification);
            nameLabel.setText(s->name, dontSendNotification);
        }
    }

    void resized() override
    {
        auto r = getLocalBounds().reduced(7, 5);
        auto top = r.removeFromTop(19);
        badgeArea = top.removeFromLeft(25).reduced(1);

        auto buttons = top.removeFromRight(92);
        stereoBtn.setBounds(buttons.removeFromLeft(25).reduced(1));
        phaseBtn.setBounds(buttons.removeFromLeft(21).reduced(1));
        muteBtn.setBounds(buttons.removeFromLeft(22).reduced(1));
        soloBtn.setBounds(buttons.removeFromLeft(22).reduced(1));
        nameLabel.setBounds(top.reduced(5, 0));

        auto meter = r.removeFromTop(12);
        vuArea = meter.reduced(31, 3);
        fader.setBounds(r.reduced(1, 2));
    }

    void paint(Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat().reduced(0.5f);
        auto& colours = ColourScheme::getInstance().colours;
        const auto accent = getRoutingAccent();
        ColourGradient stripFill(colours["Plugin Background"].brighter(0.07f), bounds.getX(), bounds.getY(),
                                 colours["Plugin Background"].darker(0.10f), bounds.getX(), bounds.getBottom(),
                                 false);
        stripFill.addColour(0.45, colours["Plugin Background"].brighter(0.01f));
        g.setGradientFill(stripFill);
        g.fillRoundedRectangle(bounds, 6.0f);
        g.setColour(accent.withAlpha(0.42f));
        g.drawRoundedRectangle(bounds.reduced(0.5f), 6.0f, 1.0f);
        g.setColour(Colours::white.withAlpha(0.045f));
        g.drawLine(bounds.getX() + 8.0f, bounds.getY() + 1.0f, bounds.getRight() - 8.0f, bounds.getY() + 1.0f);

        auto badge = badgeArea;
        paintRoutingBadge(g, badge.toFloat(), String(index + 1), accent, false);

        if (auto* s = processor->getStrip(index))
        {
            bool isStereo = s->stereo.load(std::memory_order_relaxed);
            float peakL = s->peakL.load(std::memory_order_relaxed);
            float peakForInset = peakL;
            if (isStereo)
            {
                float peakR = s->peakR.load(std::memory_order_relaxed);
                peakForInset = jmax(peakL, peakR);
                paintStereoVUHelper(g, vuArea, peakL, peakR);
            }
            else
            {
                paintMonoVU(g, vuArea, peakL);
            }
            paintInsetMeterTrack(g, vuArea.toFloat(), peakForInset, accent, isStereo);
        }
    }

    static void paintMonoVU(Graphics& g, Rectangle<int> area, float peak)
    {
        if (area.isEmpty())
            return;
        auto bar = area.reduced(0, 2);

        float dbVal = Decibels::gainToDecibels(peak, -60.0f);
        float norm = jlimit(0.0f, 1.0f, (dbVal + 60.0f) / 72.0f);
        g.setColour(Colour(0xFF1A1A1A));
        g.fillRect(bar);
        int fillW = static_cast<int>(norm * bar.getWidth());
        auto filled = bar.withWidth(fillW);
        if (dbVal > 0.0f)
            g.setColour(ColourScheme::getInstance().colours["Danger Colour"]);
        else if (dbVal > -6.0f)
            g.setColour(ColourScheme::getInstance().colours["Warning Colour"]);
        else if (dbVal > -18.0f)
            g.setColour(Colour(0xFF00CC00));
        else
            g.setColour(Colour(0xFF008800));
        g.fillRect(filled);
    }

  private:
    DawSplitterProcessor* processor;
    int index;
    std::function<void()> layoutChangeCallback;
    TextButton phaseBtn, muteBtn, soloBtn, stereoBtn;
    Slider fader;
    Label nameLabel;
    Rectangle<int> vuArea;
    Rectangle<int> badgeArea;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SplitterStripRow)
};

//==============================================================================
// Input VU row (replaces master row -- shows input signal)
class SplitterInputRow : public Component
{
  public:
    SplitterInputRow(DawSplitterProcessor* proc) : processor(proc)
    {
        nameLabel.setText("Input", dontSendNotification);
        nameLabel.setFont(Font(11.0f));
        nameLabel.setJustificationType(Justification::centredLeft);
        nameLabel.setColour(Label::textColourId, Colour(0xFF88CCFF));
        addAndMakeVisible(nameLabel);
    }

    void resized() override
    {
        auto r = getLocalBounds().reduced(7, 5);
        badgeArea = r.removeFromLeft(35).reduced(1, 12);
        nameLabel.setBounds(r.removeFromLeft(56).reduced(5, 0));
        vuArea = r.reduced(0, 20);
    }

    void paint(Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat().reduced(0.5f);
        auto accent = getRoutingAccent();
        auto& colours = ColourScheme::getInstance().colours;
        ColourGradient stripFill(colours["Plugin Background"].brighter(0.12f), bounds.getX(), bounds.getY(),
                                 colours["Plugin Background"].darker(0.13f), bounds.getX(), bounds.getBottom(),
                                 false);
        g.setGradientFill(stripFill);
        g.fillRoundedRectangle(bounds, 6.0f);
        g.setColour(accent.withAlpha(0.55f));
        g.drawRoundedRectangle(bounds.reduced(0.5f), 6.0f, 1.1f);

        auto badge = badgeArea;
        paintRoutingBadge(g, badge.toFloat(), "IN", accent, true);
        float peakL = processor->inputPeakL.load(std::memory_order_relaxed);
        float peakR = processor->inputPeakR.load(std::memory_order_relaxed);
        paintInsetMeterTrack(g, vuArea.toFloat(), jmax(peakL, peakR), accent, true);
    }

  private:
    DawSplitterProcessor* processor;
    Label nameLabel;
    Rectangle<int> vuArea;
    Rectangle<int> badgeArea;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SplitterInputRow)
};

//==============================================================================
// Main control -- returned by getControls()
class DawSplitterControl : public Component, private Timer
{
  public:
    DawSplitterControl(DawSplitterProcessor* proc) : processor(proc)
    {
        addBtn.setButtonText("+");
        addBtn.onClick = [this]() { addStripClicked(); };
        addAndMakeVisible(addBtn);

        removeBtn.setButtonText("-");
        removeBtn.onClick = [this]() { removeStripClicked(); };
        addAndMakeVisible(removeBtn);

        inputRow = std::make_unique<SplitterInputRow>(processor);
        addAndMakeVisible(inputRow.get());

        rebuildStrips();
        startTimerHz(24);
    }

    ~DawSplitterControl() override { stopTimer(); }

    void rebuildStrips()
    {
        stripRows.clear();
        int n = processor->getNumStrips();
        for (int i = 0; i < n; ++i)
        {
            auto* row = new SplitterStripRow(processor, i, [this]() { notifyParentResize(); });
            addAndMakeVisible(row);
            stripRows.add(row);
        }

        // Resize ourselves to match the new strip count
        auto newSize = processor->getSize();
        setSize(newSize.getX(), newSize.getY());

        resized();
    }

    void resized() override
    {
        auto r = getLocalBounds();
        auto header = r.removeFromTop(24);
        removeBtn.setBounds(header.removeFromRight(24));
        addBtn.setBounds(header.removeFromRight(24));

        // Input row at top (instead of master at bottom for mixer)
        inputRow->setBounds(r.removeFromTop(stripRowHeight));
        fanArea = r.removeFromTop(20).reduced(7, 0);

        for (auto* row : stripRows)
            row->setBounds(r.removeFromTop(stripRowHeight));
    }

    void paint(Graphics& g) override
    {
        paintRoutingShell(g, getLocalBounds().toFloat(), getRoutingAccent(), {});
        paintSplitterFanout(g, fanArea.toFloat(), processor->getNumStrips(), getRoutingAccent());
    }

  private:
    static constexpr int stripRowHeight = 58;
    DawSplitterProcessor* processor;
    TextButton addBtn, removeBtn;
    OwnedArray<SplitterStripRow> stripRows;
    std::unique_ptr<SplitterInputRow> inputRow;
    Rectangle<int> fanArea;

    void addStripClicked()
    {
        processor->addStrip();
        rebuildStrips();
        notifyParentResize();
    }

    void removeStripClicked()
    {
        processor->removeStrip();
        rebuildStrips();
        notifyParentResize();
    }

    void notifyParentResize()
    {
        // Walk up the component tree to find the PluginComponent that hosts us
        if (auto* pc = findParentComponentOfClass<PluginComponent>())
            pc->refreshPins();
    }

    void timerCallback() override
    {
        for (auto* row : stripRows)
            row->repaint();
        inputRow->repaint();
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DawSplitterControl)
};

Component* DawSplitterProcessor::getControls()
{
    return new DawSplitterControl(this);
}

Point<int> DawSplitterProcessor::getSize()
{
    int n = numStrips_.load(std::memory_order_acquire);
    int height = 24 + 58 + 20 + n * 58; // header + input row + fanout + strips
    return Point<int>(360, jmax(170, height));
}

//==============================================================================
// State Serialization
//==============================================================================

void DawSplitterProcessor::getStateInformation(MemoryBlock& destData)
{
    XmlElement xml("DawSplitter");
    xml.setAttribute("version", 1);

    int n = numStrips_.load(std::memory_order_acquire);
    xml.setAttribute("numStrips", n);

    for (int i = 0; i < n; ++i)
    {
        auto& s = strips_[static_cast<size_t>(i)];
        auto* stripXml = xml.createNewChildElement("Strip");
        stripXml->setAttribute("i", i);
        stripXml->setAttribute("gain", (double)s.gainDb.load(std::memory_order_relaxed));
        stripXml->setAttribute("pan", (double)s.pan.load(std::memory_order_relaxed));
        stripXml->setAttribute("mute", s.mute.load(std::memory_order_relaxed) ? 1 : 0);
        stripXml->setAttribute("solo", s.solo.load(std::memory_order_relaxed) ? 1 : 0);
        stripXml->setAttribute("phase", s.phaseInvert.load(std::memory_order_relaxed) ? 1 : 0);
        stripXml->setAttribute("stereo", s.stereo.load(std::memory_order_relaxed) ? 1 : 0);
        stripXml->setAttribute("name", s.name);
    }

    copyXmlToBinary(xml, destData);
}

void DawSplitterProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    auto xml = getXmlFromBinary(data, sizeInBytes);
    if (!xml || xml->getTagName() != "DawSplitter")
        return;

    int n = xml->getIntAttribute("numStrips", DefaultStrips);
    n = jlimit(1, MaxStrips, n);

    for (int i = 0; i < n; ++i)
        strips_[static_cast<size_t>(i)].resetDefaults(i);

    numStrips_.store(n, std::memory_order_release);

    for (auto* stripXml : xml->getChildWithTagNameIterator("Strip"))
    {
        int i = stripXml->getIntAttribute("i", -1);
        if (i >= 0 && i < n)
        {
            auto& s = strips_[static_cast<size_t>(i)];
            float g = static_cast<float>(stripXml->getDoubleAttribute("gain", 0.0));
            s.gainDb.store(g, std::memory_order_relaxed);
            float p = static_cast<float>(stripXml->getDoubleAttribute("pan", 0.0));
            s.pan.store(p, std::memory_order_relaxed);
            bool m = stripXml->getIntAttribute("mute", 0) != 0;
            s.mute.store(m, std::memory_order_relaxed);
            bool solo = stripXml->getIntAttribute("solo", 0) != 0;
            s.solo.store(solo, std::memory_order_relaxed);
            s.phaseInvert.store(stripXml->getIntAttribute("phase", 0) != 0, std::memory_order_relaxed);
            s.stereo.store(stripXml->getIntAttribute("stereo", 0) != 0, std::memory_order_relaxed);
            s.name = stripXml->getStringAttribute("name", "Out " + String(i + 1));
        }
    }

    updateChannelConfig();
}

//==============================================================================
// Plugin Description
//==============================================================================

//==============================================================================
// Pin Layout
//==============================================================================

PedalboardProcessor::PinLayout DawSplitterProcessor::getInputPinLayout() const
{
    // Input row is always stereo. In PC coords:
    // PC title=24, control at PC Y=24, control header=24
    // => input row top = 48
    // Within a 58px row: L center = +16, R center = +42
    PinLayout layout;
    layout.pinY.push_back(48 + 8);  // L (pin top = center - 8)
    layout.pinY.push_back(48 + 34); // R
    return layout;
}

PedalboardProcessor::PinLayout DawSplitterProcessor::getOutputPinLayout() const
{
    // Strip rows start after input row and fanout lane.
    // strip row i top in PC coords = 48 + 58 + 20 + i * 58 = 126 + i * 58
    PinLayout layout;
    int n = numStrips_.load(std::memory_order_acquire);
    for (int i = 0; i < n; ++i)
    {
        int rowTop = 126 + i * 58;
        bool isStereo = strips_[static_cast<size_t>(i)].stereo.load(std::memory_order_relaxed);
        if (isStereo)
        {
            layout.pinY.push_back(rowTop + 8);  // L
            layout.pinY.push_back(rowTop + 34); // R
        }
        else
        {
            layout.pinY.push_back(rowTop + 21); // Mono centered
        }
    }
    return layout;
}

bool DawSplitterProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainInputChannelSet() != AudioChannelSet::stereo())
        return false;
    return true;
}

const String DawSplitterProcessor::getInputChannelName(int channelIndex) const
{
    return channelIndex == 0 ? "Input L" : "Input R";
}

const String DawSplitterProcessor::getOutputChannelName(int channelIndex) const
{
    if (channelIndex < countTotalOutputChannels())
    {
        // Recursively find which strip this channel belongs to
        int currentCh = 0;
        int n = numStrips_.load(std::memory_order_acquire);
        for (int i = 0; i < n; ++i)
        {
            auto& s = strips_[static_cast<size_t>(i)];
            bool isStereo = s.stereo.load(std::memory_order_relaxed);
            int chans = isStereo ? 2 : 1;

            if (channelIndex < currentCh + chans)
            {
                if (isStereo)
                    return s.name + (channelIndex == currentCh ? " L" : " R");
                return s.name;
            }
            currentCh += chans;
        }
    }
    return "Output " + String(channelIndex + 1);
}

void DawSplitterProcessor::fillInPluginDescription(PluginDescription& description) const
{
    description.name = getName();
    description.descriptiveName = "DAW-style N-channel splitter";
    description.pluginFormatName = "Internal";
    description.category = "Built-in";
    description.manufacturerName = "Pedalboard";
    description.version = "1.0";
    description.fileOrIdentifier = getName();
    description.isInstrument = false;
    description.numInputChannels = getTotalNumInputChannels();
    description.numOutputChannels = getTotalNumOutputChannels();
}
