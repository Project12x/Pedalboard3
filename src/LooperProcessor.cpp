//	LooperProcessor.cpp - Simple looper processor.
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
#include "LooperControl.h"
#include "LooperEditor.h"
#include "MainTransport.h"
#include "PedalboardProcessors.h"


//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
LooperProcessor::LooperProcessor()
    : playing(false), stopPlaying(false), recording(false), stopRecording(false), syncToMainTransport(false),
      stopAfterBar(false), autoPlay(true), inputLevel(1.0f), loopLevel(1.0f), threadWriter(0),
      thumbnail(512, AudioFormatManagerSingleton::getInstance(), AudioThumbnailCacheSingleton::getInstance()),
      numerator(4), denominator(4), clickCount(0.0f), clickDec(0.0f), measureCount(0), currentRate(44100.0),
      justPaused(false), loopLength(0), loopPos(0), loopIndex(0), deleteLastBuffer(false), tempBufferWrite(0),
      fadeOutCount(-1), fadeInCount(0), autoPlayFade(1.0f), fileReader(0), fileReaderPos(0), fileReaderBufIndex(0),
      newFileLoaded(false), inputAudio(2, 2560)
{
    int i;

    loopBuffer.add(new AudioSampleBuffer(2, (44100 * 8)));
    loopBuffer[0]->clear();

    for (i = 0; i < FadeBufferSize; ++i)
    {
        tempBuffer[0][i] = 0.0f;
        tempBuffer[1][i] = 0.0f;

        fadeInBuffer[0][i] = 0.0f;
        fadeInBuffer[1][i] = 0.0f;

        fadeOutBuffer[0][i] = 0.0f;
        fadeOutBuffer[1][i] = 0.0f;
    }

    AudioThumbnailCacheSingleton::getInstance().getTimeSliceThread().addTimeSliceClient(this);

    setPlayConfigDetails(2, 2, 0, 0);

    MainTransport::getInstance()->registerTransport(this);
}

//------------------------------------------------------------------------------
LooperProcessor::~LooperProcessor()
{
    // Stop any playback/recording immediately to prevent audio thread access
    playing = false;
    stopPlaying = true;
    recording = false;
    stopRecording = true;

    // Cancel any pending async updates to prevent callback after destruction
    cancelPendingUpdate();

    // Remove from time slice thread FIRST to ensure no more callbacks
    AudioThumbnailCacheSingleton::getInstance().getTimeSliceThread().removeTimeSliceClient(this);

    // Small delay to let any in-progress audio callbacks complete
    Thread::sleep(10);

    // Saves the file.
    setFile(File());

    removeAllChangeListeners();
    MainTransport::getInstance()->unregisterTransport(this);

    if (threadWriter)
        delete threadWriter;

    if (fileReader)
        delete fileReader;
}

//------------------------------------------------------------------------------
void LooperProcessor::setFile(const File& phil)
{
    int i;

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
    }

    // Stop playback before modifying buffers (to avoid deadlock)
    if (playing)
    {
        stopPlaying = true;

        // Wait till the audio thread stops playing.
        while (playing)
        {
            Thread::sleep(10);
        }
    }

    soundFile = phil;

    if (soundFile != File())
    {
        if (fileReader)
            delete fileReader;
        fileReader = AudioFormatManagerSingleton::getInstance().createReaderFor(soundFile);
        fileReaderPos = 0;
        fileReaderBufIndex = 0;

        // Calculate ratio for resampling if file sample rate differs from device rate
        if (fileReader && currentRate > 0)
            fileReaderRatio = fileReader->sampleRate / currentRate;
        else
            fileReaderRatio = 1.0;
    }

    loopLength = 0;
    loopIndex = 0;
    loopPos = 0;

    // Remove all loop buffers except the 1st one.
    for (i = (loopBuffer.size() - 1); i > 0; --i)
        loopBuffer.remove(i);
}

//------------------------------------------------------------------------------
double LooperProcessor::getReadPosition() const
{
    if (loopLength > 0)
        return (double)(((double)loopIndex * (double)LoopBufferSize) + (double)loopPos) / (double)loopLength;
    else
        return 0.0;
}

