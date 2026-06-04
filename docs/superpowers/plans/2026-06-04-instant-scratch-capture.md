# Instant Scratch Capture Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build manual, app-level scratch recording that saves synchronized raw DI and wet Pedalboard output take bundles without adding graph nodes.

**Architecture:** Add a small app-level `ScratchRecorder` service with a testable state machine, `ThreadedWriter`-backed WAV sinks, a `ScratchTake` metadata model, and a compact `ScratchPanel`. `MeteringProcessorPlayer` supplies pre-gain input buffers and post-output-gain buffers, while `MainPanel` owns user commands, patch context, storage settings, and UI feedback.

**Tech Stack:** C++17, JUCE 8 audio/file/UI APIs, `juce::AudioFormatWriter::ThreadedWriter`, `nlohmann_json`, Catch2, CMake.

---

## Constraints And Reference Points

- Do not add a third-party library for v1.
- Preserve `RecorderProcessor` and `LooperProcessor`; do not turn scratch capture into a graph node.
- Keep audio callback work RT-safe: atomic reads, pointer passing, and `ThreadedWriter::write()` only.
- Raw capture point: `MeteringProcessorPlayer::audioDeviceIOCallbackWithContext()` before master input gain is applied.
- Wet capture point: same callback after graph processing, master bus insert, and output gain.
- Existing writer pattern: `src/RecorderProcessor.cpp` and `src/LooperProcessor.cpp`.
- Existing main UI/commands: `src/MainPanel.h`, `src/MainPanel.cpp`.
- Existing test target: `tests/CMakeLists.txt`, executable `Pedalboard3_Tests`.

## File Structure

- Create `src/ScratchTake.h`: value types for take context, take metadata, path sanitizing, unique take-folder creation, and JSON serialization.
- Create `src/ScratchTake.cpp`: implementation of `ScratchTake`.
- Create `src/ScratchRecorder.h`: recorder state machine, sink interface, status model, and public audio-thread write methods.
- Create `src/ScratchRecorder.cpp`: `ThreadedWavSink`, start/stop handling, metadata finalization, writer ownership, and recent-take tracking.
- Create `src/ScratchPanel.h`: compact panel component for recording state and recent takes.
- Create `src/ScratchPanel.cpp`: panel UI, button callbacks, timer refresh.
- Create `tests/scratch_recorder_test.cpp`: unit tests for take naming, metadata, state transitions, and raw/wet sample sync.
- Modify `src/MainPanel.h`: include scratch headers, add command IDs at the end of the enum, add UI members, add scratch helper methods, and let `MeteringProcessorPlayer` hold a `ScratchRecorder*`.
- Modify `src/MainPanel.cpp`: create scratch recorder/control, route taps, add commands/menu items, implement UI actions, and update footer layout.
- Modify `CMakeLists.txt`: add scratch source files to `Pedalboard3`.
- Modify `tests/CMakeLists.txt`: add `scratch_recorder_test.cpp`, `ScratchTake.cpp`, and `ScratchRecorder.cpp` to the test executable.

---

### Task 1: Add ScratchTake Metadata And Storage Model

**Files:**
- Create: `src/ScratchTake.h`
- Create: `src/ScratchTake.cpp`
- Create: `tests/scratch_recorder_test.cpp`
- Modify: `tests/CMakeLists.txt`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Write failing metadata/path tests**

Add `tests/scratch_recorder_test.cpp`:

```cpp
#include "ScratchTake.h"

#include <catch2/catch_test_macros.hpp>
#include <nlohmann/json.hpp>

TEST_CASE("ScratchTake sanitizes filesystem path segments", "[scratch]")
{
    REQUIRE(ScratchTake::sanitisePathSegment("Clean Patch") == "Clean Patch");
    REQUIRE(ScratchTake::sanitisePathSegment("Amp: Lead / Room?") == "Amp Lead  Room");
    REQUIRE(ScratchTake::sanitisePathSegment("   ") == "untitled");
}

TEST_CASE("ScratchTake creates stable raw wet and metadata paths", "[scratch]")
{
    auto root = juce::File::getSpecialLocation(juce::File::tempDirectory)
                    .getChildFile("Pedalboard3ScratchTakeTest")
                    .getNonexistentChildFile("take-root", "");
    root.createDirectory();

    ScratchTakeContext context;
    context.rootDirectory = root;
    context.patchName = "Lead:Idea";
    context.patchIndex = 2;
    context.documentPath = "C:/rigs/live.pdl";
    context.deviceName = "Test Device";
    context.sampleRate = 48000.0;
    context.rawChannelCount = 1;
    context.wetChannelCount = 2;
    context.masterInputGainDb = -3.0;
    context.masterOutputGainDb = -6.0;
    context.startTime = juce::Time(2026, 6, 4, 1, 2, 3, 0, false);

    auto take = ScratchTake::createPending(context);
    take.durationSamples = 96000;
    take.complete = true;

    REQUIRE(take.takeDirectory.isDirectory());
    REQUIRE(take.rawFile == take.takeDirectory.getChildFile("raw.wav"));
    REQUIRE(take.wetFile == take.takeDirectory.getChildFile("wet.wav"));
    REQUIRE(take.metadataFile == take.takeDirectory.getChildFile("take.json"));
    REQUIRE(take.rawChannelCount == 1);
    REQUIRE(take.wetChannelCount == 2);

    auto parsed = nlohmann::json::parse(take.toJsonString().toStdString());
    REQUIRE(parsed["durationSamples"] == 96000);
    REQUIRE(parsed["durationSeconds"] == 2.0);
    REQUIRE(parsed["patchName"] == "Lead:Idea");
    REQUIRE(parsed["rawFile"].get<std::string>().find("raw.wav") != std::string::npos);
    REQUIRE(parsed["wetFile"].get<std::string>().find("wet.wav") != std::string::npos);

    root.deleteRecursively();
}

TEST_CASE("ScratchTake appends suffix for colliding take folders", "[scratch]")
{
    auto root = juce::File::getSpecialLocation(juce::File::tempDirectory)
                    .getChildFile("Pedalboard3ScratchCollisionTest")
                    .getNonexistentChildFile("take-root", "");
    root.createDirectory();

    ScratchTakeContext context;
    context.rootDirectory = root;
    context.patchName = "Same Patch";
    context.patchIndex = 0;
    context.sampleRate = 44100.0;
    context.rawChannelCount = 1;
    context.wetChannelCount = 2;
    context.startTime = juce::Time(2026, 6, 4, 1, 2, 3, 0, false);

    auto first = ScratchTake::createPending(context);
    auto second = ScratchTake::createPending(context);

    REQUIRE(first.takeDirectory != second.takeDirectory);
    REQUIRE(second.takeDirectory.getFileName().endsWith("-02"));

    root.deleteRecursively();
}
```

