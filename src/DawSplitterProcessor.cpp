//  DawSplitterProcessor.cpp - DAW-style N-channel splitter node.
//  --------------------------------------------------------------------------
//  Mirror of DawMixerProcessor: 1 stereo input -> N mono outputs.

#include "DawSplitterProcessor.h"

#include "PluginComponent.h"

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

void DawSplitterProcessor::updateChannelConfig()
{
    int numInputChannels = 2;                                           // stereo input
    int numOutputChannels = numStrips_.load(std::memory_order_acquire); // 1 mono channel per strip
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
    for (int i = 0; i < MaxStrips; ++i)
    {
        stripDsp_[static_cast<size_t>(i)].init(sampleRate);

        int n = numStrips_.load(std::memory_order_acquire);
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

    // Process each output strip (stereo input -> mono output per strip)
    for (int s = 0; s < ns; ++s)
    {
        auto& strip = strips_[static_cast<size_t>(s)];
        auto& dsp = stripDsp_[static_cast<size_t>(s)];

        // Each strip is a single mono output channel
        if (s >= totalOutputChannels)
            continue;

        float* dst = buffer.getWritePointer(s);

        // Read atomic state once per block
        bool mute = strip.mute.load(std::memory_order_relaxed);
        bool solo = strip.solo.load(std::memory_order_relaxed);
        bool phaseInv = strip.phaseInvert.load(std::memory_order_relaxed);
        float gainDb = strip.gainDb.load(std::memory_order_relaxed);
        bool effectiveMute = mute || (anySolo && !solo);

        // Update smoothed gain target
        float gainLin = Decibels::decibelsToGain(gainDb);
        dsp.smoothedGain.setTargetValue(gainLin);

        // Peak metering
        float peakL = strip.peakL.load(std::memory_order_relaxed);
        float peakR = strip.peakR.load(std::memory_order_relaxed);

        for (int i = 0; i < numSamples; ++i)
        {
            float gain = dsp.smoothedGain.getNextValue();

            // Sum stereo input to mono
            float sample = (inL[i] + inR[i]) * 0.5f;

            if (phaseInv)
                sample = -sample;

            sample *= gain;

            // Peak (post-gain, pre-mute)
            float absSample = std::abs(sample);
            peakL = (absSample > peakL) ? absSample : peakL * peakDecay_;
            peakR = peakL; // mono -- same for both

            if (effectiveMute)
                dst[i] = 0.0f;
            else
                dst[i] = sample;
        }

        if (peakL < 1e-10f)
            peakL = 0.0f;
        strip.peakL.store(peakL, std::memory_order_relaxed);
        strip.peakR.store(peakL, std::memory_order_relaxed);
        strip.vuL.store(peakL, std::memory_order_relaxed);
        strip.vuR.store(peakL, std::memory_order_relaxed);
    }

    // Clear any unused output channels beyond our strips
    int usedOutputs = ns;
    for (int ch = usedOutputs; ch < totalOutputChannels; ++ch)
        buffer.clear(ch, 0, numSamples);
}

//==============================================================================
// UI -- Horizontal Strip Row (same layout as DawMixerProcessor)
//==============================================================================

class SplitterStripRow : public Component
{
  public:
    SplitterStripRow(DawSplitterProcessor* proc, int stripIndex) : processor(proc), index(stripIndex)
    {
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
        muteBtn.setColour(TextButton::buttonOnColourId, Colours::red);
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
            nameLabel.setText(s->name, dontSendNotification);
        }
    }

    void resized() override
    {
        auto r = getLocalBounds().reduced(2, 1);
        int halfH = r.getHeight() / 2;
        auto row1 = r.removeFromTop(halfH);
        auto row2 = r;

        // Row 1: [name 40] [Phase 22] [M 22] [S 22] [gap 4] [VU rest]
        nameLabel.setBounds(row1.removeFromLeft(40));
        phaseBtn.setBounds(row1.removeFromLeft(22));
        muteBtn.setBounds(row1.removeFromLeft(22));
        soloBtn.setBounds(row1.removeFromLeft(22));
        row1.removeFromLeft(4);
        vuArea = row1;

        // Row 2: [fader full width]
        fader.setBounds(row2);
    }

    void paint(Graphics& g) override
    {
        g.setColour(Colour(0xFF2A2A2A));
        g.fillRect(getLocalBounds());
        g.setColour(Colour(0xFF404040));
        g.drawHorizontalLine(getHeight() - 1, 0.0f, static_cast<float>(getWidth()));

        if (auto* s = processor->getStrip(index))
        {
            float peak = s->peakL.load(std::memory_order_relaxed);
            paintMonoVU(g, vuArea, peak);
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
            g.setColour(Colours::red);
        else if (dbVal > -6.0f)
            g.setColour(Colours::orange);
        else if (dbVal > -18.0f)
            g.setColour(Colour(0xFF00CC00));
        else
            g.setColour(Colour(0xFF008800));
        g.fillRect(filled);
    }

  private:
    DawSplitterProcessor* processor;
    int index;
    TextButton phaseBtn, muteBtn, soloBtn;
    Slider fader;
    Label nameLabel;
    Rectangle<int> vuArea;
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
        auto r = getLocalBounds().reduced(2, 1);
        nameLabel.setBounds(r.removeFromLeft(46));
        vuArea = r;
    }

    void paint(Graphics& g) override
    {
        g.setColour(Colour(0xFF333333));
        g.fillRect(getLocalBounds());
        g.setColour(Colour(0xFF606060));
        g.drawHorizontalLine(getHeight() - 1, 0.0f, static_cast<float>(getWidth()));
        float peakL = processor->inputPeakL.load(std::memory_order_relaxed);
        float peakR = processor->inputPeakR.load(std::memory_order_relaxed);
        paintStereoVU(g, vuArea, peakL, peakR);
    }

    static void paintStereoVU(Graphics& g, Rectangle<int> area, float peakL, float peakR)
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
                g.setColour(Colours::red);
            else if (dbVal > -6.0f)
                g.setColour(Colours::orange);
            else if (dbVal > -18.0f)
                g.setColour(Colour(0xFF00CC00));
            else
                g.setColour(Colour(0xFF008800));
            g.fillRect(filled);
        };

        drawBar(barL, peakL);
        drawBar(barR, peakR);
    }

  private:
    DawSplitterProcessor* processor;
    Label nameLabel;
    Rectangle<int> vuArea;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SplitterInputRow)
};

