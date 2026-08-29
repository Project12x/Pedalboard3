#include <catch2/catch_test_macros.hpp>

#include "BypassableInstance.h"
#include "LinkAudioInputProcessor.h"
#include "LinkAudioService.h"
#include "MeteringCallbackBounds.h"

namespace
{
// Minimal AudioPluginInstance double, just enough for BypassableInstance to
// wrap - mirrors the pattern already used in rt_hosting_sprint_test.cpp.
class SilentTestPlugin final : public juce::AudioPluginInstance
{
  public:
    const juce::String getName() const override { return "SilentTestPlugin"; }
    void prepareToPlay(double, int) override {}
    void releaseResources() override {}
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override {}
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    bool hasEditor() const override { return false; }
    juce::AudioProcessorEditor* createEditor() override { return nullptr; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override {}
    void setStateInformation(const void*, int) override {}
    void fillInPluginDescription(juce::PluginDescription& description) const override
    {
        description.name = getName();
        description.pluginFormatName = "Test";
    }
};
}  // namespace

TEST_CASE("Link Audio keeps Pedalboard3's engine channel width", "[link-audio][channels]")
{
    LinkAudioInputProcessor input;

    REQUIRE(LinkAudioService::maxChannels == MeteringCallbackBounds::MaxChannels);
    REQUIRE(input.getTotalNumInputChannels() == 0);
    REQUIRE(input.getTotalNumOutputChannels() == LinkAudioService::maxChannels);
}

TEST_CASE("Link Audio Input is silent without an active Link service", "[link-audio][safety]")
{
    LinkAudioService::setActiveInstance(nullptr);
    LinkAudioInputProcessor input;
    juce::AudioBuffer<float> buffer(LinkAudioService::maxChannels, 64);
    buffer.applyGain(0, 0, buffer.getNumSamples(), 1.0f);
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        juce::FloatVectorOperations::fill(buffer.getWritePointer(channel), 0.75f, buffer.getNumSamples());
    juce::MidiBuffer midi;

    input.processBlock(buffer, midi);

    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
            CHECK(buffer.getSample(channel, sample) == 0.0f);
}

TEST_CASE("Link Audio service defaults to a safe disabled state", "[link-audio][settings]")
{
    LinkAudioService service;

    CHECK_FALSE(service.isEnabled());
    CHECK(service.getSelectedIncomingChannel() == -1);
    CHECK(service.getPeerName() == "Pedalboard3");
}

TEST_CASE("LinkAudioService active-instance accessor round-trips", "[link-audio][safety]")
{
    LinkAudioService service;

    LinkAudioService::setActiveInstance(&service);
    CHECK(LinkAudioService::getActiveInstance() == &service);

    LinkAudioService::setActiveInstance(nullptr);
    CHECK(LinkAudioService::getActiveInstance() == nullptr);
}

TEST_CASE("LinkAudioService per-node sink registry is safe with Link Audio compiled out", "[link-audio][node-sink]")
{
    // Test builds always compile with PEDALBOARD3_ENABLE_LINK_AUDIO=0 (see
    // tests/CMakeLists.txt), so registerNodeSink can never hand back a real
    // sink here - this exercises the same "safe when unavailable" path a
    // release build takes whenever Link Audio is simply turned off.
    LinkAudioService service;

    auto* slot = service.registerNodeSink(juce::AudioProcessorGraph::NodeID(42), 2, "Test Node");
    CHECK(slot == nullptr);

    // Must not crash with nothing registered.
    service.unregisterNodeSink(juce::AudioProcessorGraph::NodeID(42));
    service.clearAllNodeSinks();

    CHECK(service.getAudioInputTapSlot() == nullptr);
    CHECK(service.getAudioOutputTapSlot() == nullptr);

    // Must not crash / must not dereference a null slot.
    service.publishNodeAudio(nullptr, nullptr, 0, 0);
}

TEST_CASE("LinkAudioService device I/O tap slots default to null and round-trip", "[link-audio][node-sink]")
{
    LinkAudioService service;

    CHECK(service.getAudioInputTapSlot() == nullptr);
    CHECK(service.getAudioOutputTapSlot() == nullptr);

    auto* slot = service.registerNodeSink(juce::AudioProcessorGraph::NodeID(7), 2, "Audio Input");
    service.setAudioInputTapSlot(slot);
    CHECK(service.getAudioInputTapSlot() == slot);

    service.setAudioInputTapSlot(nullptr);
    CHECK(service.getAudioInputTapSlot() == nullptr);
}

TEST_CASE("BypassableInstance is a Link Audio tap source with no live sink by default", "[link-audio][tap]")
{
    auto* plugin = new SilentTestPlugin();
    BypassableInstance wrapper(plugin);

    // BypassableInstance must be usable as an AudioTapSource, since that's
    // how every plugin/built-in effect node gets opted in to per-node Link
    // Audio publishing - see AudioTapSource.h.
    REQUIRE(dynamic_cast<AudioTapSource*>(&wrapper) != nullptr);
    CHECK(wrapper.getLinkAudioSinkSlot() == nullptr);

    wrapper.prepareToPlay(48000.0, 64);
    juce::AudioBuffer<float> audio(2, 64);
    audio.clear();
    juce::MidiBuffer midi;

    // Not opted in (default state) - processBlock must behave exactly as
    // before this feature existed, i.e. not crash and not require a sink.
    wrapper.processBlock(audio, midi);

    wrapper.setLinkAudioSinkSlot(nullptr);
    CHECK(wrapper.getLinkAudioSinkSlot() == nullptr);
    wrapper.processBlock(audio, midi);
}
