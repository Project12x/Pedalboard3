//  LabelProcessor.cpp - Implementation of the Label processor.
//  ----------------------------------------------------------------------------
//  This file is part of Pedalboard3, an audio plugin host.
//  Copyright (c) 2024 Pedalboard3 Project.
//
//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//  ----------------------------------------------------------------------------

#include "LabelProcessor.h"

#include "LabelControl.h"

LabelProcessor::LabelProcessor() : labelText("Label")
{
    // Visual-only node, no I/O
    setPlayConfigDetails(0, 0, 0, 0);
}

LabelProcessor::~LabelProcessor() {}

Component* LabelProcessor::getControls()
{
    LabelControl* control = new LabelControl(this);
    return control;
}

void LabelProcessor::fillInPluginDescription(PluginDescription& description) const
{
    description.name = "Label";
    description.descriptiveName = "Simple text label for annotations";
    description.pluginFormatName = "Internal";
    description.category = "Utility";
    description.manufacturerName = "Pedalboard3";
    description.version = "1.0";
    description.fileOrIdentifier = "Internal:Label";
    description.isInstrument = false;
    description.numInputChannels = 0;
    description.numOutputChannels = 0;
}

AudioProcessorEditor* LabelProcessor::createEditor()
{
    return nullptr; // No external editor
}

void LabelProcessor::getStateInformation(MemoryBlock& destData)
{
    XmlElement xml("LabelNodeSettings");
    xml.setAttribute("text", labelText);

    copyXmlToBinary(xml, destData);
}

void LabelProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState != nullptr && xmlState->hasTagName("LabelNodeSettings"))
    {
        labelText = xmlState->getStringAttribute("text", "Label");
        if (activeControl != nullptr)
            activeControl->updateText(labelText);
    }
}

void LabelProcessor::setText(const String& newText)
{
    labelText = newText;
}

void LabelProcessor::registerControl(LabelControl* ctrl)
{
    activeControl = ctrl;
}

void LabelProcessor::unregisterControl(LabelControl* ctrl)
{
    if (activeControl == ctrl)
        activeControl = nullptr;
}
