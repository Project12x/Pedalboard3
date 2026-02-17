/*
  ==============================================================================

    MasterGainState.cpp
    Global input/output gain state - persistence via SettingsManager.

  ==============================================================================
*/

#include "MasterGainState.h"

#include "MasterBusProcessor.h"
#include "SettingsManager.h"

// Explicit destructor here where MasterBusProcessor is a complete type
// (unique_ptr<MasterBusProcessor> in header needs this for proper deletion)
MasterGainState::~MasterGainState() = default;

void MasterGainState::prepareSmoothing(double sampleRate)
{
    // 50ms ramp eliminates zipper noise on fader moves
    smoothedInputGain.reset(sampleRate, 0.05);
    smoothedOutputGain.reset(sampleRate, 0.05);
    // Snap to current values immediately (no ramp on first block)
    smoothedInputGain.setCurrentAndTargetValue(getMasterInputGainLinear());
    smoothedOutputGain.setCurrentAndTargetValue(getMasterOutputGainLinear());
}

void MasterGainState::updateSmoothedTargets()
{
    smoothedInputGain.setTargetValue(getMasterInputGainLinear());
    smoothedOutputGain.setTargetValue(getMasterOutputGainLinear());
}

void MasterGainState::loadFromSettings()
{
    auto& settings = SettingsManager::getInstance();
    masterInputGainDb.store(static_cast<float>(settings.getDouble("MasterInputGainDb", 0.0)),
                            std::memory_order_relaxed);
    masterOutputGainDb.store(static_cast<float>(settings.getDouble("MasterOutputGainDb", 0.0)),
                             std::memory_order_relaxed);

    for (int ch = 0; ch < MaxChannels; ++ch)
    {
        String inKey = "InputChannelGainDb_" + String(ch);
        String outKey = "OutputChannelGainDb_" + String(ch);
        inputChannelGainDb[ch].store(static_cast<float>(settings.getDouble(inKey.toStdString(), 0.0)),
                                     std::memory_order_relaxed);
        outputChannelGainDb[ch].store(static_cast<float>(settings.getDouble(outKey.toStdString(), 0.0)),
                                      std::memory_order_relaxed);
    }

    // Restore master bus insert rack state
    auto rackState = settings.getString("MasterBusRackState");
    if (!rackState.isEmpty())
    {
        MemoryBlock block;
        block.fromBase64Encoding(rackState);
        if (block.getSize() > 0)
            getMasterBus().restoreState(block.getData(), static_cast<int>(block.getSize()));
    }
}

void MasterGainState::saveToSettings()
{
    auto& settings = SettingsManager::getInstance();
    settings.setValue("MasterInputGainDb", static_cast<double>(masterInputGainDb.load(std::memory_order_relaxed)));
    settings.setValue("MasterOutputGainDb", static_cast<double>(masterOutputGainDb.load(std::memory_order_relaxed)));

    for (int ch = 0; ch < MaxChannels; ++ch)
    {
        String inKey = "InputChannelGainDb_" + String(ch);
        String outKey = "OutputChannelGainDb_" + String(ch);
        float inVal = inputChannelGainDb[ch].load(std::memory_order_relaxed);
        float outVal = outputChannelGainDb[ch].load(std::memory_order_relaxed);

        // Only persist non-default values to keep settings clean
        if (std::abs(inVal) > 0.01f)
            settings.setValue(inKey.toStdString(), static_cast<double>(inVal));
        if (std::abs(outVal) > 0.01f)
            settings.setValue(outKey.toStdString(), static_cast<double>(outVal));
    }

    // Save master bus insert rack state
    if (masterBus)
    {
        MemoryBlock block;
        masterBus->saveState(block);
        if (block.getSize() > 0)
            settings.setValue("MasterBusRackState", block.toBase64Encoding());
    }
}

MasterBusProcessor& MasterGainState::getMasterBus()
{
    if (!masterBus)
        masterBus = std::make_unique<MasterBusProcessor>();
    return *masterBus;
}
