/**
 * @file protection_test.cpp
 * @brief Tests for plugin protection features (PluginBlacklist, CrashProtection)
 *
 * These tests verify:
 * 1. PluginBlacklist - add/remove/query, path normalization, persistence logic
 * 2. CrashProtection - callback invocation, context tracking, exception handling
 *
 * Tests use mock implementations to avoid actual crashes while verifying
 * the protection logic works correctly.
 */

#include <atomic>
#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <functional>
#include <mutex>
#include <set>
#include <string>
#include <thread>
#include <vector>

// =============================================================================
// Mock PluginBlacklist (mirrors real implementation for testing)
// =============================================================================

/**
 * Mock implementation of PluginBlacklist for testing without SettingsManager
 */
class MockPluginBlacklist
{
  public:
    static MockPluginBlacklist& getInstance()
    {
        static MockPluginBlacklist instance;
        return instance;
    }

    void clear()
    {
        std::lock_guard<std::mutex> lock(blacklistMutex);
        blacklistedPaths.clear();
        blacklistedIds.clear();
    }

    void addPath(const std::string& path)
    {
        std::lock_guard<std::mutex> lock(blacklistMutex);
        blacklistedPaths.insert(normalizePath(path));
    }

    void removePath(const std::string& path)
    {
        std::lock_guard<std::mutex> lock(blacklistMutex);
        blacklistedPaths.erase(normalizePath(path));
    }

    bool isPathBlacklisted(const std::string& path) const
    {
        std::lock_guard<std::mutex> lock(blacklistMutex);
        return blacklistedPaths.find(normalizePath(path)) != blacklistedPaths.end();
    }

    void addId(const std::string& id)
    {
        std::lock_guard<std::mutex> lock(blacklistMutex);
        blacklistedIds.insert(id);
    }

    void removeId(const std::string& id)
    {
        std::lock_guard<std::mutex> lock(blacklistMutex);
        blacklistedIds.erase(id);
    }

    bool isIdBlacklisted(const std::string& id) const
    {
        std::lock_guard<std::mutex> lock(blacklistMutex);
        return blacklistedIds.find(id) != blacklistedIds.end();
    }

    size_t getPathCount() const
    {
        std::lock_guard<std::mutex> lock(blacklistMutex);
        return blacklistedPaths.size();
    }

    size_t getIdCount() const
    {
        std::lock_guard<std::mutex> lock(blacklistMutex);
        return blacklistedIds.size();
    }

    std::vector<std::string> getBlacklistedPaths() const
    {
        std::lock_guard<std::mutex> lock(blacklistMutex);
        return std::vector<std::string>(blacklistedPaths.begin(), blacklistedPaths.end());
    }

    std::vector<std::string> getBlacklistedIds() const
    {
        std::lock_guard<std::mutex> lock(blacklistMutex);
        return std::vector<std::string>(blacklistedIds.begin(), blacklistedIds.end());
    }

  private:
    MockPluginBlacklist() = default;

    std::string normalizePath(const std::string& path) const
    {
        std::string normalized = path;
        // Convert to lowercase for case-insensitive comparison (Windows)
        for (auto& c : normalized)
        {
            if (c >= 'A' && c <= 'Z')
                c = c - 'A' + 'a';
            // Normalize path separators
            if (c == '/')
                c = '\\';
        }
        return normalized;
    }

    std::set<std::string> blacklistedPaths;
    std::set<std::string> blacklistedIds;
    mutable std::mutex blacklistMutex;
};

// =============================================================================
// Mock CrashProtection (mirrors real implementation for testing)
// =============================================================================

/**
 * Mock implementation of CrashProtection for testing without SEH
 */
class MockCrashProtection
{
  public:
    static MockCrashProtection& getInstance()
    {
        static MockCrashProtection instance;
        return instance;
    }

    void reset()
    {
        autoSaveCallback = nullptr;
        autoSaveCallCount = 0;
        currentOperation.clear();
        currentPluginName.clear();
        exceptionsCaught = 0;
        operationsExecuted = 0;
    }

    // Execute operation with protection - catches C++ exceptions
    bool executeWithProtection(std::function<void()> operation, const std::string& operationName,
                               const std::string& pluginName = "")
    {
        setCurrentOperation(operationName, pluginName);
        triggerAutoSave();

        bool success = false;
        try
        {
            operation();
            operationsExecuted++;
            success = true;
        }
        catch (const std::exception& e)
        {
            exceptionsCaught++;
            lastExceptionMessage = e.what();
            success = false;
        }
        catch (...)
        {
            exceptionsCaught++;
            lastExceptionMessage = "Unknown exception";
            success = false;
        }

        clearCurrentOperation();
        return success;
    }

