#include "PluginScannerIPC.h"
#include "PluginScannerTransport.h"

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <thread>

using namespace PluginScannerIPC;

TEST_CASE("scanner protocol rejects invalid framing before allocation", "[scanner protocol]")
{
    MessageHeader header;
    auto malformed = header;
    malformed.magic = 0;
    REQUIRE_FALSE(ScannerChannel::validateFrame(malformed, 0));

    malformed = header;
    malformed.version = PROTOCOL_VERSION - 1;
    REQUIRE_FALSE(ScannerChannel::validateFrame(malformed, 0));

    malformed = header;
    malformed.payloadSize = MAX_SCANNER_PAYLOAD_BYTES + 1;
    REQUIRE_FALSE(ScannerChannel::validateFrame(malformed, 0));

    malformed = header;
    malformed.payloadSize = 1;
    REQUIRE_FALSE(ScannerChannel::validateFrame(malformed, 0));
}

#if JUCE_MAC
TEST_CASE("macOS scanner endpoint exchanges authenticated messages and removes its socket", "[scanner transport]")
{
    juce::File socketFile;
    {
        ScannerHostEndpoint endpoint;
        REQUIRE(endpoint.create());
        const auto launch = endpoint.launchInfo();
        REQUIRE(launch.endpoint.isNotEmpty());
        REQUIRE(launch.sessionToken.isNotEmpty());
        socketFile = juce::File(launch.endpoint);
        REQUIRE(socketFile.exists());

        bool helperSucceeded = false;
        std::thread helper([launch, &helperSucceeded]
        {
            auto channel = connectScannerChannel(launch, 1000);
            if (channel == nullptr)
                return;
            if (!channel->send(MessageType::Ready, Handshake { PROTOCOL_VERSION, launch.sessionToken }.serialize()))
                return;

            MessageHeader request;
            juce::String payload;
            helperSucceeded = channel->receive(request, payload, 1000) && request.type == MessageType::Ping
                && channel->send(MessageType::Pong);
        });

        auto channel = endpoint.accept(1000);
        REQUIRE(channel != nullptr);
        MessageHeader ready;
        juce::String readyPayload;
        REQUIRE(channel->receive(ready, readyPayload, 1000));
        REQUIRE(ready.type == MessageType::Ready);
        REQUIRE(Handshake::deserialize(readyPayload).sessionToken == launch.sessionToken);
        REQUIRE(channel->send(MessageType::Ping));

        MessageHeader pong;
        juce::String pongPayload;
        REQUIRE(channel->receive(pong, pongPayload, 1000));
        REQUIRE(pong.type == MessageType::Pong);
        helper.join();
        REQUIRE(helperSucceeded);
    }
    REQUIRE_FALSE(socketFile.exists());
}

TEST_CASE("scanner endpoint rejects a wrong session token", "[scanner transport]")
{
    ScannerHostEndpoint endpoint;
    REQUIRE(endpoint.create());
    auto wrongLaunch = endpoint.launchInfo();
    wrongLaunch.sessionToken = "not-the-host-token";

    bool helperSucceeded = false;
    std::thread helper([wrongLaunch, &helperSucceeded]
    {
        auto channel = connectScannerChannel(wrongLaunch, 1000);
        helperSucceeded = channel != nullptr
            && channel->send(MessageType::Ready, Handshake { PROTOCOL_VERSION, wrongLaunch.sessionToken }.serialize());
    });

    auto channel = endpoint.accept(1000);
    REQUIRE(channel != nullptr);
    MessageHeader ready;
    juce::String payload;
    REQUIRE(channel->receive(ready, payload, 1000));
    REQUIRE_FALSE(Handshake::deserialize(payload).matches(endpoint.launchInfo()));
    helper.join();
    REQUIRE(helperSucceeded);
}
#endif
