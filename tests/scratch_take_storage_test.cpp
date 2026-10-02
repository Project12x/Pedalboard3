#include "ScratchTake.h"
#include "ScratchTestFiles.h"
#include "ScratchBundleIO.h"
#include <catch2/catch_test_macros.hpp>
#include <nlohmann/json.hpp>
#include <filesystem>

TEST_CASE("Scratch storage writes portable versioned metadata", "[scratch][scratch-storage]")
{
    ScratchTake take;
    take.takeDirectory = juce::File::getSpecialLocation(juce::File::tempDirectory).getChildFile("storage-test");
    take.rawFile = take.takeDirectory.getChildFile("raw.wav");
    take.wetFile = take.takeDirectory.getChildFile("wet.wav");
    const auto json = nlohmann::json::parse(take.toJsonString().toStdString());
    REQUIRE(json.value("schemaVersion", 0) == 2);
    REQUIRE(json.at("rawFile") == "raw.wav");
    REQUIRE(json.at("wetFile") == "wet.wav");
}

TEST_CASE("Scratch storage preserves relocated legacy bundles", "[scratch][scratch-storage]")
{
    ScratchTestFiles files;
    auto take = files.makeTake();
    ScratchTestFiles::wav(take.rawFile);
    ScratchTestFiles::wav(take.wetFile, 2);
    auto legacy = nlohmann::json::parse(take.toJsonString().toStdString());
    legacy.erase("schemaVersion");
    legacy.erase("rawDurationSamples");
    legacy.erase("wetDurationSamples");
    legacy["rawFile"] = take.rawFile.getFullPathName().toStdString();
    legacy["wetFile"] = take.wetFile.getFullPathName().toStdString();
    legacy["complete"] = true;
    const juce::String originalMetadata(legacy.dump());
    REQUIRE(take.metadataFile.replaceWithText(originalMetadata));
    const auto movedDirectory = files.root.getChildFile("moved");
    REQUIRE(take.takeDirectory.moveFileTo(movedDirectory));
    const auto loaded = loadScratchTake(movedDirectory);
    REQUIRE(loaded.state == ScratchTakeLoadState::Ready);
    REQUIRE(loaded.rawReadable);
    REQUIRE(loaded.wetReadable);
    REQUIRE(loaded.take.rawFile == movedDirectory.getChildFile("raw.wav"));
    REQUIRE_FALSE(loaded.take.rawDurationSamples.has_value());
    REQUIRE(loaded.take.metadataFile.loadFileAsString() == originalMetadata);
}

TEST_CASE("Scratch storage rejects invalid and unsupported manifests without mutation", "[scratch][scratch-storage]")
{
    ScratchTestFiles files;
    const auto take = files.makeTake();
    auto json = nlohmann::json::parse(take.toJsonString().toStdString());
    auto expected = ScratchTakeLoadState::Invalid;
    SECTION("wrong count type") { json["durationSamples"] = "12"; }
    SECTION("negative count") { json["durationSamples"] = -1; }
    SECTION("overflowing count") { json["durationSamples"] = 1e30; }
    SECTION("oversize unsigned count") { json["durationSamples"] = UINT64_MAX; }
    SECTION("negative rate") { json["sampleRate"] = -48000; }
    SECTION("zero rate") { json["sampleRate"] = 0; }
    SECTION("null rate") { json["sampleRate"] = nullptr; }
    SECTION("rate type") { json["sampleRate"] = "48000"; }
    SECTION("too many channels") { json["rawChannelCount"] = 17; }
    SECTION("fractional channels") { json["wetChannelCount"] = 1.5; }
    SECTION("traversal") { json["rawFile"] = "../outside.wav"; }
    SECTION("absolute external path") { json["wetFile"] = files.root.getChildFile("outside.wav").getFullPathName().toStdString(); }
    SECTION("wrong path type") { json["wetFile"] = 12; }
    SECTION("future schema") { json["schemaVersion"] = 99; expected = ScratchTakeLoadState::Unsupported; }
    SECTION("boolean schema") { json["schemaVersion"] = true; }
    SECTION("capture status") { json["captureStatus"] = "invented"; }
    SECTION("wrong boolean") { json["complete"] = "true"; }
    SECTION("oversize json") { json["patchName"] = std::string(1024 * 1024, 'x'); }
    auto bytes = juce::String(json.dump());
    REQUIRE(take.metadataFile.replaceWithText(bytes));
    const auto loaded = loadScratchTake(take.takeDirectory);
    REQUIRE(loaded.state == expected);
    REQUIRE_FALSE(loaded.rawReadable);
    REQUIRE_FALSE(loaded.wetReadable);
    REQUIRE(loaded.take.canReveal());
    REQUIRE(loaded.take.metadataFile.loadFileAsString() == bytes);
}