- [ ] **Step 2: Wire test file into CMake**

In `tests/CMakeLists.txt`, add:

```cmake
    scratch_recorder_test.cpp
    ../src/ScratchTake.cpp
```

In `CMakeLists.txt`, add app sources near other audio-core files:

```cmake
    src/ScratchTake.cpp
    src/ScratchTake.h
```

- [ ] **Step 3: Run test build and verify failure**

Run:

```powershell
cmake --build build --config Release --target Pedalboard3_Tests -- /m:1
```

Expected: compile fails because `ScratchTake.h` does not exist.

- [ ] **Step 4: Implement `ScratchTake.h`**

Create `src/ScratchTake.h`:

```cpp
#pragma once

#include <JuceHeader.h>
#include <cstdint>

struct ScratchTakeContext
{
    juce::File rootDirectory;
    juce::String patchName;
    int patchIndex = 0;
    juce::String documentPath;
    juce::String deviceName;
    double sampleRate = 44100.0;
    int rawChannelCount = 0;
    int wetChannelCount = 0;
    double masterInputGainDb = 0.0;
    double masterOutputGainDb = 0.0;
    juce::Time startTime = juce::Time::getCurrentTime();
};

struct ScratchTake
{
    juce::String takeId;
    juce::Time startTime;
    juce::File takeDirectory;
    juce::File rawFile;
    juce::File wetFile;
    juce::File metadataFile;
    juce::String patchName;
    int patchIndex = 0;
    juce::String documentPath;
    juce::String deviceName;
    double sampleRate = 44100.0;
    int rawChannelCount = 0;
    int wetChannelCount = 0;
    double masterInputGainDb = 0.0;
    double masterOutputGainDb = 0.0;
    uint64_t durationSamples = 0;
    bool complete = false;
    juce::String failureReason;

    static juce::String sanitisePathSegment(const juce::String& text);
    static ScratchTake createPending(const ScratchTakeContext& context);

    double durationSeconds() const noexcept;
    juce::String toJsonString() const;
    bool writeMetadata() const;
};
```

- [ ] **Step 5: Implement `ScratchTake.cpp`**

Create `src/ScratchTake.cpp`:

```cpp
#include "ScratchTake.h"

#include <nlohmann/json.hpp>

namespace
{
juce::String makeTimestampId(const juce::Time& time)
{
    return time.formatted("%Y%m%d-%H%M%S");
}

juce::String makeDateFolder(const juce::Time& time)
{
    return time.formatted("%Y-%m-%d");
}
}

juce::String ScratchTake::sanitisePathSegment(const juce::String& text)
{
    juce::String cleaned;
    for (auto c : text)
    {
        if (c == '<' || c == '>' || c == ':' || c == '"' || c == '/' || c == '\\' || c == '|' || c == '?' || c == '*')
            continue;
        cleaned << c;
    }

    cleaned = cleaned.trim();
    return cleaned.isEmpty() ? "untitled" : cleaned;
}

ScratchTake ScratchTake::createPending(const ScratchTakeContext& context)
{
    ScratchTake take;
    take.startTime = context.startTime;
    take.takeId = makeTimestampId(context.startTime);
    take.patchName = context.patchName;
    take.patchIndex = context.patchIndex;
    take.documentPath = context.documentPath;
    take.deviceName = context.deviceName;
    take.sampleRate = context.sampleRate;
    take.rawChannelCount = context.rawChannelCount;
    take.wetChannelCount = context.wetChannelCount;
    take.masterInputGainDb = context.masterInputGainDb;
    take.masterOutputGainDb = context.masterOutputGainDb;

    auto root = context.rootDirectory;
    if (root == juce::File())
        root = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                   .getChildFile("Pedalboard3")
                   .getChildFile("Scratch Ideas");

    auto dayFolder = root.getChildFile(makeDateFolder(context.startTime));
    dayFolder.createDirectory();

    const auto baseName = makeTimestampId(context.startTime) + "-" + sanitisePathSegment(context.patchName);
    auto candidate = dayFolder.getChildFile(baseName);
    int suffix = 2;
    while (candidate.exists())
    {
        candidate = dayFolder.getChildFile(baseName + "-" + juce::String(suffix).paddedLeft('0', 2));
        ++suffix;
    }

    candidate.createDirectory();
    take.takeDirectory = candidate;
    take.rawFile = candidate.getChildFile("raw.wav");
    take.wetFile = candidate.getChildFile("wet.wav");
    take.metadataFile = candidate.getChildFile("take.json");
    return take;
}

double ScratchTake::durationSeconds() const noexcept
{
    return sampleRate > 0.0 ? static_cast<double>(durationSamples) / sampleRate : 0.0;
}

juce::String ScratchTake::toJsonString() const
{
    nlohmann::json json;
    json["takeId"] = takeId.toStdString();
    json["startTimestamp"] = startTime.toISO8601(true).toStdString();
    json["durationSamples"] = durationSamples;
    json["durationSeconds"] = durationSeconds();
    json["sampleRate"] = sampleRate;
    json["rawChannelCount"] = rawChannelCount;
    json["wetChannelCount"] = wetChannelCount;
    json["deviceName"] = deviceName.toStdString();
    json["documentPath"] = documentPath.toStdString();
    json["patchIndex"] = patchIndex;
    json["patchName"] = patchName.toStdString();
    json["masterInputGainDb"] = masterInputGainDb;
    json["masterOutputGainDb"] = masterOutputGainDb;
    json["rawFile"] = rawFile.getFullPathName().toStdString();
    json["wetFile"] = wetFile.getFullPathName().toStdString();
    json["complete"] = complete;
    json["failureReason"] = failureReason.toStdString();
    return juce::String(json.dump(2));
}

bool ScratchTake::writeMetadata() const
{
    return metadataFile.replaceWithText(toJsonString());
}
```

- [ ] **Step 6: Run tests and commit**

Run:

```powershell
cmake --build build --config Release --target Pedalboard3_Tests -- /m:1
ctest --test-dir build -C Release --output-on-failure -R Scratch
```

Expected: scratch tests pass.

Commit:

