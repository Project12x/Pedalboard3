/*
  ==============================================================================

    Tone3000DownloadManager.cpp
    Async download manager for TONE3000 models

  ==============================================================================
*/

#include "Tone3000DownloadManager.h"
#include "Tone3000Client.h"
#include "SettingsManager.h"

#include <spdlog/spdlog.h>

//==============================================================================
Tone3000DownloadManager& Tone3000DownloadManager::getInstance()
{
    static Tone3000DownloadManager instance;
    return instance;
}

Tone3000DownloadManager::Tone3000DownloadManager()
    : Thread("Tone3000Downloads")
{
    // Default cache directory: Documents/Pedalboard3/NAM Models
    cacheDirectory = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
        .getChildFile("Pedalboard3")
        .getChildFile("NAM Models");

    // Load custom cache directory from settings if set
    juce::String customPath = SettingsManager::getInstance().getString("nam_download_directory", "");
    if (customPath.isNotEmpty())
    {
        juce::File customDir(customPath);
        if (customDir.isDirectory() || customDir.createDirectory())
            cacheDirectory = customDir;
    }

    // Ensure cache directory exists
    cacheDirectory.createDirectory();

    spdlog::info("[Tone3000DownloadManager] Cache directory: {}", cacheDirectory.getFullPathName().toStdString());

    startThread();
}

Tone3000DownloadManager::~Tone3000DownloadManager()
{
    shouldStop = true;
    stopThread(5000);
}

//==============================================================================
// Download Management

void Tone3000DownloadManager::queueDownload(const Tone3000::ToneInfo& tone)
{
    // If we don't have a URL yet, need to fetch it first
    if (tone.modelUrl.empty())
    {
        spdlog::info("[Tone3000DownloadManager] Fetching download URL for: {}", tone.name);

        Tone3000Client::getInstance().getModelDownloadInfo(juce::String(tone.id),
            [this, tone](juce::String url, int64_t fileSize, Tone3000::ApiError error) {
                if (error.isError())
                {
                    spdlog::error("[Tone3000DownloadManager] Failed to get download URL: {}", error.message);
                    notifyFailed(juce::String(tone.id), juce::String(error.message));
                    return;
                }

                queueDownload(juce::String(tone.id), juce::String(tone.name), url, fileSize);
            });
        return;
    }

    queueDownload(juce::String(tone.id), juce::String(tone.name),
                  juce::String(tone.modelUrl), tone.fileSize);
}

void Tone3000DownloadManager::queueDownload(const juce::String& toneId, const juce::String& toneName,
                                             const juce::String& url, int64_t expectedSize)
{
    // Check if already cached
    if (isCached(toneId))
    {
        spdlog::info("[Tone3000DownloadManager] Already cached: {}", toneName.toStdString());
        notifyCompleted(toneId, getCachedFile(toneId));
        return;
    }

    // Check if already in queue
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        for (const auto& task : downloadQueue)
        {
            if (task.toneId == toneId.toStdString())
            {
                spdlog::info("[Tone3000DownloadManager] Already queued: {}", toneName.toStdString());
                return;
            }
        }
    }

    // Create download task
    Tone3000::DownloadTask task;
    task.toneId = toneId.toStdString();
    task.toneName = toneName.toStdString();
    task.url = url.toStdString();
    task.targetPath = getTargetPath(toneId, toneName).getFullPathName().toStdString();
    task.state = Tone3000::DownloadState::Pending;
    task.totalBytes = expectedSize;

    {
        std::lock_guard<std::mutex> lock(queueMutex);
        downloadQueue.push_back(task);
    }

    spdlog::info("[Tone3000DownloadManager] Queued download: {} -> {}",
        toneName.toStdString(), task.targetPath);

    notifyQueued(toneId);
}

void Tone3000DownloadManager::cancelDownload(const juce::String& toneId)
{
    std::lock_guard<std::mutex> lock(queueMutex);

    // If currently downloading, mark for cancellation
    if (currentlyDownloading == toneId)
    {
        for (auto& task : downloadQueue)
        {
            if (task.toneId == toneId.toStdString())
            {
                task.state = Tone3000::DownloadState::Cancelled;
                break;
            }
        }
    }
    else
    {
        // Remove from queue
        auto it = std::remove_if(downloadQueue.begin(), downloadQueue.end(),
            [&toneId](const Tone3000::DownloadTask& task) {
                return task.toneId == toneId.toStdString();
            });

        if (it != downloadQueue.end())
        {
            downloadQueue.erase(it, downloadQueue.end());
            notifyCancelled(toneId);
        }
    }
}

