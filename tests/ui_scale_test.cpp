#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "../src/UiScale.h"

TEST_CASE("UI scale choices start at 75 percent and keep 100 percent as default", "[ui][scale]")
{
    REQUIRE(UiScale::supportedPercents() == std::array{75, 100, 125, 150, 175, 200});
    CHECK(UiScale::minimumPercent == 75);
    CHECK(UiScale::defaultPercent == 100);
    CHECK(UiScale::maximumPercent == 200);
}

TEST_CASE("UI scale accepts only supported preference choices", "[ui][scale]")
{
    CHECK(UiScale::isSupportedPercent(75));
    CHECK(UiScale::isSupportedPercent(100));
    CHECK(UiScale::isSupportedPercent(200));

    CHECK_FALSE(UiScale::isSupportedPercent(50));
    CHECK_FALSE(UiScale::isSupportedPercent(80));
    CHECK_FALSE(UiScale::isSupportedPercent(225));
}

TEST_CASE("UI scale coerces persisted values to the nearest supported choice", "[ui][scale]")
{
    CHECK(UiScale::normalisePercent(50) == 75);
    CHECK(UiScale::normalisePercent(76) == 75);
    CHECK(UiScale::normalisePercent(112) == 100);
    CHECK(UiScale::normalisePercent(113) == 125);
    CHECK(UiScale::normalisePercent(138) == 150);
    CHECK(UiScale::normalisePercent(225) == 200);
}

TEST_CASE("UI scale converts percentages into JUCE scale factors", "[ui][scale]")
{
    CHECK(UiScale::toScaleFactor(75) == Catch::Approx(0.75f));
    CHECK(UiScale::toScaleFactor(100) == Catch::Approx(1.0f));
    CHECK(UiScale::toScaleFactor(200) == Catch::Approx(2.0f));
}

TEST_CASE("Footer UI scale control hides before it crowds scaled toolbar controls", "[ui][scale]")
{
    CHECK(UiScale::footerControlMinimumWidth(75) == 720);
    CHECK(UiScale::footerControlMinimumWidth(100) == 960);
    CHECK(UiScale::footerControlMinimumWidth(150) == 1440);
    CHECK(UiScale::footerControlMinimumWidth(200) == 1920);

    CHECK(UiScale::shouldShowFooterControl(1024, 75));
    CHECK(UiScale::shouldShowFooterControl(1024, 100));
    CHECK_FALSE(UiScale::shouldShowFooterControl(1024, 125));
    CHECK_FALSE(UiScale::shouldShowFooterControl(1024, 200));
}

TEST_CASE("Footer switches to compact two-row layout when scaled controls would crowd", "[ui][scale]")
{
    CHECK(UiScale::shouldUseSingleRowFooter(1024, 100));
    CHECK_FALSE(UiScale::shouldUseSingleRowFooter(1024, 125));
    CHECK_FALSE(UiScale::shouldUseSingleRowFooter(1024, 200));
    CHECK(UiScale::shouldUseSingleRowFooter(1920, 200));

    CHECK(UiScale::footerHeight(1024, 100) == 40);
    CHECK(UiScale::footerHeight(1024, 150) == 72);
    CHECK(UiScale::footerHeight(1024, 200) == 72);
}

TEST_CASE("Visual QA UI scale override is parsed from command line", "[ui][scale][visual]")
{
    CHECK(UiScale::parseVisualQaOverride("--visual-qa-ui-scale=75").value_or(0) == 75);
    CHECK(UiScale::parseVisualQaOverride("--visual-qa-stage --visual-qa-ui-scale=150").value_or(0) == 150);
    CHECK(UiScale::parseVisualQaOverride("--visual-qa-ui-scale=175 --visual-qa-preferences").value_or(0) == 175);

    CHECK_FALSE(UiScale::parseVisualQaOverride("--visual-qa-stage").has_value());
    CHECK_FALSE(UiScale::parseVisualQaOverride("--visual-qa-ui-scale=80").has_value());
    CHECK_FALSE(UiScale::parseVisualQaOverride("--visual-qa-ui-scale=abc").has_value());
}
