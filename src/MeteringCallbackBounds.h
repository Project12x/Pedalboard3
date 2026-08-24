#pragma once

// Single source of truth for the engine's audio-thread channel-width ceiling.
// Fixed-size per-channel arrays throughout the codebase (gain state, meter
// taps, Link Audio publish buffers) are sized from this constant. It's a
// compile-time safety bound, not a target device width - actual channel
// counts used each callback are always the live device's own count, clamped
// to this ceiling via jmin()/jlimit(). Set with headroom above known
// multichannel interfaces (e.g. 24-out audio devices) rather than tuned to
// any one device.
struct MeteringCallbackBounds
{
    static constexpr int MaxChannels = 32;
    static constexpr int MaxBlockSize = 8192;
};

enum class MeteringCallbackRejectReason
{
    None,
    InvalidSampleCount,
    InvalidChannelCount,
    BlockTooLarge,
    TooManyInputChannels,
    TooManyOutputChannels
};

struct MeteringCallbackBufferPlan
{
    int numInputChannels = 0;
    int numOutputChannels = 0;
    int numSamples = 0;
    bool valid = false;
    MeteringCallbackRejectReason rejectReason = MeteringCallbackRejectReason::None;
};

constexpr MeteringCallbackBufferPlan makeMeteringCallbackBufferPlan(int numInputChannels,
                                                                    int numOutputChannels,
                                                                    int numSamples) noexcept
{
    if (numSamples <= 0)
        return {0, 0, 0, false, MeteringCallbackRejectReason::InvalidSampleCount};

    if (numInputChannels < 0 || numOutputChannels < 0)
        return {0, 0, 0, false, MeteringCallbackRejectReason::InvalidChannelCount};

    if (numSamples > MeteringCallbackBounds::MaxBlockSize)
        return {0, 0, 0, false, MeteringCallbackRejectReason::BlockTooLarge};

    if (numInputChannels > MeteringCallbackBounds::MaxChannels)
        return {0, 0, 0, false, MeteringCallbackRejectReason::TooManyInputChannels};

    if (numOutputChannels > MeteringCallbackBounds::MaxChannels)
        return {0, 0, 0, false, MeteringCallbackRejectReason::TooManyOutputChannels};

    return {numInputChannels, numOutputChannels, numSamples, true, MeteringCallbackRejectReason::None};
}
