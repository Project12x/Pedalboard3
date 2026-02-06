/*
  ==============================================================================

    ChannelRoutingProcessors.cpp
    Created: 6 Feb 2026

    Flexible channel routing processors for selecting device channels.
    These act as system nodes similar to Audio Input/Output.

  ==============================================================================
*/

#include "ChannelRoutingProcessors.h"

#include "ColourScheme.h"
#include "PedalboardProcessorEditors.h"

//==============================================================================
// Control UI for ChannelInputProcessor
class ChannelInputControl : public Component, public ComboBox::Listener, public Slider::Listener
{
  public:
    ChannelInputControl(ChannelInputProcessor* proc) : processor(proc)
    {
        addAndMakeVisible(modeCombo);
        modeCombo.addItem("Mono (1 ch)", 1);
        modeCombo.addItem("Mono->Stereo", 2);
        modeCombo.addItem("Stereo (2 ch)", 3);
        modeCombo.setSelectedId(static_cast<int>(processor->getMode()) + 1, dontSendNotification);
        modeCombo.addListener(this);

        addAndMakeVisible(modeLabel);
        modeLabel.setText("Mode:", dontSendNotification);
        modeLabel.setJustificationType(Justification::centredRight);

        addAndMakeVisible(channelSlider);
        channelSlider.setSliderStyle(Slider::IncDecButtons);
        channelSlider.setTextBoxStyle(Slider::TextBoxLeft, false, 30, 20);
        channelSlider.setRange(1, 32, 1);  // 1-based display
        channelSlider.setValue(processor->getSelectedChannel() + 1, dontSendNotification);
        channelSlider.addListener(this);

        addAndMakeVisible(channelLabel);
        channelLabel.setText("Ch:", dontSendNotification);
        channelLabel.setJustificationType(Justification::centredRight);

        addAndMakeVisible(pairSlider);
        pairSlider.setSliderStyle(Slider::IncDecButtons);
        pairSlider.setTextBoxStyle(Slider::TextBoxLeft, false, 40, 20);
        pairSlider.setRange(1, 16, 1);  // 1-based display (pair 1 = ch 1+2)
        pairSlider.setValue(processor->getSelectedPair() + 1, dontSendNotification);
        pairSlider.addListener(this);

        addAndMakeVisible(pairLabel);
        pairLabel.setText("Pair:", dontSendNotification);
        pairLabel.setJustificationType(Justification::centredRight);

        updateVisibility();
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced(4);

        auto row1 = area.removeFromTop(24);
        modeLabel.setBounds(row1.removeFromLeft(40));
        modeCombo.setBounds(row1.reduced(2));

        area.removeFromTop(4);
        auto row2 = area.removeFromTop(24);

        if (channelSlider.isVisible())
        {
            channelLabel.setBounds(row2.removeFromLeft(30));
            channelSlider.setBounds(row2.reduced(2));
        }
        else
        {
            pairLabel.setBounds(row2.removeFromLeft(35));
            pairSlider.setBounds(row2.reduced(2));
        }
    }

    void comboBoxChanged(ComboBox* combo) override
    {
        if (combo == &modeCombo)
        {
            processor->setMode(static_cast<ChannelInputProcessor::Mode>(modeCombo.getSelectedId() - 1));
            updateVisibility();
            resized();
        }
    }

    void sliderValueChanged(Slider* slider) override
    {
        if (slider == &channelSlider)
            processor->setSelectedChannel(static_cast<int>(channelSlider.getValue()) - 1);
        else if (slider == &pairSlider)
            processor->setSelectedPair(static_cast<int>(pairSlider.getValue()) - 1);
    }

    void updateVisibility()
    {
        auto mode = processor->getMode();
        bool isStereo = (mode == ChannelInputProcessor::Mode::Stereo);

        channelLabel.setVisible(!isStereo);
        channelSlider.setVisible(!isStereo);
        pairLabel.setVisible(isStereo);
        pairSlider.setVisible(isStereo);
    }

