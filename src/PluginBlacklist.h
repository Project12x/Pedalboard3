/*
  ==============================================================================

    PluginBlacklist.h

    User-configurable plugin blacklist for crash protection.
    Persists blacklisted plugin paths via SettingsManager.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <mutex>
#include <set>


/**
 * @class PluginBlacklist
 * @brief Manages a user-configurable list of blacklisted plugins.
 *
 * Plugins on the blacklist are skipped during scanning and not loaded.
 * The blacklist persists across sessions via SettingsManager.
 */
class PluginBlacklist
{
  public:
    /// Get the singleton instance
    static PluginBlacklist& getInstance();

    /// Check if a plugin path is blacklisted
    bool isBlacklisted(const juce::String& pluginPath) const;

    /// Check if a plugin identifier is blacklisted
    bool isBlacklistedById(const juce::String& pluginId) const;

    /// Add a plugin to the blacklist by path
    void addToBlacklist(const juce::String& pluginPath);

    /// Add a plugin to the blacklist by identifier
    void addToBlacklistById(const juce::String& pluginId);

    /// Remove a plugin from the blacklist by path
    void removeFromBlacklist(const juce::String& pluginPath);

    /// Remove a plugin from the blacklist by identifier
    void removeFromBlacklistById(const juce::String& pluginId);

    /// Get all blacklisted paths
    juce::StringArray getBlacklistedPaths() const;

    /// Get all blacklisted plugin identifiers
    juce::StringArray getBlacklistedIds() const;

    /// Clear the entire blacklist
    void clearBlacklist();

    /// Get count of blacklisted plugins
    int getBlacklistCount() const;

    /// Load blacklist from settings
    void loadFromSettings();

    /// Save blacklist to settings
    void saveToSettings();

  private:
    PluginBlacklist();
    ~PluginBlacklist() = default;

    PluginBlacklist(const PluginBlacklist&) = delete;
    PluginBlacklist& operator=(const PluginBlacklist&) = delete;

    /// Normalize path for case-insensitive comparison on Windows
    juce::String normalizePath(const juce::String& path) const;

    std::set<juce::String> blacklistedPaths; ///< Paths (normalized)
    std::set<juce::String> blacklistedIds;   ///< Plugin identifiers
    mutable std::mutex blacklistMutex;       ///< Thread safety
};
