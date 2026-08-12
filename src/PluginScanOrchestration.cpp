#include "PluginScanOrchestration.h"

PluginScanPlanResult runPluginScanPlan(const std::vector<PluginScanPlanJob>& jobs, const PluginCandidateRunner& run)
{
    PluginScanPlanResult result;
    for (const auto& job : jobs)
    {
        for (const auto& candidate : job.candidates)
        {
            switch (run(job.formatName, candidate))
            {
                case PluginCandidateDecision::discovered: ++result.discovered; break;
                case PluginCandidateDecision::skipped: ++result.skipped; break;
                case PluginCandidateDecision::quarantined: ++result.quarantined; break;
                case PluginCandidateDecision::failed: ++result.failed; break;
                case PluginCandidateDecision::cancelled:
                    result.cancelled = true;
                    return result;
                case PluginCandidateDecision::isolationUnavailable:
                    result.isolationUnavailable = true;
                    return result;
            }
        }
    }
    return result;
}

PluginCandidateDecision classifyHelperResult(bool helperSucceeded, bool explicitCandidateFailure)
{
    if (helperSucceeded)
        return PluginCandidateDecision::discovered;
    return explicitCandidateFailure ? PluginCandidateDecision::quarantined
                                    : PluginCandidateDecision::isolationUnavailable;
}
