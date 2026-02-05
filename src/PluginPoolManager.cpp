//  PluginPoolManager.cpp - Implementation of sliding window plugin pool.
//  ----------------------------------------------------------------------------
//  This file is part of Pedalboard3, an audio plugin host.
//  Copyright (c) 2024-2026 Antigravity.
//
//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.
//  ----------------------------------------------------------------------------

#include "PluginPoolManager.h"

#include "AudioSingletons.h"
#include "BypassableInstance.h"

#include <spdlog/spdlog.h>

namespace
{
bool shouldPoolPlugin(const PluginDescription& desc)
{
    // Skip internal plugins (Audio I/O, etc.) and AudioUnits for now.
    return desc.pluginFormatName != "Internal" && desc.pluginFormatName != "AudioUnit";
}

bool isSubGraphPlugin(const PluginDescription& desc)
{
    if (desc.pluginFormatName != "Internal")
        return false;

    // Internal rack nodes are identified by fileOrIdentifier.
    return desc.fileOrIdentifier == "Internal:SubGraph";
}

void extractPluginsFromFilter(const XmlElement& filterElem, std::vector<PluginDescription>& result);
std::vector<PluginDescription> extractPluginsFromPatchImpl(const XmlElement* patchXml);

void extractPluginsFromRackState(const XmlElement& filterElem, std::vector<PluginDescription>& result)
{
    auto* stateElem = filterElem.getChildByName("STATE");
    if (stateElem == nullptr)
        return;

    juce::MemoryBlock state;
    if (!state.fromBase64Encoding(stateElem->getAllSubText()))
        return;

    auto rackXml = juce::AudioProcessor::getXmlFromBinary(state.getData(), (int)state.getSize());
    if (!rackXml || !rackXml->hasTagName("RACK"))
        return;

    for (auto* rackFilter : rackXml->getChildWithTagNameIterator("FILTER"))
        extractPluginsFromFilter(*rackFilter, result);
}

void extractPluginsFromFilter(const XmlElement& filterElem, std::vector<PluginDescription>& result)
{
    auto* descElem = filterElem.getChildByName("PLUGIN");
    if (descElem == nullptr)
        return;

    PluginDescription desc;
    if (!desc.loadFromXml(*descElem))
        return;

    if (isSubGraphPlugin(desc))
    {
        extractPluginsFromRackState(filterElem, result);
        return;
    }

    if (shouldPoolPlugin(desc))
        result.push_back(desc);
}

std::vector<PluginDescription> extractPluginsFromPatchImpl(const XmlElement* patchXml)
{
    std::vector<PluginDescription> result;

    if (!patchXml)
        return result;

    const XmlElement* graphXml = patchXml;
    if (patchXml->hasTagName("Patch"))
        graphXml = patchXml->getChildByName("FILTERGRAPH");

    if (graphXml == nullptr)
        return result;

    // Look for FILTER elements (this is Pedalboard's XML format)
    for (auto* filterElem : graphXml->getChildWithTagNameIterator("FILTER"))
        extractPluginsFromFilter(*filterElem, result);

    return result;
}
} // namespace

//------------------------------------------------------------------------------
// Singleton instance
std::unique_ptr<PluginPoolManager> PluginPoolManager::instance = nullptr;

//------------------------------------------------------------------------------
PluginPoolManager& PluginPoolManager::getInstance()
{
    if (!instance)
    {
        instance = std::unique_ptr<PluginPoolManager>(new PluginPoolManager());
    }
    return *instance;
}

//------------------------------------------------------------------------------
void PluginPoolManager::killInstance()
{
    if (instance)
    {
        instance.reset();
        spdlog::info("[PluginPoolManager] Singleton instance destroyed");
    }
}

//------------------------------------------------------------------------------
PluginPoolManager::PluginPoolManager() : Thread("PluginPoolLoader")
{
    spdlog::info("[PluginPoolManager] Initialized with preloadRange={}", preloadRange);
}

//------------------------------------------------------------------------------
PluginPoolManager::~PluginPoolManager()
{
    // Stop background thread
    signalThreadShouldExit();
    notify();
    stopThread(5000);

    // Clear pool
    clear();

    spdlog::info("[PluginPoolManager] Destroyed");
}

