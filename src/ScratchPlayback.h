#pragma once
#include "ScratchPlaybackTypes.h"
#include "ScratchPcmQueue.h"
#include <condition_variable>
#include <mutex>
#include <thread>

class ScratchPlayback final
{
public:
    explicit ScratchPlayback(ScratchDecoderFactory = {});
    ~ScratchPlayback(); // owner must first remove the audio consumer
    void prepare(const ScratchPlaybackDevice&);
    juce::Result play(const ScratchTakeLoadResult&, ScratchPlaybackMode, const ScratchChannelMap&);
    void pause();
    void resume();
    void stop();
    void seek(double seconds);
    void setLooping(bool);
    void setVolumeDb(double);
    ScratchPlaybackStatus status() const;
    ScratchPlaybackBlock readBlock(float* const*, int channels, int samples) noexcept;
    bool isQuiescent() const noexcept;
    void audioDeviceStopped();
private:
    struct Command
    {
        ScratchTakeLoadResult take;
        ScratchPlaybackMode mode = ScratchPlaybackMode::OriginalWet;
        ScratchChannelMap map;
        ScratchPlaybackDevice device;
        double seekSeconds = 0;
        bool active = false;
    };
    struct Publication
    {
        ScratchPlaybackMode mode = ScratchPlaybackMode::OriginalWet;
        int64_t length = 0, start = 0;
    } publication; // worker writes only after consumer acknowledges retirement
    void run();
    void requestLocked();
    void fail(uint64_t, const juce::String&);
    void beginTail() noexcept;
    void renderTail(float* const*, int, int start, int frames) noexcept;
    ScratchDecoderFactory factory;
    ScratchPcmQueue queue;
    mutable std::mutex mutex;
    std::condition_variable wake;
    Command command;
    juce::String message;
    std::atomic<uint64_t> requested{0}, acknowledged{0}, published{0}, workerIdle{0}, ended{0};
    std::atomic<bool> quitting{false}, deviceStopped{true}, wanted{false}, paused{false}, looping{false}, buffering{false}, eof{false};
    std::atomic<ScratchPlaybackState> state{ScratchPlaybackState::Idle};
    std::atomic<int64_t> position{0}, length{0};
    std::atomic<double> volumeDb{-12};
    std::atomic<float> volumeGain{0.25118864f};
    // Sole-consumer state, changed by prepare/deviceStopped only with callbacks removed.
    uint64_t seenRequest = 0, playingGeneration = 0;
    ScratchPlaybackMode audioMode = ScratchPlaybackMode::OriginalWet;
    int fadeFrames = 240, tailRemaining = 0, fadeInRemaining = 0;
    bool held = false;
    std::array<float, 16> last{}, tailStart{};
    std::thread worker;
};
