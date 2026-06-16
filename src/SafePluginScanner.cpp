/*
  ==============================================================================

    SafePluginScanner.cpp

    Safe plugin scanning with out-of-process isolation and timeout protection.

  ==============================================================================
*/

#include "SafePluginScanner.h"

#include "CrashProtection.h"
#include "PluginBlacklist.h"

#include <spdlog/spdlog.h>

//==============================================================================
// SafePluginScanner
//==============================================================================

SafePluginScanner::SafePluginScanner(juce::KnownPluginList& listToAddTo, juce::AudioPluginFormat& formatToScan,
                                     juce::FileSearchPath directoriesToSearch, bool searchRecursively,
                                     const juce::File& deadMansPedalFile, bool useOutOfProcess)
    : useOutOfProcessScanning(useOutOfProcess), pluginList(listToAddTo), format(formatToScan)
{
    // Create the base scanner
    baseScanner = std::make_unique<juce::PluginDirectoryScanner>(listToAddTo, formatToScan, directoriesToSearch,
                                                                  searchRecursively, deadMansPedalFile);

    if (useOutOfProcessScanning)
    {
        // Check if scanner executable exists
        auto scannerExe = PluginScannerClient::getScannerExecutable();
        if (scannerExe.existsAsFile())
        {
            scannerClient = std::make_unique<PluginScannerClient>();
            spdlog::info("[SafePluginScanner] Using out-of-process scanning");
        }
        else
        {
            spdlog::warn("[SafePluginScanner] Scanner executable not found at {}, falling back to in-process scanning",
                         scannerExe.getFullPathName().toStdString());
            useOutOfProcessScanning = false;
        }
    }
}

SafePluginScanner::~SafePluginScanner()
{
    if (scannerClient)
        scannerClient->stopScanner();
}

juce::String SafePluginScanner::getNextPluginFileThatWillBeScanned() const
{
    if (baseScanner)
        return baseScanner->getNextPluginFileThatWillBeScanned();
    return {};
}

float SafePluginScanner::getProgress() const
{
    if (baseScanner)
        return baseScanner->getProgress();
    return 1.0f;
}

bool SafePluginScanner::scanNextFile(bool dontRescanIfAlreadyInList, juce::String& nameOfPluginBeingScanned)
{
    if (!baseScanner)
        return false;

    // Get the next file to scan from the base scanner
    juce::String nextFile = baseScanner->getNextPluginFileThatWillBeScanned();

    if (nextFile.isEmpty())
        return false;

    nameOfPluginBeingScanned = juce::File(nextFile).getFileName();

    // Check if blacklisted
    if (PluginBlacklist::getInstance().isBlacklisted(nextFile))
    {
        spdlog::debug("[SafePluginScanner] Skipping blacklisted plugin: {}", nextFile.toStdString());
        // Let the base scanner handle advancing
        return baseScanner->scanNextFile(dontRescanIfAlreadyInList, nameOfPluginBeingScanned);
    }

    // Use out-of-process scanner if available
    if (useOutOfProcessScanning && scannerClient)
    {
        juce::OwnedArray<juce::PluginDescription> results;

        bool scanSuccess = scannerClient->scanPlugin(nextFile, format.getName(), results);

        if (scanSuccess)
        {
            // Add found plugins to the list
            for (auto* desc : results)
            {
                pluginList.addType(*desc);
            }
            spdlog::debug("[SafePluginScanner] Out-of-process scan found {} plugin(s) in {}", results.size(),
                          nextFile.toStdString());
        }
        else
        {
            spdlog::warn("[SafePluginScanner] Out-of-process scan failed for: {}", nextFile.toStdString());
        }

        // Call base scanner to advance to next file
        // It may rescan but that's OK - the plugin will already be in the list so it will be fast
        juce::String dummy;
        return baseScanner->scanNextFile(true, dummy);
    }
    else
    {
        // Fall back to in-process scanning with timeout protection
        struct ScanAttemptState
        {
            std::atomic<bool> success{false};
            juce::String scannedName;
        };

        auto state = std::make_shared<ScanAttemptState>();
        auto* scanner = baseScanner.get();
        const bool dontRescan = dontRescanIfAlreadyInList;

        auto result = CrashProtection::getInstance().executeWithProtectionAndTimeout(
            [state, scanner, dontRescan]() {
                juce::String scannedName;
                const bool ok = scanner->scanNextFile(dontRescan, scannedName);
                state->success.store(ok, std::memory_order_release);
                state->scannedName = scannedName;
            },
            "Plugin Scan: " + nameOfPluginBeingScanned, scanTimeoutMs, nextFile);

        if (result == TimedOperationResult::Timeout)
        {
            spdlog::warn("[SafePluginScanner] Scan timed out, plugin blacklisted: {}", nextFile.toStdString());
            // The timeout handler already blacklisted it, check if more files remain
            return !baseScanner->getNextPluginFileThatWillBeScanned().isEmpty();
        }
        else if (result == TimedOperationResult::Exception)
        {
            spdlog::error("[SafePluginScanner] Scan threw exception: {}", nextFile.toStdString());
            return !baseScanner->getNextPluginFileThatWillBeScanned().isEmpty();
        }

        nameOfPluginBeingScanned = state->scannedName;
        return state->success.load(std::memory_order_acquire);
    }
}