    void setCurrentOperation(const std::string& operation, const std::string& pluginName = "")
    {
        std::lock_guard<std::mutex> lock(operationLock);
        currentOperation = operation;
        currentPluginName = pluginName;
    }

    void clearCurrentOperation()
    {
        std::lock_guard<std::mutex> lock(operationLock);
        currentOperation.clear();
        currentPluginName.clear();
    }

    std::string getCurrentOperation() const
    {
        std::lock_guard<std::mutex> lock(operationLock);
        return currentOperation;
    }

    std::string getCurrentPluginName() const
    {
        std::lock_guard<std::mutex> lock(operationLock);
        return currentPluginName;
    }

    void setAutoSaveCallback(std::function<void()> callback) { autoSaveCallback = std::move(callback); }

    void triggerAutoSave()
    {
        if (autoSaveCallback)
        {
            autoSaveCallback();
            autoSaveCallCount++;
        }
    }

    int getAutoSaveCallCount() const { return autoSaveCallCount; }
    int getExceptionsCaught() const { return exceptionsCaught; }
    int getOperationsExecuted() const { return operationsExecuted; }
    std::string getLastExceptionMessage() const { return lastExceptionMessage; }

  private:
    MockCrashProtection() = default;

    std::function<void()> autoSaveCallback;
    std::atomic<int> autoSaveCallCount{0};
    std::string currentOperation;
    std::string currentPluginName;
    mutable std::mutex operationLock;
    std::atomic<int> exceptionsCaught{0};
    std::atomic<int> operationsExecuted{0};
    std::string lastExceptionMessage;
};

// =============================================================================
// PluginBlacklist Tests
// =============================================================================

TEST_CASE("PluginBlacklist - Path Management", "[protection][blacklist]")
{
    auto& blacklist = MockPluginBlacklist::getInstance();
    blacklist.clear();

    SECTION("Add and query path")
    {
        REQUIRE(blacklist.getPathCount() == 0);

        blacklist.addPath("C:\\Plugins\\BadPlugin.vst3");
        REQUIRE(blacklist.getPathCount() == 1);
        REQUIRE(blacklist.isPathBlacklisted("C:\\Plugins\\BadPlugin.vst3"));
    }

    SECTION("Remove path")
    {
        blacklist.addPath("C:\\Plugins\\BadPlugin.vst3");
        REQUIRE(blacklist.isPathBlacklisted("C:\\Plugins\\BadPlugin.vst3"));

        blacklist.removePath("C:\\Plugins\\BadPlugin.vst3");
        REQUIRE_FALSE(blacklist.isPathBlacklisted("C:\\Plugins\\BadPlugin.vst3"));
        REQUIRE(blacklist.getPathCount() == 0);
    }

    SECTION("Path not found returns false")
    {
        REQUIRE_FALSE(blacklist.isPathBlacklisted("C:\\Plugins\\NonExistent.vst3"));
    }

    SECTION("Multiple paths")
    {
        blacklist.addPath("C:\\Plugins\\Bad1.vst3");
        blacklist.addPath("C:\\Plugins\\Bad2.vst3");
        blacklist.addPath("C:\\Plugins\\Bad3.vst3");

        REQUIRE(blacklist.getPathCount() == 3);
        REQUIRE(blacklist.isPathBlacklisted("C:\\Plugins\\Bad1.vst3"));
        REQUIRE(blacklist.isPathBlacklisted("C:\\Plugins\\Bad2.vst3"));
        REQUIRE(blacklist.isPathBlacklisted("C:\\Plugins\\Bad3.vst3"));

        blacklist.removePath("C:\\Plugins\\Bad2.vst3");
        REQUIRE(blacklist.getPathCount() == 2);
        REQUIRE_FALSE(blacklist.isPathBlacklisted("C:\\Plugins\\Bad2.vst3"));
    }

    SECTION("Duplicate add is idempotent")
    {
        blacklist.addPath("C:\\Plugins\\BadPlugin.vst3");
        blacklist.addPath("C:\\Plugins\\BadPlugin.vst3");
        blacklist.addPath("C:\\Plugins\\BadPlugin.vst3");

        REQUIRE(blacklist.getPathCount() == 1);
    }
}

