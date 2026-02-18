//	BypassableInstance.cpp - Wrapper class to provide a bypass to
//							 AudioPluginInstance.
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

#include "BypassableInstance.h"

#include <spdlog/spdlog.h>

//------------------------------------------------------------------------------
BypassableInstance::BypassableInstance(AudioPluginInstance* plug) : plugin(plug), tempBuffer(2, 4096), bypassRamp(0.0f)
{
    jassert(plugin);

    // The default AudioProcessor constructor creates 1 stereo input + 1 stereo output bus.
    // Synth plugins (e.g. Vapor Keys, Surge XT) have 0 input buses + 1 stereo output bus.
    // setBusesLayout requires matching bus counts, so remove excess buses first.
    configuringBuses = true;
    // Remove excess buses
    while (getBusCount(true) > plugin->getBusCount(true))
        removeBus(true);
    while (getBusCount(false) > plugin->getBusCount(false))
        removeBus(false);
    // Add missing buses (for multi-bus plugins)
    while (getBusCount(true) < plugin->getBusCount(true))
        addBus(true);
    while (getBusCount(false) < plugin->getBusCount(false))
        addBus(false);
    configuringBuses = false;

    // Now bus counts match, so setBusesLayout will succeed
    auto layout = plugin->getBusesLayout();
    setBusesLayout(layout);

    spdlog::info(
        "[BypassableInstance] ctor '{}': plugin buses in={} out={}, wrapper in={} out={}, channels in={} out={}",
        plugin->getName().toStdString(), plugin->getBusCount(true), plugin->getBusCount(false), getBusCount(true),
        getBusCount(false), getTotalNumInputChannels(), getTotalNumOutputChannels());

    // Cache channel info NOW, before this node is added to the audio graph.
    // Once the audio thread starts calling processBlock, querying the VST3
    // plugin's bus state from the UI thread causes crashes (race condition).
    cachedAcceptsMidi = plugin->acceptsMidi();
    cachedProducesMidi = plugin->producesMidi();

    cachedInputChannelCount = 0;
    for (int busIdx = 0; busIdx < plugin->getBusCount(true); ++busIdx)
    {
        if (auto* bus = plugin->getBus(true, busIdx))
        {
            int numCh = bus->getNumberOfChannels();
            for (int ch = 0; ch < numCh; ++ch)
            {
                auto chLayout = bus->getCurrentLayout();
                cachedInputChannelNames.add(chLayout.getChannelTypeName(chLayout.getTypeOfChannel(ch)));
            }
            cachedInputChannelCount += numCh;
        }
    }
    if (cachedInputChannelCount == 0)
        cachedInputChannelCount = plugin->getTotalNumInputChannels();

    cachedOutputChannelCount = 0;
    for (int busIdx = 0; busIdx < plugin->getBusCount(false); ++busIdx)
    {
        if (auto* bus = plugin->getBus(false, busIdx))
        {
            int numCh = bus->getNumberOfChannels();
            for (int ch = 0; ch < numCh; ++ch)
            {
                auto chLayout = bus->getCurrentLayout();
                cachedOutputChannelNames.add(chLayout.getChannelTypeName(chLayout.getTypeOfChannel(ch)));
            }
            cachedOutputChannelCount += numCh;
        }
    }
    if (cachedOutputChannelCount == 0)
        cachedOutputChannelCount = plugin->getTotalNumOutputChannels();
}

//------------------------------------------------------------------------------
BypassableInstance::~BypassableInstance()
{
    delete plugin;
}

//------------------------------------------------------------------------------
void BypassableInstance::prepareToPlay(double sampleRate, int estimatedSamplesPerBlock)
{
    spdlog::info("[BypassableInstance::prepareToPlay] ENTER for '{}' sr={} blockSize={}",
                 plugin->getName().toStdString(), sampleRate, estimatedSamplesPerBlock);

    // Mark as not prepared during reconfiguration
    prepared.store(false);

    int numChannels;

    // Use modern channel count APIs
    int numInputs = plugin->getTotalNumInputChannels();
    int numOutputs = plugin->getTotalNumOutputChannels();

    if (numInputs > numOutputs)
        numChannels = numInputs;
    else
        numChannels = numOutputs;

    if (numChannels <= 0)
        numChannels = 2; // Fallback to stereo to prevent zero-size buffer

    midiCollector.reset(sampleRate);

    // Since we only get an estimate of the number of samples per block, multiply
    // that number by 2 to ensure we don't run out of space.
    tempBuffer.setSize(numChannels, (estimatedSamplesPerBlock * 2));

    spdlog::info("[BypassableInstance::prepareToPlay] tempBuffer: ch={} samples={}, plugin: in={} out={}",
                 tempBuffer.getNumChannels(), tempBuffer.getNumSamples(), numInputs, numOutputs);

    plugin->setPlayHead(getPlayHead());
    // Use modern bus layout instead of deprecated setPlayConfigDetails
    auto layout = plugin->getBusesLayout();
    plugin->setBusesLayout(layout);
    plugin->prepareToPlay(sampleRate, estimatedSamplesPerBlock);

    prepared.store(true);
    spdlog::info("[BypassableInstance::prepareToPlay] DONE");
}