    void paint(Graphics& g) override
    {
        g.fillAll(ColourScheme::getInstance().colours["Plugin Background"]);
        g.setColour(ColourScheme::getInstance().colours["Plugin Border"]);
        g.drawRect(getLocalBounds(), 1);
    }

  private:
    ChannelInputProcessor* processor;
    ComboBox modeCombo;
    Label modeLabel;
    Slider channelSlider;
    Label channelLabel;
    Slider pairSlider;
    Label pairLabel;
};

//==============================================================================
// Control UI for ChannelOutputProcessor
class ChannelOutputControl : public Component, public ComboBox::Listener, public Slider::Listener
{
  public:
    ChannelOutputControl(ChannelOutputProcessor* proc) : processor(proc)
    {
        addAndMakeVisible(modeCombo);
        modeCombo.addItem("Mono (1 ch)", 1);
        modeCombo.addItem("Stereo->Mono", 2);
        modeCombo.addItem("Stereo (2 ch)", 3);
        modeCombo.setSelectedId(static_cast<int>(processor->getMode()) + 1, dontSendNotification);
        modeCombo.addListener(this);

        addAndMakeVisible(modeLabel);
        modeLabel.setText("Mode:", dontSendNotification);
        modeLabel.setJustificationType(Justification::centredRight);

        addAndMakeVisible(channelSlider);
        channelSlider.setSliderStyle(Slider::IncDecButtons);
        channelSlider.setTextBoxStyle(Slider::TextBoxLeft, false, 30, 20);
        channelSlider.setRange(1, 32, 1);
        channelSlider.setValue(processor->getSelectedChannel() + 1, dontSendNotification);
        channelSlider.addListener(this);

        addAndMakeVisible(channelLabel);
        channelLabel.setText("Ch:", dontSendNotification);
        channelLabel.setJustificationType(Justification::centredRight);

        addAndMakeVisible(pairSlider);
        pairSlider.setSliderStyle(Slider::IncDecButtons);
        pairSlider.setTextBoxStyle(Slider::TextBoxLeft, false, 40, 20);
        pairSlider.setRange(1, 16, 1);
        pairSlider.setValue(processor->getSelectedPair() + 1, dontSendNotification);
        pairSlider.addListener(this);

        addAndMakeVisible(pairLabel);
        pairLabel.setText("Pair:", dontSendNotification);
        pairLabel.setJustificationType(Justification::centredRight);

        updateVisibility();
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced(4);

        auto row1 = area.removeFromTop(24);
        modeLabel.setBounds(row1.removeFromLeft(40));
        modeCombo.setBounds(row1.reduced(2));

        area.removeFromTop(4);
        auto row2 = area.removeFromTop(24);

        if (channelSlider.isVisible())
        {
            channelLabel.setBounds(row2.removeFromLeft(30));
            channelSlider.setBounds(row2.reduced(2));
        }
        else
        {
            pairLabel.setBounds(row2.removeFromLeft(35));
            pairSlider.setBounds(row2.reduced(2));
        }
    }

    void comboBoxChanged(ComboBox* combo) override
    {
        if (combo == &modeCombo)
        {
            processor->setMode(static_cast<ChannelOutputProcessor::Mode>(modeCombo.getSelectedId() - 1));
            updateVisibility();
            resized();
        }
    }

    void sliderValueChanged(Slider* slider) override
    {
        if (slider == &channelSlider)
            processor->setSelectedChannel(static_cast<int>(channelSlider.getValue()) - 1);
        else if (slider == &pairSlider)
            processor->setSelectedPair(static_cast<int>(pairSlider.getValue()) - 1);
    }

    void updateVisibility()
    {
        auto mode = processor->getMode();
        bool isStereo = (mode == ChannelOutputProcessor::Mode::Stereo);

        channelLabel.setVisible(!isStereo);
        channelSlider.setVisible(!isStereo);
        pairLabel.setVisible(isStereo);
        pairSlider.setVisible(isStereo);
    }

    void paint(Graphics& g) override
    {
        g.fillAll(ColourScheme::getInstance().colours["Plugin Background"]);
        g.setColour(ColourScheme::getInstance().colours["Plugin Border"]);
        g.drawRect(getLocalBounds(), 1);
    }