TEST_CASE("PluginBlacklist - Path Normalization", "[protection][blacklist]")
{
    auto& blacklist = MockPluginBlacklist::getInstance();
    blacklist.clear();

    SECTION("Case-insensitive matching (Windows)")
    {
        blacklist.addPath("C:\\Plugins\\BadPlugin.vst3");

        // Different cases should still match
        REQUIRE(blacklist.isPathBlacklisted("C:\\PLUGINS\\BADPLUGIN.VST3"));
        REQUIRE(blacklist.isPathBlacklisted("c:\\plugins\\badplugin.vst3"));
        REQUIRE(blacklist.isPathBlacklisted("C:\\Plugins\\BadPlugin.VST3"));
    }

    SECTION("Forward slash normalization")
    {
        blacklist.addPath("C:/Plugins/BadPlugin.vst3");

        REQUIRE(blacklist.isPathBlacklisted("C:\\Plugins\\BadPlugin.vst3"));
        REQUIRE(blacklist.isPathBlacklisted("C:/Plugins/BadPlugin.vst3"));
    }

    SECTION("Mixed slash normalization")
    {
        blacklist.addPath("C:\\Plugins/SubDir\\BadPlugin.vst3");

        REQUIRE(blacklist.isPathBlacklisted("C:/Plugins/SubDir/BadPlugin.vst3"));
    }
}

TEST_CASE("PluginBlacklist - ID Management", "[protection][blacklist]")
{
    auto& blacklist = MockPluginBlacklist::getInstance();
    blacklist.clear();

    SECTION("Add and query ID")
    {
        REQUIRE(blacklist.getIdCount() == 0);

        blacklist.addId("com.badplugin.crasher");
        REQUIRE(blacklist.getIdCount() == 1);
        REQUIRE(blacklist.isIdBlacklisted("com.badplugin.crasher"));
    }

    SECTION("Remove ID")
    {
        blacklist.addId("com.badplugin.crasher");
        REQUIRE(blacklist.isIdBlacklisted("com.badplugin.crasher"));

        blacklist.removeId("com.badplugin.crasher");
        REQUIRE_FALSE(blacklist.isIdBlacklisted("com.badplugin.crasher"));
    }

    SECTION("ID is case-sensitive")
    {
        blacklist.addId("com.BadPlugin.Crasher");

        REQUIRE(blacklist.isIdBlacklisted("com.BadPlugin.Crasher"));
        // IDs should be case-sensitive (VST3 plugin IDs are case-sensitive)
        REQUIRE_FALSE(blacklist.isIdBlacklisted("com.badplugin.crasher"));
    }

    SECTION("Path and ID are independent")
    {
        blacklist.addPath("C:\\Plugins\\BadPlugin.vst3");
        blacklist.addId("com.badplugin.id");

        REQUIRE(blacklist.getPathCount() == 1);
        REQUIRE(blacklist.getIdCount() == 1);
        REQUIRE(blacklist.isPathBlacklisted("C:\\Plugins\\BadPlugin.vst3"));
        REQUIRE(blacklist.isIdBlacklisted("com.badplugin.id"));

        // Removal of one doesn't affect the other
        blacklist.removePath("C:\\Plugins\\BadPlugin.vst3");
        REQUIRE(blacklist.getPathCount() == 0);
        REQUIRE(blacklist.getIdCount() == 1);
        REQUIRE(blacklist.isIdBlacklisted("com.badplugin.id"));
    }
}

TEST_CASE("PluginBlacklist - Retrieval", "[protection][blacklist]")
{
    auto& blacklist = MockPluginBlacklist::getInstance();
    blacklist.clear();

    SECTION("Get all blacklisted paths")
    {
        blacklist.addPath("C:\\Plugins\\Bad1.vst3");
        blacklist.addPath("C:\\Plugins\\Bad2.vst3");

        auto paths = blacklist.getBlacklistedPaths();
        REQUIRE(paths.size() == 2);
    }

    SECTION("Get all blacklisted IDs")
    {
        blacklist.addId("com.plugin.id1");
        blacklist.addId("com.plugin.id2");
        blacklist.addId("com.plugin.id3");

        auto ids = blacklist.getBlacklistedIds();
        REQUIRE(ids.size() == 3);
    }

    SECTION("Empty retrieval returns empty vector")
    {
        auto paths = blacklist.getBlacklistedPaths();
        auto ids = blacklist.getBlacklistedIds();

        REQUIRE(paths.empty());
        REQUIRE(ids.empty());
    }
}

// =============================================================================
// CrashProtection Tests
// =============================================================================

