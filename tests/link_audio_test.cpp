#include <catch2/catch_test_macros.hpp>

#include "LinkAudioInputProcessor.h"
#include "LinkAudioService.h"

TEST_CASE("Link Audio keeps Pedalboard3's 16-channel engine width", "[link-audio][channels]")
{
    LinkAudioInputProcessor input;

    REQUIRE(LinkAudioService::maxChannels == 16);
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
