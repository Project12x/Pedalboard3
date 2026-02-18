/*
  ==============================================================================

    PluginScannerIPC.h

    Inter-process communication protocol for plugin scanning.
    Defines message formats exchanged between host and scanner processes.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

namespace PluginScannerIPC
{

// Pipe name for Windows named pipes
constexpr const char* PIPE_NAME = "\\\\.\\pipe\\Pedalboard3PluginScanner";

// Protocol version for compatibility checking
constexpr int PROTOCOL_VERSION = 1;

// Timeout for scanner operations (ms)
constexpr int SCAN_TIMEOUT_MS = 30000;

// Message types
enum class MessageType : uint8_t
{
    // Host -> Scanner
    Ping = 0,
    ScanPlugin = 1,
    Shutdown = 2,

    // Scanner -> Host
    Pong = 100,
    ScanResult = 101,
    ScanError = 102,
    ScanCrash = 103,
    Ready = 104
};

// Scan result codes
enum class ScanResultCode : uint8_t
{
    Success = 0,
    LoadFailed = 1,
    InvalidFormat = 2,
    Timeout = 3,
    Crashed = 4,
    Blacklisted = 5
};

/**
 * @brief Message header for IPC communication.
 */
struct MessageHeader
{
    uint32_t magic = 0x50444233;  // "PDB3"
    uint8_t version = PROTOCOL_VERSION;
    MessageType type;
    uint32_t payloadSize = 0;
};

/**
 * @brief Scan request sent from host to scanner.
 */
struct ScanRequest
{
    juce::String pluginPath;
    juce::String formatName;  // "VST3", "VST", "AU", etc.

    juce::String serialize() const
    {
        juce::DynamicObject::Ptr obj = new juce::DynamicObject();
        obj->setProperty("path", pluginPath);
        obj->setProperty("format", formatName);
        return juce::JSON::toString(juce::var(obj.get()));
    }

    static ScanRequest deserialize(const juce::String& json)
    {
        ScanRequest req;
        auto parsed = juce::JSON::parse(json);
        if (auto* obj = parsed.getDynamicObject())
        {
            req.pluginPath = obj->getProperty("path").toString();
            req.formatName = obj->getProperty("format").toString();
        }
        return req;
    }
};

/**
 * @brief Scan result sent from scanner to host.
 */
struct ScanResponse
{
    ScanResultCode resultCode = ScanResultCode::LoadFailed;
    juce::String errorMessage;
    juce::String pluginXml;  // Serialized PluginDescription if successful

    juce::String serialize() const
    {
        juce::DynamicObject::Ptr obj = new juce::DynamicObject();
        obj->setProperty("code", static_cast<int>(resultCode));
        obj->setProperty("error", errorMessage);
        obj->setProperty("xml", pluginXml);
        return juce::JSON::toString(juce::var(obj.get()));
    }

    static ScanResponse deserialize(const juce::String& json)
    {
        ScanResponse resp;
        auto parsed = juce::JSON::parse(json);
        if (auto* obj = parsed.getDynamicObject())
        {
            resp.resultCode = static_cast<ScanResultCode>(static_cast<int>(obj->getProperty("code")));
            resp.errorMessage = obj->getProperty("error").toString();
            resp.pluginXml = obj->getProperty("xml").toString();
        }
        return resp;
    }
};

/**
 * @brief Helper to write a message to a pipe/stream.
 */
inline bool writeMessage(juce::OutputStream& stream, MessageType type, const juce::String& payload = {})
{
    MessageHeader header;
    header.type = type;
    auto payloadBytes = payload.toUTF8();
    header.payloadSize = static_cast<uint32_t>(payloadBytes.length());

    if (!stream.write(&header, sizeof(header)))
        return false;

    if (header.payloadSize > 0)
    {
        if (!stream.write(payloadBytes.getAddress(), header.payloadSize))
            return false;
    }

    stream.flush();
    return true;
}

/**
 * @brief Helper to read a message from a pipe/stream.
 */
inline bool readMessage(juce::InputStream& stream, MessageHeader& header, juce::String& payload)
{
    if (stream.read(&header, static_cast<int>(sizeof(header))) != static_cast<int>(sizeof(header)))
        return false;

    if (header.magic != 0x50444233)
        return false;

    if (header.payloadSize > 0)
    {
        juce::MemoryBlock block(header.payloadSize);
        if (stream.read(block.getData(), static_cast<int>(header.payloadSize)) != static_cast<int>(header.payloadSize))
            return false;
        payload = juce::String::fromUTF8(static_cast<const char*>(block.getData()), static_cast<int>(header.payloadSize));
    }
    else
    {
        payload.clear();
    }

    return true;
}

}  // namespace PluginScannerIPC
