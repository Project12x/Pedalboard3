#include "PlatformPaths.h"

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <fstream>
#include <iterator>
#include <string>

namespace
{
std::string readProjectSource(const char* relativePath)
{
    std::ifstream input(std::string(PEDALBOARD3_SOURCE_DIR) + "/" + relativePath);
    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}
}

TEST_CASE("macOS user data lives in Application Support", "[macos][paths]")
{
    const juce::File library("/Users/test/Library");
    const auto result = pedalboard3::platform::userDataDirectoryForBase(library, true);
    CHECK(result.getFullPathName() == "/Users/test/Library/Application Support/Pedalboard3");
}

TEST_CASE("non-macOS user data remains below the JUCE application-data root", "[paths]")
{
    const juce::File root("/var/test-app-data");
    const auto result = pedalboard3::platform::userDataDirectoryForBase(root, false);
    CHECK(result.getFullPathName() == "/var/test-app-data/Pedalboard3");
}

TEST_CASE("documentation resolves inside a macOS application bundle", "[macos][paths]")
{
    const juce::File app("/Applications/Pedalboard3.app");
    const auto result = pedalboard3::platform::documentationDirectoryForApplication(app, true);
    CHECK(result.getFullPathName() ==
          "/Applications/Pedalboard3.app/Contents/Resources/documentation");
}

TEST_CASE("documentation remains adjacent to non-bundle executables", "[paths]")
{
    const juce::File executable("/opt/Pedalboard3/Pedalboard3.exe");
    const auto result = pedalboard3::platform::documentationDirectoryForApplication(executable, false);
    CHECK(result.getFullPathName() == "/opt/Pedalboard3/documentation");
}

TEST_CASE("application data consumers use the shared path policy", "[macos][paths]")
{
    constexpr std::array<const char*, 9> consumers{
        "src/App.cpp",
        "src/JuceHelperStuff.cpp",
        "src/LogFile.cpp",
        "src/PresetManager.cpp",
        "src/UserPresetWindow.h",
        "src/UserPresetWindow.cpp",
        "src/ScratchRecorder.cpp",
        "src/ScratchTake.cpp",
        "src/SettingsManager.cpp"
    };

    for (const auto* path : consumers)
    {
        INFO(path);
        const auto source = readProjectSource(path);
        REQUIRE_FALSE(source.empty());
        CHECK(source.find("userApplicationDataDirectory") == std::string::npos);
        CHECK(source.find("PlatformPaths.h") != std::string::npos);
    }
}