//------------------------------------------------------------------------------
void PluginPoolManager::setPreloadRange(int patchesAhead)
{
    preloadRange = juce::jlimit(1, 5, patchesAhead);
    spdlog::info("[PluginPoolManager] Preload range set to {}", preloadRange);
}

//------------------------------------------------------------------------------
void PluginPoolManager::setMemoryLimit(size_t bytes)
{
    memoryLimit = bytes;
}

//------------------------------------------------------------------------------
size_t PluginPoolManager::getPoolMemoryUsage() const
{
    ScopedLock lock(poolLock);

    // Rough estimate: count number of plugins * average plugin size
    // Real implementation would query each plugin's memory footprint
    size_t estimate = 0;
    for (const auto& [key, pooled] : pluginPool)
    {
        if (pooled && pooled->instance)
        {
            // Estimate ~20MB per plugin instance (very rough)
            estimate += 20 * 1024 * 1024;
        }
    }
    return estimate;
}

//------------------------------------------------------------------------------
void PluginPoolManager::clear()
{
    ScopedLock lock(poolLock);

    // Stop any pending loads
    loadQueue.clear();

    // Release all plugins
    pluginPool.clear();
    patchDefinitions.clear();
    patchPluginRequirements.clear();
    loadedPatches.clear();
    patchLoadProgress.clear();

    currentPatchIndex.store(0);

    spdlog::info("[PluginPoolManager] Pool cleared");
}

//------------------------------------------------------------------------------
void PluginPoolManager::addPatchDefinition(int patchIndex, std::unique_ptr<XmlElement> patchXml)
{
    if (!patchXml)
        return;

    ScopedLock lock(poolLock);

    // Store the patch definition
    patchDefinitions[patchIndex] = std::move(patchXml);

    // Extract plugin requirements
    auto plugins = extractPluginsFromPatch(patchDefinitions[patchIndex].get());
    std::vector<String> identifiers;
    for (const auto& desc : plugins)
    {
        identifiers.push_back(createPluginIdentifier(desc));
    }
    patchPluginRequirements[patchIndex] = std::move(identifiers);

    spdlog::debug("[PluginPoolManager] Added patch {} with {} plugins", patchIndex,
                  patchPluginRequirements[patchIndex].size());
}

//------------------------------------------------------------------------------
void PluginPoolManager::setCurrentPosition(int setlistIndex)
{
    int oldPosition = currentPatchIndex.exchange(setlistIndex);

    if (oldPosition == setlistIndex)
        return;

    spdlog::info("[PluginPoolManager] Position changed {} -> {}", oldPosition, setlistIndex);

    {
        ScopedLock lock(poolLock);

        // Clear load queue and reprioritize
        loadQueue.clear();

        // Queue patches in priority order:
        // 1. Current patch (if not loaded)
        // 2. Next patches (in order)
        // 3. Previous patch (for going back)

        if (!isPatchReady(setlistIndex))
            loadQueue.push_back(setlistIndex);

        for (int i = 1; i <= preloadRange; ++i)
        {
            int nextIndex = setlistIndex + i;
            if (patchDefinitions.count(nextIndex) > 0 && !isPatchReady(nextIndex))
                loadQueue.push_back(nextIndex);
        }

        // Previous patch (lower priority)
        int prevIndex = setlistIndex - 1;
        if (prevIndex >= 0 && patchDefinitions.count(prevIndex) > 0 && !isPatchReady(prevIndex))
            loadQueue.push_back(prevIndex);
    }

    // Wake up background thread
    if (!isThreadRunning())
        startThread();
    else
        notify();

    // Release plugins outside new window
    releaseUnusedPlugins();
}

//------------------------------------------------------------------------------
bool PluginPoolManager::isPatchReady(int patchIndex) const
{
    ScopedLock lock(poolLock);
    return loadedPatches.count(patchIndex) > 0;
}

//------------------------------------------------------------------------------
float PluginPoolManager::getPatchLoadProgress(int patchIndex) const
{
    ScopedLock lock(poolLock);

    if (loadedPatches.count(patchIndex) > 0)
        return 1.0f;

    auto it = patchLoadProgress.find(patchIndex);
    if (it != patchLoadProgress.end())
        return it->second;

    return 0.0f;
}

