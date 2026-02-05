/*
  ==============================================================================

    PluginScannerClient.cpp

    Host-side client for communicating with the out-of-process plugin scanner.

  ==============================================================================
*/

#include "PluginScannerClient.h"

#include "PluginBlacklist.h"

#include <spdlog/spdlog.h>

#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #define NOMINMAX
    #include <windows.h>
#endif

using namespace PluginScannerIPC;

//------------------------------------------------------------------------------
PluginScannerClient::PluginScannerClient()
{
    spdlog::debug("[PluginScannerClient] Created");
}

//------------------------------------------------------------------------------
PluginScannerClient::~PluginScannerClient()
{
    stopScanner();
}

//------------------------------------------------------------------------------
juce::File PluginScannerClient::getScannerExecutable()
{
    // Scanner should be in the same directory as the main executable
    auto appDir = juce::File::getSpecialLocation(juce::File::currentExecutableFile).getParentDirectory();

#ifdef _WIN32
    return appDir.getChildFile("Pedalboard3Scanner.exe");
#else
    return appDir.getChildFile("Pedalboard3Scanner");
#endif
}

//------------------------------------------------------------------------------
bool PluginScannerClient::isScannerRunning() const
{
#ifdef _WIN32
    HANDLE hProcess = static_cast<HANDLE>(scannerProcess);
    if (hProcess == nullptr || hProcess == INVALID_HANDLE_VALUE)
        return false;

    DWORD exitCode;
    if (GetExitCodeProcess(hProcess, &exitCode))
        return exitCode == STILL_ACTIVE;

    return false;
#else
    return false;
#endif
}

//------------------------------------------------------------------------------
bool PluginScannerClient::startScanner()
{
#ifdef _WIN32
    if (isScannerRunning())
        return true;

    // Close any existing handles
    stopScanner();

    spdlog::info("[PluginScannerClient] Starting scanner process");

    // Create the named pipe for communication
    HANDLE hPipe = CreateNamedPipeA(PIPE_NAME,
                                    PIPE_ACCESS_DUPLEX,
                                    PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
                                    1,       // Max instances
                                    65536,   // Output buffer size
                                    65536,   // Input buffer size
                                    0,       // Default timeout
                                    nullptr  // Security attributes
    );

    if (hPipe == INVALID_HANDLE_VALUE)
    {
        spdlog::error("[PluginScannerClient] Failed to create named pipe: {}", GetLastError());
        return false;
    }

    pipeHandle = hPipe;

    // Launch the scanner process
    auto scannerExe = getScannerExecutable();
    if (!scannerExe.existsAsFile())
    {
        spdlog::error("[PluginScannerClient] Scanner executable not found: {}", scannerExe.getFullPathName().toStdString());
        CloseHandle(hPipe);
        pipeHandle = nullptr;
        return false;
    }

    STARTUPINFOA si = {};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi = {};

    juce::String cmdLine = "\"" + scannerExe.getFullPathName() + "\"";

    if (!CreateProcessA(nullptr, const_cast<char*>(cmdLine.toRawUTF8()), nullptr, nullptr, FALSE,
                        CREATE_NO_WINDOW,  // Run without console window
                        nullptr, nullptr, &si, &pi))
    {
        spdlog::error("[PluginScannerClient] Failed to start scanner process: {}", GetLastError());
        CloseHandle(hPipe);
        pipeHandle = nullptr;
        return false;
    }

    scannerProcess = pi.hProcess;
    CloseHandle(pi.hThread);

    // Wait for scanner to connect to our pipe
    spdlog::debug("[PluginScannerClient] Waiting for scanner to connect...");

    if (!ConnectNamedPipe(hPipe, nullptr))
    {
        DWORD err = GetLastError();
        if (err != ERROR_PIPE_CONNECTED)
        {
            spdlog::error("[PluginScannerClient] Scanner failed to connect: {}", err);
            stopScanner();
            return false;
        }
    }

    // Wait for Ready message
    MessageHeader header;
    juce::String payload;

    // Set a timeout for reading
    COMMTIMEOUTS timeouts = {};
    timeouts.ReadTotalTimeoutConstant = 5000;  // 5 second timeout
    SetCommTimeouts(hPipe, &timeouts);

    DWORD bytesRead;
    if (!ReadFile(hPipe, &header, sizeof(header), &bytesRead, nullptr) || bytesRead != sizeof(header))
    {
        spdlog::error("[PluginScannerClient] Failed to read Ready message from scanner");
        stopScanner();
        return false;
    }

    if (header.type != MessageType::Ready)
    {
        spdlog::error("[PluginScannerClient] Expected Ready message, got: {}", static_cast<int>(header.type));
        stopScanner();
        return false;
    }

    spdlog::info("[PluginScannerClient] Scanner process started and ready");
    listeners.call(&Listener::scannerStarted);

    return true;
#else
    spdlog::warn("[PluginScannerClient] Out-of-process scanning not implemented on this platform");
    return false;
#endif
}

