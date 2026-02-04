//	RecorderProcessor.cpp - Simple audio recorder.
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

#include "AudioRecorderControl.h"
#include "AudioSingletons.h"
#include "MainTransport.h"
#include "PedalboardProcessorEditors.h"
#include "PedalboardProcessors.h"


//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
RecorderProcessor::RecorderProcessor()
    : threadWriter(0),
      thumbnail(512, AudioFormatManagerSingleton::getInstance(), AudioThumbnailCacheSingleton::getInstance()),
      recording(false), stopRecording(false), syncToMainTransport(false), currentRate(44100.0)
{
    setPlayConfigDetails(2, 0, 0, 0);

    MainTransport::getInstance()->registerTransport(this);
}

//------------------------------------------------------------------------------
RecorderProcessor::~RecorderProcessor()
{
    removeAllChangeListeners();
    MainTransport::getInstance()->unregisterTransport(this);

    if (threadWriter)
        delete threadWriter;
}

//------------------------------------------------------------------------------
void RecorderProcessor::setFile(const File& phil)
{
    WavAudioFormat wavFormat;
    StringPairArray metadata;
    AudioFormatWriter* writer;
    FileOutputStream* philStream;

    if (recording)
    {
        stopRecording = true;

        // Wait till the end of the current audio buffer.
        while (recording)
        {
            Thread::sleep(10);
        }
    }

    if (threadWriter)
    {
        delete threadWriter;
        threadWriter = 0;

        thumbnail.clear();
    }

    soundFile = phil;

    // So we overwrite any previous file.
    if (soundFile.existsAsFile())
    {
        if (!soundFile.deleteFile())
        {
            AlertWindow::showMessageBox(AlertWindow::WarningIcon, "Could not delete existing file",
                                        "Have you got the file open elsewhere? (e.g. "
                                        "in another File Player)");

            soundFile = File();
        }
    }

    if (soundFile != File())
    {
        philStream = new FileOutputStream(soundFile);

        writer = wavFormat.createWriterFor(philStream, currentRate, 2, 16, metadata, 0);
        if (!writer)
        {
            delete philStream;
            soundFile = File();
            threadWriter = 0;
        }
        else
        {
            threadWriter = new AudioFormatWriter::ThreadedWriter(
                writer, AudioThumbnailCacheSingleton::getInstance().getTimeSliceThread(), 16384);
            threadWriter->setDataReceiver(&thumbnail);
        }
    }
}

//------------------------------------------------------------------------------
void RecorderProcessor::cacheFile(const File& phil)
{
    soundFile = phil;
}

//------------------------------------------------------------------------------
Component* RecorderProcessor::getControls()
{
    AudioRecorderControl* retval = new AudioRecorderControl(this, thumbnail);

    return retval;
}

//------------------------------------------------------------------------------
void RecorderProcessor::updateEditorBounds(const Rectangle<int>& bounds)
{
    editorBounds = bounds;
}

//------------------------------------------------------------------------------
void RecorderProcessor::changeListenerCallback(ChangeBroadcaster* source)
{
    if (source == MainTransport::getInstance())
    {
        if (syncToMainTransport)
        {
            // Play/pause the transport source.
            if (MainTransport::getInstance()->getState())
            {
                if (!recording)
                    setFile(soundFile);

                if (!recording && !stopRecording && threadWriter)
                    recording = true;
            }
            else
            {
                if (recording)
                {
                    stopRecording = true;

                    setFile(File());
                }
            }
            sendChangeMessage();
        }
    }
}

//------------------------------------------------------------------------------
void RecorderProcessor::fillInPluginDescription(PluginDescription& description) const
{
    description.name = "Audio Recorder";
    description.descriptiveName = "Simple audio recorder.";
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
void RecorderProcessor::processBlock(AudioSampleBuffer& buffer, MidiBuffer& midiMessages)
{
    float* data[2];

    jassert(buffer.getNumChannels() > 1);

    data[0] = buffer.getWritePointer(0);
    data[1] = buffer.getWritePointer(1);

    if (recording && threadWriter)
    {
        threadWriter->write((const float**)data, buffer.getNumSamples());

        if (stopRecording)
        {
            recording = false;
            stopRecording = false;
        }
    }
    else if (recording)
    {
        recording = false;
        stopRecording = false;
    }
}

//------------------------------------------------------------------------------
AudioProcessorEditor* RecorderProcessor::createEditor()
{
    return new AudioRecorderEditor(this, editorBounds, thumbnail);
}

//------------------------------------------------------------------------------
void RecorderProcessor::prepareToPlay(double sampleRate, int estimatedSamplesPerBlock)
{
    currentRate = sampleRate;
}

//------------------------------------------------------------------------------
const String RecorderProcessor::getParameterName(int parameterIndex)
{
    String retval;

    switch (parameterIndex)
    {
    case Record:
        retval = "Record";
        break;
    case SyncToMainTransport:
        retval = "Sync to Main Transport";
        break;
    }

    return retval;
}

//------------------------------------------------------------------------------
float RecorderProcessor::getParameter(int parameterIndex)
{
    float retval = 0.0f;

    switch (parameterIndex)
    {
    case SyncToMainTransport:
        retval = syncToMainTransport ? 1.0f : 0.0f;
        break;
    }

    return retval;
}

//------------------------------------------------------------------------------
const String RecorderProcessor::getParameterText(int parameterIndex)
{
    String retval;

    switch (parameterIndex)
    {
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
void RecorderProcessor::setParameter(int parameterIndex, float newValue)
{
    switch (parameterIndex)
    {
    case Record:
        if (newValue > 0.5f)
        {
            if (!recording)
                setFile(soundFile);

            if (!recording && !stopRecording && threadWriter)
                recording = true;
            else if (recording)
            {
                stopRecording = true;

                // Saves the file to disk.
                setFile(File());

                if (syncToMainTransport)
                    MainTransport::getInstance()->transportFinished();
            }
            sendChangeMessage();
        }
        break;
    case SyncToMainTransport:
        if (newValue > 0.5f)
            syncToMainTransport = true;
        else
            syncToMainTransport = false;
        sendChangeMessage();
        break;
    }
}

//------------------------------------------------------------------------------
void RecorderProcessor::getStateInformation(MemoryBlock& destData)
{
    XmlElement xml("Pedalboard3RecorderSettings");

    xml.setAttribute("editorX", editorBounds.getX());
    xml.setAttribute("editorY", editorBounds.getY());
    xml.setAttribute("editorW", editorBounds.getWidth());
    xml.setAttribute("editorH", editorBounds.getHeight());

    xml.setAttribute("file", soundFile.getFullPathName());
    xml.setAttribute("syncToMainTransport", syncToMainTransport);

    copyXmlToBinary(xml, destData);
}

//------------------------------------------------------------------------------
void RecorderProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    if (xmlState != 0)
    {
        if (xmlState->hasTagName("Pedalboard3RecorderSettings"))
        {
            editorBounds.setX(xmlState->getIntAttribute("editorX"));
            editorBounds.setY(xmlState->getIntAttribute("editorY"));
            editorBounds.setWidth(xmlState->getIntAttribute("editorW"));
            editorBounds.setHeight(xmlState->getIntAttribute("editorH"));

            setFile(xmlState->getStringAttribute("file"));
            syncToMainTransport = xmlState->getBoolAttribute("syncToMainTransport");
        }
    }
}
