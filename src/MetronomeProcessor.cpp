//	MetronomeProcessor.cpp - Simple metronome.
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
#include "MainTransport.h"
#include "MetronomeControl.h"
#include "PedalboardProcessorEditors.h"
#include "PedalboardProcessors.h"


//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
MetronomeProcessor::MetronomeProcessor()
    : playing(false), syncToMainTransport(false), numerator(4), denominator(4), sineX0(1.0f), sineX1(0.0f),
      sineEnv(0.0f), clickCount(0.0f), clickDec(0.0f), measureCount(0), isAccent(true)
{
    setPlayConfigDetails(0, 1, 0, 0);

    MainTransport::getInstance()->registerTransport(this);
}

//------------------------------------------------------------------------------
MetronomeProcessor::~MetronomeProcessor()
{
    removeAllChangeListeners();
    MainTransport::getInstance()->unregisterTransport(this);
    transportSource[0].setSource(0);
    transportSource[1].setSource(0);
}

//------------------------------------------------------------------------------
void MetronomeProcessor::setAccentFile(const File& phil)
{
    int readAheadSize;

    files[0] = phil;

    // Unload the previous file source and delete it.
    transportSource[0].stop();
    transportSource[0].setSource(0);
    soundFileSource[0] = 0;

    AudioFormatReader* reader = AudioFormatManagerSingleton::getInstance().createReaderFor(phil);

    if (reader != 0)
    {
        soundFileSource[0].reset(new AudioFormatReaderSource(reader, true));
        soundFileSource[0]->setLooping(false);

        if (soundFileSource[0]->getTotalLength() < 32768)
            readAheadSize = 0;
        else
            readAheadSize = 32768;

        // Plug it into our transport source.
        transportSource[0].setSource(soundFileSource[0].get(),
                                     readAheadSize, // Tells it to buffer this many samples ahead.
                                     &(AudioThumbnailCacheSingleton::getInstance().getTimeSliceThread()));
    }
    else
        files[0] = File();
}

//------------------------------------------------------------------------------
void MetronomeProcessor::setClickFile(const File& phil)
{
    int readAheadSize;

    files[1] = phil;

    // Unload the previous file source and delete it.
    transportSource[1].stop();
    transportSource[1].setSource(0);
    soundFileSource[1] = 0;

    AudioFormatReader* reader = AudioFormatManagerSingleton::getInstance().createReaderFor(phil);

    if (reader != 0)
    {
        soundFileSource[1].reset(new AudioFormatReaderSource(reader, true));
        soundFileSource[1]->setLooping(false);

        if (soundFileSource[1]->getTotalLength() < 32768)
            readAheadSize = 0;
        else
            readAheadSize = 32768;

        // Plug it into our transport source.
        transportSource[1].setSource(soundFileSource[1].get(),
                                     readAheadSize, // Tells it to buffer this many samples ahead.
                                     &(AudioThumbnailCacheSingleton::getInstance().getTimeSliceThread()));
    }
    else
        files[1] = File();
}

//------------------------------------------------------------------------------
Component* MetronomeProcessor::getControls()
{
    MetronomeControl* retval = new MetronomeControl(this, false);

    return retval;
}

//------------------------------------------------------------------------------
void MetronomeProcessor::updateEditorBounds(const Rectangle<int>& bounds)
{
    editorBounds = bounds;
}

//------------------------------------------------------------------------------
void MetronomeProcessor::changeListenerCallback(ChangeBroadcaster* source)
{
    if (source == MainTransport::getInstance())
    {
        if (syncToMainTransport)
        {
            // Play/pause the transport source.
            if (MainTransport::getInstance()->getState())
            {
                if (!playing)
                {
                    clickCount = 0.0f;
                    measureCount = 0;

                    playing = true;
                }
            }
            else
            {
                if (playing)
                    playing = false;
            }
            sendChangeMessage();
        }
    }
}

//------------------------------------------------------------------------------
void MetronomeProcessor::fillInPluginDescription(PluginDescription& description) const
{
    description.name = "Metronome";
    description.descriptiveName = "Simple metronome.";
    description.pluginFormatName = "Internal";
    description.category = "Pedalboard Processors";
    description.manufacturerName = "Niall Moody";
    description.version = "1.00";
    description.uniqueId = description.name.hashCode();
    description.isInstrument = false;
    description.numInputChannels = 0;
    description.numOutputChannels = 1;
}

