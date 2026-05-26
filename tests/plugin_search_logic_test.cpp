/**
 * @file plugin_search_logic_test.cpp
 * @brief Regression tests for plugin search matching and category filtering.
 */

#include <catch2/catch_test_macros.hpp>

#include "../src/PluginSearchLogic.h"

namespace
{

juce::PluginDescription makePlugin(juce::String name, juce::String manufacturer, juce::String format,
                                   juce::String category, bool isInstrument)
{
    juce::PluginDescription desc;
    desc.name = std::move(name);
    desc.manufacturerName = std::move(manufacturer);
    desc.pluginFormatName = std::move(format);
    desc.category = std::move(category);
    desc.isInstrument = isInstrument;
    return desc;
}

} // namespace

TEST_CASE("Plugin search fuzzy scoring ranks common match types", "[plugin-search][fuzzy]")
{
    using PluginSearchLogic::fuzzyScore;

    const int exact = fuzzyScore("serum", "Serum");
    const int prefix = fuzzyScore("fab", "FabFilter Pro-Q 3");
    const int substring = fuzzyScore("filter", "FabFilter Pro-Q 3");
    const int initials = fuzzyScore("fpq", "FabFilter Pro-Q 3");
    const int loose = fuzzyScore("srum", "Serum");
    const int miss = fuzzyScore("xyz", "Serum");

    REQUIRE(exact > prefix);
    REQUIRE(prefix > substring);
    REQUIRE(substring > initials);
    REQUIRE(initials > loose);
    REQUIRE(loose > miss);
    REQUIRE(miss == 0);
}

TEST_CASE("Plugin search scores manufacturer matches", "[plugin-search][fuzzy]")
{
    auto plugin = makePlugin("Pro-Q 3", "FabFilter", "VST3", "EQ", false);

    REQUIRE(PluginSearchLogic::scorePlugin("fab", plugin) > 0);
    REQUIRE(PluginSearchLogic::scorePlugin("pro", plugin) > 0);
    REQUIRE(PluginSearchLogic::scorePlugin("", plugin) == 100);
    REQUIRE(PluginSearchLogic::scorePlugin("no-match", plugin) == 0);
}

TEST_CASE("Plugin search category filters distinguish effects, instruments, and internal tools",
          "[plugin-search][category]")
{
    using PluginSearchLogic::Category;
    using PluginSearchLogic::matchesCategory;

    auto effect = makePlugin("Compressor", "Vendor", "VST3", "Dynamics", false);
    auto instrument = makePlugin("Synth", "Vendor", "VST3", "Instrument", true);
    auto internal = makePlugin("Level", "Pedalboard3", "Internal", "Pedalboard", false);
    auto builtIn = makePlugin("Effect Rack", "Pedalboard3", "VST3", "Built-in", false);

    REQUIRE(matchesCategory(effect, Category::All));
    REQUIRE(matchesCategory(effect, Category::Effects));
    REQUIRE_FALSE(matchesCategory(effect, Category::Instruments));
    REQUIRE_FALSE(matchesCategory(effect, Category::Internal));

    REQUIRE_FALSE(matchesCategory(instrument, Category::Effects));
    REQUIRE(matchesCategory(instrument, Category::Instruments));

    REQUIRE_FALSE(matchesCategory(internal, Category::Effects));
    REQUIRE(matchesCategory(internal, Category::Internal));

    REQUIRE_FALSE(matchesCategory(builtIn, Category::Effects));
    REQUIRE(matchesCategory(builtIn, Category::Internal));
}
