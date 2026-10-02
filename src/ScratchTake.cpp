#include "ScratchTake.h"
#include "ScratchBundleIO.h"

#include <nlohmann/json.hpp>
#include <cmath>
#include <limits>

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

juce::File getDefaultScratchRoot()
{
    return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
        .getChildFile("Pedalboard3")
        .getChildFile("Scratch Ideas");
}

bool isEmptyFile(const juce::File& file)
{
    return file.getFullPathName().isEmpty();
}

void assignStoragePaths(ScratchTake& take, const juce::File& takeDirectory)
{
    take.takeId = takeDirectory.getFileName();
    take.takeDirectory = takeDirectory;
    take.rawFile = takeDirectory.getChildFile("raw.wav");
    take.wetFile = takeDirectory.getChildFile("wet.wav");
    take.metadataFile = takeDirectory.getChildFile("take.json");
    take.rigFile = takeDirectory.getChildFile("rig.xml");
}
}

juce::String ScratchTake::sanitisePathSegment(const juce::String& text)
{
    juce::String cleaned;

    for (auto c : text)
    {
        if (c < 32 || c == 127 || c == '<' || c == '>' || c == ':' || c == '"' || c == '/' || c == '\\' || c == '|' || c == '?' || c == '*')
            continue;

        cleaned << c;
    }

    cleaned = cleaned.trim();
    while (cleaned.endsWithChar('.') || cleaned.endsWithChar(' '))
        cleaned = cleaned.dropLastCharacters(1).trim();

    return cleaned.isEmpty() ? "untitled" : cleaned;
}

ScratchTake ScratchTake::createPending(const ScratchTakeContext& context)
{
    ScratchTake take;
    take.startTime = context.startTime;
    take.patchName = context.patchName;
    take.patchIndex = context.patchIndex;
    take.documentPath = context.documentPath;
    take.deviceName = context.deviceName;
    take.sampleRate = context.sampleRate;
    take.rawChannelCount = context.rawChannelCount;
    take.wetChannelCount = context.wetChannelCount;
    take.masterInputGainDb = context.masterInputGainDb;
    take.masterOutputGainDb = context.masterOutputGainDb;

    auto root = isEmptyFile(context.rootDirectory) ? getDefaultScratchRoot() : context.rootDirectory;
    auto dayFolder = root.getChildFile(makeDateFolder(context.startTime));

    const auto baseName = makeTimestampId(context.startTime) + "-" + sanitisePathSegment(context.patchName);
    auto candidate = dayFolder.getChildFile(baseName);

    if (!dayFolder.createDirectory())
    {
        assignStoragePaths(take, candidate);
        take.failureReason = "Unable to create scratch day folder: " + dayFolder.getFullPathName();
        return take;
    }

    int suffix = 2;

    while (candidate.exists())
    {
        candidate = dayFolder.getChildFile(baseName + "-" + juce::String(suffix).paddedLeft('0', 2));
        ++suffix;
    }

    assignStoragePaths(take, candidate);
    if (!candidate.createDirectory())
        take.failureReason = "Unable to create scratch take folder: " + candidate.getFullPathName();

    return take;
}

double ScratchTake::durationSeconds() const noexcept
{
    return sampleRate > 0.0 ? static_cast<double>(durationSamples) / sampleRate : 0.0;
}

juce::String ScratchTake::displayDateLabel() const
{
    return startTime.formatted("%Y-%m-%d");
}

juce::String ScratchTake::displayTimeLabel() const
{
    return startTime.formatted("%H:%M:%S");
}

bool ScratchTake::canReveal() const
{
    return takeDirectory.isDirectory();
}

bool ScratchTake::canPlayWetPreview() const
{
    return complete && wetFile.existsAsFile();
}

bool ScratchTake::canReampRawCapture() const
{
    return complete && rawFile.existsAsFile();
}

