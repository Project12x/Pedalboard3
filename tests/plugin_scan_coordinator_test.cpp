#include "PluginFormatIdentity.h"
#include "PluginScanCoordinator.h"
#include "SupportedPluginFormats.h"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cmath>

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

TEST_CASE("scan progress is format ordered and counts completed candidates", "[plugins][scan-coordinator]")
{
    REQUIRE(std::abs(calculateOverallPluginScanProgress(0, 0.5f, 2) - 0.25f) < 0.0001f);
    REQUIRE(std::abs(calculateOverallPluginScanProgress(1, 0.5f, 2) - 0.75f) < 0.0001f);

    const auto summary = makePluginScanSummary({ PluginScanCandidateOutcome::discovered,
                                                  PluginScanCandidateOutcome::skipped,
                                                  PluginScanCandidateOutcome::quarantined,
                                                  PluginScanCandidateOutcome::failed }, false);
    REQUIRE(summary.discovered == 1);
    REQUIRE(summary.skipped == 1);
    REQUIRE(summary.quarantined == 1);
    REQUIRE(summary.failed == 1);
    REQUIRE_FALSE(summary.cancelled);
}

TEST_CASE("cancelled scan preserves resolved results and marks cancellation", "[plugins][scan-coordinator]")
{
    const auto summary = makePluginScanSummary({ PluginScanCandidateOutcome::discovered,
                                                  PluginScanCandidateOutcome::quarantined }, true);
    REQUIRE(summary.discovered == 1);
    REQUIRE(summary.quarantined == 1);
    REQUIRE(summary.failed == 0);
    REQUIRE(summary.cancelled);
}