```powershell
git add CMakeLists.txt tests/CMakeLists.txt tests/scratch_recorder_test.cpp src/ScratchTake.h src/ScratchTake.cpp
git commit -m "feat: add scratch take metadata model"
```

---

### Task 2: Add Testable ScratchRecorder State Machine

**Files:**
- Create: `src/ScratchRecorder.h`
- Create: `src/ScratchRecorder.cpp`
- Modify: `tests/scratch_recorder_test.cpp`
- Modify: `tests/CMakeLists.txt`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Add failing state-machine tests**

Append to `tests/scratch_recorder_test.cpp`:

```cpp
#include "ScratchRecorder.h"

namespace
{
class MemoryScratchSink final : public ScratchAudioSink
{
  public:
    bool open(const juce::File&, double, int channelsToUse, juce::TimeSliceThread&) override
    {
        channels = channelsToUse;
        opened = channelsToUse > 0;
        return opened;
    }

    bool write(const float* const* data, int numChannels, int numSamples) noexcept override
    {
        if (!opened || data == nullptr || numChannels <= 0 || numSamples <= 0)
            return false;
        samplesWritten += static_cast<uint64_t>(numSamples);
        channels = numChannels;
        return true;
    }

    void close() override { opened = false; }
    uint64_t getSamplesWritten() const noexcept override { return samplesWritten; }

    bool opened = false;
    int channels = 0;
    uint64_t samplesWritten = 0;
};

class MemorySinkFactory final : public ScratchAudioSinkFactory
{
  public:
    std::unique_ptr<ScratchAudioSink> create() override
    {
        auto sink = std::make_unique<MemoryScratchSink>();
        sinks.add(sink.get());
        return sink;
    }

    juce::Array<MemoryScratchSink*> sinks;
};
}

TEST_CASE("ScratchRecorder records raw and wet blocks with matching sample counts", "[scratch]")
{
    auto root = juce::File::getSpecialLocation(juce::File::tempDirectory)
                    .getChildFile("Pedalboard3ScratchRecorderTest")
                    .getNonexistentChildFile("take-root", "");
    root.createDirectory();

    MemorySinkFactory factory;
    ScratchRecorder recorder(factory);

    ScratchTakeContext context;
    context.rootDirectory = root;
    context.patchName = "Recorder Test";
    context.sampleRate = 48000.0;
    context.rawChannelCount = 1;
    context.wetChannelCount = 2;

    REQUIRE(recorder.start(context));
    REQUIRE(recorder.getStatus().state == ScratchRecorderState::Recording);

    float raw[4] = {0.1f, 0.2f, 0.3f, 0.4f};
    const float* rawPtrs[1] = {raw};
    float wetL[4] = {0.5f, 0.6f, 0.7f, 0.8f};
    float wetR[4] = {0.9f, 1.0f, 0.9f, 0.8f};
    float* wetPtrs[2] = {wetL, wetR};

    recorder.writeRawBlock(rawPtrs, 1, 4);
    recorder.writeWetBlock(wetPtrs, 2, 4);
    recorder.requestStop();
    recorder.finishPendingStopForTests();

    auto status = recorder.getStatus();
    REQUIRE(status.state == ScratchRecorderState::Saved);
    REQUIRE(status.elapsedSamples == 4);
    REQUIRE(status.rawSamplesWritten == 4);
    REQUIRE(status.wetSamplesWritten == 4);
    REQUIRE(status.lastTake.has_value());
    REQUIRE(status.lastTake->complete);
    REQUIRE(status.recentTakes.size() == 1);
    REQUIRE(status.recentTakes.front().complete);

    root.deleteRecursively();
}

TEST_CASE("ScratchRecorder refuses missing input or output channels", "[scratch]")
{
    MemorySinkFactory factory;
    ScratchRecorder recorder(factory);

    ScratchTakeContext context;
    context.sampleRate = 44100.0;
    context.rawChannelCount = 0;
    context.wetChannelCount = 2;
    REQUIRE_FALSE(recorder.start(context));
    REQUIRE(recorder.getStatus().state == ScratchRecorderState::Failed);

    context.rawChannelCount = 1;
    context.wetChannelCount = 0;
    REQUIRE_FALSE(recorder.start(context));
    REQUIRE(recorder.getStatus().state == ScratchRecorderState::Failed);
}
```

- [ ] **Step 2: Wire recorder files into CMake**

In `CMakeLists.txt`, add:

```cmake
    src/ScratchRecorder.cpp
    src/ScratchRecorder.h
```

In `tests/CMakeLists.txt`, add:

```cmake
    ../src/ScratchRecorder.cpp
```

- [ ] **Step 3: Run build and verify failure**

Run:

```powershell
cmake --build build --config Release --target Pedalboard3_Tests -- /m:1
```

Expected: compile fails because `ScratchRecorder.h` does not exist.

- [ ] **Step 4: Implement `ScratchRecorder.h`**

Create `src/ScratchRecorder.h`:

```cpp
#pragma once

#include "ScratchTake.h"

#include <JuceHeader.h>
#include <atomic>
#include <memory>
#include <optional>
#include <vector>

enum class ScratchRecorderState
{
    Ready,
    Recording,
    Saving,
    Saved,
    Failed
};

struct ScratchRecorderStatus
{
    ScratchRecorderState state = ScratchRecorderState::Ready;
    juce::String message = "Ready";
    uint64_t elapsedSamples = 0;
    uint64_t rawSamplesWritten = 0;
    uint64_t wetSamplesWritten = 0;
    std::optional<ScratchTake> lastTake;
    std::vector<ScratchTake> recentTakes;
};

class ScratchAudioSink
{
  public:
    virtual ~ScratchAudioSink() = default;
    virtual bool open(const juce::File& file, double sampleRate, int channels, juce::TimeSliceThread& thread) = 0;
    virtual bool write(const float* const* data, int numChannels, int numSamples) noexcept = 0;
    virtual void close() = 0;
    virtual uint64_t getSamplesWritten() const noexcept = 0;
};

class ScratchAudioSinkFactory
{
  public:
    virtual ~ScratchAudioSinkFactory() = default;
    virtual std::unique_ptr<ScratchAudioSink> create() = 0;
};

class ThreadedWavSinkFactory final : public ScratchAudioSinkFactory
{
  public:
    std::unique_ptr<ScratchAudioSink> create() override;
};

class ScratchRecorder final : private juce::AsyncUpdater
{
  public:
    explicit ScratchRecorder(ScratchAudioSinkFactory& factory);
    ~ScratchRecorder() override;

    bool start(const ScratchTakeContext& context);
    void requestStop();
    void stopForDeviceChange();

    void writeRawBlock(const float* const* inputChannelData, int numInputChannels, int numSamples) noexcept;
    void writeWetBlock(float* const* outputChannelData, int numOutputChannels, int numSamples) noexcept;

    ScratchRecorderStatus getStatus() const;
    bool isRecording() const noexcept;
    juce::File getScratchRoot() const;
    void setScratchRoot(const juce::File& root);

    void finishPendingStopForTests();

  private:
    void handleAsyncUpdate() override;
    void finishStop();
    void failStart(const juce::String& message);

    ScratchAudioSinkFactory& sinkFactory;
    juce::TimeSliceThread writerThread{"Scratch Recorder Writer"};
    juce::CriticalSection stateLock;
    std::unique_ptr<ScratchAudioSink> rawSink;
    std::unique_ptr<ScratchAudioSink> wetSink;
    ScratchTake currentTake;
    ScratchRecorderStatus status;
    std::vector<ScratchTake> recentTakes;
    juce::File scratchRoot;
    std::atomic<int> state{static_cast<int>(ScratchRecorderState::Ready)};
    std::atomic<uint64_t> rawSamplesWritten{0};
    std::atomic<uint64_t> wetSamplesWritten{0};
    std::atomic<bool> stopRequested{false};
    std::atomic<bool> writeError{false};
};
```