//==============================================================================
// Main control -- returned by getControls()
class DawSplitterControl : public Component, private Timer
{
  public:
    DawSplitterControl(DawSplitterProcessor* proc) : processor(proc)
    {
        titleLabel.setText("DAW Splitter", dontSendNotification);
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
            auto* row = new SplitterStripRow(processor, i);
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

        // Input row at top (instead of master at bottom for mixer)
        inputRow->setBounds(r.removeFromTop(stripRowHeight));

        for (auto* row : stripRows)
            row->setBounds(r.removeFromTop(stripRowHeight));
    }

    void paint(Graphics& g) override { g.fillAll(Colour(0xFF222222)); }

  private:
    static constexpr int stripRowHeight = 44;
    DawSplitterProcessor* processor;
    Label titleLabel;
    TextButton addBtn, removeBtn;
    OwnedArray<SplitterStripRow> stripRows;
    std::unique_ptr<SplitterInputRow> inputRow;

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
    int height = 24 + (n + 1) * 44; // header + input row + strips
    return Point<int>(340, jmax(160, height));
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
            s.name = stripXml->getStringAttribute("name", "Out " + String(i + 1));
        }
    }

    updateChannelConfig();
}

//==============================================================================
// Plugin Description
//==============================================================================

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
    if (channelIndex < numStrips_.load(std::memory_order_acquire))
        return strips_[static_cast<size_t>(channelIndex)].name;
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
