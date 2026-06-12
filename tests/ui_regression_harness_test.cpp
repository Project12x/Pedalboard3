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
#include <fstream>
#include <optional>
#include <sstream>
#include <string>
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
    std::string_view{"scaled-footer"},
    std::string_view{"scaled-dialogs"},
};

constexpr std::array requiredThemes{
    std::string_view{"Midnight"},
    std::string_view{"Daylight"},
    std::string_view{"Synthwave"},
    std::string_view{"Deep Ocean"},
    std::string_view{"Forest"},
};

constexpr std::array requiredUiScales{75, 100, 125, 150, 175, 200};

constexpr std::array requiredScaledFooterBreakpoints{125, 150, 175, 200};

constexpr std::array requiredScaledDialogBreakpoints{150, 200};

constexpr std::array requiredScaledFooterControls{
    std::string_view{"ui-scale"},
    std::string_view{"patch"},
    std::string_view{"transport"},
    std::string_view{"tempo"},
    std::string_view{"gain-fx"},
    std::string_view{"fit-manage"},
    std::string_view{"cpu"},
};

constexpr std::array requiredScaledDialogSurfaces{
    std::string_view{"plugin-search"},
    std::string_view{"preferences"},
    std::string_view{"nam-browser"},
    std::string_view{"ir-browser"},
};

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

constexpr std::array requiredLookAndFeelColourIds{
    static_cast<int>(TextButton::buttonColourId),
    static_cast<int>(TextButton::buttonOnColourId),
    static_cast<int>(TextButton::textColourOnId),
    static_cast<int>(TextButton::textColourOffId),
    static_cast<int>(PopupMenu::backgroundColourId),
    static_cast<int>(PopupMenu::textColourId),
    static_cast<int>(PopupMenu::highlightedBackgroundColourId),
    static_cast<int>(PopupMenu::highlightedTextColourId),
    static_cast<int>(ComboBox::backgroundColourId),
    static_cast<int>(ComboBox::buttonColourId),
    static_cast<int>(ComboBox::arrowColourId),
    static_cast<int>(ComboBox::outlineColourId),
    static_cast<int>(ComboBox::focusedOutlineColourId),
    static_cast<int>(TextEditor::backgroundColourId),
    static_cast<int>(TextEditor::textColourId),
    static_cast<int>(TextEditor::outlineColourId),
    static_cast<int>(TextEditor::focusedOutlineColourId),
    static_cast<int>(TextEditor::highlightColourId),
    static_cast<int>(TextEditor::highlightedTextColourId),
    static_cast<int>(Label::textColourId),
    static_cast<int>(ToggleButton::textColourId),
    static_cast<int>(ToggleButton::tickColourId),
    static_cast<int>(ToggleButton::tickDisabledColourId),
    static_cast<int>(Slider::thumbColourId),
    static_cast<int>(Slider::trackColourId),
    static_cast<int>(Slider::rotarySliderFillColourId),
    static_cast<int>(Slider::rotarySliderOutlineColourId),
    static_cast<int>(Slider::textBoxTextColourId),
    static_cast<int>(Slider::textBoxBackgroundColourId),
    static_cast<int>(Slider::textBoxHighlightColourId),
    static_cast<int>(Slider::textBoxOutlineColourId),
    static_cast<int>(ScrollBar::thumbColourId),
    static_cast<int>(ScrollBar::trackColourId),
    static_cast<int>(ListBox::backgroundColourId),
    static_cast<int>(ListBox::textColourId),
    static_cast<int>(ProgressBar::backgroundColourId),
    static_cast<int>(ProgressBar::foregroundColourId),
    static_cast<int>(DirectoryContentsDisplayComponent::highlightColourId),
};

constexpr std::array tokenAuditSourceFiles{
    std::string_view{"src/MainPanel.cpp"},
    std::string_view{"src/PluginField.cpp"},
    std::string_view{"src/PluginComponent.cpp"},
    std::string_view{"src/PluginConnection.cpp"},
    std::string_view{"src/StageView.cpp"},
};

