/*
  ==============================================================================

    SafePluginScanner.h

    Safe plugin scanning with out-of-process isolation and timeout protection.
    Integrates with JUCE's KnownPluginList for seamless compatibility.

  ==============================================================================
*/

#pragma once

#include "PluginScannerClient.h"

#include <JuceHeader.h>

/**
 * @class SafePluginScanner
 * @brief Provides safe plugin scanning with crash isolation and timeouts.
 *
 * Uses the out-of-process scanner when available, falling back to
 * in-process scanning with timeout protection otherwise.
 *
 * Note: This class wraps PluginDirectoryScanner rather than inheriting from it,
 * since JUCE's PluginDirectoryScanner methods aren't virtual.
 */
class SafePluginScanner
{
  public:
    /**
     * @brief Create a safe plugin scanner.
     * @param listToAddTo The KnownPluginList to add discovered plugins to
     * @param formatToScan The plugin format to scan
     * @param directoriesToSearch Directories to search for plugins
     * @param searchRecursively Whether to search subdirectories
     * @param deadMansPedalFile File for tracking crashed plugins (JUCE crash protection)
     * @param useOutOfProcess Whether to use out-of-process scanning (default: true)
     */
    SafePluginScanner(juce::KnownPluginList& listToAddTo, juce::AudioPluginFormat& formatToScan,
                      juce::FileSearchPath directoriesToSearch, bool searchRecursively,
                      const juce::File& deadMansPedalFile, bool useOutOfProcess = true);

    ~SafePluginScanner();

    /**
     * @brief Scan the next plugin file.
     * @param dontRescanIfAlreadyInList Skip plugins already in the list
     * @param nameOfPluginBeingScanned Output: name of the plugin being scanned
     * @return true if there are more plugins to scan
     */
    bool scanNextFile(bool dontRescanIfAlreadyInList, juce::String& nameOfPluginBeingScanned);

    /**
     * @brief Get scan progress (0.0 to 1.0).
     */
    float getProgress() const;

    /**
     * @brief Get the next file that will be scanned.
     */
    juce::String getNextPluginFileThatWillBeScanned() const;

    /**
     * @brief Check if out-of-process scanning is being used.
     */
    bool isUsingOutOfProcess() const { return useOutOfProcessScanning && scannerClient != nullptr; }

    /**
     * @brief Set timeout for individual plugin scans (ms).
     */
    void setScanTimeout(int timeoutMs) { scanTimeoutMs = timeoutMs; }

  private:
    std::unique_ptr<juce::PluginDirectoryScanner> baseScanner;
    bool useOutOfProcessScanning;
    std::unique_ptr<PluginScannerClient> scannerClient;
    int scanTimeoutMs = 30000;

    juce::KnownPluginList& pluginList;
    juce::AudioPluginFormat& format;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SafePluginScanner)
};

/**
 * @class SafePluginListComponent
 * @brief Drop-in replacement for PluginListComponent with out-of-process scanning.
 *
 * Uses SafePluginScanner internally for crash-safe plugin discovery.
 */
class SafePluginListComponent : public juce::Component,
                                public juce::TableListBoxModel,
                                public juce::Button::Listener,
                                private juce::Timer
{
  public:
    SafePluginListComponent(juce::AudioPluginFormatManager& formatManager, juce::KnownPluginList& listToRepresent,
                            const juce::File& deadMansPedalFile, juce::PropertiesFile* propertiesToUse = nullptr);

    ~SafePluginListComponent() override;

    // Component overrides
    void resized() override;
    void paint(juce::Graphics& g) override;

    // TableListBoxModel overrides
    int getNumRows() override;
    void paintRowBackground(juce::Graphics&, int rowNumber, int width, int height, bool rowIsSelected) override;
    void paintCell(juce::Graphics&, int rowNumber, int columnId, int width, int height, bool rowIsSelected) override;
    void cellClicked(int rowNumber, int columnId, const juce::MouseEvent&) override;
    void sortOrderChanged(int newSortColumnId, bool isForwards) override;

    // Button::Listener
    void buttonClicked(juce::Button* button) override;

    /**
     * @brief Start scanning for plugins.
     */
    void startScan();

    /**
     * @brief Cancel an ongoing scan.
     */
    void cancelScan();

    /**
     * @brief Check if a scan is in progress.
     */
    bool isScanning() const { return scanning; }

  private:
    void timerCallback() override;
    void updateList();
    void scanFinished();

    juce::AudioPluginFormatManager& formatManager;
    juce::KnownPluginList& pluginList;
    juce::File deadMansPedal;

    std::unique_ptr<juce::TableListBox> table;
    std::unique_ptr<juce::TextButton> scanButton;
    std::unique_ptr<juce::TextButton> clearButton;
    std::unique_ptr<juce::TextButton> removeButton;
    std::unique_ptr<juce::Label> progressLabel;
    std::unique_ptr<juce::ProgressBar> progressBar;

    std::unique_ptr<SafePluginScanner> scanner;
    double scanProgress = 0.0;
    bool scanning = false;

    juce::StringArray pluginNames;
    juce::Array<int> sortedIndices;
    int sortColumnId = 1;
    bool sortForward = true;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SafePluginListComponent)
};
