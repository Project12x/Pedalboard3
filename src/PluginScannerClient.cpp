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
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #include <windows.h>
#endif

using namespace PluginScannerIPC;

#ifdef _WIN32
namespace
{
constexpr int kScannerStartupTimeoutMs = 5000;
constexpr uint32_t kMaxScannerPayloadBytes = 16u * 1024u * 1024u;
constexpr DWORD kPipePollIntervalMs = 10;

bool waitForScannerConnection(HANDLE pipe, HANDLE process, int timeoutMs)
{
    const auto deadline = GetTickCount64() + static_cast<ULONGLONG>(timeoutMs);

    for (;;)
    {
        if (ConnectNamedPipe(pipe, nullptr))
            return true;

        const auto error = GetLastError();
        if (error == ERROR_PIPE_CONNECTED)
            return true;

        if (error != ERROR_PIPE_LISTENING)
            return false;

        if (process != nullptr && WaitForSingleObject(process, 0) != WAIT_TIMEOUT)
            return false;

        if (GetTickCount64() >= deadline)
            return false;

        Sleep(kPipePollIntervalMs);
    }
}

bool waitForPipeBytes(HANDLE pipe, HANDLE process, DWORD bytesNeeded, int timeoutMs)
{
    const auto deadline = GetTickCount64() + static_cast<ULONGLONG>(timeoutMs);

    for (;;)
    {
        DWORD bytesAvailable = 0;
        if (!PeekNamedPipe(pipe, nullptr, 0, nullptr, &bytesAvailable, nullptr))
            return false;

        if (bytesAvailable >= bytesNeeded)
            return true;

        if (process != nullptr && WaitForSingleObject(process, 0) != WAIT_TIMEOUT)
            return false;

        if (GetTickCount64() >= deadline)
            return false;

        Sleep(kPipePollIntervalMs);
    }
}

bool readExactWithTimeout(HANDLE pipe, HANDLE process, void* destination, DWORD bytesToRead, int timeoutMs)
{
    auto* cursor = static_cast<char*>(destination);
    DWORD totalRead = 0;

    while (totalRead < bytesToRead)
    {
        const auto remaining = bytesToRead - totalRead;
        if (!waitForPipeBytes(pipe, process, remaining, timeoutMs))
            return false;

        DWORD bytesRead = 0;
        if (!ReadFile(pipe, cursor + totalRead, remaining, &bytesRead, nullptr) || bytesRead == 0)
            return false;

        totalRead += bytesRead;
    }

    return true;
}

bool writeExact(HANDLE pipe, const void* source, DWORD bytesToWrite)
{
    const auto* cursor = static_cast<const char*>(source);
    DWORD totalWritten = 0;

    while (totalWritten < bytesToWrite)
    {
        DWORD bytesWritten = 0;
        if (!WriteFile(pipe, cursor + totalWritten, bytesToWrite - totalWritten, &bytesWritten, nullptr) ||
            bytesWritten == 0)
            return false;

        totalWritten += bytesWritten;
    }

    return true;
}
} // namespace
#endif

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
                                    PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_NOWAIT,
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

    if (!waitForScannerConnection(hPipe, static_cast<HANDLE>(scannerProcess), kScannerStartupTimeoutMs))
    {
        spdlog::error("[PluginScannerClient] Scanner failed to connect within {}ms", kScannerStartupTimeoutMs);
        stopScanner();
        return false;
    }

    DWORD pipeMode = PIPE_READMODE_BYTE | PIPE_WAIT;
    if (!SetNamedPipeHandleState(hPipe, &pipeMode, nullptr, nullptr))
    {
        spdlog::error("[PluginScannerClient] Failed to switch scanner pipe to blocking reads: {}", GetLastError());
        stopScanner();
        return false;
    }

    // Wait for Ready message
    MessageHeader header;

    if (!readExactWithTimeout(hPipe, static_cast<HANDLE>(scannerProcess), &header, sizeof(header),
                              kScannerStartupTimeoutMs))
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
        writeExact(hPipe, &header, sizeof(header));

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

    if (!writeExact(hPipe, &header, sizeof(header)))
    {
        spdlog::error("[PluginScannerClient] Failed to send scan request header");
        handleScannerCrash();
        return false;
    }

    if (header.payloadSize > 0)
    {
        auto payloadBytes = payload.toUTF8();
        if (!writeExact(hPipe, payloadBytes.getAddress(), header.payloadSize))
        {
            spdlog::error("[PluginScannerClient] Failed to send scan request payload");
            handleScannerCrash();
            return false;
        }
    }

    // Check if scanner is still running
    if (!isScannerRunning())
    {
        spdlog::error("[PluginScannerClient] Scanner crashed during scan of: {}", pluginPath.toStdString());
        handleScannerCrash();
        return false;
    }

    // Read response header
    if (!readExactWithTimeout(hPipe, static_cast<HANDLE>(scannerProcess), &header, sizeof(header), SCAN_TIMEOUT_MS))
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

    if (header.payloadSize > kMaxScannerPayloadBytes)
    {
        spdlog::error("[PluginScannerClient] Scanner response payload too large: {} bytes", header.payloadSize);
        handleScannerCrash();
        return false;
    }

    // Read response payload
    juce::String responsePayload;
    if (header.payloadSize > 0)
    {
        juce::HeapBlock<char> buffer(header.payloadSize + 1);
        if (!readExactWithTimeout(hPipe, static_cast<HANDLE>(scannerProcess), buffer.get(), header.payloadSize,
                                  SCAN_TIMEOUT_MS))
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
