//  DawMixerProcessor.cpp - DAW-style N-channel mixing console node.
//  --------------------------------------------------------------------------

#include "DawMixerProcessor.h"

#include "ColourScheme.h"
#include "PluginComponent.h"

#include <functional>
#include <spdlog/spdlog.h>

//==============================================================================
// DawMixerProcessor
//==============================================================================

DawMixerProcessor::DawMixerProcessor()
{
    // Initialize default strips
    for (int i = 0; i < DefaultStrips; ++i)
        strips_[static_cast<size_t>(i)].resetDefaults(i);

    numStrips_.store(DefaultStrips, std::memory_order_release);
    updateChannelConfig();
}

DawMixerProcessor::~DawMixerProcessor() {}

// Stereo support helper
int DawMixerProcessor::countTotalInputChannels() const
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

void DawMixerProcessor::updateChannelConfig()
{
    int numInputChannels = countTotalInputChannels();
    int numOutputChannels = 2; // stereo master
    setPlayConfigDetails(numInputChannels, numOutputChannels, getSampleRate(), getBlockSize());
}

// Lock-free: just bump the atomic counter + init defaults. No allocation.
void DawMixerProcessor::addStrip()
{
    int n = numStrips_.load(std::memory_order_acquire);
    if (n >= MaxStrips)
        return;

    // Init the new strip's defaults (message thread only writes to strips_[n])
    strips_[static_cast<size_t>(n)].resetDefaults(n);

    // Init DSP for the new strip
    if (currentSampleRate_ > 0.0)
        stripDsp_[static_cast<size_t>(n)].init(currentSampleRate_);

    // Publish the new count -- audio thread will see this and include the new strip
    numStrips_.store(n + 1, std::memory_order_release);
    updateChannelConfig();
}

// Lock-free: just decrement the atomic counter. No deallocation.
void DawMixerProcessor::removeStrip()
{
    int n = numStrips_.load(std::memory_order_acquire);
    if (n <= 1)
        return;

    // Just shrink the active count -- audio thread will stop reading beyond n-1
    numStrips_.store(n - 1, std::memory_order_release);
    updateChannelConfig();
}

DawMixerProcessor::StripState* DawMixerProcessor::getStrip(int index)
{
    if (index >= 0 && index < numStrips_.load(std::memory_order_acquire))
        return &strips_[static_cast<size_t>(index)];
    return nullptr;
}

const DawMixerProcessor::StripState* DawMixerProcessor::getStrip(int index) const
{
    if (index >= 0 && index < numStrips_.load(std::memory_order_acquire))
        return &strips_[static_cast<size_t>(index)];
    return nullptr;
}

//==============================================================================
// DSP
//==============================================================================

void DawMixerProcessor::computeVuDecay(double sampleRate)
{
    currentSampleRate_ = sampleRate;
    // Peak decay: ~300ms from peak to -60dB
    double samplesFor300ms = sampleRate * 0.3;
    peakDecay_ = static_cast<float>(std::pow(0.001, 1.0 / samplesFor300ms));
}

void DawMixerProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    computeVuDecay(sampleRate);

    // Init ALL MaxStrips DSP instances (cheap, avoids any runtime allocation)
    int n = numStrips_.load(std::memory_order_acquire);
    for (int i = 0; i < MaxStrips; ++i)
    {
        stripDsp_[static_cast<size_t>(i)].init(sampleRate);

        // Snap smoothed gain to current value for active strips
        if (i < n)
        {
            float gainLin =
                Decibels::decibelsToGain(strips_[static_cast<size_t>(i)].gainDb.load(std::memory_order_relaxed));
            stripDsp_[static_cast<size_t>(i)].smoothedGain.setCurrentAndTargetValue(gainLin);
        }
    }

    // Master gain
    smoothedMasterGain_.reset(sampleRate, GainRampSeconds);
    float mGain = Decibels::decibelsToGain(masterGainDb.load(std::memory_order_relaxed));
    smoothedMasterGain_.setCurrentAndTargetValue(mGain);

    // Master VU
    masterVuDspL_.init(static_cast<float>(sampleRate));
    masterVuDspR_.init(static_cast<float>(sampleRate));

    // Pre-allocate temp buffer for mixing
    tempBuffer_.setSize(2, samplesPerBlock, false, true, true);
}

