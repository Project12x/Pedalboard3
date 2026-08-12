#pragma once

#include <JuceHeader.h>

#include <mutex>
#include <vector>

enum class PluginQuarantineReason
{
    crash,
    timeout,
    malformedResponse,
    loadFailure
};

struct PluginQuarantineEntry
{
    juce::String formatName;
    juce::String identifier;
    PluginQuarantineReason reason;
    juce::Time timestamp;
    juce::String detail;
};

/** Persisted, format-scoped automatic exclusion for plug-ins that cannot load safely. */
class PluginQuarantine
{
public:
    explicit PluginQuarantine(juce::File persistenceFile);

    bool contains(juce::StringRef formatName, juce::StringRef identifier) const;
    bool record(const PluginQuarantineEntry& entry);
    bool remove(juce::StringRef formatName, juce::StringRef identifier);
    bool clear();
    std::vector<PluginQuarantineEntry> entries() const;

private:
    void load();
    bool saveLocked() const;

    juce::File file;
    std::vector<PluginQuarantineEntry> storedEntries;
    mutable std::mutex mutex;
};
