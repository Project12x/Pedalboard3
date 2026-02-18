/*
  ==============================================================================

    Tone3000DownloadManager.h
    Async download manager for TONE3000 models

  ==============================================================================
*/

#pragma once

#include "Tone3000Types.h"
#include <JuceHeader.h>

#include <deque>
#include <mutex>
#include <atomic>
#include <optional>

//==============================================================================
/**
    Manages async downloads of NAM models from TONE3000.
    Runs downloads on a background thread with progress callbacks.
*/
class Tone3000DownloadManager : public juce::Thread
{
public:
    //==========================================================================
    // Listener Interface

    class Listener
    {
    public:
        virtual ~Listener() = default;

        /// Called when a download is queued
        virtual void downloadQueued(const juce::String& toneId) {}

        /// Called when download starts
        virtual void downloadStarted(const juce::String& toneId) {}

        /// Called periodically with download progress
        virtual void downloadProgress(const juce::String& toneId, float progress,
                                       int64_t bytesDownloaded, int64_t totalBytes) {}

        /// Called when download completes successfully
        virtual void downloadCompleted(const juce::String& toneId, const juce::File& file) {}

        /// Called when download fails
        virtual void downloadFailed(const juce::String& toneId, const juce::String& errorMessage) {}

        /// Called when download is cancelled
        virtual void downloadCancelled(const juce::String& toneId) {}
    };

    //==========================================================================
    // Singleton Access

    static Tone3000DownloadManager& getInstance();

    //==========================================================================
    // Download Management

    /// Queue a download for a tone
    void queueDownload(const Tone3000::ToneInfo& tone);

    /// Queue a download with explicit URL
    void queueDownload(const juce::String& toneId, const juce::String& toneName,
                        const juce::String& url, int64_t expectedSize = 0,
                        const juce::String& platform = "nam");

    /// Cancel a specific download
    void cancelDownload(const juce::String& toneId);

    /// Cancel all downloads
    void cancelAll();

    /// Get current download queue
    std::vector<Tone3000::DownloadTask> getQueue() const;

    /// Get download task by ID (returns empty if not found)
    std::optional<Tone3000::DownloadTask> getTask(const juce::String& toneId) const;

    /// Check if a tone is currently downloading or queued
    bool isDownloading(const juce::String& toneId) const;

    /// Check if any downloads are active
    bool hasActiveDownloads() const;

    //==========================================================================
    // Cache Management

    /// Get the download/cache directory
    juce::File getCacheDirectory() const;

    /// Set custom cache directory
    void setCacheDirectory(const juce::File& directory);

    /// Check if a tone is already cached
    bool isCached(const juce::String& toneId) const;

    /// Get cached file path for a tone (empty if not cached)
    juce::File getCachedFile(const juce::String& toneId) const;

    /// Clear all cached downloads
    void clearCache();

    /// Get total cache size in bytes
    int64_t getCacheSize() const;

    //==========================================================================
    // Listeners

    void addListener(Listener* listener);
    void removeListener(Listener* listener);

private:
    Tone3000DownloadManager();
    ~Tone3000DownloadManager() override;

    void run() override;

    /// Process a single download task
    bool processDownload(Tone3000::DownloadTask& task);

    /// Notify listeners on message thread
    void notifyQueued(const juce::String& toneId);
    void notifyStarted(const juce::String& toneId);
    void notifyProgress(const juce::String& toneId, float progress, int64_t bytes, int64_t total);
    void notifyCompleted(const juce::String& toneId, const juce::File& file);
    void notifyFailed(const juce::String& toneId, const juce::String& error);
    void notifyCancelled(const juce::String& toneId);

    /// Get target file path for a tone
    juce::File getTargetPath(const juce::String& toneId, const juce::String& toneName,
                              const juce::String& platform) const;

    //==========================================================================
    // Members

    std::deque<Tone3000::DownloadTask> downloadQueue;
    mutable std::mutex queueMutex;

    std::atomic<bool> shouldStop{false};
    std::atomic<bool> cancelCurrentDownload{false};
    juce::String currentlyDownloading;

    juce::File cacheDirectory;
    juce::ListenerList<Listener> listeners;

    // Configuration
    static constexpr int DOWNLOAD_BUFFER_SIZE = 8192;
    static constexpr int PROGRESS_UPDATE_INTERVAL_MS = 100;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Tone3000DownloadManager)
};
