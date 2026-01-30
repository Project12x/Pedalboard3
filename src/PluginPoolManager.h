//  PluginPoolManager.h - Manages a sliding window pool of preloaded plugins
//                        for instant patch switching.
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

#ifndef PLUGINPOOLMANAGER_H_
#define PLUGINPOOLMANAGER_H_

#include <JuceHeader.h>
#include <map>
#include <memory>
#include <set>
#include <vector>

class FilterGraph;

//------------------------------------------------------------------------------
/// A cached plugin instance with its state.
struct PooledPlugin
{
    std::unique_ptr<AudioPluginInstance> instance;
    PluginDescription description;
    bool isActive = false; // Currently in use by active patch
    int refCount = 0;      // How many patches in window need this
    Time lastUsed;
};

//------------------------------------------------------------------------------
/// Listener interface for pool loading progress notifications.
class PluginPoolListener
{
  public:
    virtual ~PluginPoolListener() = default;

    /// Called when a patch's plugins are being loaded.
    virtual void patchLoadingProgress(int patchIndex, float progress) = 0;

    /// Called when a patch is fully loaded and ready.
    virtual void patchReady(int patchIndex) = 0;
};

//------------------------------------------------------------------------------
/// Manages a sliding window pool of preloaded plugins for instant patch switching.
///
/// Instead of loading/unloading entire patches, this maintains a live pool of
/// plugins for the current patch plus N patches ahead/behind in the setlist.
/// This matches the Gig Performer architecture for zero-gap switching.
class PluginPoolManager : private Thread
{
  public:
    //--------------------------------------------------------------------------
    // Singleton access

    /// Gets the global PluginPoolManager instance.
    static PluginPoolManager& getInstance();

    /// Shuts down and destroys the singleton instance.
    static void killInstance();

    /// Destructor.
    ~PluginPoolManager() override;

    //--------------------------------------------------------------------------
    // Configuration

    /// Sets how many patches ahead to preload (1-5).
    void setPreloadRange(int patchesAhead);

    /// Gets the current preload range.
    int getPreloadRange() const { return preloadRange; }

    /// Sets the memory limit for the pool (optional, 0 = unlimited).
    void setMemoryLimit(size_t bytes);

    /// Gets estimated memory usage of the pool.
    size_t getPoolMemoryUsage() const;

    //--------------------------------------------------------------------------
    // Setlist Management

    /// Clears all cached patches and plugin pool.
    void clear();

    /// Adds a patch's XML definition to the pool's knowledge.
    /// Call this for each patch in the setlist.
    void addPatchDefinition(int patchIndex, std::unique_ptr<XmlElement> patchXml);

    /// Gets the number of known patch definitions.
    int getNumPatches() const { return static_cast<int>(patchDefinitions.size()); }

    //--------------------------------------------------------------------------
    // Position & Switching

    /// Sets the current setlist position and triggers background preloading.
    /// This slides the loading window to keep prev/next patches ready.
    void setCurrentPosition(int setlistIndex);

    /// Gets the current position.
    int getCurrentPosition() const { return currentPatchIndex.load(); }

    /// Checks if a patch is fully loaded and ready for instant switch.
    bool isPatchReady(int patchIndex) const;

    /// Gets loading progress for a patch (0.0 to 1.0).
    float getPatchLoadProgress(int patchIndex) const;

    //--------------------------------------------------------------------------
    // Plugin Access

    /// Gets or creates a plugin instance from the pool.
    /// Returns nullptr if plugin couldn't be created.
    AudioPluginInstance* getOrCreatePlugin(const PluginDescription& desc);

    /// Gets a plugin by its identifier string (from pool).
    AudioPluginInstance* getPluginByIdentifier(const String& identifier);

    //--------------------------------------------------------------------------
    // Listeners

    void addListener(PluginPoolListener* listener);
    void removeListener(PluginPoolListener* listener);

  private:
    /// Private constructor for singleton pattern.
    PluginPoolManager();

    /// Static singleton instance pointer.
    static std::unique_ptr<PluginPoolManager> instance;

    //--------------------------------------------------------------------------
    // Thread implementation (background loading)

    void run() override;

    /// Queue a patch for background loading.
    void queuePatchLoad(int patchIndex);

    /// Load a single patch's plugins (called from background thread).
    void loadPatchPlugins(int patchIndex);

    /// Parse plugin descriptions from patch XML.
    std::vector<PluginDescription> extractPluginsFromPatch(const XmlElement* patchXml);

    /// Release plugins that are outside the current window.
    void releaseUnusedPlugins();

    /// Creates identifier string for a plugin description.
    static String createPluginIdentifier(const PluginDescription& desc);

    //--------------------------------------------------------------------------
    // Data

    /// Plugin pool - key is plugin identifier, value is pooled instance.
    std::map<String, std::unique_ptr<PooledPlugin>> pluginPool;

    /// Patch definitions (XML) - key is patch index.
    std::map<int, std::unique_ptr<XmlElement>> patchDefinitions;

    /// Which plugins each patch needs - key is patch index.
    std::map<int, std::vector<String>> patchPluginRequirements;

    /// Set of patches that are fully loaded.
    std::set<int> loadedPatches;

    /// Loading progress per patch (0.0 to 1.0).
    std::map<int, float> patchLoadProgress;

    /// Current patch index (atomic for thread safety).
    std::atomic<int> currentPatchIndex{0};

    /// Preload range (patches ahead to load).
    int preloadRange = 2;

    /// Memory limit (0 = unlimited).
    size_t memoryLimit = 0;

    /// Queue of patches to load.
    std::vector<int> loadQueue;

    /// Mutex for thread-safe access.
    mutable CriticalSection poolLock;

    /// Listeners for progress notifications.
    ListenerList<PluginPoolListener> listeners;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginPoolManager)
};

#endif // PLUGINPOOLMANAGER_H_
