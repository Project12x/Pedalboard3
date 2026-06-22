/*
  ==============================================================================

    TunerProcessor.cpp
    Monophonic chromatic tuner implementation

  ==============================================================================
*/

#include "TunerProcessor.h"

#include "TunerControl.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <thread>

namespace
{
constexpr int kTunerAudioPinY = 78;
constexpr int kTunerStateVersion = 2;
constexpr float kDefaultReferenceA4Hz = 440.0f;
constexpr float kMinReferenceA4Hz = 400.0f;
constexpr float kMaxReferenceA4Hz = 480.0f;
constexpr int kStableAcquireWindows = 2;
constexpr int kFastAcquireWindows = 1;
constexpr int kStableHoldWindows = 6;
constexpr int kFastHoldWindows = 0;
constexpr std::array<int, 6> kStandardGuitarStringMidiNotes{{40, 45, 50, 55, 59, 64}};
constexpr float kGuitarStringInTuneCents = 4.0f;
constexpr float kGuitarStringCaptureRangeCents = 250.0f;

struct GuitarStringMatch
{
    int index = -1;
    float cents = 0.0f;
};

float sanitizeReferenceA4Hz(float frequencyHz) noexcept
{
    if (!std::isfinite(frequencyHz))
        return kDefaultReferenceA4Hz;

    return std::clamp(frequencyHz, kMinReferenceA4Hz, kMaxReferenceA4Hz);
}

float frequencyForMidiNote(int midiNote, float refA4Hz) noexcept
{
    return refA4Hz * std::pow(2.0f, static_cast<float>(midiNote - 69) / 12.0f);
}

GuitarStringMatch findClosestGuitarString(float frequencyHz, float refA4Hz) noexcept
{
    if (!std::isfinite(frequencyHz) || !std::isfinite(refA4Hz) || frequencyHz <= 0.0f || refA4Hz <= 0.0f)
        return {};

    GuitarStringMatch bestMatch;
    float bestAbsCents = kGuitarStringCaptureRangeCents;

    for (int i = 0; i < static_cast<int>(kStandardGuitarStringMidiNotes.size()); ++i)
    {
        const auto targetHz = frequencyForMidiNote(kStandardGuitarStringMidiNotes[static_cast<size_t>(i)], refA4Hz);
        if (!std::isfinite(targetHz) || targetHz <= 0.0f)
            continue;

        const auto cents = 1200.0f * std::log2(frequencyHz / targetHz);
        const auto absCents = std::abs(cents);
        if (absCents <= bestAbsCents)
        {
            bestAbsCents = absCents;
            bestMatch.index = i;
            bestMatch.cents = cents;
        }
    }

    return bestMatch;
}
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
    detectedConfidence.store(0.0f, std::memory_order_release);
    driftPhase.store(0.0f, std::memory_order_release);
    resetGuitarStringChecklist();
    resetResponseSmoothing();

    backgroundAnalyzer.prepare(newSampleRate, estimatedSamplesPerBlock);
    startAnalysisThread();
}

void TunerProcessor::releaseResources()
{
    stopAnalysisThread();
}

void TunerProcessor::setReferenceA4Hz(float frequencyHz) noexcept
{
    referenceA4Hz.store(sanitizeReferenceA4Hz(frequencyHz), std::memory_order_release);
}

TunerProcessor::ResponseMode TunerProcessor::getResponseMode() const noexcept
{
    const auto rawMode = responseMode.load(std::memory_order_acquire);
    return rawMode == static_cast<int>(ResponseMode::Fast) ? ResponseMode::Fast : ResponseMode::Stable;
}

void TunerProcessor::setResponseMode(ResponseMode mode) noexcept
{
    const auto sanitizedMode = mode == ResponseMode::Fast ? ResponseMode::Fast : ResponseMode::Stable;
    responseMode.store(static_cast<int>(sanitizedMode), std::memory_order_release);
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

        backgroundAnalyzer.setReferenceA4Hz(referenceA4Hz.load(std::memory_order_acquire));
        backgroundAnalyzer.reset();
        backgroundAnalyzer.pushSamples(analysisWindows[static_cast<size_t>(slot)].data(),
                                       pedalboard3::dsp::TunerAnalysis::kAnalysisWindowSize);
        auto result = backgroundAnalyzer.analyze();
        applyAnalysisResult(result);
    }
}

