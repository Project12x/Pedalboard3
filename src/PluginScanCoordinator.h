#pragma once

#include "SafePluginScanner.h"

#include <JuceHeader.h>

#include <atomic>
#include <vector>

struct PluginScanProgress
{
    juce::String formatName;
    juce::String candidate;
    float formatProgress = 0.0f;
    float overallProgress = 0.0f;
};

struct PluginScanSummary
{
    int discovered = 0;
    int skipped = 0;
    int quarantined = 0;
    int failed = 0;
    bool cancelled = false;
    bool isolationUnavailable = false;
};

float calculateOverallPluginScanProgress(int jobIndex, float formatProgress, int jobCount);
PluginScanSummary makePluginScanSummary(const std::vector<PluginScanCandidateOutcome>& outcomes, bool cancelled);

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

class SafePluginScanner;
class PluginQuarantine;

/** Sequentially scans each registered external format on one worker thread. */
class PluginScanCoordinator : private juce::Thread,
                              private juce::Timer
{
  public:
    class Listener
    {
      public:
        virtual ~Listener() = default;
        virtual void pluginScanProgress(const PluginScanProgress&) {}
        virtual void pluginScanFinished(const PluginScanSummary&) {}
    };

    PluginScanCoordinator(juce::AudioPluginFormatManager&, juce::KnownPluginList&, const juce::File& dataDirectory,
                          bool diagnosticInProcessFallback = false);
    ~PluginScanCoordinator() override;

    void start();
    void cancel();
    bool isScanning() const { return scanning.load(std::memory_order_acquire); }
    PluginScanProgress getProgress() const;
    PluginScanSummary getSummary() const;
    void addListener(Listener* listener) { listeners.add(listener); }
    void removeListener(Listener* listener) { listeners.remove(listener); }

  private:
    void run() override;
    void timerCallback() override;
    void publishProgress(const PluginScanProgress&);
    void publishFinished(const PluginScanSummary&);

    juce::AudioPluginFormatManager& formatManager;
    juce::KnownPluginList& pluginList;
    juce::File dataDirectory;
    bool diagnosticInProcessFallback = false;
    std::unique_ptr<PluginQuarantine> quarantine;
    std::unique_ptr<SafePluginScanner> activeScanner;
    juce::ListenerList<Listener> listeners;
    mutable juce::CriticalSection stateLock;
    PluginScanProgress lastProgress;
    PluginScanSummary lastSummary;
    std::atomic<bool> scanning { false };
    std::atomic<bool> cancellationRequested { false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginScanCoordinator)
};