juce::String ScratchTake::toJsonString() const
{
    nlohmann::json json;
    json["schemaVersion"] = 2;
    json["takeId"] = takeId.toStdString();
    json["startTimestamp"] = startTime.toISO8601(true).toStdString();
    json["durationSamples"] = durationSamples;
    json["rawDurationSamples"] = rawDurationSamples ? nlohmann::json(*rawDurationSamples) : nlohmann::json(nullptr);
    json["wetDurationSamples"] = wetDurationSamples ? nlohmann::json(*wetDurationSamples) : nlohmann::json(nullptr);
    json["captureStatus"] = captureStatus.toStdString();
    static constexpr const char* snapshotNames[] = {"none", "pending", "saved", "failed"};
    json["snapshotStatus"] = snapshotNames[static_cast<int>(snapshotState)];
    json["snapshotMessage"] = snapshotMessage.toStdString();
    json["rigFile"] = "rig.xml";
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
    json["rawFile"] = "raw.wav";
    json["wetFile"] = "wet.wav";
    json["complete"] = complete;
    json["failureReason"] = failureReason.toStdString();

    return juce::String(json.dump(2));
}

bool ScratchTake::writeMetadata() const
{
    return writeScratchTextAtomically(metadataFile, toJsonString()).wasOk();
}

namespace
{
using Json = nlohmann::json;
constexpr int kMaxManifestBytes = 1024 * 1024;

bool validCount(const Json& value)
{
    if (!value.is_number_integer())
        return false;
    if (value.is_number_unsigned())
        return value.get<uint64_t>() <= static_cast<uint64_t>(std::numeric_limits<int64_t>::max());
    return value.get<int64_t>() >= 0;
}

bool validNumber(const Json& object, const char* key, bool positive = false)
{
    if (!object.contains(key))
        return !positive;
    const auto& value = object.at(key);
    return value.is_number() && std::isfinite(value.get<double>())
        && (!positive || value.get<double>() > 0);
}

bool validString(const Json& object, const char* key)
{
    return !object.contains(key) || object.at(key).is_string();
}

struct WavInfo
{
    bool readable = false;
    uint64_t frames = 0;
    double rate = 0;
    int channels = 0;
};

bool boundedWavContainer(juce::FileInputStream& stream)
{
    // Inspect chunk boundaries before JUCE parses metadata. Its WAV reader trusts
    // declared payload lengths (and zero-fills truncated audio). We do not decode
    // samples here; large audio chunks are skipped, non-audio chunks are bounded.
    const auto size = stream.getTotalLength();
    if (size < 12) return false;
    auto tag = [&stream]() {
        char value[4]{};
        return stream.read(value, 4) == 4 ? juce::String::fromUTF8(value, 4) : juce::String{};
    };
    const auto type = tag();
    const bool rf64 = type == "RF64";
    if (!rf64 && type != "RIFF") return false;
    uint64_t extent = static_cast<uint32_t>(stream.readInt());
    if (tag() != "WAVE") return false;
    uint64_t rf64DataSize = 0;
    if (rf64)
    {
        if (size < 48 || tag() != "ds64") return false;
        const auto dsSize = static_cast<uint32_t>(stream.readInt());
        if (dsSize < 28 || dsSize > kMaxManifestBytes || dsSize > static_cast<uint64_t>(size - 20)) return false;
        extent = static_cast<uint64_t>(stream.readInt64());
        rf64DataSize = static_cast<uint64_t>(stream.readInt64());
        if (!stream.setPosition(20 + static_cast<juce::int64>(dsSize) + (dsSize & 1))) return false;
    }
    if (extent < 4 || extent > static_cast<uint64_t>(size - 8)) return false;
    const auto end = static_cast<juce::int64>(extent + 8);
    bool formatSeen = false, audioSeen = false;
    int chunks = 0;
    while (stream.getPosition() < end)
    {
        if (++chunks > 4096 || end - stream.getPosition() < 8) return false;
        const auto chunk = tag();
        uint64_t length = static_cast<uint32_t>(stream.readInt());
        if (chunk == "data" && rf64 && length == UINT32_MAX) length = rf64DataSize;
        if (length > static_cast<uint64_t>(end - stream.getPosition())) return false;
        if (chunk != "data" && length > kMaxManifestBytes) return false;
        if (chunk == "fmt ") formatSeen = length >= 16;
        if (chunk == "data") audioSeen = length > 0;
        const auto next = stream.getPosition() + static_cast<juce::int64>(length + (length & 1));
        if (next > end || !stream.setPosition(static_cast<juce::int64>(next))) return false;
    }
    return formatSeen && audioSeen && stream.setPosition(0);
}

WavInfo inspectWav(const juce::File& file)
{
    if (!file.existsAsFile() || file.isSymbolicLink())
        return {};
    auto stream = file.createInputStream();
    if (!stream || !boundedWavContainer(*stream))
        return {};
    juce::WavAudioFormat format;
    std::unique_ptr<juce::AudioFormatReader> reader(format.createReaderFor(stream.release(), true));
    if (!reader || reader->lengthInSamples <= 0 || reader->numChannels == 0 || reader->numChannels > 16
        || !std::isfinite(reader->sampleRate) || reader->sampleRate <= 0)
        return {};
    return {true, static_cast<uint64_t>(reader->lengthInSamples), reader->sampleRate,
            static_cast<int>(reader->numChannels)};
}
}