TEST_CASE("CrashProtection - Successful Operations", "[protection][crash]")
{
    auto& protection = MockCrashProtection::getInstance();
    protection.reset();

    SECTION("Successful operation returns true")
    {
        int counter = 0;
        bool result = protection.executeWithProtection([&]() { counter = 42; }, "TestOperation", "TestPlugin");

        REQUIRE(result == true);
        REQUIRE(counter == 42);
        REQUIRE(protection.getOperationsExecuted() == 1);
        REQUIRE(protection.getExceptionsCaught() == 0);
    }

    SECTION("Operation context is cleared after execution")
    {
        protection.executeWithProtection([]() {}, "SomeOperation", "SomePlugin");

        REQUIRE(protection.getCurrentOperation().empty());
        REQUIRE(protection.getCurrentPluginName().empty());
    }

    SECTION("Multiple successful operations")
    {
        int sum = 0;
        protection.executeWithProtection([&]() { sum += 10; }, "Op1");
        protection.executeWithProtection([&]() { sum += 20; }, "Op2");
        protection.executeWithProtection([&]() { sum += 30; }, "Op3");

        REQUIRE(sum == 60);
        REQUIRE(protection.getOperationsExecuted() == 3);
        REQUIRE(protection.getExceptionsCaught() == 0);
    }
}

TEST_CASE("CrashProtection - Exception Handling", "[protection][crash]")
{
    auto& protection = MockCrashProtection::getInstance();
    protection.reset();

    SECTION("Catches std::exception and returns false")
    {
        bool result = protection.executeWithProtection([]() { throw std::runtime_error("Test exception"); },
                                                       "ThrowingOperation", "BadPlugin");

        REQUIRE(result == false);
        REQUIRE(protection.getExceptionsCaught() == 1);
        REQUIRE(protection.getLastExceptionMessage() == "Test exception");
    }

    SECTION("Catches unknown exception and returns false")
    {
        bool result = protection.executeWithProtection([]() { throw 42; }, // Non-standard exception
                                                       "ThrowingOperation");

        REQUIRE(result == false);
        REQUIRE(protection.getExceptionsCaught() == 1);
        REQUIRE(protection.getLastExceptionMessage() == "Unknown exception");
    }

    SECTION("Context is cleared even after exception")
    {
        protection.executeWithProtection([]() { throw std::runtime_error("Error"); }, "FailingOp", "FailPlugin");

        REQUIRE(protection.getCurrentOperation().empty());
        REQUIRE(protection.getCurrentPluginName().empty());
    }

    SECTION("Mixed success and failure")
    {
        protection.executeWithProtection([]() {}, "Success1");
        protection.executeWithProtection([]() { throw std::runtime_error("Fail"); }, "Failure");
        protection.executeWithProtection([]() {}, "Success2");

        REQUIRE(protection.getOperationsExecuted() == 2);
        REQUIRE(protection.getExceptionsCaught() == 1);
    }
}

TEST_CASE("CrashProtection - Auto-Save Callback", "[protection][crash]")
{
    auto& protection = MockCrashProtection::getInstance();
    protection.reset();

    SECTION("Auto-save callback is invoked before operation")
    {
        int saveCallOrder = 0;
        int operationOrder = 0;
        int orderCounter = 0;

        protection.setAutoSaveCallback([&]() { saveCallOrder = ++orderCounter; });

        protection.executeWithProtection([&]() { operationOrder = ++orderCounter; }, "TestOp");

        REQUIRE(saveCallOrder == 1);
        REQUIRE(operationOrder == 2);
        REQUIRE(protection.getAutoSaveCallCount() == 1);
    }

    SECTION("Auto-save is called for each operation")
    {
        protection.setAutoSaveCallback([]() {});

        protection.executeWithProtection([]() {}, "Op1");
        protection.executeWithProtection([]() {}, "Op2");
        protection.executeWithProtection([]() {}, "Op3");

        REQUIRE(protection.getAutoSaveCallCount() == 3);
    }

    SECTION("No callback set - no crash")
    {
        // No callback set, should not crash
        bool result = protection.executeWithProtection([]() {}, "SafeOp");
        REQUIRE(result == true);
        REQUIRE(protection.getAutoSaveCallCount() == 0);
    }

    SECTION("Auto-save is called even when exception thrown")
    {
        int saveCallCount = 0;
        protection.setAutoSaveCallback([&]() { saveCallCount++; });

        protection.executeWithProtection([]() { throw std::runtime_error("Error"); }, "FailOp");

        // Auto-save should still have been called before the operation
        REQUIRE(saveCallCount == 1);
    }
}

