#include "ScratchRigSnapshot.h"
#include "ScratchBundleIO.h"

juce::String serializeScratchRig(const ScratchRigSnapshotData& data)
{
    juce::XmlElement rig("ScratchRig");
    rig.setAttribute("schemaVersion", 1); rig.setAttribute("appVersion", data.appVersion);
    if (data.patch) rig.addChildElement(new juce::XmlElement(*data.patch));
    auto* master = rig.createNewChildElement("MasterInsert");
    master->setAttribute("bypassed", data.masterInsertBypassed);
    master->addTextElement(data.masterInsertState.toBase64Encoding());
    auto* gains = rig.createNewChildElement("Gains");
    gains->setAttribute("masterInputDb", data.masterInputDb); gains->setAttribute("masterOutputDb", data.masterOutputDb);
    for (int ch = 0; ch < 16; ++ch)
    {
        auto* input = gains->createNewChildElement("Input"); input->setAttribute("channel", ch); input->setAttribute("gainDb", data.inputChannelDb[ch]);
        auto* output = gains->createNewChildElement("Output"); output->setAttribute("channel", ch); output->setAttribute("gainDb", data.outputChannelDb[ch]);
    }
    auto* device = rig.createNewChildElement("Device");
    device->setAttribute("name", data.context.deviceName); device->setAttribute("sampleRate", data.context.sampleRate);
    device->setAttribute("rawChannels", data.context.rawChannelCount); device->setAttribute("wetChannels", data.context.wetChannelCount);
    device->setAttribute("patchName", data.context.patchName); device->setAttribute("patchIndex", data.context.patchIndex);
    device->setAttribute("documentPath", data.context.documentPath);
    auto* assets = rig.createNewChildElement("Assets"); assets->setAttribute("opaquePluginDependenciesUnenumerated", true);
    for (const auto& asset : data.assets)
    {
        auto* child = assets->createNewChildElement("Asset"); child->setAttribute("nodePath", asset.nodePath);
        child->setAttribute("kind", asset.kind); child->setAttribute("path", asset.file.getFullPathName());
    }
    return rig.toString();
}

juce::Result writeScratchRig(const juce::File& directory, const juce::String& xml)
{
    return writeScratchTextAtomically(directory.getChildFile("rig.xml"), xml);
}
