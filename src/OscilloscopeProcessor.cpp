/*
  ==============================================================================

    OscilloscopeProcessor.cpp
    Real-time audio oscilloscope with embedded display

  ==============================================================================
*/

#include "OscilloscopeProcessor.h"
#include "OscilloscopeControl.h"

//==============================================================================
OscilloscopeProcessor::OscilloscopeProcessor()
{
    circularBuffer.fill(0.0f);
    displaySnapshot.fill(0.0f);
}

OscilloscopeProcessor::~OscilloscopeProcessor() = default;

//==============================================================================
Component* OscilloscopeProcessor::getControls()
{
    return new OscilloscopeControl(this);
}

void OscilloscopeProcessor::updateEditorBounds(const Rectangle<int>& bounds)
{
    editorBounds = bounds;
}

void OscilloscopeProcessor::getDisplayBuffer(std::array<float, DISPLAY_SAMPLES>& output) const
{
    // Copy the pre-built snapshot - no race condition
    output = displaySnapshot;
}

//==============================================================================
void OscilloscopeProcessor::fillInPluginDescription(PluginDescription& description) const
{
    description.name = "Oscilloscope";
    description.descriptiveName = "Oscilloscope";
    description.pluginFormatName = "Internal";
    description.category = "Built-in";
    description.manufacturerName = "Pedalboard";
    description.version = "1.0";
    description.fileOrIdentifier = "Oscilloscope";
    description.isInstrument = false;
    description.numInputChannels = 2;
    description.numOutputChannels = 2;
}

void OscilloscopeProcessor::prepareToPlay(double sampleRate, int)
{
    currentSampleRate = sampleRate;
    circularBuffer.fill(0.0f);
    displaySnapshot.fill(0.0f);
    writePos = 0;
    lastSampleWasNegative = true;
    samplesSinceTrigger = DISPLAY_SAMPLES; // Start ready for new trigger
}

void OscilloscopeProcessor::processBlock(AudioSampleBuffer& buffer, MidiBuffer&)
{
    if (buffer.getNumChannels() == 0)
        return;

    const float* inputData = buffer.getReadPointer(0);
    int numSamples = buffer.getNumSamples();

    float trigger = triggerLevel.load();

    for (int i = 0; i < numSamples; ++i)
    {
        float sample = inputData[i];

        // Write to circular buffer
        circularBuffer[writePos] = sample;

        // Check for trigger (rising edge zero-crossing)
        bool isNegative = sample < trigger;
        if (lastSampleWasNegative && !isNegative && samplesSinceTrigger >= DISPLAY_SAMPLES)
        {
            // New trigger - start counting samples for snapshot
            samplesSinceTrigger = 0;
        }
        lastSampleWasNegative = isNegative;

        // Build snapshot after trigger
        if (samplesSinceTrigger < DISPLAY_SAMPLES)
        {
            displaySnapshot[samplesSinceTrigger] = sample;
            samplesSinceTrigger++;
        }

        writePos = (writePos + 1) % BUFFER_SIZE;
    }
}

//==============================================================================
AudioProcessorEditor* OscilloscopeProcessor::createEditor()
{
    return nullptr; // Uses embedded controls
}

void OscilloscopeProcessor::getStateInformation(MemoryBlock& destData)
{
    // Could save trigger level here if needed
}

void OscilloscopeProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    // Could restore trigger level here if needed
}