//------------------------------------------------------------------------------
AudioPluginInstance* PluginPoolManager::getOrCreatePlugin(const PluginDescription& desc)
{
    String identifier = createPluginIdentifier(desc);

    {
        ScopedLock lock(poolLock);

        auto it = pluginPool.find(identifier);
        if (it != pluginPool.end() && it->second && it->second->instance)
        {
            spdlog::debug("[PluginPoolManager] Returning cached plugin: {}", desc.name.toStdString());
            return it->second->instance.get();
        }
    }

    // Not in pool - create new instance
    spdlog::info("[PluginPoolManager] Creating new plugin: {}", desc.name.toStdString());

    String errorMessage;
    auto newInstance =
        AudioPluginFormatManagerSingleton::getInstance().createPluginInstance(desc, 44100.0, 512, errorMessage);

    if (!newInstance)
    {
        spdlog::error("[PluginPoolManager] Failed to create plugin {}: {}", desc.name.toStdString(),
                      errorMessage.toStdString());
        return nullptr;
    }

    // Configure stereo layout
    AudioProcessor::BusesLayout stereoLayout;
    stereoLayout.inputBuses.add(AudioChannelSet::stereo());
    stereoLayout.outputBuses.add(AudioChannelSet::stereo());
    if (newInstance->checkBusesLayoutSupported(stereoLayout))
        newInstance->setBusesLayout(stereoLayout);

    // Store in pool
    {
        ScopedLock lock(poolLock);

        auto pooled = std::make_unique<PooledPlugin>();
        pooled->instance = std::move(newInstance);
        pooled->description = desc;
        pooled->lastUsed = Time::getCurrentTime();

        AudioPluginInstance* result = pooled->instance.get();
        pluginPool[identifier] = std::move(pooled);
        return result;
    }
}

//------------------------------------------------------------------------------
AudioPluginInstance* PluginPoolManager::getPluginByIdentifier(const String& identifier)
{
    ScopedLock lock(poolLock);

    auto it = pluginPool.find(identifier);
    if (it != pluginPool.end() && it->second && it->second->instance)
        return it->second->instance.get();

    return nullptr;
}

//------------------------------------------------------------------------------
void PluginPoolManager::addListener(PluginPoolListener* listener)
{
    listeners.add(listener);
}

//------------------------------------------------------------------------------
void PluginPoolManager::removeListener(PluginPoolListener* listener)
{
    listeners.remove(listener);
}

//------------------------------------------------------------------------------
void PluginPoolManager::run()
{
    spdlog::info("[PluginPoolManager] Background loader thread started");

    while (!threadShouldExit())
    {
        int patchToLoad = -1;

        {
            ScopedLock lock(poolLock);
            if (!loadQueue.empty())
            {
                patchToLoad = loadQueue.front();
                loadQueue.erase(loadQueue.begin());
            }
        }

        if (patchToLoad >= 0)
        {
            loadPatchPlugins(patchToLoad);
        }
        else
        {
            // Wait for more work
            wait(-1);
        }
    }

    spdlog::info("[PluginPoolManager] Background loader thread stopped");
}

//------------------------------------------------------------------------------
void PluginPoolManager::queuePatchLoad(int patchIndex)
{
    ScopedLock lock(poolLock);

    // Add to queue if not already there and not already loaded
    if (loadedPatches.count(patchIndex) == 0)
    {
        bool found = std::find(loadQueue.begin(), loadQueue.end(), patchIndex) != loadQueue.end();
        if (!found)
        {
            loadQueue.push_back(patchIndex);
            notify();
        }
    }
}