void Tone3000DownloadManager::cancelAll()
{
    std::lock_guard<std::mutex> lock(queueMutex);

    for (auto& task : downloadQueue)
    {
        if (task.isActive())
        {
            task.state = Tone3000::DownloadState::Cancelled;
            notifyCancelled(juce::String(task.toneId));
        }
    }

    downloadQueue.clear();
}

std::vector<Tone3000::DownloadTask> Tone3000DownloadManager::getQueue() const
{
    std::lock_guard<std::mutex> lock(queueMutex);
    return std::vector<Tone3000::DownloadTask>(downloadQueue.begin(), downloadQueue.end());
}

const Tone3000::DownloadTask* Tone3000DownloadManager::getTask(const juce::String& toneId) const
{
    std::lock_guard<std::mutex> lock(queueMutex);

    for (const auto& task : downloadQueue)
    {
        if (task.toneId == toneId.toStdString())
            return &task;
    }

    return nullptr;
}

bool Tone3000DownloadManager::isDownloading(const juce::String& toneId) const
{
    std::lock_guard<std::mutex> lock(queueMutex);

    for (const auto& task : downloadQueue)
    {
        if (task.toneId == toneId.toStdString() && task.isActive())
            return true;
    }

    return false;
}

bool Tone3000DownloadManager::hasActiveDownloads() const
{
    std::lock_guard<std::mutex> lock(queueMutex);

    for (const auto& task : downloadQueue)
    {
        if (task.isActive())
            return true;
    }

    return false;
}

//==============================================================================
// Cache Management

juce::File Tone3000DownloadManager::getCacheDirectory() const
{
    return cacheDirectory;
}

void Tone3000DownloadManager::setCacheDirectory(const juce::File& directory)
{
    if (directory.isDirectory() || directory.createDirectory())
    {
        cacheDirectory = directory;
        SettingsManager::getInstance().setValue("nam_download_directory",
            directory.getFullPathName());
        spdlog::info("[Tone3000DownloadManager] NAM download directory changed to: {}",
            directory.getFullPathName().toStdString());
    }
}

bool Tone3000DownloadManager::isCached(const juce::String& toneId) const
{
    return getCachedFile(toneId).existsAsFile();
}

juce::File Tone3000DownloadManager::getCachedFile(const juce::String& toneId) const
{
    // Look for .nam file in tone's cache folder
    juce::File toneDir = cacheDirectory.getChildFile(toneId);

    if (toneDir.isDirectory())
    {
        auto files = toneDir.findChildFiles(juce::File::findFiles, false, "*.nam");
        if (!files.isEmpty())
            return files[0];
    }

    return {};
}

void Tone3000DownloadManager::clearCache()
{
    spdlog::info("[Tone3000DownloadManager] Clearing cache...");

    // Delete all subdirectories in cache
    auto dirs = cacheDirectory.findChildFiles(juce::File::findDirectories, false);
    for (auto& dir : dirs)
        dir.deleteRecursively();

    spdlog::info("[Tone3000DownloadManager] Cache cleared");
}

int64_t Tone3000DownloadManager::getCacheSize() const
{
    int64_t totalSize = 0;

    auto files = cacheDirectory.findChildFiles(juce::File::findFiles, true);
    for (const auto& file : files)
        totalSize += file.getSize();

    return totalSize;
}

//==============================================================================
// Listeners

void Tone3000DownloadManager::addListener(Listener* listener)
{
    listeners.add(listener);
}

void Tone3000DownloadManager::removeListener(Listener* listener)
{
    listeners.remove(listener);
}

//==============================================================================
// Thread Implementation