TEST_CASE("Scratch storage exposes readable interrupted audio without certifying capture", "[scratch][scratch-storage]")
{
    ScratchTestFiles files;
    auto take = files.makeTake();
    take.rawDurationSamples = 480;
    take.wetDurationSamples = 240;
    take.durationSamples = 240;
    take.captureStatus = "interrupted";
    ScratchTestFiles::wav(take.rawFile);
    ScratchTestFiles::wav(take.wetFile, 2, 240);
    REQUIRE(take.writeMetadata());
    const auto loaded = loadScratchTake(take.takeDirectory);
    REQUIRE(loaded.state == ScratchTakeLoadState::Incomplete);
    REQUIRE(loaded.rawReadable);
    REQUIRE(loaded.wetReadable);
    REQUIRE(loaded.take.rawDurationSamples == 480);
    REQUIRE(loaded.take.wetDurationSamples == 240);
    REQUIRE_FALSE(loaded.take.complete);
    REQUIRE(loaded.message.isNotEmpty());
}

TEST_CASE("Scratch storage malformed JSON and missing audio remain inspectable", "[scratch][scratch-storage]")
{
    ScratchTestFiles files;
    auto take = files.makeTake();
    SECTION("bad json")
    {
        REQUIRE(take.metadataFile.replaceWithText("{invalid"));
        REQUIRE(loadScratchTake(take.takeDirectory).state == ScratchTakeLoadState::Invalid);
    }
    SECTION("missing audio despite complete metadata")
    {
        take.complete = true;
        take.captureStatus = "complete";
        REQUIRE(take.writeMetadata());
        const auto loaded = loadScratchTake(take.takeDirectory);
        REQUIRE(loaded.state == ScratchTakeLoadState::Incomplete);
        REQUIRE_FALSE(loaded.rawReadable);
        REQUIRE_FALSE(loaded.wetReadable);
    }
    SECTION("truncated WAV payload")
    {
        ScratchTestFiles::wav(take.rawFile);
        ScratchTestFiles::wav(take.wetFile, 2);
        take.complete = true;
        take.captureStatus = "complete";
        REQUIRE(take.writeMetadata());
        {
            juce::FileOutputStream stream(take.rawFile);
            REQUIRE(stream.setPosition(take.rawFile.getSize() - 100));
            REQUIRE(stream.truncate().wasOk());
        }
        const auto loaded = loadScratchTake(take.takeDirectory);
        REQUIRE(loaded.state == ScratchTakeLoadState::Incomplete);
        REQUIRE_FALSE(loaded.rawReadable);
        REQUIRE(loaded.wetReadable);
    }
}

TEST_CASE("Scratch storage failed replacement preserves previous bytes and cleans temporary files", "[scratch][scratch-storage]")
{
    ScratchTestFiles files;
    const auto target = files.root.getChildFile("take.json");
    REQUIRE(target.replaceWithText("previous bytes"));
    juce::File temporaryFile;
    const auto failure = writeScratchTextAtomically(target, "replacement bytes",
        [&](const juce::File& temporary, const juce::File& destination) {
            temporaryFile = temporary;
            REQUIRE(temporary.getParentDirectory() == target.getParentDirectory());
            REQUIRE(destination == target);
            REQUIRE(temporary.loadFileAsString() == "replacement bytes");
            return false;
        });
    REQUIRE(failure.failed());
    REQUIRE(target.loadFileAsString() == "previous bytes");
    REQUIRE_FALSE(temporaryFile.exists());
    REQUIRE(files.root.findChildFiles(juce::File::findFiles, false).size() == 1);
    REQUIRE(writeScratchTextAtomically(target, "replacement bytes").wasOk());
    REQUIRE(target.loadFileAsString() == "replacement bytes");
}

TEST_CASE("Scratch storage does not follow linked bundle files or directories", "[scratch][scratch-storage]")
{
    ScratchTestFiles files;
    const auto take = files.makeTake();
    REQUIRE(take.writeMetadata());
    const auto outside = files.root.getChildFile("outside.wav");
    ScratchTestFiles::wav(outside);
    const auto path = [](const juce::File& file) { return std::filesystem::u8path(file.getFullPathName().toStdString()); };
    std::error_code error;
    SECTION("linked audio")
    {
        std::filesystem::create_symlink(path(outside), path(take.rawFile), error);
        if (error) SKIP("Host does not permit symlink creation: " << error.message());
        REQUIRE(loadScratchTake(take.takeDirectory).state == ScratchTakeLoadState::Invalid);
    }
    SECTION("linked directory")
    {
        const auto link = files.root.getChildFile("linked-take");
        std::filesystem::create_directory_symlink(path(take.takeDirectory), path(link), error);
        if (error) SKIP("Host does not permit symlink creation: " << error.message());
        REQUIRE(loadScratchTake(link).state == ScratchTakeLoadState::Invalid);
        // Remove only the link, not its target; fixture cleanup owns the real directory.
        REQUIRE(std::filesystem::remove(path(link), error));
    }
}