  private:
    ChannelOutputProcessor* processor;
    ComboBox modeCombo;
    Label modeLabel;
    Slider channelSlider;
    Label channelLabel;
    Slider pairSlider;
    Label pairLabel;
};

//==============================================================================
// ChannelInputProcessor Implementation
//==============================================================================

ChannelInputProcessor::ChannelInputProcessor()
{
    // Default stereo mode: 2 inputs (from audioInputNode), 2 outputs
    setPlayConfigDetails(2, 2, 0, 0);
}

ChannelInputProcessor::~ChannelInputProcessor() {}

void ChannelInputProcessor::setMode(Mode newMode)
{
    mode.store(newMode);
    updateChannelConfig();
}

void ChannelInputProcessor::setSelectedChannel(int channel)
{
    selectedChannel.store(jmax(0, channel));
}

void ChannelInputProcessor::setSelectedPair(int pair)
{
    selectedPair.store(jmax(0, pair));
}

int ChannelInputProcessor::getOutputChannelCount() const
{
    Mode m = mode.load();
    return (m == Mode::Mono) ? 1 : 2;
}

int ChannelInputProcessor::getInputChannelCount() const
{
    Mode m = mode.load();
    return (m == Mode::Stereo) ? 2 : 1;
}

void ChannelInputProcessor::updateChannelConfig()
{
    // Input count depends on mode, output count depends on mode
    int inChannels = getInputChannelCount();
    int outChannels = getOutputChannelCount();
    setPlayConfigDetails(inChannels, outChannels, getSampleRate(), getBlockSize());
}

Component* ChannelInputProcessor::getControls()
{
    return new ChannelInputControl(this);
}

void ChannelInputProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    updateChannelConfig();
}

void ChannelInputProcessor::processBlock(AudioBuffer<float>& buffer, MidiBuffer& midiMessages)
{
    // This processor receives audio from audioInputNode via graph connections
    // and routes selected channels to its outputs

    Mode m = mode.load();
    int numSamples = buffer.getNumSamples();

    switch (m)
    {
    case Mode::Mono:
        // 1 input -> 1 output (pass-through)
        // Input channel is already the selected channel via graph connection
        break;

    case Mode::MonoToStereo:
        // 1 input -> 2 outputs (duplicate)
        if (buffer.getNumChannels() >= 2)
        {
            buffer.copyFrom(1, 0, buffer, 0, 0, numSamples);
        }
        break;

    case Mode::Stereo:
        // 2 inputs -> 2 outputs (pass-through)
        // Already correctly connected via graph
        break;
    }
}

AudioProcessorEditor* ChannelInputProcessor::createEditor()
{
    return new GenericAudioProcessorEditor(*this);
}

void ChannelInputProcessor::fillInPluginDescription(PluginDescription& description) const
{
    description.name = "Channel Input";
    description.descriptiveName = "Device channel input selector (replaces Audio Input).";
    description.pluginFormatName = "Internal";
    description.category = "Routing";
    description.manufacturerName = "Pedalboard3";
    description.version = "1.00";
    description.uniqueId = description.name.hashCode();
    description.isInstrument = false;
    description.numInputChannels = getInputChannelCount();
    description.numOutputChannels = getOutputChannelCount();
}

bool ChannelInputProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    int inCh = layouts.getMainInputChannels();
    int outCh = layouts.getMainOutputChannels();

    // Input: 1 or 2 channels (from audioInputNode)
    // Output: 1 or 2 channels
    if (inCh == 1 && outCh == 1)
        return true;  // Mono mode
    if (inCh == 1 && outCh == 2)
        return true;  // MonoToStereo mode
    if (inCh == 2 && outCh == 2)
        return true;  // Stereo mode

    return false;
}

const String ChannelInputProcessor::getOutputChannelName(int channelIndex) const
{
    Mode m = mode.load();
    if (m == Mode::Mono)
    {
        return "Out " + String(selectedChannel.load() + 1);
    }
    int pair = selectedPair.load();
    int baseChannel = pair * 2;
    return channelIndex == 0 ? "Out " + String(baseChannel + 1) : "Out " + String(baseChannel + 2);
}