//------------------------------------------------------------------------------
void BypassableInstance::resyncChannelCount()
{
    int numInputs = plugin->getTotalNumInputChannels();
    int numOutputs = plugin->getTotalNumOutputChannels();
    int numChannels = jmax(numInputs, numOutputs);

    if (numChannels <= 0)
        numChannels = 2;

    int currentTempChannels = tempBuffer.getNumChannels();

    spdlog::info("[BypassableInstance::resyncChannelCount] '{}' in={} out={} maxCh={} tempBufCh={}",
                 plugin->getName().toStdString(), numInputs, numOutputs, numChannels, currentTempChannels);

    if (numChannels != currentTempChannels)
    {
        // Resize tempBuffer to match new channel count.
        // Keep the same sample count (it was 2x blockSize from prepareToPlay).
        int numSamples = tempBuffer.getNumSamples();
        if (numSamples <= 0)
            numSamples = 1024; // fallback

        tempBuffer.setSize(numChannels, numSamples, false, true, true);

        spdlog::info("[BypassableInstance::resyncChannelCount] Resized tempBuffer to {}ch x {} samples", numChannels,
                     numSamples);
    }

    // Update the wrapper's own declared channel count so the graph
    // allocates the right buffer size for this node.
    setPlayConfigDetails(numInputs, numOutputs, getSampleRate(), getBlockSize());
}

//------------------------------------------------------------------------------
void BypassableInstance::releaseResources()
{
    plugin->releaseResources();
}

//------------------------------------------------------------------------------
void BypassableInstance::processBlock(AudioSampleBuffer& buffer, MidiBuffer& midiMessages)
{
    // Don't call into plugin before prepareToPlay completes
    if (!prepared.load())
        return;

    int i, j;
    float rampVal = bypassRamp;
    MidiBuffer tempMidi;
    MidiBuffer::Iterator it(midiMessages);

    const int bufferChannels = buffer.getNumChannels();
    const int bufferSamples = buffer.getNumSamples();
    const int pluginChannels = tempBuffer.getNumChannels();

    // The graph may pass a buffer with fewer channels than the plugin expects
    // (e.g., 0 channels for a synth with no input connections). We must provide
    // a buffer with enough channels for the plugin to write its output.
    const bool needTempForPlugin = (bufferChannels < pluginChannels);

    // Hard bounds check on sample count
    if (bufferSamples > tempBuffer.getNumSamples())
        return;

    // Pass on any MIDI messages received via OSC.
    midiCollector.removeNextBlockOfMessages(tempMidi, bufferSamples);
    if (!midiMessages.isEmpty())
    {
        MidiMessage tempMess;
        int tempSample;

        while (it.getNextEvent(tempMess, tempSample))
        {
            // Filter out any messages on the wrong channel.
            if ((midiChannel == 0) || (tempMess.getChannel() == midiChannel))
                tempMidi.addEvent(tempMess, tempSample);
        }
    }

    if (needTempForPlugin)
    {
        // Copy whatever input channels exist into tempBuffer, zero the rest
        for (i = 0; i < bufferChannels; ++i)
            tempBuffer.copyFrom(i, 0, buffer, i, 0, bufferSamples);
        for (i = bufferChannels; i < pluginChannels; ++i)
            tempBuffer.clear(i, 0, bufferSamples);

        // Process into tempBuffer (which has enough channels for the plugin)
        AudioSampleBuffer pluginBuffer(tempBuffer.getArrayOfWritePointers(), pluginChannels, bufferSamples);
        plugin->processBlock(pluginBuffer, tempMidi);

        // Copy back the channels that fit into the output buffer
        for (i = 0; i < bufferChannels; ++i)
            buffer.copyFrom(i, 0, tempBuffer, i, 0, bufferSamples);
    }
    else
    {
        // Normal path: buffer has enough channels
        // Save original audio for bypass crossfade
        // Clamp to tempBuffer's capacity to prevent overrun if plugin changed
        // its channel count after prepareToPlay
        const int safeCopyChannels = jmin(bufferChannels, pluginChannels);
        for (i = 0; i < safeCopyChannels; ++i)
            tempBuffer.copyFrom(i, 0, buffer, i, 0, bufferSamples);

        // Get the plugin's audio.
        plugin->processBlock(buffer, tempMidi);
    }

    // Add any new midi data to midiMessages.
    if (!tempMidi.isEmpty())
        midiMessages.swapWith(tempMidi);

    // Add the correct (bypassed or un-bypassed) audio back to the buffer.
    // Only apply bypass crossfade when we have the original audio saved
    if (!needTempForPlugin)
    {
        const int safeCrossfadeChannels = jmin(bufferChannels, pluginChannels);
        for (j = 0; j < safeCrossfadeChannels; ++j)
        {
            float* origData = tempBuffer.getWritePointer(j);
            float* newData = buffer.getWritePointer(j);

            rampVal = bypassRamp;
            for (i = 0; i < bufferSamples; ++i)
            {
                newData[i] = (origData[i] * rampVal) + (newData[i] * (1.0f - rampVal));

                if (bypass && (rampVal < 1.0f))
                {
                    rampVal += 0.001f;
                    if (rampVal > 1.0f)
                        rampVal = 1.0f;
                }
                else if (!bypass && (rampVal > 0.0f))
                {
                    rampVal -= 0.001f;
                    if (rampVal < 0.0f)
                        rampVal = 0.0f;
                }
            }
        }
        bypassRamp = rampVal;
    }
}

//------------------------------------------------------------------------------
void BypassableInstance::setBypass(bool val)
{
    bypass = val;
}

//------------------------------------------------------------------------------
void BypassableInstance::setMIDIChannel(int val)
{
    midiChannel = val;
}

//------------------------------------------------------------------------------
void BypassableInstance::addMidiMessage(const MidiMessage& message)
{
    if ((midiChannel == 0) || (message.getChannel() == midiChannel))
        midiCollector.addMessageToQueue(message);
}
