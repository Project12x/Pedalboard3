#include "PluginScannerIPC.h"
#include "PluginScannerTransport.h"

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <cerrno>
#include <chrono>
#include <cstring>
#include <fstream>
#include <thread>
#include <vector>

#if JUCE_MAC
    #include <sys/socket.h>
    #include <sys/un.h>
    #include <unistd.h>
#endif

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

TEST_CASE("Windows scanner transport remains named-pipe capable", "[scanner transport][windows-contract]")
{
    std::ifstream transport(PEDALBOARD3_SOURCE_DIR "/src/PluginScannerTransport.cpp");
    REQUIRE(transport.is_open());
    const std::string source(std::istreambuf_iterator<char>(transport), {});
    REQUIRE(source.find("CreateNamedPipeA") != std::string::npos);
    REQUIRE(source.find("CreateFileA") != std::string::npos);
    REQUIRE(source.find("randomPipeName") != std::string::npos);
    REQUIRE(source.find("FILE_FLAG_OVERLAPPED") != std::string::npos);
    REQUIRE(source.find("OVERLAPPED") != std::string::npos);
    REQUIRE(source.find("WaitForSingleObject") != std::string::npos);
    REQUIRE(source.find("CancelIoEx") != std::string::npos);
    REQUIRE(source.find("juce::ignoreUnused(timeoutMs);\n    const auto pipe = toHandle(listener);") == std::string::npos);

    std::ifstream client(PEDALBOARD3_SOURCE_DIR "/src/PluginScannerClient.cpp");
    REQUIRE(client.is_open());
    const std::string clientSource(std::istreambuf_iterator<char>(client), {});
    REQUIRE(clientSource.find("#if JUCE_MAC || defined(_WIN32)") != std::string::npos);
    REQUIRE(clientSource.find("--scanner-endpoint") != std::string::npos);
}

#if JUCE_MAC
namespace
{
void requireUnixDomainSocketSupport()
{
    const auto socketPath = juce::File::getSpecialLocation(juce::File::tempDirectory)
                                .getChildFile("pb3-transport-probe-" + juce::String(static_cast<int>(getpid())) + ".sock");
    const auto descriptor = socket(AF_UNIX, SOCK_STREAM, 0);
    REQUIRE(descriptor >= 0);

    sockaddr_un address {};
    address.sun_family = AF_UNIX;
    std::strncpy(address.sun_path, socketPath.getFullPathName().toRawUTF8(), sizeof(address.sun_path) - 1);
    const auto bound = bind(descriptor, reinterpret_cast<sockaddr*>(&address), sizeof(address));
    const auto bindError = errno;
    close(descriptor);
    socketPath.deleteFile();

    if (bound != 0 && (bindError == EPERM || bindError == EACCES))
        SKIP("the test runner sandbox forbids AF_UNIX sockets");
    REQUIRE(bound == 0);
}
} // namespace

TEST_CASE("macOS endpoint path does not disclose its session token", "[scanner transport]")
{
    requireUnixDomainSocketSupport();
    ScannerHostEndpoint endpoint;
    REQUIRE(endpoint.create());
    const auto launch = endpoint.launchInfo();
    REQUIRE(juce::File(launch.endpoint).getFileName().contains(launch.sessionToken) == false);
}

TEST_CASE("macOS scanner endpoint exchanges authenticated messages and removes its socket", "[scanner transport]")
{
    requireUnixDomainSocketSupport();
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

TEST_CASE("scanner channel receives a fragmented real socket frame", "[scanner transport]")
{
    requireUnixDomainSocketSupport();
    ScannerHostEndpoint endpoint;
    REQUIRE(endpoint.create());
    const auto launch = endpoint.launchInfo();
    const juce::String expectedPayload("fragmented payload");

    std::thread helper([launch, expectedPayload]
    {
        const auto descriptor = socket(AF_UNIX, SOCK_STREAM, 0);
        if (descriptor < 0)
            return;

        sockaddr_un address {};
        address.sun_family = AF_UNIX;
        std::strncpy(address.sun_path, launch.endpoint.toRawUTF8(), sizeof(address.sun_path) - 1);
        if (connect(descriptor, reinterpret_cast<sockaddr*>(&address), sizeof(address)) != 0)
        {
            close(descriptor);
            return;
        }

        MessageHeader header;
        header.type = MessageType::Ready;
        const auto payload = expectedPayload.toUTF8();
        header.payloadSize = static_cast<uint32_t>(payload.length());
        const auto* headerBytes = reinterpret_cast<const char*>(&header);
        send(descriptor, headerBytes, 3, 0);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        send(descriptor, headerBytes + 3, sizeof(header) - 3, 0);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        send(descriptor, payload.getAddress(), 2, 0);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        send(descriptor, payload.getAddress() + 2, payload.length() - 2, 0);
        close(descriptor);
    });

    auto channel = endpoint.accept(1000);
    REQUIRE(channel != nullptr);
    MessageHeader received;
    juce::String payload;
    REQUIRE(channel->receive(received, payload, 500));
    REQUIRE(received.type == MessageType::Ready);
    REQUIRE(payload == expectedPayload);
    helper.join();
}

TEST_CASE("scanner endpoint rejects a wrong session token", "[scanner transport]")
{
    requireUnixDomainSocketSupport();
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