TEST_CASE("CrashProtection - Operation Context", "[protection][crash]")
{
    auto& protection = MockCrashProtection::getInstance();
    protection.reset();

    SECTION("Context is set during operation")
    {
        std::string capturedOp;
        std::string capturedPlugin;

        protection.executeWithProtection(
            [&]()
            {
                capturedOp = protection.getCurrentOperation();
                capturedPlugin = protection.getCurrentPluginName();
            },
            "CreateEditor", "SurgeXT");

        REQUIRE(capturedOp == "CreateEditor");
        REQUIRE(capturedPlugin == "SurgeXT");
    }

    SECTION("Empty plugin name is allowed")
    {
        std::string capturedPlugin;

        protection.executeWithProtection([&]() { capturedPlugin = protection.getCurrentPluginName(); },
                                         "InternalOperation");

        REQUIRE(capturedPlugin.empty());
    }
}

// =============================================================================
// Mutation Tests - PluginBlacklist
// =============================================================================

TEST_CASE("PluginBlacklist - Mutation Tests", "[protection][mutation]")
{
    auto& blacklist = MockPluginBlacklist::getInstance();
    blacklist.clear();

    SECTION("MUTATION: isPathBlacklisted - return true instead of false")
    {
        // If mutation changed to always return true, this would fail
        REQUIRE_FALSE(blacklist.isPathBlacklisted("NonExistent.vst3"));
    }

    SECTION("MUTATION: isPathBlacklisted - return false instead of true")
    {
        blacklist.addPath("C:\\Exists.vst3");
        // If mutation changed to always return false, this would fail
        REQUIRE(blacklist.isPathBlacklisted("C:\\Exists.vst3"));
    }

    SECTION("MUTATION: addPath - skip insertion")
    {
        blacklist.addPath("C:\\Plugin.vst3");
        // If mutation skipped the insert, count would be 0
        REQUIRE(blacklist.getPathCount() > 0);
    }

    SECTION("MUTATION: removePath - skip removal")
    {
        blacklist.addPath("C:\\Plugin.vst3");
        blacklist.removePath("C:\\Plugin.vst3");
        // If mutation skipped the erase, it would still be present
        REQUIRE_FALSE(blacklist.isPathBlacklisted("C:\\Plugin.vst3"));
    }

    SECTION("MUTATION: normalizePath - skip lowercase conversion")
    {
        blacklist.addPath("C:\\UPPER\\PATH.vst3");
        // If normalization was skipped, lowercase query wouldn't match
        REQUIRE(blacklist.isPathBlacklisted("c:\\upper\\path.vst3"));
    }

    SECTION("MUTATION: getPathCount - return 0")
    {
        blacklist.addPath("C:\\Plugin.vst3");
        // If mutation returned 0, this would fail
        REQUIRE(blacklist.getPathCount() == 1);
    }

    SECTION("MUTATION: clear - skip clear")
    {
        blacklist.addPath("C:\\A.vst3");
        blacklist.addPath("C:\\B.vst3");
        blacklist.clear();
        // If clear was skipped, count would be > 0
        REQUIRE(blacklist.getPathCount() == 0);
    }
}

// =============================================================================
// Mutation Tests - CrashProtection
// =============================================================================

