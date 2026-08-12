#include "PluginScanCoordinator.h"

namespace
{
juce::String deadMansPedalSuffix(const juce::String& formatName)
{
    auto suffix = formatName.toLowerCase().retainCharacters("abcdefghijklmnopqrstuvwxyz0123456789");
    return suffix.isEmpty() ? "plugin" : suffix;
}
} // namespace

std::vector<PluginScanJob> makePluginScanPlan(juce::AudioPluginFormatManager& manager,
                                              const juce::File& dataDirectory)
{
    std::vector<PluginScanJob> jobs;

    for (int index = 0; index < manager.getNumFormats(); ++index)
    {
        auto* format = manager.getFormat(index);
        const auto formatName = format->getName();

        if (formatName == "Internal")
            continue;

        jobs.push_back({formatName,
                        format,
                        format->getDefaultLocationsToSearch(),
                        formatName == "AudioUnit",
                        dataDirectory.getChildFile("RecentlyCrashedPluginsList-" + deadMansPedalSuffix(formatName))});
    }

    return jobs;
}
