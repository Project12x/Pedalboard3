#include "../src/BypassableInstance.h"
#include "../src/SafetyLimiter.h"
#include "../src/VirtualMidiInputProcessor.h"

#include <catch2/catch_test_macros.hpp>

#include <fstream>
#include <limits>
#include <memory>
#include <string>
#include <vector>

namespace
{
struct RecordedMidiEvent
{
    int samplePosition = 0;
    int channel = 0;
    int noteNumber = -1;
    int controllerNumber = -1;
    bool noteOn = false;
    bool allNotesOff = false;
    bool allSoundOff = false;
    bool resetAllControllers = false;
};

RecordedMidiEvent describeMidiEvent(const juce::MidiMessage& message, int samplePosition)
{
    RecordedMidiEvent event;
    event.samplePosition = samplePosition;
    event.channel = message.getChannel();
    event.noteOn = message.isNoteOn();
    event.noteNumber = message.isNoteOnOrOff() ? message.getNoteNumber() : -1;
    event.controllerNumber = message.isController() ? message.getControllerNumber() : -1;
    event.allNotesOff = message.isAllNotesOff();
    event.allSoundOff = message.isAllSoundOff();
    event.resetAllControllers = message.isResetAllControllers();
    return event;
}

std::vector<RecordedMidiEvent> describeMidiBuffer(const juce::MidiBuffer& midi)
{
    std::vector<RecordedMidiEvent> events;

    for (const auto metadata : midi)
        events.push_back(describeMidiEvent(metadata.getMessage(), metadata.samplePosition));

    return events;
}

class MidiRoutingProbePlugin final : public juce::AudioPluginInstance
{
public:
    const juce::String getName() const override { return "MidiRoutingProbe"; }
    void prepareToPlay(double, int) override {}
    void releaseResources() override {}
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return true; }
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

    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi) override
    {
        juce::ignoreUnused(buffer);
        receivedEvents = describeMidiBuffer(midi);

        if (clearInputDuringProcess)
            midi.clear();

        if (emitOutputNote)
            midi.addEvent(juce::MidiMessage::noteOn(10, 72, static_cast<juce::uint8>(100)), 3);
    }

    std::vector<RecordedMidiEvent> receivedEvents;
    bool clearInputDuringProcess = true;
    bool emitOutputNote = false;
};
}

