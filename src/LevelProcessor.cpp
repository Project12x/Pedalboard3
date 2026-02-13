//	LevelProcessor.cpp - Simple level processor.
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
LevelProcessor::LevelProcessor()
{
    setPlayConfigDetails(2, 2, 0, 0);
}

//------------------------------------------------------------------------------
LevelProcessor::~LevelProcessor() {}

//------------------------------------------------------------------------------
Component* LevelProcessor::getControls()
{
    LevelControl* retval = new LevelControl(this);

    return retval;
}

//------------------------------------------------------------------------------
void LevelProcessor::updateEditorBounds(const Rectangle<int>& bounds)
{
    editorBounds = bounds;
}

//------------------------------------------------------------------------------
void LevelProcessor::fillInPluginDescription(PluginDescription& description) const
{
    description.name = "Level";
    description.descriptiveName = "Simple level processor.";
    description.pluginFormatName = "Internal";
    description.category = "Pedalboard Processors";
    description.manufacturerName = "Niall Moody";
    description.version = "1.00";
    description.uniqueId = description.name.hashCode();
    description.isInstrument = false;
    description.numInputChannels = 2;
    description.numOutputChannels = 2;
}

//------------------------------------------------------------------------------
void LevelProcessor::processBlock(AudioSampleBuffer& buffer, MidiBuffer& midiMessages)
{
    int i;
    float* dataLeft;
    float* dataRight;

    jassert(buffer.getNumChannels() > 1);

    dataLeft = buffer.getWritePointer(0);
    dataRight = buffer.getWritePointer(1);

    const float currentLevel = level.load() * 2.0f;
    for (i = 0; i < buffer.getNumSamples(); ++i)
    {
        dataLeft[i] = dataLeft[i] * currentLevel;
        dataRight[i] = dataRight[i] * currentLevel;
    }
}

//------------------------------------------------------------------------------
AudioProcessorEditor* LevelProcessor::createEditor()
{
    return new LevelEditor(this, editorBounds);
}

//------------------------------------------------------------------------------
const String LevelProcessor::getParameterText(int parameterIndex)
{
    String retval;

    retval << (level.load() * 2.0f);

    return retval;
}

//------------------------------------------------------------------------------
void LevelProcessor::setParameter(int parameterIndex, float newValue)
{
    level.store(newValue);
}

//------------------------------------------------------------------------------
void LevelProcessor::getStateInformation(MemoryBlock& destData)
{
    XmlElement xml("Pedalboard3LevelSettings");

    xml.setAttribute("leve", level.load());

    xml.setAttribute("editorX", editorBounds.getX());
    xml.setAttribute("editorY", editorBounds.getY());
    xml.setAttribute("editorW", editorBounds.getWidth());
    xml.setAttribute("editorH", editorBounds.getHeight());

    copyXmlToBinary(xml, destData);
}

//------------------------------------------------------------------------------
void LevelProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    if (xmlState != 0)
    {
        if (xmlState->hasTagName("Pedalboard3LevelSettings"))
        {
            editorBounds.setX(xmlState->getIntAttribute("editorX"));
            editorBounds.setY(xmlState->getIntAttribute("editorY"));
            editorBounds.setWidth(xmlState->getIntAttribute("editorW"));
            editorBounds.setHeight(xmlState->getIntAttribute("editorH"));

            level.store((float)xmlState->getDoubleAttribute("leve", 0.5));
        }
    }
}
