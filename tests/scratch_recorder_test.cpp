#include "ScratchRecorder.h"
#include "ScratchTake.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <nlohmann/json.hpp>

namespace
{
class ScopedTempDirectory
{
public:
    explicit ScopedTempDirectory(const juce::String& prefix)
        : directory(juce::File::getSpecialLocation(juce::File::tempDirectory)
                        .getChildFile(prefix)
                        .getNonexistentChildFile("take-root", ""))
    {
        directory.createDirectory();
    }

    ~ScopedTempDirectory()
    {
        directory.deleteRecursively();
    }

    const juce::File& get() const noexcept { return directory; }

private:
    juce::File directory;
};

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
        if (!opened || data == nullptr || numChannels < channels || numSamples <= 0)
            return false;

        for (int channel = 0; channel < channels; ++channel)
            if (data[channel] == nullptr)
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

TEST_CASE("ScratchTake sanitizes filesystem path segments", "[scratch]")
{
    REQUIRE(ScratchTake::sanitisePathSegment("Clean Patch") == "Clean Patch");
    REQUIRE(ScratchTake::sanitisePathSegment("Amp: Lead / Room?") == "Amp Lead  Room");
    REQUIRE(ScratchTake::sanitisePathSegment("Control\tName") == "ControlName");
    REQUIRE(ScratchTake::sanitisePathSegment("Trailing. ") == "Trailing");
    REQUIRE(ScratchTake::sanitisePathSegment("   ") == "untitled");
}

TEST_CASE("ScratchTake creates stable raw wet and metadata paths", "[scratch]")
{
    ScopedTempDirectory root("Pedalboard3ScratchTakeTest");

    ScratchTakeContext context;
    context.rootDirectory = root.get();
    context.patchName = "Lead:Idea";
    context.patchIndex = 2;
    context.documentPath = "C:/rigs/live.pdl";
    context.deviceName = "Test Device";
    context.sampleRate = 48000.0;
    context.rawChannelCount = 1;
    context.wetChannelCount = 2;
    context.masterInputGainDb = -3.0;
    context.masterOutputGainDb = -6.0;
    context.startTime = juce::Time(2026, 5, 4, 1, 2, 3, 0, true);

    auto take = ScratchTake::createPending(context);
    take.durationSamples = 96000;
    take.complete = true;

    REQUIRE(take.takeDirectory.isDirectory());
    REQUIRE(take.takeDirectory.getParentDirectory().getFileName() == "2026-06-04");
    REQUIRE(take.takeDirectory.getFileName() == "20260604-010203-LeadIdea");
    REQUIRE(take.rawFile == take.takeDirectory.getChildFile("raw.wav"));
    REQUIRE(take.wetFile == take.takeDirectory.getChildFile("wet.wav"));
    REQUIRE(take.metadataFile == take.takeDirectory.getChildFile("take.json"));
    REQUIRE(take.rawChannelCount == 1);
    REQUIRE(take.wetChannelCount == 2);

    auto parsed = nlohmann::json::parse(take.toJsonString().toStdString());
    REQUIRE(take.takeId == take.takeDirectory.getFileName());
    REQUIRE(parsed["takeId"] == "20260604-010203-LeadIdea");
    REQUIRE(parsed["durationSamples"] == 96000);
    REQUIRE(parsed["durationSeconds"].get<double>() == Catch::Approx(2.0));
    REQUIRE(parsed["sampleRate"].get<double>() == Catch::Approx(48000.0));
    REQUIRE(parsed["rawChannelCount"] == 1);
    REQUIRE(parsed["wetChannelCount"] == 2);
    REQUIRE(parsed["deviceName"] == "Test Device");
    REQUIRE(parsed["documentPath"] == "C:/rigs/live.pdl");
    REQUIRE(parsed["patchIndex"] == 2);
    REQUIRE(parsed["patchName"] == "Lead:Idea");
    REQUIRE(parsed["masterInputGainDb"].get<double>() == Catch::Approx(-3.0));
    REQUIRE(parsed["masterOutputGainDb"].get<double>() == Catch::Approx(-6.0));
    REQUIRE(parsed["rawFile"].get<std::string>().find("raw.wav") != std::string::npos);
    REQUIRE(parsed["wetFile"].get<std::string>().find("wet.wav") != std::string::npos);
    REQUIRE(parsed["complete"] == true);
    REQUIRE(parsed["failureReason"] == "");

    REQUIRE(take.writeMetadata());
    auto metadata = take.metadataFile.loadFileAsString();
    auto metadataJson = nlohmann::json::parse(metadata.toStdString());
    REQUIRE(metadataJson["takeId"] == parsed["takeId"]);
}

TEST_CASE("ScratchTake appends suffix for colliding take folders", "[scratch]")
{
    ScopedTempDirectory root("Pedalboard3ScratchCollisionTest");

    ScratchTakeContext context;
    context.rootDirectory = root.get();
    context.patchName = "Same Patch";
    context.patchIndex = 0;
    context.sampleRate = 44100.0;
    context.rawChannelCount = 1;
    context.wetChannelCount = 2;
    context.startTime = juce::Time(2026, 5, 4, 1, 2, 3, 0, true);

    auto first = ScratchTake::createPending(context);
    auto second = ScratchTake::createPending(context);

    REQUIRE(first.takeDirectory != second.takeDirectory);
    REQUIRE(second.takeDirectory.getFileName().endsWith("-02"));
    REQUIRE(first.takeId == first.takeDirectory.getFileName());
    REQUIRE(second.takeId == second.takeDirectory.getFileName());
    REQUIRE(first.takeId != second.takeId);
}

TEST_CASE("ScratchTake reports storage creation failures without throwing", "[scratch]")
{
    ScopedTempDirectory root("Pedalboard3ScratchStorageFailureTest");
    const auto rootFile = root.get().getChildFile("blocked-root");
    REQUIRE(rootFile.replaceWithText("not a directory"));

    ScratchTakeContext context;
    context.rootDirectory = rootFile;
    context.patchName = "Lead";
    context.rawChannelCount = 1;
    context.wetChannelCount = 2;
    context.startTime = juce::Time(2026, 5, 4, 1, 2, 3, 0, true);

    auto take = ScratchTake::createPending(context);

    REQUIRE_FALSE(take.failureReason.isEmpty());
    REQUIRE_FALSE(take.takeDirectory.isDirectory());
    REQUIRE(take.takeId == "20260604-010203-Lead");
}

TEST_CASE("ScratchRecorder records raw and wet blocks with matching sample counts", "[scratch]")
{
    ScopedTempDirectory root("Pedalboard3ScratchRecorderTest");
    MemorySinkFactory factory;
    ScratchRecorder recorder(factory);

    ScratchTakeContext context;
    context.rootDirectory = root.get();
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
}

TEST_CASE("ScratchRecorder marks audio device interruptions incomplete", "[scratch]")
{
    ScopedTempDirectory root("Pedalboard3ScratchDeviceInterruptTest");
    MemorySinkFactory factory;
    ScratchRecorder recorder(factory);

    ScratchTakeContext context;
    context.rootDirectory = root.get();
    context.patchName = "Device Interrupt";
    context.sampleRate = 48000.0;
    context.rawChannelCount = 1;
    context.wetChannelCount = 2;

    REQUIRE(recorder.start(context));

    float raw[4] = {0.1f, 0.2f, 0.3f, 0.4f};
    const float* rawPtrs[1] = {raw};
    float wetL[4] = {0.5f, 0.6f, 0.7f, 0.8f};
    float wetR[4] = {0.9f, 1.0f, 0.9f, 0.8f};
    float* wetPtrs[2] = {wetL, wetR};

    recorder.writeRawBlock(rawPtrs, 1, 4);
    recorder.writeWetBlock(wetPtrs, 2, 4);
    recorder.stopForDeviceChange();
    recorder.finishPendingStopForTests();

    const auto status = recorder.getStatus();
    REQUIRE(status.state == ScratchRecorderState::Failed);
    REQUIRE(status.lastTake.has_value());
    REQUIRE_FALSE(status.lastTake->complete);
    REQUIRE(status.lastTake->failureReason == "Audio device changed during scratch capture");
}

TEST_CASE("ScratchRecorder marks patch change interruptions incomplete", "[scratch]")
{
    ScopedTempDirectory root("Pedalboard3ScratchPatchInterruptTest");
    MemorySinkFactory factory;
    ScratchRecorder recorder(factory);

    ScratchTakeContext context;
    context.rootDirectory = root.get();
    context.patchName = "Patch Interrupt";
    context.sampleRate = 48000.0;
    context.rawChannelCount = 1;
    context.wetChannelCount = 2;

    REQUIRE(recorder.start(context));

    float raw[4] = {0.1f, 0.2f, 0.3f, 0.4f};
    const float* rawPtrs[1] = {raw};
    float wetL[4] = {0.5f, 0.6f, 0.7f, 0.8f};
    float wetR[4] = {0.9f, 1.0f, 0.9f, 0.8f};
    float* wetPtrs[2] = {wetL, wetR};

    recorder.writeRawBlock(rawPtrs, 1, 4);
    recorder.writeWetBlock(wetPtrs, 2, 4);
    recorder.stopForPatchChange();
    recorder.finishPendingStopForTests();

    const auto status = recorder.getStatus();
    REQUIRE(status.state == ScratchRecorderState::Failed);
    REQUIRE(status.lastTake.has_value());
    REQUIRE_FALSE(status.lastTake->complete);
    REQUIRE(status.lastTake->failureReason == "Patch changed during scratch capture");
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

TEST_CASE("ScratchRecorder reports null channel write failures without crashing", "[scratch]")
{
    ScopedTempDirectory root("Pedalboard3ScratchNullChannelTest");
    ThreadedWavSinkFactory factory;
    ScratchRecorder recorder(factory);

    ScratchTakeContext context;
    context.rootDirectory = root.get();
    context.patchName = "Null Channel Test";
    context.sampleRate = 48000.0;
    context.rawChannelCount = 1;
    context.wetChannelCount = 2;

    REQUIRE(recorder.start(context));

    const float* rawPtrs[1] = {nullptr};
    float wetL[4] = {0.5f, 0.6f, 0.7f, 0.8f};
    float wetR[4] = {0.9f, 1.0f, 0.9f, 0.8f};
    float* wetPtrs[2] = {wetL, wetR};

    recorder.writeRawBlock(rawPtrs, 1, 4);
    recorder.writeWetBlock(wetPtrs, 2, 4);
    recorder.requestStop();
    recorder.finishPendingStopForTests();

    const auto status = recorder.getStatus();
    REQUIRE(status.state == ScratchRecorderState::Failed);
    REQUIRE(status.rawSamplesWritten == 0);
    REQUIRE(status.wetSamplesWritten == 4);
    REQUIRE(status.lastTake.has_value());
    REQUIRE_FALSE(status.lastTake->complete);
    REQUIRE(status.lastTake->failureReason == "Scratch capture write failed");
}
