/*
  ==============================================================================

    CrashProtection.h

    Defensive crash protection for plugin operations.
    Provides SEH wrappers, auto-save, and watchdog capabilities.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <atomic>
#include <chrono>
#include <functional>
#include <spdlog/spdlog.h>
#include <thread>


/**
 * @class CrashProtection
 * @brief Provides defensive crash protection for risky plugin operations.
 *
 * Features:
 * - SEH wrappers for catching hardware exceptions on Windows
 * - Auto-save triggers before risky operations
 * - Watchdog thread to detect UI hangs
 * - Crash state logging for diagnostics
 */
class CrashProtection
{
  public:
    /// Get the singleton instance
    static CrashProtection& getInstance();

    /**
     * @brief Execute a function with SEH protection (Windows only).
     * @param operation The function to execute safely
     * @param operationName Name for logging
     * @param pluginName Optional plugin name for crash context
     * @return true if operation completed without exception
     */
    bool executeWithProtection(std::function<void()> operation, const juce::String& operationName,
                               const juce::String& pluginName = {});

    /**
     * @brief Set the current operation context for crash logs.
     * Call before risky operations so crash logs know what was happening.
     */
    void setCurrentOperation(const juce::String& operation, const juce::String& pluginName = {});

    /**
     * @brief Clear the current operation context.
     * Call after operation completes successfully.
     */
    void clearCurrentOperation();

    /**
     * @brief Get the current operation for crash logging.
     */
    juce::String getCurrentOperation() const;

    /**
     * @brief Get the current plugin name for crash logging.
     */
    juce::String getCurrentPluginName() const;

    /**
     * @brief Set auto-save callback to be called before risky operations.
     */
    void setAutoSaveCallback(std::function<void()> callback);

    /**
     * @brief Trigger the auto-save callback.
     */
    void triggerAutoSave();

    /**
     * @brief Start the watchdog thread.
     * @param timeoutMs Timeout in milliseconds before considering UI hung
     */
    void startWatchdog(int timeoutMs = 10000);

    /**
     * @brief Stop the watchdog thread.
     */
    void stopWatchdog();

    /**
     * @brief Ping the watchdog to indicate the UI is responsive.
     * Call this from the message thread periodically.
     */
    void pingWatchdog();

    /**
     * @brief Check if the watchdog detected a hang.
     */
    bool isHangDetected() const { return hangDetected.load(); }

    /**
     * @brief Write crash context to log file.
     * Call this from a crash handler.
     */
    void writeCrashContext();

  private:
    CrashProtection();
    ~CrashProtection();

    CrashProtection(const CrashProtection&) = delete;
    CrashProtection& operator=(const CrashProtection&) = delete;

    void watchdogLoop();

    juce::String currentOperation;
    juce::String currentPluginName;
    mutable juce::CriticalSection operationLock;

    std::function<void()> autoSaveCallback;

    // Watchdog
    std::atomic<bool> watchdogRunning{false};
    std::atomic<bool> hangDetected{false};
    std::atomic<std::chrono::steady_clock::time_point> lastPing;
    std::thread watchdogThread;
    int watchdogTimeoutMs = 10000;
};

/**
 * @brief RAII helper to set/clear operation context.
 */
class ScopedOperationContext
{
  public:
    ScopedOperationContext(const juce::String& operation, const juce::String& pluginName = {})
    {
        CrashProtection::getInstance().setCurrentOperation(operation, pluginName);
    }

    ~ScopedOperationContext() { CrashProtection::getInstance().clearCurrentOperation(); }
};

// Macro for wrapping risky operations
#define PROTECTED_OPERATION(name, plugin, code)                                                                        \
    CrashProtection::getInstance().executeWithProtection([&]() { code; }, name, plugin)