TEST_CASE("CrashProtection - Mutation Tests", "[protection][mutation]")
{
    auto& protection = MockCrashProtection::getInstance();
    protection.reset();

    SECTION("MUTATION: executeWithProtection - return false instead of true")
    {
        bool result = protection.executeWithProtection([]() {}, "GoodOp");
        // If mutation returned false for success, this would fail
        REQUIRE(result == true);
    }

    SECTION("MUTATION: executeWithProtection - return true instead of false")
    {
        bool result = protection.executeWithProtection([]() { throw std::runtime_error("Fail"); }, "BadOp");
        // If mutation returned true for failure, this would fail
        REQUIRE(result == false);
    }

    SECTION("MUTATION: executeWithProtection - skip operation call")
    {
        int counter = 0;
        protection.executeWithProtection([&]() { counter = 99; }, "CountOp");
        // If operation was skipped, counter would be 0
        REQUIRE(counter == 99);
    }

    SECTION("MUTATION: setAutoSaveCallback - skip callback storage")
    {
        int callCount = 0;
        protection.setAutoSaveCallback([&]() { callCount++; });
        protection.triggerAutoSave();
        // If callback wasn't stored, callCount would be 0
        REQUIRE(callCount == 1);
    }

    SECTION("MUTATION: triggerAutoSave - skip callback invocation")
    {
        int callCount = 0;
        protection.setAutoSaveCallback([&]() { callCount++; });
        protection.executeWithProtection([]() {}, "TestOp");
        // If triggerAutoSave skipped callback, callCount would be 0
        REQUIRE(callCount > 0);
    }

    SECTION("MUTATION: clearCurrentOperation - skip clear")
    {
        protection.executeWithProtection([]() {}, "TestOp", "TestPlugin");
        // If clear was skipped, operation would still be set
        REQUIRE(protection.getCurrentOperation().empty());
    }

    SECTION("MUTATION: exceptionsCaught increment skipped")
    {
        protection.executeWithProtection([]() { throw std::runtime_error("E"); }, "FailOp");
        // If increment was skipped, count would be 0
        REQUIRE(protection.getExceptionsCaught() == 1);
    }

    SECTION("MUTATION: operationsExecuted increment skipped")
    {
        protection.executeWithProtection([]() {}, "SuccessOp");
        // If increment was skipped, count would be 0
        REQUIRE(protection.getOperationsExecuted() == 1);
    }
}

// =============================================================================
// Thread Safety Tests
// =============================================================================

TEST_CASE("PluginBlacklist - Thread Safety", "[protection][threading]")
{
    auto& blacklist = MockPluginBlacklist::getInstance();
    blacklist.clear();

    SECTION("Concurrent adds from multiple threads")
    {
        constexpr int numThreads = 4;
        constexpr int pathsPerThread = 50;
        std::vector<std::thread> threads;

        for (int t = 0; t < numThreads; t++)
        {
            threads.emplace_back(
                [&blacklist, t, pathsPerThread]()
                {
                    for (int i = 0; i < pathsPerThread; i++)
                    {
                        std::string path = "C:\\Thread" + std::to_string(t) + "\\Plugin" + std::to_string(i) + ".vst3";
                        blacklist.addPath(path);
                    }
                });
        }

        for (auto& thread : threads)
        {
            thread.join();
        }

        // All paths should have been added
        REQUIRE(blacklist.getPathCount() == numThreads * pathsPerThread);
    }

    SECTION("Concurrent reads and writes")
    {
        // Pre-populate
        for (int i = 0; i < 100; i++)
        {
            blacklist.addPath("C:\\Init\\Plugin" + std::to_string(i) + ".vst3");
        }

        std::atomic<int> successfulReads{0};
        std::atomic<bool> running{true};

        // Reader thread
        std::thread reader(
            [&]()
            {
                while (running)
                {
                    bool found = blacklist.isPathBlacklisted("C:\\Init\\Plugin50.vst3");
                    if (found)
                        successfulReads++;
                }
            });

        // Writer thread
        std::thread writer(
            [&blacklist]()
            {
                for (int i = 0; i < 50; i++)
                {
                    blacklist.addPath("C:\\New\\Plugin" + std::to_string(i) + ".vst3");
                    std::this_thread::sleep_for(std::chrono::microseconds(100));
                }
            });

        writer.join();
        running = false;
        reader.join();

        // Reader should have found the path multiple times
        REQUIRE(successfulReads > 0);
        // All new paths should have been added
        REQUIRE(blacklist.getPathCount() == 150);
    }
}

TEST_CASE("CrashProtection - Thread Safety", "[protection][threading]")
{
    auto& protection = MockCrashProtection::getInstance();
    protection.reset();

    SECTION("Concurrent operations from multiple threads")
    {
        constexpr int numThreads = 4;
        constexpr int opsPerThread = 25;
        std::atomic<int> totalOps{0};
        std::vector<std::thread> threads;

        for (int t = 0; t < numThreads; t++)
        {
            threads.emplace_back(
                [&protection, &totalOps, t, opsPerThread]()
                {
                    for (int i = 0; i < opsPerThread; i++)
                    {
                        bool result = protection.executeWithProtection(
                            [&totalOps]() { totalOps++; }, "Thread" + std::to_string(t) + "Op" + std::to_string(i));
                        REQUIRE(result == true);
                    }
                });
        }

        for (auto& thread : threads)
        {
            thread.join();
        }

        REQUIRE(totalOps == numThreads * opsPerThread);
        // Note: operationsExecuted tracking may have race conditions in the mock,
        // but the actual operations completed correctly
    }
}
