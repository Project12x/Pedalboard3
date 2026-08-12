/*
  =============================================================================
    PluginScannerTransport.cpp
  =============================================================================
*/

#include "PluginScannerTransport.h"

#if JUCE_MAC || JUCE_LINUX
    #include <cerrno>
    #include <chrono>
    #include <cstring>
    #include <poll.h>
    #include <sys/socket.h>
    #include <sys/stat.h>
    #include <sys/un.h>
    #include <unistd.h>
#endif

using namespace PluginScannerIPC;

namespace
{
#if JUCE_MAC || JUCE_LINUX
bool waitFor(int handle, short events, int timeoutMs)
{
    pollfd descriptor { handle, events, 0 };
    return poll(&descriptor, 1, timeoutMs) > 0 && (descriptor.revents & events) != 0;
}

bool readExact(int handle, void* destination, size_t bytes, int timeoutMs)
{
    auto* cursor = static_cast<char*>(destination);
    size_t total = 0;
    while (total < bytes)
    {
        if (!waitFor(handle, POLLIN, timeoutMs))
            return false;
        const auto readBytes = recv(handle, cursor + total, bytes - total, 0);
        if (readBytes <= 0)
            return false;
        total += static_cast<size_t>(readBytes);
    }
    return true;
}

bool writeExact(int handle, const void* source, size_t bytes, int timeoutMs)
{
    const auto* cursor = static_cast<const char*>(source);
    size_t total = 0;
    while (total < bytes)
    {
        if (!waitFor(handle, POLLOUT, timeoutMs))
            return false;
        const auto written = send(handle, cursor + total, bytes - total, 0);
        if (written <= 0)
            return false;
        total += static_cast<size_t>(written);
    }
    return true;
}

juce::String makeRandomToken()
{
    auto& random = juce::Random::getSystemRandom();
    return juce::String::toHexString(random.nextInt64()) + juce::String::toHexString(random.nextInt64());
}
#endif
} // namespace

ScannerChannel::ScannerChannel(int nativeHandle) : handle(nativeHandle) {}

ScannerChannel::~ScannerChannel()
{
#if JUCE_MAC || JUCE_LINUX
    if (handle >= 0)
        close(handle);
#endif
}

ScannerChannel::ScannerChannel(ScannerChannel&& other) noexcept : handle(other.handle)
{
    other.handle = -1;
}

ScannerChannel& ScannerChannel::operator=(ScannerChannel&& other) noexcept
{
    if (this != &other)
    {
#if JUCE_MAC || JUCE_LINUX
        if (handle >= 0)
            close(handle);
#endif
        handle = other.handle;
        other.handle = -1;
    }
    return *this;
}

bool ScannerChannel::validateFrame(const MessageHeader& header, size_t availablePayloadBytes)
{
    return header.magic == 0x50444233 && header.version == PROTOCOL_VERSION
        && header.payloadSize <= MAX_SCANNER_PAYLOAD_BYTES && availablePayloadBytes >= header.payloadSize;
}

bool ScannerChannel::send(MessageType type, const juce::String& payload)
{
#if JUCE_MAC || JUCE_LINUX
    if (!isOpen())
        return false;
    const auto bytes = payload.toUTF8();
    if (bytes.length() > static_cast<int>(MAX_SCANNER_PAYLOAD_BYTES))
        return false;
    MessageHeader header;
    header.type = type;
    header.payloadSize = static_cast<uint32_t>(bytes.length());
    return writeExact(handle, &header, sizeof(header), 5000)
        && (header.payloadSize == 0 || writeExact(handle, bytes.getAddress(), header.payloadSize, 5000));
#else
    juce::ignoreUnused(type, payload);
    return false;
#endif
}

bool ScannerChannel::receive(MessageHeader& header, juce::String& payload, int timeoutMs)
{
#if JUCE_MAC || JUCE_LINUX
    if (!isOpen() || !readExact(handle, &header, sizeof(header), timeoutMs))
        return false;
    if (header.magic != 0x50444233 || header.version != PROTOCOL_VERSION ||
        header.payloadSize > MAX_SCANNER_PAYLOAD_BYTES)
        return false;
    juce::MemoryBlock data(header.payloadSize);
    if (header.payloadSize > 0 && !readExact(handle, data.getData(), header.payloadSize, timeoutMs))
        return false;
    payload = juce::String::fromUTF8(static_cast<const char*>(data.getData()), static_cast<int>(header.payloadSize));
    return validateFrame(header, header.payloadSize);
#else
    juce::ignoreUnused(header, payload, timeoutMs);
    return false;
#endif
}

