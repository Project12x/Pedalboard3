#include "ScratchRigSnapshot.h"
#include "ScratchRecorder.h"
#include "ScratchTestFiles.h"
#include <catch2/catch_approx.hpp>

TEST_CASE("Scratch rig serializer retains detached live state and context", "[scratch][scratch-rig]")
{
    ScratchRigSnapshotData data;
    data.patch = std::make_unique<juce::XmlElement>("Patch");
    auto* plugin = data.patch->createNewChildElement("UnsavedProbe");
    plugin->setAttribute("gain", 0.371);
    const char bytes[] = {0, 4, 17, 0, 127};
    data.masterInsertState.append(bytes, sizeof(bytes)); data.masterInsertBypassed = true;
    data.masterInputDb = -3; data.masterOutputDb = -7;
    data.inputChannelDb[15] = -12; data.outputChannelDb[0] = 2;
    data.context.sampleRate = 96000; data.context.rawChannelCount = 16; data.context.wetChannelCount = 2;
    data.context.deviceName = "Probe device"; data.appVersion = "test-build";
    auto root = juce::File::getSpecialLocation(juce::File::tempDirectory);
    data.assets.push_back({"graph/7/loop/9", "NAM", root.getChildFile("amp.nam")});
    data.assets.push_back({"master/2", "IR", root.getChildFile("cab.wav")});
    const auto xml = juce::parseXML(serializeScratchRig(data));
    REQUIRE(xml != nullptr); REQUIRE(xml->hasTagName("ScratchRig"));
    REQUIRE(xml->getIntAttribute("schemaVersion") == 1);
    REQUIRE(xml->getStringAttribute("appVersion") == "test-build");
    REQUIRE(xml->getChildByName("Patch")->getChildByName("UnsavedProbe")->getDoubleAttribute("gain") == Catch::Approx(0.371));
    auto* master = xml->getChildByName("MasterInsert"); REQUIRE(master->getBoolAttribute("bypassed"));
    juce::MemoryBlock restored; REQUIRE(restored.fromBase64Encoding(master->getAllSubText()));
    REQUIRE(restored == data.masterInsertState);
    auto* gains = xml->getChildByName("Gains");
    REQUIRE(gains->getDoubleAttribute("masterInputDb") == -3);
    REQUIRE(gains->getDoubleAttribute("masterOutputDb") == -7);
    REQUIRE(gains->getNumChildElements() == 32);
    REQUIRE(gains->getChildElement(30)->getIntAttribute("channel") == 15);
    REQUIRE(gains->getChildElement(30)->getDoubleAttribute("gainDb") == -12);
    REQUIRE(xml->getChildByName("Device")->getDoubleAttribute("sampleRate") == 96000);
    REQUIRE(xml->getChildByName("Device")->getIntAttribute("rawChannels") == 16);
    auto* assets = xml->getChildByName("Assets"); REQUIRE(assets->getNumChildElements() == 2);
    REQUIRE(assets->getFirstChildElement()->getStringAttribute("nodePath") == "graph/7/loop/9");
    REQUIRE(assets->getFirstChildElement()->getStringAttribute("path") == data.assets[0].file.getFullPathName());
    REQUIRE(assets->getBoolAttribute("opaquePluginDependenciesUnenumerated"));
}

TEST_CASE("Scratch snapshot distinguishes audio success from context failure", "[scratch][scratch-rig]")
{
    ScratchTestFiles files; ThreadedWavSinkFactory factory; ScratchRecorder recorder(factory);
    ScratchTakeContext context; context.rootDirectory = files.root; context.sampleRate = 48000;
    context.rawChannelCount = context.wetChannelCount = 1;
    REQUIRE(recorder.prepareTake(context).wasOk());
    const auto prepared = recorder.getStatus().activeTake.value();
    REQUIRE(prepared.rigFile.createDirectory().wasOk()); // deterministic replacement failure
    const auto result = writeScratchRig(prepared.takeDirectory, "<ScratchRig schemaVersion=\"1\"/>");
    REQUIRE(result.failed());
    REQUIRE(recorder.completePreparedSnapshot(recorder.getPreparationId(), ScratchSnapshotState::Failed, result.getErrorMessage()).wasOk());
    juce::AudioBuffer<float> audio(1, 128); audio.clear();
    REQUIRE(recorder.beginDeviceBlock(audio.getArrayOfReadPointers(), 1, 128));
    recorder.endDeviceBlock(audio.getArrayOfWritePointers(), 1, 128);
    recorder.requestStop(); recorder.finishPendingStopForTests();
    const auto take = recorder.getStatus().lastTake.value();
    REQUIRE(take.takeDirectory == prepared.takeDirectory);
    REQUIRE(take.complete); REQUIRE(take.snapshotState == ScratchSnapshotState::Failed);
    REQUIRE_FALSE(take.snapshotMessage.isEmpty()); REQUIRE_FALSE(take.rigFile.existsAsFile());
}

TEST_CASE("Scratch preparation ignores stale completion after cancellation and a newer REC", "[scratch][scratch-rig]")
{
    ScratchTestFiles files; ThreadedWavSinkFactory factory; ScratchRecorder recorder(factory);
    ScratchTakeContext context; context.rootDirectory = files.root;
    context.rawChannelCount = context.wetChannelCount = 1;
    REQUIRE(recorder.prepareTake(context).wasOk()); const auto oldId = recorder.getPreparationId();
    REQUIRE(recorder.getStatus().state == ScratchRecorderState::Preparing);
    REQUIRE(recorder.startPreparedTake().failed());
    REQUIRE(recorder.prepareTake(context).failed()); // no second bundle while preparing
    recorder.requestStop(); // double-click REC cancels preparation
    REQUIRE_FALSE(recorder.isRecording());
    REQUIRE(recorder.completePreparedSnapshot(oldId, ScratchSnapshotState::Saved, {}).failed());
    REQUIRE(recorder.prepareTake(context).wasOk()); const auto newId = recorder.getPreparationId();
    REQUIRE(newId > oldId);
    REQUIRE(recorder.completePreparedSnapshot(oldId, ScratchSnapshotState::Saved, {}).failed());
    REQUIRE(recorder.getStatus().state == ScratchRecorderState::Preparing);
    REQUIRE(recorder.completePreparedSnapshot(newId, ScratchSnapshotState::Failed, "Probe failure").wasOk());
    REQUIRE(recorder.isRecording()); recorder.requestStop(); recorder.finishPendingStopForTests();
}

TEST_CASE("Scratch missing rig sidecar warns without invalidating intact audio", "[scratch][scratch-rig]")
{
    ScratchTestFiles files; auto take = files.makeTake();
    ScratchTestFiles::wav(take.rawFile, 1); ScratchTestFiles::wav(take.wetFile, 2);
    take.durationSamples = 480; take.rawDurationSamples = take.wetDurationSamples = 480;
    take.complete = true; take.captureStatus = "complete"; take.snapshotState = ScratchSnapshotState::Saved;
    REQUIRE(take.writeMetadata());
    const auto loaded = loadScratchTake(take.takeDirectory);
    REQUIRE(loaded.state == ScratchTakeLoadState::Ready);
    REQUIRE(loaded.take.snapshotState == ScratchSnapshotState::Failed);
    REQUIRE_FALSE(loaded.take.snapshotMessage.isEmpty());
}
