/*
  ==============================================================================

    PluginScannerMain.cpp

    Out-of-process plugin scanner executable.
    Scans plugins in isolation so crashes don't affect the main application.

  ==============================================================================
*/

#include <JuceHeader.h>

#include "../PluginScannerIPC.h"
#include "../PluginScannerTransport.h"
#include "../SupportedPluginFormats.h"

#include <iostream>
#include <deque>
#include <mutex>

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

        registerSupportedPluginFormats(formatManager, PluginFormatRole::scanner);

        std::cerr << "[Scanner] Registered " << formatManager.getNumFormats() << " plugin formats" << std::endl;

        const auto arguments = juce::StringArray::fromTokens(commandLine, true);
        const auto endpointIndex = arguments.indexOf("--scanner-endpoint");
        const auto tokenIndex = arguments.indexOf("--scanner-token");
        if (endpointIndex < 0 || tokenIndex < 0 || endpointIndex + 1 >= arguments.size() || tokenIndex + 1 >= arguments.size())
        {
            std::cerr << "[Scanner] Missing authenticated transport arguments" << std::endl;
            setApplicationReturnValue(1);
            quit();
            return;
        }
        launchInfo.endpoint = arguments[endpointIndex + 1];
        launchInfo.sessionToken = arguments[tokenIndex + 1];

        // Connect to the host's private endpoint.
        if (!connectToHost())
        {
            std::cerr << "[Scanner] Failed to connect to host pipe" << std::endl;
            setApplicationReturnValue(1);
            quit();
            return;
        }

        std::cerr << "[Scanner] Connected to host, entering message loop" << std::endl;

        // Send ready message
        sendMessage(MessageType::Ready, Handshake { PROTOCOL_VERSION, launchInfo.sessionToken }.serialize());

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
    struct PendingScanResponse
    {
        PluginScannerIPC::MessageType type = PluginScannerIPC::MessageType::ScanError;
        juce::String payload;
    };

    juce::AudioPluginFormatManager formatManager;
    juce::ThreadPool scanWorker { 1 };
    ScannerLaunchInfo launchInfo;
    std::unique_ptr<ScannerChannel> channel;
    std::mutex responseMutex;
    std::deque<PendingScanResponse> completedResponses;
    std::atomic<bool> requestActive { false };

    bool connectToHost()
    {
        channel = connectScannerChannel(launchInfo, 5000);
        return channel != nullptr;
    }

    void disconnectFromHost()
    {
        channel.reset();
    }

    bool sendMessage(MessageType type, const juce::String& payload = {})
    {
        return channel != nullptr && channel->send(type, payload);
    }

    bool readMessage(MessageHeader& header, juce::String& payload)
    {
        return channel != nullptr && channel->receive(header, payload, 0);
    }

    void startTimer(int intervalMs) { juce::Timer::callAfterDelay(intervalMs, [this]() { timerCallback(); }); }

    void stopTimer() {}

    void timerCallback()
    {
        drainCompletedResponses();
        processMessages();

        // Schedule next callback if still running
        if (!juce::JUCEApplicationBase::isStandaloneApp() ||
            juce::JUCEApplicationBase::getInstance()->isInitialising())
            return;

        juce::Timer::callAfterDelay(10, [this]() { timerCallback(); });
    }

    void drainCompletedResponses()
    {
        std::deque<PendingScanResponse> responses;
        {
            std::lock_guard<std::mutex> lock(responseMutex);
            responses.swap(completedResponses);
        }

        for (const auto& response : responses)
            sendMessage(response.type, response.payload);
    }

    void queueResponse(PendingScanResponse response)
    {
        std::lock_guard<std::mutex> lock(responseMutex);
        completedResponses.push_back(std::move(response));
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

        if (requestActive.exchange(true))
        {
            ScanResponse response;
            response.resultCode = ScanResultCode::LoadFailed;
            response.errorMessage = "Scanner already has an active request";
            sendMessage(MessageType::ScanError, response.serialize());
            return;
        }

        scanWorker.addJob([this, request]
        {
            PendingScanResponse pending;
            ScanResponse response;

            // The sole worker owns potentially blocking plug-in discovery. The scanner's
            // JUCE message thread remains free for AUv3 completion callbacks.
            juce::AudioPluginFormat* format = nullptr;
            for (int i = 0; i < formatManager.getNumFormats(); ++i)
            {
                auto* candidate = formatManager.getFormat(i);
                if (candidate->getName() == request.formatName)
                {
                    format = candidate;
                    break;
                }
            }

            if (!format)
            {
                response.resultCode = ScanResultCode::InvalidFormat;
                response.errorMessage = "Unknown format: " + request.formatName;
                pending.type = MessageType::ScanError;
            }
            else
            {
                juce::OwnedArray<juce::PluginDescription> results;
                format->findAllTypesForFile(results, request.pluginPath);

                if (results.isEmpty())
                {
                    response.resultCode = ScanResultCode::LoadFailed;
                    response.errorMessage = "No plugins found in file";
                    pending.type = MessageType::ScanError;
                }
                else
                {
                    juce::XmlElement root("PLUGINS");
                    for (auto* desc : results)
                        if (auto xml = desc->createXml())
                            root.addChildElement(xml.release());

                    response.resultCode = ScanResultCode::Success;
                    response.pluginXml = root.toString();
                    pending.type = MessageType::ScanResult;
                }
            }

            pending.payload = response.serialize();
            queueResponse(std::move(pending));
            requestActive.store(false);
        });
    }
};

// Create the application instance
START_JUCE_APPLICATION(PluginScannerApplication)
