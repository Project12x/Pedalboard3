//	VuMeterProcessor.cpp - Simple VU Meter.
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
VuMeterProcessor::VuMeterProcessor()
{
    setPlayConfigDetails(2, 0, 0, 0);
}

//------------------------------------------------------------------------------
VuMeterProcessor::~VuMeterProcessor() {}

//------------------------------------------------------------------------------
Component* VuMeterProcessor::getControls()
{
    VuMeterControl* retval = new VuMeterControl(this);

    return retval;
}

//------------------------------------------------------------------------------
void VuMeterProcessor::updateEditorBounds(const Rectangle<int>& bounds)
{
    editorBounds = bounds;
}

//------------------------------------------------------------------------------
void VuMeterProcessor::fillInPluginDescription(PluginDescription& description) const
{
    description.name = "VU Meter";
    description.descriptiveName = "Simple VU Meter.";
    description.pluginFormatName = "Internal";
    description.category = "Pedalboard Processors";
    description.manufacturerName = "Niall Moody";
    description.version = "1.00";
    description.uniqueId = description.name.hashCode();
    description.isInstrument = false;
    description.numInputChannels = 2;
    description.numOutputChannels = 0;
}

//------------------------------------------------------------------------------
void VuMeterProcessor::processBlock(AudioSampleBuffer& buffer, MidiBuffer& midiMessages)
{
    int i;
    float* dataLeft;
    float* dataRight;

    jassert(buffer.getNumChannels() > 1);

    dataLeft = buffer.getWritePointer(0);
    dataRight = buffer.getWritePointer(1);

    float curLeft = levelLeft.load();
    float curRight = levelRight.load();

    for (i = 0; i < buffer.getNumSamples(); ++i)
    {
        if (fabsf(dataLeft[i]) > curLeft)
            curLeft = fabsf(dataLeft[i]);
        else if (curLeft > 0.0f)
        {
            curLeft -= 0.00001f;
            if (curLeft < 0.0f)
                curLeft = 0.0f;
        }

        if (fabsf(dataRight[i]) > curRight)
            curRight = fabsf(dataRight[i]);
        else if (curRight > 0.0f)
        {
            curRight -= 0.00001f;
            if (curRight < 0.0f)
                curRight = 0.0f;
        }
    }

    levelLeft.store(curLeft);
    levelRight.store(curRight);
}

//------------------------------------------------------------------------------
AudioProcessorEditor* VuMeterProcessor::createEditor()
{
    return new VuMeterEditor(this, editorBounds);
}

//------------------------------------------------------------------------------
void VuMeterProcessor::getStateInformation(MemoryBlock& destData)
{
    XmlElement xml("Pedalboard3VuMeterSettings");

    xml.setAttribute("editorX", editorBounds.getX());
    xml.setAttribute("editorY", editorBounds.getY());
    xml.setAttribute("editorW", editorBounds.getWidth());
    xml.setAttribute("editorH", editorBounds.getHeight());

    copyXmlToBinary(xml, destData);
}

//------------------------------------------------------------------------------
void VuMeterProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    if (xmlState != 0)
    {
        if (xmlState->hasTagName("Pedalboard3VuMeterSettings"))
        {
            editorBounds.setX(xmlState->getIntAttribute("editorX"));
            editorBounds.setY(xmlState->getIntAttribute("editorY"));
            editorBounds.setWidth(xmlState->getIntAttribute("editorW"));
            editorBounds.setHeight(xmlState->getIntAttribute("editorH"));
        }
    }
}
