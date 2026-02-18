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
    : numerator(4), denominator(4), sineX0(1.0f), sineX1(0.0f),
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
}

//------------------------------------------------------------------------------
static void loadFileIntoBuffer(const File& phil, juce::AudioBuffer<float>& buffer, int& length)
{
    length = 0;
    buffer.setSize(0, 0);

    std::unique_ptr<AudioFormatReader> reader(
        AudioFormatManagerSingleton::getInstance().createReaderFor(phil));

    if (reader != nullptr)
    {
        int numSamples = (int)reader->lengthInSamples;
        int numChannels = (int)reader->numChannels;
        if (numChannels < 1) numChannels = 1;
        if (numChannels > 2) numChannels = 2;

        buffer.setSize(numChannels, numSamples);
        reader->read(&buffer, 0, numSamples, 0, true, numChannels > 1);
        length = numSamples;
    }
}

//------------------------------------------------------------------------------
void MetronomeProcessor::setAccentFile(const File& phil)
{
    // Guard: only write staging buffer when audio thread is not consuming it.
    // pendingClickReady == false means UI owns the staging buffer.
    if (pendingClickReady[0].load(std::memory_order_acquire))
        return; // Previous buffer not yet consumed by audio thread

    files[0] = phil;

    if (phil.existsAsFile())
    {
        loadFileIntoBuffer(phil, pendingClickBuffers[0], pendingClickBufferLength[0]);
        pendingClickReady[0].store(true, std::memory_order_release);
    }
    else
    {
        pendingClickBuffers[0].setSize(0, 0);
        pendingClickBufferLength[0] = 0;
        pendingClickReady[0].store(true, std::memory_order_release);
    }
}

//------------------------------------------------------------------------------
void MetronomeProcessor::setClickFile(const File& phil)
{
    // Guard: only write staging buffer when audio thread is not consuming it.
    if (pendingClickReady[1].load(std::memory_order_acquire))
        return; // Previous buffer not yet consumed by audio thread

    files[1] = phil;

    if (phil.existsAsFile())
    {
        loadFileIntoBuffer(phil, pendingClickBuffers[1], pendingClickBufferLength[1]);
        pendingClickReady[1].store(true, std::memory_order_release);
    }
    else
    {
        pendingClickBuffers[1].setSize(0, 0);
        pendingClickBufferLength[1] = 0;
        pendingClickReady[1].store(true, std::memory_order_release);
    }
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
                    clickCount.store(0.0f);
                    measureCount.store(0);

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
    const int numSamples = buffer.getNumSamples();

    jassert(buffer.getNumChannels() > 0);

    data = buffer.getWritePointer(0);

    // Consume any pending click buffer updates from UI thread (lock-free)
    for (int idx = 0; idx < 2; ++idx)
    {
        if (pendingClickReady[idx].load(std::memory_order_acquire))
        {
            std::swap(clickBuffers[idx], pendingClickBuffers[idx]);
            clickBufferLength[idx] = pendingClickBufferLength[idx];
            clickPlayPos[idx] = -1;
            pendingClickReady[idx].store(false, std::memory_order_release);
        }
    }

    // Clear the buffer first.
    for (i = 0; i < numSamples; ++i)
        data[i] = 0.0f;

    // Load cross-thread counters into locals for the per-sample loop.
    float localClickCount = clickCount.load();
    int localMeasureCount = measureCount.load();

    for (i = 0; i < numSamples; ++i)
    {
        if (playing)
        {
            localClickCount -= clickDec;
            if (localClickCount <= 0.0f)
            {
                sineX0 = 1.0f;
                sineX1 = 0.0f;

                // The accent.
                if (!localMeasureCount)
                {
                    sineCoeff = 2.0f * sinf(3.1415926535897932384626433832795f * 880.0f * 2.0f / 44100.0f);
                    localMeasureCount = numerator.load();
                    isAccent = true;

                    if (clickBufferLength[0] > 0)
                        clickPlayPos[0] = 0; // Trigger accent sample playback
                    else
                        sineEnv = 1.0f;
                }
                else
                {
                    sineCoeff = 2.0f * sinf(3.1415926535897932384626433832795f * 440.0f * 2.0f / 44100.0f);
                    isAccent = false;

                    if (clickBufferLength[1] > 0)
                        clickPlayPos[1] = 0; // Trigger click sample playback
                    else
                        sineEnv = 1.0f;
                }

                --localMeasureCount;

                // Update localClickCount.
                {
                    AudioPlayHead* playHead = getPlayHead();
                    AudioPlayHead::CurrentPositionInfo posInfo;

                    if (playHead)
                        getPlayHead()->getCurrentPosition(posInfo);
                    else
                        posInfo.bpm = 120.0f;

                    clickDec = (float)(1.0f / getSampleRate());
                    localClickCount += (float)(60.0 / posInfo.bpm) * (4.0f / denominator.load());
                }
            }

            // Play back preloaded click samples (RT-safe: just buffer reads)
            bool samplePlaying = false;
            for (int idx = 0; idx < 2; ++idx)
            {
                if (clickPlayPos[idx] >= 0 && clickPlayPos[idx] < clickBufferLength[idx])
                {
                    data[i] += clickBuffers[idx].getSample(0, clickPlayPos[idx]);
                    clickPlayPos[idx]++;
                    if (clickPlayPos[idx] >= clickBufferLength[idx])
                        clickPlayPos[idx] = -1; // Done playing
                    samplePlaying = true;
                }
            }

            // Fall back to sine click if no sample is playing
            if (!samplePlaying && sineEnv > 0.0f)
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

    // Store locals back to atomics.
    clickCount.store(localClickCount);
    measureCount.store(localMeasureCount);
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
        retval = (float)numerator.load();
        break;
    case Denominator:
        retval = (float)denominator.load();
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
                clickCount.store(0.0f);
                measureCount.store(0);

                playing = true;
            }
            else if (playing)
                playing = false;
            sendChangeMessage();
        }
        break;
    case Numerator:
        numerator.store((int)newValue);
        break;
    case Denominator:
        denominator.store((int)newValue);
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
    xml.setAttribute("numerator", numerator.load());
    xml.setAttribute("denominator", denominator.load());
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
            numerator.store(xmlState->getIntAttribute("numerator"));
            denominator.store(xmlState->getIntAttribute("denominator"));
            setAccentFile(File(xmlState->getStringAttribute("accentFile")));
            setClickFile(File(xmlState->getStringAttribute("clickFile")));
        }
    }
}