- [ ] **Step 5: Implement `ScratchRecorder.cpp`**

Create `src/ScratchRecorder.cpp`:

```cpp
#include "ScratchRecorder.h"

#include "AudioSingletons.h"
#include "SettingsManager.h"

#include <spdlog/spdlog.h>

namespace
{
class ThreadedWavSink final : public ScratchAudioSink
{
  public:
    bool open(const juce::File& file, double sampleRate, int channels, juce::TimeSliceThread& thread) override
    {
        if (channels <= 0 || sampleRate <= 0.0)
            return false;

        auto stream = std::make_unique<juce::FileOutputStream>(file);
        if (!stream->openedOk())
            return false;

        juce::WavAudioFormat wavFormat;
        juce::StringPairArray metadata;
        std::unique_ptr<juce::AudioFormatWriter> writer(
            wavFormat.createWriterFor(stream.get(), sampleRate, static_cast<unsigned int>(channels), 24, metadata, 0));
        if (!writer)
            return false;

        stream.release();
        threadedWriter = std::make_unique<juce::AudioFormatWriter::ThreadedWriter>(writer.release(), thread, 32768);
        channelCount = channels;
        return true;
    }

    bool write(const float* const* data, int numChannels, int numSamples) noexcept override
    {
        if (!threadedWriter || data == nullptr || numChannels <= 0 || numSamples <= 0)
            return false;

        const auto channelsToWrite = juce::jmin(numChannels, channelCount);
        if (channelsToWrite <= 0)
            return false;

        const bool ok = threadedWriter->write(data, numSamples);
        if (ok)
            samplesWritten += static_cast<uint64_t>(numSamples);
        return ok;
    }

    void close() override { threadedWriter.reset(); }
    uint64_t getSamplesWritten() const noexcept override { return samplesWritten; }

  private:
    std::unique_ptr<juce::AudioFormatWriter::ThreadedWriter> threadedWriter;
    int channelCount = 0;
    std::atomic<uint64_t> samplesWritten{0};
};
}

std::unique_ptr<ScratchAudioSink> ThreadedWavSinkFactory::create()
{
    return std::make_unique<ThreadedWavSink>();
}

ScratchRecorder::ScratchRecorder(ScratchAudioSinkFactory& factory) : sinkFactory(factory)
{
    writerThread.startThread();
}

ScratchRecorder::~ScratchRecorder()
{
    requestStop();
    finishStop();
    writerThread.stopThread(1000);
}

bool ScratchRecorder::start(const ScratchTakeContext& context)
{
    const juce::ScopedLock lock(stateLock);
    if (isRecording())
        return false;

    if (context.rawChannelCount <= 0)
    {
        failStart("No input channels available");
        return false;
    }

    if (context.wetChannelCount <= 0)
    {
        failStart("No output channels available");
        return false;
    }

    ScratchTakeContext resolved = context;
    if (resolved.rootDirectory == juce::File())
        resolved.rootDirectory = getScratchRoot();

    currentTake = ScratchTake::createPending(resolved);
    rawSink = sinkFactory.create();
    wetSink = sinkFactory.create();

    if (!rawSink || !wetSink ||
        !rawSink->open(currentTake.rawFile, currentTake.sampleRate, currentTake.rawChannelCount, writerThread) ||
        !wetSink->open(currentTake.wetFile, currentTake.sampleRate, currentTake.wetChannelCount, writerThread))
    {
        if (rawSink)
            rawSink->close();
        if (wetSink)
            wetSink->close();
        rawSink.reset();
        wetSink.reset();
        currentTake.failureReason = "Could not create scratch WAV writers";
        currentTake.writeMetadata();
        status.state = ScratchRecorderState::Failed;
        status.message = currentTake.failureReason;
        recentTakes.insert(recentTakes.begin(), currentTake);
        if (recentTakes.size() > 8)
            recentTakes.resize(8);
        status.lastTake = currentTake;
        status.recentTakes = recentTakes;
        state.store(static_cast<int>(ScratchRecorderState::Failed), std::memory_order_release);
        return false;
    }

    rawSamplesWritten.store(0, std::memory_order_relaxed);
    wetSamplesWritten.store(0, std::memory_order_relaxed);
    stopRequested.store(false, std::memory_order_release);
    writeError.store(false, std::memory_order_release);
    status.state = ScratchRecorderState::Recording;
    status.message = "Recording";
    status.elapsedSamples = 0;
    status.rawSamplesWritten = 0;
    status.wetSamplesWritten = 0;
    status.lastTake.reset();
    status.recentTakes = recentTakes;
    state.store(static_cast<int>(ScratchRecorderState::Recording), std::memory_order_release);
    return true;
}

void ScratchRecorder::requestStop()
{
    if (isRecording())
    {
        stopRequested.store(true, std::memory_order_release);
        triggerAsyncUpdate();
    }
}

void ScratchRecorder::stopForDeviceChange()
{
    if (isRecording())
    {
        currentTake.failureReason = "Audio device changed during scratch capture";
        requestStop();
    }
}

void ScratchRecorder::writeRawBlock(const float* const* inputChannelData, int numInputChannels, int numSamples) noexcept
{
    if (!isRecording() || stopRequested.load(std::memory_order_acquire))
        return;

    auto* sink = rawSink.get();
    if (sink == nullptr || !sink->write(inputChannelData, numInputChannels, numSamples))
        writeError.store(true, std::memory_order_release);
    else
        rawSamplesWritten.store(sink->getSamplesWritten(), std::memory_order_relaxed);
}

void ScratchRecorder::writeWetBlock(float* const* outputChannelData, int numOutputChannels, int numSamples) noexcept
{
    if (!isRecording() || stopRequested.load(std::memory_order_acquire))
        return;

    auto* sink = wetSink.get();
    if (sink == nullptr || !sink->write(const_cast<const float* const*>(outputChannelData), numOutputChannels, numSamples))
        writeError.store(true, std::memory_order_release);
    else
        wetSamplesWritten.store(sink->getSamplesWritten(), std::memory_order_relaxed);
}

ScratchRecorderStatus ScratchRecorder::getStatus() const
{
    const juce::ScopedLock lock(stateLock);
    auto copy = status;
    copy.rawSamplesWritten = rawSamplesWritten.load(std::memory_order_relaxed);
    copy.wetSamplesWritten = wetSamplesWritten.load(std::memory_order_relaxed);
    copy.elapsedSamples = juce::jmin(copy.rawSamplesWritten, copy.wetSamplesWritten);
    copy.state = static_cast<ScratchRecorderState>(state.load(std::memory_order_acquire));
    return copy;
}

bool ScratchRecorder::isRecording() const noexcept
{
    return state.load(std::memory_order_acquire) == static_cast<int>(ScratchRecorderState::Recording);
}

juce::File ScratchRecorder::getScratchRoot() const
{
    auto configured = SettingsManager::getInstance().getString("ScratchCaptureFolder");
    if (configured.isNotEmpty())
        return juce::File(configured);
    return SettingsManager::getInstance().getUserDataDirectory().getChildFile("Scratch Ideas");
}

void ScratchRecorder::setScratchRoot(const juce::File& root)
{
    scratchRoot = root;
    SettingsManager::getInstance().setValue("ScratchCaptureFolder", root.getFullPathName());
}

void ScratchRecorder::finishPendingStopForTests()
{
    finishStop();
}

void ScratchRecorder::handleAsyncUpdate()
{
    finishStop();
}

void ScratchRecorder::finishStop()
{
    const juce::ScopedLock lock(stateLock);
    if (state.load(std::memory_order_acquire) != static_cast<int>(ScratchRecorderState::Recording))
        return;

    state.store(static_cast<int>(ScratchRecorderState::Saving), std::memory_order_release);
    status.state = ScratchRecorderState::Saving;
    status.message = "Saving";

    if (rawSink)
    {
        rawSamplesWritten.store(rawSink->getSamplesWritten(), std::memory_order_relaxed);
        rawSink->close();
    }
    if (wetSink)
    {
        wetSamplesWritten.store(wetSink->getSamplesWritten(), std::memory_order_relaxed);
        wetSink->close();
    }

    rawSink.reset();
    wetSink.reset();

    const auto rawSamples = rawSamplesWritten.load(std::memory_order_relaxed);
    const auto wetSamples = wetSamplesWritten.load(std::memory_order_relaxed);
    currentTake.durationSamples = juce::jmin(rawSamples, wetSamples);
    currentTake.complete = !writeError.load(std::memory_order_acquire) && rawSamples == wetSamples;
    if (!currentTake.complete && currentTake.failureReason.isEmpty())
        currentTake.failureReason = "Raw and wet scratch files were not completed with matching sample counts";

    currentTake.writeMetadata();
    recentTakes.insert(recentTakes.begin(), currentTake);
    if (recentTakes.size() > 8)
        recentTakes.resize(8);
    status.lastTake = currentTake;
    status.recentTakes = recentTakes;
    status.rawSamplesWritten = rawSamples;
    status.wetSamplesWritten = wetSamples;
    status.elapsedSamples = currentTake.durationSamples;
    status.state = currentTake.complete ? ScratchRecorderState::Saved : ScratchRecorderState::Failed;
    status.message = currentTake.complete ? "Saved" : currentTake.failureReason;
    state.store(static_cast<int>(status.state), std::memory_order_release);
}

void ScratchRecorder::failStart(const juce::String& message)
{
    status = {};
    status.state = ScratchRecorderState::Failed;
    status.message = message;
    status.recentTakes = recentTakes;
    state.store(static_cast<int>(ScratchRecorderState::Failed), std::memory_order_release);
}
```

