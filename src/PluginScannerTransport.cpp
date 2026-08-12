/*
  =============================================================================
    PluginScannerTransport.cpp
  =============================================================================
*/

#include "PluginScannerTransport.h"

#include <array>
#include <chrono>
#include <cstddef>

#if JUCE_MAC || JUCE_LINUX
    #include <cerrno>
    #include <cstring>
    #include <poll.h>
    #include <sys/socket.h>
    #include <sys/stat.h>
    #include <sys/un.h>
    #include <unistd.h>
#endif

#ifdef _WIN32
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #include <windows.h>
#endif

using namespace PluginScannerIPC;

namespace
{
using Deadline = std::chrono::steady_clock::time_point;

Deadline makeDeadline(int timeoutMs)
{
    return std::chrono::steady_clock::now() + std::chrono::milliseconds(juce::jmax(0, timeoutMs));
}

int remainingMilliseconds(const Deadline& deadline)
{
    return juce::jmax(0, static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(
        deadline - std::chrono::steady_clock::now()).count()));
}

juce::String makeRandomToken()
{
    auto& random = juce::Random::getSystemRandom();
    return juce::String::toHexString(random.nextInt64()) + juce::String::toHexString(random.nextInt64());
}

#if JUCE_MAC || JUCE_LINUX
bool waitFor(int handle, short events, const Deadline& deadline)
{
    pollfd descriptor { handle, events, 0 };
    return poll(&descriptor, 1, remainingMilliseconds(deadline)) > 0 && (descriptor.revents & events) != 0;
}

bool readAvailable(int handle, std::vector<uint8_t>& buffer, const Deadline& deadline)
{
    if (!waitFor(handle, POLLIN, deadline))
        return false;
    std::array<uint8_t, 4096> chunk {};
    const auto bytes = recv(handle, chunk.data(), chunk.size(), 0);
    if (bytes <= 0)
        return false;
    buffer.insert(buffer.end(), chunk.begin(), chunk.begin() + bytes);
    return true;
}

bool writeExact(int handle, const void* source, size_t bytes, const Deadline& deadline)
{
    const auto* cursor = static_cast<const char*>(source);
    size_t total = 0;
    while (total < bytes)
    {
        if (!waitFor(handle, POLLOUT, deadline))
            return false;
        const auto written = send(handle, cursor + total, bytes - total, 0);
        if (written <= 0)
            return false;
        total += static_cast<size_t>(written);
    }
    return true;
}

juce::String randomSocketName()
{
    return "pb3-" + makeRandomToken() + ".sock";
}
#endif

#ifdef _WIN32
HANDLE toHandle(intptr_t handle) { return reinterpret_cast<HANDLE>(handle); }
intptr_t fromHandle(HANDLE handle) { return reinterpret_cast<intptr_t>(handle); }

juce::String randomPipeName()
{
    auto& random = juce::Random::getSystemRandom();
    return juce::String(PIPE_NAME) + "-" + juce::String::toHexString(random.nextInt64())
        + juce::String::toHexString(random.nextInt64());
}

bool waitForOverlapped(HANDLE pipe, OVERLAPPED& operation, const Deadline& deadline, DWORD& transferred)
{
    const auto result = WaitForSingleObject(operation.hEvent, static_cast<DWORD>(remainingMilliseconds(deadline)));
    if (result == WAIT_OBJECT_0)
        return GetOverlappedResult(pipe, &operation, &transferred, FALSE) != FALSE;

    CancelIoEx(pipe, &operation);
    WaitForSingleObject(operation.hEvent, INFINITE);
    return false;
}

bool startAndWait(HANDLE pipe, bool started, DWORD error, OVERLAPPED& operation,
                  const Deadline& deadline, DWORD& transferred)
{
    if (started)
        return transferred != 0;
    if (error != ERROR_IO_PENDING)
        return false;
    return waitForOverlapped(pipe, operation, deadline, transferred) && transferred != 0;
}

bool readPipeExact(HANDLE pipe, void* destination, DWORD bytes, const Deadline& deadline)
{
    auto* cursor = static_cast<char*>(destination);
    DWORD received = 0;
    while (received < bytes)
    {
        HANDLE event = CreateEventA(nullptr, TRUE, FALSE, nullptr);
        if (event == nullptr)
            return false;
        OVERLAPPED operation {};
        operation.hEvent = event;
        DWORD read = 0;
        const auto started = ReadFile(pipe, cursor + received, bytes - received, &read, &operation) != FALSE;
        const auto error = started ? ERROR_SUCCESS : GetLastError();
        const auto completed = startAndWait(pipe, started, error, operation, deadline, read);
        CloseHandle(event);
        if (!completed)
            return false;
        received += read;
    }
    return true;
}

