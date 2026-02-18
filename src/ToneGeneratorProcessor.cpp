/*
  ==============================================================================

    ToneGeneratorProcessor.cpp
    Test signal generator implementation

  ==============================================================================
*/

#include "ToneGeneratorProcessor.h"

#include "ToneGeneratorControl.h"

//==============================================================================
ToneGeneratorProcessor::ToneGeneratorProcessor()
{
    setPlayConfigDetails(0, 2, 44100.0, 512); // No inputs, stereo output
}

ToneGeneratorProcessor::~ToneGeneratorProcessor() {}

//==============================================================================
void ToneGeneratorProcessor::fillInPluginDescription(PluginDescription& description) const
{
    description.name = "Tone Generator";
    description.descriptiveName = "Test signal generator for tuner and plugin testing";
    description.pluginFormatName = "Internal";
    description.category = "Test Tools";
    description.manufacturerName = "Pedalboard3";
    description.version = "1.0";
    description.fileOrIdentifier = "tonegenerator";
    description.lastFileModTime = Time();
    description.lastInfoUpdateTime = Time();
    description.uniqueId = 0x746F6E65; // "tone"
    description.isInstrument = true;
    description.numInputChannels = 0;
    description.numOutputChannels = 2;
}

//==============================================================================
void ToneGeneratorProcessor::prepareToPlay(double newSampleRate, int estimatedSamplesPerBlock)
{
    sampleRate = newSampleRate;
    phase = 0.0;

    // Calculate initial phase increment
    float actualFreq = getActualFrequency();
    phaseIncrement = actualFreq / sampleRate;
}

float ToneGeneratorProcessor::getActualFrequency() const
{
    float base = baseFrequency.load();
    float cents = detuneCents.load();

    // Apply cents offset: freq * 2^(cents/1200)
    return base * std::pow(2.0f, cents / 1200.0f);
}

//==============================================================================
void ToneGeneratorProcessor::setFrequency(float freqHz)
{
    baseFrequency.store(jlimit(20.0f, 20000.0f, freqHz));
}

void ToneGeneratorProcessor::setMidiNote(int midiNote)
{
    setFrequency(midiNoteToFrequency(midiNote));
}

void ToneGeneratorProcessor::setDetuneCents(float cents)
{
    detuneCents.store(jlimit(-100.0f, 100.0f, cents));
}

void ToneGeneratorProcessor::setWaveform(Waveform wf)
{
    currentWaveform.store(wf);
}

void ToneGeneratorProcessor::setTestMode(TestMode mode)
{
    currentTestMode.store(mode);
    // Reset test mode state
    sweepPosition = 0.0f;
    driftPhase = 0.0f;
    octaveJumpIndex = 0;
}

void ToneGeneratorProcessor::setAmplitude(float amp)
{
    amplitude.store(jlimit(0.0f, 1.0f, amp));
}

void ToneGeneratorProcessor::setPlaying(bool shouldPlay)
{
    playing.store(shouldPlay);
    if (shouldPlay)
        phase = 0.0; // Reset phase on start
}

//==============================================================================
float ToneGeneratorProcessor::midiNoteToFrequency(int midiNote)
{
    return A4_FREQ * std::pow(2.0f, (midiNote - A4_MIDI) / 12.0f);
}

int ToneGeneratorProcessor::frequencyToMidiNote(float frequency)
{
    if (frequency <= 0.0f)
        return -1;
    return static_cast<int>(std::round(12.0f * std::log2(frequency / A4_FREQ) + A4_MIDI));
}

float ToneGeneratorProcessor::frequencyToCents(float frequency, int targetNote)
{
    float targetFreq = midiNoteToFrequency(targetNote);
    return 1200.0f * std::log2(frequency / targetFreq);
}

//==============================================================================
void ToneGeneratorProcessor::processBlock(AudioSampleBuffer& buffer, MidiBuffer& midiMessages)
{
    ignoreUnused(midiMessages);

    int numSamples = buffer.getNumSamples();
    int numChannels = buffer.getNumChannels();

    if (!playing.load())
    {
        buffer.clear();
        return;
    }

    // Update test mode modulation
    updateTestModeModulation();

    // Calculate current phase increment
    float actualFreq = getActualFrequency();
    phaseIncrement = actualFreq / sampleRate;

    float amp = amplitude.load();

    for (int sample = 0; sample < numSamples; ++sample)
    {
        float sampleValue = generateSample() * amp;

        // Write to all channels (mono signal, stereo output)
        for (int ch = 0; ch < numChannels; ++ch)
        {
            buffer.setSample(ch, sample, sampleValue);
        }

        updatePhase();
    }
}

