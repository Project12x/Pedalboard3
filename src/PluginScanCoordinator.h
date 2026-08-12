#pragma once

#include <JuceHeader.h>

#include <vector>

struct PluginScanJob
{
    juce::String formatName;
    juce::AudioPluginFormat* format = nullptr;
    juce::FileSearchPath locations;
    bool allowAsyncInstantiation = false;
    juce::File deadMansPedalFile;
};

/**
 * Builds a deterministic scan queue from the formats registered in manager.
 * Registered formats retain their order; the Internal format is never scanned.
 */
std::vector<PluginScanJob> makePluginScanPlan(juce::AudioPluginFormatManager& manager,
                                              const juce::File& dataDirectory);