//------------------------------------------------------------------------------
void MetronomeProcessor::processBlock(AudioSampleBuffer& buffer, MidiBuffer& midiMessages)
{
    int i;
    float* data;
    bool skipTransport = false;
    AudioSourceChannelInfo bufferInfo;
    const int numSamples = buffer.getNumSamples();

    jassert(buffer.getNumChannels() > 0);

    data = buffer.getWritePointer(0);

    // Clear the buffer first.
    for (i = 0; i < numSamples; ++i)
        data[i] = 0.0f;

    for (i = 0; i < numSamples; ++i)
    {
        if (playing)
        {
            clickCount -= clickDec;
            if (clickCount <= 0.0f)
            {
                sineX0 = 1.0f;
                sineX1 = 0.0f;

                // The accent.
                if (!measureCount)
                {
                    sineCoeff = 2.0f * sinf(3.1415926535897932384626433832795f * 880.0f * 2.0f / 44100.0f);
                    measureCount = numerator;
                    isAccent = true;

                    if (files[0] != File())
                    {
                        transportSource[0].setPosition(0.0);
                        transportSource[0].start();

                        // Fill out the transportSource's buffer.
                        bufferInfo.buffer = &buffer;
                        bufferInfo.startSample = i;
                        bufferInfo.numSamples = numSamples - i;

                        transportSource[0].getNextAudioBlock(bufferInfo);
                        skipTransport = true;
                    }
                    else
                        sineEnv = 1.0f;
                }
                else
                {
                    sineCoeff = 2.0f * sinf(3.1415926535897932384626433832795f * 440.0f * 2.0f / 44100.0f);
                    isAccent = false;

                    if (files[1] != File())
                    {
                        transportSource[1].setPosition(0.0);
                        transportSource[1].start();

                        // Fill out the transportSource's buffer.
                        bufferInfo.buffer = &buffer;
                        bufferInfo.startSample = i;
                        bufferInfo.numSamples = numSamples - i;

                        transportSource[1].getNextAudioBlock(bufferInfo);
                        skipTransport = true;
                    }
                    else
                        sineEnv = 1.0f;
                }

                --measureCount;

                // Update clickCount.
                {
                    AudioPlayHead* playHead = getPlayHead();
                    AudioPlayHead::CurrentPositionInfo posInfo;

                    if (playHead)
                        getPlayHead()->getCurrentPosition(posInfo);
                    else
                        posInfo.bpm = 120.0f;

                    clickDec = (float)(1.0f / getSampleRate());
                    clickCount += (float)(60.0 / posInfo.bpm) * (4.0f / denominator);
                }
            }

            // if(isAccent && (files[0] != File()))
            if (isAccent && (transportSource[0].isPlaying()))
            {
            }
            // else if(!isAccent && (files[1] !=File()))
            else if (!isAccent && (transportSource[1].isPlaying()))
            {
            }
            else if (sineEnv > 0.0f)
            {
                sineX0 -= sineCoeff * sineX1;
                sineX1 += sineCoeff * sineX0;

                data[i] = sineX1 * sineEnv;

                sineEnv -= 0.0001f;
                if (sineEnv < 0.0f)
                    sineEnv = 0.0f;
            }
        }
    }

    if (!skipTransport)
    {
        // Fill out the transportSource's buffer.
        bufferInfo.buffer = &buffer;
        bufferInfo.startSample = 0;
        bufferInfo.numSamples = numSamples;

        if (transportSource[0].isPlaying())
            transportSource[0].getNextAudioBlock(bufferInfo);
        if (transportSource[1].isPlaying())
            transportSource[1].getNextAudioBlock(bufferInfo);
    }
}

//------------------------------------------------------------------------------
AudioProcessorEditor* MetronomeProcessor::createEditor()
{
    return new MetronomeEditor(this, editorBounds);
}

//------------------------------------------------------------------------------
const String MetronomeProcessor::getParameterName(int parameterIndex)
{
    String retval;

    switch (parameterIndex)
    {
    case Play:
        retval = "Play";
        break;
    case SyncToMainTransport:
        retval = "Sync to Main Transport";
        break;
    }

    return retval;
}

//------------------------------------------------------------------------------
float MetronomeProcessor::getParameter(int parameterIndex)
{
    float retval = 0.0f;

    switch (parameterIndex)
    {
    case Numerator:
        retval = (float)numerator;
        break;
    case Denominator:
        retval = (float)denominator;
        break;
    case SyncToMainTransport:
        retval = syncToMainTransport ? 1.0f : 0.0f;
        break;
    }

    return retval;
}

//------------------------------------------------------------------------------
const String MetronomeProcessor::getParameterText(int parameterIndex)
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
void MetronomeProcessor::setParameter(int parameterIndex, float newValue)
{
    switch (parameterIndex)
    {
    case Play:
        if (newValue > 0.5f)
        {
            if (!playing)
            {
                clickCount = 0.0f;
                measureCount = 0;

                playing = true;
            }
            else if (playing)
                playing = false;
            sendChangeMessage();
        }
        break;
    case Numerator:
        numerator = (int)newValue;
        break;
    case Denominator:
        denominator = (int)newValue;
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
void MetronomeProcessor::getStateInformation(MemoryBlock& destData)
{
    XmlElement xml("Pedalboard3MetronomeSettings");

    xml.setAttribute("editorX", editorBounds.getX());
    xml.setAttribute("editorY", editorBounds.getY());
    xml.setAttribute("editorW", editorBounds.getWidth());
    xml.setAttribute("editorH", editorBounds.getHeight());

    xml.setAttribute("syncToMainTransport", syncToMainTransport);
    xml.setAttribute("numerator", numerator);
    xml.setAttribute("denominator", denominator);
    xml.setAttribute("accentFile", files[0].getFullPathName());
    xml.setAttribute("clickFile", files[1].getFullPathName());

    copyXmlToBinary(xml, destData);
}

//------------------------------------------------------------------------------
void MetronomeProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    if (xmlState != 0)
    {
        if (xmlState->hasTagName("Pedalboard3MetronomeSettings"))
        {
            editorBounds.setX(xmlState->getIntAttribute("editorX"));
            editorBounds.setY(xmlState->getIntAttribute("editorY"));
            editorBounds.setWidth(xmlState->getIntAttribute("editorW"));
            editorBounds.setHeight(xmlState->getIntAttribute("editorH"));

            syncToMainTransport = xmlState->getBoolAttribute("syncToMainTransport");
            numerator = xmlState->getIntAttribute("numerator");
            denominator = xmlState->getIntAttribute("denominator");
            setAccentFile(File(xmlState->getStringAttribute("accentFile")));
            setClickFile(File(xmlState->getStringAttribute("clickFile")));
        }
    }
}