ScratchTakeLoadResult loadScratchTake(const juce::File& directory)
{
    ScratchTakeLoadResult result;
    auto& take = result.take;
    assignStoragePaths(take, directory);
    take.patchName = directory.getFileName();
    result.message = "Invalid or missing take metadata";
    if (!directory.isDirectory() || directory.isSymbolicLink() || directory.getParentDirectory().isSymbolicLink()
        || take.metadataFile.isSymbolicLink() || take.rawFile.isSymbolicLink()
        || take.wetFile.isSymbolicLink() || take.rigFile.isSymbolicLink())
        return result;

    auto stream = take.metadataFile.createInputStream();
    if (!stream || stream->getTotalLength() <= 0 || stream->getTotalLength() > kMaxManifestBytes)
        return result;
    // Bound the read too, in case a different process grows the file after stat.
    juce::MemoryBlock bytes;
    stream->readIntoMemoryBlock(bytes, kMaxManifestBytes + 1);
    if (bytes.getSize() == 0 || bytes.getSize() > kMaxManifestBytes)
        return result;
    const auto* begin = static_cast<const char*>(bytes.getData());
    const auto json = Json::parse(begin, begin + bytes.getSize(), nullptr, false);
    if (json.is_discarded() || !json.is_object())
        return result;

    if (json.contains("schemaVersion"))
    {
        const auto& version = json.at("schemaVersion");
        if (!validCount(version) || version.get<uint64_t>() == 0)
            return result;
        if (version.get<uint64_t>() > 2)
        {
            result.state = ScratchTakeLoadState::Unsupported;
            result.message = "Newer take format; Reveal to inspect (not modified)";
            return result;
        }
        take.schemaVersion = version.get<int>();
    }
    else
        take.schemaVersion = 1;

    for (const auto* key : {"takeId", "startTimestamp", "patchName", "documentPath", "deviceName",
                            "failureReason", "rawFile", "wetFile", "rigFile", "captureStatus",
                            "snapshotStatus", "snapshotMessage"})
        if (!validString(json, key))
            return result;
    for (const auto* key : {"durationSamples", "rawDurationSamples", "wetDurationSamples"})
        if (json.contains(key) && !(juce::String(key) != "durationSamples" && json.at(key).is_null())
            && !validCount(json.at(key)))
            return result;
    for (const auto* key : {"rawChannelCount", "wetChannelCount"})
        if (!json.contains(key) || !validCount(json.at(key)) || json.at(key).get<uint64_t>() > 16)
            return result;
    if (!validNumber(json, "sampleRate", true) || !validNumber(json, "masterInputGainDb")
        || !validNumber(json, "masterOutputGainDb") || !validNumber(json, "durationSeconds")
        || (json.contains("complete") && !json.at("complete").is_boolean())
        || (json.contains("patchIndex") && (!validCount(json.at("patchIndex"))
            || json.at("patchIndex").get<uint64_t>() > static_cast<uint64_t>(std::numeric_limits<int>::max()))))
        return result;

    // V2 owns a fixed bundle layout. Legacy absolute paths are informational,
    // never followed: moving a folder must not open another performance's audio.
    if (take.schemaVersion == 2)
        for (const auto* key : {"rawFile", "wetFile", "rigFile"})
        {
            const auto expected = juce::String(key) == "rawFile" ? "raw.wav" : juce::String(key) == "wetFile" ? "wet.wav" : "rig.xml";
            if (json.contains(key) && json.at(key) != expected)
                return result;
        }

    auto text = [&json](const char* key, const juce::String& fallback = {}) {
        return json.contains(key) ? juce::String(json.at(key).get<std::string>()) : fallback;
    };
    take.takeId = text("takeId", directory.getFileName());
    take.patchName = text("patchName", directory.getFileName());
    take.startTime = juce::Time::fromISO8601(text("startTimestamp"));
    take.patchIndex = json.value("patchIndex", 0);
    take.documentPath = text("documentPath");
    take.deviceName = text("deviceName");
    take.sampleRate = json.at("sampleRate").get<double>();
    take.rawChannelCount = json.at("rawChannelCount").get<int>();
    take.wetChannelCount = json.at("wetChannelCount").get<int>();
    take.masterInputGainDb = json.value("masterInputGainDb", 0.0);
    take.masterOutputGainDb = json.value("masterOutputGainDb", 0.0);
    take.durationSamples = json.value("durationSamples", uint64_t{});
    auto optionalCount = [&json](const char* key) -> std::optional<uint64_t> {
        if (!json.contains(key) || json.at(key).is_null()) return {};
        return json.at(key).get<uint64_t>();
    };
    take.rawDurationSamples = optionalCount("rawDurationSamples");
    take.wetDurationSamples = optionalCount("wetDurationSamples");
    take.complete = json.value("complete", false);
    take.failureReason = text("failureReason");
    take.captureStatus = take.schemaVersion == 1 ? (take.complete ? "complete" : "interrupted")
        : text("captureStatus", "interrupted");
    if (take.captureStatus != "recording" && take.captureStatus != "complete"
        && take.captureStatus != "interrupted" && take.captureStatus != "failed")
        return result;
    const auto snapshot = text("snapshotStatus", "none");
    if (snapshot == "none") take.snapshotState = ScratchSnapshotState::None;
    else if (snapshot == "pending") take.snapshotState = ScratchSnapshotState::Pending;
    else if (snapshot == "saved") take.snapshotState = ScratchSnapshotState::Saved;
    else if (snapshot == "failed") take.snapshotState = ScratchSnapshotState::Failed;
    else return result;
    take.snapshotMessage = text("snapshotMessage");

    const auto raw = inspectWav(take.rawFile);
    const auto wet = inspectWav(take.wetFile);
    result.rawReadable = raw.readable;
    result.wetReadable = wet.readable;
    const bool pairMatches = raw.readable && wet.readable && raw.frames == wet.frames
        && raw.rate == take.sampleRate && wet.rate == take.sampleRate
        && raw.channels == take.rawChannelCount && wet.channels == take.wetChannelCount
        && (!take.rawDurationSamples || *take.rawDurationSamples == raw.frames)
        && (!take.wetDurationSamples || *take.wetDurationSamples == wet.frames);
    const bool ready = take.complete && take.captureStatus == "complete" && pairMatches;
    result.state = ready ? ScratchTakeLoadState::Ready : ScratchTakeLoadState::Incomplete;
    result.message = ready ? juce::String{} : "Incomplete capture: " + (take.failureReason.isNotEmpty()
        ? take.failureReason : juce::String("audio is missing, interrupted, or does not match its metadata"));
    return result;
}
