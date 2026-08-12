#include "PluginScanCoordinator.h"

namespace
{
juce::String deadMansPedalSuffix(const juce::String& formatName)
{
    auto suffix = formatName.toLowerCase().retainCharacters("abcdefghijklmnopqrstuvwxyz0123456789");
    return suffix.isEmpty() ? "plugin" : suffix;
}
}

float calculateOverallPluginScanProgress(int jobIndex, float formatProgress, int jobCount)
{
    if (jobCount <= 0)
        return 1.0f;
    return juce::jlimit(0.0f, 1.0f, (static_cast<float>(jobIndex) + juce::jlimit(0.0f, 1.0f, formatProgress))
                                             / static_cast<float>(jobCount));
}

PluginScanSummary makePluginScanSummary(const std::vector<PluginScanCandidateOutcome>& outcomes, bool cancelled)
{
    PluginScanSummary summary;
    summary.cancelled = cancelled;
    for (const auto outcome : outcomes)
    {
        switch (outcome)
        {
            case PluginScanCandidateOutcome::discovered: ++summary.discovered; break;
            case PluginScanCandidateOutcome::skipped: ++summary.skipped; break;
            case PluginScanCandidateOutcome::quarantined: ++summary.quarantined; break;
            case PluginScanCandidateOutcome::failed: ++summary.failed; break;
        }
    }
    return summary;
}

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
        jobs.push_back({ formatName, format, format->getDefaultLocationsToSearch(), formatName == "AudioUnit",
                         dataDirectory.getChildFile("RecentlyCrashedPluginsList-" + deadMansPedalSuffix(formatName)) });
    }
    return jobs;
}
