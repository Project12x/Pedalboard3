/*
  =============================================================================
    PluginScannerTransport.h
  =============================================================================
*/

#pragma once

#include "PluginScannerIPC.h"

#include <memory>

class ScannerChannel
{
  public:
    ScannerChannel() = default;
    ~ScannerChannel();
    ScannerChannel(ScannerChannel&&) noexcept;
    ScannerChannel& operator=(ScannerChannel&&) noexcept;

    bool send(PluginScannerIPC::MessageType type, const juce::String& payload = {});
    bool receive(PluginScannerIPC::MessageHeader& header, juce::String& payload, int timeoutMs);
    bool isOpen() const;

    static bool validateFrame(const PluginScannerIPC::MessageHeader& header, size_t availablePayloadBytes);

  private:
    friend class ScannerHostEndpoint;
    friend std::unique_ptr<ScannerChannel> connectScannerChannel(const PluginScannerIPC::ScannerLaunchInfo&, int);
    explicit ScannerChannel(int nativeHandle);
    int handle = -1;

    JUCE_DECLARE_NON_COPYABLE(ScannerChannel)
};

class ScannerHostEndpoint
{
  public:
    ScannerHostEndpoint() = default;
    ~ScannerHostEndpoint();
    ScannerHostEndpoint(ScannerHostEndpoint&&) noexcept;
    ScannerHostEndpoint& operator=(ScannerHostEndpoint&&) noexcept;

    bool create();
    PluginScannerIPC::ScannerLaunchInfo launchInfo() const { return info; }
    std::unique_ptr<ScannerChannel> accept(int timeoutMs);

  private:
    int listener = -1;
    juce::File socketFile;
    PluginScannerIPC::ScannerLaunchInfo info;

    JUCE_DECLARE_NON_COPYABLE(ScannerHostEndpoint)
};

std::unique_ptr<ScannerChannel> connectScannerChannel(const PluginScannerIPC::ScannerLaunchInfo& launchInfo,
                                                       int timeoutMs);
