#include "SupportedPluginFormats.h"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>

namespace
{
bool containsOnlyOne(const juce::StringArray& names, const juce::String& name)
{
    return std::count(names.begin(), names.end(), name) == 1;
}
} // namespace

TEST_CASE("application formats are registered exactly once", "[plugins][formats]")
{
    juce::AudioPluginFormatManager manager;
    int internalRegistrationCalls = 0;
    auto registerInternal = [&] (auto&) { ++internalRegistrationCalls; };
    registerSupportedPluginFormats(manager, PluginFormatRole::application, registerInternal);
    auto names = getRegisteredFormatNames(manager);
    REQUIRE(internalRegistrationCalls == 1);
    REQUIRE(names.contains("VST3"));
#if JUCE_MAC
    REQUIRE(names.contains("AudioUnit"));
#else
    REQUIRE_FALSE(names.contains("AudioUnit"));
#endif
    for (const auto& name : names)
        REQUIRE(containsOnlyOne(names, name));
}

TEST_CASE("scanner excludes internal processors", "[plugins][formats]")
{
    juce::AudioPluginFormatManager manager;
    registerSupportedPluginFormats(manager, PluginFormatRole::scanner);
    registerSupportedPluginFormats(manager, PluginFormatRole::scanner);
    REQUIRE_FALSE(getRegisteredFormatNames(manager).contains("Internal"));
    auto names = getRegisteredFormatNames(manager);
    for (const auto& name : names)
        REQUIRE(containsOnlyOne(names, name));
}
