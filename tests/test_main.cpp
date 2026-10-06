#include "AudioSingletons.h"
#include <catch2/catch_session.hpp>
#include <cstdio>
#include <cstdlib>

#if JUCE_WINDOWS
#include <windows.h>
#endif

namespace
{
struct HostServiceShutdown
{
#if JUCE_WINDOWS
    HostServiceShutdown()
    {
        auto& worker = AudioThumbnailCacheSingleton::getInstance().getTimeSliceThread();
        const auto id = static_cast<DWORD>(reinterpret_cast<uintptr_t>(worker.getThreadId()));
        thumbnailThread = OpenThread(SYNCHRONIZE, FALSE, id);
        if (thumbnailThread == nullptr)
            failShutdown("cannot retain thumbnail worker handle");
    }

    HostServiceShutdown(const HostServiceShutdown&) = delete;
    HostServiceShutdown& operator=(const HostServiceShutdown&) = delete;
#endif

    ~HostServiceShutdown()
    {
        // Join the worker while the CRT is still live, not from global
        // destructors under its exit lock. Also covers --list-tests/CLI errors.
#if JUCE_WINDOWS
        auto& worker = AudioThumbnailCacheSingleton::getInstance().getTimeSliceThread();
        worker.signalThreadShouldExit();
        worker.notify();
        // JUCE clears its handle before the native thread finishes CRT teardown.
        // Own a separate handle and join before deleting the cache; this also
        // avoids its destructor's forced-termination fallback on a hung worker.
        const auto result = WaitForSingleObject(thumbnailThread, 5000);
        CloseHandle(thumbnailThread);
        if (result != WAIT_OBJECT_0)
            failShutdown("thumbnail worker did not terminate cooperatively");
#endif
        AudioThumbnailCacheSingleton::killInstance();
        AudioPluginFormatManagerSingleton::killInstance();
        AudioFormatManagerSingleton::killInstance();
    }

#if JUCE_WINDOWS
private:
    [[noreturn]] static void failShutdown(const char* reason)
    {
        std::fprintf(stderr, "FAIL: test-runner shutdown: %s\n", reason);
        // Do not enter unsafe static teardown after a failed lifetime boundary.
        std::_Exit(87);
    }

    HANDLE thumbnailThread = nullptr;
#endif
};
}

int main(int argc, char* argv[])
{
    const HostServiceShutdown shutdown;
    return Catch::Session().run(argc, argv);
}
