#include "PluginScanCoordinator.h"

#include "PluginQuarantine.h"
#include "SafePluginScanner.h"

namespace
{
juce::String deadMansPedalSuffix(const juce::String& formatName)
{
    auto suffix = formatName.toLowerCase().retainCharacters("abcdefghijklmnopqrstuvwxyz0123456789");
    return suffix.isEmpty() ? "plugin" : suffix;
}
} // namespace

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

        jobs.push_back({formatName,
                        format,
                        format->getDefaultLocationsToSearch(),
                        formatName == "AudioUnit",
                        dataDirectory.getChildFile("RecentlyCrashedPluginsList-" + deadMansPedalSuffix(formatName))});
    }

    return jobs;
}

PluginScanCoordinator::PluginScanCoordinator(juce::AudioPluginFormatManager& manager, juce::KnownPluginList& list,
                                             const juce::File& directory, bool diagnosticFallback)
    : juce::Thread("Plugin Scan Coordinator"), formatManager(manager), pluginList(list), dataDirectory(directory),
      diagnosticInProcessFallback(diagnosticFallback),
      quarantine(std::make_unique<PluginQuarantine>(directory.getChildFile("PluginQuarantine.xml")))
{
}

PluginScanCoordinator::~PluginScanCoordinator()
{
    cancel();
    waitForThreadToExit(-1);
    stopTimer();
}

void PluginScanCoordinator::start()
{
    if (scanning.exchange(true, std::memory_order_acq_rel))
        return;

    cancellationRequested.store(false, std::memory_order_release);
    {
        const juce::ScopedLock lock(stateLock);
        lastProgress = {};
        lastSummary = {};
    }
    startTimer(50);
    startThread();
}

void PluginScanCoordinator::cancel()
{
    cancellationRequested.store(true, std::memory_order_release);
    signalThreadShouldExit();
}

PluginScanProgress PluginScanCoordinator::getProgress() const
{
    const juce::ScopedLock lock(stateLock);
    return lastProgress;
}

PluginScanSummary PluginScanCoordinator::getSummary() const
{
    const juce::ScopedLock lock(stateLock);
    return lastSummary;
}

void PluginScanCoordinator::publishProgress(const PluginScanProgress& progress)
{
    const juce::ScopedLock lock(stateLock);
    lastProgress = progress;
}

void PluginScanCoordinator::publishFinished(const PluginScanSummary& summary)
{
    const juce::ScopedLock lock(stateLock);
    lastSummary = summary;
}

void PluginScanCoordinator::run()
{
    std::vector<PluginScanCandidateOutcome> outcomes;
    const auto jobs = makePluginScanPlan(formatManager, dataDirectory);
    bool isolationUnavailable = false;

    for (size_t jobIndex = 0; jobIndex < jobs.size() && ! threadShouldExit(); ++jobIndex)
    {
        const auto& job = jobs[jobIndex];
        activeScanner = std::make_unique<SafePluginScanner>(pluginList, *job.format, job.locations, true,
                                                             job.deadMansPedalFile, job.allowAsyncInstantiation,
                                                             true, diagnosticInProcessFallback, quarantine.get());

        if (activeScanner->isIsolationUnavailable())
        {
            isolationUnavailable = true;
            break;
        }

        while (! threadShouldExit())
        {
            juce::String candidate;
            const auto hasMore = activeScanner->scanNextFile(true, candidate);
            if (candidate.isNotEmpty())
                outcomes.push_back(activeScanner->getLastOutcome());

            publishProgress({ job.formatName, candidate, activeScanner->getProgress(),
                              calculateOverallPluginScanProgress(static_cast<int>(jobIndex), activeScanner->getProgress(),
                                                                 static_cast<int>(jobs.size())) });
            if (! hasMore)
                break;
        }
        activeScanner.reset();
    }

    auto summary = makePluginScanSummary(outcomes, cancellationRequested.load(std::memory_order_acquire));
    summary.isolationUnavailable = isolationUnavailable;
    publishFinished(summary);
    scanning.store(false, std::memory_order_release);
}

void PluginScanCoordinator::timerCallback()
{
    const auto progress = getProgress();
    listeners.call([&] (Listener& listener) { listener.pluginScanProgress(progress); });
    if (! isScanning() && ! isThreadRunning())
    {
        const auto summary = getSummary();
        listeners.call([&] (Listener& listener) { listener.pluginScanFinished(summary); });
        stopTimer();
    }
}
