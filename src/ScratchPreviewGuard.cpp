#include "ScratchPreviewGuard.h"
#include <algorithm>
#include <cmath>

bool ScratchPreviewGuard::process(float* const* data, int channels, int samples, bool muted) noexcept
{
    auto clear = [&] { for (int ch = 0; ch < channels; ++ch) if (data[ch]) std::fill_n(data[ch], samples, 0.0f); };
    if (muted || latched) { clear(); return false; }
    bool trip = core.process(data, channels, samples);
    for (int ch = 0; ch < channels && !trip; ++ch)
        if (data[ch]) for (int i = 0; i < samples; ++i)
            if (!std::isfinite(data[ch][i])) { trip = true; break; }
    if (trip) { latched = true; clear(); return true; }
    for (int ch = 0; ch < channels; ++ch)
        if (data[ch]) for (int i = 0; i < samples; ++i)
            data[ch][i] = std::clamp(data[ch][i], -1.0f, 1.0f);
    return false;
}