//------------------------------------------------------------------------------
void PluginScannerClient::stopScanner()
{
#ifdef _WIN32
    HANDLE hPipe = static_cast<HANDLE>(pipeHandle);
    HANDLE hProcess = static_cast<HANDLE>(scannerProcess);

    if (hPipe != nullptr && hPipe != INVALID_HANDLE_VALUE)
    {
        // Try to send shutdown message
        MessageHeader header;
        header.type = MessageType::Shutdown;
        header.payloadSize = 0;
        DWORD bytesWritten;
        WriteFile(hPipe, &header, sizeof(header), &bytesWritten, nullptr);

        CloseHandle(hPipe);
        pipeHandle = nullptr;
    }

    if (hProcess != nullptr && hProcess != INVALID_HANDLE_VALUE)
    {
        // Wait briefly for graceful shutdown
        if (WaitForSingleObject(hProcess, 1000) == WAIT_TIMEOUT)
        {
            spdlog::warn("[PluginScannerClient] Scanner didn't exit gracefully, terminating");
            TerminateProcess(hProcess, 1);
        }

        CloseHandle(hProcess);
        scannerProcess = nullptr;
    }

    spdlog::debug("[PluginScannerClient] Scanner stopped");
    listeners.call(&Listener::scannerStopped);
#endif
}

//------------------------------------------------------------------------------
bool PluginScannerClient::ensureScannerRunning()
{
    if (isScannerRunning())
        return true;

    return startScanner();
}

