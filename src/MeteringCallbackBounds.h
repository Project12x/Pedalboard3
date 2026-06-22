#pragma once

struct MeteringCallbackBounds
{
    static constexpr int MaxChannels = 16;
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
