/*
  ==============================================================================

    CrashProtection.cpp

    Implementation of defensive crash protection for plugin operations.

  ==============================================================================
*/

#include "CrashProtection.h"

#include <spdlog/spdlog.h>

#ifdef _WIN32
    #include <excpt.h>
    #include <windows.h>

#endif

//------------------------------------------------------------------------------
CrashProtection& CrashProtection::getInstance()
{
    static CrashProtection instance;
    return instance;
}

//------------------------------------------------------------------------------
CrashProtection::CrashProtection()
{
    lastPing.store(std::chrono::steady_clock::now());
}

//------------------------------------------------------------------------------
CrashProtection::~CrashProtection()
{
    stopWatchdog();
}

//------------------------------------------------------------------------------
#ifdef _WIN32
// Windows-specific exception filter for SEH
static int exceptionFilter(unsigned int code, struct _EXCEPTION_POINTERS* ep)
{
    // Log the exception type
    const char* exceptionName = "Unknown";
    switch (code)
    {
    case EXCEPTION_ACCESS_VIOLATION:
        exceptionName = "Access Violation";
        break;
    case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
        exceptionName = "Array Bounds Exceeded";
        break;
    case EXCEPTION_DATATYPE_MISALIGNMENT:
        exceptionName = "Datatype Misalignment";
        break;
    case EXCEPTION_FLT_DIVIDE_BY_ZERO:
        exceptionName = "Float Divide by Zero";
        break;
    case EXCEPTION_FLT_OVERFLOW:
        exceptionName = "Float Overflow";
        break;
    case EXCEPTION_ILLEGAL_INSTRUCTION:
        exceptionName = "Illegal Instruction";
        break;
    case EXCEPTION_INT_DIVIDE_BY_ZERO:
        exceptionName = "Integer Divide by Zero";
        break;
    case EXCEPTION_INT_OVERFLOW:
        exceptionName = "Integer Overflow";
        break;
    case EXCEPTION_STACK_OVERFLOW:
        exceptionName = "Stack Overflow";
        break;
    case EXCEPTION_PRIV_INSTRUCTION:
        exceptionName = "Privileged Instruction";
        break;
    }

    spdlog::critical("[CrashProtection] SEH caught exception: {} (0x{:08X})", exceptionName, code);

    if (ep && ep->ContextRecord)
    {
    #ifdef _M_X64
        spdlog::critical("[CrashProtection] RIP: 0x{:016X}", ep->ContextRecord->Rip);
    #else
        spdlog::critical("[CrashProtection] EIP: 0x{:08X}", ep->ContextRecord->Eip);
    #endif
    }

    return EXCEPTION_EXECUTE_HANDLER;
}
#endif

//------------------------------------------------------------------------------
#ifdef _WIN32
// Separate function for SEH block - must be in a function with no C++ objects
// that require unwinding
typedef void (*RawOperationFunc)(void* context);

static bool executeSEHBlock(RawOperationFunc func, void* context)
{
    bool success = false;
    __try
    {
        func(context);
        success = true;
    }
    __except (exceptionFilter(GetExceptionCode(), GetExceptionInformation()))
    {
        success = false;
    }
    return success;
}
#endif

//------------------------------------------------------------------------------
bool CrashProtection::executeWithProtection(std::function<void()> operation, const juce::String& operationName,
                                            const juce::String& pluginName)
{
    setCurrentOperation(operationName, pluginName);
    triggerAutoSave();

    spdlog::debug("[CrashProtection] Starting protected operation: {} (plugin: {})", operationName.toStdString(),
                  pluginName.isEmpty() ? "none" : pluginName.toStdString());

    bool success = false;

#ifdef _WIN32
    // Use the separate SEH wrapper function to avoid C2712
    auto wrapper = [](void* ctx)
    {
        auto* op = static_cast<std::function<void()>*>(ctx);
        (*op)();
    };

    success = executeSEHBlock(wrapper, &operation);

    if (!success)
    {
        spdlog::critical("[CrashProtection] Operation failed with SEH exception: {}", operationName.toStdString());
        writeCrashContext();
    }
#else
    // On non-Windows, just run the operation directly
    // Could add signal handlers here for Unix
    try
    {
        operation();
        success = true;
    }
    catch (const std::exception& e)
    {
        spdlog::critical("[CrashProtection] Operation failed with exception: {} - {}", operationName.toStdString(),
                         e.what());
        writeCrashContext();
        success = false;
    }
    catch (...)
    {
        spdlog::critical("[CrashProtection] Operation failed with unknown exception: {}", operationName.toStdString());
        writeCrashContext();
        success = false;
    }
#endif

    clearCurrentOperation();
    return success;
}

