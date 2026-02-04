//	FilePlayerProcessor.cpp - Processor which plays back an audio file.
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

#include "AudioSingletons.h"
#include "FilePlayerControl.h"
#include "MainTransport.h"
#include "PedalboardProcessorEditors.h"
#include "PedalboardProcessors.h"


//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
FilePlayerProcessor::FilePlayerProcessor() : looping(false), syncToMainTransport(false), justPaused(false)
{
    setPlayConfigDetails(0, 2, 0, 0);
    transportSource.addChangeListener(this);

    MainTransport::getInstance()->registerTransport(this);
}

//------------------------------------------------------------------------------
FilePlayerProcessor::FilePlayerProcessor(const File& phil)
    : looping(false), syncToMainTransport(false), justPaused(false)
{
    setPlayConfigDetails(0, 2, 0, 0);
    transportSource.addChangeListener(this);

    MainTransport::getInstance()->registerTransport(this);

    setFile(phil);
}

//------------------------------------------------------------------------------
FilePlayerProcessor::~FilePlayerProcessor()
{
    removeAllChangeListeners();
    MainTransport::getInstance()->unregisterTransport(this);
    transportSource.setSource(0);
}

//------------------------------------------------------------------------------
void FilePlayerProcessor::setFile(const File& phil)
{
    int readAheadSize;

    soundFile = phil;

    // Unload the previous file source and delete it.
    transportSource.stop();
    transportSource.setSource(0);
    soundFileSource = 0;

    AudioFormatReader* reader = AudioFormatManagerSingleton::getInstance().createReaderFor(phil);

    if (reader != 0)
    {
        soundFileSource.reset(new AudioFormatReaderSource(reader, true));
        soundFileSource->setLooping(looping);

        if (soundFileSource->getTotalLength() < 32768)
            readAheadSize = 0;
        else
            readAheadSize = 32768;

        // Plug it into our transport source.
        transportSource.setSource(soundFileSource.get(),
                                  readAheadSize, // Tells it to buffer this many samples ahead.
                                  &(AudioThumbnailCacheSingleton::getInstance().getTimeSliceThread()));
    }
}

//------------------------------------------------------------------------------
Component* FilePlayerProcessor::getControls()
{
    FilePlayerControl* retval = new FilePlayerControl(this);

    return retval;
}

//------------------------------------------------------------------------------
void FilePlayerProcessor::updateEditorBounds(const Rectangle<int>& bounds)
{
    editorBounds = bounds;
}

//------------------------------------------------------------------------------
void FilePlayerProcessor::changeListenerCallback(ChangeBroadcaster* source)
{
    if (source == MainTransport::getInstance())
    {
        if (syncToMainTransport)
        {
            // Play/pause the transport source.
            if (MainTransport::getInstance()->getState())
            {
                if (!transportSource.isPlaying())
                    transportSource.start();
            }
            else
            {
                if (transportSource.isPlaying())
                {
                    justPaused = true;
                    transportSource.stop();
                }
            }

            // Return to zero if necessary.
            if (MainTransport::getInstance()->getReturnToZero())
            {
                transportSource.setPosition(0.0);
                sendChangeMessage();
            }
        }
    }
    else
    {
        if (!transportSource.isPlaying() && !justPaused)
        {
            transportSource.setPosition(0.0);
            MainTransport::getInstance()->transportFinished();
        }
        if (justPaused)
            justPaused = false;

        sendChangeMessage();
    }
}

//------------------------------------------------------------------------------
void FilePlayerProcessor::fillInPluginDescription(PluginDescription& description) const
{
    description.name = "File Player";
    description.descriptiveName = "Processor which plays back an audio file.";
    description.pluginFormatName = "Internal";
    description.category = "Pedalboard Processors";
    description.manufacturerName = "Niall Moody";
    description.version = "1.00";
    description.uniqueId = description.name.hashCode();
    description.isInstrument = false;
    description.numInputChannels = 0;
    description.numOutputChannels = 2;
}

//------------------------------------------------------------------------------
void FilePlayerProcessor::processBlock(AudioSampleBuffer& buffer, MidiBuffer& midiMessages)
{
    AudioSourceChannelInfo bufferInfo;

    jassert(buffer.getNumChannels() > 0);

    // Fill out the transportSource's buffer.
    bufferInfo.buffer = &buffer;
    bufferInfo.startSample = 0;
    bufferInfo.numSamples = buffer.getNumSamples();

    // Write the transportSource's audio to the buffer.
    transportSource.getNextAudioBlock(bufferInfo);
}

