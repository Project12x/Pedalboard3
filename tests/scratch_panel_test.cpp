#include "ScratchPanelPresentation.h"
#include "ScratchPanelLayout.h"
#include <catch2/catch_test_macros.hpp>

TEST_CASE("Scratch selection follows directory across reordered and replaced snapshots", "[scratch][scratch-ui]")
{
    ScratchLibrarySnapshot snapshot;
    const auto root = juce::File::getSpecialLocation(juce::File::tempDirectory);
    for (int i = 0; i < 12; ++i)
    {
        ScratchTakeLoadResult entry;
        entry.take.takeDirectory = root.getChildFile("take-" + juce::String(i));
        entry.take.takeId = "duplicate";
        snapshot.entries.push_back(entry);
    }
    const auto selected = snapshot.entries[11].take.takeDirectory;
    REQUIRE(findScratchTakeRow(snapshot, selected) == 11);
    snapshot.entries.insert(snapshot.entries.begin(), ScratchTakeLoadResult{});
    ++snapshot.generation;
    REQUIRE(findScratchTakeRow(snapshot, selected) == 12);
    snapshot.root = selected.getParentDirectory().getParentDirectory();
    snapshot.scanning = true;
    snapshot.entries.clear();
    REQUIRE(findScratchTakeRow(snapshot, selected) == -1);
    REQUIRE(reconcileScratchSelection(snapshot, selected) == selected);
    snapshot.scanning = false;
    REQUIRE(reconcileScratchSelection(snapshot, selected) == juce::File());
}

TEST_CASE("Scratch malformed and future takes remain reveal-only", "[scratch][scratch-ui]")
{
    ScratchTakeLoadResult entry;
    entry.take.takeDirectory = juce::File::getSpecialLocation(juce::File::tempDirectory).getChildFile("bad-take");
    entry.rawReadable = entry.wetReadable = true; // State must gate actions, not just stale file flags.
    for (auto state : {ScratchTakeLoadState::Invalid, ScratchTakeLoadState::Unsupported})
    {
        entry.state = state;
        const auto actions = scratchTakeActions(entry);
        REQUIRE_FALSE(actions.play);
        REQUIRE_FALSE(actions.reamp);
        REQUIRE(actions.reveal);
    }
}

TEST_CASE("Scratch shortcuts leave text input and other windows alone", "[scratch][scratch-ui]")
{
    REQUIRE(shouldHandleScratchSpace(true, false));
    REQUIRE_FALSE(shouldHandleScratchSpace(true, true));
    REQUIRE_FALSE(shouldHandleScratchSpace(false, false));
}

TEST_CASE("Scratch interrupted takes expose only their readable audio sides", "[scratch][scratch-ui]")
{
    ScratchTakeLoadResult entry;
    entry.state = ScratchTakeLoadState::Incomplete;
    entry.take.takeDirectory = juce::File::getSpecialLocation(juce::File::tempDirectory).getChildFile("interrupted");
    entry.message = "Interrupted capture - audio pair incomplete";
    for (bool rawReadable : {false, true})
        for (bool wetReadable : {false, true})
        {
            entry.rawReadable = rawReadable;
            entry.wetReadable = wetReadable;
            const auto actions = scratchTakeActions(entry);
            CHECK(actions.play == wetReadable);
            CHECK(actions.reamp == rawReadable);
            CHECK(actions.reveal);
            CHECK_FALSE(entry.take.complete);
            CHECK_FALSE(entry.message.isEmpty());
        }
}

TEST_CASE("Scratch transport remains reachable at narrow and default sizes", "[scratch][scratch-ui]")
{
    for (int width : {360, 568, 648, 948})
    {
        const juce::Rectangle<int> bounds{0, 0, width, 84};
        const auto layout = ScratchPanelLayout::calculateTransportLayout(bounds);
        for (const auto rectangle : {layout.wet, layout.raw, layout.pause, layout.stop, layout.loop, layout.volume})
        { REQUIRE(bounds.contains(rectangle)); REQUIRE(rectangle.getWidth() >= 48); REQUIRE(rectangle.getHeight() >= 24); }
        REQUIRE_FALSE(layout.wet.intersects(layout.raw));
        REQUIRE_FALSE(layout.pause.intersects(layout.volume));
        REQUIRE_FALSE(layout.loop.intersects(layout.volume));
    }
}

TEST_CASE("Scratch capture phases explain disabled audition", "[scratch][scratch-ui]")
{
    for (auto state : {ScratchRecorderState::Preparing, ScratchRecorderState::Recording, ScratchRecorderState::Saving})
    {
        REQUIRE(scratchCaptureBlocksAudition(state));
        REQUIRE_FALSE(scratchAuditionDisabledReason(state).isEmpty());
    }
    REQUIRE_FALSE(scratchCaptureBlocksAudition(ScratchRecorderState::Saved));
    REQUIRE(scratchAuditionDisabledReason(ScratchRecorderState::Saved).isEmpty());
}