//------------------------------------------------------------------------------
void LooperProcessor::handleAsyncUpdate()
{
    int abortCounter = 2000; //== 2 seconds, assuming a perfect 1 millisecond wait
                             // from Thread::sleep().
    WavAudioFormat wavFormat;
    StringPairArray metadata;
    AudioFormatWriter* writer;
    FileOutputStream* philStream;

    // Just in case.
    if (playing)
        stopPlaying = true;
    while (playing)
    {
        Thread::sleep(1);
        --abortCounter;
        if (!abortCounter)
        {
            AlertWindow::showMessageBox(AlertWindow::WarningIcon, "Looper Error",
                                        "Unable to start recording; Looper still playing. Aborting.");

            return;
        }
    };

    thumbnail.reset(2, getSampleRate());

    setFile(soundFile);

    if (fileReader)
    {
        delete fileReader;
        fileReader = 0;
    }

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

    if (threadWriter)
    {
        if (playing)
            stopPlaying = true;
        while (playing)
        {
            Thread::sleep(10);
        }

        clickCount = 0.0f;
        clickDec = 0.0f;
        measureCount = 0;
        fadeOutCount = -1;
        if (autoPlay)
            autoPlayFade = 0.0f;

        recording = true;
    }
}

//------------------------------------------------------------------------------
int LooperProcessor::useTimeSlice()
{
    int retval = 250; // Wait 1/4 second before checking again.

    if (recording)
    {
        if (loopIndex == (loopBuffer.size() - 1))
        {
            // 16538 = ~3/8 of a second. Should be enough considering the wait
            // time we return from this method?
            if (loopPos > (LoopBufferSize - 16538))
            {
                loopBuffer.add(new AudioSampleBuffer(2, LoopBufferSize));
                loopBuffer[loopBuffer.size() - 1]->clear();
            }
        }
    }
    else if (deleteLastBuffer)
    {
        loopBuffer.remove(loopBuffer.size() - 1);
        deleteLastBuffer = false;
    }
    else if (fileReader)
    {
        if (fileReaderBufIndex >= loopBuffer.size())
        {
            loopBuffer.add(new AudioSampleBuffer(2, LoopBufferSize));
            loopBuffer[loopBuffer.size() - 1]->clear();

            jassert(fileReaderBufIndex < loopBuffer.size());
        }

        // Calculate how many source samples we need to read
        int64 sourceSamplesToRead = (int64)std::ceil(LoopBufferSize * fileReaderRatio);

        // Clamp to remaining samples in file
        int64 remainingInFile = fileReader->lengthInSamples - fileReaderPos;
        if (sourceSamplesToRead > remainingInFile)
            sourceSamplesToRead = remainingInFile;

        if (sourceSamplesToRead > 0 && fileReaderRatio != 1.0)
        {
            // Need to resample: read into temp buffer, then resample into loop buffer
            AudioSampleBuffer tempBuffer(2, (int)sourceSamplesToRead);
            fileReader->read(&tempBuffer, 0, (int)sourceSamplesToRead, fileReaderPos, true, true);

            // Use LagrangeInterpolator for each channel
            int outputSamples = (int)(sourceSamplesToRead / fileReaderRatio);
            if (outputSamples > LoopBufferSize)
                outputSamples = LoopBufferSize;

            LagrangeInterpolator interpolatorL, interpolatorR;
            interpolatorL.process(fileReaderRatio, tempBuffer.getReadPointer(0),
                                  loopBuffer[fileReaderBufIndex]->getWritePointer(0), outputSamples);
            interpolatorR.process(fileReaderRatio, tempBuffer.getReadPointer(1),
                                  loopBuffer[fileReaderBufIndex]->getWritePointer(1), outputSamples);

            fileReaderPos += sourceSamplesToRead;
            loopLength += outputSamples;
        }
        else if (sourceSamplesToRead > 0)
        {
            // No resampling needed (same sample rate)
            fileReader->read(loopBuffer[fileReaderBufIndex], 0, (int)sourceSamplesToRead, fileReaderPos, true, true);
            fileReaderPos += sourceSamplesToRead;
            loopLength += sourceSamplesToRead;
        }

        ++fileReaderBufIndex;

        if (fileReaderPos >= fileReader->lengthInSamples)
        {
            delete fileReader;
            fileReader = 0;
        }

        // While we're reading the file, have a shorter delay between calls
        // so it gets done faster.
        retval = 20;
    }

    return retval;
}

//------------------------------------------------------------------------------
Component* LooperProcessor::getControls()
{
    return new LooperControl(this, &thumbnail);
}

