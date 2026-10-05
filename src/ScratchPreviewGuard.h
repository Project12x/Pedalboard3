#pragma once
#include "dsp/SafetyLimiterCore.h"

// Independent preview protection: never registers as the graph's limiter.
class ScratchPreviewGuard
{
public:
    void prepare(double sampleRate) { core.prepare(sampleRate); }
    // Processing-thread only; explicit user unlock, not a source/device change.
    void reset() noexcept { core.reset(); latched = false; }
    bool process(float* const*, int channels, int samples, bool muted) noexcept;
private:
    SafetyLimiterCore core;
    bool latched = false;
};