static std::string readTextFileForRtTest(const char* path)
{
    std::ifstream file(path, std::ios::binary);
    REQUIRE(file.is_open());

    return std::string(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
}

TEST_CASE("Virtual MIDI processBlock uses RT-safe diagnostics", "[rt][virtual-midi]")
{
    VirtualMidiInputProcessor processor;
    processor.prepareToPlay(48000.0, 64);

    juce::AudioBuffer<float> audio(0, 64);
    juce::MidiBuffer midi;

    processor.addMidiMessage(juce::MidiMessage::noteOn(1, 60, static_cast<juce::uint8>(100)));
    processor.processBlock(audio, midi);

    REQUIRE(processor.getProcessBlockCallCount() == 1);
    REQUIRE(processor.getProducedMidiMessageCount() >= 1);
    REQUIRE_FALSE(midi.isEmpty());
    REQUIRE(VirtualMidiInputProcessor::getInstance() == &processor);
}

TEST_CASE("Virtual MIDI destructor does not clear a newer active instance", "[rt][virtual-midi]")
{
    auto first = std::make_unique<VirtualMidiInputProcessor>();
    VirtualMidiInputProcessor second;

    VirtualMidiInputProcessor::setInstance(first.get());
    VirtualMidiInputProcessor::setInstance(&second);
    first.reset();

    REQUIRE(VirtualMidiInputProcessor::getInstance() == &second);
    VirtualMidiInputProcessor::setInstance(nullptr);
}

TEST_CASE("BypassableInstance forwards nonmatching MIDI channel messages unchanged", "[rt][bypassable][midi]")
{
    auto* plugin = new MidiRoutingProbePlugin();
    plugin->emitOutputNote = true;
    BypassableInstance wrapper(plugin);
    wrapper.setMIDIChannel(2);
    wrapper.prepareToPlay(48000.0, 64);

    juce::AudioBuffer<float> audio(2, 64);
    audio.clear();

    juce::MidiBuffer midi;
    midi.addEvent(juce::MidiMessage::noteOn(2, 60, static_cast<juce::uint8>(100)), 1);
    midi.addEvent(juce::MidiMessage::noteOn(5, 65, static_cast<juce::uint8>(100)), 2);

    wrapper.processBlock(audio, midi);

    REQUIRE(plugin->receivedEvents.size() == 1);
    REQUIRE(plugin->receivedEvents[0].channel == 2);
    REQUIRE(plugin->receivedEvents[0].noteNumber == 60);

    const auto outputEvents = describeMidiBuffer(midi);
    REQUIRE(outputEvents.size() == 2);
    REQUIRE(outputEvents[0].channel == 5);
    REQUIRE(outputEvents[0].noteNumber == 65);
    REQUIRE(outputEvents[0].samplePosition == 2);
    REQUIRE(outputEvents[1].channel == 10);
    REQUIRE(outputEvents[1].noteNumber == 72);
    REQUIRE(outputEvents[1].samplePosition == 3);
}

TEST_CASE("BypassableInstance broadcasts panic MIDI messages through channel filters", "[rt][bypassable][midi]")
{
    auto* plugin = new MidiRoutingProbePlugin();
    BypassableInstance wrapper(plugin);
    wrapper.setMIDIChannel(2);
    wrapper.prepareToPlay(48000.0, 64);

    juce::AudioBuffer<float> audio(2, 64);
    audio.clear();

    juce::MidiBuffer midi;
    midi.addEvent(juce::MidiMessage::allNotesOff(5), 4);
    midi.addEvent(juce::MidiMessage::allSoundOff(6), 5);
    midi.addEvent(juce::MidiMessage::allControllersOff(7), 6);

    wrapper.processBlock(audio, midi);

    REQUIRE(plugin->receivedEvents.size() == 3);
    REQUIRE(plugin->receivedEvents[0].allNotesOff);
    REQUIRE(plugin->receivedEvents[1].allSoundOff);
    REQUIRE(plugin->receivedEvents[2].resetAllControllers);

    const auto outputEvents = describeMidiBuffer(midi);
    REQUIRE(outputEvents.size() == 3);
    REQUIRE(outputEvents[0].allNotesOff);
    REQUIRE(outputEvents[0].samplePosition == 4);
    REQUIRE(outputEvents[1].allSoundOff);
    REQUIRE(outputEvents[1].samplePosition == 5);
    REQUIRE(outputEvents[2].resetAllControllers);
    REQUIRE(outputEvents[2].samplePosition == 6);
}

TEST_CASE("BypassableInstance emits one downstream copy of pass-through panic MIDI", "[rt][bypassable][midi]")
{
    auto* plugin = new MidiRoutingProbePlugin();
    plugin->clearInputDuringProcess = false;
    BypassableInstance wrapper(plugin);
    wrapper.setMIDIChannel(2);
    wrapper.prepareToPlay(48000.0, 64);

    juce::AudioBuffer<float> audio(2, 64);
    audio.clear();

    juce::MidiBuffer midi;
    midi.addEvent(juce::MidiMessage::noteOn(2, 60, static_cast<juce::uint8>(100)), 1);
    midi.addEvent(juce::MidiMessage::allNotesOff(5), 4);
    midi.addEvent(juce::MidiMessage::allSoundOff(6), 5);
    midi.addEvent(juce::MidiMessage::allControllersOff(7), 6);

    wrapper.processBlock(audio, midi);

    REQUIRE(plugin->receivedEvents.size() == 4);
    REQUIRE(plugin->receivedEvents[0].channel == 2);
    REQUIRE(plugin->receivedEvents[0].noteNumber == 60);
    REQUIRE(plugin->receivedEvents[1].allNotesOff);
    REQUIRE(plugin->receivedEvents[2].allSoundOff);
    REQUIRE(plugin->receivedEvents[3].resetAllControllers);

    const auto outputEvents = describeMidiBuffer(midi);
    REQUIRE(outputEvents.size() == 4);
    REQUIRE(outputEvents[0].channel == 2);
    REQUIRE(outputEvents[0].noteNumber == 60);
    REQUIRE(outputEvents[0].samplePosition == 1);
    REQUIRE(outputEvents[1].allNotesOff);
    REQUIRE(outputEvents[1].samplePosition == 4);
    REQUIRE(outputEvents[2].allSoundOff);
    REQUIRE(outputEvents[2].samplePosition == 5);
    REQUIRE(outputEvents[3].resetAllControllers);
    REQUIRE(outputEvents[3].samplePosition == 6);
}

TEST_CASE("SafetyLimiter mutes invalid samples and unmute resets runtime state", "[rt][safety-limiter]")
{
    SafetyLimiterProcessor limiter;
    limiter.prepareToPlay(48000.0, 64);

    juce::AudioBuffer<float> buffer(2, 64);
    juce::MidiBuffer midi;
    buffer.clear();
    buffer.setSample(0, 12, std::numeric_limits<float>::quiet_NaN());

    limiter.processBlock(buffer, midi);
    REQUIRE(limiter.isMuted());

    limiter.unmute();
    REQUIRE_FALSE(limiter.isMuted());

    buffer.clear();
    buffer.setSample(0, 0, 0.25f);
    buffer.setSample(1, 0, 0.25f);
    limiter.processBlock(buffer, midi);

    REQUIRE_FALSE(limiter.isMuted());
}

TEST_CASE("SafetyLimiter detects sustained DC offset", "[rt][safety-limiter]")
{
    SafetyLimiterProcessor limiter;
    limiter.prepareToPlay(48000.0, 64);

    juce::AudioBuffer<float> buffer(2, 64);
    juce::MidiBuffer midi;

    for (int block = 0; block < 420 && !limiter.isMuted(); ++block)
    {
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            buffer.clear(ch, 0, buffer.getNumSamples());

        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
                buffer.setSample(ch, sample, 0.75f);

        limiter.processBlock(buffer, midi);
    }

    REQUIRE(limiter.isMuted());
}

TEST_CASE("NAMCore process path does not own model handoff", "[rt][nam]")
{
    const auto source = readTextFileForRtTest(PEDALBOARD3_SOURCE_DIR "/src/NAMCore.cpp");
    const auto processorSource = readTextFileForRtTest(PEDALBOARD3_SOURCE_DIR "/src/NAMProcessor.cpp");

    REQUIRE(source.find("stagedModel") == std::string::npos);
    REQUIRE(source.find("impl->model = std::move(resamplingModel);") != std::string::npos);
    REQUIRE(source.find("impl->a2Model = std::move(a2Model);") != std::string::npos);
    REQUIRE(source.find("impl->model = std::move(impl->stagedModel);") == std::string::npos);

    REQUIRE(processorSource.find("Deferred model load until processor is inactive") != std::string::npos);
    REQUIRE(processorSource.find("Deferred IR load until processor is inactive") != std::string::npos);
}

TEST_CASE("NAM A2 core build stays isolated from legacy runtime path", "[rt][nam][a2]")
{
    const auto cmake = readTextFileForRtTest(PEDALBOARD3_SOURCE_DIR "/CMakeLists.txt");
    const auto namCoreSource = readTextFileForRtTest(PEDALBOARD3_SOURCE_DIR "/src/NAMCore.cpp");
    const auto a2Version = readTextFileForRtTest(PEDALBOARD3_SOURCE_DIR
                                                 "/external/NeuralAmpModelerCoreA2/NAM/version.h");
    const auto a2License = readTextFileForRtTest(PEDALBOARD3_SOURCE_DIR "/external/NeuralAmpModelerCoreA2/LICENSE");
    const auto thirdPartyLicenses = readTextFileForRtTest(PEDALBOARD3_SOURCE_DIR "/THIRD_PARTY_LICENSES.md");

    const auto a2AdapterSource = readTextFileForRtTest(PEDALBOARD3_SOURCE_DIR "/src/NAMCoreA2.cpp");

    REQUIRE(cmake.find("add_library(Pedalboard3_NAMCoreA2 STATIC") != std::string::npos);
    REQUIRE(cmake.find("add_library(Pedalboard3_NAMCoreA2Adapter STATIC") != std::string::npos);
    REQUIRE(cmake.find("nam=pedalboard3_nam_a2") != std::string::npos);
    REQUIRE(cmake.find("Pedalboard3_NAMCoreA2Adapter") != std::string::npos);
    REQUIRE(cmake.find("cxx_std_20") != std::string::npos);
    REQUIRE(cmake.find("external/NeuralAmpModelerCoreA2") != std::string::npos);
    REQUIRE(cmake.find("NAM_ENABLE_A2_FAST") != std::string::npos);

    REQUIRE(namCoreSource.find("../external/NeuralAmpModelerCore/wrapper/ResamplingNAM.h") != std::string::npos);
    REQUIRE(namCoreSource.find("isArchitecture2Model") != std::string::npos);
    REQUIRE(namCoreSource.find("architecture_version") != std::string::npos);
    REQUIRE(namCoreSource.find("../external/NeuralAmpModelerCoreA2") == std::string::npos);

    REQUIRE(a2AdapterSource.find("#define nam pedalboard3_nam_a2") != std::string::npos);
    REQUIRE(a2AdapterSource.find("NumInputChannels() != 1") != std::string::npos);
    REQUIRE(a2AdapterSource.find("NumOutputChannels() != 1") != std::string::npos);
    REQUIRE(a2AdapterSource.find("impl->model->process(inputs, outputs, numSamples)") != std::string::npos);

    REQUIRE(a2Version.find("NEURAL_AMP_MODELER_DSP_VERSION_MINOR 5") != std::string::npos);
    REQUIRE(a2Version.find("NEURAL_AMP_MODELER_DSP_VERSION_PATCH 3") != std::string::npos);
    REQUIRE(a2License.find("MIT License") != std::string::npos);
    REQUIRE(thirdPartyLicenses.find("NeuralAmpModelerCore A2") != std::string::npos);
    REQUIRE(thirdPartyLicenses.find("9c7b185de346fe0725dea537bcee4bc38b5bb6d6") != std::string::npos);
}

TEST_CASE("SafePluginListComponent keeps plugin scanning off the message timer", "[rt][scanner]")
{
    const auto header = readTextFileForRtTest(PEDALBOARD3_SOURCE_DIR "/src/SafePluginScanner.h");
    const auto source = readTextFileForRtTest(PEDALBOARD3_SOURCE_DIR "/src/SafePluginScanner.cpp");

    REQUIRE(header.find("private juce::Thread") != std::string::npos);
    REQUIRE(header.find("void run() override;") != std::string::npos);

    const auto runStart = source.find("void SafePluginListComponent::run()");
    REQUIRE(runStart != std::string::npos);
    const auto timerStart = source.find("void SafePluginListComponent::timerCallback()");
    REQUIRE(timerStart != std::string::npos);
    const auto updateStart = source.find("void SafePluginListComponent::updateList()", timerStart);
    REQUIRE(updateStart != std::string::npos);

    const auto timerBody = source.substr(timerStart, updateStart - timerStart);
    REQUIRE(source.substr(runStart, timerStart - runStart).find("scanNextFile") != std::string::npos);
    REQUIRE(timerBody.find("scanNextFile") == std::string::npos);
}

TEST_CASE("Plugin scanner IPC uses bounded named-pipe waits", "[rt][scanner]")
{
    const auto source = readTextFileForRtTest(PEDALBOARD3_SOURCE_DIR "/src/PluginScannerClient.cpp");

    REQUIRE(source.find("PIPE_NOWAIT") != std::string::npos);
    REQUIRE(source.find("waitForScannerConnection") != std::string::npos);
    REQUIRE(source.find("PeekNamedPipe") != std::string::npos);
    REQUIRE(source.find("readExactWithTimeout") != std::string::npos);
    REQUIRE(source.find("kMaxScannerPayloadBytes") != std::string::npos);
    REQUIRE(source.find("SetCommTimeouts") == std::string::npos);
    REQUIRE(source.find("FlushFileBuffers") == std::string::npos);
}

TEST_CASE("FilterGraph restore prepares nodes before bounded callback-lock commit", "[rt][graph-restore]")
{
    const auto header = readTextFileForRtTest(PEDALBOARD3_SOURCE_DIR "/src/FilterGraph.h");
    const auto source = readTextFileForRtTest(PEDALBOARD3_SOURCE_DIR "/src/FilterGraph.cpp");

    REQUIRE(header.find("PreparedGraphRestore") != std::string::npos);
    REQUIRE(header.find("prepareRestoreFromXml") != std::string::npos);
    REQUIRE(header.find("commitPreparedRestore") != std::string::npos);

    const auto helperStart =
        source.find("std::unique_ptr<AudioProcessor> FilterGraph::createProcessorForXmlNode");
    const auto prepareStart = source.find("FilterGraph::PreparedGraphRestore FilterGraph::prepareRestoreFromXml");
    const auto commitStart = source.find("void FilterGraph::commitPreparedRestore");
    const auto restoreStart = source.find("void FilterGraph::restoreFromXml");

    REQUIRE(helperStart != std::string::npos);
    REQUIRE(prepareStart != std::string::npos);
    REQUIRE(commitStart != std::string::npos);
    REQUIRE(restoreStart != std::string::npos);
    REQUIRE(helperStart < prepareStart);
    REQUIRE(prepareStart < commitStart);
    REQUIRE(commitStart < restoreStart);

    const auto helperBody = source.substr(helperStart, prepareStart - helperStart);
    REQUIRE(helperBody.find("setStateInformation") != std::string::npos);
    REQUIRE(helperBody.find("graph.addNode") == std::string::npos);
    REQUIRE(helperBody.find("graph.clear") == std::string::npos);

    const auto prepareBody = source.substr(prepareStart, commitStart - prepareStart);
    REQUIRE(prepareBody.find("createProcessorForXmlNode") != std::string::npos);
    REQUIRE(prepareBody.find("graph.addNode") == std::string::npos);
    REQUIRE(prepareBody.find("graph.clear") == std::string::npos);

    const auto commitBody = source.substr(commitStart, restoreStart - commitStart);
    REQUIRE(commitBody.find("setStateInformation") == std::string::npos);

    const auto lockStart = commitBody.find("const juce::ScopedLock sl(graph.getCallbackLock())");
    REQUIRE(lockStart != std::string::npos);

    const auto clearPos = commitBody.find("graph.clear()", lockStart);
    const auto infraPos = commitBody.find("createInfrastructureNodes()", lockStart);
    const auto addNodePos = commitBody.find("graph.addNode", lockStart);
    const auto addConnectionPos = commitBody.find("graph.addConnection", lockStart);
    const auto removeIllegalPos = commitBody.find("graph.removeIllegalConnections()", lockStart);
    const auto propertiesPos = commitBody.find("node->properties.set", removeIllegalPos);
    const auto oscRegistrationPos = commitBody.find("registerMIDIProcessor", removeIllegalPos);
    const auto changedPos = commitBody.find("changed();");

    REQUIRE(clearPos != std::string::npos);
    REQUIRE(infraPos != std::string::npos);
    REQUIRE(addNodePos != std::string::npos);
    REQUIRE(addConnectionPos != std::string::npos);
    REQUIRE(removeIllegalPos != std::string::npos);
    REQUIRE(propertiesPos != std::string::npos);
    REQUIRE(oscRegistrationPos != std::string::npos);
    REQUIRE(changedPos != std::string::npos);

    REQUIRE(clearPos < infraPos);
    REQUIRE(infraPos < addNodePos);
    REQUIRE(addNodePos < addConnectionPos);
    REQUIRE(addConnectionPos < removeIllegalPos);
    REQUIRE(removeIllegalPos < propertiesPos);
    REQUIRE(propertiesPos < oscRegistrationPos);
    REQUIRE(oscRegistrationPos < changedPos);

    const auto restoreBody = source.substr(restoreStart);
    REQUIRE(restoreBody.find("prepareRestoreFromXml") != std::string::npos);
    REQUIRE(restoreBody.find("commitPreparedRestore") != std::string::npos);
    REQUIRE(restoreBody.find("graph.clear()") == std::string::npos);
    REQUIRE(restoreBody.find("graph.addNode") == std::string::npos);
}

TEST_CASE("MidiMappingManager callback defers app work through FIFO", "[rt][midi][mapping]")
{
    const auto source = readTextFileForRtTest(PEDALBOARD3_SOURCE_DIR "/src/MidiMappingManager.cpp");

    const auto callbackStart = source.find("void MidiMappingManager::midiCcReceived");
    REQUIRE(callbackStart != std::string::npos);
    const auto nextFunction = source.find("void MidiMappingManager::setAppFifo", callbackStart);
    REQUIRE(nextFunction != std::string::npos);

    const auto callbackBody = source.substr(callbackStart, nextFunction - callbackStart);
    REQUIRE(callbackBody.find("SettingsManager::getInstance()") == std::string::npos);
    REQUIRE(callbackBody.find("getFirstCommandTarget") == std::string::npos);
    REQUIRE(callbackBody.find("dynamic_cast<MainPanel*>") == std::string::npos);
    REQUIRE(callbackBody.find("invokeCommandFromOtherThread") == std::string::npos);
    REQUIRE(callbackBody.find("updateTempoFromOtherThread") == std::string::npos);
    REQUIRE(callbackBody.find("switchPatchFromProgramChange") == std::string::npos);

    REQUIRE(callbackBody.find("writeID") != std::string::npos);
    REQUIRE(callbackBody.find("writeTempo") != std::string::npos);
    REQUIRE(callbackBody.find("writePatchChange") != std::string::npos);
}