//==============================================================================
// SafePluginListComponent
//==============================================================================

SafePluginListComponent::SafePluginListComponent(juce::AudioPluginFormatManager& fm,
                                                 juce::KnownPluginList& listToRepresent,
                                                 const juce::File& deadMansPedalFile,
                                                 juce::PropertiesFile* /*propertiesToUse*/)
    : juce::Thread("Safe Plugin Scanner"),
      formatManager(fm), pluginList(listToRepresent), deadMansPedal(deadMansPedalFile)
{
    // Create table
    table = std::make_unique<juce::TableListBox>("plugins", this);
    table->getHeader().addColumn("Name", 1, 200, 100, 400);
    table->getHeader().addColumn("Type", 2, 80, 50, 100);
    table->getHeader().addColumn("Category", 3, 100, 50, 150);
    table->getHeader().addColumn("Manufacturer", 4, 150, 100, 250);
    addAndMakeVisible(table.get());

    // Create buttons
    scanButton = std::make_unique<juce::TextButton>("Scan for new plugins...");
    scanButton->addListener(this);
    addAndMakeVisible(scanButton.get());

    clearButton = std::make_unique<juce::TextButton>("Clear list");
    clearButton->addListener(this);
    addAndMakeVisible(clearButton.get());

    removeButton = std::make_unique<juce::TextButton>("Remove selected");
    removeButton->addListener(this);
    addAndMakeVisible(removeButton.get());

    // Progress UI
    progressLabel = std::make_unique<juce::Label>("progress", "");
    progressLabel->setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(progressLabel.get());

    progressBar = std::make_unique<juce::ProgressBar>(scanProgress);
    progressBar->setVisible(false);
    addAndMakeVisible(progressBar.get());

    updateList();
}

SafePluginListComponent::~SafePluginListComponent()
{
    cancelScan();
    signalThreadShouldExit();
    waitForThreadToExit(-1);
    stopTimer();
    scanner.reset();
}

void SafePluginListComponent::resized()
{
    auto bounds = getLocalBounds().reduced(4);

    auto buttonArea = bounds.removeFromBottom(30);
    scanButton->setBounds(buttonArea.removeFromLeft(150));
    buttonArea.removeFromLeft(8);
    clearButton->setBounds(buttonArea.removeFromLeft(80));
    buttonArea.removeFromLeft(8);
    removeButton->setBounds(buttonArea.removeFromLeft(100));

    bounds.removeFromBottom(4);

    auto progressArea = bounds.removeFromBottom(24);
    progressLabel->setBounds(progressArea.removeFromLeft(200));
    progressBar->setBounds(progressArea);

    bounds.removeFromBottom(4);
    table->setBounds(bounds);
}

void SafePluginListComponent::paint(juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
}

int SafePluginListComponent::getNumRows()
{
    return pluginList.getNumTypes();
}

void SafePluginListComponent::paintRowBackground(juce::Graphics& g, int rowNumber, int /*width*/, int /*height*/,
                                                 bool rowIsSelected)
{
    if (rowIsSelected)
        g.fillAll(juce::Colours::lightblue);
    else if (rowNumber % 2)
        g.fillAll(juce::Colour(0xffeeeeee));
}

void SafePluginListComponent::paintCell(juce::Graphics& g, int rowNumber, int columnId, int width, int height,
                                        bool /*rowIsSelected*/)
{
    if (rowNumber >= pluginList.getNumTypes())
        return;

    auto types = pluginList.getTypes();
    if (rowNumber >= types.size())
        return;

    const auto& desc = types[static_cast<size_t>(rowNumber)];

    g.setColour(juce::Colours::black);
    g.setFont(13.0f);

    juce::String text;
    switch (columnId)
    {
    case 1:
        text = desc.name;
        break;
    case 2:
        text = desc.pluginFormatName;
        break;
    case 3:
        text = desc.category;
        break;
    case 4:
        text = desc.manufacturerName;
        break;
    }

    g.drawText(text, 4, 0, width - 8, height, juce::Justification::centredLeft, true);
}

void SafePluginListComponent::cellClicked(int rowNumber, int /*columnId*/, const juce::MouseEvent&)
{
    table->selectRow(rowNumber);
}