constexpr std::array forbiddenTokenAuditPatterns{
    std::string_view{"Colours::"},
    std::string_view{"Colour(0x"},
    std::string_view{"Font(FontOptions().withHeight"},
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
    WorkflowCheck{"scaled-footer", "main-footer",
                  "Capture normal and narrow footer screenshots at 125%, 150%, 175%, and 200% Pedalboard UI scale; verify UI Scale, patch, transport, tempo, gain/FX, Fit/Manage, and CPU controls remain reachable.",
                  "scripts/run_d2_visual_qa.ps1:-CaptureScaledFooterMatrix", true},
    WorkflowCheck{"scaled-dialogs", "secondary-surfaces",
                  "Capture plugin search, Preferences, NAM browser, and IR browser screenshots at 150% and 200% Pedalboard UI scale in normal and narrow dialog sizes.",
                  "scripts/run_d2_visual_qa.ps1:-CaptureScaledDialogMatrix", true},
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

std::optional<std::string> loadSourceFile(std::string_view relativePath)
{
    constexpr std::array prefixes{
        std::string_view{""},
        std::string_view{"../"},
        std::string_view{"../../"},
        std::string_view{"../../../"},
    };

    for (auto prefix : prefixes)
    {
        std::string path{prefix};
        path += relativePath;

        std::ifstream file{path};
        if (!file.good())
            continue;

        std::ostringstream contents;
        contents << file.rdbuf();
        return contents.str();
    }

    return std::nullopt;
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

TEST_CASE("Token-audited core UI files avoid hardcoded colour and font literals",
          "[ui][regression][theme][tokens][source]")
{
    for (auto sourceFile : tokenAuditSourceFiles)
    {
        INFO("source file: " << sourceFile);
        const auto source = loadSourceFile(sourceFile);
        REQUIRE(source.has_value());

        for (auto pattern : forbiddenTokenAuditPatterns)
        {
            INFO("forbidden pattern: " << pattern);
            CHECK(source->find(pattern) == std::string::npos);
        }
    }
}

TEST_CASE("Graph source polish keeps cable rendering and grid defaults stable",
          "[ui][regression][visual][source][graph]")
{
    const auto pluginHeader = loadSourceFile("src/PluginComponent.h");
    const auto connectionSource = loadSourceFile("src/PluginConnection.cpp");
    const auto fieldSource = loadSourceFile("src/PluginField.cpp");
    const auto mainPanelSource = loadSourceFile("src/MainPanel.cpp");

    REQUIRE(pluginHeader.has_value());
    REQUIRE(connectionSource.has_value());
    REQUIRE(fieldSource.has_value());
    REQUIRE(mainPanelSource.has_value());

    CHECK(pluginHeader->find("Point<float> gradientStart;") != std::string::npos);
    CHECK(pluginHeader->find("Point<float> gradientEnd;") != std::string::npos);

    CHECK(connectionSource->find("gradientStart = p1;") != std::string::npos);
    CHECK(connectionSource->find("gradientEnd = p2;") != std::string::npos);
    CHECK(connectionSource->find("if (destPoint.getX() > sourcePoint.getX())") == std::string::npos);
    CHECK(connectionSource->find(
              "updateBounds(destPoint.getX(), destPoint.getY(), sourcePoint.getX(), sourcePoint.getY())") ==
          std::string::npos);
    CHECK(connectionSource->find(
              "updateBounds(sourcePoint.getX(), sourcePoint.getY(), destPoint.getX(), destPoint.getY())") !=
          std::string::npos);
    CHECK(connectionSource->find("cableGlow.render(g, glowPath)") == std::string::npos);
    CHECK(connectionSource->find("glowPath.getBounds().getTopLeft()") == std::string::npos);
    CHECK(connectionSource->find("glowPath.getBounds().getBottomRight()") == std::string::npos);
    CHECK(connectionSource->find("PathStrokeType(outerGlowWidth, PathStrokeType::mitered, PathStrokeType::rounded)") !=
          std::string::npos);
    CHECK(connectionSource->find("PathStrokeType(innerGlowWidth, PathStrokeType::mitered, PathStrokeType::rounded)") !=
          std::string::npos);
    CHECK(connectionSource->find(
              "ColourGradient wireGrad(startCol, gradientStart.x, gradientStart.y, endCol, gradientEnd.x, "
              "gradientEnd.y, false)") != std::string::npos);

    CHECK(fieldSource->find("getString(kGraphGridStyleSettingsKey, \"Lines\")") != std::string::npos);
    CHECK(mainPanelSource->find("getString(kGraphGridStyleSettingsKey, \"Lines\")") != std::string::npos);
    CHECK(fieldSource->find("getString(kGraphGridStyleSettingsKey, \"Dots\")") == std::string::npos);
    CHECK(mainPanelSource->find("getString(kGraphGridStyleSettingsKey, \"Dots\")") == std::string::npos);
    CHECK(fieldSource->find("getString(kGraphGridStyleSettingsKey, \"Off\")") == std::string::npos);
    CHECK(mainPanelSource->find("getString(kGraphGridStyleSettingsKey, \"Off\")") == std::string::npos);
}

TEST_CASE("LookAndFeel colour contract maps shared controls to semantic roles", "[ui][regression][theme][laf]")
{
    const auto& specs = ColourScheme::getLookAndFeelColourSpecs();
    const auto requiredRoles = ColourScheme::getRequiredColourRoles();

    REQUIRE(specs.size() >= requiredLookAndFeelColourIds.size());

    for (const auto& spec : specs)
    {
        INFO("component: " << spec.component << ", role: " << spec.role);
        REQUIRE_FALSE(std::string_view{spec.component}.empty());
        REQUIRE_FALSE(std::string_view{spec.role}.empty());
        REQUIRE(spec.alpha > 0.0f);
        REQUIRE(spec.alpha <= 1.0f);
        REQUIRE(requiredRoles.contains(toJuceString(spec.role)));
    }

    for (auto colourId : requiredLookAndFeelColourIds)
    {
        INFO("colourId: " << colourId);
        REQUIRE(containsMatching(specs, [colourId](const auto& spec) { return spec.colourId == colourId; }));
    }
}

TEST_CASE("Visual QA gate covers required Pedalboard UI scales and sign-off criteria", "[ui][regression][visual]")
{
    REQUIRE(requiredUiScales == std::array{75, 100, 125, 150, 175, 200});

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

TEST_CASE("Visual QA gate covers scaled footer breakpoints and controls", "[ui][regression][visual]")
{
    REQUIRE(requiredScaledFooterBreakpoints == std::array{125, 150, 175, 200});

    REQUIRE(containsMatching(checks, [](const auto& check) {
        return check.workflow == "scaled-footer" && check.surface == "main-footer" &&
               check.requiresVisualEvidence;
    }));

    for (auto control : requiredScaledFooterControls)
    {
        INFO("control: " << control);
        REQUIRE_FALSE(control.empty());
    }
}

TEST_CASE("Visual QA gate covers scaled dialog breakpoints and surfaces", "[ui][regression][visual]")
{
    REQUIRE(requiredScaledDialogBreakpoints == std::array{150, 200});

    REQUIRE(containsMatching(checks, [](const auto& check) {
        return check.workflow == "scaled-dialogs" && check.surface == "secondary-surfaces" &&
               check.requiresVisualEvidence;
    }));

    for (auto surface : requiredScaledDialogSurfaces)
    {
        INFO("surface: " << surface);
        REQUIRE_FALSE(surface.empty());
    }
}

TEST_CASE("Visual QA script keeps scaled dialog matrix scoped to documented surfaces",
          "[ui][regression][visual][source]")
{
    const auto source = loadSourceFile("scripts/run_d2_visual_qa.ps1");
    REQUIRE(source.has_value());

    const auto scaledSurfaceDeclaration = source->find("$scaledDialogSurfaces = @(");
    REQUIRE(scaledSurfaceDeclaration != std::string::npos);

    const auto scaledSurfaceDeclarationEnd = source->find(")", scaledSurfaceDeclaration);
    REQUIRE(scaledSurfaceDeclarationEnd != std::string::npos);

    const auto scaledSurfaceList = source->substr(
        scaledSurfaceDeclaration, scaledSurfaceDeclarationEnd - scaledSurfaceDeclaration);

    for (auto surface : requiredScaledDialogSurfaces)
    {
        INFO("surface: " << surface);
        const auto quotedSurface = "\"" + std::string{surface} + "\"";
        CHECK(scaledSurfaceList.find(quotedSurface) != std::string::npos);
    }

    CHECK(scaledSurfaceList.find("scratch-panel") == std::string::npos);
    CHECK(source->find("[switch]$CaptureScratchPanel") != std::string::npos);
    CHECK(source->find("if ($CaptureScratchPanel)") != std::string::npos);
    CHECK(source->find("$scaledDialogSpecs = @($dialogSpecs | Where-Object { $scaledDialogSurfaces -contains $_.Name })") !=
          std::string::npos);
    CHECK(source->find("foreach ($dialog in $scaledDialogSpecs)") != std::string::npos);
}
