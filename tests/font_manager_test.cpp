/**
 * @file font_manager_test.cpp
 * @brief Tests for the semantic typography system in FontManager
 *
 * Verifies that:
 * 1. All semantic font methods return valid, non-zero-height fonts
 * 2. The type scale ordering is correct (heading > subheading > body > ...)
 * 3. Expected heights match the defined scale
 * 4. Display/mono display fonts respect caller-specified sizes
 * 5. Bold variants are actually bold
 */

#include "../src/FontManager.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>


using Catch::Matchers::WithinAbs;

TEST_CASE("FontManager singleton", "[fonts]")
{
    SECTION("getInstance returns same instance")
    {
        auto& a = FontManager::getInstance();
        auto& b = FontManager::getInstance();
        REQUIRE(&a == &b);
    }

    SECTION("Fonts are available")
    {
        REQUIRE(FontManager::getInstance().areFontsAvailable());
    }
}

TEST_CASE("Semantic font methods return valid fonts", "[fonts]")
{
    auto& fm = FontManager::getInstance();

    SECTION("getHeadingFont")
    {
        Font f = fm.getHeadingFont();
        REQUIRE(f.getHeight() > 0.0f);
    }

    SECTION("getSubheadingFont")
    {
        Font f = fm.getSubheadingFont();
        REQUIRE(f.getHeight() > 0.0f);
    }

    SECTION("getBodyFont")
    {
        Font f = fm.getBodyFont();
        REQUIRE(f.getHeight() > 0.0f);
    }

    SECTION("getBodyBoldFont")
    {
        Font f = fm.getBodyBoldFont();
        REQUIRE(f.getHeight() > 0.0f);
    }

    SECTION("getLabelFont")
    {
        Font f = fm.getLabelFont();
        REQUIRE(f.getHeight() > 0.0f);
    }

    SECTION("getCaptionFont")
    {
        Font f = fm.getCaptionFont();
        REQUIRE(f.getHeight() > 0.0f);
    }

    SECTION("getBadgeFont")
    {
        Font f = fm.getBadgeFont();
        REQUIRE(f.getHeight() > 0.0f);
    }

    SECTION("getDisplayFont")
    {
        Font f = fm.getDisplayFont(48.0f);
        REQUIRE(f.getHeight() > 0.0f);
    }

    SECTION("getMonoDisplayFont")
    {
        Font f = fm.getMonoDisplayFont(32.0f);
        REQUIRE(f.getHeight() > 0.0f);
    }
}

TEST_CASE("Type scale ordering", "[fonts]")
{
    auto& fm = FontManager::getInstance();

    float headingH = fm.getHeadingFont().getHeight();
    float subheadingH = fm.getSubheadingFont().getHeight();
    float bodyH = fm.getBodyFont().getHeight();
    float labelH = fm.getLabelFont().getHeight();
    float captionH = fm.getCaptionFont().getHeight();
    float badgeH = fm.getBadgeFont().getHeight();

    SECTION("Heading > Subheading > Body > Label > Caption > Badge")
    {
        REQUIRE(headingH > subheadingH);
        REQUIRE(subheadingH > bodyH);
        REQUIRE(bodyH > labelH);
        REQUIRE(labelH > captionH);
        REQUIRE(captionH > badgeH);
    }
}

TEST_CASE("Type scale exact heights", "[fonts]")
{
    auto& fm = FontManager::getInstance();

    SECTION("Heading is 18px")
    {
        REQUIRE_THAT(fm.getHeadingFont().getHeight(), WithinAbs(18.0, 0.5));
    }

    SECTION("Subheading is 15px")
    {
        REQUIRE_THAT(fm.getSubheadingFont().getHeight(), WithinAbs(15.0, 0.5));
    }

    SECTION("Body is 13px")
    {
        REQUIRE_THAT(fm.getBodyFont().getHeight(), WithinAbs(13.0, 0.5));
    }

    SECTION("BodyBold is 13px")
    {
        REQUIRE_THAT(fm.getBodyBoldFont().getHeight(), WithinAbs(13.0, 0.5));
    }

    SECTION("Label is 12px")
    {
        REQUIRE_THAT(fm.getLabelFont().getHeight(), WithinAbs(12.0, 0.5));
    }

    SECTION("Caption is 11px")
    {
        REQUIRE_THAT(fm.getCaptionFont().getHeight(), WithinAbs(11.0, 0.5));
    }

    SECTION("Badge is 9px")
    {
        REQUIRE_THAT(fm.getBadgeFont().getHeight(), WithinAbs(9.0, 0.5));
    }
}

TEST_CASE("Display fonts respect caller size", "[fonts]")
{
    auto& fm = FontManager::getInstance();

    SECTION("getDisplayFont uses requested height")
    {
        REQUIRE_THAT(fm.getDisplayFont(48.0f).getHeight(), WithinAbs(48.0, 0.5));
        REQUIRE_THAT(fm.getDisplayFont(72.0f).getHeight(), WithinAbs(72.0, 0.5));
        REQUIRE_THAT(fm.getDisplayFont(24.0f).getHeight(), WithinAbs(24.0, 0.5));
    }

    SECTION("getMonoDisplayFont uses requested height")
    {
        REQUIRE_THAT(fm.getMonoDisplayFont(32.0f).getHeight(), WithinAbs(32.0, 0.5));
        REQUIRE_THAT(fm.getMonoDisplayFont(16.0f).getHeight(), WithinAbs(16.0, 0.5));
    }
}

TEST_CASE("Bold variants are bold", "[fonts]")
{
    auto& fm = FontManager::getInstance();

    SECTION("BodyBold is bold, Body is not")
    {
        REQUIRE(fm.getBodyBoldFont().isBold());
        REQUIRE_FALSE(fm.getBodyFont().isBold());
    }

    SECTION("Heading is bold")
    {
        REQUIRE(fm.getHeadingFont().isBold());
    }

    SECTION("Badge is bold")
    {
        REQUIRE(fm.getBadgeFont().isBold());
    }

    SECTION("Caption is not bold")
    {
        REQUIRE_FALSE(fm.getCaptionFont().isBold());
    }

    SECTION("Label is not bold")
    {
        REQUIRE_FALSE(fm.getLabelFont().isBold());
    }
}

TEST_CASE("Low-level API still works", "[fonts]")
{
    auto& fm = FontManager::getInstance();

    SECTION("getUIFont returns valid font")
    {
        Font f = fm.getUIFont(14.0f);
        REQUIRE(f.getHeight() > 0.0f);
    }

    SECTION("getUIFont bold")
    {
        Font f = fm.getUIFont(14.0f, true);
        REQUIRE(f.isBold());
    }

    SECTION("getMonoFont returns valid font")
    {
        Font f = fm.getMonoFont(12.0f);
        REQUIRE(f.getHeight() > 0.0f);
    }
}
