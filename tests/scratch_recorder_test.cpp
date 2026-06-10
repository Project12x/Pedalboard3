#include "ScratchRecorder.h"
#include "ScratchPanelLayout.h"
#include "ScratchPanelPresentation.h"
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

TEST_CASE("ScratchTake reports row action availability from saved files", "[scratch]")
{
    ScopedTempDirectory root("Pedalboard3ScratchTakeActionAvailabilityTest");

    ScratchTakeContext context;
    context.rootDirectory = root.get();
    context.patchName = "Action Patch";
    context.sampleRate = 48000.0;
    context.rawChannelCount = 1;
    context.wetChannelCount = 2;
    context.startTime = juce::Time(2026, 5, 4, 1, 2, 3, 0, true);

    auto take = ScratchTake::createPending(context);
    take.complete = true;

    REQUIRE(take.canReveal());
    REQUIRE_FALSE(take.canPlayWetPreview());
    REQUIRE_FALSE(take.canReampRawCapture());

    REQUIRE(take.wetFile.replaceWithText("wet"));
    REQUIRE(take.canPlayWetPreview());
    REQUIRE_FALSE(take.canReampRawCapture());

    REQUIRE(take.rawFile.replaceWithText("raw"));
    REQUIRE(take.canReampRawCapture());

    take.complete = false;
    REQUIRE_FALSE(take.canPlayWetPreview());
    REQUIRE_FALSE(take.canReampRawCapture());
    REQUIRE(take.canReveal());
}

TEST_CASE("ScratchTake exposes stable display date and time labels", "[scratch]")
{
    ScratchTake take;
    take.startTime = juce::Time(2026, 5, 4, 1, 2, 3, 0, true);

    REQUIRE(take.displayDateLabel() == "2026-06-04");
    REQUIRE(take.displayTimeLabel() == "01:02:03");
}

TEST_CASE("Scratch take row actions stay separated at panel width", "[scratch]")
{
    const auto layout = ScratchPanelLayout::calculateTakeRowActions(600 - 36 - 14);

    REQUIRE(layout.play.getWidth() >= 56);
    REQUIRE(layout.reamp.getWidth() >= 64);
    REQUIRE(layout.reveal.getWidth() >= 60);
    REQUIRE(layout.play.getRight() + 8 <= layout.reamp.getX());
    REQUIRE(layout.reamp.getRight() + 8 <= layout.reveal.getX());
    REQUIRE(layout.reveal.getRight() <= layout.rowRight);
}

TEST_CASE("Scratch destination actions reserve non-overlapping controls", "[scratch]")
{
    const auto layout = ScratchPanelLayout::calculateDestinationLayout({0, 0, 600 - 36, 54});

    REQUIRE(layout.text.getWidth() >= 240);
    REQUIRE(layout.choose.getWidth() >= 80);
    REQUIRE(layout.reset.getWidth() >= 68);
    REQUIRE(layout.reveal.getWidth() >= 68);
    REQUIRE(layout.text.getRight() + 8 <= layout.choose.getX());
    REQUIRE(layout.choose.getRight() + 8 <= layout.reset.getX());
    REQUIRE(layout.reset.getRight() + 8 <= layout.reveal.getX());
    REQUIRE(layout.reveal.getRight() <= layout.rowRight);
}

TEST_CASE("Scratch panel content bounds cap oversized dialogs without shrinking compact panels", "[scratch]")
{
    const auto compact = ScratchPanelLayout::calculateContentBounds({0, 0, 600, 560});
    REQUIRE(compact.getX() == 0);
    REQUIRE(compact.getWidth() == 600);

    const auto wide = ScratchPanelLayout::calculateContentBounds({0, 0, 1800, 900});
    REQUIRE(wide.getWidth() == ScratchPanelLayout::maxContentWidth);
    REQUIRE(wide.getHeight() == 900);
    REQUIRE(wide.getCentreX() == 900);
}

TEST_CASE("Scratch destination display compacts long paths", "[scratch]")
{
    const juce::String longPath("C:\\Users\\estee\\Documents\\Pedalboard\\Very Long Folder Name\\Scratch Ideas");

    const auto compacted = ScratchPanelLayout::compactDestinationPathForDisplay(longPath, 42);

    REQUIRE(compacted.length() <= 42);
    REQUIRE(compacted.contains("..."));
    REQUIRE(compacted.endsWith("Scratch Ideas"));
}

