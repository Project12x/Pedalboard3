#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "../src/StageLayout.h"

TEST_CASE("Stage layout scales typography and controls with viewport", "[ui][regression][stage][layout]")
{
    const auto compact = StageLayout::calculateMetrics(760, 540, true);
    const auto wide = StageLayout::calculateMetrics(1920, 1080, true);

    CHECK(compact.patchNameFontHeight >= 42.0f);
    CHECK(wide.patchNameFontHeight > compact.patchNameFontHeight);
    CHECK(wide.patchNameFontHeight <= 86.0f);

    CHECK(compact.nextPatchFontHeight >= 22.0f);
    CHECK(wide.nextPatchFontHeight > compact.nextPatchFontHeight);

    CHECK(compact.navButtonWidth >= 104);
    CHECK(wide.navButtonWidth > compact.navButtonWidth);
    CHECK(wide.footerHeight >= compact.footerHeight);
}

TEST_CASE("Stage layout reserves non-overlapping live performance regions", "[ui][regression][stage][layout]")
{
    const auto withTuner = StageLayout::calculateMetrics(980, 740, true);
    const auto withoutTuner = StageLayout::calculateMetrics(980, 740, false);

    CHECK(withTuner.headerHeight >= 44);
    CHECK(withTuner.footerHeight >= 72);
    CHECK(withTuner.tunerHeight >= 128);
    CHECK(withTuner.patchAreaMinHeight >= 220);
    CHECK(withTuner.headerHeight + withTuner.footerHeight + withTuner.tunerHeight + withTuner.patchAreaMinHeight <=
          740);

    CHECK(withoutTuner.tunerHeight == 0);
    CHECK(withoutTuner.patchAreaMinHeight > withTuner.patchAreaMinHeight);
}

TEST_CASE("Stage tuner strip is reserved only for hero patch view", "[ui][regression][stage][layout]")
{
    CHECK(StageLayout::shouldReserveTunerStrip(true, false, true));
    CHECK_FALSE(StageLayout::shouldReserveTunerStrip(false, false, true));
    CHECK_FALSE(StageLayout::shouldReserveTunerStrip(true, true, true));
    CHECK_FALSE(StageLayout::shouldReserveTunerStrip(true, false, false));
}

TEST_CASE("Stage chrome and safety bar metrics preserve reachable controls", "[ui][regression][stage][layout]")
{
    const auto compact = StageLayout::calculateMetrics(760, 540, true);
    const auto wide = StageLayout::calculateMetrics(1920, 1080, true);

    CHECK(compact.topBarChipHeight < compact.headerHeight);
    CHECK(compact.utilityButtonHeight < compact.headerHeight);
    CHECK(compact.panicButtonHeight < compact.footerHeight);
    CHECK(compact.sliderTopOffset + compact.sliderHeight <= compact.footerHeight);
    CHECK(compact.meterTopOffset + compact.meterHeight * 2.0f + compact.meterChannelGap * 2.0f <
          compact.sliderTopOffset);

    CHECK(compact.heroHorizontalInset * 2 < 760);
    CHECK(compact.liveQueueRailWidth == 0);
    CHECK(compact.progressActiveWidth > compact.progressDotSize);
    CHECK(compact.maxProgressDots >= 6);

    CHECK(wide.liveQueueRailWidth >= 210);
    CHECK(wide.liveQueueRowHeight > 0);
    CHECK(wide.liveQueueHeaderHeight > 0);
    CHECK(wide.topBarChipWidth > compact.topBarChipWidth);
    CHECK(wide.meterWidth > compact.meterWidth);
    CHECK(wide.maxProgressDots >= compact.maxProgressDots);
}

TEST_CASE("Stage patch labels preserve short names and elide long names", "[ui][regression][stage][layout]")
{
    CHECK(StageLayout::elideLabel("Clean Lead", 18) == "Clean Lead");

    const auto elided = StageLayout::elideLabel("Very Long Worship Lead With Huge Ambient Tail", 18);
    CHECK(elided.length() <= 18);
    CHECK(elided.endsWith("..."));
    CHECK(elided.startsWith("Very Long"));

    CHECK(StageLayout::elideLabel("ABCDE", 3) == "ABC");
}

TEST_CASE("Stage grid bank labels and visible windows are deterministic", "[ui][regression][stage][layout]")
{
    CHECK(StageLayout::formatBankLabel(0) == "Bank A");
    CHECK(StageLayout::formatBankLabel(2) == "Bank C");
    CHECK(StageLayout::formatBankLabel(26) == "Bank 27");

    CHECK(StageLayout::collectVisibleBankIndices(0, 0, 3).empty());
    CHECK(StageLayout::collectVisibleBankIndices(1, 3, 5) == std::vector<int>{0, 1, 2});
    CHECK(StageLayout::collectVisibleBankIndices(4, 8, 3) == std::vector<int>{3, 4, 5});
    CHECK(StageLayout::collectVisibleBankIndices(0, 8, 3) == std::vector<int>{0, 1, 2});
    CHECK(StageLayout::collectVisibleBankIndices(7, 8, 3) == std::vector<int>{5, 6, 7});
}
