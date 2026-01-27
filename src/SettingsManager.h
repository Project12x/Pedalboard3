#pragma once

/**
 * @file SettingsManager.h
 * @brief Modern JSON-based settings persistence for Pedalboard 3.
 *
 * Replaces the legacy PropertiesSingleton/PropertiesFile system.
 * Settings are stored as human-readable JSON in %APPDATA%/Pedalboard3/settings.json.
 *
 * @author Pedalboard Team
 * @date 2026-01-24
 */

#include <JuceHeader.h>
#include <filesystem>
#include <mutex>
#include <nlohmann/json.hpp>

/**
 * @class SettingsManager
 * @brief Thread-safe singleton for application settings.
 *
 * Usage:
 * @code
 *   // Get a setting
 *   bool audioEnabled = SettingsManager::getInstance().getBool("AudioInput", true);
 *
 *   // Set a setting (auto-saves to disk)
 *   SettingsManager::getInstance().setValue("AudioInput", false);
 * @endcode
 */
class SettingsManager
{
  public:
    /**
     * @brief Get the singleton instance.
     * @return Reference to the SettingsManager singleton.
     */
    static SettingsManager& getInstance();

    /**
     * @brief Load settings from JSON file.
     * Called automatically on first access. Safe to call multiple times.
     */
    void load();

    /**
     * @brief Save settings to JSON file.
     * Called automatically after each setValue() call.
     */
    void save();

    // --- Typed Getters ---

    /** @brief Get a string value. */
    juce::String getString(const std::string& key, const juce::String& defaultValue = {}) const;

    /** @brief Get a boolean value. */
    bool getBool(const std::string& key, bool defaultValue = false) const;

    /** @brief Get an integer value. */
    int getInt(const std::string& key, int defaultValue = 0) const;

    /** @brief Get a double value. */
    double getDouble(const std::string& key, double defaultValue = 0.0) const;

    /** @brief Get a string array (stored as JSON array). */
    juce::StringArray getStringArray(const std::string& key) const;

    // --- Setters (auto-save after each call) ---

    /** @brief Set a string value. */
    void setValue(const std::string& key, const juce::String& value);

    /** @brief Set a boolean value. */
    void setValue(const std::string& key, bool value);

    /** @brief Set an integer value. */
    void setValue(const std::string& key, int value);

    /** @brief Set a double value. */
    void setValue(const std::string& key, double value);

    /** @brief Set an XML element (stored as string). */
    void setValue(const std::string& key, const juce::XmlElement& xml);

    /** @brief Set a string array (stored as JSON array). */
    void setStringArray(const std::string& key, const juce::StringArray& value);

    /**
     * @brief Initialize the settings system.
     * Creates the settings directory if needed and loads existing settings.
     */
    void initialise();

    /**
     * @brief Get the user data directory.
     * @return Path to %APPDATA%/Pedalboard3 (or equivalent).
     */
    juce::File getUserDataDirectory() const;

    /**
     * @brief Get an XML value from settings.
     * @param key The setting key.
     * @return Parsed XmlElement, or nullptr if not found.
     */
    std::unique_ptr<juce::XmlElement> getXmlValue(const std::string& key) const;

  private:
    SettingsManager();
    ~SettingsManager() = default;

    SettingsManager(const SettingsManager&) = delete;
    SettingsManager& operator=(const SettingsManager&) = delete;

    std::filesystem::path getSettingsFilePath() const;

    nlohmann::json settingsData;      ///< In-memory settings cache
    mutable std::mutex settingsMutex; ///< Thread safety for settings access
    bool needsSaving = false;         ///< Dirty flag for pending saves
};
