#pragma once
#include "ScratchTake.h"
#include <catch2/catch_test_macros.hpp>

// Every fixture stays under the coordinator's process-local TEMP/TMP root.
struct ScratchTestFiles
{
    juce::File root = juce::File::getSpecialLocation(juce::File::tempDirectory)
        .getChildFile("scratch-" + juce::Uuid().toString());
    ScratchTestFiles() { REQUIRE(root.createDirectory().wasOk()); }
    ~ScratchTestFiles() { root.deleteRecursively(); }

    ScratchTake makeTake()
    {
        ScratchTakeContext context;
        context.rootDirectory = root;
        context.patchName = "Storage fixture";
        context.sampleRate = 48000;
        context.rawChannelCount = 1;
        context.wetChannelCount = 2;
        return ScratchTake::createPending(context);
    }

    static void wav(const juce::File& file, int channels = 1, int samples = 480, double rate = 48000)
    {
        juce::WavAudioFormat format;
        std::unique_ptr<juce::OutputStream> stream(file.createOutputStream());
        REQUIRE(stream != nullptr);
        auto writer = format.createWriterFor(stream, juce::AudioFormatWriter::Options{}
            .withSampleRate(rate).withNumChannels(channels).withBitsPerSample(24));
        REQUIRE(writer != nullptr);
        juce::AudioBuffer<float> audio(channels, samples);
        audio.clear();
        if (samples > 0)
            audio.setSample(0, samples * 3 / 4, 0.75f);
        REQUIRE(writer->writeFromAudioSampleBuffer(audio, 0, samples));
    }
};
