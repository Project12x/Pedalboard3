/*
  ==============================================================================

    PluginScannerClient.h

    Host-side client for communicating with the out-of-process plugin scanner.

  ==============================================================================
*/

#pragma once

#include "PluginScannerIPC.h"

#include <JuceHeader.h>

/**
 * @class PluginScannerClient
 * @brief Manages communication with the out-of-process plugin scanner.
 *
 * Launches the scanner process, sends scan requests, and handles responses.
 * If the scanner crashes, it's automatically restarted for the next scan.
 */
class PluginScannerClient
{
  public:
    PluginScannerClient();
    ~PluginScannerClient();

    /**
     * @brief Scan a plugin file using the out-of-process scanner.
     * @param pluginPath Full path to the plugin file
     * @param formatName Plugin format name (e.g., "VST3")
     * @param results Output array for found plugin descriptions
     * @return true if scan completed successfully
     */
    bool scanPlugin(const juce::String& pluginPath, const juce::String& formatName,
                    juce::OwnedArray<juce::PluginDescription>& results);

    /**
     * @brief Check if the scanner process is currently running.
     */
    bool isScannerRunning() const;

    /**
     * @brief Explicitly start the scanner process.
     * Usually called automatically by scanPlugin().
     */
    bool startScanner();

    /**
     * @brief Stop the scanner process.
     */
    void stopScanner();

    /**
     * @brief Get the path to the scanner executable.
     */
    static juce::File getScannerExecutable();

    /**
     * @brief Listener interface for scan progress updates.
     */
    class Listener
    {
      public:
        virtual ~Listener() = default;
        virtual void scannerStarted() {}
        virtual void scannerStopped() {}
        virtual void scanProgress(const juce::String& pluginPath) {}
        virtual void scanComplete(const juce::String& pluginPath, bool success) {}
        virtual void scannerCrashed(const juce::String& lastPlugin) {}
    };

    void addListener(Listener* listener);
    void removeListener(Listener* listener);

  private:
    bool ensureScannerRunning();
    bool sendRequest(const PluginScannerIPC::ScanRequest& request);
    bool waitForResponse(PluginScannerIPC::ScanResponse& response, int timeoutMs);
    void handleScannerCrash();

    // Platform-specific handles stored as void* to avoid Windows header in header file
    void* pipeHandle = nullptr;
    void* scannerProcess = nullptr;

    juce::String lastScannedPlugin;
    juce::ListenerList<Listener> listeners;
    juce::CriticalSection scanLock;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginScannerClient)
};