void Tone3000DownloadManager::run()
{
    spdlog::info("[Tone3000DownloadManager] Download thread started");

    while (!shouldStop && !threadShouldExit())
    {
        Tone3000::DownloadTask* taskToProcess = nullptr;

        // Find next pending task
        {
            std::lock_guard<std::mutex> lock(queueMutex);

            for (auto& task : downloadQueue)
            {
                if (task.state == Tone3000::DownloadState::Pending)
                {
                    task.state = Tone3000::DownloadState::Downloading;
                    taskToProcess = &task;
                    currentlyDownloading = juce::String(task.toneId);
                    break;
                }
            }
        }

        if (taskToProcess)
        {
            notifyStarted(juce::String(taskToProcess->toneId));

            bool success = processDownload(*taskToProcess);

            // Clean up completed/failed tasks
            {
                std::lock_guard<std::mutex> lock(queueMutex);

                auto it = std::remove_if(downloadQueue.begin(), downloadQueue.end(),
                    [](const Tone3000::DownloadTask& t) {
                        return t.state == Tone3000::DownloadState::Completed ||
                               t.state == Tone3000::DownloadState::Failed ||
                               t.state == Tone3000::DownloadState::Cancelled;
                    });

                downloadQueue.erase(it, downloadQueue.end());
                currentlyDownloading = "";
            }
        }
        else
        {
            // No tasks, wait a bit
            Thread::sleep(100);
        }
    }

    spdlog::info("[Tone3000DownloadManager] Download thread stopped");
}

bool Tone3000DownloadManager::processDownload(Tone3000::DownloadTask& task)
{
    spdlog::info("[Tone3000DownloadManager] Starting download: {}", task.toneName);

    // Create target directory
    juce::File targetFile(task.targetPath);
    targetFile.getParentDirectory().createDirectory();

    // Create temporary file for download
    juce::File tempFile = targetFile.getSiblingFile(targetFile.getFileName() + ".tmp");

    // Set up URL with progress callback
    juce::URL url(juce::String(task.url));

    auto lastProgressUpdate = juce::Time::getMillisecondCounter();

    // Get auth token for download (TONE3000 may require authentication)
    juce::String authHeader;
    auto tokens = Tone3000Client::getInstance().getTokens();
    if (tokens.isValid())
        authHeader = "Authorization: Bearer " + juce::String(tokens.accessToken);

    auto options = juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inAddress)
        .withConnectionTimeoutMs(30000)
        .withExtraHeaders(authHeader)
        .withProgressCallback([&](int bytesDownloaded, int totalBytes) -> bool {
            task.bytesDownloaded = bytesDownloaded;
            if (totalBytes > 0)
                task.totalBytes = totalBytes;

            task.progress = task.totalBytes > 0
                ? static_cast<float>(bytesDownloaded) / task.totalBytes
                : 0.0f;

            // Throttle progress updates
            auto now = juce::Time::getMillisecondCounter();
            if (now - lastProgressUpdate >= PROGRESS_UPDATE_INTERVAL_MS)
            {
                lastProgressUpdate = now;
                notifyProgress(juce::String(task.toneId), task.progress,
                              task.bytesDownloaded, task.totalBytes);
            }

            // Check for cancellation
            if (task.state == Tone3000::DownloadState::Cancelled)
                return false;

            return !shouldStop && !threadShouldExit();
        });

    auto stream = url.createInputStream(options);

    if (!stream)
    {
        task.state = Tone3000::DownloadState::Failed;
        task.errorMessage = "Failed to connect to download server";
        spdlog::error("[Tone3000DownloadManager] {}: {}", task.toneName, task.errorMessage);
        notifyFailed(juce::String(task.toneId), juce::String(task.errorMessage));
        return false;
    }

    // Write to temp file - scope the FileOutputStream so it closes before we move the file
    int64_t totalBytesWritten = 0;
    {
        juce::FileOutputStream fileStream(tempFile);

        if (!fileStream.openedOk())
        {
            task.state = Tone3000::DownloadState::Failed;
            task.errorMessage = "Failed to create download file";
            spdlog::error("[Tone3000DownloadManager] {}: {}", task.toneName, task.errorMessage);
            notifyFailed(juce::String(task.toneId), juce::String(task.errorMessage));
            return false;
        }

        // Download in chunks
        char buffer[DOWNLOAD_BUFFER_SIZE];

        while (!shouldStop && !threadShouldExit() && task.state == Tone3000::DownloadState::Downloading)
        {
            int bytesRead = stream->read(buffer, DOWNLOAD_BUFFER_SIZE);

            if (bytesRead <= 0)
                break;

            fileStream.write(buffer, bytesRead);
            totalBytesWritten += bytesRead;
        }

        fileStream.flush();
    }  // FileOutputStream closes here

    spdlog::info("[Tone3000DownloadManager] Download finished, wrote {} bytes to temp file", totalBytesWritten);

    // Check if we actually downloaded anything
    if (totalBytesWritten == 0)
    {
        task.state = Tone3000::DownloadState::Failed;
        task.errorMessage = "No data received from server";
        tempFile.deleteFile();
        spdlog::error("[Tone3000DownloadManager] {}: {}", task.toneName, task.errorMessage);
        notifyFailed(juce::String(task.toneId), juce::String(task.errorMessage));
        return false;
    }

    // Check if we got a small response (likely an error page)
    if (totalBytesWritten < 1000)
    {
        // Read the temp file content to check if it's an error response
        juce::String content = tempFile.loadFileAsString();
        spdlog::warn("[Tone3000DownloadManager] Small download ({} bytes), content: {}",
            totalBytesWritten, content.substring(0, 200).toStdString());

        if (content.containsIgnoreCase("error") || content.containsIgnoreCase("unauthorized") ||
            content.containsIgnoreCase("<!DOCTYPE") || content.containsIgnoreCase("<html"))
        {
            task.state = Tone3000::DownloadState::Failed;
            task.errorMessage = "Server returned error instead of file";
            tempFile.deleteFile();
            spdlog::error("[Tone3000DownloadManager] {}: {}", task.toneName, task.errorMessage);
            notifyFailed(juce::String(task.toneId), juce::String(task.errorMessage));
            return false;
        }
    }

    // Check if cancelled
    if (task.state == Tone3000::DownloadState::Cancelled)
    {
        tempFile.deleteFile();
        spdlog::info("[Tone3000DownloadManager] Download cancelled: {}", task.toneName);
        notifyCancelled(juce::String(task.toneId));
        return false;
    }

    // Move temp file to final location
    if (!tempFile.moveFileTo(targetFile))
    {
        task.state = Tone3000::DownloadState::Failed;
        task.errorMessage = "Failed to save downloaded file";
        tempFile.deleteFile();
        spdlog::error("[Tone3000DownloadManager] {}: {}", task.toneName, task.errorMessage);
        notifyFailed(juce::String(task.toneId), juce::String(task.errorMessage));
        return false;
    }

    task.state = Tone3000::DownloadState::Completed;
    task.progress = 1.0f;

    spdlog::info("[Tone3000DownloadManager] Download complete: {} ({} bytes)",
        task.toneName, targetFile.getSize());

    notifyCompleted(juce::String(task.toneId), targetFile);
    return true;
}