//------------------------------------------------------------------------------
void LooperProcessor::updateEditorBounds(const Rectangle<int>& bounds)
{
    editorBounds = bounds;
}

//------------------------------------------------------------------------------
void LooperProcessor::changeListenerCallback(ChangeBroadcaster* source)
{
    if (source == MainTransport::getInstance())
    {
        if (syncToMainTransport)
        {
            // Play/pause the transport source.
            if (MainTransport::getInstance()->getState() && !playing)
                setParameter(Play, 1.0f);
            else if (playing && !stopPlaying)
                setParameter(Play, 1.0f);

            if (MainTransport::getInstance()->getReturnToZero())
                setParameter(ReturnToZero, 1.0f);

            sendChangeMessage();
        }
    }
}

//------------------------------------------------------------------------------
void LooperProcessor::fillInPluginDescription(PluginDescription& description) const
{
    description.name = "Looper";
    description.descriptiveName = "Simple looper processor.";
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
void LooperProcessor::processBlock(AudioSampleBuffer& buffer, MidiBuffer& midiMessages)
{
    int i, j;
    float tempf;
    float* data[2];
    uint64_t end, end2;
    float* inputData[2];
    int fadeOutStart = 0;
    int samplesToRecord = buffer.getNumSamples();
    const int numSamples = buffer.getNumSamples();

    jassert(buffer.getNumChannels() > 1);

    data[0] = buffer.getWritePointer(0);
    data[1] = buffer.getWritePointer(1);

    inputAudio.copyFrom(0, 0, buffer, 0, 0, numSamples);
    inputAudio.copyFrom(1, 0, buffer, 1, 0, numSamples);
    inputData[0] = inputAudio.getWritePointer(0);
    inputData[1] = inputAudio.getWritePointer(1);

    if (recording && threadWriter)
    {
        // If we're stopping after a bar, need to keep track of where we are in
        // the bar.
        if (stopAfterBar)
        {
            for (i = 0; i < samplesToRecord; ++i)
            {
                clickCount -= clickDec;
                if (clickCount <= 0.0f)
                {
                    AudioPlayHead* playHead = getPlayHead();
                    AudioPlayHead::CurrentPositionInfo posInfo;

                    ++measureCount;
                    if (measureCount == (numerator + 1))
                    {
                        samplesToRecord = i;
                        stopRecording = true;
                        fadeInCount = (loopLength - 1) + samplesToRecord;

                        break;
                    }

                    if (playHead)
                        getPlayHead()->getCurrentPosition(posInfo);
                    else
                        posInfo.bpm = 120.0f;

                    clickDec = (float)(1.0f / getSampleRate());
                    clickCount += (float)(60.0 / posInfo.bpm) * (4.0f / denominator);
                }
            }
        }

        /// Write the audio data to the file.
        threadWriter->write((const float**)data, samplesToRecord);

        // Copy fade in buffer if necessary.
        if ((loopPos == 0) && (loopIndex == 0))
            fillFadeInBuffer();

        if (loopIndex < loopBuffer.size())
        {
            if ((loopPos + samplesToRecord) < LoopBufferSize)
                i = samplesToRecord;
            else
                i = LoopBufferSize - loopPos;

            // Write the audio data to the current loop buffer.
            loopBuffer[loopIndex]->copyFrom(0, loopPos, buffer, 0, 0, i);
            loopBuffer[loopIndex]->copyFrom(1, loopPos, buffer, 1, 0, i);

            tempf = loopBuffer[loopIndex]->getWritePointer(0, 0)[0];
            tempf = fadeInBuffer[0][127];
            if (i < samplesToRecord)
            {
                ++loopIndex;
                loopPos = 0;
                if (loopIndex >= loopBuffer.size())
                {
                    AlertWindow::showMessageBoxAsync(AlertWindow::WarningIcon, "Looper Error",
                                                     "Not enough memory to continue. Recording stopped.");
                    stopRecording = true;
                    loopLength += i;
                    fadeOutStart = i;
                }
                else
                {
                    loopBuffer[loopIndex]->copyFrom(0, 0, buffer, 0, i, (samplesToRecord - i));
                    loopBuffer[loopIndex]->copyFrom(1, 0, buffer, 1, i, (samplesToRecord - i));

                    loopPos = samplesToRecord - i;
                    loopLength += samplesToRecord;
                }
            }
            else
            {
                loopPos += samplesToRecord;
                loopLength += samplesToRecord;
            }
        }
        else
        {
            AlertWindow::showMessageBoxAsync(AlertWindow::WarningIcon, "Looper Error",
                                             "Not enough memory to continue. Recording stopped.");
            stopRecording = true;
        }

        if (stopRecording)
        {
            recording = false;
            stopRecording = false;
            if (autoPlay)
                playing = true;
            sendChangeMessage();

            if (loopIndex < (loopBuffer.size() - 1))
                deleteLastBuffer = true;

            loopPos = 0;
            loopIndex = 0;
            fadeInCount = (loopLength - 1);

            // Starts the count of [NumBufferSize] samples so we can store the
            // fade out buffer.
            fadeOutStart = samplesToRecord;
            fadeOutCount = 0;
        }
    }
    else if (recording)
    {
        recording = false;
        stopRecording = false;
        if (autoPlay)
            playing = true;
        sendChangeMessage();

        loopPos = 0;
        loopIndex = 0;
    }

    // Apply input level gain change.
    buffer.applyGain(0, numSamples, inputLevel);

    if (playing && (loopLength > 0))
    {
        if (loopBuffer.size() == 1)
            end = (uint64_t)loopLength;
        else if (loopIndex < (loopBuffer.size() - 1))
            end = (uint64_t)LoopBufferSize;
        else
            end = loopLength - (uint64_t)(LoopBufferSize * (loopBuffer.size() - 1));

        if ((loopPos + numSamples) < end)
            i = numSamples;
        else
            i = (int)(end - (uint64_t)loopPos);

        // Output the fade out buffer.
        if ((loopPos < FadeBufferSize) && (fadeOutCount == -1))
        {
            end2 = FadeBufferSize - loopPos;
            if (end2 > numSamples)
                end2 = numSamples;

            for (j = 0; j < end2; ++j)
            {
                tempf = 1.0f - ((float)(loopPos + j) / (float)FadeBufferSize);
                data[0][j] += fadeOutBuffer[0][loopPos + j] * tempf * loopLevel;
                data[1][j] += fadeOutBuffer[1][loopPos + j] * tempf * loopLevel;
            }
        }

        // Output the fade in buffer.
        for (j = 0; j < numSamples; ++j, --fadeInCount)
        {
            if (fadeInCount <= (FadeBufferSize - 1))
            {
                tempf = 1.0f - ((float)fadeInCount / (float)(FadeBufferSize - 1));
                data[0][j] += fadeInBuffer[0][FadeBufferSize - 1 - fadeInCount] * tempf * loopLevel;
                data[1][j] += fadeInBuffer[1][FadeBufferSize - 1 - fadeInCount] * tempf * loopLevel;

                if ((FadeBufferSize - 1 - fadeInCount) == 127)
                    tempf = fadeInBuffer[0][127];
            }

            if (fadeInCount == 0)
                fadeInCount = loopLength;
        }

        tempf = loopBuffer[loopIndex]->getWritePointer(0, 0)[0];
        if (autoPlayFade < 1.0f)
        {
            buffer.addFromWithRamp(0, 0, loopBuffer[loopIndex]->getWritePointer(0, loopPos), i, 0.0f, loopLevel);
            buffer.addFromWithRamp(1, 0, loopBuffer[loopIndex]->getWritePointer(1, loopPos), i, 0.0f, loopLevel);
            autoPlayFade = 1.0f;
        }
        else
        {
            buffer.addFrom(0, 0, *(loopBuffer[loopIndex]), 0, loopPos, i, loopLevel);
            buffer.addFrom(1, 0, *(loopBuffer[loopIndex]), 1, loopPos, i, loopLevel);
        }
        if (i < numSamples)
        {
            ++loopIndex;
            if (loopIndex >= loopBuffer.size())
                loopIndex = 0;
            loopPos = 0;

            if (fadeOutCount == -1)
            {
                end = FadeBufferSize - loopPos;
                if (end > (numSamples - i))
                    end = (numSamples - i);

                for (j = 0; j < end; ++j)
                {
                    tempf = 1.0f - ((float)(loopPos + j) / (float)FadeBufferSize);
                    data[0][i + j] += fadeOutBuffer[0][loopPos + j] * tempf * loopLevel;
                    data[1][i + j] += fadeOutBuffer[1][loopPos + j] * tempf * loopLevel;
                }
            }

            buffer.addFrom(0, i, *(loopBuffer[loopIndex]), 0, loopPos, (numSamples - i), loopLevel);
            buffer.addFrom(1, i, *(loopBuffer[loopIndex]), 1, loopPos, (numSamples - i), loopLevel);
            tempf = buffer.getWritePointer(0, 0)[0];

            loopPos = numSamples - i;
        }
        else
            loopPos += i;

        if (stopPlaying)
        {
            playing = false;
            stopPlaying = false;
        }
    }
    else if (stopPlaying)
    {
        playing = false;
        stopPlaying = false;
    }

    for (i = 0; i < numSamples; ++i)
    {
        if (fadeOutCount >= FadeBufferSize)
        {
            fadeOutCount = -1;
            fillFadeOutBuffer();
        }

        tempBuffer[0][tempBufferWrite] = inputData[0][i];
        tempBuffer[1][tempBufferWrite] = inputData[1][i];

        ++tempBufferWrite;
        tempBufferWrite %= FadeBufferSize;

        if ((i >= fadeOutStart) && (fadeOutCount > -1))
            ++fadeOutCount;
    }
}

//------------------------------------------------------------------------------
void LooperProcessor::prepareToPlay(double sampleRate, int estimatedSamplesPerBlock)
{
    currentRate = sampleRate;

    inputAudio.setSize(2, estimatedSamplesPerBlock);
}

//------------------------------------------------------------------------------
AudioProcessorEditor* LooperProcessor::createEditor()
{
    return new LooperEditor(this, &thumbnail);
}

//------------------------------------------------------------------------------
const String LooperProcessor::getParameterName(int parameterIndex)
{
    String retval;

    switch (parameterIndex)
    {
    case Play:
        retval = "Play/Pause";
        break;
    case ReturnToZero:
        retval = "Return to Zero";
        break;
    case Record:
        retval = "Record";
        break;
    case ReadPosition:
        retval = "Read Position";
        break;
    case SyncToMainTransport:
        retval = "Sync to Main Transport";
        break;
    case StopAfterBar:
        retval = "Stop After Bar";
        break;
    case AutoPlay:
        retval = "Auto-play";
        break;
    case BarNumerator:
        retval = "Bar Numerator";
        break;
    case BarDenominator:
        retval = "Bar Denominator";
        break;
    case InputLevel:
        retval = "Input Leve";
        break;
    case LoopLevel:
        retval = "Loop Leve";
        break;
    }

    return retval;
}

//------------------------------------------------------------------------------
float LooperProcessor::getParameter(int parameterIndex)
{
    float retval = 0.0f;

    switch (parameterIndex)
    {
    case SyncToMainTransport:
        retval = syncToMainTransport ? 1.0f : 0.0f;
        break;
    case StopAfterBar:
        retval = stopAfterBar ? 1.0f : 0.0f;
        break;
    case AutoPlay:
        retval = autoPlay ? 1.0f : 0.0f;
        break;
    case BarNumerator:
        retval = (float)numerator;
        break;
    case BarDenominator:
        retval = (float)denominator;
        break;
    case InputLevel:
        retval = inputLevel * 0.5f;
        break;
    case LoopLevel:
        retval = loopLevel * 0.5f;
        break;
    }

    return retval;
}

//------------------------------------------------------------------------------
const String LooperProcessor::getParameterText(int parameterIndex)
{
    String retval;

    return retval;
}

//------------------------------------------------------------------------------
void LooperProcessor::setParameter(int parameterIndex, float newValue)
{
    switch (parameterIndex)
    {
    case Play:
        if (newValue > 0.5f)
        {
            if (!playing)
                playing = true;
            else
                stopPlaying = true;
        }
        sendChangeMessage();
        break;
    case ReturnToZero:
        loopIndex = 0;
        loopPos = 0;
        fadeInCount = loopLength - 1;
        break;
    case Record:
        if (newValue > 0.5f)
        {
            if (playing)
                stopPlaying = true;

            if (!recording && !stopRecording) // && threadWriter)
            {
                triggerAsyncUpdate();
            }
            else if (recording)
            {
                stopRecording = true;

                if (syncToMainTransport)
                    MainTransport::getInstance()->transportFinished();
            }
            sendChangeMessage();
        }
        break;
    case ReadPosition:
    {
        int tempindex = 0;
        uint64_t tempint = 0;
        uint64_t pos = (uint64_t)(newValue * (float)loopLength);

        fadeInCount = loopLength - 1 - pos;

        // This is a stupid way of doing this, but my brain's not working
        // right now...
        while (tempint < pos)
        {
            ++tempindex;
            tempint += LoopBufferSize;
        }
        loopIndex = tempindex - 1;
        loopPos = pos % LoopBufferSize;

        sendChangeMessage();
    }
    break;
    case SyncToMainTransport:
        syncToMainTransport = (newValue > 0.5f) ? true : false;
        sendChangeMessage();
        break;
    case StopAfterBar:
        stopAfterBar = (newValue > 0.5f) ? true : false;
        break;
    case AutoPlay:
        autoPlay = (newValue > 0.5f) ? true : false;
        break;
    case BarNumerator:
        numerator = (int)newValue;
        break;
    case BarDenominator:
        denominator = (int)newValue;
        break;
    case InputLevel:
        inputLevel = newValue * 2.0f;
        break;
    case LoopLevel:
        loopLevel = newValue * 2.0f;
        break;
    }
}

//------------------------------------------------------------------------------
void LooperProcessor::getStateInformation(MemoryBlock& destData)
{
    XmlElement xml("Pedalboard3LooperSettings");

    xml.setAttribute("soundFile", soundFile.getFullPathName());
    xml.setAttribute("syncToMainTransport", syncToMainTransport);
    xml.setAttribute("stopAfterBar", stopAfterBar);
    xml.setAttribute("autoPlay", autoPlay);
    xml.setAttribute("barNumerator", numerator);
    xml.setAttribute("barDenominator", denominator);
    xml.setAttribute("inputLeve", inputLevel);
    xml.setAttribute("loopLeve", loopLevel);

    xml.setAttribute("editorX", editorBounds.getX());
    xml.setAttribute("editorY", editorBounds.getY());
    xml.setAttribute("editorW", editorBounds.getWidth());
    xml.setAttribute("editorH", editorBounds.getHeight());

    copyXmlToBinary(xml, destData);
}

//------------------------------------------------------------------------------
void LooperProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    if (xmlState != 0)
    {
        if (xmlState->hasTagName("Pedalboard3LooperSettings"))
        {
            editorBounds.setX(xmlState->getIntAttribute("editorX"));
            editorBounds.setY(xmlState->getIntAttribute("editorY"));
            editorBounds.setWidth(xmlState->getIntAttribute("editorW"));
            editorBounds.setHeight(xmlState->getIntAttribute("editorH"));

            // Load the file, indicate to change listeners a new file's been
            // loaded.
            setFile(File(xmlState->getStringAttribute("soundFile")));
            newFileLoaded = true;
            sendChangeMessage();

            syncToMainTransport = xmlState->getBoolAttribute("syncToMainTransport", false);
            stopAfterBar = xmlState->getBoolAttribute("stopAfterBar", false);
            autoPlay = xmlState->getBoolAttribute("autoPlay", true);
            numerator = xmlState->getIntAttribute("barNumerator", 4);
            denominator = xmlState->getIntAttribute("barDenominator", 4);
            inputLevel = (float)xmlState->getDoubleAttribute("inputLeve", 0.5);
            loopLevel = (float)xmlState->getDoubleAttribute("loopLevel", 0.5);
        }
    }
}

//------------------------------------------------------------------------------
void LooperProcessor::fillFadeInBuffer()
{
    int i, j;

    for (i = 0, j = tempBufferWrite; i < FadeBufferSize; ++i, ++j)
    {
        j %= FadeBufferSize;

        fadeInBuffer[0][i] = tempBuffer[0][j];
        fadeInBuffer[1][i] = tempBuffer[1][j];
    }
}

//------------------------------------------------------------------------------
void LooperProcessor::fillFadeOutBuffer()
{
    int i, j;

    for (i = 0, j = tempBufferWrite; i < FadeBufferSize; ++i, ++j)
    {
        j %= FadeBufferSize;

        fadeOutBuffer[0][i] = tempBuffer[0][j];
        fadeOutBuffer[1][i] = tempBuffer[1][j];
    }
}
