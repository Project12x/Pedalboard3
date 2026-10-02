#include "ScratchPanelPresentation.h"
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