void DawMixerProcessor::processBlock(AudioBuffer<float>& buffer, MidiBuffer& /*midi*/)
{
    const int numSamples = buffer.getNumSamples();
    const int ns = numStrips_.load(std::memory_order_acquire);
    const int totalInputChannels = buffer.getNumChannels();

    if (ns == 0 || numSamples == 0)
    {
        buffer.clear();
        return;
    }

    // tempBuffer_ sized in prepareToPlay -- assert never exceeded on RT thread
    jassert(tempBuffer_.getNumSamples() >= numSamples);

    tempBuffer_.clear();

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

    float* mixL = tempBuffer_.getWritePointer(0);
    float* mixR = tempBuffer_.getWritePointer(1);

    int currentInputChannel = 0;

    // Process each strip
    for (int s = 0; s < ns; ++s)
    {
        auto& strip = strips_[static_cast<size_t>(s)];
        auto& dsp = stripDsp_[static_cast<size_t>(s)];

        bool isStereo = strip.stereo.load(std::memory_order_relaxed);
        int channelsNeeded = isStereo ? 2 : 1;

        // Skip if we run out of input channels
        if (currentInputChannel + channelsNeeded > totalInputChannels)
            break;

        const float* srcL = buffer.getReadPointer(currentInputChannel);
        const float* srcR = isStereo ? buffer.getReadPointer(currentInputChannel + 1) : nullptr;
        currentInputChannel += channelsNeeded;

        // Read atomic state once per block
        bool mute = strip.mute.load(std::memory_order_relaxed);
        bool solo = strip.solo.load(std::memory_order_relaxed);
        bool phaseInv = strip.phaseInvert.load(std::memory_order_relaxed);
        float pan = strip.pan.load(std::memory_order_relaxed);
        float gainDb = strip.gainDb.load(std::memory_order_relaxed);
        bool effectiveMute = mute || (anySolo && !solo);

        // Update smoothed gain target
        float gainLin = Decibels::decibelsToGain(gainDb);
        dsp.smoothedGain.setTargetValue(gainLin);

        // Pan/Balance calculation
        float panL, panR;
        if (isStereo)
        {
            // Stereo Balance: center=1/1, left=1/att, right=att/1
            if (pan <= 0.0f)
            {
                panL = 1.0f;
                panR = 1.0f + pan; // pan is negative, so 1-1 = 0 at full left
            }
            else
            {
                panL = 1.0f - pan;
                panR = 1.0f;
            }
        }
        else
        {
            // Mono Pan: equal-power -3dB
            panL = std::sqrt(0.5f * (1.0f - pan));
            panR = std::sqrt(0.5f * (1.0f + pan));
        }

        // Peak metering state
        float peakL = strip.peakL.load(std::memory_order_relaxed);
        float peakR = strip.peakR.load(std::memory_order_relaxed);

        for (int i = 0; i < numSamples; ++i)
        {
            float gain = dsp.smoothedGain.getNextValue();

            float l, r;
            if (isStereo)
            {
                l = srcL[i];
                r = srcR[i];
            }
            else
            {
                l = srcL[i];
                r = l; // mono source split to stereo
            }

            if (phaseInv)
            {
                l = -l;
                r = -r;
            }

            l *= gain * panL;
            r *= gain * panR;

            // Peak detection (post-gain/pan, pre-mute)
            float absL = std::abs(l);
            float absR = std::abs(r);
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

    // Master bus
    float mGainDb = masterGainDb.load(std::memory_order_relaxed);
    float mGainLin = Decibels::decibelsToGain(mGainDb);
    smoothedMasterGain_.setTargetValue(mGainLin);
    bool mMute = masterMute.load(std::memory_order_relaxed);

    float masterPkL = masterPeakL.load(std::memory_order_relaxed);
    float masterPkR = masterPeakR.load(std::memory_order_relaxed);

    float* outL = buffer.getWritePointer(0);
    float* outR = buffer.getWritePointer(1);

    for (int i = 0; i < numSamples; ++i)
    {
        float gain = smoothedMasterGain_.getNextValue();
        float l = mixL[i] * gain;
        float r = mixR[i] * gain;

        if (mMute)
        {
            l = 0.0f;
            r = 0.0f;
        }

        outL[i] = l;
        outR[i] = r;

        float absL = std::abs(l);
        float absR = std::abs(r);
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

    // Clear unused output channels
    for (int ch = 2; ch < buffer.getNumChannels(); ++ch)
        buffer.clear(ch, 0, numSamples);
}

//==============================================================================
// Shared VU painting helper (used by both DawStripRow and DawMasterRow)
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

//==============================================================================
// UI -- Horizontal Strip Row
//==============================================================================

class DawStripRow : public Component
{
  public:
    DawStripRow(DawMixerProcessor* proc, int stripIndex, std::function<void()> onLayoutChange)
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
        fader.setRange(DawMixerProcessor::MinGainDb, DawMixerProcessor::MaxGainDb, 0.1);
        fader.setDoubleClickReturnValue(true, 0.0);
        fader.setSkewFactorFromMidPoint(-12.0);
        fader.onValueChange = [this]()
        {
            auto* s = processor->getStrip(index);
            if (s)
                s->gainDb.store(static_cast<float>(fader.getValue()), std::memory_order_relaxed);
            else
                spdlog::warn("[DawMixer] fader change: index={} getStrip RETURNED NULL! numStrips={}", index,
                             processor->getNumStrips());
        };
        addAndMakeVisible(fader);

        panKnob.setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
        panKnob.setTextBoxStyle(Slider::NoTextBox, false, 0, 0);
        panKnob.setRange(-1.0, 1.0, 0.01);
        panKnob.setDoubleClickReturnValue(true, 0.0);
        panKnob.onValueChange = [this]()
        {
            if (auto* s = processor->getStrip(index))
                s->pan.store(static_cast<float>(panKnob.getValue()), std::memory_order_relaxed);
        };
        addAndMakeVisible(panKnob);

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
            panKnob.setValue(s->pan.load(std::memory_order_relaxed), dontSendNotification);
            muteBtn.setToggleState(s->mute.load(std::memory_order_relaxed), dontSendNotification);
            soloBtn.setToggleState(s->solo.load(std::memory_order_relaxed), dontSendNotification);
            phaseBtn.setToggleState(s->phaseInvert.load(std::memory_order_relaxed), dontSendNotification);
            stereoBtn.setToggleState(s->stereo.load(std::memory_order_relaxed), dontSendNotification);
            nameLabel.setText(s->name, dontSendNotification);
        }
    }

    void resized() override
    {
        auto r = getLocalBounds().reduced(2, 1);
        int halfH = r.getHeight() / 2;
        auto row1 = r.removeFromTop(halfH);
        auto row2 = r;

        // Row 1: [name 30] [ST 24] [Ph 22] [M 22] [S 22] [gap 4] [VU rest]
        nameLabel.setBounds(row1.removeFromLeft(30));
        stereoBtn.setBounds(row1.removeFromLeft(28));
        phaseBtn.setBounds(row1.removeFromLeft(22));
        muteBtn.setBounds(row1.removeFromLeft(22));
        soloBtn.setBounds(row1.removeFromLeft(22));
        row1.removeFromLeft(4);
        vuArea = row1;

        // Row 2: [fader rest] [pan 28]
        panKnob.setBounds(row2.removeFromRight(28));
        fader.setBounds(row2);
    }

    void paint(Graphics& g) override
    {
        g.setColour(Colour(0xFF2A2A2A));
        g.fillRect(getLocalBounds());
        g.setColour(Colour(0xFF404040));
        g.drawHorizontalLine(getHeight() - 1, 0.0f, static_cast<float>(getWidth()));

        auto* s = processor->getStrip(index);
        if (!s)
        {
            // DEBUG: log once per strip
            static bool logged[32] = {};
            if (index < 32 && !logged[index])
            {
                spdlog::warn("[DawMixer] paint: getStrip({}) returned NULL! numStrips={}", index,
                             processor->getNumStrips());
                logged[index] = true;
            }
        }
        else
        {
            bool isStereo = s->stereo.load(std::memory_order_relaxed);
            float peakL = s->peakL.load(std::memory_order_relaxed);

            if (isStereo)
            {
                float peakR = s->peakR.load(std::memory_order_relaxed);
                paintStereoVUHelper(g, vuArea, peakL, peakR);
            }
            else
            {
                paintMonoVU(g, vuArea, peakL);
            }
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
    DawMixerProcessor* processor;
    int index;
    std::function<void()> layoutChangeCallback;
    TextButton phaseBtn, muteBtn, soloBtn, stereoBtn;
    Slider fader, panKnob;
    Label nameLabel;
    Rectangle<int> vuArea;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DawStripRow)
};

//==============================================================================
// Master strip row (output)
class DawMasterRow : public Component
{
  public:
    DawMasterRow(DawMixerProcessor* proc) : processor(proc)
    {
        muteBtn.setButtonText("M");
        muteBtn.setClickingTogglesState(true);
        muteBtn.setColour(TextButton::buttonOnColourId, ColourScheme::getInstance().colours["Danger Colour"]);
        muteBtn.onClick = [this]()
        { processor->masterMute.store(muteBtn.getToggleState(), std::memory_order_relaxed); };
        addAndMakeVisible(muteBtn);

        fader.setSliderStyle(Slider::LinearHorizontal);
        fader.setTextBoxStyle(Slider::TextBoxRight, false, 48, 18);
        fader.setRange(DawMixerProcessor::MinGainDb, DawMixerProcessor::MaxGainDb, 0.1);
        fader.setDoubleClickReturnValue(true, 0.0);
        fader.setSkewFactorFromMidPoint(-12.0);
        fader.setValue(processor->masterGainDb.load(std::memory_order_relaxed), dontSendNotification);
        fader.onValueChange = [this]()
        { processor->masterGainDb.store(static_cast<float>(fader.getValue()), std::memory_order_relaxed); };
        addAndMakeVisible(fader);

        nameLabel.setText("Master", dontSendNotification);
        nameLabel.setFont(Font(11.0f));
        nameLabel.setJustificationType(Justification::centredLeft);
        nameLabel.setColour(Label::textColourId, Colour(0xFFFFCC00));
        addAndMakeVisible(nameLabel);
    }

    void resized() override
    {
        auto r = getLocalBounds().reduced(2, 1);
        int halfH = r.getHeight() / 2;
        auto row1 = r.removeFromTop(halfH);
        auto row2 = r;
        nameLabel.setBounds(row1.removeFromLeft(46));
        muteBtn.setBounds(row1.removeFromLeft(22));
        row1.removeFromLeft(4);
        vuArea = row1;
        fader.setBounds(row2);
    }

    void paint(Graphics& g) override
    {
        g.setColour(Colour(0xFF333333));
        g.fillRect(getLocalBounds());
        g.setColour(Colour(0xFF606060));
        g.drawHorizontalLine(0, 0.0f, static_cast<float>(getWidth()));
        float peakL = processor->masterPeakL.load(std::memory_order_relaxed);
        float peakR = processor->masterPeakR.load(std::memory_order_relaxed);
        paintStereoVUHelper(g, vuArea, peakL, peakR);
    }

  private:
    DawMixerProcessor* processor;
    TextButton muteBtn;
    Slider fader;
    Label nameLabel;
    Rectangle<int> vuArea;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DawMasterRow)
};

//==============================================================================
// Main control -- returned by getControls()
class DawMixerControl : public Component, private Timer
{
  public:
    DawMixerControl(DawMixerProcessor* proc) : processor(proc)
    {
        titleLabel.setText("DAW Mixer", dontSendNotification);
        titleLabel.setFont(Font(13.0f, Font::bold));
        titleLabel.setColour(Label::textColourId, Colours::white);
        titleLabel.setJustificationType(Justification::centredLeft);
        addAndMakeVisible(titleLabel);

        addBtn.setButtonText("+");
        addBtn.onClick = [this]() { addStripClicked(); };
        addAndMakeVisible(addBtn);

        removeBtn.setButtonText("-");
        removeBtn.onClick = [this]() { removeStripClicked(); };
        addAndMakeVisible(removeBtn);

        masterRow = std::make_unique<DawMasterRow>(processor);
        addAndMakeVisible(masterRow.get());

        rebuildStrips();
        startTimerHz(24);
    }

    ~DawMixerControl() override { stopTimer(); }

    void rebuildStrips()
    {
        stripRows.clear();
        int n = processor->getNumStrips();
        for (int i = 0; i < n; ++i)
        {
            auto* row = new DawStripRow(processor, i, [this]() { notifyParentResize(); });
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
        titleLabel.setBounds(header);
        masterRow->setBounds(r.removeFromBottom(stripRowHeight));
        for (auto* row : stripRows)
            row->setBounds(r.removeFromTop(stripRowHeight));
    }

    void paint(Graphics& g) override { g.fillAll(Colour(0xFF222222)); }

  private:
    static constexpr int stripRowHeight = 52;
    DawMixerProcessor* processor;
    Label titleLabel;
    TextButton addBtn, removeBtn;
    OwnedArray<DawStripRow> stripRows;
    std::unique_ptr<DawMasterRow> masterRow;

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
        masterRow->repaint();
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DawMixerControl)
};

Component* DawMixerProcessor::getControls()
{
    return new DawMixerControl(this);
}

Point<int> DawMixerProcessor::getSize()
{
    int n = numStrips_.load(std::memory_order_acquire);
    int height = 24 + (n + 1) * 52; // header + strips + master
    return Point<int>(340, jmax(160, height));
}

//==============================================================================
// State Serialization
//==============================================================================

void DawMixerProcessor::getStateInformation(MemoryBlock& destData)
{
    XmlElement xml("DawMixer");
    xml.setAttribute("version", 1);

    int n = numStrips_.load(std::memory_order_acquire);
    xml.setAttribute("numStrips", n);
    xml.setAttribute("masterGain", (double)masterGainDb.load(std::memory_order_relaxed));
    xml.setAttribute("masterMute", masterMute.load(std::memory_order_relaxed) ? 1 : 0);

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

void DawMixerProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    auto xml = getXmlFromBinary(data, sizeInBytes);
    if (!xml || xml->getTagName() != "DawMixer")
        return;

    int n = xml->getIntAttribute("numStrips", DefaultStrips);
    n = jlimit(1, MaxStrips, n);

    // Reset all strips then restore saved state
    for (int i = 0; i < n; ++i)
        strips_[static_cast<size_t>(i)].resetDefaults(i);

    numStrips_.store(n, std::memory_order_release);

    masterGainDb.store(static_cast<float>(xml->getDoubleAttribute("masterGain", 0.0)), std::memory_order_relaxed);
    masterMute.store(xml->getIntAttribute("masterMute", 0) != 0, std::memory_order_relaxed);

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
            s.name = stripXml->getStringAttribute("name", "Ch " + String(i + 1));
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

PedalboardProcessor::PinLayout DawMixerProcessor::getInputPinLayout() const
{
    // Pin coordinates are in PluginComponent space.
    // PC title=24, control placed at PC Y=24, control header=24
    // => strip row i top in PC coords = 48 + i * 52
    // Within a 52px row:
    //   mono pin center = row + 26   => pin top = row + 18
    //   stereo L center = row + 14   => pin top = row + 6
    //   stereo R center = row + 38   => pin top = row + 30
    PinLayout layout;
    int n = numStrips_.load(std::memory_order_acquire);
    for (int i = 0; i < n; ++i)
    {
        int rowTop = 48 + i * 52;
        bool isStereo = strips_[static_cast<size_t>(i)].stereo.load(std::memory_order_relaxed);
        if (isStereo)
        {
            layout.pinY.push_back(rowTop + 6);  // L
            layout.pinY.push_back(rowTop + 30); // R
        }
        else
        {
            layout.pinY.push_back(rowTop + 18); // Mono centered
        }
    }
    return layout;
}

PedalboardProcessor::PinLayout DawMixerProcessor::getOutputPinLayout() const
{
    // Master row is after all strip rows.
    // master row top in PC coords = 48 + numStrips * 52
    PinLayout layout;
    int n = numStrips_.load(std::memory_order_acquire);
    int masterTop = 48 + n * 52;
    layout.pinY.push_back(masterTop + 6);  // L
    layout.pinY.push_back(masterTop + 30); // R
    return layout;
}

bool DawMixerProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != AudioChannelSet::stereo())
        return false;
    return true;
}

const String DawMixerProcessor::getInputChannelName(int channelIndex) const
{
    if (channelIndex < countTotalInputChannels())
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
    return "Input " + String(channelIndex + 1);
}

const String DawMixerProcessor::getOutputChannelName(int channelIndex) const
{
    return channelIndex == 0 ? "Master L" : "Master R";
}

void DawMixerProcessor::fillInPluginDescription(PluginDescription& description) const
{
    description.name = getName();
    description.descriptiveName = "DAW-style N-channel mixer";
    description.pluginFormatName = "Internal";
    description.category = "Built-in";
    description.manufacturerName = "Pedalboard";
    description.version = "1.0";
    description.fileOrIdentifier = getName();
    description.isInstrument = false;
    description.numInputChannels = getTotalNumInputChannels();
    description.numOutputChannels = getTotalNumOutputChannels();
}
