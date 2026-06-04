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