void SafePluginListComponent::sortOrderChanged(int newSortColumnId, bool isForwards)
{
    sortColumnId = newSortColumnId;
    sortForward = isForwards;
    updateList();
}

void SafePluginListComponent::buttonClicked(juce::Button* button)
{
    if (button == scanButton.get())
    {
        if (scanning)
            cancelScan();
        else
            startScan();
    }
    else if (button == clearButton.get())
    {
        pluginList.clear();
        updateList();
    }
    else if (button == removeButton.get())
    {
        auto selected = table->getSelectedRow();
        if (selected >= 0 && selected < pluginList.getNumTypes())
        {
            auto types = pluginList.getTypes();
            pluginList.removeType(types[static_cast<size_t>(selected)]);
            updateList();
        }
    }
}

void SafePluginListComponent::startScan()
{
    if (scanning)
        return;

    signalThreadShouldExit();
    waitForThreadToExit(-1);

    scanning = true;
    scanButton->setButtonText("Cancel scan");
    scanButton->setEnabled(true);
    progressBar->setVisible(true);
    scanProgress = 0.0;
    workerScanProgress.store(0.0, std::memory_order_release);
    scannerThreadFinished.store(false, std::memory_order_release);
    scanCancellationRequested.store(false, std::memory_order_release);
    pendingListUpdate.store(false, std::memory_order_release);
    progressLabel->setText("Starting scan...", juce::dontSendNotification);

    {
        const juce::ScopedLock lock(scanStateLock);
        pluginBeingScanned.clear();
    }

    // Create scanner for VST3 format
    for (int i = 0; i < formatManager.getNumFormats(); ++i)
    {
        auto* format = formatManager.getFormat(i);
        if (format->getName() == "VST3")
        {
            auto searchPaths = format->getDefaultLocationsToSearch();
            scanner = std::make_unique<SafePluginScanner>(pluginList, *format, searchPaths, true, deadMansPedal, true);
            break;
        }
    }

    if (scanner && startThread())
        startTimer(100);
    else
        scanFinished();
}

void SafePluginListComponent::cancelScan()
{
    if (!scanning)
        return;

    scanCancellationRequested.store(true, std::memory_order_release);
    signalThreadShouldExit();
    scanButton->setEnabled(false);
    progressLabel->setText("Cancelling scan...", juce::dontSendNotification);
}

void SafePluginListComponent::run()
{
    while (!threadShouldExit())
    {
        auto* activeScanner = scanner.get();
        if (activeScanner == nullptr)
            break;

        juce::String pluginName;
        const bool hasMore = activeScanner->scanNextFile(true, pluginName);

        {
            const juce::ScopedLock lock(scanStateLock);
            pluginBeingScanned = pluginName;
        }

        workerScanProgress.store(static_cast<double>(activeScanner->getProgress()), std::memory_order_release);
        pendingListUpdate.store(true, std::memory_order_release);

        if (!hasMore)
            break;
    }

    scannerThreadFinished.store(true, std::memory_order_release);
}

void SafePluginListComponent::timerCallback()
{
    if (!scanner)
    {
        scanFinished();
        return;
    }

    juce::String pluginName;
    {
        const juce::ScopedLock lock(scanStateLock);
        pluginName = pluginBeingScanned;
    }

    scanProgress = workerScanProgress.load(std::memory_order_acquire);

    if (scanCancellationRequested.load(std::memory_order_acquire))
        progressLabel->setText("Cancelling scan...", juce::dontSendNotification);
    else if (pluginName.isNotEmpty())
        progressLabel->setText("Scanning: " + pluginName, juce::dontSendNotification);

    if (pendingListUpdate.exchange(false, std::memory_order_acq_rel))
        updateList();

    if (scannerThreadFinished.load(std::memory_order_acquire) && !isThreadRunning())
    {
        scanFinished();
    }
}

void SafePluginListComponent::updateList()
{
    table->updateContent();
    table->repaint();
}

void SafePluginListComponent::scanFinished()
{
    if (isThreadRunning())
        return;

    const bool wasCancelled = scanCancellationRequested.load(std::memory_order_acquire);

    scanning = false;
    scanner.reset();
    stopTimer();
    scanCancellationRequested.store(false, std::memory_order_release);
    scannerThreadFinished.store(false, std::memory_order_release);
    pendingListUpdate.store(false, std::memory_order_release);

    scanButton->setButtonText("Scan for new plugins...");
    scanButton->setEnabled(true);
    progressBar->setVisible(false);
    progressLabel->setText(wasCancelled ? "Scan cancelled."
                                         : "Scan complete. Found " + juce::String(pluginList.getNumTypes()) + " plugins.",
                           juce::dontSendNotification);

    updateList();
}
