#pragma once

#include <functional>
#include <string>
#include <vector>

enum class PluginCandidateDecision
{
    discovered,
    skipped,
    quarantined,
    failed,
    cancelled,
    isolationUnavailable
};

enum class PluginScanCandidateOutcome
{
    discovered,
    skipped,
    quarantined,
    failed
};

struct PluginScanPlanJob
{
    std::string formatName;
    std::vector<std::string> candidates;
};

struct PluginScanPlanResult
{
    int discovered = 0;
    int skipped = 0;
    int quarantined = 0;
    int failed = 0;
    bool cancelled = false;
    bool isolationUnavailable = false;
};

using PluginCandidateRunner = std::function<PluginCandidateDecision(const std::string&, const std::string&)>;

/** Runs jobs/candidates in order and stops only after a resolved cancellation or unavailable isolation. */
PluginScanPlanResult runPluginScanPlan(const std::vector<PluginScanPlanJob>& jobs, const PluginCandidateRunner& run);

PluginCandidateDecision classifyHelperResult(bool helperSucceeded, bool explicitCandidateFailure);