bool writePipeExact(HANDLE pipe, const void* source, DWORD bytes, const Deadline& deadline)
{
    const auto* cursor = static_cast<const char*>(source);
    DWORD writtenTotal = 0;
    while (writtenTotal < bytes)
    {
        HANDLE event = CreateEventA(nullptr, TRUE, FALSE, nullptr);
        if (event == nullptr)
            return false;
        OVERLAPPED operation {};
        operation.hEvent = event;
        DWORD written = 0;
        const auto started = WriteFile(pipe, cursor + writtenTotal, bytes - writtenTotal, &written, &operation) != FALSE;
        const auto error = started ? ERROR_SUCCESS : GetLastError();
        const auto completed = startAndWait(pipe, started, error, operation, deadline, written);
        CloseHandle(event);
        if (!completed)
            return false;
        writtenTotal += written;
    }
    return true;
}
#endif
} // namespace

ScannerChannel::ScannerChannel(intptr_t nativeHandle) : handle(nativeHandle) {}

ScannerChannel::~ScannerChannel()
{
#if JUCE_MAC || JUCE_LINUX
    if (handle >= 0)
        close(static_cast<int>(handle));
#elif defined(_WIN32)
    if (handle != -1)
        CloseHandle(toHandle(handle));
#endif
}

ScannerChannel::ScannerChannel(ScannerChannel&& other) noexcept
    : handle(other.handle), receiveBuffer(std::move(other.receiveBuffer))
{
    other.handle = -1;
}

ScannerChannel& ScannerChannel::operator=(ScannerChannel&& other) noexcept
{
    if (this != &other)
    {
#if JUCE_MAC || JUCE_LINUX
        if (handle >= 0)
            close(static_cast<int>(handle));
#elif defined(_WIN32)
        if (handle != -1)
            CloseHandle(toHandle(handle));
#endif
        handle = other.handle;
        receiveBuffer = std::move(other.receiveBuffer);
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
    const auto deadline = makeDeadline(5000);
    return writeExact(static_cast<int>(handle), &header, sizeof(header), deadline)
        && (header.payloadSize == 0 || writeExact(static_cast<int>(handle), bytes.getAddress(), header.payloadSize, deadline));
#elif defined(_WIN32)
    if (!isOpen())
        return false;
    const auto bytes = payload.toUTF8();
    if (bytes.length() > static_cast<int>(MAX_SCANNER_PAYLOAD_BYTES))
        return false;
    MessageHeader header;
    header.type = type;
    header.payloadSize = static_cast<uint32_t>(bytes.length());
    const auto deadline = makeDeadline(5000);
    return writePipeExact(toHandle(handle), &header, sizeof(header), deadline)
        && (header.payloadSize == 0 || writePipeExact(toHandle(handle), bytes.getAddress(), header.payloadSize, deadline));
#else
    juce::ignoreUnused(type, payload);
    return false;
#endif
}

bool ScannerChannel::receive(MessageHeader& header, juce::String& payload, int timeoutMs)
{
#if JUCE_MAC || JUCE_LINUX
    if (!isOpen())
        return false;
    const auto deadline = makeDeadline(timeoutMs);
    for (;;)
    {
        if (receiveBuffer.size() >= sizeof(MessageHeader))
        {
            std::memcpy(&header, receiveBuffer.data(), sizeof(header));
            if (header.magic != 0x50444233 || header.version != PROTOCOL_VERSION ||
                header.payloadSize > MAX_SCANNER_PAYLOAD_BYTES)
                return false;
            const auto frameBytes = sizeof(MessageHeader) + static_cast<size_t>(header.payloadSize);
            if (receiveBuffer.size() >= frameBytes)
            {
                payload = juce::String::fromUTF8(reinterpret_cast<const char*>(receiveBuffer.data() + sizeof(MessageHeader)),
                                                  static_cast<int>(header.payloadSize));
                receiveBuffer.erase(receiveBuffer.begin(), receiveBuffer.begin() + static_cast<ptrdiff_t>(frameBytes));
                return true;
            }
        }
        if (!readAvailable(static_cast<int>(handle), receiveBuffer, deadline))
            return false;
    }
#elif defined(_WIN32)
    const auto deadline = makeDeadline(timeoutMs);
    if (!isOpen() || !readPipeExact(toHandle(handle), &header, sizeof(header), deadline)
        || header.magic != 0x50444233 || header.version != PROTOCOL_VERSION
        || header.payloadSize > MAX_SCANNER_PAYLOAD_BYTES)
        return false;
    juce::MemoryBlock data(header.payloadSize);
    if (header.payloadSize > 0 && !readPipeExact(toHandle(handle), data.getData(), header.payloadSize, deadline))
        return false;
    payload = juce::String::fromUTF8(static_cast<const char*>(data.getData()), static_cast<int>(header.payloadSize));
    return true;
#else
    juce::ignoreUnused(header, payload, timeoutMs);
    return false;
#endif
}

bool ScannerChannel::isOpen() const { return handle != -1; }

ScannerHostEndpoint::~ScannerHostEndpoint()
{
#if JUCE_MAC || JUCE_LINUX
    if (listener >= 0)
        close(static_cast<int>(listener));
    if (socketFile.exists())
        socketFile.deleteFile();
#endif
#ifdef _WIN32
    if (listener != -1)
        CloseHandle(toHandle(listener));
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
            close(static_cast<int>(listener));
        if (socketFile.exists())
            socketFile.deleteFile();
#endif
#ifdef _WIN32
        if (listener != -1)
            CloseHandle(toHandle(listener));
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
    socketFile = juce::File("/tmp").getChildFile(randomSocketName());
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
        close(static_cast<int>(listener));
        listener = -1;
        socketFile.deleteFile();
        return false;
    }
    return true;
#else
#if defined(_WIN32)
    info.sessionToken = makeRandomToken();
    info.endpoint = randomPipeName();
    const auto pipe = CreateNamedPipeA(info.endpoint.toRawUTF8(), PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
                                       PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
                                       1, 65536, 65536, 0, nullptr);
    if (pipe == INVALID_HANDLE_VALUE)
        return false;
    listener = fromHandle(pipe);
    return true;
#else
    return false;
#endif
#endif
}