- [ ] **Step 6: Run tests and commit**

Run:

```powershell
cmake --build build --config Release --target Pedalboard3_Tests -- /m:1
ctest --test-dir build -C Release --output-on-failure -R Scratch
```

Expected: scratch tests pass.

Commit:

```powershell
git add CMakeLists.txt tests/CMakeLists.txt tests/scratch_recorder_test.cpp src/ScratchRecorder.h src/ScratchRecorder.cpp
git commit -m "feat: add scratch recorder core"
```

---

### Task 3: Hook Raw And Wet Audio Callback Taps

**Files:**
- Modify: `src/MainPanel.h`
- Modify: `src/MainPanel.cpp`
- Modify: `tests/scratch_recorder_test.cpp`

- [ ] **Step 1: Add a recorder pointer to `MeteringProcessorPlayer`**

In `src/MainPanel.h`, include `ScratchRecorder.h` and add this public setter to `MeteringProcessorPlayer`:

```cpp
    void setScratchRecorder(ScratchRecorder* recorderToUse) noexcept
    {
        scratchRecorder.store(recorderToUse, std::memory_order_release);
    }
```

Add this private member:

```cpp
    std::atomic<ScratchRecorder*> scratchRecorder{nullptr};
```

- [ ] **Step 2: Tap raw input before input gain**

At the start of `MeteringProcessorPlayer::audioDeviceIOCallbackWithContext()`, after `auto& gainState = MasterGainState::getInstance();`, add:

```cpp
        if (auto* recorder = scratchRecorder.load(std::memory_order_acquire))
            recorder->writeRawBlock(inputChannelData, numInputChannels, numSamples);
```

This must remain before the block that creates `actualInput`.

- [ ] **Step 3: Tap wet output after output gain**

After the output gain loop and before safety limiter metering, add:

```cpp
        if (auto* recorder = scratchRecorder.load(std::memory_order_acquire))
            recorder->writeWetBlock(outputChannelData, numOutputChannels, numSamples);
```

- [ ] **Step 4: Add a compile test**

Run:

```powershell
cmake --build build --config Release --target Pedalboard3 -- /m:1
```

Expected: app target builds.

- [ ] **Step 5: Commit**

```powershell
git add src/MainPanel.h
git commit -m "feat: tap scratch recorder in audio callback"
```

---

### Task 4: MainPanel Ownership, Commands, And Patch Context

**Files:**
- Modify: `src/MainPanel.h`
- Modify: `src/MainPanel.cpp`

- [ ] **Step 1: Add MainPanel members and command IDs**

Append new command IDs to the end of the `MainPanel` command enum in `src/MainPanel.h`:

```cpp
        OptionsUiScaleResetDefault,
        ScratchCaptureToggle,
        ScratchPanelOpen,
        ScratchRevealFolder
```

Add private members:

```cpp
    ThreadedWavSinkFactory scratchSinkFactory;
    ScratchRecorder scratchRecorder{scratchSinkFactory};
    std::unique_ptr<Drawable> scratchRecordImage;
    std::unique_ptr<Drawable> scratchStopImage;
    TextButton* scratchRecordButton = nullptr;
    TextButton* scratchPanelButton = nullptr;
    Label* scratchStatusLabel = nullptr;
```

Add private helper declarations:

```cpp
    ScratchTakeContext createScratchTakeContext() const;
    void toggleScratchCapture();
    void openScratchPanel();
    void revealScratchFolder();
    void refreshScratchControls();
```

- [ ] **Step 2: Construct visible scratch controls**

In the `MainPanel` constructor after `uiScaleFooterComboBox` setup, add:

```cpp
    addAndMakeVisible(scratchRecordButton = new TextButton("scratchRecordButton"));
    scratchRecordButton->setButtonText("REC");
    scratchRecordButton->setTooltip("Record scratch idea");
    scratchRecordButton->addListener(this);

    addAndMakeVisible(scratchStatusLabel = new Label("scratchStatusLabel", "Ready"));
    scratchStatusLabel->setFont(FontManager::getInstance().getLabelFont());
    scratchStatusLabel->setJustificationType(Justification::centredLeft);
    scratchStatusLabel->setTooltip("Scratch capture status");

    addAndMakeVisible(scratchPanelButton = new TextButton("scratchPanelButton"));
    scratchPanelButton->setButtonText("Takes");
    scratchPanelButton->setTooltip("Open scratch takes");
    scratchPanelButton->addListener(this);
```

After `graphPlayer.setProcessor(&signalPath.getGraph());`, add:

```cpp
    graphPlayer.setScratchRecorder(&scratchRecorder);
```

In the destructor, before audio callbacks are removed or the graph player is cleared, add:

```cpp
    scratchRecorder.requestStop();
    graphPlayer.setScratchRecorder(nullptr);
```

- [ ] **Step 3: Add commands and menu entries**

In `getAllCommands()`, append:

```cpp
                             ScratchCaptureToggle,
                             ScratchPanelOpen,
                             ScratchRevealFolder
```

In `getMenuForIndex()` under the File menu after `FileSaveAs`, add:

```cpp
        retval.addSeparator();
        retval.addCommandItem(commandManager, ScratchCaptureToggle);
        retval.addCommandItem(commandManager, ScratchPanelOpen);
        retval.addCommandItem(commandManager, ScratchRevealFolder);
```

In `getCommandInfo()`, add:

```cpp
    case ScratchCaptureToggle:
        result.setInfo("Start/Stop Scratch Capture", "Records synchronized raw and wet scratch WAV files.", fileCategory, 0);
        result.addDefaultKeypress(L'r', ModifierKeys::commandModifier | ModifierKeys::shiftModifier);
        break;
    case ScratchPanelOpen:
        result.setInfo("Open Scratch Panel", "Shows recent scratch takes and capture status.", fileCategory, 0);
        break;
    case ScratchRevealFolder:
        result.setInfo("Reveal Scratch Ideas Folder", "Opens the scratch ideas folder.", fileCategory, 0);
        break;
```

In `perform()`, add:

```cpp
    case ScratchCaptureToggle:
        toggleScratchCapture();
        break;
    case ScratchPanelOpen:
        openScratchPanel();
        break;
    case ScratchRevealFolder:
        revealScratchFolder();
        break;
```

- [ ] **Step 4: Implement scratch helpers**

Add these methods to `src/MainPanel.cpp` near other `MainPanel` helpers:

```cpp
ScratchTakeContext MainPanel::createScratchTakeContext() const
{
    ScratchTakeContext context;
    context.rootDirectory = scratchRecorder.getScratchRoot();
    context.patchIndex = currentPatch;
    context.patchName = patchComboBox != nullptr ? patchComboBox->getText() : "<untitled>";
    context.documentPath = getFile().getFullPathName();

    if (auto* device = deviceManager.getCurrentAudioDevice())
    {
        context.deviceName = device->getName();
        context.sampleRate = device->getCurrentSampleRate();
        context.rawChannelCount = device->getActiveInputChannels().countNumberOfSetBits();
        context.wetChannelCount = device->getActiveOutputChannels().countNumberOfSetBits();
    }

    auto& gainState = MasterGainState::getInstance();
    context.masterInputGainDb = gainState.masterInputGainDb.load(std::memory_order_relaxed);
    context.masterOutputGainDb = gainState.masterOutputGainDb.load(std::memory_order_relaxed);
    return context;
}

void MainPanel::toggleScratchCapture()
{
    if (scratchRecorder.isRecording())
    {
        scratchRecorder.requestStop();
        showToast("Saving scratch take");
    }
    else if (scratchRecorder.start(createScratchTakeContext()))
    {
        showToast("Scratch recording");
    }
    else
    {
        showToast(scratchRecorder.getStatus().message);
    }

    refreshScratchControls();
}

void MainPanel::openScratchPanel()
{
    auto* panel = new ScratchPanel(scratchRecorder);
    panel->setSize(420, 320);
    JuceHelperStuff::showNonModalDialog("Scratch Takes", panel, this,
                                        ColourScheme::getInstance().colours["Window Background"], true, true);
}

void MainPanel::revealScratchFolder()
{
    scratchRecorder.getScratchRoot().createDirectory();
    scratchRecorder.getScratchRoot().revealToUser();
}

void MainPanel::refreshScratchControls()
{
    auto status = scratchRecorder.getStatus();
    if (scratchRecordButton != nullptr)
        scratchRecordButton->setButtonText(status.state == ScratchRecorderState::Recording ? "STOP" : "REC");
    if (scratchStatusLabel != nullptr)
        scratchStatusLabel->setText(status.message, dontSendNotification);
}
```

