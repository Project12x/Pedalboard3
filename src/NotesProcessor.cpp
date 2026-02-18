#include "NotesProcessor.h"

#include "NotesControl.h"

NotesProcessor::NotesProcessor() : currentText("Double click to edit note...")
{
    // Visual-only node, no I/O
    setPlayConfigDetails(0, 0, 0, 0);
}

NotesProcessor::~NotesProcessor() {}

Component* NotesProcessor::getControls()
{
    NotesControl* control = new NotesControl(this);
    registerControl(control);
    return control;
}

void NotesProcessor::fillInPluginDescription(PluginDescription& description) const
{
    description.name = "Notes";
    description.descriptiveName = "Displays text notes on the canvas.";
    description.pluginFormatName = "Internal";
    description.category = "Pedalboard";
    description.manufacturerName = "Antigravity";
    description.version = "1.0.0";
    description.uniqueId = description.name.hashCode();
    description.isInstrument = false;
    description.numInputChannels = 0;
    description.numOutputChannels = 0;
}

AudioProcessorEditor* NotesProcessor::createEditor()
{
    // The graph uses getControls() for the canvas node,
    // but if a separate editor window is requested:
    return nullptr; // We don't really support a separate window for this yet
}

void NotesProcessor::getStateInformation(MemoryBlock& destData)
{
    XmlElement xml("NotesNodeSettings");
    xml.setAttribute("text", currentText);
    xml.setAttribute("editorX", editorBounds.getX());
    xml.setAttribute("editorY", editorBounds.getY());
    xml.setAttribute("editorW", editorBounds.getWidth());
    xml.setAttribute("editorH", editorBounds.getHeight());
    copyXmlToBinary(xml, destData);
}

void NotesProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState != nullptr && xmlState->hasTagName("NotesNodeSettings"))
    {
        setText(xmlState->getStringAttribute("text", "New Note"));

        editorBounds.setX(xmlState->getIntAttribute("editorX"));
        editorBounds.setY(xmlState->getIntAttribute("editorY"));
        editorBounds.setWidth(xmlState->getIntAttribute("editorW"));
        editorBounds.setHeight(xmlState->getIntAttribute("editorH"));
    }
}

void NotesProcessor::setText(const String& newText)
{
    if (currentText != newText)
    {
        currentText = newText;
        if (activeControl)
            activeControl->updateText(currentText);
    }
}

void NotesProcessor::registerControl(NotesControl* ctrl)
{
    // We only support tracking one active control for now (usually the canvas one)
    activeControl = ctrl;
}

void NotesProcessor::unregisterControl(NotesControl* ctrl)
{
    if (activeControl == ctrl)
        activeControl = nullptr;
}
