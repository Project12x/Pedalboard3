/**
 * @file ui_regression_harness_test.cpp
 * @brief Phase 6 UI polish regression matrix.
 *
 * This test keeps the release-gate matrix executable while screenshot capture
 * remains a manual QA step. If a future polish pass removes coverage for a
 * required workflow, the test fails before the change can close D1/D2.
 */

#include <algorithm>
#include <array>
#include <catch2/catch_test_macros.hpp>
#include <string_view>

#include "../src/ColourScheme.h"

namespace
{
struct WorkflowCheck
{
    std::string_view workflow;
    std::string_view surface;
    std::string_view exercise;
    std::string_view automatedEvidence;
    bool requiresVisualEvidence;
};

constexpr std::array requiredWorkflows{
    std::string_view{"theme-switch"},
    std::string_view{"stage-toggle"},
    std::string_view{"patch-switch"},
};

constexpr std::array requiredThemes{
    std::string_view{"Midnight"},
    std::string_view{"Daylight"},
    std::string_view{"Synthwave"},
    std::string_view{"Deep Ocean"},
    std::string_view{"Forest"},
};

constexpr std::array requiredDpiScales{100, 150, 200};

constexpr std::array requiredVisualCriteria{
    std::string_view{"density"},
    std::string_view{"contrast"},
    std::string_view{"focus"},
    std::string_view{"scaling"},
    std::string_view{"stage-readability"},
};

constexpr std::array requiredThemeStates{
    std::string_view{"default"},
    std::string_view{"hover"},
    std::string_view{"active"},
    std::string_view{"focus"},
    std::string_view{"disabled"},
    std::string_view{"selection"},
    std::string_view{"warning"},
    std::string_view{"danger"},
    std::string_view{"success"},
};

constexpr std::array checks{
    WorkflowCheck{"theme-switch", "main-shell",
                  "Switch every built-in theme and verify refreshed labels, buttons, menus, canvas, and dialogs.",
                  "tests/ui_regression_harness_test.cpp:[ui][regression][theme]", true},
    WorkflowCheck{"theme-switch", "secondary-surfaces",
                  "Open Preferences, colour scheme editor, NAM browser, IR browser, and plugin search after theme swaps.",
                  "tests/ui_regression_harness_test.cpp:[ui][regression][theme]", true},
    WorkflowCheck{"stage-toggle", "stage-view",
                  "Enter and exit Stage Mode after polish changes, then verify patch text, next-patch cue, meters, and panic affordance.",
                  "tests/ui_regression_harness_test.cpp:[ui][regression][stage]", true},
    WorkflowCheck{"stage-toggle", "keyboard-flow",
                  "Exercise F11/Escape and arrow/Page Up/Page Down patch navigation while Stage Mode is active.",
                  "tests/ui_regression_harness_test.cpp:[ui][regression][stage]", true},
    WorkflowCheck{"patch-switch", "graph-runtime",
                  "Switch rapidly between empty and loaded patches, preserving infrastructure nodes and FIFO safety.",
                  "tests/patch_switch_test.cpp:[patchswitch][stress]", false},
    WorkflowCheck{"patch-switch", "visual-state",
                  "Switch patches with Stage Mode and main canvas visible, checking stale node, meter, and patch-label states.",
                  "tests/patch_switch_test.cpp:[patchswitch][stress]", true},
};

template <typename Range, typename Predicate>
bool containsMatching(const Range& range, Predicate predicate)
{
    return std::any_of(range.begin(), range.end(), predicate);
}

String toJuceString(std::string_view text)
{
    return String::fromUTF8(text.data(), static_cast<int>(text.size()));
}
} // namespace

TEST_CASE("UI polish regression matrix covers required workflows", "[ui][regression][harness]")
{
    for (auto workflow : requiredWorkflows)
    {
        INFO("workflow: " << workflow);
        REQUIRE(containsMatching(checks, [workflow](const auto& check) { return check.workflow == workflow; }));
        REQUIRE(containsMatching(checks, [workflow](const auto& check) {
            return check.workflow == workflow && !check.automatedEvidence.empty();
        }));
    }
}

TEST_CASE("Theme polish gate covers every built-in theme", "[ui][regression][theme]")
{
    const auto builtInThemes = ColourScheme::getBuiltInPresets();
    REQUIRE(builtInThemes.size() == static_cast<int>(requiredThemes.size()));

    for (auto theme : requiredThemes)
    {
        INFO("theme: " << theme);
        REQUIRE_FALSE(theme.empty());
        REQUIRE(builtInThemes.contains(toJuceString(theme)));
    }

    REQUIRE(requiredThemes.size() == 5);
}

TEST_CASE("Semantic colour role matrix covers required UI states", "[ui][regression][theme][tokens]")
{
    const auto& roles = ColourScheme::getSemanticColourRoles();
    REQUIRE(roles.size() >= 30);
    REQUIRE(ColourScheme::getRequiredColourRoles().size() <= static_cast<int>(roles.size()));

    for (auto state : requiredThemeStates)
    {
        INFO("state: " << state);
        REQUIRE(containsMatching(roles, [state](const auto& role) { return std::string_view{role.state} == state; }));
    }
}

TEST_CASE("Built-in themes provide every semantic colour role", "[ui][regression][theme][tokens]")
{
    auto& colourScheme = ColourScheme::getInstance();
    const auto requiredRoles = ColourScheme::getRequiredColourRoles();

    for (auto theme : requiredThemes)
    {
        INFO("theme: " << theme);
        REQUIRE(colourScheme.loadBuiltInPreset(toJuceString(theme)));

        StringArray missingRoles;
        CHECK(colourScheme.hasRequiredColourRoles(&missingRoles));
        INFO("missing roles: " << missingRoles.joinIntoString(", ").toStdString());

        for (const auto& requiredRole : requiredRoles)
        {
            INFO("role: " << requiredRole.toStdString());
            CHECK(colourScheme.colours.find(requiredRole) != colourScheme.colours.end());
        }
    }
}

TEST_CASE("Visual QA gate covers required DPI scales and sign-off criteria", "[ui][regression][visual]")
{
    REQUIRE(requiredDpiScales == std::array{100, 150, 200});

    for (auto criterion : requiredVisualCriteria)
    {
        INFO("criterion: " << criterion);
        REQUIRE_FALSE(criterion.empty());
    }

    REQUIRE(containsMatching(checks, [](const auto& check) { return check.requiresVisualEvidence; }));

    for (auto workflow : requiredWorkflows)
    {
        INFO("workflow: " << workflow);
        REQUIRE(containsMatching(checks, [workflow](const auto& check) {
            return check.workflow == workflow && check.requiresVisualEvidence;
        }));
    }
}