//------------------------------------------------------------------------------
void FilePlayerProcessor::prepareToPlay(double sampleRate, int estimatedSamplesPerBlock)
{
    // Why are the arguments the opposite way round???
    transportSource.prepareToPlay(estimatedSamplesPerBlock, sampleRate);
}

//------------------------------------------------------------------------------
void FilePlayerProcessor::releaseResources()
{
    transportSource.releaseResources();
}

//------------------------------------------------------------------------------
AudioProcessorEditor* FilePlayerProcessor::createEditor()
{
    return new FilePlayerEditor(this, editorBounds);
}

//------------------------------------------------------------------------------
const String FilePlayerProcessor::getParameterName(int parameterIndex)
{
    String retval;

    switch (parameterIndex)
    {
    case Play:
        retval = "Play";
        break;
    case ReturnToZero:
        retval = "Return to Zero";
        break;
    case Looping:
        retval = "Looping";
        break;
    case ReadPosition:
        retval = "Read Position";
        break;
    case SyncToMainTransport:
        retval = "Sync to Main Transport";
        break;
    case Trigger:
        retval = "Trigger";
        break;
    }

    return retval;
}

//------------------------------------------------------------------------------
float FilePlayerProcessor::getParameter(int parameterIndex)
{
    float retval = 0.0f;

    switch (parameterIndex)
    {
    case Looping:
        retval = looping ? 1.0f : 0.0f;
        break;
    case SyncToMainTransport:
        retval = syncToMainTransport ? 1.0f : 0.0f;
        break;
    }

    return retval;
}

//------------------------------------------------------------------------------
const String FilePlayerProcessor::getParameterText(int parameterIndex)
{
    String retval;

    switch (parameterIndex)
    {
    case Looping:
        if (looping)
            retval = "looping";
        else
            retval = "not looping";
        break;
    case SyncToMainTransport:
        if (syncToMainTransport)
            retval = "synced";
        else
            retval = "not synced";
        break;
    }

    return retval;
}

//------------------------------------------------------------------------------
void FilePlayerProcessor::setParameter(int parameterIndex, float newValue)
{
    switch (parameterIndex)
    {
    case Play:
        if (newValue > 0.5f)
        {
            if (transportSource.isPlaying())
            {
                justPaused = true;
                transportSource.stop();
            }
            else
                transportSource.start();
        }
        sendChangeMessage();
        break;
    case ReturnToZero:
        transportSource.setPosition(0.0);
        sendChangeMessage();
        break;
    case Looping:
        if (newValue > 0.5f)
            looping = true;
        else
            looping = false;
        if (soundFileSource)
            soundFileSource->setLooping(looping);
        sendChangeMessage();
        break;
    case ReadPosition:
        transportSource.setPosition(newValue * transportSource.getLengthInSeconds());
        sendChangeMessage();
        break;
    case SyncToMainTransport:
        if (newValue > 0.5f)
            syncToMainTransport = true;
        else
            syncToMainTransport = false;
        sendChangeMessage();
        break;
    case Trigger:
        if (newValue > 0.5f)
        {
            transportSource.setPosition(0.0);

            if (!transportSource.isPlaying())
                transportSource.start();

            sendChangeMessage();
        }
        break;
    }
}

//------------------------------------------------------------------------------
void FilePlayerProcessor::getStateInformation(MemoryBlock& destData)
{
    XmlElement xml("Pedalboard3FilePlayerSettings");

    xml.setAttribute("file", soundFile.getFullPathName());
    xml.setAttribute("looping", looping);
    xml.setAttribute("syncToMainTransport", syncToMainTransport);

    xml.setAttribute("editorX", editorBounds.getX());
    xml.setAttribute("editorY", editorBounds.getY());
    xml.setAttribute("editorW", editorBounds.getWidth());
    xml.setAttribute("editorH", editorBounds.getHeight());

    copyXmlToBinary(xml, destData);
}

//------------------------------------------------------------------------------
void FilePlayerProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    if (xmlState != 0)
    {
        if (xmlState->hasTagName("Pedalboard3FilePlayerSettings"))
        {
            setFile(xmlState->getStringAttribute("file"));
            looping = xmlState->getBoolAttribute("looping");
            if (soundFileSource)
                soundFileSource->setLooping(looping);
            syncToMainTransport = xmlState->getBoolAttribute("syncToMainTransport");

            editorBounds.setX(xmlState->getIntAttribute("editorX"));
            editorBounds.setY(xmlState->getIntAttribute("editorY"));
            editorBounds.setWidth(xmlState->getIntAttribute("editorW"));
            editorBounds.setHeight(xmlState->getIntAttribute("editorH"));
        }
    }
}
