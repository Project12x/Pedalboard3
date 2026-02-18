/*
  ==============================================================================

    PluginScannerMain.cpp

    Out-of-process plugin scanner executable.
    Scans plugins in isolation so crashes don't affect the main application.

  ==============================================================================
*/

#include <JuceHeader.h>

#include "../PluginScannerIPC.h"

#include <iostream>

#ifdef _WIN32
    #include <windows.h>
#endif

using namespace PluginScannerIPC;

class PluginScannerApplication : public juce::JUCEApplicationBase
{
  public:
    PluginScannerApplication() = default;

    const juce::String getApplicationName() override { return "Pedalboard3Scanner"; }
    const juce::String getApplicationVersion() override { return "1.0.0"; }
    bool moreThanOneInstanceAllowed() override { return true; }

    void suspended() override {}
    void resumed() override {}
    void unhandledException(const std::exception*, const juce::String&, int) override
    {
        std::cerr << "[Scanner] Unhandled exception caught" << std::endl;
    }

    void initialise(const juce::String& commandLine) override
    {
        std::cerr << "[Scanner] Starting Pedalboard3 Plugin Scanner" << std::endl;

        // Initialize plugin formats - JUCE 8 requires explicit format registration
        formatManager.addFormat(std::make_unique<juce::VST3PluginFormat>());

        std::cerr << "[Scanner] Registered " << formatManager.getNumFormats() << " plugin formats" << std::endl;

        // Connect to the host's named pipe
        if (!connectToHost())
        {
            std::cerr << "[Scanner] Failed to connect to host pipe" << std::endl;
            setApplicationReturnValue(1);
            quit();
            return;
        }

        std::cerr << "[Scanner] Connected to host, entering message loop" << std::endl;

        // Send ready message
        sendMessage(MessageType::Ready);

        // Start message processing
        startTimer(10);
    }

    void shutdown() override
    {
        std::cerr << "[Scanner] Shutting down" << std::endl;
        stopTimer();
        disconnectFromHost();
    }

    void systemRequestedQuit() override { quit(); }

    void anotherInstanceStarted(const juce::String&) override {}

  private:
    juce::AudioPluginFormatManager formatManager;

#ifdef _WIN32
    HANDLE pipeHandle = INVALID_HANDLE_VALUE;
#endif

    bool connectToHost()
    {
#ifdef _WIN32
        // Try to connect to the named pipe created by the host
        for (int attempt = 0; attempt < 10; ++attempt)
        {
            pipeHandle = CreateFileA(PIPE_NAME, GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);

            if (pipeHandle != INVALID_HANDLE_VALUE)
            {
                // Set pipe to message mode
                DWORD mode = PIPE_READMODE_BYTE;
                SetNamedPipeHandleState(pipeHandle, &mode, nullptr, nullptr);
                return true;
            }

            // Wait a bit before retry
            if (GetLastError() == ERROR_PIPE_BUSY)
            {
                WaitNamedPipeA(PIPE_NAME, 1000);
            }
            else
            {
                Sleep(100);
            }
        }
        return false;
#else
        // Unix implementation would use Unix domain sockets
        return false;
#endif
    }

    void disconnectFromHost()
    {
#ifdef _WIN32
        if (pipeHandle != INVALID_HANDLE_VALUE)
        {
            CloseHandle(pipeHandle);
            pipeHandle = INVALID_HANDLE_VALUE;
        }
#endif
    }

    bool sendMessage(MessageType type, const juce::String& payload = {})
    {
#ifdef _WIN32
        if (pipeHandle == INVALID_HANDLE_VALUE)
            return false;

        MessageHeader header;
        header.type = type;
        auto payloadBytes = payload.toUTF8();
        header.payloadSize = static_cast<uint32_t>(payloadBytes.length());

        DWORD bytesWritten;
        if (!WriteFile(pipeHandle, &header, sizeof(header), &bytesWritten, nullptr))
            return false;

        if (header.payloadSize > 0)
        {
            if (!WriteFile(pipeHandle, payloadBytes.getAddress(), header.payloadSize, &bytesWritten, nullptr))
                return false;
        }

        FlushFileBuffers(pipeHandle);
        return true;
#else
        return false;
#endif
    }

