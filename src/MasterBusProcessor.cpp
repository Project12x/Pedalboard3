/*
  ==============================================================================

    MasterBusProcessor.cpp
    Master bus insert rack - wraps SubGraphProcessor for device-level
    audio processing.

  ==============================================================================
*/

#include "MasterBusProcessor.h"

#include "SubGraphProcessor.h"

#include <spdlog/spdlog.h>


MasterBusProcessor::MasterBusProcessor()
{
    rack = std::make_unique<SubGraphProcessor>();
    rack->setRackName("Master Insert");
    spdlog::info("[MasterBusProcessor] Created master insert rack");
}

MasterBusProcessor::~MasterBusProcessor()
{
    release();
    spdlog::debug("[MasterBusProcessor] Destroyed");
}

void MasterBusProcessor::prepare(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    currentBlockSize = samplesPerBlock;

    if (rack)
    {
        rack->prepareToPlay(sampleRate, samplesPerBlock);
        prepared.store(true, std::memory_order_release);
        spdlog::info("[MasterBusProcessor] Prepared: {} Hz, {} samples", sampleRate, samplesPerBlock);
    }
}

void MasterBusProcessor::release()
{
    prepared.store(false, std::memory_order_release);
    if (rack)
        rack->releaseResources();
}

void MasterBusProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    if (!prepared.load(std::memory_order_acquire) || !rack)
        return;

    if (bypassed.load(std::memory_order_relaxed))
        return;

    rack->processBlock(buffer, midi);
}

void MasterBusProcessor::saveState(juce::MemoryBlock& destData) const
{
    if (rack)
    {
        rack->getStateInformation(destData);
        spdlog::debug("[MasterBusProcessor] Saved state ({} bytes)", destData.getSize());
    }
}

void MasterBusProcessor::restoreState(const void* data, int sizeInBytes)
{
    if (rack && data && sizeInBytes > 0)
    {
        rack->setStateInformation(data, sizeInBytes);
        spdlog::info("[MasterBusProcessor] Restored state ({} bytes)", sizeInBytes);
    }
}

bool MasterBusProcessor::hasPlugins() const
{
    if (!rack)
        return false;

    // SubGraphProcessor always has 3 I/O nodes (audio in, audio out, midi in)
    // If there are more nodes, the user has added plugins
    return rack->getInternalGraph().getNumNodes() > 3;
}