//==============================================================================
// Notification Helpers

void Tone3000DownloadManager::notifyQueued(const juce::String& toneId)
{
    juce::MessageManager::callAsync([this, toneId]() {
        listeners.call(&Listener::downloadQueued, toneId);
    });
}

void Tone3000DownloadManager::notifyStarted(const juce::String& toneId)
{
    juce::MessageManager::callAsync([this, toneId]() {
        listeners.call(&Listener::downloadStarted, toneId);
    });
}

void Tone3000DownloadManager::notifyProgress(const juce::String& toneId, float progress,
                                              int64_t bytes, int64_t total)
{
    juce::MessageManager::callAsync([this, toneId, progress, bytes, total]() {
        listeners.call(&Listener::downloadProgress, toneId, progress, bytes, total);
    });
}

void Tone3000DownloadManager::notifyCompleted(const juce::String& toneId, const juce::File& file)
{
    juce::MessageManager::callAsync([this, toneId, file]() {
        listeners.call(&Listener::downloadCompleted, toneId, file);
    });
}

void Tone3000DownloadManager::notifyFailed(const juce::String& toneId, const juce::String& error)
{
    juce::MessageManager::callAsync([this, toneId, error]() {
        listeners.call(&Listener::downloadFailed, toneId, error);
    });
}

void Tone3000DownloadManager::notifyCancelled(const juce::String& toneId)
{
    juce::MessageManager::callAsync([this, toneId]() {
        listeners.call(&Listener::downloadCancelled, toneId);
    });
}

juce::File Tone3000DownloadManager::getTargetPath(const juce::String& toneId,
                                                   const juce::String& toneName) const
{
    // Create folder for this tone
    juce::File toneDir = cacheDirectory.getChildFile(toneId);

    // Sanitize filename
    juce::String safeName = toneName.removeCharacters("<>:\"/\\|?*")
                                     .replaceCharacter(' ', '_');

    if (safeName.isEmpty())
        safeName = "model";

    return toneDir.getChildFile(safeName + ".nam");
}
