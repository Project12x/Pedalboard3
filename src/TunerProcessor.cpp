/*
  ==============================================================================

    TunerProcessor.cpp
    Monophonic chromatic tuner implementation

  ==============================================================================
*/

#include "TunerProcessor.h"

#include "TunerControl.h"

#include <chrono>
#include <cmath>
#include <thread>

namespace
{
constexpr int kTunerAudioPinY = 78;
}

//==============================================================================
TunerProcessor::TunerProcessor() : PedalboardProcessor()
{
    setPlayConfigDetails(1, 1, 0, 0);
}

TunerProcessor::~TunerProcessor()
{
    stopAnalysisThread();
}

//==============================================================================
void TunerProcessor::prepareToPlay(double newSampleRate, int estimatedSamplesPerBlock)
{
    stopAnalysisThread();

    sampleRate = newSampleRate;
    bufferWritePos = 0;
    samplesAvailable = 0;
    samplesUntilNextAnalysis = 0;
    writeAnalysisWindowSlot = 0;
    analysisRing.fill(0.0f);
    for (auto& window : analysisWindows)
        window.fill(0.0f);

    publishedAnalysisWindowSlot.store(-1, std::memory_order_release);
    publishedAnalysisSequence.store(0, std::memory_order_release);
    analysisWindowPending.store(false, std::memory_order_release);
    pitchDetected.store(false, std::memory_order_release);
    detectedFrequency.store(0.0f, std::memory_order_release);
    detectedNote.store(-1, std::memory_order_release);
    centsDeviation.store(0.0f, std::memory_order_release);
    strobePhase.store(0.0f, std::memory_order_release);

    backgroundAnalyzer.prepare(newSampleRate, estimatedSamplesPerBlock);
    startAnalysisThread();
}

void TunerProcessor::releaseResources()
{
    stopAnalysisThread();
}

//==============================================================================
void TunerProcessor::processBlock(AudioSampleBuffer& buffer, MidiBuffer& midiMessages)
{
    ignoreUnused(midiMessages);

    if (buffer.getNumChannels() == 0 || buffer.getNumSamples() == 0)
        return;

    const float* inputData = buffer.getReadPointer(0);
    const int numSamples = buffer.getNumSamples();
    constexpr int analysisWindowSize = pedalboard3::dsp::TunerAnalysis::kAnalysisWindowSize;

    for (int i = 0; i < numSamples; ++i)
    {
        analysisRing[static_cast<size_t>(bufferWritePos)] = inputData[i];
        bufferWritePos = (bufferWritePos + 1) % analysisWindowSize;

        if (samplesAvailable < analysisWindowSize)
        {
            ++samplesAvailable;
            continue;
        }

        --samplesUntilNextAnalysis;
        if (samplesUntilNextAnalysis <= 0)
        {
            samplesUntilNextAnalysis = ANALYSIS_HOP;
            publishAnalysisWindow();
        }
    }

    if (muteOutput.load())
        buffer.clear();
}

//==============================================================================
void TunerProcessor::startAnalysisThread()
{
    if (analysisThread.joinable())
        return;

    stopAnalysisThreadFlag.store(false, std::memory_order_release);
    analysisThread = std::thread([this] { analysisThreadMain(); });
}

void TunerProcessor::stopAnalysisThread() noexcept
{
    stopAnalysisThreadFlag.store(true, std::memory_order_release);

    if (analysisThread.joinable() && analysisThread.get_id() != std::this_thread::get_id())
        analysisThread.join();

    analysisWindowPending.store(false, std::memory_order_release);
}

void TunerProcessor::publishAnalysisWindow() noexcept
{
    if (analysisWindowPending.load(std::memory_order_acquire))
        return;

    constexpr int analysisWindowSize = pedalboard3::dsp::TunerAnalysis::kAnalysisWindowSize;
    auto& target = analysisWindows[static_cast<size_t>(writeAnalysisWindowSlot)];
    for (int i = 0; i < analysisWindowSize; ++i)
    {
        const int sourceIndex = (bufferWritePos + i) % analysisWindowSize;
        target[static_cast<size_t>(i)] = analysisRing[static_cast<size_t>(sourceIndex)];
    }

    publishedAnalysisWindowSlot.store(writeAnalysisWindowSlot, std::memory_order_release);
    publishedAnalysisSequence.fetch_add(1, std::memory_order_acq_rel);
    analysisWindowPending.store(true, std::memory_order_release);
    writeAnalysisWindowSlot = (writeAnalysisWindowSlot + 1) % 2;
}

void TunerProcessor::analysisThreadMain() noexcept
{
    while (!stopAnalysisThreadFlag.load(std::memory_order_acquire))
    {
        if (!analysisWindowPending.exchange(false, std::memory_order_acquire))
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
            continue;
        }

        const int slot = publishedAnalysisWindowSlot.load(std::memory_order_acquire);
        if (slot < 0 || slot >= 2)
            continue;

        backgroundAnalyzer.reset();
        backgroundAnalyzer.pushSamples(analysisWindows[static_cast<size_t>(slot)].data(),
                                       pedalboard3::dsp::TunerAnalysis::kAnalysisWindowSize);
        auto result = backgroundAnalyzer.analyze();
        applyAnalysisResult(result);
    }
}

void TunerProcessor::applyAnalysisResult(const pedalboard3::dsp::TunerAnalysisResult& result) noexcept
{
    if (result.detected && result.frequencyHz > 20.0f && result.frequencyHz < 5000.0f && result.midiNote >= 0)
    {
        detectedFrequency.store(result.frequencyHz, std::memory_order_release);
        detectedNote.store(result.midiNote, std::memory_order_release);
        centsDeviation.store(result.cents, std::memory_order_release);
        pitchDetected.store(true, std::memory_order_release);
        updateStrobePhase(result.frequencyHz, result.midiNote, result.referenceA4Hz);
        return;
    }

    pitchDetected.store(false, std::memory_order_release);
    detectedFrequency.store(0.0f, std::memory_order_release);
    detectedNote.store(-1, std::memory_order_release);
    centsDeviation.store(0.0f, std::memory_order_release);
}

//==============================================================================
void TunerProcessor::updateStrobePhase(float frequency, int midiNote, float referenceA4Hz) noexcept
{
    if (!std::isfinite(frequency) || !std::isfinite(referenceA4Hz) || frequency <= 0.0f || referenceA4Hz <= 0.0f)
        return;

    const float targetFreq = referenceA4Hz * std::pow(2.0f, static_cast<float>(midiNote - A4_MIDI) / 12.0f);
    if (!std::isfinite(targetFreq) || targetFreq <= 0.0f)
        return;

    // Phase rotates based on frequency error.
    const float freqError = frequency - targetFreq;
    const float phaseRate = freqError * 0.01f;

    float currentPhase = strobePhase.load();
    currentPhase += phaseRate;

    // Wrap phase to 0-1.
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

PedalboardProcessor::PinLayout TunerProcessor::getInputPinLayout() const
{
    PinLayout layout;
    layout.pinY.push_back(kTunerAudioPinY);
    return layout;
}

PedalboardProcessor::PinLayout TunerProcessor::getOutputPinLayout() const
{
    PinLayout layout;
    layout.pinY.push_back(kTunerAudioPinY);
    return layout;
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