//------------------------------------------------------------------------------
void CrashProtection::setCurrentOperation(const juce::String& operation, const juce::String& pluginName)
{
    juce::ScopedLock lock(operationLock);
    currentOperation = operation;
    currentPluginName = pluginName;
}

//------------------------------------------------------------------------------
void CrashProtection::clearCurrentOperation()
{
    juce::ScopedLock lock(operationLock);
    currentOperation.clear();
    currentPluginName.clear();
}

//------------------------------------------------------------------------------
juce::String CrashProtection::getCurrentOperation() const
{
    juce::ScopedLock lock(operationLock);
    return currentOperation;
}

//------------------------------------------------------------------------------
juce::String CrashProtection::getCurrentPluginName() const
{
    juce::ScopedLock lock(operationLock);
    return currentPluginName;
}

//------------------------------------------------------------------------------
void CrashProtection::setAutoSaveCallback(std::function<void()> callback)
{
    autoSaveCallback = std::move(callback);
}

//------------------------------------------------------------------------------
void CrashProtection::triggerAutoSave()
{
    if (autoSaveCallback)
    {
        spdlog::debug("[CrashProtection] Triggering auto-save before risky operation");
        try
        {
            autoSaveCallback();
        }
        catch (const std::exception& e)
        {
            spdlog::warn("[CrashProtection] Auto-save failed: {}", e.what());
        }
    }
}

//------------------------------------------------------------------------------
void CrashProtection::startWatchdog(int timeoutMs)
{
    if (watchdogRunning.load())
        return;

    watchdogTimeoutMs = timeoutMs;
    watchdogRunning.store(true);
    hangDetected.store(false);
    lastPing.store(std::chrono::steady_clock::now());

    watchdogThread = std::thread(&CrashProtection::watchdogLoop, this);
    spdlog::info("[CrashProtection] Watchdog started with {}ms timeout", timeoutMs);
}

//------------------------------------------------------------------------------
void CrashProtection::stopWatchdog()
{
    if (!watchdogRunning.load())
        return;

    watchdogRunning.store(false);

    if (watchdogThread.joinable())
        watchdogThread.join();

    spdlog::info("[CrashProtection] Watchdog stopped");
}

//------------------------------------------------------------------------------
void CrashProtection::pingWatchdog()
{
    lastPing.store(std::chrono::steady_clock::now());
}

//------------------------------------------------------------------------------
void CrashProtection::watchdogLoop()
{
    while (watchdogRunning.load())
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));

        auto now = std::chrono::steady_clock::now();
        auto lastPingTime = lastPing.load();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastPingTime).count();

        if (elapsed > watchdogTimeoutMs)
        {
            if (!hangDetected.load())
            {
                hangDetected.store(true);
                spdlog::critical("[CrashProtection] WATCHDOG: UI thread appears hung! No ping for {}ms", elapsed);
                spdlog::critical("[CrashProtection] Last operation: {} (plugin: {})",
                                 getCurrentOperation().toStdString(), getCurrentPluginName().toStdString());
                writeCrashContext();
            }
        }
        else if (hangDetected.load())
        {
            // Recovered from hang
            hangDetected.store(false);
            spdlog::warn("[CrashProtection] WATCHDOG: UI thread recovered after hang");
        }
    }
}

//------------------------------------------------------------------------------
void CrashProtection::writeCrashContext()
{
    juce::String op = getCurrentOperation();
    juce::String plugin = getCurrentPluginName();

    spdlog::critical("[CrashProtection] === CRASH CONTEXT ===");
    spdlog::critical("[CrashProtection] Operation: {}", op.isEmpty() ? "(none)" : op.toStdString());
    spdlog::critical("[CrashProtection] Plugin: {}", plugin.isEmpty() ? "(none)" : plugin.toStdString());
    spdlog::critical("[CrashProtection] Timestamp: {}",
                     juce::Time::getCurrentTime().toString(true, true, true, true).toStdString());
    spdlog::critical("[CrashProtection] =====================");

    // Force flush
    spdlog::default_logger()->flush();
}