float ChannelInputProcessor::getParameter(int parameterIndex)
{
    switch (parameterIndex)
    {
    case ModeParam:
        return static_cast<float>(mode.load()) / 2.0f;
    case ChannelParam:
        return static_cast<float>(selectedChannel.load()) / 31.0f;
    case PairParam:
        return static_cast<float>(selectedPair.load()) / 15.0f;
    default:
        return 0.0f;
    }
}

void ChannelInputProcessor::setParameter(int parameterIndex, float newValue)
{
    switch (parameterIndex)
    {
    case ModeParam:
        setMode(static_cast<Mode>(jlimit(0, 2, static_cast<int>(newValue * 2.0f + 0.5f))));
        break;
    case ChannelParam:
        setSelectedChannel(static_cast<int>(newValue * 31.0f + 0.5f));
        break;
    case PairParam:
        setSelectedPair(static_cast<int>(newValue * 15.0f + 0.5f));
        break;
    }
}

const String ChannelInputProcessor::getParameterName(int parameterIndex)
{
    switch (parameterIndex)
    {
    case ModeParam:
        return "Mode";
    case ChannelParam:
        return "Channel";
    case PairParam:
        return "Pair";
    default:
        return "";
    }
}

const String ChannelInputProcessor::getParameterText(int parameterIndex)
{
    switch (parameterIndex)
    {
    case ModeParam:
        switch (mode.load())
        {
        case Mode::Mono:
            return "Mono";
        case Mode::MonoToStereo:
            return "Mono->Stereo";
        case Mode::Stereo:
            return "Stereo";
        }
        break;
    case ChannelParam:
        return String(selectedChannel.load() + 1);
    case PairParam:
        return String(selectedPair.load() + 1);
    }
    return "";
}

void ChannelInputProcessor::getStateInformation(MemoryBlock& destData)
{
    XmlElement xml("ChannelInputSettings");
    xml.setAttribute("mode", static_cast<int>(mode.load()));
    xml.setAttribute("channel", selectedChannel.load());
    xml.setAttribute("pair", selectedPair.load());
    copyXmlToBinary(xml, destData);
}

void ChannelInputProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState != nullptr && xmlState->hasTagName("ChannelInputSettings"))
    {
        setMode(static_cast<Mode>(xmlState->getIntAttribute("mode", 2)));
        setSelectedChannel(xmlState->getIntAttribute("channel", 0));
        setSelectedPair(xmlState->getIntAttribute("pair", 0));
    }
}

//==============================================================================
// ChannelOutputProcessor Implementation
//==============================================================================

ChannelOutputProcessor::ChannelOutputProcessor()
{
    // Sink node: 2 inputs (default stereo), 0 outputs
    setPlayConfigDetails(2, 0, 0, 0);
}

ChannelOutputProcessor::~ChannelOutputProcessor() {}

void ChannelOutputProcessor::setMode(Mode newMode)
{
    mode.store(newMode);
    updateChannelConfig();
}

void ChannelOutputProcessor::setSelectedChannel(int channel)
{
    selectedChannel.store(jmax(0, channel));
}

void ChannelOutputProcessor::setSelectedPair(int pair)
{
    selectedPair.store(jmax(0, pair));
}

int ChannelOutputProcessor::getInputChannelCount() const
{
    Mode m = mode.load();
    return (m == Mode::Mono) ? 1 : 2;
}

void ChannelOutputProcessor::updateChannelConfig()
{
    // Sink node: input count depends on mode, always 0 outputs
    int inChannels = getInputChannelCount();
    setPlayConfigDetails(inChannels, 0, getSampleRate(), getBlockSize());
}

Component* ChannelOutputProcessor::getControls()
{
    return new ChannelOutputControl(this);
}

void ChannelOutputProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    updateChannelConfig();
}