//------------------------------------------------------------------------------
void PluginPoolManager::loadPatchPlugins(int patchIndex)
{
    spdlog::info("[PluginPoolManager] Loading patch {}", patchIndex);

    std::vector<PluginDescription> plugins;

    {
        ScopedLock lock(poolLock);

        auto it = patchDefinitions.find(patchIndex);
        if (it == patchDefinitions.end())
        {
            spdlog::warn("[PluginPoolManager] Patch {} not found in definitions", patchIndex);
            return;
        }

        plugins = extractPluginsFromPatch(it->second.get());
        patchLoadProgress[patchIndex] = 0.0f;
    }

    if (plugins.empty())
    {
        ScopedLock lock(poolLock);
        loadedPatches.insert(patchIndex);
        patchLoadProgress[patchIndex] = 1.0f;
        MessageManager::callAsync([this, patchIndex]()
                                  { listeners.call(&PluginPoolListener::patchReady, patchIndex); });
        return;
    }

    // Load each plugin
    int loaded = 0;
    for (const auto& desc : plugins)
    {
        if (threadShouldExit())
            return;

        // Check if need to abort (position changed significantly)
        int currentPos = currentPatchIndex.load();
        if (std::abs(patchIndex - currentPos) > preloadRange + 1)
        {
            spdlog::info("[PluginPoolManager] Aborting load of patch {} (too far from current {})", patchIndex,
                         currentPos);
            return;
        }

        // Load the plugin
        getOrCreatePlugin(desc);
        loaded++;

        // Update progress
        float progress = static_cast<float>(loaded) / static_cast<float>(plugins.size());
        {
            ScopedLock lock(poolLock);
            patchLoadProgress[patchIndex] = progress;
        }

        // Notify listeners on message thread
        MessageManager::callAsync([this, patchIndex, progress]()
                                  { listeners.call(&PluginPoolListener::patchLoadingProgress, patchIndex, progress); });
    }

    // Mark as loaded
    {
        ScopedLock lock(poolLock);
        loadedPatches.insert(patchIndex);
        patchLoadProgress[patchIndex] = 1.0f;
    }

    spdlog::info("[PluginPoolManager] Patch {} fully loaded ({} plugins)", patchIndex, plugins.size());

    // Notify listeners
    MessageManager::callAsync([this, patchIndex]() { listeners.call(&PluginPoolListener::patchReady, patchIndex); });
}

//------------------------------------------------------------------------------
std::vector<PluginDescription> PluginPoolManager::extractPluginsFromPatch(const XmlElement* patchXml)
{
    return extractPluginsFromPatchImpl(patchXml);
}

#if defined(PEDALBOARD3_TESTS)
std::vector<PluginDescription> PluginPoolManager::extractPluginsFromPatchForTest(const XmlElement* patchXml)
{
    return extractPluginsFromPatchImpl(patchXml);
}
#endif

//------------------------------------------------------------------------------
void PluginPoolManager::releaseUnusedPlugins()
{
    ScopedLock lock(poolLock);

    int currentPos = currentPatchIndex.load();

    // Build set of plugins needed by patches in current window
    std::set<String> neededPlugins;

    for (int i = currentPos - 1; i <= currentPos + preloadRange; ++i)
    {
        if (i >= 0 && patchPluginRequirements.count(i) > 0)
        {
            for (const auto& id : patchPluginRequirements[i])
                neededPlugins.insert(id);
        }
    }

    // Find and release plugins outside window
    std::vector<String> toRemove;
    for (const auto& [key, pooled] : pluginPool)
    {
        if (neededPlugins.count(key) == 0)
            toRemove.push_back(key);
    }

    for (const auto& key : toRemove)
    {
        spdlog::debug("[PluginPoolManager] Releasing unused plugin: {}", key.toStdString());
        pluginPool.erase(key);
    }

    // Also remove loaded status for patches outside window
    std::vector<int> patchesToUnload;
    for (int patch : loadedPatches)
    {
        if (patch < currentPos - 1 || patch > currentPos + preloadRange)
            patchesToUnload.push_back(patch);
    }
    for (int patch : patchesToUnload)
    {
        loadedPatches.erase(patch);
        patchLoadProgress.erase(patch);
    }

    if (!toRemove.empty())
    {
        spdlog::info("[PluginPoolManager] Released {} unused plugins", toRemove.size());
    }
}

//------------------------------------------------------------------------------
String PluginPoolManager::createPluginIdentifier(const PluginDescription& desc)
{
    // Create unique identifier: format + name + uid
    return desc.pluginFormatName + "|" + desc.name + "|" + String(desc.uniqueId);
}
