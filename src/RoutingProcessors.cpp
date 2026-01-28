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
// Controls for Mixer
class MixerControl : public Component, public Slider::Listener
{
  public:
    MixerControl(MixerProcessor* proc) : processor(proc)
    {
        addAndMakeVisible(volA);
        volA.setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
        volA.setTextBoxStyle(Slider::NoTextBox, false, 0, 0);
        volA.setRange(0.0, 1.0);
        volA.setValue(processor->getLevelA(), dontSendNotification);
        volA.addListener(this);

        addAndMakeVisible(volB);
        volB.setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
        volB.setTextBoxStyle(Slider::NoTextBox, false, 0, 0);
        volB.setRange(0.0, 1.0);
        volB.setValue(processor->getLevelB(), dontSendNotification);
        volB.addListener(this);

        addAndMakeVisible(labelA);
        labelA.setText("A", dontSendNotification);
        labelA.setJustificationType(Justification::centred);

        addAndMakeVisible(labelB);
        labelB.setText("B", dontSendNotification);
        labelB.setJustificationType(Justification::centred);
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced(4);
        int w = area.getWidth() / 2;

        auto left = area.removeFromLeft(w);
        labelA.setBounds(left.removeFromBottom(20));
        volA.setBounds(left.reduced(2));

        labelB.setBounds(area.removeFromBottom(20));
        volB.setBounds(area.reduced(2));
    }

    void sliderValueChanged(Slider* s) override
    {
        if (s == &volA)
            processor->setLevelA((float)s->getValue());
        else if (s == &volB)
            processor->setLevelB((float)s->getValue());
    }

    void paint(Graphics& g) override
    {
        g.fillAll(ColourScheme::getInstance().colours["Plugin Background"]);
        g.setColour(ColourScheme::getInstance().colours["Plugin Border"]);
        g.drawRect(getLocalBounds(), 1);
    }

  private:
    MixerProcessor* processor;
    Slider volA, volB;
    Label labelA, labelB;
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

Component* MixerProcessor::getControls()
{
    return new MixerControl(this);
}

void MixerProcessor::processBlock(AudioBuffer<float>& buffer, MidiBuffer& midiMessages)
{
    int numIn = getTotalNumInputChannels();
    int numOut = getTotalNumOutputChannels();

    if (numIn < 4 || numOut < 2)
        return;

    // Inputs are read-only pointers usually, but getWritePointer gives read/write.
    // Since we are writing to 0 and 1, we must read 2 and 3 first or buffer carefully.
    // If input 0 is same buffer as output 0, we can write in place.
    // But we need to mix in 2 and 3.
    // Safer to read all inputs to temps or read 2/3 and add to 0/1.

    auto* inAL = buffer.getReadPointer(0);
    auto* inAR = buffer.getReadPointer(1);
    auto* inBL = buffer.getReadPointer(2);
    auto* inBR = buffer.getReadPointer(3);

    auto* outL = buffer.getWritePointer(0);
    auto* outR = buffer.getWritePointer(1);

    int numSamples = buffer.getNumSamples();
    float gA = levelA;
    float gB = levelB;

    for (int i = 0; i < numSamples; ++i)
    {
        float valAL = inAL[i];
        float valAR = inAR[i];
        float valBL = inBL[i];
        float valBR = inBR[i];

        outL[i] = (valAL * gA) + (valBL * gB);
        outR[i] = (valAR * gA) + (valBR * gB);
    }
}

AudioProcessorEditor* MixerProcessor::createEditor()
{
    return new GenericAudioProcessorEditor(*this);
}

void MixerProcessor::fillInPluginDescription(PluginDescription& description) const
{
    description.name = "Mixer";
    description.descriptiveName = "Mixes two stereo pairs (A and B) to stereo.";
    description.pluginFormatName = "Internal";
    description.category = "Routing";
    description.manufacturerName = "Pedalboard3";
    description.version = "1.00";
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

void MixerProcessor::setLevelA(float level)
{
    levelA = level;
}
void MixerProcessor::setLevelB(float level)
{
    levelB = level;
}

float MixerProcessor::getParameter(int parameterIndex)
{
    if (parameterIndex == LevelA)
        return levelA;
    if (parameterIndex == LevelB)
        return levelB;
    return 0.0f;
}

void MixerProcessor::setParameter(int parameterIndex, float newValue)
{
    if (parameterIndex == LevelA)
        levelA = newValue;
    else if (parameterIndex == LevelB)
        levelB = newValue;
}

const String MixerProcessor::getParameterName(int parameterIndex)
{
    if (parameterIndex == LevelA)
        return "Level A";
    if (parameterIndex == LevelB)
        return "Level B";
    return "";
}

const String MixerProcessor::getParameterText(int parameterIndex)
{
    return String(getParameter(parameterIndex), 2);
}

void MixerProcessor::getStateInformation(MemoryBlock& destData)
{
    XmlElement xml("MixerSettings");
    xml.setAttribute("levelA", levelA);
    xml.setAttribute("levelB", levelB);
    copyXmlToBinary(xml, destData);
}

void MixerProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState != nullptr && xmlState->hasTagName("MixerSettings"))
    {
        levelA = (float)xmlState->getDoubleAttribute("levelA", 0.707);
        levelB = (float)xmlState->getDoubleAttribute("levelB", 0.707);
    }
}