std::unique_ptr<ScannerChannel> ScannerHostEndpoint::accept(int timeoutMs)
{
#if JUCE_MAC || JUCE_LINUX
    if (listener < 0 || !waitFor(static_cast<int>(listener), POLLIN, makeDeadline(timeoutMs)))
        return {};
    const auto accepted = ::accept(static_cast<int>(listener), nullptr, nullptr);
    return accepted >= 0 ? std::unique_ptr<ScannerChannel>(new ScannerChannel(accepted)) : nullptr;
#else
#if defined(_WIN32)
    const auto pipe = toHandle(listener);
    HANDLE event = CreateEventA(nullptr, TRUE, FALSE, nullptr);
    if (event == nullptr)
        return {};
    OVERLAPPED operation {};
    operation.hEvent = event;
    const auto started = ConnectNamedPipe(pipe, &operation) != FALSE;
    const auto error = started ? ERROR_SUCCESS : GetLastError();
    DWORD ignored = 0;
    const auto connected = started || error == ERROR_PIPE_CONNECTED
        || startAndWait(pipe, started, error, operation, makeDeadline(timeoutMs), ignored);
    CloseHandle(event);
    if (!connected)
        return {};
    listener = -1;
    return std::unique_ptr<ScannerChannel>(new ScannerChannel(fromHandle(pipe)));
#else
    juce::ignoreUnused(timeoutMs);
    return {};
#endif
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
#if defined(_WIN32)
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);
    do
    {
        const auto pipe = CreateFileA(launchInfo.endpoint.toRawUTF8(), GENERIC_READ | GENERIC_WRITE,
                                      0, nullptr, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, nullptr);
        if (pipe != INVALID_HANDLE_VALUE)
            return std::unique_ptr<ScannerChannel>(new ScannerChannel(fromHandle(pipe)));
        juce::Thread::sleep(10);
    } while (std::chrono::steady_clock::now() < deadline);
#else
    juce::ignoreUnused(launchInfo, timeoutMs);
#endif
#endif
    return {};
}
