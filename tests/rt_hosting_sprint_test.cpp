#include "../src/SafetyLimiter.h"
#include "../src/VirtualMidiInputProcessor.h"

#include <catch2/catch_test_macros.hpp>

#include <fstream>
#include <limits>
#include <memory>
#include <string>

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
    REQUIRE(source.find("impl->model = std::move(impl->stagedModel);") == std::string::npos);

    REQUIRE(processorSource.find("Deferred model load until processor is inactive") != std::string::npos);
    REQUIRE(processorSource.find("Deferred IR load until processor is inactive") != std::string::npos);
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
