#include "AtomicFileWriter.h"
#include "PluginQuarantine.h"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("quarantine is scoped by format and survives reload", "[plugins][quarantine]")
{
    juce::TemporaryFile temp;
    const auto file = temp.getFile();

    {
        PluginQuarantine quarantine(file);
        quarantine.record({ "AudioUnit", "vendor.twin", PluginQuarantineReason::timeout,
                            juce::Time::getCurrentTime(), "30 second timeout" });
        REQUIRE(quarantine.contains("AudioUnit", "vendor.twin"));
        REQUIRE_FALSE(quarantine.contains("VST3", "vendor.twin"));
    }

    REQUIRE(PluginQuarantine(file).contains("AudioUnit", "vendor.twin"));
}

TEST_CASE("persisted quarantine remains until explicitly cleared", "[plugins][quarantine]")
{
    juce::TemporaryFile temp;
    const auto file = temp.getFile();
    PluginQuarantine quarantine(file);
    REQUIRE(quarantine.record({ "VST3", "/Plugins/unsafe.vst3", PluginQuarantineReason::crash,
                                juce::Time::getCurrentTime(), "helper crash" }));

    PluginQuarantine reloaded(file);
    REQUIRE(reloaded.contains("VST3", "/Plugins/unsafe.vst3"));
    REQUIRE(reloaded.remove("VST3", "/Plugins/unsafe.vst3"));
    REQUIRE_FALSE(PluginQuarantine(file).contains("VST3", "/Plugins/unsafe.vst3"));
}

TEST_CASE("atomic replacement keeps existing target when writer fails", "[plugins][atomic replacement]")
{
    juce::TemporaryFile temp;
    const auto target = temp.getFile();
    REQUIRE(target.replaceWithText("original"));

    const auto writerThatFails = [] (const juce::File&, juce::StringRef) { return false; };
    REQUIRE_FALSE(replaceTextAtomically(target, "replacement", writerThatFails));
    REQUIRE(target.loadFileAsString() == "original");
}
