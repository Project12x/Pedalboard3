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