- [ ] **Step 5: Wire button clicks**

In `buttonClicked()`, add:

```cpp
    else if (buttonThatWasClicked == scratchRecordButton)
        commandManager->invokeDirectly(ScratchCaptureToggle, true);
    else if (buttonThatWasClicked == scratchPanelButton)
        commandManager->invokeDirectly(ScratchPanelOpen, true);
```

- [ ] **Step 6: Build and commit**

Run:

```powershell
cmake --build build --config Release --target Pedalboard3 -- /m:1
```

Expected: app target builds.

Commit:

```powershell
git add src/MainPanel.h src/MainPanel.cpp
git commit -m "feat: add scratch capture commands"
```

---

### Task 5: ScratchPanel UI

**Files:**
- Create: `src/ScratchPanel.h`
- Create: `src/ScratchPanel.cpp`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Add panel files to CMake**

In `CMakeLists.txt`, add:

```cmake
    src/ScratchPanel.cpp
    src/ScratchPanel.h
```

- [ ] **Step 2: Implement `ScratchPanel.h`**

Create `src/ScratchPanel.h`:

```cpp
#pragma once

#include "ScratchRecorder.h"

#include <JuceHeader.h>

class ScratchPanel final : public juce::Component,
                           private juce::Button::Listener,
                           private juce::Timer
{
  public:
    explicit ScratchPanel(ScratchRecorder& recorderToUse);
    ~ScratchPanel() override;

    void resized() override;
    void paint(juce::Graphics& g) override;

  private:
    void buttonClicked(juce::Button* button) override;
    void timerCallback() override;
    void refresh();

    ScratchRecorder& recorder;
    juce::TextButton recordButton{"recordButton"};
    juce::TextButton revealButton{"revealButton"};
    juce::Label statusLabel{"statusLabel", "Ready"};
    juce::Label elapsedLabel{"elapsedLabel", "00:00"};
    juce::TextEditor recentTakesBox{"recentTakesBox"};
};
```

- [ ] **Step 3: Implement `ScratchPanel.cpp`**

Create `src/ScratchPanel.cpp`:

```cpp
#include "ScratchPanel.h"

#include "ColourScheme.h"
#include "FontManager.h"

ScratchPanel::ScratchPanel(ScratchRecorder& recorderToUse) : recorder(recorderToUse)
{
    addAndMakeVisible(recordButton);
    recordButton.setButtonText("REC");
    recordButton.addListener(this);

    addAndMakeVisible(revealButton);
    revealButton.setButtonText("Reveal");
    revealButton.addListener(this);

    for (auto* label : {&statusLabel, &elapsedLabel})
    {
        addAndMakeVisible(label);
        label->setFont(FontManager::getInstance().getLabelFont());
        label->setJustificationType(juce::Justification::centredLeft);
    }

    addAndMakeVisible(recentTakesBox);
    recentTakesBox.setMultiLine(true);
    recentTakesBox.setReadOnly(true);
    recentTakesBox.setScrollbarsShown(true);
    recentTakesBox.setText("No scratch takes yet", juce::dontSendNotification);

    startTimerHz(10);
    refresh();
}

ScratchPanel::~ScratchPanel()
{
    stopTimer();
    recordButton.removeListener(this);
    revealButton.removeListener(this);
}

void ScratchPanel::paint(juce::Graphics& g)
{
    g.fillAll(ColourScheme::getInstance().colours["Window Background"]);
}

void ScratchPanel::resized()
{
    auto bounds = getLocalBounds().reduced(12);
    auto top = bounds.removeFromTop(48);
    recordButton.setBounds(top.removeFromLeft(96));
    top.removeFromLeft(8);
    statusLabel.setBounds(top.removeFromLeft(160));
    revealButton.setBounds(top.removeFromRight(96));

    bounds.removeFromTop(10);
    elapsedLabel.setBounds(bounds.removeFromTop(28));
    recentTakesBox.setBounds(bounds);
}

void ScratchPanel::buttonClicked(juce::Button* button)
{
    if (button == &recordButton)
    {
        if (recorder.isRecording())
            recorder.requestStop();
    }
    else if (button == &revealButton)
    {
        recorder.getScratchRoot().createDirectory();
        recorder.getScratchRoot().revealToUser();
    }

    refresh();
}

void ScratchPanel::timerCallback()
{
    refresh();
}

void ScratchPanel::refresh()
{
    const auto status = recorder.getStatus();
    recordButton.setButtonText(status.state == ScratchRecorderState::Recording ? "STOP" : "REC");
    statusLabel.setText(status.message, juce::dontSendNotification);

    const auto seconds = status.elapsedSamples > 0 && status.lastTake.has_value() && status.lastTake->sampleRate > 0.0
                             ? static_cast<int>(status.elapsedSamples / status.lastTake->sampleRate)
                             : 0;
    elapsedLabel.setText(juce::String::formatted("%02d:%02d", seconds / 60, seconds % 60),
                         juce::dontSendNotification);

    if (!status.recentTakes.empty())
    {
        juce::String listText;
        for (const auto& take : status.recentTakes)
        {
            listText << take.startTime.formatted("%H:%M:%S") << "  "
                     << (take.patchName.isNotEmpty() ? take.patchName : "<untitled>")
                     << "  raw/wet "
                     << (take.complete ? "saved" : "incomplete")
                     << juce::newLine;
        }
        recentTakesBox.setText(listText.trimEnd(), juce::dontSendNotification);
    }
    else
    {
        recentTakesBox.setText("No scratch takes yet", juce::dontSendNotification);
    }
}
```

- [ ] **Step 4: Build and commit**

Run:

```powershell
cmake --build build --config Release --target Pedalboard3 -- /m:1
```

Expected: app target builds.

Commit:

```powershell
git add CMakeLists.txt src/ScratchPanel.h src/ScratchPanel.cpp src/MainPanel.cpp
git commit -m "feat: add scratch capture panel"
```

---

### Task 6: Footer Layout Integration

**Files:**
- Modify: `src/MainPanel.cpp`