float ToneGeneratorProcessor::generateSample()
{
    Waveform wf = currentWaveform.load();

    switch (wf)
    {
    case Waveform::Sine:
        return static_cast<float>(std::sin(phase * MathConstants<double>::twoPi));

    case Waveform::Saw:
        // Saw wave: -1 to +1 over the phase cycle
        return static_cast<float>(2.0 * phase - 1.0);

    case Waveform::Square:
        return (phase < 0.5) ? 1.0f : -1.0f;

    case Waveform::WhiteNoise:
        return random.nextFloat() * 2.0f - 1.0f;

    default:
        return 0.0f;
    }
}

void ToneGeneratorProcessor::updatePhase()
{
    phase += phaseIncrement;
    if (phase >= 1.0)
        phase -= 1.0;
}

void ToneGeneratorProcessor::updateTestModeModulation()
{
    TestMode mode = currentTestMode.load();

    switch (mode)
    {
    case TestMode::Static:
        // No modulation
        break;

    case TestMode::Sweep:
    {
        // Sweep from 20Hz to 2kHz over ~10 seconds
        sweepPosition += 0.0001f;
        if (sweepPosition > 1.0f)
            sweepPosition = 0.0f;

        float freq = 20.0f * std::pow(100.0f, sweepPosition); // Log sweep
        baseFrequency.store(freq);
        break;
    }

    case TestMode::Drift:
    {
        // Slow ±5 cent drift (tests tuner stability)
        driftPhase += 0.00005f;
        if (driftPhase > 1.0f)
            driftPhase -= 1.0f;

        float driftCents = 5.0f * std::sin(driftPhase * MathConstants<float>::twoPi);
        detuneCents.store(driftCents);
        break;
    }

    case TestMode::OctaveJump:
    {
        // Jump between octaves every ~2 seconds
        static int sampleCounter = 0;
        sampleCounter++;
        if (sampleCounter > static_cast<int>(sampleRate * 2.0))
        {
            sampleCounter = 0;
            octaveJumpIndex = (octaveJumpIndex + 1) % 4;

            // A2, A3, A4, A5
            float octaveFreqs[] = {110.0f, 220.0f, 440.0f, 880.0f};
            baseFrequency.store(octaveFreqs[octaveJumpIndex]);
        }
        break;
    }

    case TestMode::RandomJump:
    {
        // Random frequency change every ~1 second
        static int sampleCounter = 0;
        sampleCounter++;
        if (sampleCounter > static_cast<int>(sampleRate))
        {
            sampleCounter = 0;
            // Random frequency 55Hz - 880Hz (A1 - A5)
            float freq = 55.0f * std::pow(2.0f, random.nextFloat() * 4.0f);
            baseFrequency.store(freq);
        }
        break;
    }
    }
}

//==============================================================================
Component* ToneGeneratorProcessor::getControls()
{
    return new ToneGeneratorControl(this);
}

AudioProcessorEditor* ToneGeneratorProcessor::createEditor()
{
    return nullptr; // Uses getControls() instead
}

void ToneGeneratorProcessor::updateEditorBounds(const Rectangle<int>& bounds)
{
    editorBounds = bounds;
}

//==============================================================================
void ToneGeneratorProcessor::getStateInformation(MemoryBlock& destData)
{
    XmlElement state("ToneGeneratorState");
    state.setAttribute("frequency", static_cast<double>(baseFrequency.load()));
    state.setAttribute("detune", static_cast<double>(detuneCents.load()));
    state.setAttribute("amplitude", static_cast<double>(amplitude.load()));
    state.setAttribute("waveform", static_cast<int>(currentWaveform.load()));
    state.setAttribute("testMode", static_cast<int>(currentTestMode.load()));

    copyXmlToBinary(state, destData);
}

void ToneGeneratorProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    auto xmlState = getXmlFromBinary(data, sizeInBytes);
    if (xmlState != nullptr && xmlState->hasTagName("ToneGeneratorState"))
    {
        baseFrequency.store(static_cast<float>(xmlState->getDoubleAttribute("frequency", 440.0)));
        detuneCents.store(static_cast<float>(xmlState->getDoubleAttribute("detune", 0.0)));
        amplitude.store(static_cast<float>(xmlState->getDoubleAttribute("amplitude", 0.5)));
        currentWaveform.store(static_cast<Waveform>(xmlState->getIntAttribute("waveform", 0)));
        currentTestMode.store(static_cast<TestMode>(xmlState->getIntAttribute("testMode", 0)));
    }
}
