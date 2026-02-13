/*
  ==============================================================================

    MasterGainState.cpp
    Global input/output gain state - persistence via SettingsManager.

  ==============================================================================
*/

#include "MasterGainState.h"
#include "SettingsManager.h"

void MasterGainState::loadFromSettings()
{
    auto& settings = SettingsManager::getInstance();
    masterInputGainDb.store(static_cast<float>(settings.getDouble("MasterInputGainDb", 0.0)), std::memory_order_relaxed);
    masterOutputGainDb.store(static_cast<float>(settings.getDouble("MasterOutputGainDb", 0.0)), std::memory_order_relaxed);

    for (int ch = 0; ch < MaxChannels; ++ch)
    {
        String inKey = "InputChannelGainDb_" + String(ch);
        String outKey = "OutputChannelGainDb_" + String(ch);
        inputChannelGainDb[ch].store(static_cast<float>(settings.getDouble(inKey.toStdString(), 0.0)), std::memory_order_relaxed);
        outputChannelGainDb[ch].store(static_cast<float>(settings.getDouble(outKey.toStdString(), 0.0)), std::memory_order_relaxed);
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
}