- [ ] **Step 1: Add scratch controls to visibility reset**

In `MainPanel::resized()`, near existing footer visibility setup, add:

```cpp
    scratchRecordButton->setVisible(true);
    scratchStatusLabel->setVisible(true);
    scratchPanelButton->setVisible(true);
```

- [ ] **Step 2: Add scratch layout lambda**

Near the footer layout lambdas in `MainPanel::resized()`, add:

```cpp
    auto layoutScratchControls = [this, gap, controlH](int left, int controlY, int areaW)
    {
        const int recW = 52;
        const int panelW = 56;
        const int minStatusW = 64;

        if (areaW >= recW + gap + minStatusW + gap + panelW)
        {
            int x = left;
            scratchRecordButton->setVisible(true);
            scratchRecordButton->setBounds(x, controlY, recW, controlH);
            x += recW + gap;
            scratchStatusLabel->setVisible(true);
            scratchStatusLabel->setBounds(x, controlY, areaW - recW - panelW - gap * 2, controlH);
            scratchPanelButton->setVisible(true);
            scratchPanelButton->setBounds(left + areaW - panelW, controlY, panelW, controlH);
        }
        else if (areaW >= recW + gap + panelW)
        {
            scratchRecordButton->setVisible(true);
            scratchRecordButton->setBounds(left, controlY, recW, controlH);
            scratchStatusLabel->setVisible(false);
            scratchPanelButton->setVisible(true);
            scratchPanelButton->setBounds(left + areaW - panelW, controlY, panelW, controlH);
        }
        else
        {
            scratchRecordButton->setVisible(areaW >= recW);
            scratchRecordButton->setBounds(left, controlY, recW, controlH);
            scratchStatusLabel->setVisible(false);
            scratchPanelButton->setVisible(false);
        }
    };
```

- [ ] **Step 3: Place controls without clipping existing footer items**

For `stackedFooter`, place scratch controls on row 3 before gain controls:

```cpp
            layoutScratchControls(8, row3Y, 180);
            layoutGainControls(196, row3Y, footerW - 204, 64);
```

For non-stacked two-row footer, reserve the left of row 2 if enough width exists:

```cpp
            const int scratchW = footerLayoutW >= 780 ? 180 : 112;
            layoutScratchControls(8, row2Y, scratchW);
            const int transportEndX = layoutTransportControls(8 + scratchW + 8, row2Y);
```

For single-row footer, keep the record button visible and hide status if space is tight:

```cpp
        layoutScratchControls(8, footerY, 112);
        layoutPatchControls(128, footerY, 312);
```

- [ ] **Step 4: Build and run visual regression harness**

Run:

```powershell
cmake --build build --config Release --target Pedalboard3 -- /m:1
cmake --build build --config Release --target Pedalboard3_Tests -- /m:1
ctest --test-dir build -C Release --output-on-failure -R UI
```

Expected: app builds and UI regression tests pass.

- [ ] **Step 5: Commit**

```powershell
git add src/MainPanel.cpp
git commit -m "feat: keep scratch capture visible in footer"
```

---

### Task 7: Device Change And Patch Switch Safety

**Files:**
- Modify: `src/MainPanel.cpp`
- Modify: `src/ScratchRecorder.h`
- Modify: `src/ScratchRecorder.cpp`

- [ ] **Step 1: Stop on device change**

In `MainPanel::changeListenerCallback(ChangeBroadcaster* changedObject)`, when `changedObject == &deviceManager`, call:

```cpp
        scratchRecorder.stopForDeviceChange();
        refreshScratchControls();
```

Keep existing device-change behavior intact.

- [ ] **Step 2: Stop before patch switch**

At the start of `MainPanel::switchPatch(int newPatch, bool savePrev, bool reloadPatch)`, add:

```cpp
    if (scratchRecorder.isRecording())
    {
        scratchRecorder.requestStop();
        showToast("Scratch take saved before patch switch");
    }
```

- [ ] **Step 3: Add state text for interrupted takes**

In `ScratchRecorder::stopForDeviceChange()`, ensure `currentTake.failureReason` is set before stopping:

```cpp
    currentTake.failureReason = "Audio device changed during scratch capture";
```

In `finishStop()`, preserve this message instead of replacing it with sample mismatch text.

- [ ] **Step 4: Build and commit**

Run:

```powershell
cmake --build build --config Release --target Pedalboard3 -- /m:1
```

Expected: app target builds.

Commit:

```powershell
git add src/MainPanel.cpp src/ScratchRecorder.h src/ScratchRecorder.cpp
git commit -m "fix: stop scratch capture on context changes"
```

---

### Task 8: End-To-End Verification

**Files:**
- Modify: none unless failures are found.

- [ ] **Step 1: Run scratch tests**

Run:

```powershell
ctest --test-dir build -C Release --output-on-failure -R Scratch
```

Expected: all scratch tests pass.

- [ ] **Step 2: Run full test target**

Run:

```powershell
ctest --test-dir build -C Release --output-on-failure
```

Expected: all tests pass.

- [ ] **Step 3: Build the app**

Run:

```powershell
cmake --build build --config Release --target Pedalboard3 -- /m:1
```

Expected: app target builds.

- [ ] **Step 4: Manual capture verification**

Launch Pedalboard and verify:

```text
1. Start Pedalboard.
2. Confirm scratch control is visible without opening a plugin editor.
3. Press REC.
4. Play guitar or send test input through the audio interface.
5. Press STOP.
6. Use Takes or Reveal to open the Scratch Ideas folder.
7. Confirm the take folder contains raw.wav, wet.wav, and take.json.
8. Confirm take.json has equal durationSamples for raw/wet completion.
9. Confirm no Audio Recorder node was added to the graph.
10. Start recording again, switch patches, and confirm capture stops cleanly.
```

- [ ] **Step 5: Final commit if verification fixes were needed**

If verification required fixes, commit only the changed files:

```powershell
git status --short
git add <changed-files>
git commit -m "fix: polish scratch capture verification"
```

## Self-Review Checklist

- Spec coverage: global manual capture, raw/wet synchronized files, take bundle metadata, compact panel, commands, settings, RT-safety, and device/patch interruption are covered.
- No new external libraries are introduced.
- `RecorderProcessor` and `LooperProcessor` remain intact.
- Command IDs are appended to preserve saved mappings.
- Audio callback changes are limited to atomic pointer load plus recorder write calls.
- Tests cover naming, metadata, start refusal, state transitions, and raw/wet sample counts.
