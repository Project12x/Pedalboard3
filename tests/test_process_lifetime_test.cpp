#include "AudioSingletons.h"
#include <catch2/catch_test_macros.hpp>
#include <cstdio>
#include <cstdlib>

#if JUCE_WINDOWS
#include <windows.h>

namespace
{
HANDLE thumbnailThreadForExitProbe = nullptr;

void verifyThumbnailJoinedBeforeStaticDestruction()
{
    const auto result = WaitForSingleObject(thumbnailThreadForExitProbe, 0);
    CloseHandle(thumbnailThreadForExitProbe);
    if (result != WAIT_OBJECT_0)
    {
        std::fputs("FAIL: thumbnail worker survived main and entered CRT static destruction\n", stderr);
        // This regression runs at the boundary Catch2 cannot observe. Avoid
        // entering the known unsafe destructor path after reporting failure.
        std::_Exit(86);
    }
}
}

TEST_CASE("Test runner joins the host thumbnail worker before CRT exit", "[test-lifecycle]")
{
    auto& worker = AudioThumbnailCacheSingleton::getInstance().getTimeSliceThread();
    const auto id = static_cast<DWORD>(reinterpret_cast<uintptr_t>(worker.getThreadId()));
    thumbnailThreadForExitProbe = OpenThread(SYNCHRONIZE, FALSE, id);
    REQUIRE(thumbnailThreadForExitProbe != nullptr);
    REQUIRE(std::atexit(verifyThumbnailJoinedBeforeStaticDestruction) == 0);
}
#endif
