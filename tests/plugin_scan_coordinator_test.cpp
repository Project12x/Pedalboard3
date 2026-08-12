#include "PluginFormatIdentity.h"
#include "PluginScanCoordinator.h"
#include "SupportedPluginFormats.h"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>

TEST_CASE("exact identity includes format", "[plugins][scan-plan]")
{
    juce::PluginDescription au, vst;
    au.name = vst.name = "Twin";
    au.fileOrIdentifier = vst.fileOrIdentifier = "vendor.product";
    au.pluginFormatName = "AudioUnit";
    vst.pluginFormatName = "VST3";

    REQUIRE(makeExactPluginKey(au) != makeExactPluginKey(vst));
    REQUIRE(pluginFormatBadge("AudioUnit") == "AU");
}

TEST_CASE("mac scan plan enables asynchronous Audio Units", "[plugins][scan-plan]")
{
    juce::AudioPluginFormatManager manager;
    registerSupportedPluginFormats(manager, PluginFormatRole::scanner);

    const auto jobs = makePluginScanPlan(manager, juce::File::getSpecialLocation(juce::File::tempDirectory));
    REQUIRE_FALSE(jobs.empty());
    REQUIRE(jobs.front().formatName == "VST3");

#if JUCE_MAC
    const auto au = std::find_if(jobs.begin(), jobs.end(),
                                 [] (const auto& job) { return job.formatName == "AudioUnit"; });
    REQUIRE(au != jobs.end());
    REQUIRE(au->allowAsyncInstantiation);
#endif
}