bool ScannerChannel::isOpen() const { return handle >= 0; }

ScannerHostEndpoint::~ScannerHostEndpoint()
{
#if JUCE_MAC || JUCE_LINUX
    if (listener >= 0)
        close(listener);
    if (socketFile.exists())
        socketFile.deleteFile();
#endif
}

ScannerHostEndpoint::ScannerHostEndpoint(ScannerHostEndpoint&& other) noexcept
    : listener(other.listener), socketFile(std::move(other.socketFile)), info(std::move(other.info))
{
    other.listener = -1;
}

ScannerHostEndpoint& ScannerHostEndpoint::operator=(ScannerHostEndpoint&& other) noexcept
{
    if (this != &other)
    {
#if JUCE_MAC || JUCE_LINUX
        if (listener >= 0)
            close(listener);
        if (socketFile.exists())
            socketFile.deleteFile();
#endif
        listener = other.listener;
        socketFile = std::move(other.socketFile);
        info = std::move(other.info);
        other.listener = -1;
    }
    return *this;
}

bool ScannerHostEndpoint::create()
{
#if JUCE_MAC || JUCE_LINUX
    if (listener >= 0)
        return true;
    info.sessionToken = makeRandomToken();
    socketFile = juce::File("/tmp").getChildFile("pb3-" + info.sessionToken + ".sock");
    info.endpoint = socketFile.getFullPathName();

    const auto originalMask = umask(0077);
    listener = socket(AF_UNIX, SOCK_STREAM, 0);
    if (listener < 0)
    {
        umask(originalMask);
        return false;
    }
    sockaddr_un address {};
    address.sun_family = AF_UNIX;
    std::strncpy(address.sun_path, info.endpoint.toRawUTF8(), sizeof(address.sun_path) - 1);
    const auto bound = bind(listener, reinterpret_cast<sockaddr*>(&address), sizeof(address)) == 0;
    umask(originalMask);
    if (!bound || chmod(info.endpoint.toRawUTF8(), 0600) != 0 || listen(listener, 1) != 0)
    {
        close(listener);
        listener = -1;
        socketFile.deleteFile();
        return false;
    }
    return true;
#else
    return false;
#endif
}

std::unique_ptr<ScannerChannel> ScannerHostEndpoint::accept(int timeoutMs)
{
#if JUCE_MAC || JUCE_LINUX
    if (listener < 0 || !waitFor(listener, POLLIN, timeoutMs))
        return {};
    const auto accepted = ::accept(listener, nullptr, nullptr);
    return accepted >= 0 ? std::unique_ptr<ScannerChannel>(new ScannerChannel(accepted)) : nullptr;
#else
    juce::ignoreUnused(timeoutMs);
    return {};
#endif
}

std::unique_ptr<ScannerChannel> connectScannerChannel(const ScannerLaunchInfo& launchInfo, int timeoutMs)
{
#if JUCE_MAC || JUCE_LINUX
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);
    do
    {
        const auto socketHandle = socket(AF_UNIX, SOCK_STREAM, 0);
        if (socketHandle < 0)
            return {};
        sockaddr_un address {};
        address.sun_family = AF_UNIX;
        std::strncpy(address.sun_path, launchInfo.endpoint.toRawUTF8(), sizeof(address.sun_path) - 1);
        if (connect(socketHandle, reinterpret_cast<sockaddr*>(&address), sizeof(address)) == 0)
            return std::unique_ptr<ScannerChannel>(new ScannerChannel(socketHandle));
        close(socketHandle);
        juce::Thread::sleep(10);
    } while (std::chrono::steady_clock::now() < deadline);
#else
    juce::ignoreUnused(launchInfo, timeoutMs);
#endif
    return {};
}
