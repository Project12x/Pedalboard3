#include "PluginFormatIdentity.h"
#include "PluginScanCoordinator.h"
#include "SupportedPluginFormats.h"
#include "PluginScanOrchestration.h"
#include "ScannerResponseWorker.h"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cmath>
#include <condition_variable>
#include <mutex>
#include <thread>

TEST_CASE("scan orchestration preserves format order and cancellation boundary", "[plugins][scan-orchestration]")
{
    std::vector<PluginScanPlanJob> jobs { { "VST3", { "v1" } }, { "AudioUnit", { "a1", "a2" } } };
    std::vector<std::string> visits;
    const auto result = runPluginScanPlan(jobs, [&] (const std::string& format, const std::string& candidate)
    {
        visits.push_back(format + ":" + candidate);
        return candidate == "a1" ? PluginCandidateDecision::cancelled : PluginCandidateDecision::discovered;
    });
    REQUIRE(visits == std::vector<std::string> { "VST3:v1", "AudioUnit:a1" });
    REQUIRE(result.discovered == 1);
    REQUIRE(result.cancelled);
}

TEST_CASE("scan orchestration quarantines only explicit candidate failure and continues", "[plugins][scan-orchestration]")
{
    std::vector<PluginScanPlanJob> jobs { { "AudioUnit", { "bad", "good" } } };
    const auto result = runPluginScanPlan(jobs, [] (const std::string&, const std::string& candidate)
    {
        return candidate == "bad" ? PluginCandidateDecision::quarantined : PluginCandidateDecision::discovered;
    });
    REQUIRE(result.quarantined == 1);
    REQUIRE(result.discovered == 1);
    REQUIRE_FALSE(result.isolationUnavailable);
}

TEST_CASE("scan orchestration stops for unavailable isolation without quarantine", "[plugins][scan-orchestration]")
{
    std::vector<PluginScanPlanJob> jobs { { "VST3", { "candidate" } } };
    const auto result = runPluginScanPlan(jobs, [] (const std::string&, const std::string&)
    {
        return PluginCandidateDecision::isolationUnavailable;
    });
    REQUIRE(result.isolationUnavailable);
    REQUIRE(result.quarantined == 0);
}

TEST_CASE("helper outcome classifies infrastructure failures separately", "[plugins][scan-orchestration]")
{
    REQUIRE(classifyHelperResult(true, false) == PluginCandidateDecision::discovered);
    REQUIRE(classifyHelperResult(false, true) == PluginCandidateDecision::quarantined);
    REQUIRE(classifyHelperResult(false, false) == PluginCandidateDecision::isolationUnavailable);
}

TEST_CASE("response worker shutdown joins active work and rejects later submissions", "[plugins][scanner-worker]")
{
    ScannerResponseWorker worker;
    std::mutex mutex;
    std::condition_variable condition;
    bool entered = false, release = false, shutdownReturned = false;
    REQUIRE(worker.submit([&]
    {
        std::unique_lock<std::mutex> lock(mutex);
        entered = true;
        condition.notify_all();
        condition.wait(lock, [&] { return release; });
        return ScannerWorkerResponse { 1, "late" };
    }));
    {
        std::unique_lock<std::mutex> lock(mutex);
        condition.wait(lock, [&] { return entered; });
    }
    std::thread stopping([&]
    {
        worker.shutdown();
        std::lock_guard<std::mutex> lock(mutex);
        shutdownReturned = true;
        condition.notify_all();
    });
    {
        std::unique_lock<std::mutex> lock(mutex);
        REQUIRE_FALSE(condition.wait_for(lock, std::chrono::milliseconds(20), [&] { return shutdownReturned; }));
        release = true;
        condition.notify_all();
        REQUIRE(condition.wait_for(lock, std::chrono::seconds(1), [&] { return shutdownReturned; }));
    }
    stopping.join();
    REQUIRE_FALSE(worker.submit([] { return ScannerWorkerResponse {}; }));
    REQUIRE(worker.drainResponses().empty());
}

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
