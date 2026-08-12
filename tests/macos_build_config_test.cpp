#include <catch2/catch_test_macros.hpp>

#include <fstream>
#include <iterator>
#include <string>

namespace
{
std::string readSource(const char* relativePath)
{
    std::ifstream input(std::string(PEDALBOARD3_SOURCE_DIR) + "/" + relativePath);
    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}
}

TEST_CASE("macOS build configuration isolates platform code", "[macos][build]")
{
    const auto cmake = readSource("CMakeLists.txt");
    const auto pluginHeader = readSource("src/PluginComponent.h");
    const auto persistence = readSource("src/PluginFieldPersistence.cpp");
    const auto app = readSource("src/App.cpp");
    const auto compileCommands = readSource("build/macos-arm64-debug/compile_commands.json");

    REQUIRE_FALSE(cmake.empty());
    REQUIRE_FALSE(compileCommands.empty());
    CHECK(cmake.find("MICROPHONE_PERMISSION_ENABLED TRUE") != std::string::npos);
    CHECK(cmake.find("if(WIN32)\n    juce_add_console_app(Pedalboard3Scanner") != std::string::npos);
    CHECK(pluginHeader.find("c0xHeaders.h") == std::string::npos);
    const auto poolShutdown = app.find("PluginPoolManager::killInstance()");
    const auto formatShutdown = app.find("AudioPluginFormatManagerSingleton::killInstance()");
    REQUIRE(poolShutdown != std::string::npos);
    REQUIRE(formatShutdown != std::string::npos);
    CHECK(poolShutdown < formatShutdown);
    CHECK(pluginHeader.find("std::shared_ptr<MemoryBlock>") != std::string::npos);
    CHECK(persistence.find("#ifdef __APPLE__\nstruct NodeAndId") == std::string::npos);
    CHECK(persistence.find("AudioProcessorGraph::Node::Ptr node;") != std::string::npos);

#if JUCE_MAC
    CHECK(compileCommands.find("JUCE_ASIO=1") == std::string::npos);
    CHECK(compileCommands.find("JUCE_WASAPI=1") == std::string::npos);
    CHECK(compileCommands.find("JUCE_DIRECTSOUND=1") == std::string::npos);
    CHECK(compileCommands.find("JUCE_WIN_PER_MONITOR_DPI_AWARE=1") == std::string::npos);
#endif
}