//------------------------------------------------------------------------------
bool PluginScannerClient::scanPlugin(const juce::String& pluginPath, const juce::String& formatName,
                                     juce::OwnedArray<juce::PluginDescription>& results)
{
    juce::ScopedLock lock(scanLock);

    spdlog::debug("[PluginScannerClient] Scanning plugin: {}", pluginPath.toStdString());
    lastScannedPlugin = pluginPath;

    listeners.call(&Listener::scanProgress, pluginPath);

    // Ensure scanner is running
    if (!ensureScannerRunning())
    {
        spdlog::error("[PluginScannerClient] Failed to start scanner for: {}", pluginPath.toStdString());
        return false;
    }

#ifdef _WIN32
    HANDLE hPipe = static_cast<HANDLE>(pipeHandle);

    // Build and send request
    ScanRequest request;
    request.pluginPath = pluginPath;
    request.formatName = formatName;

    auto payload = request.serialize();
    MessageHeader header;
    header.type = MessageType::ScanPlugin;
    header.payloadSize = static_cast<uint32_t>(payload.toUTF8().length());

    DWORD bytesWritten;
    if (!WriteFile(hPipe, &header, sizeof(header), &bytesWritten, nullptr))
    {
        spdlog::error("[PluginScannerClient] Failed to send scan request header");
        handleScannerCrash();
        return false;
    }

    if (header.payloadSize > 0)
    {
        auto payloadBytes = payload.toUTF8();
        if (!WriteFile(hPipe, payloadBytes.getAddress(), header.payloadSize, &bytesWritten, nullptr))
        {
            spdlog::error("[PluginScannerClient] Failed to send scan request payload");
            handleScannerCrash();
            return false;
        }
    }

    FlushFileBuffers(hPipe);

    // Wait for response with timeout
    COMMTIMEOUTS timeouts = {};
    timeouts.ReadTotalTimeoutConstant = SCAN_TIMEOUT_MS;
    SetCommTimeouts(hPipe, &timeouts);

    // Check if scanner is still running
    if (!isScannerRunning())
    {
        spdlog::error("[PluginScannerClient] Scanner crashed during scan of: {}", pluginPath.toStdString());
        handleScannerCrash();
        return false;
    }

    // Read response header
    DWORD bytesRead;
    if (!ReadFile(hPipe, &header, sizeof(header), &bytesRead, nullptr) || bytesRead != sizeof(header))
    {
        if (!isScannerRunning())
        {
            spdlog::error("[PluginScannerClient] Scanner crashed during scan of: {}", pluginPath.toStdString());
            handleScannerCrash();
        }
        else
        {
            spdlog::error("[PluginScannerClient] Timeout waiting for scan response: {}", pluginPath.toStdString());
            // Timeout - blacklist the plugin
            PluginBlacklist::getInstance().addToBlacklist(pluginPath);
            stopScanner();  // Kill the hung scanner
        }
        return false;
    }

    // Read response payload
    juce::String responsePayload;
    if (header.payloadSize > 0)
    {
        juce::HeapBlock<char> buffer(header.payloadSize + 1);
        if (!ReadFile(hPipe, buffer.get(), header.payloadSize, &bytesRead, nullptr))
        {
            spdlog::error("[PluginScannerClient] Failed to read response payload");
            return false;
        }
        buffer[header.payloadSize] = 0;
        responsePayload = juce::String::fromUTF8(buffer.get(), static_cast<int>(header.payloadSize));
    }

    // Parse response
    auto response = ScanResponse::deserialize(responsePayload);

    if (response.resultCode != ScanResultCode::Success)
    {
        spdlog::warn("[PluginScannerClient] Scan failed for {}: {}", pluginPath.toStdString(),
                     response.errorMessage.toStdString());
        listeners.call(&Listener::scanComplete, pluginPath, false);
        return false;
    }

    // Parse plugin descriptions from XML
    if (auto xml = juce::XmlDocument::parse(response.pluginXml))
    {
        for (auto* pluginXml : xml->getChildIterator())
        {
            auto desc = std::make_unique<juce::PluginDescription>();
            if (desc->loadFromXml(*pluginXml))
            {
                results.add(desc.release());
            }
        }
    }

    spdlog::info("[PluginScannerClient] Successfully scanned {}: {} plugin(s) found", pluginPath.toStdString(),
                 results.size());
    listeners.call(&Listener::scanComplete, pluginPath, true);

    return true;
#else
    juce::ignoreUnused(pluginPath, formatName, results);
    return false;
#endif
}

//------------------------------------------------------------------------------
void PluginScannerClient::handleScannerCrash()
{
    spdlog::error("[PluginScannerClient] Scanner crashed while scanning: {}", lastScannedPlugin.toStdString());

    // Auto-blacklist the plugin that caused the crash
    if (lastScannedPlugin.isNotEmpty())
    {
        spdlog::warn("[PluginScannerClient] Auto-blacklisting crashed plugin: {}", lastScannedPlugin.toStdString());
        PluginBlacklist::getInstance().addToBlacklist(lastScannedPlugin);
    }

    listeners.call(&Listener::scannerCrashed, lastScannedPlugin);

    // Clean up handles
    stopScanner();
}

//------------------------------------------------------------------------------
void PluginScannerClient::addListener(Listener* listener)
{
    listeners.add(listener);
}

//------------------------------------------------------------------------------
void PluginScannerClient::removeListener(Listener* listener)
{
    listeners.remove(listener);
}
