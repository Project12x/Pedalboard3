#include "SettingsManager.h"

SettingsManager& SettingsManager::getInstance()
{
    static SettingsManager instance;
    return instance;
}

SettingsManager::SettingsManager() {}

void SettingsManager::initialise()
{
    load();
}

juce::File SettingsManager::getUserDataDirectory() const
{
    auto appData = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory);
    return appData.getChildFile("Pedalboard3");
}

void SettingsManager::load()
{
    std::lock_guard<std::mutex> lock(settingsMutex);

    auto file = juce::File(getSettingsFilePath().string());

    if (file.existsAsFile())
    {
        try
        {
            auto text = file.loadFileAsString();
            if (text.isNotEmpty())
            {
                settingsData = nlohmann::json::parse(text.toStdString());
            }
        }
        catch (const std::exception& e)
        {
            juce::Logger::writeToLog("Error loading settings.json: " + juce::String(e.what()));
        }
    }
}

void SettingsManager::save()
{
    std::lock_guard<std::mutex> lock(settingsMutex);

    auto file = juce::File(getSettingsFilePath().string());

    if (!file.exists())
    {
        file.getParentDirectory().createDirectory();
    }

    try
    {
        std::string jsonStr = settingsData.dump(4);
        file.replaceWithText(jsonStr);
        needsSaving = false;
    }
    catch (const std::exception& e)
    {
        juce::Logger::writeToLog("Error saving settings.json: " + juce::String(e.what()));
    }
}

std::filesystem::path SettingsManager::getSettingsFilePath() const
{
    return std::filesystem::path(getUserDataDirectory().getChildFile("settings.json").getFullPathName().toStdString());
}

// Getters
juce::String SettingsManager::getString(const std::string& key, const juce::String& defaultValue) const
{
    std::lock_guard<std::mutex> lock(settingsMutex);
    if (settingsData.contains(key) && settingsData[key].is_string())
    {
        return juce::String(settingsData[key].get<std::string>());
    }
    return defaultValue;
}

bool SettingsManager::getBool(const std::string& key, bool defaultValue) const
{
    std::lock_guard<std::mutex> lock(settingsMutex);
    if (settingsData.contains(key) && settingsData[key].is_boolean())
    {
        return settingsData[key].get<bool>();
    }
    return defaultValue;
}

int SettingsManager::getInt(const std::string& key, int defaultValue) const
{
    std::lock_guard<std::mutex> lock(settingsMutex);
    if (settingsData.contains(key) && settingsData[key].is_number_integer())
    {
        return settingsData[key].get<int>();
    }
    return defaultValue;
}

double SettingsManager::getDouble(const std::string& key, double defaultValue) const
{
    std::lock_guard<std::mutex> lock(settingsMutex);
    if (settingsData.contains(key) && settingsData[key].is_number())
    {
        return settingsData[key].get<double>();
    }
    return defaultValue;
}

std::unique_ptr<juce::XmlElement> SettingsManager::getXmlValue(const std::string& key) const
{
    std::lock_guard<std::mutex> lock(settingsMutex);
    if (settingsData.contains(key) && settingsData[key].is_string())
    {
        juce::String xmlString = juce::String(settingsData[key].get<std::string>());
        if (xmlString.isNotEmpty())
        {
            return juce::XmlDocument::parse(xmlString);
        }
    }
    return nullptr;
}

// Setters
void SettingsManager::setValue(const std::string& key, const juce::String& value)
{
    {
        std::lock_guard<std::mutex> lock(settingsMutex);
        settingsData[key] = value.toStdString();
        needsSaving = true;
    }
    save();
}

void SettingsManager::setValue(const std::string& key, bool value)
{
    {
        std::lock_guard<std::mutex> lock(settingsMutex);
        settingsData[key] = value;
        needsSaving = true;
    }
    save();
}

void SettingsManager::setValue(const std::string& key, int value)
{
    {
        std::lock_guard<std::mutex> lock(settingsMutex);
        settingsData[key] = value;
        needsSaving = true;
    }
    save();
}

void SettingsManager::setValue(const std::string& key, double value)
{
    {
        std::lock_guard<std::mutex> lock(settingsMutex);
        settingsData[key] = value;
        needsSaving = true;
    }
    save();
}

void SettingsManager::setValue(const std::string& key, const juce::XmlElement& xml)
{
    {
        std::lock_guard<std::mutex> lock(settingsMutex);
        settingsData[key] = xml.toString().toStdString();
        needsSaving = true;
    }
    save();
}

juce::StringArray SettingsManager::getStringArray(const std::string& key) const
{
    std::lock_guard<std::mutex> lock(settingsMutex);
    juce::StringArray result;

    if (settingsData.contains(key) && settingsData[key].is_array())
    {
        for (const auto& item : settingsData[key])
        {
            if (item.is_string())
            {
                result.add(juce::String(item.get<std::string>()));
            }
        }
    }
    return result;
}

void SettingsManager::setStringArray(const std::string& key, const juce::StringArray& value)
{
    {
        std::lock_guard<std::mutex> lock(settingsMutex);
        nlohmann::json arr = nlohmann::json::array();
        for (const auto& str : value)
        {
            arr.push_back(str.toStdString());
        }
        settingsData[key] = arr;
        needsSaving = true;
    }
    save();
}
