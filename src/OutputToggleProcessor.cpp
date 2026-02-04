//	OutputToggleProcessor.cpp - Simple output toggle processor.
//	----------------------------------------------------------------------------
//	This file is part of Pedalboard3, an audio plugin host.
//	Copyright (c) 2011 Niall Moody.
//
//	This program is free software: you can redistribute it and/or modify
//	it under the terms of the GNU General Public License as published by
//	the Free Software Foundation, either version 3 of the License, or
//	(at your option) any later version.
//
//	This program is distributed in the hope that it will be useful,
//	but WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//	GNU General Public License for more details.
//
//	You should have received a copy of the GNU General Public License
//	along with this program.  If not, see <http://www.gnu.org/licenses/>.
//	----------------------------------------------------------------------------

#include "PedalboardProcessorEditors.h"
#include "PedalboardProcessors.h"


//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
OutputToggleProcessor::OutputToggleProcessor() : toggle(false), fade(0.0f)
{
    setPlayConfigDetails(1, 2, 0, 0);
}

//------------------------------------------------------------------------------
OutputToggleProcessor::~OutputToggleProcessor() {}

//------------------------------------------------------------------------------
Component* OutputToggleProcessor::getControls()
{
    OutputToggleControl* retval = new OutputToggleControl(this);

    return retval;
}

//------------------------------------------------------------------------------
void OutputToggleProcessor::updateEditorBounds(const Rectangle<int>& bounds)
{
    editorBounds = bounds;
}

//------------------------------------------------------------------------------
void OutputToggleProcessor::fillInPluginDescription(PluginDescription& description) const
{
    description.name = "Output Toggle";
    description.descriptiveName = "Simple output toggle processor.";
    description.pluginFormatName = "Internal";
    description.category = "Pedalboard Processors";
    description.manufacturerName = "Niall Moody";
    description.version = "1.00";
    description.uniqueId = description.name.hashCode();
    description.isInstrument = false;
    description.numInputChannels = 1;
    description.numOutputChannels = 2;
}

//------------------------------------------------------------------------------
void OutputToggleProcessor::processBlock(AudioSampleBuffer& buffer, MidiBuffer& midiMessages)
{
    int i;
    float tempf;
    float* data[2];

    jassert(buffer.getNumChannels() > 1);

    data[0] = buffer.getWritePointer(0);
    data[1] = buffer.getWritePointer(1);

    for (i = 0; i < buffer.getNumSamples(); ++i)
    {
        if (!toggle && (fade > 0.0f))
            fade -= 0.001f;
        else if (toggle && (fade < 1.0f))
            fade += 0.001f;

        tempf = data[0][i];
        data[0][i] = tempf * (1.0f - fade);
        data[1][i] = tempf * fade;
    }
}

//------------------------------------------------------------------------------
AudioProcessorEditor* OutputToggleProcessor::createEditor()
{
    return new OutputToggleEditor(this, editorBounds);
}

//------------------------------------------------------------------------------
const String OutputToggleProcessor::getParameterText(int parameterIndex)
{
    String retval;

    if (toggle)
        retval = "Output 1";
    else
        retval = "Output 2";

    return retval;
}

//------------------------------------------------------------------------------
void OutputToggleProcessor::setParameter(int parameterIndex, float newValue)
{
    toggle = newValue > 0.5f;
}

//------------------------------------------------------------------------------
void OutputToggleProcessor::getStateInformation(MemoryBlock& destData)
{
    XmlElement xml("Pedalboard3OutputToggleSettings");

    xml.setAttribute("toggle", toggle);

    xml.setAttribute("editorX", editorBounds.getX());
    xml.setAttribute("editorY", editorBounds.getY());
    xml.setAttribute("editorW", editorBounds.getWidth());
    xml.setAttribute("editorH", editorBounds.getHeight());

    copyXmlToBinary(xml, destData);
}

//------------------------------------------------------------------------------
void OutputToggleProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    if (xmlState != 0)
    {
        if (xmlState->hasTagName("Pedalboard3OutputToggleSettings"))
        {
            toggle = xmlState->getBoolAttribute("toggle", false);

            editorBounds.setX(xmlState->getIntAttribute("editorX"));
            editorBounds.setY(xmlState->getIntAttribute("editorY"));
            editorBounds.setWidth(xmlState->getIntAttribute("editorW"));
            editorBounds.setHeight(xmlState->getIntAttribute("editorH"));
        }
    }
}