    bool readMessage(MessageHeader& header, juce::String& payload)
    {
#ifdef _WIN32
        if (pipeHandle == INVALID_HANDLE_VALUE)
            return false;

        // Check if data is available
        DWORD bytesAvailable = 0;
        if (!PeekNamedPipe(pipeHandle, nullptr, 0, nullptr, &bytesAvailable, nullptr))
            return false;

        if (bytesAvailable < sizeof(MessageHeader))
            return false;

        DWORD bytesRead;
        if (!ReadFile(pipeHandle, &header, sizeof(header), &bytesRead, nullptr))
            return false;

        if (header.magic != 0x50444233)
            return false;

        if (header.payloadSize > 0)
        {
            juce::HeapBlock<char> buffer(header.payloadSize + 1);
            if (!ReadFile(pipeHandle, buffer.get(), header.payloadSize, &bytesRead, nullptr))
                return false;
            buffer[header.payloadSize] = 0;
            payload = juce::String::fromUTF8(buffer.get(), header.payloadSize);
        }
        else
        {
            payload.clear();
        }

        return true;
#else
        return false;
#endif
    }

    void startTimer(int intervalMs) { juce::Timer::callAfterDelay(intervalMs, [this]() { timerCallback(); }); }

    void stopTimer() {}

    void timerCallback()
    {
        processMessages();

        // Schedule next callback if still running
        if (!juce::JUCEApplicationBase::isStandaloneApp() ||
            juce::JUCEApplicationBase::getInstance()->isInitialising())
            return;

        juce::Timer::callAfterDelay(10, [this]() { timerCallback(); });
    }

    void processMessages()
    {
        MessageHeader header;
        juce::String payload;

        while (readMessage(header, payload))
        {
            switch (header.type)
            {
            case MessageType::Ping:
                sendMessage(MessageType::Pong);
                break;

            case MessageType::ScanPlugin:
                handleScanRequest(payload);
                break;

            case MessageType::Shutdown:
                std::cerr << "[Scanner] Received shutdown command" << std::endl;
                quit();
                return;

            default:
                std::cerr << "[Scanner] Unknown message type: " << static_cast<int>(header.type) << std::endl;
                break;
            }
        }
    }

    void handleScanRequest(const juce::String& payload)
    {
        auto request = ScanRequest::deserialize(payload);
        std::cerr << "[Scanner] Scanning: " << request.pluginPath.toStdString() << std::endl;

        ScanResponse response;

        // Find the appropriate format
        juce::AudioPluginFormat* format = nullptr;
        for (int i = 0; i < formatManager.getNumFormats(); ++i)
        {
            auto* f = formatManager.getFormat(i);
            if (f->getName() == request.formatName)
            {
                format = f;
                break;
            }
        }

        if (!format)
        {
            response.resultCode = ScanResultCode::InvalidFormat;
            response.errorMessage = "Unknown format: " + request.formatName;
            sendMessage(MessageType::ScanError, response.serialize());
            return;
        }

        // Try to scan the plugin
        juce::OwnedArray<juce::PluginDescription> results;
        format->findAllTypesForFile(results, request.pluginPath);

        if (results.isEmpty())
        {
            response.resultCode = ScanResultCode::LoadFailed;
            response.errorMessage = "No plugins found in file";
            sendMessage(MessageType::ScanError, response.serialize());
            return;
        }

        // Serialize all found plugins to XML
        juce::XmlElement root("PLUGINS");
        for (auto* desc : results)
        {
            if (auto xml = desc->createXml())
                root.addChildElement(xml.release());
        }

        response.resultCode = ScanResultCode::Success;
        response.pluginXml = root.toString();

        std::cerr << "[Scanner] Found " << results.size() << " plugin(s)" << std::endl;

        sendMessage(MessageType::ScanResult, response.serialize());
    }
};

// Create the application instance
START_JUCE_APPLICATION(PluginScannerApplication)