TEST_CASE("ScratchRecorder resets scratch root to application default", "[scratch]")
{
    ScopedTempDirectory customRoot("Pedalboard3ScratchCustomRootTest");
    MemorySinkFactory factory;
    ScratchRecorder recorder(factory);

    recorder.setScratchRoot(customRoot.get());
    REQUIRE(recorder.getScratchRoot().getFullPathName() == customRoot.get().getFullPathName());

    recorder.resetScratchRootToDefault();

    REQUIRE(recorder.getScratchRoot().getFullPathName() ==
            ScratchRecorder::getDefaultScratchRoot().getFullPathName());
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

TEST_CASE("ScratchRecorder exposes active take context while recording", "[scratch]")
{
    ScopedTempDirectory root("Pedalboard3ScratchRecorderStatusTest");
    MemorySinkFactory factory;
    ScratchRecorder recorder(factory);

    ScratchTakeContext context;
    context.rootDirectory = root.get();
    context.patchName = "Live Idea";
    context.patchIndex = 3;
    context.deviceName = "Status Device";
    context.sampleRate = 48000.0;
    context.rawChannelCount = 1;
    context.wetChannelCount = 2;
    context.masterInputGainDb = -2.5;
    context.masterOutputGainDb = -4.0;

    REQUIRE(recorder.start(context));

    auto status = recorder.getStatus();
    REQUIRE(status.state == ScratchRecorderState::Recording);
    REQUIRE(status.activeTake.has_value());
    REQUIRE(status.activeTake->patchName == "Live Idea");
    REQUIRE(status.activeTake->patchIndex == 3);
    REQUIRE(status.activeTake->deviceName == "Status Device");
    REQUIRE(status.activeTake->sampleRate == Catch::Approx(48000.0));
    REQUIRE(status.activeTake->rawChannelCount == 1);
    REQUIRE(status.activeTake->wetChannelCount == 2);
    REQUIRE(status.activeTake->masterInputGainDb == Catch::Approx(-2.5));
    REQUIRE(status.activeTake->masterOutputGainDb == Catch::Approx(-4.0));
    REQUIRE(status.scratchRoot.getFullPathName() == root.get().getFullPathName());
    REQUIRE_FALSE(status.lastTake.has_value());

    float raw[480] = {};
    const float* rawPtrs[1] = {raw};
    float wetL[480] = {};
    float wetR[480] = {};
    float* wetPtrs[2] = {wetL, wetR};

    recorder.writeRawBlock(rawPtrs, 1, 480);
    recorder.writeWetBlock(wetPtrs, 2, 480);

    status = recorder.getStatus();
    REQUIRE(status.elapsedSamples == 480);
    REQUIRE(status.activeTake.has_value());
    REQUIRE(status.activeTake->sampleRate == Catch::Approx(48000.0));

    recorder.requestStop();
    recorder.finishPendingStopForTests();

    status = recorder.getStatus();
    REQUIRE(status.state == ScratchRecorderState::Saved);
    REQUIRE_FALSE(status.activeTake.has_value());
    REQUIRE(status.lastTake.has_value());
    REQUIRE(status.lastTake->patchName == "Live Idea");
}

TEST_CASE("Scratch panel elapsed label uses active take sample rate while recording", "[scratch]")
{
    ScratchRecorderStatus status;
    status.state = ScratchRecorderState::Recording;
    status.elapsedSamples = 48000;

    ScratchTake activeTake;
    activeTake.sampleRate = 48000.0;
    status.activeTake = activeTake;

    REQUIRE(ScratchPanelPresentation::formatElapsedLabel(status) == "00:01");

    status.elapsedSamples = 96000;
    REQUIRE(ScratchPanelPresentation::formatElapsedLabel(status) == "00:02");
}

TEST_CASE("Scratch panel status labels expose raw wet capture context", "[scratch]")
{
    ScratchRecorderStatus status;
    status.state = ScratchRecorderState::Ready;
    REQUIRE(ScratchPanelPresentation::formatStatusLine(status).contains("RAW + WET"));
    REQUIRE(ScratchPanelPresentation::formatFooterStatusLine(status).contains("RAW + WET"));

    ScratchTake activeTake;
    activeTake.sampleRate = 48000.0;
    activeTake.rawChannelCount = 1;
    activeTake.wetChannelCount = 2;
    status.state = ScratchRecorderState::Recording;
    status.elapsedSamples = 96000;
    status.activeTake = activeTake;

    REQUIRE(ScratchPanelPresentation::formatCapturePairLabel(status) == "RAW 1ch + WET 2ch");
    REQUIRE(ScratchPanelPresentation::formatStatusLine(status).contains("Recording 00:02"));
    REQUIRE(ScratchPanelPresentation::formatStatusLine(status).contains("RAW 1ch + WET 2ch"));
    REQUIRE(ScratchPanelPresentation::formatFooterStatusLine(status).contains("REC 00:02"));
    REQUIRE(ScratchPanelPresentation::formatFooterStatusLine(status).contains("RAW 1ch + WET 2ch"));

    status.state = ScratchRecorderState::Saving;
    REQUIRE(ScratchPanelPresentation::formatStatusLine(status).contains("Saving take"));
    REQUIRE(ScratchPanelPresentation::formatFooterStatusLine(status) == "Saving RAW + WET");

    status.state = ScratchRecorderState::Failed;
    status.message = "No input channels available";
    REQUIRE(ScratchPanelPresentation::formatStatusLine(status) == "No input channels available");
    REQUIRE(ScratchPanelPresentation::formatFooterStatusLine(status) == "No input channels available");
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
