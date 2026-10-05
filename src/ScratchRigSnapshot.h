#pragma once
#include "ScratchTake.h"
#include <array>
#include <vector>

struct ScratchAssetReference { juce::String nodePath, kind; juce::File file; };
struct ScratchRigSnapshotData
{
    std::unique_ptr<juce::XmlElement> patch;
    juce::MemoryBlock masterInsertState;
    bool masterInsertBypassed = false;
    std::array<double, 16> inputChannelDb{}, outputChannelDb{};
    double masterInputDb = 0, masterOutputDb = 0;
    juce::String appVersion;
    ScratchTakeContext context;
    std::vector<ScratchAssetReference> assets;
};
juce::String serializeScratchRig(const ScratchRigSnapshotData&);
juce::Result writeScratchRig(const juce::File& takeDirectory, const juce::String& xml);
