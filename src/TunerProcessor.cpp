/*
  ==============================================================================

    TunerProcessor.cpp
    Dual-mode chromatic tuner implementation

  ==============================================================================
*/

#include "TunerProcessor.h"

#include "TunerControl.h"

#include <cmath>

//==============================================================================
TunerProcessor::TunerProcessor() : PedalboardProcessor() {}

TunerProcessor::~TunerProcessor() {}

//==============================================================================
void TunerProcessor::prepareToPlay(double newSampleRate, int estimatedSamplesPerBlock)
{
    sampleRate = newSampleRate;
    bufferWritePos = 0;
    samplesUntilNextAnalysis = 0;
    analysisBuffer.fill(0.0f);
    yinBuffer.fill(0.0f);
}

//==============================================================================
void TunerProcessor::processBlock(AudioSampleBuffer& buffer, MidiBuffer& midiMessages)
{
    ignoreUnused(midiMessages);

    if (buffer.getNumChannels() == 0 || buffer.getNumSamples() == 0)
        return;

    // Use first channel for pitch detection
    const float* inputData = buffer.getReadPointer(0);
    const int numSamples = buffer.getNumSamples();

    for (int i = 0; i < numSamples; ++i)
    {
        // Fill analysis buffer
        analysisBuffer[bufferWritePos] = inputData[i];
        bufferWritePos = (bufferWritePos + 1) % BUFFER_SIZE;

        --samplesUntilNextAnalysis;

        if (samplesUntilNextAnalysis <= 0)
        {
            samplesUntilNextAnalysis = ANALYSIS_HOP;

            // Create contiguous buffer for analysis
            std::array<float, BUFFER_SIZE> contiguousBuffer;
            for (int j = 0; j < BUFFER_SIZE; ++j)
            {
                contiguousBuffer[j] = analysisBuffer[(bufferWritePos + j) % BUFFER_SIZE];
            }

            // Run pitch detection
            float frequency = detectPitchYIN(contiguousBuffer.data(), BUFFER_SIZE);

            if (frequency > 20.0f && frequency < 5000.0f)
            {
                detectedFrequency.store(frequency);
                pitchDetected.store(true);
                updateNoteAndCents(frequency);
                updateStrobePhase(frequency);
            }
            else
            {
                pitchDetected.store(false);
                detectedNote.store(-1);
                centsDeviation.store(0.0f);
            }
        }
    }

    if (muteOutput.load())
        buffer.clear();
}

//==============================================================================
float TunerProcessor::detectPitchYIN(const float* samples, int numSamples)
{
    const int halfSize = numSamples / 2;
    const float yinThreshold = 0.15f;

    // Step 1 & 2: Calculate difference function and cumulative mean normalized difference
    yinBuffer[0] = 1.0f;
    float runningSum = 0.0f;

    for (int tau = 1; tau < halfSize; ++tau)
    {
        float sum = 0.0f;
        for (int j = 0; j < halfSize; ++j)
        {
            float delta = samples[j] - samples[j + tau];
            sum += delta * delta;
        }

        runningSum += sum;
        yinBuffer[tau] = (runningSum > 0.0f) ? (sum * tau / runningSum) : 1.0f;
    }

    // Step 3: Absolute threshold
    int tauEstimate = -1;
    for (int tau = 2; tau < halfSize; ++tau)
    {
        if (yinBuffer[tau] < yinThreshold)
        {
            // Find local minimum
            while (tau + 1 < halfSize && yinBuffer[tau + 1] < yinBuffer[tau])
            {
                ++tau;
            }
            tauEstimate = tau;
            break;
        }
    }

    if (tauEstimate == -1)
        return 0.0f;

    // Step 4: Parabolic interpolation for better accuracy
    float betterTau = static_cast<float>(tauEstimate);
    if (tauEstimate > 0 && tauEstimate < halfSize - 1)
    {
        float s0 = yinBuffer[tauEstimate - 1];
        float s1 = yinBuffer[tauEstimate];
        float s2 = yinBuffer[tauEstimate + 1];
        betterTau = tauEstimate + (s2 - s0) / (2.0f * (2.0f * s1 - s2 - s0));
    }

    return static_cast<float>(sampleRate) / betterTau;
}

//==============================================================================
void TunerProcessor::updateNoteAndCents(float frequency)
{
    // Calculate MIDI note number from frequency
    float midiNoteFloat = 12.0f * std::log2(frequency / A4_FREQ) + A4_MIDI;
    int midiNote = static_cast<int>(std::round(midiNoteFloat));

    // Calculate cents deviation
    float exactNote = 12.0f * std::log2(frequency / A4_FREQ) + A4_MIDI;
    float cents = (exactNote - midiNote) * 100.0f;

    detectedNote.store(midiNote);
    centsDeviation.store(cents);
}

//==============================================================================
void TunerProcessor::updateStrobePhase(float frequency)
{
    // Calculate target frequency for the detected note
    int midiNote = detectedNote.load();
    float targetFreq = A4_FREQ * std::pow(2.0f, (midiNote - A4_MIDI) / 12.0f);

    // Phase rotates based on frequency error
    float freqError = frequency - targetFreq;
    float phaseRate = freqError * 0.01f; // Scale down for visible rotation

    float currentPhase = strobePhase.load();
    currentPhase += phaseRate;

    // Wrap phase to 0-1
    while (currentPhase >= 1.0f)
        currentPhase -= 1.0f;
    while (currentPhase < 0.0f)
        currentPhase += 1.0f;

    strobePhase.store(currentPhase);
}

//==============================================================================
void TunerProcessor::getStateInformation(MemoryBlock& destData)
{
    // Save strobe mode preference if desired
    MemoryOutputStream stream(destData, false);
    stream.writeInt(1); // version
}

void TunerProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    MemoryInputStream stream(data, static_cast<size_t>(sizeInBytes), false);
    int version = stream.readInt();
    ignoreUnused(version);
}

//==============================================================================
Component* TunerProcessor::getControls()
{
    return new TunerControl(this);
}

AudioProcessorEditor* TunerProcessor::createEditor()
{
    return nullptr; // Not used - we use getControls() instead
}

void TunerProcessor::updateEditorBounds(const Rectangle<int>& bounds)
{
    editorBounds = bounds;
}

//==============================================================================
void TunerProcessor::fillInPluginDescription(PluginDescription& description) const
{
    description.name = "Tuner";
    description.descriptiveName = "Chromatic Tuner";
    description.pluginFormatName = "Internal";
    description.category = "Pedalboard Processors";
    description.manufacturerName = "Pedalboard3";
    description.version = "1.0.0";
    description.fileOrIdentifier = "Tuner";
    description.uniqueId = 0x54554E52; // "TUNR"
    description.isInstrument = false;
    description.numInputChannels = 1;
    description.numOutputChannels = 1;
}