void ChannelOutputProcessor::processBlock(AudioBuffer<float>& buffer, MidiBuffer& midiMessages)
{
    // This is a sink node that sends to device output channels
    // For stereo-to-mono mode, sum the channels
    Mode m = mode.load();

    if (m == Mode::StereoToMono && buffer.getNumChannels() >= 2)
    {
        auto* ch0 = buffer.getWritePointer(0);
        auto* ch1 = buffer.getReadPointer(1);
        int numSamples = buffer.getNumSamples();

        for (int i = 0; i < numSamples; ++i)
        {
            ch0[i] = (ch0[i] + ch1[i]) * 0.5f;
        }
    }
}

AudioProcessorEditor* ChannelOutputProcessor::createEditor()
{
    return new GenericAudioProcessorEditor(*this);
}

void ChannelOutputProcessor::fillInPluginDescription(PluginDescription& description) const
{
    description.name = "Channel Output";
    description.descriptiveName = "Device channel output selector (replaces Audio Output).";
    description.pluginFormatName = "Internal";
    description.category = "Routing";
    description.manufacturerName = "Pedalboard3";
    description.version = "1.00";
    description.uniqueId = description.name.hashCode();
    description.isInstrument = false;
    description.numInputChannels = getInputChannelCount();
    description.numOutputChannels = 0;
}

bool ChannelOutputProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    int inCh = layouts.getMainInputChannels();
    int outCh = layouts.getMainOutputChannels();

    // Sink: 1 or 2 inputs, 0 outputs
    if (outCh != 0)
        return false;
    if (inCh == 1 || inCh == 2)
        return true;

    return false;
}

const String ChannelOutputProcessor::getInputChannelName(int channelIndex) const
{
    Mode m = mode.load();
    if (m == Mode::Mono)
    {
        return "In";
    }
    return channelIndex == 0 ? "In L" : "In R";
}

float ChannelOutputProcessor::getParameter(int parameterIndex)
{
    switch (parameterIndex)
    {
    case ModeParam:
        return static_cast<float>(mode.load()) / 2.0f;
    case ChannelParam:
        return static_cast<float>(selectedChannel.load()) / 31.0f;
    case PairParam:
        return static_cast<float>(selectedPair.load()) / 15.0f;
    default:
        return 0.0f;
    }
}

void ChannelOutputProcessor::setParameter(int parameterIndex, float newValue)
{
    switch (parameterIndex)
    {
    case ModeParam:
        setMode(static_cast<Mode>(jlimit(0, 2, static_cast<int>(newValue * 2.0f + 0.5f))));
        break;
    case ChannelParam:
        setSelectedChannel(static_cast<int>(newValue * 31.0f + 0.5f));
        break;
    case PairParam:
        setSelectedPair(static_cast<int>(newValue * 15.0f + 0.5f));
        break;
    }
}

const String ChannelOutputProcessor::getParameterName(int parameterIndex)
{
    switch (parameterIndex)
    {
    case ModeParam:
        return "Mode";
    case ChannelParam:
        return "Channel";
    case PairParam:
        return "Pair";
    default:
        return "";
    }
}

const String ChannelOutputProcessor::getParameterText(int parameterIndex)
{
    switch (parameterIndex)
    {
    case ModeParam:
        switch (mode.load())
        {
        case Mode::Mono:
            return "Mono";
        case Mode::StereoToMono:
            return "Stereo->Mono";
        case Mode::Stereo:
            return "Stereo";
        }
        break;
    case ChannelParam:
        return String(selectedChannel.load() + 1);
    case PairParam:
        return String(selectedPair.load() + 1);
    }
    return "";
}

void ChannelOutputProcessor::getStateInformation(MemoryBlock& destData)
{
    XmlElement xml("ChannelOutputSettings");
    xml.setAttribute("mode", static_cast<int>(mode.load()));
    xml.setAttribute("channel", selectedChannel.load());
    xml.setAttribute("pair", selectedPair.load());
    copyXmlToBinary(xml, destData);
}

void ChannelOutputProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState != nullptr && xmlState->hasTagName("ChannelOutputSettings"))
    {
        setMode(static_cast<Mode>(xmlState->getIntAttribute("mode", 2)));
        setSelectedChannel(xmlState->getIntAttribute("channel", 0));
        setSelectedPair(xmlState->getIntAttribute("pair", 0));
    }
}