void TunerProcessor::applyAnalysisResult(const pedalboard3::dsp::TunerAnalysisResult& result) noexcept
{
    const auto mode = getResponseMode();
    const int requiredHits = mode == ResponseMode::Fast ? kFastAcquireWindows : kStableAcquireWindows;
    const int holdWindows = mode == ResponseMode::Fast ? kFastHoldWindows : kStableHoldWindows;

    if (result.detected && result.frequencyHz > 20.0f && result.frequencyHz < 5000.0f && result.midiNote >= 0)
    {
        if (candidateNote == result.midiNote)
            ++candidateHitCount;
        else
        {
            candidateNote = result.midiNote;
            candidateHitCount = 1;
        }

        if (candidateHitCount < requiredHits)
            return;

        heldMissCount = 0;
        detectedFrequency.store(result.frequencyHz, std::memory_order_release);
        detectedNote.store(result.midiNote, std::memory_order_release);
        centsDeviation.store(result.cents, std::memory_order_release);
        detectedConfidence.store(result.confidence, std::memory_order_release);
        pitchDetected.store(true, std::memory_order_release);
        updateDriftPhase(result.frequencyHz, result.midiNote, result.referenceA4Hz);
        updateGuitarStringChecklist(result.frequencyHz, result.referenceA4Hz);
        return;
    }

    candidateNote = -1;
    candidateHitCount = 0;

    if (pitchDetected.load(std::memory_order_acquire) && heldMissCount < holdWindows)
    {
        ++heldMissCount;
        return;
    }

    clearAnalysisResult();
}

void TunerProcessor::clearAnalysisResult() noexcept
{
    pitchDetected.store(false, std::memory_order_release);
    detectedFrequency.store(0.0f, std::memory_order_release);
    detectedNote.store(-1, std::memory_order_release);
    centsDeviation.store(0.0f, std::memory_order_release);
    detectedConfidence.store(0.0f, std::memory_order_release);
    currentGuitarStringIndex.store(-1, std::memory_order_release);
    currentGuitarStringCents.store(0.0f, std::memory_order_release);
}

void TunerProcessor::resetResponseSmoothing() noexcept
{
    candidateNote = -1;
    candidateHitCount = 0;
    heldMissCount = 0;
}

//==============================================================================
void TunerProcessor::resetGuitarStringChecklist() noexcept
{
    guitarStringInTuneMask.store(0, std::memory_order_release);
    currentGuitarStringIndex.store(-1, std::memory_order_release);
    currentGuitarStringCents.store(0.0f, std::memory_order_release);
}

void TunerProcessor::updateGuitarStringChecklist(float frequencyHz, float refA4Hz) noexcept
{
    const auto match = findClosestGuitarString(frequencyHz, refA4Hz);
    if (match.index < 0)
    {
        currentGuitarStringIndex.store(-1, std::memory_order_release);
        currentGuitarStringCents.store(0.0f, std::memory_order_release);
        return;
    }

    currentGuitarStringIndex.store(match.index, std::memory_order_release);
    currentGuitarStringCents.store(match.cents, std::memory_order_release);

    const int stringBit = 1 << match.index;
    auto mask = guitarStringInTuneMask.load(std::memory_order_acquire);
    if (std::abs(match.cents) <= kGuitarStringInTuneCents)
        mask |= stringBit;
    else
        mask &= ~stringBit;

    guitarStringInTuneMask.store(mask, std::memory_order_release);
}

//==============================================================================
void TunerProcessor::updateDriftPhase(float frequency, int midiNote, float refA4Hz) noexcept
{
    if (!std::isfinite(frequency) || !std::isfinite(refA4Hz) || frequency <= 0.0f || refA4Hz <= 0.0f)
        return;

    const float targetFreq = refA4Hz * std::pow(2.0f, static_cast<float>(midiNote - A4_MIDI) / 12.0f);
    if (!std::isfinite(targetFreq) || targetFreq <= 0.0f)
        return;

    // Phase rotates based on frequency error.
    const float freqError = frequency - targetFreq;
    const float phaseRate = freqError * 0.01f;

    float currentPhase = driftPhase.load();
    currentPhase += phaseRate;

    // Wrap phase to 0-1.
    while (currentPhase >= 1.0f)
        currentPhase -= 1.0f;
    while (currentPhase < 0.0f)
        currentPhase += 1.0f;

    driftPhase.store(currentPhase);
}

//==============================================================================
void TunerProcessor::getStateInformation(MemoryBlock& destData)
{
    MemoryOutputStream stream(destData, false);
    stream.writeInt(kTunerStateVersion);
    stream.writeFloat(getReferenceA4Hz());
    stream.writeInt(static_cast<int>(getResponseMode()));
}

void TunerProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    MemoryInputStream stream(data, static_cast<size_t>(sizeInBytes), false);
    const int version = stream.readInt();

    if (version >= 2)
    {
        setReferenceA4Hz(stream.readFloat());
        const auto storedMode = stream.readInt();
        setResponseMode(storedMode == static_cast<int>(ResponseMode::Fast) ? ResponseMode::Fast : ResponseMode::Stable);
    }
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
