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
#include <cmath>
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

double srgbChannelToLinear(uint8 channel)
{
    const double srgb = static_cast<double>(channel) / 255.0;
    return srgb <= 0.03928 ? srgb / 12.92 : std::pow((srgb + 0.055) / 1.055, 2.4);
}

double relativeLuminance(Colour colour)
{
    return 0.2126 * srgbChannelToLinear(colour.getRed()) +
           0.7152 * srgbChannelToLinear(colour.getGreen()) +
           0.0722 * srgbChannelToLinear(colour.getBlue());
}

double contrastRatio(Colour a, Colour b)
{
    const auto lumA = relativeLuminance(a);
    const auto lumB = relativeLuminance(b);
    const auto lighter = std::max(lumA, lumB);
    const auto darker = std::min(lumA, lumB);
    return (lighter + 0.05) / (darker + 0.05);
}

Colour alphaComposite(Colour foreground, Colour background, float alpha)
{
    return Colour::fromFloatRGBA(foreground.getFloatRed() * alpha + background.getFloatRed() * (1.0f - alpha),
                                 foreground.getFloatGreen() * alpha + background.getFloatGreen() * (1.0f - alpha),
                                 foreground.getFloatBlue() * alpha + background.getFloatBlue() * (1.0f - alpha),
                                 1.0f);
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

TEST_CASE("Daylight theme uses layered off-white surfaces instead of plain white",
          "[ui][regression][theme][tokens]")
{
    auto& colourScheme = ColourScheme::getInstance();
    REQUIRE(colourScheme.loadBuiltInPreset("Daylight"));

    const auto maxChannel = [](Colour colour) {
        return std::max({colour.getRed(), colour.getGreen(), colour.getBlue()});
    };

    constexpr std::array softlyTintedLightSurfaces{
        std::string_view{"Window Background"},
        std::string_view{"Field Background"},
        std::string_view{"Plugin Background"},
        std::string_view{"Button Highlight"},
        std::string_view{"Text Editor Colour"},
        std::string_view{"Dialog Inner Background"},
        std::string_view{"Dialog Background"},
    };

    for (auto role : softlyTintedLightSurfaces)
    {
        const auto key = toJuceString(role);
        INFO("role: " << role);
        REQUIRE(colourScheme.colours.find(key) != colourScheme.colours.end());
        CHECK(colourScheme.colours[key].getARGB() != 0xFFFFFFFF);
        CHECK(maxChannel(colourScheme.colours[key]) <= 243);
    }

    CHECK(colourScheme.colours["Window Background"].getARGB() == 0xFFDDE5EA);
    CHECK(colourScheme.colours["Field Background"].getARGB() == 0xFFE3EBF0);
    CHECK(colourScheme.colours["Plugin Background"].getARGB() == 0xFFD0DAE2);
    CHECK(colourScheme.colours["Plugin Border"].getARGB() == 0xFF748390);
    CHECK(colourScheme.colours["Button Highlight"].getARGB() == 0xFFE6EDF1);
    CHECK(colourScheme.colours["Accent Colour"].getARGB() == 0xFF0077CC);
    CHECK(colourScheme.colours["Plugin Background"].getBrightness() <
          colourScheme.colours["Field Background"].getBrightness());
    CHECK(colourScheme.colours["Dialog Inner Background"].getBrightness() <
          colourScheme.colours["Field Background"].getBrightness());
    CHECK(colourScheme.colours["Plugin Border"].getBrightness() < 0.58f);
    CHECK(colourScheme.colours["Text Colour"].getBrightness() < 0.12f);

    const auto text = colourScheme.colours["Text Colour"];
    constexpr std::array textSurfaces{
        std::string_view{"Plugin Background"},
        std::string_view{"Field Background"},
        std::string_view{"Button Colour"},
        std::string_view{"Dialog Inner Background"},
    };

    for (auto role : textSurfaces)
    {
        const auto key = toJuceString(role);
        INFO("contrast role: " << role);
        const auto surface = colourScheme.colours[key];
        CHECK(contrastRatio(text, surface) >= 11.0);
        CHECK(contrastRatio(alphaComposite(text, surface, 0.62f), surface) >= 4.5);
        CHECK(contrastRatio(alphaComposite(text, surface, 0.46f), surface) >= 3.0);
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
    CHECK(connectionSource->find("const float outerGlowWidth = selected ? 11.0f : 7.5f;") != std::string::npos);
    CHECK(connectionSource->find("const float innerGlowWidth = selected ? 6.5f : 4.5f;") != std::string::npos);
    CHECK(connectionSource->find("cableColour.withAlpha(selected ? 0.12f : 0.045f)") != std::string::npos);
    CHECK(connectionSource->find("cableColour.withAlpha(selected ? 0.18f : 0.07f)") != std::string::npos);
    CHECK(connectionSource->find("PathStrokeType(outerGlowWidth, PathStrokeType::curved, PathStrokeType::rounded)") !=
          std::string::npos);
    CHECK(connectionSource->find("PathStrokeType(innerGlowWidth, PathStrokeType::curved, PathStrokeType::rounded)") !=
          std::string::npos);
    CHECK(connectionSource->find("PathStrokeType drawnType(4.4f, PathStrokeType::curved, PathStrokeType::rounded);") !=
          std::string::npos);
    CHECK(connectionSource->find("PathStrokeType hitType(18.0f, PathStrokeType::curved, PathStrokeType::rounded);") !=
          std::string::npos);
    CHECK(connectionSource->find("glowPath = tempPath;") != std::string::npos);
    CHECK(connectionSource->find(
              "ColourGradient wireGrad(startCol, gradientStart.x, gradientStart.y, endCol, gradientEnd.x, "
              "gradientEnd.y, false)") != std::string::npos);

    CHECK(fieldSource->find("getString(kGraphGridStyleSettingsKey, \"Lines\")") != std::string::npos);
    CHECK(mainPanelSource->find("getString(kGraphGridStyleSettingsKey, \"Lines\")") != std::string::npos);
    CHECK(fieldSource->find("getString(kGraphGridStyleSettingsKey, \"Dots\")") == std::string::npos);
    CHECK(mainPanelSource->find("getString(kGraphGridStyleSettingsKey, \"Dots\")") == std::string::npos);
    CHECK(fieldSource->find("getString(kGraphGridStyleSettingsKey, \"Off\")") == std::string::npos);
    CHECK(mainPanelSource->find("getString(kGraphGridStyleSettingsKey, \"Off\")") == std::string::npos);
    CHECK(fieldSource->find("const float gridSize = 24.0f;") != std::string::npos);
    CHECK(fieldSource->find("Colour gridCol = gridAccent.withAlpha(gridStyle == \"dots\" ? 0.060f : 0.030f);") !=
          std::string::npos);
    CHECK(fieldSource->find("g.drawLine(x, clip.getY(), x, clip.getBottom(), 0.45f);") != std::string::npos);
    CHECK(fieldSource->find("majorGridSize") == std::string::npos);
    CHECK(fieldSource->find("majorGridCol") == std::string::npos);
    CHECK(fieldSource->find("firstMajorX") == std::string::npos);
    CHECK(fieldSource->find("const float gridSize = 20.0f;") == std::string::npos);
}

TEST_CASE("Built-in node polish source contract covers label, notes, tuner, mixer, and splitter",
          "[ui][regression][visual][source][nodes]")
{
    const auto pluginSource = loadSourceFile("src/PluginComponent.cpp");
    const auto processorHeader = loadSourceFile("src/PedalboardProcessors.h");
    const auto labelSource = loadSourceFile("src/LabelControl.cpp");
    const auto notesSource = loadSourceFile("src/NotesControl.cpp");
    const auto markdownTokeniserSource = loadSourceFile("src/MarkdownTokeniser.cpp");
    const auto notesProcessorSource = loadSourceFile("src/NotesProcessor.h");
    const auto notesProcessorImplSource = loadSourceFile("src/NotesProcessor.cpp");
    const auto namProcessorHeader = loadSourceFile("src/NAMProcessor.h");
    const auto irProcessorHeader = loadSourceFile("src/IRLoaderProcessor.h");
    const auto reverbProcessorHeader = loadSourceFile("src/ReverbSCProcessor.h");
    const auto oscilloscopeProcessorHeader = loadSourceFile("src/OscilloscopeProcessor.h");
    const auto toneProcessorHeader = loadSourceFile("src/ToneGeneratorProcessor.h");
    const auto tunerProcessorHeader = loadSourceFile("src/TunerProcessor.h");
    const auto tunerSource = loadSourceFile("src/TunerControl.cpp");
    const auto routingSource = loadSourceFile("src/RoutingProcessors.cpp");
    const auto routingHeader = loadSourceFile("src/RoutingProcessors.h");
    const auto internalFiltersSource = loadSourceFile("src/InternalFilters.cpp");
    const auto internalFiltersHeader = loadSourceFile("src/InternalFilters.h");

    REQUIRE(pluginSource.has_value());
    REQUIRE(processorHeader.has_value());
    REQUIRE(labelSource.has_value());
    REQUIRE(notesSource.has_value());
    REQUIRE(markdownTokeniserSource.has_value());
    REQUIRE(notesProcessorSource.has_value());
    REQUIRE(notesProcessorImplSource.has_value());
    REQUIRE(namProcessorHeader.has_value());
    REQUIRE(irProcessorHeader.has_value());
    REQUIRE(reverbProcessorHeader.has_value());
    REQUIRE(oscilloscopeProcessorHeader.has_value());
    REQUIRE(toneProcessorHeader.has_value());
    REQUIRE(tunerProcessorHeader.has_value());
    REQUIRE(tunerSource.has_value());
    REQUIRE(routingSource.has_value());
    REQUIRE(routingHeader.has_value());
    REQUIRE(internalFiltersSource.has_value());
    REQUIRE(internalFiltersHeader.has_value());

    CHECK(pluginSource->find("bool isLabelNodeName(const String& name)") != std::string::npos);
    CHECK(pluginSource->find(
              "return name.equalsIgnoreCase(\"Label\") || name.equalsIgnoreCase(\"Label Node\");") !=
          std::string::npos);
    CHECK(pluginSource->find("bool isStickyNoteNodeName(const String& name)") != std::string::npos);
    CHECK(pluginSource->find(
              "return name.equalsIgnoreCase(\"Notes\") || name.equalsIgnoreCase(\"Note\");") !=
          std::string::npos);
    CHECK(pluginSource->find("std::unique_ptr<Drawable> createNoteCloseDrawable") != std::string::npos);
    CHECK(pluginSource->find("const bool stickyNoteNode = isStickyNoteNodeName(pluginName);") != std::string::npos);
    CHECK(pluginSource->find("const auto noteCloseBase = ColourScheme::getInstance().colours[\"Warning Colour\"];") !=
          std::string::npos);
    CHECK(pluginSource->find("closeUp = createNoteCloseDrawable(noteCloseBase.darker(0.18f), 0.76f);") !=
          std::string::npos);
    CHECK(pluginSource->find("closeOver = createNoteCloseDrawable(noteCloseBase.brighter(0.12f), 0.94f);") !=
          std::string::npos);
    CHECK(pluginSource->find("closeDown = createNoteCloseDrawable(noteCloseBase.darker(0.32f), 0.98f);") !=
          std::string::npos);
    CHECK(pluginSource->find("enum class EmbeddedNodeShellKind") != std::string::npos);
    CHECK(pluginSource->find("struct EmbeddedNodeShellPolicy") != std::string::npos);
    CHECK(pluginSource->find("EmbeddedNodeShellPolicy getEmbeddedNodeShellPolicy(const String& pluginName, const String& visualCategoryName = {})") !=
          std::string::npos);
    CHECK(processorHeader->find("struct NodeShellPolicy") != std::string::npos);
    CHECK(processorHeader->find("enum class Kind") != std::string::npos);
    CHECK(processorHeader->find("static NodeShellPolicy heroChassis(int topOffset, int heightPadding)") !=
          std::string::npos);
    CHECK(processorHeader->find("static NodeShellPolicy directPainted(bool suppressUtilityHostParamPin = false)") !=
          std::string::npos);
    CHECK(processorHeader->find("static NodeShellPolicy embeddedParameterSurface(int heightPadding)") !=
          std::string::npos);
    CHECK(processorHeader->find("static NodeShellPolicy compactPinLabels()") != std::string::npos);
    CHECK(processorHeader->find("virtual NodeShellPolicy getNodeShellPolicy() const { return {}; }") !=
          std::string::npos);
    CHECK(pluginSource->find("EmbeddedNodeShellPolicy adaptNodeShellPolicy(const PedalboardProcessor::NodeShellPolicy& source)") !=
          std::string::npos);
    CHECK(pluginSource->find("const auto advertisedPolicy = adaptNodeShellPolicy(proc->getNodeShellPolicy());") !=
          std::string::npos);
    CHECK(pluginSource->find("if (advertisedPolicy.hasProcessorContract)") != std::string::npos);
    CHECK(namProcessorHeader->find("NodeShellPolicy getNodeShellPolicy() const override { return NodeShellPolicy::heroChassis(70, 84); }") !=
          std::string::npos);
    CHECK(irProcessorHeader->find("NodeShellPolicy getNodeShellPolicy() const override { return NodeShellPolicy::heroChassis(78, 112); }") !=
          std::string::npos);
    CHECK(reverbProcessorHeader->find("NodeShellPolicy getNodeShellPolicy() const override { return NodeShellPolicy::embeddedParameterSurface(60); }") !=
          std::string::npos);
    CHECK(tunerProcessorHeader->find("NodeShellPolicy getNodeShellPolicy() const override { return NodeShellPolicy::directPainted(); }") !=
          std::string::npos);
    CHECK(oscilloscopeProcessorHeader->find("NodeShellPolicy getNodeShellPolicy() const override { return NodeShellPolicy::directPainted(true); }") !=
          std::string::npos);
    CHECK(toneProcessorHeader->find("NodeShellPolicy getNodeShellPolicy() const override { return NodeShellPolicy::directPainted(true); }") !=
          std::string::npos);
    CHECK(notesProcessorSource->find("NodeShellPolicy getNodeShellPolicy() const override { return NodeShellPolicy::directPainted(); }") !=
          std::string::npos);
    CHECK(routingHeader->find("NodeShellPolicy getNodeShellPolicy() const override { return NodeShellPolicy::compactPinLabels(); }") !=
          std::string::npos);
    CHECK(pluginSource->find("policy.kind = EmbeddedNodeShellKind::DirectPainted;") != std::string::npos);
    CHECK(pluginSource->find("policy.suppressesHostEditorButton = true;") != std::string::npos);
    CHECK(pluginSource->find("policy.suppressesHostMappingsButton = true;") != std::string::npos);
    CHECK(pluginSource->find("policy.suppressesHostBypassButton = true;") != std::string::npos);
    CHECK(pluginSource->find("policy.suppressesHostMidiOrParamPin = true;") != std::string::npos);
    CHECK(pluginSource->find("policy.drawsHostPinText = false;") != std::string::npos);
    CHECK(pluginSource->find("policy.showsHostTitleLabel = false;") != std::string::npos);
    CHECK(pluginSource->find("pluginName == \"Tuner\" || pluginName == \"Oscilloscope\" || pluginName == \"Tone Generator\" ||") !=
          std::string::npos);
    CHECK(pluginSource->find("isStickyNoteNodeName(pluginName);") !=
          std::string::npos);
    CHECK(pluginSource->find("int getEmbeddedNodeControlLeftOffset(int hostWidth, Point<int> compSize, const String& pluginName)") !=
          std::string::npos);
    CHECK(pluginSource->find("if (getEmbeddedNodeShellPolicy(pluginName).kind == EmbeddedNodeShellKind::DirectPainted)\n        return 0;") !=
          std::string::npos);
    CHECK(pluginSource->find("int getEmbeddedNodeControlLeftOffset(int hostWidth, Point<int> compSize, const EmbeddedNodeShellPolicy& policy)") !=
          std::string::npos);
    CHECK(pluginSource->find("if (policy.kind == EmbeddedNodeShellKind::DirectPainted)\n        return 0;") !=
          std::string::npos);
    CHECK(pluginSource->find("return (hostWidth / 2) - (compSize.getX() / 2);") != std::string::npos);
    CHECK(pluginSource->find("const auto controlShellPolicy = getEmbeddedNodeShellPolicy(node->getProcessor(), pluginName, visualCategoryName);") !=
          std::string::npos);
    CHECK(pluginSource->find("tempint = getEmbeddedNodeControlLeftOffset(getWidth(), compSize, controlShellPolicy);") !=
          std::string::npos);
    CHECK(pluginSource->find("if (shellPolicy.kind == EmbeddedNodeShellKind::DirectPainted)\n        {\n            w = compSize.getX();\n            h = compSize.getY();") !=
          std::string::npos);
    CHECK(pluginSource->find("if (shellPolicy.kind != EmbeddedNodeShellKind::DirectPainted)\n            h = compSize.getY() + getEmbeddedNodeControlHeightPadding(shellPolicy);") !=
          std::string::npos);
    CHECK(pluginSource->find("if (!onlyUpdateWidth && shellPolicy.kind != EmbeddedNodeShellKind::DirectPainted)\n        h += getNodeParameterControlsHeight();") !=
          std::string::npos);
    CHECK(pluginSource->find("int cx = getEmbeddedNodeControlLeftOffset(getWidth(), compSize, shellPolicy);") !=
          std::string::npos);
    CHECK(pluginSource->find("if (isStickyNoteNodeName(pluginName))\n        {\n            w = compSize.getX();\n            h = compSize.getY();") ==
          std::string::npos);
    CHECK(pluginSource->find("bool usesCompactHostPinLabels(const String& pluginName)") != std::string::npos);
    CHECK(pluginSource->find("return getEmbeddedNodeShellPolicy(pluginName).usesCompactHostPinLabels;") !=
          std::string::npos);
    CHECK(pluginSource->find("bool shouldDrawHostPinText(const String& pluginName)") != std::string::npos);
    CHECK(pluginSource->find("return getEmbeddedNodeShellPolicy(pluginName).drawsHostPinText;") !=
          std::string::npos);
    CHECK(pluginSource->find("return getEmbeddedNodeShellPolicy(pluginName).showsHostTitleLabel;") !=
          std::string::npos);
    CHECK(pluginSource->find("const float nodeBorderWidth = highlighted ? 0.82f : 0.58f;") != std::string::npos);
    CHECK(pluginSource->find("beingDragged ? 1.12f : 0.72f") != std::string::npos);
    CHECK(pluginSource->find("const bool labelNode = isLabelNodeName(pluginName);") != std::string::npos);
    CHECK(pluginSource->find("const auto shellPolicy = getEmbeddedNodeShellPolicy(node->getProcessor(), pluginName, visualCategoryName);") !=
          std::string::npos);
    CHECK(pluginSource->find("const bool suppressHostEditorButton = labelNode || shellPolicy.suppressesHostEditorButton;") !=
          std::string::npos);
    CHECK(pluginSource->find("const bool suppressHostMappingsButton = labelNode || shellPolicy.suppressesHostMappingsButton;") !=
          std::string::npos);
    CHECK(pluginSource->find("const bool suppressHostBypassButton = labelNode || shellPolicy.suppressesHostBypassButton;") !=
          std::string::npos);
    CHECK(pluginSource->find("if (!suppressHostEditorButton)") != std::string::npos);
    CHECK(pluginSource->find("if (!suppressHostMappingsButton)") != std::string::npos);
    CHECK(pluginSource->find("if (!suppressHostBypassButton)\n        {\n            bypassButton = new DrawableButton") !=
          std::string::npos);
    CHECK(pluginSource->find("bool suppressesHostParamPinForUtilityNode(const String& pluginName)") !=
          std::string::npos);
    CHECK(pluginSource->find("return getEmbeddedNodeShellPolicy(pluginName).suppressesUtilityHostParamPin;") !=
          std::string::npos);
    CHECK(pluginSource->find("bool shouldCreateHostMidiOrParamPin(AudioProcessor* plugin, const String& pluginName, int numInputs, int numOutputs)") !=
          std::string::npos);
    CHECK(pluginSource->find("const auto shellPolicy = getEmbeddedNodeShellPolicy(plugin, pluginName);") !=
          std::string::npos);
    CHECK(pluginSource->find("if (shellPolicy.suppressesHostMidiOrParamPin || suppressesHostParamPinForUtilityNode(pluginName))") !=
          std::string::npos);
    CHECK(pluginSource->find("if (shouldCreateHostMidiOrParamPin(plugin, pluginName, numIn, numOut) && pluginName != \"MIDI Input\")") !=
          std::string::npos);
    CHECK(pluginSource->find("String getMidiOrParameterPinLabel(AudioProcessor* plugin, const String& pluginName, bool outputPin)") !=
          std::string::npos);
    CHECK(pluginSource->find("g->addLineOfText(tempFont, getMidiOrParameterPinLabel(plugin, pluginName, false), 10.0f, y);") !=
          std::string::npos);
    CHECK(pluginSource->find("g->addLineOfText(tempFont, getMidiOrParameterPinLabel(plugin, pluginName, true),") !=
          std::string::npos);
    CHECK(pluginSource->find("bool shouldSuppressWholeNodeDragFrom(Component* component)") != std::string::npos);
    CHECK(pluginSource->find("dynamic_cast<Button*>(current) != nullptr || dynamic_cast<Slider*>(current) != nullptr") !=
          std::string::npos);
    CHECK(pluginSource->find("dynamic_cast<ComboBox*>(current) != nullptr || dynamic_cast<TextEditor*>(current) != nullptr") !=
          std::string::npos);
    CHECK(pluginSource->find("dynamic_cast<NodeParameterMiniControl*>(current) != nullptr") != std::string::npos);
    CHECK(pluginSource->find("dynamic_cast<ResizableBorderComponent*>(current) != nullptr") != std::string::npos);
    CHECK(pluginSource->find("comp->addMouseListener(this, true);") != std::string::npos);
    CHECK(pluginSource->find("auto localEvent = e.getEventRelativeTo(this);") != std::string::npos);
    CHECK(pluginSource->find("shouldSuppressWholeNodeDragFrom(e.originalComponent)") != std::string::npos);
    CHECK(pluginSource->find("isStickyNoteResizeHandleEvent(pluginName, e)") != std::string::npos);
    CHECK(pluginSource->find("dragX = localEvent.getPosition().getX();") != std::string::npos);
    CHECK(pluginSource->find("dragY = localEvent.getPosition().getY();") != std::string::npos);
    CHECK(pluginSource->find("// Title bar drag logic (only for events on PluginComponent itself)") == std::string::npos);
    const auto labelNodeMarker = pluginSource->find("const bool labelNode = isLabelNodeName(pluginName);");
    REQUIRE(labelNodeMarker != std::string::npos);
    const auto deleteButtonMarker =
        pluginSource->find("deleteButton = new DrawableButton(\"DeleteFilterButton\", DrawableButton::ImageRaw);",
                           labelNodeMarker);
    REQUIRE(deleteButtonMarker != std::string::npos);
    const auto optionalButtonSection = pluginSource->substr(labelNodeMarker, deleteButtonMarker - labelNodeMarker);
    CHECK(optionalButtonSection.find("deleteButton =") == std::string::npos);
    CHECK(pluginSource->find("if (bypassable != nullptr && bypassButton != nullptr)") != std::string::npos);
    CHECK(pluginSource->find("const auto shellPolicy = getEmbeddedNodeShellPolicy(plugin, pluginName, visualCategoryName);") !=
          std::string::npos);
    CHECK(pluginSource->find("bool showLabels = (!proc) || shellPolicy.drawsHostPinText;") !=
          std::string::npos);
    CHECK(pluginSource->find("const bool compactPinLabels = shellPolicy.usesCompactHostPinLabels;") !=
          std::string::npos);
    CHECK(pluginSource->find("bool useNumberedNames = compactPinLabels || ignorePinNames || (pluginName == \"Audio Output\");") !=
          std::string::npos);
    CHECK(pluginSource->find("bool useNumberedNames = compactPinLabels || ignorePinNames || (pluginName == \"Audio Input\");") !=
          std::string::npos);

    CHECK(labelSource->find("const auto paper = colours[\"Warning Colour\"].withMultipliedSaturation(0.32f).brighter(1.24f);") !=
          std::string::npos);
    CHECK(labelSource->find("paperPath.startNewSubPath") != std::string::npos);
    CHECK(labelSource->find("editor->setMultiLine(true, true);") != std::string::npos);
    CHECK(labelSource->find("g.fillRoundedRectangle(bounds.getX() + 4.0f") == std::string::npos);
    CHECK(notesSource->find("Colour notePaperColour()") != std::string::npos);
    CHECK(notesSource->find("return Colour(0xFFFEF7E0);") != std::string::npos);
    CHECK(notesSource->find("Colour noteInkColour()") != std::string::npos);
    CHECK(notesSource->find("return Colour(0xFF5C3D0F);") != std::string::npos);
    CHECK(notesSource->find("paperPath.startNewSubPath") != std::string::npos);
    CHECK(notesSource->find("auto header = bounds.withHeight(30.0f);") != std::string::npos);
    CHECK(notesSource->find("ColourGradient headerFill(noteHeaderTopColour()") != std::string::npos);
    CHECK(notesSource->find("g.drawText(\"N o t e\"") != std::string::npos);
    CHECK(notesSource->find("renderedText.draw(g, getTextAreaBounds().toFloat());") !=
          std::string::npos);
    CHECK(notesSource->find("editor->setMultiLine(true, true);") != std::string::npos);
    CHECK(notesSource->find("editor->setReturnKeyStartsNewLine(true);") != std::string::npos);
    CHECK(notesSource->find("editor->setScrollbarsShown(true);") != std::string::npos);
    CHECK(notesSource->find("editor->onTextChange = [this]()") != std::string::npos);
    CHECK(notesSource->find("constexpr const char* kNotesEmptyHint = \"Double click to edit note...\";") !=
          std::string::npos);
    CHECK(notesSource->find("String sanitiseNoteEditorText(String text)") != std::string::npos);
    CHECK(notesSource->find("editor->setText(sanitiseNoteEditorText(processor != nullptr ? processor->getText() : editor->getText()), false);") !=
          std::string::npos);
    CHECK(notesSource->find("processor->setText(sanitiseNoteEditorText(text));") != std::string::npos);
    CHECK(notesSource->find("Rectangle<int> NotesControl::getTextAreaBounds() const") != std::string::npos);
    CHECK(notesSource->find("void NotesControl::refreshWrappedTextLayout()") != std::string::npos);
    CHECK(notesSource->find("void NotesControl::resized()\n{\n    refreshWrappedTextLayout();\n}") !=
          std::string::npos);
    CHECK(notesSource->find("editor->setBounds(getTextAreaBounds());") !=
          std::string::npos);
    CHECK(notesSource->find("editor->applyFontToAllText(FontManager::getInstance().getBodyFont().withHeight(13.0f));") !=
          std::string::npos);
    CHECK(notesSource->find("CodeEditorComponent") == std::string::npos);
    CHECK(notesSource->find("CodeDocument") == std::string::npos);
    CHECK(notesSource->find("setSize(jmax(kMinNoteNodeWidth, initialSize.getX()), jmax(kMinNoteNodeHeight, initialSize.getY()));") !=
          std::string::npos);
    CHECK(notesSource->find("processor->updateEditorBounds(Rectangle<int>(0, 0, newWidth, newHeight));") !=
          std::string::npos);
    CHECK(notesSource->find("ctx.appendedAnyText") != std::string::npos);
    CHECK(notesSource->find("renderedText.setWordWrap(AttributedString::byChar);") != std::string::npos);
    CHECK(notesSource->find("renderedText.append(kNotesEmptyHint, FontManager::getInstance().getBodyFont().withHeight(13.0f),") !=
          std::string::npos);
    CHECK(notesSource->find("renderedText.append(markdown, FontManager::getInstance().getBodyFont().withHeight(13.0f), noteInkColour());") !=
          std::string::npos);
    CHECK(markdownTokeniserSource->find("Colours::white") == std::string::npos);
    CHECK(markdownTokeniserSource->find("Colour(0xFF5C3D0F)") != std::string::npos);
    CHECK(markdownTokeniserSource->find("Colour(0xFFB45309)") != std::string::npos);
    CHECK(notesProcessorSource->find("if (editorBounds.getWidth() > 0 && editorBounds.getHeight() > 0)") !=
          std::string::npos);
    CHECK(notesProcessorImplSource->find("xml.setAttribute(\"editorW\", size.getX());") != std::string::npos);
    CHECK(notesSource->find("int parentWidth = newWidth + 20;") == std::string::npos);

    const auto namSource = loadSourceFile("src/NAMControl.cpp");
    REQUIRE(namSource.has_value());
    CHECK(namSource->find("ampHostTextBright = cs.colours[\"Text Colour\"];") != std::string::npos);
    CHECK(namSource->find("ampPanelTextBright = ampInsetBg.contrasting(0.92f);") != std::string::npos);
    CHECK(namSource->find("const auto shellTextDim = isEmbeddedInGraphNode() ? laf.ampHostTextDim : laf.ampTextDim;") !=
          std::string::npos);
    CHECK(namSource->find("std::make_unique<Slider>(Slider::LinearVertical, Slider::TextBoxBelow)") !=
          std::string::npos);
    CHECK(namSource->find("void NAMControl::layoutParamEqBandDeck(Rectangle<int> deckBounds, bool embedded)") !=
          std::string::npos);
    CHECK(namSource->find("slider->setSliderStyle(Slider::LinearVertical);") != std::string::npos);
    CHECK(namSource->find("frequency->setTextBoxStyle(Slider::NoTextBox, false, 0, 0);") != std::string::npos);

    CHECK(tunerSource->find("auto tunerAccent = colours[\"Tuner Active Colour\"];") != std::string::npos);
    CHECK(tunerSource->find("ColourGradient panelFill(") != std::string::npos);
    CHECK(tunerSource->find("drawTunerGlassPanel(g, bounds);") != std::string::npos);
    CHECK(tunerSource->find("drawTunerHeader(g, headerArea);") != std::string::npos);
    CHECK(tunerSource->find("auto track = Rectangle<float>(16.0f, jmin(64.0f, slot.getHeight() - 17.0f))") !=
          std::string::npos);
    CHECK(tunerSource->find(
              "auto zone = Rectangle<float>(track.getX(), track.getCentreY() - 6.5f, track.getWidth(), 13.0f);") !=
          std::string::npos);
    CHECK(tunerSource->find("auto dot = Rectangle<float>(12.0f, 6.0f).withCentre({track.getCentreX(), y});") !=
          std::string::npos);
    CHECK(tunerSource->find("g.fillRoundedRectangle(dot, 3.0f);") != std::string::npos);
    CHECK(tunerSource->find("\"EMPTY\"") == std::string::npos);
    CHECK(tunerSource->find("if (bypassed || detected)\n    {\n        auto statePill") !=
          std::string::npos);
    CHECK(tunerSource->find("class TunerModeButtonLookAndFeel final : public LookAndFeel_V4") !=
          std::string::npos);
    CHECK(tunerSource->find("needleModeButton->setLookAndFeel(&tunerModeButtonLookAndFeel);") !=
          std::string::npos);
    CHECK(tunerSource->find("bypassButton->setLookAndFeel(&tunerModeButtonLookAndFeel);") !=
          std::string::npos);
    CHECK(tunerSource->find("Colour(0xFF00E676)") == std::string::npos);
    CHECK(tunerSource->find("Colour(0xFF76FF03)") == std::string::npos);
    CHECK(tunerSource->find("Colour(0xFFFFEB3B)") == std::string::npos);
    CHECK(tunerSource->find("Colour(0xFFFF9800)") == std::string::npos);
    CHECK(tunerSource->find("Colour(0xFFFF5252)") == std::string::npos);
    CHECK(tunerSource->find("Colours::white") == std::string::npos);
    CHECK(tunerSource->find("Colours::black") == std::string::npos);
    CHECK(tunerSource->find("Colours::yellow") == std::string::npos);
    const auto sixStringStart = tunerSource->find("void TunerControl::drawSixStringDisplay");
    const auto noteGlyphStart = tunerSource->find("void TunerControl::drawNoteGlyph", sixStringStart);
    REQUIRE(sixStringStart != std::string::npos);
    REQUIRE(noteGlyphStart != std::string::npos);
    const auto sixStringSource = tunerSource->substr(sixStringStart, noteGlyphStart - sixStringStart);
    CHECK(sixStringSource.find("auto centreLine = slot.withSizeKeepingCentre") == std::string::npos);
    CHECK(sixStringSource.find("g.fillEllipse(dot);") == std::string::npos);

    CHECK(routingSource->find("static Colour getRoutingNodeAccent()") != std::string::npos);
    CHECK(routingSource->find("Colour(0xFFCCAA00)") == std::string::npos);
    CHECK(routingSource->find("Colour(0xFFFF8800)") == std::string::npos);
    CHECK(routingSource->find("Colours::white") == std::string::npos);
    CHECK(routingSource->find("Colours::black") == std::string::npos);
    CHECK(routingSource->find("static void paintRoutingBadge(Graphics& g, Rectangle<float> bounds, const String& text, Colour accent, bool primary)") !=
          std::string::npos);
    CHECK(routingSource->find("const auto labelBase = primary ? colours[\"Plugin Background\"].interpolatedWith(accent, 0.28f) : base;") !=
          std::string::npos);
    CHECK(routingSource->find("labelBase.contrasting(0.90f)") != std::string::npos);
    CHECK(routingSource->find("static void paintRoutingNodeShell(Graphics& g, Rectangle<float> bounds, Colour accent, const String& title)") ==
          std::string::npos);
    CHECK(routingSource->find("static void paintRoutingMeterTrack(Graphics& g, Rectangle<float> bounds, float level, Colour accent, bool muted)") !=
          std::string::npos);
    CHECK(routingSource->find("static void paintMixerStripDeck(Graphics& g, Rectangle<float> bounds, Colour accent, const String& label)") !=
          std::string::npos);
    CHECK(routingSource->find("static void paintMixerMasterDeck(Graphics& g, Rectangle<float> bounds, Colour accent)") !=
          std::string::npos);
    CHECK(routingSource->find("static void paintMixerPanRail(Graphics& g, Rectangle<float> bounds, float pan, Colour accent)") !=
          std::string::npos);
    CHECK(routingSource->find("paintRoutingFanout(g, fanoutArea.toFloat(), getRoutingNodeAccent(), false, false,\n                           processor->getNumStrips());") !=
          std::string::npos);
    CHECK(routingSource->find("paintMixerStripDeck(g, stripDecks[ch].toFloat(), accent, getRoutingVisualLabel(ch));") !=
          std::string::npos);
    CHECK(routingSource->find("paintRoutingBadge(g, badgeAreas[ch].toFloat(), getRoutingVisualLabel(ch), accent, true);") !=
          std::string::npos);
    CHECK(routingSource->find("paintRoutingBadge(g, masterBadgeArea.toFloat(), \"M\", accent, true);") !=
          std::string::npos);
    CHECK(routingSource->find("addStripButton.setButtonText(\"+\");") != std::string::npos);
    CHECK(routingSource->find("removeStripButton.setButtonText(\"-\");") != std::string::npos);
    CHECK(routingSource->find("void addStripClicked()") != std::string::npos);
    CHECK(routingSource->find("void removeStripClicked()") != std::string::npos);
    CHECK(routingSource->find("Colour(0xFF00CC00)") == std::string::npos);
    CHECK(routingSource->find("Colour(0xFF008800)") == std::string::npos);

    CHECK(routingHeader->find("static constexpr int MaxStrips = 32;") != std::string::npos);
    CHECK(routingHeader->find("static constexpr int DefaultStrips = 2;") != std::string::npos);
    CHECK(routingHeader->find("struct StripState") != std::string::npos);
    CHECK(routingHeader->find("std::array<StripState, MaxStrips> strips_;") != std::string::npos);
    CHECK(routingHeader->find("std::atomic<int> numStrips_") != std::string::npos);
    CHECK(routingHeader->find("AudioBuffer<float> inputSnapshot_;") != std::string::npos);
    CHECK(routingHeader->find("int getNumStrips() const") != std::string::npos);
    CHECK(routingHeader->find("void addStrip();") != std::string::npos);
    CHECK(routingHeader->find("void removeStrip();") != std::string::npos);

    const auto userFacingStart = internalFiltersSource->find("void InternalPluginFormat::getUserFacingTypes");
    REQUIRE(userFacingStart != std::string::npos);
    const auto userFacingSource = internalFiltersSource->substr(userFacingStart);
    CHECK(userFacingSource.find("dawMixerProcFilter") == std::string::npos);
    CHECK(userFacingSource.find("dawSplitterProcFilter") == std::string::npos);
    CHECK(internalFiltersHeader->find("dawMixerProcFilter") == std::string::npos);
    CHECK(internalFiltersHeader->find("dawSplitterProcFilter") == std::string::npos);
}

TEST_CASE("Visible routing mixer and splitter nodes match mockup routing polish without losing controls",
          "[ui][regression][visual][source][nodes][routing]")
{
    const auto routingSource = loadSourceFile("src/RoutingProcessors.cpp");
    const auto routingHeader = loadSourceFile("src/RoutingProcessors.h");

    REQUIRE(routingSource.has_value());
    REQUIRE(routingHeader.has_value());

    CHECK(routingSource->find("static Colour getRoutingNodeAccent()") != std::string::npos);
    CHECK(routingSource->find("static void paintRoutingNodeShell(Graphics& g, Rectangle<float> bounds, Colour accent, const String& title)") ==
          std::string::npos);
    CHECK(routingSource->find("g.fillRoundedRectangle(outer.reduced(0.5f), 8.0f);") == std::string::npos);
    CHECK(routingSource->find("g.drawRoundedRectangle(outer.reduced(0.5f), 8.0f") == std::string::npos);
    CHECK(routingSource->find("g.fillRoundedRectangle(header.withHeight(4.0f).withY(header.getCentreY() - 2.0f), 2.0f);") ==
          std::string::npos);
    CHECK(routingSource->find("static void paintRoutingBadge(Graphics& g, Rectangle<float> bounds, const String& text, Colour accent, bool primary)") !=
          std::string::npos);
    CHECK(routingSource->find("static void paintRoutingFanout(Graphics& g, Rectangle<float> bounds, Colour accent, bool muteA, bool muteB,\n                               int routeCount = 2)") !=
          std::string::npos);
    CHECK(routingSource->find("static String getRoutingVisualLabel(int index)") != std::string::npos);
    CHECK(routingSource->find("static void paintRoutingMeterTrack(Graphics& g, Rectangle<float> bounds, float level, Colour accent, bool muted)") !=
          std::string::npos);
    CHECK(routingSource->find("static void paintMixerStripDeck(Graphics& g, Rectangle<float> bounds, Colour accent, const String& label)") !=
          std::string::npos);
    CHECK(routingSource->find("g.fillRect(deck);") != std::string::npos);
    CHECK(routingSource->find("g.drawVerticalLine(roundToInt(bounds.getRight()), bounds.getY() + 9.0f, bounds.getBottom() - 9.0f);") !=
          std::string::npos);
    CHECK(routingSource->find("static void paintMixerPanRail(Graphics& g, Rectangle<float> bounds, float pan, Colour accent)") !=
          std::string::npos);

    CHECK(routingSource->find("static void paintRoutingRow(Graphics& g, Rectangle<float> bounds, Colour accent, bool muted, bool input)") !=
          std::string::npos);
    CHECK(routingSource->find("muteButtons[index].setButtonText(\"M\");") != std::string::npos);
    CHECK(routingSource->find("stereoButtons[index].setButtonText(\"ST\");") != std::string::npos);
    CHECK(routingSource->find("paintRoutingFanout(g, fanoutArea.toFloat(), getRoutingNodeAccent(), false, false,\n                           processor->getNumStrips());") !=
          std::string::npos);
    CHECK(routingSource->find("std::array<TextButton, SplitterProcessor::MaxStrips> muteButtons;") !=
          std::string::npos);
    CHECK(routingSource->find("std::array<Rectangle<int>, SplitterProcessor::MaxStrips> outRows;") !=
          std::string::npos);
    CHECK(routingSource->find("inputSnapshot_.setSize(2, samplesPerBlock, false, true, true);") !=
          std::string::npos);
    CHECK(routingSource->find("FloatVectorOperations::copy(inputSnapshot_.getWritePointer(0)") !=
          std::string::npos);
    CHECK(routingSource->find("for (int i = 0; i < processor->getNumStrips(); ++i)") !=
          std::string::npos);
    CHECK(routingSource->find("paintRoutingBadge(g, outBadgeA.toFloat(), getRoutingVisualLabel(0), accent, !muteA.getToggleState());") ==
          std::string::npos);
    CHECK(routingSource->find("paintRoutingMeterTrack(g, laneA.toFloat(), 0.72f, accent, muteA.getToggleState());") ==
          std::string::npos);
    const auto splitterControlStart = routingSource->find("class SplitterControl : public Component, public Button::Listener");
    const auto splitterProcessorStart = routingSource->find("// SplitterProcessor Implementation");
    REQUIRE(splitterControlStart != std::string::npos);
    REQUIRE(splitterProcessorStart != std::string::npos);
    const auto visibleSplitterSource =
        routingSource->substr(splitterControlStart, splitterProcessorStart - splitterControlStart);
    CHECK(visibleSplitterSource.find("paintRoutingNodeShell(g, getLocalBounds().toFloat(), getRoutingNodeAccent(), {});") ==
          std::string::npos);

    CHECK(routingHeader->find("bool isVerticalLayout() const") != std::string::npos);
    CHECK(routingHeader->find("void setVerticalLayout(bool vertical)") != std::string::npos);
    CHECK(routingSource->find("layoutModeButton.setButtonText(processor->isVerticalLayout() ? \"V\" : \"H\");") !=
          std::string::npos);
    CHECK(routingSource->find("f.setSliderStyle(processor->isVerticalLayout() ? Slider::LinearVertical : Slider::LinearHorizontal);") !=
          std::string::npos);
    CHECK(routingSource->find("p.setSliderStyle(Slider::RotaryHorizontalVerticalDrag);") != std::string::npos);
    CHECK(routingSource->find("m.setButtonText(\"M\");") != std::string::npos);
    CHECK(routingSource->find("s.setButtonText(\"S\");") != std::string::npos);
    CHECK(routingSource->find("ph.setButtonText(CharPointer_UTF8") != std::string::npos);
    CHECK(routingSource->find("masterMuteButton.setButtonText(\"M\");") != std::string::npos);
    CHECK(routingSource->find("auto& mf = masterFader;") != std::string::npos);
    CHECK(routingSource->find("mf.setSliderStyle(processor->isVerticalLayout() ? Slider::LinearVertical : Slider::LinearHorizontal);") !=
          std::string::npos);
    CHECK(routingSource->find("paintRoutingBadge(g, badgeAreas[ch].toFloat(), getRoutingVisualLabel(ch), accent, true);") !=
          std::string::npos);
    CHECK(routingSource->find("paintMixerStripDeck(g, stripDecks[ch].toFloat(), accent, getRoutingVisualLabel(ch));") !=
          std::string::npos);
    CHECK(routingSource->find("paintMixerPanRail(g, panRails[ch].toFloat(), static_cast<float>(panKnobs[ch].getValue()), accent);") !=
          std::string::npos);
    CHECK(routingSource->find("drawControlLabel(g, panLabelAreas[ch].toFloat(), \"PAN\", cs);") !=
          std::string::npos);
    CHECK(routingSource->find("drawControlLabel(g, gainLabelAreas[ch].toFloat(), \"VOL\", cs);") !=
          std::string::npos);
    CHECK(routingSource->find("drawHorizontalVuMeter(g, meterRails[ch], ch, cs);") !=
          std::string::npos);
    CHECK(routingSource->find("drawGainRail(g, gainRails[ch].toFloat(), processor->getChannelGainDb(ch), accent, cs);") !=
          std::string::npos);
    CHECK(routingSource->find("if (processor->isVerticalLayout())\n                drawFaderFill(g, area, processor->getChannelGainDb(ch), cs);") !=
          std::string::npos);
    CHECK(routingSource->find("ColourGradient gainFill(accent.withAlpha(0.58f)") != std::string::npos);
    CHECK(routingSource->find("auto thumb = Rectangle<float>(thumbX - 3.0f, track.getCentreY() - 6.5f, 6.0f, 13.0f);") !=
          std::string::npos);
    CHECK(routingSource->find("static void paintMixerMasterDeck(Graphics& g, Rectangle<float> bounds, Colour accent)") !=
          std::string::npos);
    CHECK(routingSource->find("paintMixerMasterDeck(g, masterDeck.toFloat(), accent);") != std::string::npos);
    CHECK(routingSource->find("paintRoutingBadge(g, masterBadgeArea.toFloat(), \"M\", accent, true);") !=
          std::string::npos);
    CHECK(routingSource->find("drawMasterFader(g, cs);") != std::string::npos);
    CHECK(routingHeader->find("float getMasterGainDb() const") != std::string::npos);
    CHECK(routingHeader->find("void setMasterMute(bool m)") != std::string::npos);
    CHECK(routingHeader->find("SmoothedValue<float, ValueSmoothingTypes::Multiplicative> smoothedMasterGain_;") !=
          std::string::npos);
    CHECK(routingSource->find("xml.setAttribute(\"version\", 5);") != std::string::npos);
    CHECK(routingSource->find("xml.setAttribute(\"verticalLayout\", isVerticalLayout());") !=
          std::string::npos);
    CHECK(routingSource->find("setVerticalLayout(xmlState->getBoolAttribute(\"verticalLayout\", false));") !=
          std::string::npos);
    const auto mixerControlStart = routingSource->find("Component* MixerProcessor::getControls()");
    const auto mixerProcessStart = routingSource->find("void MixerProcessor::processBlock");
    REQUIRE(mixerControlStart != std::string::npos);
    REQUIRE(mixerProcessStart != std::string::npos);
    const auto visibleMixerSource = routingSource->substr(mixerControlStart, mixerProcessStart - mixerControlStart);
    CHECK(visibleMixerSource.find("paintRoutingNodeShell(g, getLocalBounds().toFloat(), getRoutingNodeAccent(), {});") ==
          std::string::npos);
    CHECK(routingSource->find("f.setTextBoxStyle(Slider::NoTextBox, false, 0, 0);") != std::string::npos);
    CHECK(routingSource->find("f.setAlpha(0.01f);") != std::string::npos);
    CHECK(routingSource->find("p.setAlpha(0.01f);") != std::string::npos);
    CHECK(routingSource->find("std::array<Rectangle<int>, MixerProcessor::MaxStrips> valueAreas;") !=
          std::string::npos);
    CHECK(routingSource->find("std::array<Rectangle<int>, MixerProcessor::MaxStrips> gainRails;") !=
          std::string::npos);
    CHECK(routingSource->find("std::array<Rectangle<int>, MixerProcessor::MaxStrips> meterRails;") !=
          std::string::npos);
    CHECK(routingSource->find("std::array<Rectangle<int>, MixerProcessor::MaxStrips> panLabelAreas;") !=
          std::string::npos);
    CHECK(routingSource->find("std::array<Rectangle<int>, MixerProcessor::MaxStrips> gainLabelAreas;") !=
          std::string::npos);
    CHECK(routingSource->find("void layoutHorizontalStrips(Rectangle<int> area)") != std::string::npos);
    CHECK(routingSource->find("void layoutVerticalStrips(Rectangle<int> area)") != std::string::npos);
    CHECK(routingSource->find("if (processor->isVerticalLayout())\n                layoutVerticalStrips(area);\n            else\n                layoutHorizontalStrips(area);") !=
          std::string::npos);

    CHECK(routingHeader->find("Point<int> getSize() override;") != std::string::npos);
    CHECK(routingHeader->find("static constexpr int MaxStrips = 32;") != std::string::npos);
    CHECK(routingHeader->find("static constexpr int NumChannels = 2;") == std::string::npos);
    CHECK(routingSource->find("Point<int> SplitterProcessor::getSize()") != std::string::npos);
    CHECK(routingSource->find("Point<int> MixerProcessor::getSize()") != std::string::npos);
    CHECK(routingSource->find("return Point<int>(kMixerHorizontalNodeWidth, 34 + getNumStrips() * kMixerHorizontalStripRowHeight + kMixerHorizontalMasterRowHeight);") !=
          std::string::npos);
    CHECK(routingSource->find("const int stripW = jmax(50, area.getWidth() / (activeStrips + 1));") !=
          std::string::npos);
    CHECK(routingSource->find("auto strip = area.removeFromLeft(stripW).reduced(4, 0);") !=
          std::string::npos);
    CHECK(routingSource->find("auto row = area.removeFromTop(kMixerHorizontalStripRowHeight);") !=
          std::string::npos);
    CHECK(routingSource->find("meterRails[ch] = strip.removeFromTop(12).reduced(34, 3);") !=
          std::string::npos);
    CHECK(routingSource->find("gainRails[ch] = gainRow.reduced(4, 5);") !=
          std::string::npos);
    CHECK(routingSource->find("vuAreas[ch] = strip.removeFromTop(56).withSizeKeepingCentre(18, 56);") !=
          std::string::npos);
    CHECK(routingSource->find("return Point<int>(jmax(kMixerVerticalNodeMinWidth, 70 + (getNumStrips() + 1) * kMixerVerticalStripWidth),\n                      kMixerVerticalNodeHeight);") !=
          std::string::npos);
    CHECK(routingSource->find("int pinY = 57;") != std::string::npos);
    CHECK(routingSource->find("pinY += 22;") != std::string::npos);
    CHECK(routingSource->find("const int rowTop = firstRowTop + i * kMixerHorizontalStripRowHeight;") !=
          std::string::npos);
    CHECK(routingSource->find("layout.pinY.push_back(stripTop + 35);\n            layout.pinY.push_back(stripTop + 61);") ==
          std::string::npos);
}

TEST_CASE("Dynamic PedalboardProcessor pins resync wrapper channels before rebuilding pin components",
          "[ui][regression][visual][source][nodes]")
{
    const auto pluginSource = loadSourceFile("src/PluginComponent.cpp");
    REQUIRE(pluginSource.has_value());

    const auto refreshStart = pluginSource->find("void PluginComponent::refreshPins()");
    REQUIRE(refreshStart != std::string::npos);
    const auto refreshEnd = pluginSource->find("void PluginComponent::createPins()", refreshStart);
    REQUIRE(refreshEnd != std::string::npos);
    const auto refreshBody = pluginSource->substr(refreshStart, refreshEnd - refreshStart);

    const auto resyncCall = refreshBody.find("bypassable->resyncChannelCount();");
    const auto determineCall = refreshBody.find("determineSize();");
    const auto createPinsCall = refreshBody.find("createPins();");
    REQUIRE(resyncCall != std::string::npos);
    REQUIRE(determineCall != std::string::npos);
    REQUIRE(createPinsCall != std::string::npos);

    CHECK(resyncCall < determineCall);
    CHECK(resyncCall < createPinsCall);
}

TEST_CASE("MIDI source nodes keep source-side labels and bottom keyboard hint",
          "[ui][regression][visual][source][nodes][midi]")
{
    const auto pluginSource = loadSourceFile("src/PluginComponent.cpp");
    const auto virtualMidiHeader = loadSourceFile("src/VirtualMidiInputProcessor.h");
    const auto virtualMidiSource = loadSourceFile("src/VirtualMidiInputProcessor.cpp");

    REQUIRE(pluginSource.has_value());
    REQUIRE(virtualMidiHeader.has_value());
    REQUIRE(virtualMidiSource.has_value());

    CHECK(pluginSource->find("shouldCreateHostMidiOrParamPin(plugin, pluginName, numIn, numOut) && pluginName != \"MIDI Input\"") !=
          std::string::npos);
    CHECK(pluginSource->find("producesMidiSafe(plugin) || pluginName == \"MIDI Input\" || (plugin->getName() == \"OSC Input\")") !=
          std::string::npos);
    CHECK(pluginSource->find("const bool midiInputSourceNode = pluginName == \"MIDI Input\";") !=
          std::string::npos);
    CHECK(pluginSource->find("titleLabel->setJustificationType(midiInputSourceNode ? Justification::centredRight") !=
          std::string::npos);
    CHECK(pluginSource->find("positionOutputTextForCurrentWidth();") != std::string::npos);
    CHECK(virtualMidiHeader->find("Point<int> getSize() override { return Point<int>(100, 60); }") !=
          std::string::npos);
    CHECK(virtualMidiSource->find("class VirtualMidiInputControl final : public Component") !=
          std::string::npos);
    CHECK(virtualMidiSource->find("labelArea = labelArea.removeFromBottom") != std::string::npos);
    CHECK(virtualMidiSource->find("g.drawText(\"Virtual Keyboard\", labelArea, Justification::centredRight, true);") !=
          std::string::npos);
    CHECK(virtualMidiSource->find("Colours::white") == std::string::npos);
}

TEST_CASE("Effect Rack nested graph polish source contract keeps semantic graph chrome",
          "[ui][regression][visual][source][subgraph]")
{
    const auto pluginSource = loadSourceFile("src/PluginComponent.cpp");
    const auto filterGraphSource = loadSourceFile("src/FilterGraph.cpp");
    const auto mainFieldSource = loadSourceFile("src/PluginField.cpp");
    const auto subGraphSource = loadSourceFile("src/SubGraphEditorComponent.cpp");
    const auto subGraphProcessorSource = loadSourceFile("src/SubGraphProcessor.cpp");

    REQUIRE(pluginSource.has_value());
    REQUIRE(filterGraphSource.has_value());
    REQUIRE(mainFieldSource.has_value());
    REQUIRE(subGraphSource.has_value());
    REQUIRE(subGraphProcessorSource.has_value());

    CHECK(pluginSource->find("bool containsTokenWord(const String& text, StringRef token)") != std::string::npos);
    CHECK(pluginSource->find("!CharacterFunctions::isLetterOrDigit(beforeChar)") != std::string::npos);
    CHECK(pluginSource->find("!CharacterFunctions::isLetterOrDigit(afterChar)") != std::string::npos);

    const auto rackTokenCheck = pluginSource->find(
        "if (containsAnyToken(text, {\"effect rack\", \"subgraph\"}) || containsTokenWord(text, \"rack\"))");
    REQUIRE(rackTokenCheck != std::string::npos);
    CHECK(pluginSource->find("return {\"rack\", graphCategoryColour(\"Graph Category Modulation\")};",
                             rackTokenCheck) != std::string::npos);

    const auto instrumentFallback = pluginSource->find("if (desc.isInstrument)");
    const auto moduleFallback = pluginSource->find("return {\"module\", graphCategoryColour(\"Graph Category Delay\")};");
    REQUIRE(instrumentFallback != std::string::npos);
    REQUIRE(moduleFallback != std::string::npos);
    CHECK(rackTokenCheck < instrumentFallback);
    CHECK(rackTokenCheck < moduleFallback);
    CHECK(pluginSource->find("const bool rackNode = visualCategoryName == \"rack\";") != std::string::npos);
    CHECK(pluginSource->find("rackNode ? 0.145f") != std::string::npos);
    CHECK(pluginSource->find("rackNode ? colours[\"Graph Category Modulation\"] : visualAccentColour") !=
          std::string::npos);
    CHECK(pluginSource->find("Point<int> getEmbeddedNodeControlSize(PedalboardProcessor* proc, const String& pluginName)") !=
          std::string::npos);
    CHECK(pluginSource->find("bool isHeroChassisNodeName(const String& pluginName)") != std::string::npos);
    CHECK(pluginSource->find("int getEmbeddedNodeControlTopOffset(const String& pluginName)") != std::string::npos);
    CHECK(pluginSource->find("void drawHeroChassisNodeChrome(Graphics& g, Rectangle<float> bounds, const String& pluginName, AudioProcessor* processor, bool highlighted, bool bypassed)") !=
          std::string::npos);
    CHECK(pluginSource->find("if (pluginName == \"NAM Loader\")") != std::string::npos);
    CHECK(pluginSource->find("if (pluginName == \"IR Loader\")") != std::string::npos);
    CHECK(pluginSource->find("return {520, 704};") == std::string::npos);
    CHECK(pluginSource->find("return {480, 558};") == std::string::npos);
    CHECK(pluginSource->find("compSize = getEmbeddedNodeControlSize(proc, pluginName);") != std::string::npos);
    CHECK(pluginSource->find("void drawEffectRackSubgraphPreview(Graphics& g, Rectangle<float> rackPreview, Colour accentColour)") !=
          std::string::npos);
    CHECK(pluginSource->find("drawEffectRackSubgraphPreview(g, getEffectRackSubgraphPreviewBounds(), accentColour);") !=
          std::string::npos);
    CHECK(pluginSource->find("getEffectRackSubgraphPreviewBounds().contains(localEvent.position.toFloat())") !=
          std::string::npos);
    CHECK(pluginSource->find("visualCategoryName == \"rack\" && e.getNumberOfClicks() >= 2") !=
          std::string::npos);
    CHECK(pluginSource->find("openPluginEditor(false); // Open rack editor from preview double-click") !=
          std::string::npos);
    CHECK(pluginSource->find("const int openButtonRight = bypassButton != nullptr ? bypassButton->getX() - buttonGap") !=
          std::string::npos);
    CHECK(pluginSource->find("countSubGraphContentProcessors") == std::string::npos);
    CHECK(pluginSource->find("#include \"SubGraphProcessor.h\"") != std::string::npos);
    CHECK(pluginSource->find("constexpr const char* kRackNodeWidthProperty = \"nodeWidth\";") !=
          std::string::npos);
    CHECK(pluginSource->find("constexpr const char* kRackNodeHeightProperty = \"nodeHeight\";") !=
          std::string::npos);
    CHECK(pluginSource->find("Point<int> getDefaultRackNodeSize()") != std::string::npos);
    CHECK(pluginSource->find("return {324, 212};") != std::string::npos);
    CHECK(pluginSource->find("Rectangle<float> PluginComponent::getEffectRackSubgraphPreviewBounds() const") !=
          std::string::npos);
    CHECK(pluginSource->find("void drawEffectRackShell(Graphics& g, Rectangle<float> bounds, Colour accentColour, bool highlighted, bool bypassed)") ==
          std::string::npos);
    CHECK(pluginSource->find("drawEffectRackShell(g, getLocalBounds().toFloat(), accentColour, highlighted, bypassed);") ==
          std::string::npos);
    CHECK(pluginSource->find("g.drawText(\"RACK\"") == std::string::npos);
    CHECK(pluginSource->find("const int storedW = storedWRaw > 0 ? storedWRaw : rackDefault.getX();") !=
          std::string::npos);
    CHECK(pluginSource->find("const int storedH = storedHRaw > 0 ? storedHRaw : rackDefault.getY();") !=
          std::string::npos);
    CHECK(pluginSource->find("std::make_unique<ResizableBorderComponent>(this, &rackBoundsConstrainer)") !=
          std::string::npos);
    CHECK(pluginSource->find("rackBoundsConstrainer.setMinimumSize(248, 152);") != std::string::npos);
    CHECK(pluginSource->find("node->properties.set(kRackNodeWidthProperty, getWidth());") !=
          std::string::npos);
    CHECK(pluginSource->find("node->properties.set(kRackNodeHeightProperty, getHeight());") !=
          std::string::npos);
    CHECK(pluginSource->find("drawEffectRackPortSummary") == std::string::npos);
    CHECK(pluginSource->find("int countEffectRackNestedProcessors(AudioProcessor* processor)") != std::string::npos);
    CHECK(pluginSource->find("id == rack->getRackAudioInputNodeId()") != std::string::npos);
    CHECK(pluginSource->find("id == rack->getRackAudioOutputNodeId()") != std::string::npos);
    CHECK(pluginSource->find("id == rack->getRackMidiInputNodeId()") != std::string::npos);
    CHECK(pluginSource->find("String getRackBoundaryDisplayName(AudioProcessorGraph::Node* node, const String& fallback)") !=
          std::string::npos);
    CHECK(pluginSource->find("bool isRackAudioBoundaryNode(AudioProcessorGraph::Node* node)") !=
          std::string::npos);
    CHECK(pluginSource->find("bool isRackBoundaryNode(AudioProcessorGraph::Node* node)") !=
          std::string::npos);
    CHECK(pluginSource->find("void PluginComponent::layoutTitleLabel()") != std::string::npos);
    CHECK(pluginSource->find("const int titleLeft = isRackBoundaryNode(node) ? 12 : (isAudioIONode() ? 22 : 20);") !=
          std::string::npos);
    CHECK(pluginSource->find("if (isRackBoundaryNode(node))\n            w = jmax(w, 140);") !=
          std::string::npos);
    CHECK(pluginSource->find("displayName = getRackBoundaryDisplayName(node, pluginName);") !=
          std::string::npos);
    CHECK(pluginSource->find("return !isRackAudioBoundaryNode(node) && ((pluginName == \"Audio Input\") || (pluginName == \"Audio Output\"));") !=
          std::string::npos);
    CHECK(pluginSource->find("titleLabel = new Label(\"titleLabe\", displayName);") !=
          std::string::npos);
    CHECK(subGraphProcessorSource->find("node->properties.set(\"rackPortRole\", \"audio-in\");") !=
          std::string::npos);
    CHECK(subGraphProcessorSource->find("node->properties.set(\"rackPortRole\", \"audio-out\");") !=
          std::string::npos);
    CHECK(subGraphProcessorSource->find("node->properties.set(\"rackPortRole\", \"midi-in\");") !=
          std::string::npos);
    CHECK(pluginSource->find("g.drawText(\"EMPTY RACK\"") == std::string::npos);
    CHECK(pluginSource->find("g.drawText(\"SUB-GRAPH\"") != std::string::npos);
    CHECK(pluginSource->find("g.fillRoundedRectangle(rackPreview, 9.0f);") != std::string::npos);
    CHECK(pluginSource->find("const float gridStep = 13.0f;") != std::string::npos);
    CHECK(pluginSource->find("const float dotSize = 1.1f;") != std::string::npos);
    CHECK(pluginSource->find(
              "g.fillEllipse(xDot - dotSize * 0.5f, yDot - dotSize * 0.5f, dotSize, dotSize);") !=
          std::string::npos);
    CHECK(pluginSource->find("constexpr int visibleProcessors = 3;") != std::string::npos);
    CHECK(pluginSource->find("const float nodeW = jmin(32.0f, jmax(24.0f, (graphContent.getWidth() - 72.0f) / 3.0f));") !=
          std::string::npos);
    CHECK(pluginSource->find("const float nodeH = 28.0f;") != std::string::npos);
    CHECK(pluginSource->find("auto nodeRect = Rectangle<float>(nodeX, laneY - nodeH * 0.5f, nodeW, nodeH);") !=
          std::string::npos);
    CHECK(pluginSource->find("constexpr int visibleProcessors = 4;") == std::string::npos);
    CHECK(pluginSource->find("((i % 2 == 0) ? -4.0f : 4.0f)") == std::string::npos);
    CHECK(pluginSource->find("const float gridStep = 10.0f;") == std::string::npos);
    CHECK(pluginSource->find("g.fillRect(xLine, yDot, 1.0f, 1.0f);") == std::string::npos);
    CHECK(pluginSource->find(
              "g.fillRoundedRectangle(rackPreview.reduced(2.0f).translated(0.0f, 1.0f), 6.0f);") ==
          std::string::npos);
    CHECK(pluginSource->find("g.drawText(\"IN\",") == std::string::npos);
    CHECK(pluginSource->find("g.drawText(\"OUT\",") == std::string::npos);
    CHECK(pluginSource->find("\" processors nested\"") != std::string::npos);
    CHECK(pluginSource->find("g.drawText(\"Open\"") == std::string::npos);
    CHECK(pluginSource->find("void drawEffectRackFooterSummary(Graphics& g, Rectangle<float> bounds, AudioProcessor* processor, Colour accentColour)") !=
          std::string::npos);
    CHECK(pluginSource->find("drawEffectRackFooterSummary(g, rackFooterSummary") != std::string::npos);
    CHECK(pluginSource->find("return {12.0f, headerHeight + 10.0f, w - 24.0f, jmax(86.0f, h - headerHeight - 66.0f)};") !=
          std::string::npos);
    CHECK(pluginSource->find("editButton->setButtonText(\"Open\");") != std::string::npos);
    CHECK(pluginSource->find("mappingsButton->setButtonText(\"Map\");") != std::string::npos);
    CHECK(pluginSource->find("nodeFill.addColour(0.62") != std::string::npos);
    CHECK(pluginSource->find("jackBounds") == std::string::npos);
    CHECK(pluginSource->find("screwColour") == std::string::npos);
    CHECK(pluginSource->find("outer.getX() + 4.0f, header.getBottom() + 7.0f") == std::string::npos);
    CHECK(pluginSource->find("const float chassisBorderWidth = highlighted ? 1.6f : 1.1f;") !=
          std::string::npos);
    CHECK(pluginSource->find("const float nodeBorderWidth = highlighted ? 0.82f : 0.58f;") !=
          std::string::npos);
    CHECK(pluginSource->find(
              "g.drawRoundedRectangle(2.0f, 2.0f, w - 4.0f, h - 4.0f, cornerRadius, nodeBorderWidth);") !=
          std::string::npos);
    CHECK(pluginSource->find("g.fillRoundedRectangle(3.0f, 7.0f, 2.0f, h - 14.0f, 1.0f);") ==
          std::string::npos);
    CHECK(pluginSource->find("titleLabel->setVisible(shouldShowHostTitleLabel(pluginName));") !=
          std::string::npos);

    CHECK(mainFieldSource->find("const auto gridStyle = getGraphGridStyle();") != std::string::npos);
    CHECK(mainFieldSource->find("const auto canvasAccent = colours[\"Accent Colour\"];") !=
          std::string::npos);
    CHECK(mainFieldSource->find("ColourGradient radialTint(canvasAccent.withAlpha(0.024f)") !=
          std::string::npos);
    CHECK(mainFieldSource->find("const float gridSize = 24.0f;") != std::string::npos);
    CHECK(mainFieldSource->find("const float majorGridSize = gridSize * 4.0f;") == std::string::npos);
    CHECK(mainFieldSource->find("const auto gridAccent = canvasAccent.interpolatedWith(colours[\"Text Colour\"], 0.08f);") !=
          std::string::npos);
    CHECK(mainFieldSource->find("gridAccent.withAlpha(gridStyle == \"dots\" ? 0.060f : 0.030f)") !=
          std::string::npos);
    CHECK(mainFieldSource->find("Colour majorGridCol") == std::string::npos);
    CHECK(mainFieldSource->find("const auto dotSize = 1.15f;") != std::string::npos);
    CHECK(mainFieldSource->find("firstMajorX") == std::string::npos);

    CHECK(subGraphSource->find("String getSubGraphGridStyle()") != std::string::npos);
    CHECK(subGraphSource->find("getString(kSubGraphGridStyleSettingsKey, \"Lines\")") != std::string::npos);
    CHECK(subGraphSource->find("style != \"dots\" && style != \"lines\" && style != \"off\"") !=
          std::string::npos);
    CHECK(subGraphSource->find("const auto rackAccent = colours[\"Graph Category Modulation\"];") !=
          std::string::npos);
    CHECK(subGraphSource->find("const auto rackCanvasBase = colours[\"Field Background\"].interpolatedWith(rackAccent, 0.16f);") !=
          std::string::npos);
    CHECK(subGraphSource->find("Colour bgCol = rackCanvasBase.darker(0.08f);") != std::string::npos);
    CHECK(subGraphSource->find("const auto textureColour = rackAccent.withAlpha(0.025f);") != std::string::npos);
    CHECK(subGraphSource->find("setResizable(true, true);") != std::string::npos);
    CHECK(subGraphSource->find("setResizeLimits(520, 360, 1600, 1200);") != std::string::npos);
    CHECK(subGraphSource->find("const auto accent = colours[\"Graph Category Modulation\"];") != std::string::npos);
    CHECK(subGraphSource->find("colour[\"Accent Colour\"]") == std::string::npos);
    CHECK(subGraphSource->find("const auto gridStyle = getSubGraphGridStyle();") != std::string::npos);
    CHECK(subGraphSource->find("const float gridSize = 24.0f;") != std::string::npos);
    CHECK(subGraphSource->find("const auto gridAccent = rackAccent;") != std::string::npos);
    CHECK(subGraphSource->find("gridAccent.withAlpha(gridStyle == \"dots\" ? 0.09f : 0.075f)") !=
          std::string::npos);
    CHECK(subGraphSource->find("const auto dotSize = 1.5f;") != std::string::npos);
    CHECK(subGraphSource->find("const float majorGridSize = gridSize * 4.0f;") == std::string::npos);
    CHECK(subGraphSource->find("if (gridStyle == \"dots\")") != std::string::npos);
    CHECK(subGraphSource->find("if (gridStyle != \"off\")") != std::string::npos);

    CHECK(filterGraphSource->find("e->setAttribute(\"nodeWidth\", (int)node->properties.getWithDefault(\"nodeWidth\", 0));") !=
          std::string::npos);
    CHECK(filterGraphSource->find("e->setAttribute(\"nodeHeight\", (int)node->properties.getWithDefault(\"nodeHeight\", 0));") !=
          std::string::npos);
    CHECK(filterGraphSource->find("node.nodeWidth = e->getIntAttribute(\"nodeWidth\", 0);") !=
          std::string::npos);
    CHECK(filterGraphSource->find("node.nodeHeight = e->getIntAttribute(\"nodeHeight\", 0);") !=
          std::string::npos);
    CHECK(filterGraphSource->find("node->properties.set(\"nodeWidth\", preparedNode.nodeWidth);") !=
          std::string::npos);
    CHECK(filterGraphSource->find("node->properties.set(\"nodeHeight\", preparedNode.nodeHeight);") !=
          std::string::npos);

    CHECK(subGraphSource->find("0xFF00AAAA") == std::string::npos);
    CHECK(subGraphSource->find("0xFF00CCCC") == std::string::npos);
    CHECK(subGraphSource->find("0xFF00DDDD") == std::string::npos);
    CHECK(subGraphSource->find("0xFF1A2A2A") == std::string::npos);

    const auto editorPaintStart = subGraphSource->find("void SubGraphEditorComponent::paint(Graphics& g)");
    REQUIRE(editorPaintStart != std::string::npos);
    const auto editorPaintEnd = subGraphSource->find("void SubGraphEditorComponent::resized()", editorPaintStart);
    REQUIRE(editorPaintEnd != std::string::npos);
    const auto editorPaint = subGraphSource->substr(editorPaintStart, editorPaintEnd - editorPaintStart);
    CHECK(editorPaint.find("colours[\"Window Background\"") != std::string::npos);
    CHECK(editorPaint.find("colours[\"Plugin Border\"") != std::string::npos);
    CHECK(editorPaint.find("ColourGradient") != std::string::npos);
    CHECK(editorPaint.find("fillRoundedRectangle") != std::string::npos);
}

TEST_CASE("NAM and IR loader graph-node source guardrails keep chassis hooks reachable",
          "[ui][regression][source][nodes][loader][design-guardrail]")
{
    const auto pluginSource = loadSourceFile("src/PluginComponent.cpp");
    const auto namSource = loadSourceFile("src/NAMControl.cpp");
    const auto irSource = loadSourceFile("src/IRLoaderControl.cpp");

    REQUIRE(pluginSource.has_value());
    REQUIRE(namSource.has_value());
    REQUIRE(irSource.has_value());

    // This is not a visual approval test. It only keeps the node-porting work
    // from drifting back to generic or function-stripping implementations.
    CHECK(pluginSource->find("Point<int> getEmbeddedNodeControlSize(PedalboardProcessor* proc, const String& pluginName)") !=
          std::string::npos);
    CHECK(pluginSource->find("int getEmbeddedNodeControlTopOffset(const String& pluginName)") !=
          std::string::npos);
    CHECK(pluginSource->find("int getEmbeddedNodeControlHeightPadding(const String& pluginName)") !=
          std::string::npos);
    CHECK(pluginSource->find("policy.controlHeightPadding = 84;") != std::string::npos);
    CHECK(pluginSource->find("return 116;") == std::string::npos);
    CHECK(pluginSource->find("return {520, 704};") == std::string::npos);
    CHECK(pluginSource->find("return {480, 558};") == std::string::npos);
    CHECK(pluginSource->find("class PluginNodeFooterButtonLookAndFeel final : public LookAndFeel_V4") !=
          std::string::npos);
    CHECK(pluginSource->find("void PluginComponent::layoutFooterButtons()") != std::string::npos);
    CHECK(pluginSource->find("editButton->setButtonText(\"Edit\");") != std::string::npos);
    CHECK(pluginSource->find("mappingsButton->setButtonText(\"Map\");") != std::string::npos);
    CHECK(pluginSource->find("IconManager::getInstance().drawDomainGlyphTile") != std::string::npos);
    CHECK(pluginSource->find("IconManager::DomainGlyph::Amp") != std::string::npos);
    CHECK(pluginSource->find("IconManager::DomainGlyph::Cabinet") != std::string::npos);
    CHECK(namSource->find("Colours::white") == std::string::npos);
    CHECK(namSource->find("ampHostTextBright = cs.colours[\"Text Colour\"];") != std::string::npos);
    CHECK(namSource->find("ampPanelTextBright = ampInsetBg.contrasting(0.92f);") != std::string::npos);
    CHECK(namSource->find("ampLedOff = ampPanelTextBright.withAlpha(0.42f);") != std::string::npos);
    CHECK(namSource->find("g.setColour(laf.ampHostTextDim.withAlpha(0.86f));") != std::string::npos);
    CHECK(namSource->find("g.setColour(laf.ampPanelTextDim.withAlpha(0.72f));") != std::string::npos);
    CHECK(namSource->find("auto area = bounds.reduced(3, 2);") != std::string::npos);
    CHECK(namSource->find("auto modelButtons = captureSection.removeFromTop(27).reduced(3, 2);") !=
          std::string::npos);
    CHECK(namSource->find(".reduced(8, 10)") != std::string::npos);

    CHECK(namSource->find("drawSectionHeader") != std::string::npos);
    CHECK(namSource->find("drawInsetField") != std::string::npos);
    CHECK(namSource->find("drawEmbeddedToneCurve") != std::string::npos);
    CHECK(namSource->find("drawEmbeddedEqCurve") != std::string::npos);
    CHECK(namSource->find("\"Capture\"") != std::string::npos);
    CHECK(namSource->find("\"Cabinet IR\"") != std::string::npos);
    CHECK(namSource->find("auto slotGrid = cabinetSection.removeFromTop") != std::string::npos);
    CHECK(namSource->find("\"Signal Chain\"") == std::string::npos);
    CHECK(namSource->find("auto slotGrid = signal.removeFromTop") == std::string::npos);
    CHECK(namSource->find("loadModelButton->setBounds") != std::string::npos);
    CHECK(namSource->find("browseModelsButton->setBounds") != std::string::npos);
    CHECK(namSource->find("clearModelButton->setBounds") != std::string::npos);
    CHECK(namSource->find("irEnabledButton->setBounds") != std::string::npos);
    CHECK(namSource->find("ir2EnabledButton->setBounds") != std::string::npos);
    CHECK(namSource->find("fxLoopEnabledButton->setBounds") != std::string::npos);
    CHECK(namSource->find("editFxLoopButton->setBounds") != std::string::npos);

    CHECK(irSource->find("drawSectionHeader") != std::string::npos);
    CHECK(irSource->find("drawEmbeddedChassisCard") == std::string::npos);
    CHECK(irSource->find("drawEmbeddedStatusPill") != std::string::npos);
    CHECK(irSource->find("drawEmbeddedIRWaveform") != std::string::npos);
    CHECK(irSource->find("drawEmbeddedXfadeReadout") != std::string::npos);
    CHECK(irSource->find("drawEmbeddedFilterChip") != std::string::npos);
    CHECK(irSource->find("\"Impulse Responses\"") != std::string::npos);
    CHECK(irSource->find("\"Blend\"") != std::string::npos);
    CHECK(irSource->find("\"Filter\"") != std::string::npos);
    CHECK(irSource->find("loadButton->setBounds") != std::string::npos);
    CHECK(irSource->find("browseButton->setBounds") != std::string::npos);
    CHECK(irSource->find("clearButton->setBounds") != std::string::npos);
    CHECK(irSource->find("loadButton2->setBounds") != std::string::npos);
    CHECK(irSource->find("browseButton2->setBounds") != std::string::npos);
    CHECK(irSource->find("clearButton2->setBounds") != std::string::npos);
    CHECK(irSource->find("Colours::white") == std::string::npos);
    CHECK(irSource->find("Colour hostText;") != std::string::npos);
    CHECK(irSource->find("Colour panelText;") != std::string::npos);
    CHECK(irSource->find("const auto hostText = colours[\"Text Colour\"];") != std::string::npos);
    CHECK(irSource->find("const auto panelText = insetColour.contrasting(0.92f);") != std::string::npos);
    CHECK(irSource->find("const auto shellTextDim = embeddedInGraphNode ? palette.hostTextDim : palette.textDim;") !=
          std::string::npos);
    CHECK(irSource->find("g.setColour(palette.hostTextDim.withAlpha(0.80f));") !=
          std::string::npos);
    CHECK(irSource->find("g.setColour(palette.panelTextDim.withAlpha(0.70f));") !=
          std::string::npos);
    CHECK(irSource->find("slider->setColour(Slider::textBoxTextColourId, palette.panelText.withAlpha(0.9f));") !=
          std::string::npos);
    CHECK(irSource->find("const auto textColour = faceColour.contrasting(0.92f);") ==
          std::string::npos);
    CHECK(irSource->find("uint32 text") == std::string::npos);
    CHECK(irSource->find("auto area = bounds.reduced(5, 6);") != std::string::npos);
    CHECK(irSource->find(".reduced(5, 6)") != std::string::npos);

}

TEST_CASE("Internal plugin descriptions are runtime catalog entries, not persisted scan results",
          "[ui][regression][source][plugins]")
{
    const auto mainPanelSource = loadSourceFile("src/MainPanel.cpp");

    REQUIRE(mainPanelSource.has_value());

    CHECK(mainPanelSource->find("bool isRuntimeInternalPluginXml(const XmlElement& pluginXml)") !=
          std::string::npos);
    CHECK(mainPanelSource->find("return pluginXml.hasTagName(\"PLUGIN\") &&\n           pluginXml.getStringAttribute(\"format\") == \"Internal\";") !=
          std::string::npos);
    CHECK(mainPanelSource->find("void removeRuntimeInternalPluginsFromPluginListXml(XmlElement& pluginListXml)") !=
          std::string::npos);
    CHECK(mainPanelSource->find("std::unique_ptr<XmlElement> createPersistablePluginListXml(const KnownPluginList& pluginList)") !=
          std::string::npos);
    CHECK(mainPanelSource->find("removeRuntimeInternalPluginsFromPluginListXml(*savedPluginList);\n        pluginList.recreateFromXml(*savedPluginList);") !=
          std::string::npos);
    CHECK(mainPanelSource->find("auto savedPluginList = createPersistablePluginListXml(pluginList);") !=
          std::string::npos);
    CHECK(mainPanelSource->find("pluginList.createXml();\n    if (savedPluginList != nullptr)\n        removeRuntimeInternalPluginsFromPluginListXml(*savedPluginList);") !=
          std::string::npos);
}

TEST_CASE("NAM and IR loader mockup ports cannot remove existing app-only controls",
          "[ui][regression][source][nodes][loader][function-contract]")
{
    const auto pluginSource = loadSourceFile("src/PluginComponent.cpp");
    const auto namHeader = loadSourceFile("src/NAMControl.h");
    const auto namSource = loadSourceFile("src/NAMControl.cpp");
    const auto irHeader = loadSourceFile("src/IRLoaderControl.h");
    const auto irSource = loadSourceFile("src/IRLoaderControl.cpp");

    REQUIRE(pluginSource.has_value());
    REQUIRE(namHeader.has_value());
    REQUIRE(namSource.has_value());
    REQUIRE(irHeader.has_value());
    REQUIRE(irSource.has_value());

    // Node-host controls are real Pedalboard functions. The mockup may restyle them,
    // but the graph node must not lose edit, mapping, bypass, or delete reachability.
    CHECK(pluginSource->find("editButton = new TextButton") != std::string::npos);
    CHECK(pluginSource->find("mappingsButton = new TextButton") != std::string::npos);
    CHECK(pluginSource->find("bypassButton = new DrawableButton") != std::string::npos);
    CHECK(pluginSource->find("deleteButton = new DrawableButton") != std::string::npos);
    CHECK(pluginSource->find("openMappingsWindow();") != std::string::npos);
    CHECK(pluginSource->find("bypassable->setBypass(bypassButton->getToggleState());") != std::string::npos);

    // NAM graph-node body must preserve controls that exist in the app but are
    // absent or compressed in the mockup: FX loop, gain/gate, PRE/POST, normalize,
    // stack EQ, and four-band parametric EQ.
    for (const auto* token : {
             "std::unique_ptr<TextButton> loadModelButton",
             "std::unique_ptr<TextButton> browseModelsButton",
             "std::unique_ptr<TextButton> clearModelButton",
             "std::unique_ptr<Label> modelNameLabel",
             "std::unique_ptr<Label> modelArchLabel",
             "std::unique_ptr<TextButton> loadIRButton",
             "std::unique_ptr<TextButton> clearIRButton",
             "std::unique_ptr<ToggleButton> irEnabledButton",
             "std::unique_ptr<TextButton> loadIR2Button",
             "std::unique_ptr<TextButton> clearIR2Button",
             "std::unique_ptr<ToggleButton> ir2EnabledButton",
             "std::unique_ptr<Slider> irBlendSlider",
             "std::unique_ptr<Slider> irLowCutSlider",
             "std::unique_ptr<Slider> irHighCutSlider",
             "std::unique_ptr<ToggleButton> fxLoopEnabledButton",
             "std::unique_ptr<TextButton> editFxLoopButton",
             "std::unique_ptr<Slider> inputGainSlider",
             "std::unique_ptr<Slider> outputGainSlider",
             "std::unique_ptr<Slider> noiseGateSlider",
             "std::unique_ptr<ToggleButton> toneStackEnabledButton",
             "std::unique_ptr<TextButton> toneStackPreButton",
             "std::unique_ptr<TextButton> toneEqModeStackButton",
             "std::unique_ptr<TextButton> toneEqModeParamButton",
             "std::unique_ptr<Slider> bassSlider",
             "std::unique_ptr<Slider> midSlider",
             "std::unique_ptr<Slider> trebleSlider",
             "std::array<std::unique_ptr<Slider>, NAMProcessor::kParamEqBandCount> paramEqFrequencySliders",
             "std::array<std::unique_ptr<Slider>, NAMProcessor::kParamEqBandCount> paramEqGainSliders",
             "std::array<std::unique_ptr<Slider>, NAMProcessor::kParamEqBandCount> paramEqQSliders",
             "std::unique_ptr<ToggleButton> normalizeButton"})
    {
        INFO("Missing NAM header control token: " << token);
        CHECK(namHeader->find(token) != std::string::npos);
    }

    for (const auto* token : {
             "loadModelButton->setBounds",
             "browseModelsButton->setBounds",
             "clearModelButton->setBounds",
             "modelNameLabel->setBounds",
             "loadIRButton->setBounds",
             "clearIRButton->setBounds",
             "irEnabledButton->setBounds",
             "loadIR2Button->setBounds",
             "clearIR2Button->setBounds",
             "ir2EnabledButton->setBounds",
             "irBlendSlider->setBounds",
             "irLowCutSlider->setBounds",
             "irHighCutSlider->setBounds",
             "fxLoopEnabledButton->setBounds",
             "editFxLoopButton->setBounds",
             "inputGainSlider->setBounds",
             "outputGainSlider->setBounds",
             "noiseGateSlider->setBounds",
             "toneStackEnabledButton->setBounds",
             "toneStackPreButton->setBounds",
             "toneEqModeStackButton->setBounds",
             "toneEqModeParamButton->setBounds",
             "normalizeButton->setBounds",
             "paramEqFrequencySliders[band]->setBounds",
             "paramEqGainSliders[band]->setBounds",
             "paramEqQSliders[band]->setBounds",
             "bassSlider->setBounds",
             "midSlider->setBounds",
             "trebleSlider->setBounds"})
    {
        INFO("Missing NAM embedded layout token: " << token);
        CHECK(namSource->find(token) != std::string::npos);
    }

    for (const auto* token : {
             "NAMModelBrowser::showWindow",
             "namProcessor->clearModel();",
             "namProcessor->clearIR();",
             "namProcessor->clearIR2();",
             "namProcessor->setIREnabled",
             "namProcessor->setIR2Enabled",
             "namProcessor->setEffectsLoopEnabled",
             "new FXLoopWindow",
             "namProcessor->setToneStackEnabled",
             "namProcessor->setToneStackPre",
             "namProcessor->setToneEqMode(NAMProcessor::ToneEqMode::Stack)",
             "namProcessor->setToneEqMode(NAMProcessor::ToneEqMode::Parametric)",
             "namProcessor->setNormalizeOutput",
             "namProcessor->setInputGain",
             "namProcessor->setOutputGain",
             "namProcessor->setNoiseGateThreshold",
             "namProcessor->setBass",
             "namProcessor->setMid",
             "namProcessor->setTreble",
             "namProcessor->setIRLowCut",
             "namProcessor->setIRHighCut",
             "namProcessor->setIRBlend",
             "namProcessor->setParamEqBandFrequency",
             "namProcessor->setParamEqBandGain",
             "namProcessor->setParamEqBandQ"})
    {
        INFO("Missing NAM behavior token: " << token);
        CHECK(namSource->find(token) != std::string::npos);
    }

    // Standalone IR Loader node must keep both disk loading and browser loading
    // for both slots, plus all existing blend/mix/filter controls.
    for (const auto* token : {
             "std::unique_ptr<TextButton> loadButton",
             "std::unique_ptr<TextButton> browseButton",
             "std::unique_ptr<TextButton> clearButton",
             "std::unique_ptr<Label> irNameLabel",
             "std::unique_ptr<TextButton> loadButton2",
             "std::unique_ptr<TextButton> browseButton2",
             "std::unique_ptr<TextButton> clearButton2",
             "std::unique_ptr<Label> irName2Label",
             "std::unique_ptr<Slider> blendSlider",
             "std::unique_ptr<Slider> mixSlider",
             "std::unique_ptr<Slider> lowCutSlider",
             "std::unique_ptr<Slider> highCutSlider"})
    {
        INFO("Missing IR header control token: " << token);
        CHECK(irHeader->find(token) != std::string::npos);
    }

    for (const auto* token : {
             "loadButton->setBounds",
             "browseButton->setBounds",
             "clearButton->setBounds",
             "irNameLabel->setBounds",
             "loadButton2->setBounds",
             "browseButton2->setBounds",
             "clearButton2->setBounds",
             "irName2Label->setBounds",
             "blendSlider->setBounds",
             "mixSlider->setBounds",
             "lowCutSlider->setBounds",
             "highCutSlider->setBounds",
             "IRBrowser::showWindow",
             "irProcessor->loadIRFile(File());",
             "irProcessor->clearIR2();",
             "irProcessor->setMix",
             "irProcessor->setLowCut",
             "irProcessor->setHighCut",
             "irProcessor->setBlend"})
    {
        INFO("Missing IR embedded layout/behavior token: " << token);
        CHECK(irSource->find(token) != std::string::npos);
    }
}

TEST_CASE("Scratch footer source contract keeps panel affordance reachable",
          "[ui][regression][visual][source][scratch]")
{
    const auto mainPanelSource = loadSourceFile("src/MainPanel.cpp");
    REQUIRE(mainPanelSource.has_value());

    CHECK(mainPanelSource->find("const int scratchRecordW = 52;") != std::string::npos);
    CHECK(mainPanelSource->find("const int scratchPanelW = 76;") != std::string::npos);
    CHECK(mainPanelSource->find("const int scratchMediumW = scratchRecordW + gap + scratchPanelW;") !=
          std::string::npos);
    CHECK(mainPanelSource->find("const int scratchFullW = scratchMediumW + gap + scratchMinStatusW;") !=
          std::string::npos);
    CHECK(mainPanelSource->find("layoutScratchControls(8, footerY, scratchFullW);") != std::string::npos);
    CHECK(mainPanelSource->find("layoutScratchControls(8, row3Y, scratchW);") != std::string::npos);
    CHECK(mainPanelSource->find("footerLayoutW >= 780 ? scratchFullW : scratchMediumW") != std::string::npos);
    CHECK(mainPanelSource->find("layoutScratchControls(8, footerY, 112)") == std::string::npos);
    CHECK(mainPanelSource->find("layoutScratchControls(8, row2Y, 112)") == std::string::npos);
    CHECK(mainPanelSource->find("layoutScratchControls(8, row3Y, 180)") == std::string::npos);
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
    const auto mainPanelSource = loadSourceFile("src/MainPanel.cpp");
    REQUIRE(source.has_value());
    REQUIRE(mainPanelSource.has_value());

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

    CHECK(source->find("$mockupNodeSnapshotNames = @(\"nam-loader\", \"ir-loader\", \"effect-rack\", \"tuner\", \"mixer\", \"splitter\", \"notes\")") !=
          std::string::npos);
    CHECK(source->find("$appNodeSnapshotNames = $mockupNodeSnapshotNames + @(\"oscilloscope\", \"tone-generator\")") !=
          std::string::npos);
    CHECK(source->find("foreach ($nodeName in $appNodeSnapshotNames)") != std::string::npos);
    CHECK(source->find("& node $mockupCaptureScript --out $outputDir --nodes ($mockupNodeSnapshotNames -join \",\") --browser msedge") !=
          std::string::npos);
    CHECK(source->find("foreach ($nodeName in $mockupNodeSnapshotNames)") != std::string::npos);
    CHECK(mainPanelSource->find("return \"app-node-oscilloscope.png\";") != std::string::npos);
    CHECK(mainPanelSource->find("return \"app-node-tone-generator.png\";") != std::string::npos);
}

TEST_CASE("NAM and IR library polish source contract covers favorites and IR folder setting",
          "[ui][regression][visual][source][library]")
{
    const auto browserHeader = loadSourceFile("src/NAMModelBrowser.h");
    const auto browserSource = loadSourceFile("src/NAMModelBrowser.cpp");
    const auto preferencesHeader = loadSourceFile("src/PreferencesDialog.h");
    const auto preferencesSource = loadSourceFile("src/PreferencesDialog.cpp");

    REQUIRE(browserHeader.has_value());
    REQUIRE(browserSource.has_value());
    REQUIRE(preferencesHeader.has_value());
    REQUIRE(preferencesSource.has_value());

    CHECK(browserSource->find("kNamFavoritesSettingsKey = \"NAMModelFavorites\"") != std::string::npos);
    CHECK(browserSource->find("kIrFavoritesSettingsKey = \"IRFavorites\"") != std::string::npos);
    CHECK(browserSource->find("kIrLibraryDirectorySettingsKey = \"IRLibraryDirectory\"") != std::string::npos);
    CHECK(browserSource->find("getStringArray(kNamFavoritesSettingsKey)") != std::string::npos);
    CHECK(browserSource->find("setStringArray(kNamFavoritesSettingsKey, favouriteModelPaths)") != std::string::npos);
    CHECK(browserSource->find("getStringArray(kIrFavoritesSettingsKey)") != std::string::npos);
    CHECK(browserSource->find("setStringArray(kIrFavoritesSettingsKey, favouriteIRPaths)") != std::string::npos);

    CHECK(browserHeader->find("StringArray favouriteModelPaths;") != std::string::npos);
    CHECK(browserHeader->find("StringArray favouriteIRPaths;") != std::string::npos);
    CHECK(browserHeader->find("std::unique_ptr<TextButton> favoriteButton;") != std::string::npos);
    CHECK(browserHeader->find("std::unique_ptr<TextButton> irFavoriteButton;") != std::string::npos);
    CHECK(browserHeader->find("bool isFavouriteModel(const NAMModelInfo& model) const;") != std::string::npos);
    CHECK(browserHeader->find("bool isFavouriteIR(const IRFileInfo& ir) const;") != std::string::npos);
    CHECK(browserHeader->find("bool syncIRDirectoryFromSettingsIfAllowed();") != std::string::npos);
    CHECK(browserHeader->find("bool irDirectoryManuallySelected = false;") != std::string::npos);

    CHECK(browserSource->find("setMinimumHorizontalScale(0.72f)") != std::string::npos);
    CHECK(browserSource->find("titleLabel->setJustificationType(Justification::centred);") != std::string::npos);
    CHECK(browserSource->find("const auto separatorBounds = detailsPanelBounds.toFloat().reduced(18.0f, 0.0f);") !=
          std::string::npos);
    CHECK(browserSource->find("bounds.removeFromTop(compactLayout ? 14 : 20);") != std::string::npos);
    CHECK(browserSource->find("bool canShowThreePillarLibraryLayout(int availableWidth, bool compactLayout)") !=
          std::string::npos);
    CHECK(browserSource->find("const bool showRail = canShowThreePillarLibraryLayout(bounds.getWidth(), compactLayout);") !=
          std::string::npos);
    CHECK(browserSource->find("const int railWidth = getLibraryRailWidth(bounds.getWidth(), compactLayout);") !=
          std::string::npos);
    CHECK(browserSource->find("Rectangle<int> trimListBoxBoundsToFullRows(Rectangle<int> area, int rowHeight)") !=
          std::string::npos);
    CHECK(browserSource->find("modelList->setBounds(trimListBoxBoundsToFullRows(listArea, modelList->getRowHeight()));") !=
          std::string::npos);
    CHECK(browserSource->find("irList->setBounds(trimListBoxBoundsToFullRows(listArea, irList->getRowHeight()));") !=
          std::string::npos);
    CHECK(browserSource->find("const bool showRail = !compactLayout && bounds.getWidth() >= 720;") ==
          std::string::npos);
    CHECK(browserSource->find("jlimit(210, 270, roundToInt(bounds.getWidth() * 0.48f))") != std::string::npos);
    CHECK(browserSource->find("const bool showFullTechnicalDetails = true;") != std::string::npos);
    CHECK(browserSource->find("addAndMakeVisible(labelPtr.get());") != std::string::npos);
    CHECK(browserSource->find("closeButton->setLookAndFeel(nullptr);") != std::string::npos);
    CHECK(browserSource->find("titleLabel->setJustificationType(Justification::centred);") !=
          std::string::npos);
    CHECK(browserSource->find("closeButton->setColour(TextButton::buttonColourId, Colours::transparentBlack);") !=
          std::string::npos);
    CHECK(browserSource->find("closeButton->setColour(TextButton::buttonOnColourId, Colours::transparentBlack);") !=
          std::string::npos);
    CHECK(browserSource->find("closeButton->setColour(TextButton::buttonColourId, palette.inset.withAlpha(0.72f));") ==
          std::string::npos);
    CHECK(browserSource->find("const int headerGap = compactLayout ? 10 : 14;") != std::string::npos);
    CHECK(browserSource->find("const int searchHeight = compactLayout ? 32 : 34;") != std::string::npos);
    CHECK(browserSource->find("const auto titleRowOriginal = bounds.removeFromTop(compactLayout ? 28 : 34);") !=
          std::string::npos);
    CHECK(browserSource->find("const int tabStripX = titleRowOriginal.getX() + jmax(0, (titleRowOriginal.getWidth() - tabStripWidth) / 2);") !=
          std::string::npos);
    CHECK(browserSource->find("auto tabRow = Rectangle<int>(tabStripX, titleRowOriginal.getY(), tabStripWidth, titleRowOriginal.getHeight());") !=
          std::string::npos);
    CHECK(browserSource->find("auto titleTextRow = titleRowOriginal.withRight(jmax(titleRowOriginal.getX(), tabRow.getX() - titleGap));") !=
          std::string::npos);
    CHECK(browserSource->find("titleLabel->setBounds(titleTextRow);") != std::string::npos);
    CHECK(browserSource->find("g.drawText(window.getName(), titleSpaceX, 0, titleSpaceW, h, Justification::centred, true);") !=
          std::string::npos);
    CHECK(browserSource->find("Plain close mark. The content window already carries the styled Close action.") !=
          std::string::npos);
    CHECK(browserSource->find("auto area = getLocalBounds().toFloat().withSizeKeepingCentre(16.0f, 16.0f);") !=
          std::string::npos);
    CHECK(browserSource->find("g.strokePath(mark, PathStrokeType(1.7f, PathStrokeType::curved, PathStrokeType::rounded));") !=
          std::string::npos);
    CHECK(browserSource->find("auto area = getLocalBounds().toFloat().reduced(5.0f);") == std::string::npos);
    CHECK(browserSource->find("g.drawLine(cross.getX(), cross.getY(), cross.getRight(), cross.getBottom(), 1.25f);") ==
          std::string::npos);
    CHECK(browserSource->find("bool NAMModelBrowserComponent::syncIRDirectoryFromSettingsIfAllowed()") !=
          std::string::npos);
    CHECK(browserSource->find("if (irDirectoryManuallySelected)") != std::string::npos);
    CHECK(browserSource->find("irDirectoryManuallySelected = true;") != std::string::npos);

    const auto switchToTabBlockStart = browserSource->find("void NAMModelBrowserComponent::switchToTab");
    REQUIRE(switchToTabBlockStart != std::string::npos);
    const auto switchToTabBlockEnd = browserSource->find("void NAMModelBrowserComponent::textEditorTextChanged",
                                                         switchToTabBlockStart);
    REQUIRE(switchToTabBlockEnd != std::string::npos);
    const auto switchToTabBlock =
        browserSource->substr(switchToTabBlockStart, switchToTabBlockEnd - switchToTabBlockStart);
    CHECK(switchToTabBlock.find("syncIRDirectoryFromSettingsIfAllowed()") != std::string::npos);
    CHECK(switchToTabBlock.find("scanIRDirectory(irDirectory)") != std::string::npos);

    const auto showWindowBlockStart = browserSource->find("void NAMModelBrowser::showWindow");
    REQUIRE(showWindowBlockStart != std::string::npos);
    const auto showWindowBlockEnd = browserSource->find("//==============================================================================",
                                                        showWindowBlockStart);
    REQUIRE(showWindowBlockEnd != std::string::npos);
    const auto showWindowBlock = browserSource->substr(showWindowBlockStart, showWindowBlockEnd - showWindowBlockStart);
    CHECK(showWindowBlock.find("syncIRDirectoryFromSettingsIfAllowed()") != std::string::npos);

    CHECK(preferencesHeader->find("Label* irDirLabel;") != std::string::npos);
    CHECK(preferencesHeader->find("Label* irDirValue;") != std::string::npos);
    CHECK(preferencesHeader->find("TextButton* irDirBrowseButton;") != std::string::npos);
    CHECK(preferencesSource->find("kIrLibraryDirectorySettingsKey = \"IRLibraryDirectory\"") != std::string::npos);
    CHECK(preferencesSource->find("getString(kIrLibraryDirectorySettingsKey, \"\")") != std::string::npos);
    CHECK(preferencesSource->find("setValue(kIrLibraryDirectorySettingsKey, selectedDir.getFullPathName())") !=
          std::string::npos);
}

TEST_CASE("NAM online browser polish source contract matches library visual structure",
          "[ui][regression][visual][source][library]")
{
    const auto onlineHeader = loadSourceFile("src/NAMOnlineBrowser.h");
    const auto onlineSource = loadSourceFile("src/NAMOnlineBrowser.cpp");
    REQUIRE(onlineHeader.has_value());
    REQUIRE(onlineSource.has_value());

    CHECK(onlineHeader->find("std::unique_ptr<juce::Component> detailsContent;") != std::string::npos);
    CHECK(onlineHeader->find("std::unique_ptr<juce::Viewport> detailsViewport;") == std::string::npos);
    CHECK(onlineSource->find("BrowserPalette makeOnlineBrowserPalette()") != std::string::npos);
    CHECK(onlineSource->find("0xFF211A2B, 0xFF140F1B, 0xFF271F33, 0xFF30273D, 0xFF0E0A14, 0xFF473A57, 0xFF5B4C6E") !=
          std::string::npos);
    CHECK(onlineSource->find("const auto palette = makeOnlineBrowserPalette();") != std::string::npos);
    CHECK(onlineSource->find("visual sync with makeBrowserPalette() in NAMModelBrowser.cpp") != std::string::npos);
    CHECK(onlineSource->find("const auto surface = palette.face.interpolatedWith(palette.inset, 0.18f);") !=
          std::string::npos);
    CHECK(onlineSource->find("surface.interpolatedWith(palette.accent, 0.16f)") != std::string::npos);
    CHECK(onlineSource->find("g.setColour(rowIsSelected ? palette.text : palette.text.withAlpha(0.95f));") !=
          std::string::npos);
    CHECK(onlineSource->find("bounds.removeFromTop(compactLayout ? 24 : 32);") != std::string::npos);
    CHECK(onlineSource->find("auto searchRow = bounds.removeFromTop(compactLayout ? 36 : 40);") !=
          std::string::npos);
    CHECK(onlineSource->find("const int searchButtonWidth = compactLayout ? 82 : 94;") != std::string::npos);
    CHECK(onlineSource->find("const int searchButtonGap = compactLayout ? 10 : 12;") != std::string::npos);
    CHECK(onlineSource->find("const int searchGroupWidth = juce::jmin(maxSearchGroupWidth, searchRow.getWidth());") !=
          std::string::npos);
    CHECK(onlineSource->find("auto searchGroup = searchRow.withSizeKeepingCentre(searchGroupWidth, searchRow.getHeight());") !=
          std::string::npos);
    const auto searchButtonLayout = onlineSource->find("searchButton->setBounds(searchGroup.removeFromRight(");
    const auto searchBoxLayout = onlineSource->find("searchBox->setBounds(searchGroup.removeFromLeft(searchBoxWidth));");
    REQUIRE(searchButtonLayout != std::string::npos);
    REQUIRE(searchBoxLayout != std::string::npos);
    CHECK(searchButtonLayout < searchBoxLayout);
    CHECK(onlineSource->find("auto toolbarBounds = outer.removeFromTop(compactLayout ? 106 : 118).toFloat();") !=
          std::string::npos);
    CHECK(onlineSource->find("const int maxSearchBoxWidth = compactLayout ? 280 : 380;") != std::string::npos);
    CHECK(onlineSource->find("resultsList->setBounds(trimListBoxBoundsToFullRows(listArea, resultsList->getRowHeight()));") !=
          std::string::npos);
    const auto selectedActionLayout =
        onlineSource->find("Keep the primary model actions visible directly under the selected-model hero.");
    const auto selectedDetailRows = onlineSource->find("auto row = detailsArea.removeFromTop(rowHeight);");
    REQUIRE(selectedActionLayout != std::string::npos);
    REQUIRE(selectedDetailRows != std::string::npos);
    CHECK(selectedActionLayout < selectedDetailRows);
    CHECK(onlineSource->find("auto actionRow = detailsArea.removeFromTop(compactLayout ? 31 : 33);") !=
          std::string::npos);
    CHECK(onlineSource->find("const int downloadWidth = compactLayout ? 136 : 156;") != std::string::npos);
    CHECK(onlineSource->find("auto searchRow = bounds.removeFromTop(compactLayout ? 32 : 36);") ==
          std::string::npos);
    CHECK(onlineSource->find("auto buttonRow = detailsArea.removeFromTop(compactLayout ? 30 : 32);") ==
          std::string::npos);
    CHECK(onlineSource->find("const int maxSearchBoxWidth = compactLayout ? 210 : 240;") == std::string::npos);
    CHECK(onlineSource->find("const int maxSearchBoxWidth = compactLayout ? 220 : 260;") == std::string::npos);
    CHECK(onlineSource->find("class OnlineBrowserActionButtonLookAndFeel") != std::string::npos);
    CHECK(onlineSource->find("const auto searchFill = palette.accent;") != std::string::npos);
    CHECK(onlineSource->find("const auto downloadFill = palette.accent;") != std::string::npos);
    CHECK(onlineSource->find("const auto loadFill = palette.accent2;") != std::string::npos);
    CHECK(onlineSource->find("const bool warmPrimary = label == \"Search\" || label == \"Download\" || label.startsWith(\"Downloading\");") !=
          std::string::npos);
    CHECK(onlineSource->find("const bool coolAudition = label == \"Load\";") != std::string::npos);
    CHECK(onlineSource->find("const auto actionAccent = coolAudition ? palette.accent2 : warmPrimary ? palette.accent : baseColour;") !=
          std::string::npos);
    CHECK(onlineSource->find("auto base = palette.face2.interpolatedWith(actionAccent, button.isEnabled() ? 0.16f : 0.05f);") !=
          std::string::npos);
    CHECK(onlineSource->find("base = palette.inset.interpolatedWith(palette.accent, isMouseOverButton ? 0.24f : 0.16f);") !=
          std::string::npos);
    CHECK(onlineSource->find("base = palette.inset.interpolatedWith(palette.accent2, isMouseOverButton ? 0.22f : 0.14f);") !=
          std::string::npos);
    CHECK(onlineSource->find("juce::ColourGradient fill(base.brighter(warmPrimary || coolAudition ? 0.06f : 0.12f)") !=
          std::string::npos);
    CHECK(onlineSource->find("warmPrimary || coolAudition ? actionAccent : isMouseOverButton ? baseColour : palette.edge") !=
          std::string::npos);
    const auto searchIconBlockStart = onlineSource->find("void NAMOnlineBrowserComponent::paintOverChildren");
    REQUIRE(searchIconBlockStart != std::string::npos);
    const auto searchIconBlockEnd = onlineSource->find("void NAMOnlineBrowserComponent::resized", searchIconBlockStart);
    REQUIRE(searchIconBlockEnd != std::string::npos);
    const auto searchIconBlock = onlineSource->substr(searchIconBlockStart, searchIconBlockEnd - searchIconBlockStart);
    CHECK(searchIconBlock.find("const auto palette = makeOnlineBrowserPalette();") != std::string::npos);
    CHECK(searchIconBlock.find("g.setColour(palette.text.withAlpha(0.45f));") != std::string::npos);
    CHECK(searchIconBlock.find("ColourScheme::getInstance().colours") == std::string::npos);
    CHECK(onlineSource->find("searchButton->setColour(juce::TextButton::textColourOffId, searchFill);") !=
          std::string::npos);
    CHECK(onlineSource->find("downloadButton->setColour(juce::TextButton::textColourOffId, downloadFill);") !=
          std::string::npos);
    CHECK(onlineSource->find("loadButton->setColour(juce::TextButton::textColourOffId, loadFill);") !=
          std::string::npos);
    CHECK(onlineSource->find("searchButton->setLookAndFeel(&onlineBrowserActionButtonLookAndFeel);") !=
          std::string::npos);
    CHECK(onlineSource->find("downloadButton->setLookAndFeel(&onlineBrowserActionButtonLookAndFeel);") !=
          std::string::npos);
    CHECK(onlineSource->find("loadButton->setLookAndFeel(&onlineBrowserActionButtonLookAndFeel);") !=
          std::string::npos);
    CHECK(onlineSource->find("loginButton->setLookAndFeel(&onlineBrowserActionButtonLookAndFeel);") !=
          std::string::npos);
    CHECK(onlineSource->find("logoutButton->setLookAndFeel(&onlineBrowserActionButtonLookAndFeel);") !=
          std::string::npos);
    CHECK(onlineSource->find("prevPageButton->setLookAndFeel(&onlineBrowserActionButtonLookAndFeel);") !=
          std::string::npos);
    CHECK(onlineSource->find("nextPageButton->setLookAndFeel(&onlineBrowserActionButtonLookAndFeel);") !=
          std::string::npos);
    CHECK(onlineSource->find("loginButton->setLookAndFeel(nullptr);") != std::string::npos);
    CHECK(onlineSource->find("nextPageButton->setLookAndFeel(nullptr);") != std::string::npos);
    CHECK(onlineSource->find("detailsContent->setInterceptsMouseClicks(false, true);") != std::string::npos);
    CHECK(onlineSource->find("detailsContent->setBounds(bounds);") != std::string::npos);
    CHECK(onlineSource->find("detailsContent->addAndMakeVisible(downloadButton.get());") != std::string::npos);
    CHECK(onlineSource->find("detailsViewport->setViewedComponent(detailsContent.get(), false);") ==
          std::string::npos);
    CHECK(onlineSource->find("detailsContent->addAndMakeVisible(nameLabel.get());") != std::string::npos);
    CHECK(onlineSource->find("melatonin::DropShadow shadow{juce::Colours::black.withAlpha(0.28f), 10, {0, 4}};") !=
          std::string::npos);
    CHECK(onlineSource->find("juce::ColourGradient cardGrad(palette.face2, detailsBounds.getX(), detailsBounds.getY(),") !=
          std::string::npos);
    CHECK(onlineSource->find("palette.face, detailsBounds.getX(), detailsBounds.getBottom(), false);") !=
          std::string::npos);
    CHECK(onlineSource->find("g.setColour(palette.edgeHi.withAlpha(0.36f));") != std::string::npos);
    CHECK(onlineSource->find("setMinimumHorizontalScale(0.72f)") != std::string::npos);
    CHECK(onlineSource->find("g.drawText(juce::String(selectedTone->name),") != std::string::npos);
    CHECK(onlineSource->find("g.drawText(\"by \" + juce::String(selectedTone->authorName),") !=
          std::string::npos);
}

TEST_CASE("NAM parametric EQ is additive to the existing tone stack",
          "[ui][regression][visual][source][nam][eq]")
{
    const auto processorHeader = loadSourceFile("src/NAMProcessor.h");
    const auto processorSource = loadSourceFile("src/NAMProcessor.cpp");
    const auto controlHeader = loadSourceFile("src/NAMControl.h");
    const auto controlSource = loadSourceFile("src/NAMControl.cpp");

    REQUIRE(processorHeader.has_value());
    REQUIRE(processorSource.has_value());
    REQUIRE(controlHeader.has_value());
    REQUIRE(controlSource.has_value());

    CHECK(processorHeader->find("enum class ToneEqMode") != std::string::npos);
    CHECK(processorHeader->find("Stack = 0") != std::string::npos);
    CHECK(processorHeader->find("Parametric = 1") != std::string::npos);
    CHECK(processorHeader->find("static constexpr int kParamEqBandCount = 12;") != std::string::npos);
    CHECK(processorHeader->find("int getActiveParamEqBandCount() const;") != std::string::npos);
    CHECK(processorHeader->find("void setActiveParamEqBandCount(int count);") != std::string::npos);

    CHECK(processorHeader->find("float getBass() const") != std::string::npos);
    CHECK(processorHeader->find("void setBass(float value)") != std::string::npos);
    CHECK(processorHeader->find("std::atomic<float> bass{5.0f}") != std::string::npos);
    CHECK(processorHeader->find("std::atomic<float> mid{5.0f}") != std::string::npos);
    CHECK(processorHeader->find("std::atomic<float> treble{5.0f}") != std::string::npos);

    CHECK(processorHeader->find("ToneEqMode getToneEqMode() const;") != std::string::npos);
    CHECK(processorHeader->find("void setToneEqMode(ToneEqMode mode);") != std::string::npos);
    CHECK(processorHeader->find("float getParamEqBandFrequency(int bandIndex) const;") != std::string::npos);
    CHECK(processorHeader->find("void setParamEqBandFrequency(int bandIndex, float freqHz);") !=
          std::string::npos);
    CHECK(processorHeader->find("float getParamEqBandGain(int bandIndex) const;") != std::string::npos);
    CHECK(processorHeader->find("void setParamEqBandGain(int bandIndex, float gainDb);") != std::string::npos);
    CHECK(processorHeader->find("float getParamEqBandQ(int bandIndex) const;") != std::string::npos);
    CHECK(processorHeader->find("void setParamEqBandQ(int bandIndex, float q);") != std::string::npos);

    CHECK(processorSource->find("applySelectedToneEq(inputData, numSamples);") != std::string::npos);
    CHECK(processorSource->find("applySelectedToneEq(outputData, numSamples);") != std::string::npos);
    CHECK(processorSource->find("void NAMProcessor::applyParametricEq(float* data, int numSamples)") !=
          std::string::npos);
    CHECK(processorSource->find("void NAMProcessor::updateParametricEqCoefficients()") != std::string::npos);
    CHECK(processorSource->find("stream.writeInt(9); // Version (9 = NAM A2 slimmable size)") !=
          std::string::npos);
    CHECK(processorSource->find("if (version >= 7 && !stream.isExhausted())") != std::string::npos);
    CHECK(processorSource->find("if (version >= 8 && !stream.isExhausted())") != std::string::npos);
    CHECK(processorSource->find("setActiveParamEqBandCount(stream.readInt())") != std::string::npos);

    CHECK(controlHeader->find("void updateEqModeVisibility();") != std::string::npos);
    CHECK(controlHeader->find("std::unique_ptr<TextButton> toneEqModeStackButton;") != std::string::npos);
    CHECK(controlHeader->find("std::unique_ptr<TextButton> toneEqModeParamButton;") != std::string::npos);
    CHECK(controlHeader->find("std::unique_ptr<TextButton> paramEqBandCountButton;") != std::string::npos);
    CHECK(controlHeader->find("std::array<std::unique_ptr<Slider>, NAMProcessor::kParamEqBandCount> paramEqFrequencySliders;") !=
          std::string::npos);
    CHECK(controlHeader->find("std::array<std::unique_ptr<Slider>, NAMProcessor::kParamEqBandCount> paramEqGainSliders;") !=
          std::string::npos);
    CHECK(controlHeader->find("std::array<std::unique_ptr<Slider>, NAMProcessor::kParamEqBandCount> paramEqQSliders;") !=
          std::string::npos);

    CHECK(controlSource->find("\"STACK\"") != std::string::npos);
    CHECK(controlSource->find("\"PARAM\"") != std::string::npos);
    CHECK(controlSource->find("\"Parametric EQ\"") != std::string::npos);
    CHECK(controlSource->find("toneEqModeStackButton->setBounds") != std::string::npos);
    CHECK(controlSource->find("toneEqModeParamButton->setBounds") != std::string::npos);
    CHECK(controlSource->find("bassSlider->setVisible(!parametric") != std::string::npos);
    CHECK(controlSource->find("paramEqFrequencySliders[band]->setVisible(visible)") != std::string::npos);
    CHECK(controlSource->find("paramEqBandCountButton->setButtonText") != std::string::npos);
}

TEST_CASE("Tuner node polish mirrors mockup readout structure without removing real modes",
          "[ui][regression][visual][source][nodes][tuner]")
{
    const auto tunerHeader = loadSourceFile("src/TunerControl.h");
    const auto tunerSource = loadSourceFile("src/TunerControl.cpp");
    const auto tunerProcessorHeader = loadSourceFile("src/TunerProcessor.h");
    const auto tunerProcessorSource = loadSourceFile("src/TunerProcessor.cpp");
    const auto stageViewHeader = loadSourceFile("src/StageView.h");
    const auto stageViewSource = loadSourceFile("src/StageView.cpp");

    REQUIRE(tunerHeader.has_value());
    REQUIRE(tunerSource.has_value());
    REQUIRE(tunerProcessorHeader.has_value());
    REQUIRE(tunerProcessorSource.has_value());
    REQUIRE(stageViewHeader.has_value());
    REQUIRE(stageViewSource.has_value());

    CHECK(tunerHeader->find("void drawTunerGlassPanel(Graphics& g, Rectangle<float> bounds);") !=
          std::string::npos);
    CHECK(tunerHeader->find("void drawTunerHeader(Graphics& g, Rectangle<float> bounds);") !=
          std::string::npos);
    CHECK(tunerHeader->find("void drawNoteGlyph(Graphics& g, Rectangle<float> bounds, const String& noteName, Colour noteColour);") !=
          std::string::npos);
    CHECK(tunerHeader->find("void drawNeedleArcBackdrop(Graphics& g, Point<float> centre, float radius);") !=
          std::string::npos);
    CHECK(tunerHeader->find("void drawModeSegmentedControl(Graphics& g, Rectangle<float> bounds);") !=
          std::string::npos);
    CHECK(tunerHeader->find("void drawCoarseDeviationStrip(Graphics& g, Rectangle<float> bounds);") !=
          std::string::npos);
    CHECK(tunerHeader->find("void drawStatusBadge(Graphics& g, Rectangle<float> bounds);") !=
          std::string::npos);
    CHECK(tunerHeader->find("void drawSignalConfidenceStrip(Graphics& g, Rectangle<float> bounds);") !=
          std::string::npos);
    CHECK(tunerHeader->find("void drawPitchTrace(Graphics& g, Rectangle<float> bounds);") !=
          std::string::npos);
    CHECK(tunerHeader->find("void drawReferenceResponseRail(Graphics& g, Rectangle<float> bounds);") !=
          std::string::npos);
    CHECK(tunerHeader->find("std::unique_ptr<TextButton> needleModeButton;") != std::string::npos);
    CHECK(tunerHeader->find("std::unique_ptr<TextButton> driftModeButton;") != std::string::npos);
    CHECK(tunerHeader->find("TunerMode::Needle") != std::string::npos);
    CHECK(tunerHeader->find("TunerMode::PitchDrift") != std::string::npos);
    CHECK(tunerHeader->find("TunerMode::Strobe") == std::string::npos);
    CHECK(tunerHeader->find("TunerMode::Poly") == std::string::npos);

    CHECK(tunerSource->find("needleModeButton = std::make_unique<TextButton>(\"NEEDLE\");") !=
          std::string::npos);
    CHECK(tunerSource->find("driftModeButton = std::make_unique<TextButton>(\"DRIFT\");") !=
          std::string::npos);
    CHECK(tunerSource->find("driftModeButton->setTooltip(\"Pitch drift view\");") != std::string::npos);
    CHECK(tunerSource->find("sixStringModeButton = std::make_unique<TextButton>(\"STRINGS\");") !=
          std::string::npos);
    CHECK(tunerSource->find("sixStringModeButton->setTooltip(\"Six-string guitar reference view\");") !=
          std::string::npos);
    CHECK(tunerSource->find("\"STROBE\"") == std::string::npos);
    CHECK(tunerSource->find("\"POLY\"") == std::string::npos);
    CHECK(tunerHeader->find("0.1 cent") == std::string::npos);
    CHECK(tunerHeader->find("strobe-view") == std::string::npos);
    CHECK(tunerHeader->find("STROBE:") == std::string::npos);
    CHECK(tunerProcessorHeader->find("0.1 cent") == std::string::npos);
    CHECK(tunerHeader->find("Turbo Tuner") == std::string::npos);
    CHECK(tunerProcessorHeader->find("Phase-based strobe") == std::string::npos);
    CHECK(tunerProcessorHeader->find("Pro:") == std::string::npos);
    CHECK(tunerSource->find("setSize(390, 320);") != std::string::npos);
    CHECK(tunerSource->find("drawTunerGlassPanel(g, bounds);") != std::string::npos);
    CHECK(tunerSource->find("drawTunerHeader(g, headerArea);") != std::string::npos);
    CHECK(tunerSource->find("bounds.removeFromTop(41);") != std::string::npos);
    CHECK(tunerSource->find("auto modeArea = area.removeFromTop(29);") != std::string::npos);
    CHECK(tunerSource->find("bypassButton = std::make_unique<TextButton>(\"BYPASS\");") !=
          std::string::npos);
    CHECK(tunerSource->find("drawModeSegmentedControl(g, modeArea);") != std::string::npos);
    CHECK(tunerSource->find("drawSignalConfidenceStrip(g, confidenceArea);") != std::string::npos);
    CHECK(tunerSource->find("drawPitchTrace(g, traceArea);") != std::string::npos);
    CHECK(tunerSource->find("drawReferenceResponseRail(g, railArea);") != std::string::npos);
    CHECK(tunerSource->find("auto bypassPlate = bounds.removeFromRight(82.0f).reduced(1.0f);") !=
          std::string::npos);
    CHECK(tunerSource->find("for (int i = 1; i < 3; ++i)") != std::string::npos);
    CHECK(tunerSource->find("const auto separatorX = plate.getX() + segmentW * static_cast<float>(i);") !=
          std::string::npos);
    CHECK(tunerSource->find("g.drawLine(separatorX, plate.getY() + 5.0f, separatorX, plate.getBottom() - 5.0f, 0.8f);") !=
          std::string::npos);
    CHECK(tunerSource->find("drawNoteGlyph(g, bounds, noteName, noteCol);") != std::string::npos);
    CHECK(tunerSource->find("drawCoarseDeviationStrip(g, coarseArea);") != std::string::npos);
    CHECK(tunerSource->find("drawNeedleArcBackdrop(g, {centreX, bottomY}, meterRadius);") !=
          std::string::npos);
    CHECK(tunerSource->find("drawStatusBadge(g, statusArea);") != std::string::npos);
    CHECK(tunerSource->find("\"In Tune\"") != std::string::npos);
    CHECK(tunerSource->find("\"A=440\"") != std::string::npos);
    CHECK(tunerSource->find("const float dotX = track.getX() + track.getWidth() * normalized;") !=
          std::string::npos);
    CHECK(tunerSource->find("needleModeButton->setBounds") != std::string::npos);
    CHECK(tunerSource->find("driftModeButton->setBounds") != std::string::npos);
    CHECK(tunerSource->find("modeButton") == std::string::npos);
    CHECK(tunerSource->find("polyModeButton") == std::string::npos);
    CHECK(tunerProcessorHeader->find("Point<int> getSize() override { return Point<int>(390, 320); }") !=
          std::string::npos);
    CHECK(tunerProcessorHeader->find("float getDetectedConfidence() const") != std::string::npos);
    CHECK(tunerProcessorHeader->find("PinLayout getInputPinLayout() const override;") != std::string::npos);
    CHECK(tunerProcessorHeader->find("PinLayout getOutputPinLayout() const override;") != std::string::npos);
    CHECK(tunerProcessorSource->find("setPlayConfigDetails(1, 1, 0, 0);") != std::string::npos);
    CHECK(tunerProcessorSource->find("constexpr int kTunerAudioPinY = 78;") != std::string::npos);
    CHECK(tunerProcessorSource->find("layout.pinY.push_back(kTunerAudioPinY);") != std::string::npos);
    CHECK(tunerProcessorHeader->find("#include \"dsp/TunerAnalysis.h\"") != std::string::npos);
    CHECK(tunerProcessorHeader->find("#include <thread>") != std::string::npos);
    CHECK(tunerProcessorHeader->find("void startAnalysisThread();") != std::string::npos);
    CHECK(tunerProcessorHeader->find("void stopAnalysisThread() noexcept;") != std::string::npos);
    CHECK(tunerProcessorHeader->find("void publishAnalysisWindow() noexcept;") != std::string::npos);
    CHECK(tunerProcessorHeader->find("void analysisThreadMain() noexcept;") != std::string::npos);
    CHECK(tunerProcessorHeader->find("enum class ResponseMode") != std::string::npos);
    CHECK(tunerProcessorHeader->find("void setReferenceA4Hz(float frequencyHz) noexcept;") !=
          std::string::npos);
    CHECK(tunerProcessorHeader->find("float getReferenceA4Hz() const") != std::string::npos);
    CHECK(tunerProcessorHeader->find("int getGuitarStringInTuneMask() const") != std::string::npos);
    CHECK(tunerProcessorHeader->find("int getCurrentGuitarStringIndex() const") != std::string::npos);
    CHECK(tunerProcessorHeader->find("float getCurrentGuitarStringCents() const") != std::string::npos);
    CHECK(tunerProcessorHeader->find("void resetGuitarStringChecklist() noexcept;") != std::string::npos);
    CHECK(tunerProcessorHeader->find("std::atomic<int> guitarStringInTuneMask{0};") != std::string::npos);
    CHECK(tunerProcessorHeader->find("std::atomic<int> currentGuitarStringIndex{-1};") !=
          std::string::npos);
    CHECK(tunerProcessorHeader->find("void setResponseMode(ResponseMode mode) noexcept;") !=
          std::string::npos);
    CHECK(tunerProcessorHeader->find("ResponseMode getResponseMode() const noexcept;") !=
          std::string::npos);
    CHECK(tunerProcessorHeader->find("std::atomic<float> referenceA4Hz{440.0f};") !=
          std::string::npos);
    CHECK(tunerProcessorHeader->find("std::atomic<float> detectedConfidence{0.0f};") !=
          std::string::npos);
    CHECK(tunerProcessorHeader->find("std::atomic<int> responseMode") != std::string::npos);
    CHECK(tunerProcessorHeader->find("int heldMissCount = 0;") != std::string::npos);
    CHECK(tunerProcessorHeader->find("std::thread analysisThread;") != std::string::npos);
    CHECK(tunerProcessorHeader->find("std::atomic<bool> analysisWindowPending{false};") !=
          std::string::npos);
    CHECK(tunerProcessorHeader->find("std::array<std::array<float, pedalboard3::dsp::TunerAnalysis::kAnalysisWindowSize>, 2> analysisWindows;") !=
          std::string::npos);
    CHECK(tunerProcessorSource->find("stopAnalysisThread();") != std::string::npos);
    CHECK(tunerProcessorSource->find("startAnalysisThread();") != std::string::npos);
    CHECK(tunerProcessorSource->find("publishAnalysisWindow();") != std::string::npos);
    CHECK(tunerProcessorSource->find("analysisWindowPending.exchange(false") != std::string::npos);
    CHECK(tunerProcessorSource->find("backgroundAnalyzer.pushSamples(analysisWindows") != std::string::npos);
    CHECK(tunerProcessorSource->find("backgroundAnalyzer.analyze();") != std::string::npos);
    CHECK(tunerProcessorSource->find("backgroundAnalyzer.setReferenceA4Hz(referenceA4Hz.load") !=
          std::string::npos);
    CHECK(tunerProcessorSource->find("updateGuitarStringChecklist(result.frequencyHz, result.referenceA4Hz);") !=
          std::string::npos);
    CHECK(tunerProcessorSource->find("constexpr std::array<int, 6> kStandardGuitarStringMidiNotes") !=
          std::string::npos);
    CHECK(tunerSource->find("tunerProcessor->getGuitarStringInTuneMask()") != std::string::npos);
    CHECK(tunerSource->find("tunerProcessor->getCurrentGuitarStringIndex()") != std::string::npos);
    CHECK(tunerSource->find("strings[i].startsWith(root)") == std::string::npos);
    CHECK(tunerSource->find("\"ALL STRINGS READY\"") != std::string::npos);
    CHECK(tunerHeader->find("std::array<float, kPitchTraceSize> pitchTraceCents{};") != std::string::npos);
    CHECK(tunerHeader->find("std::array<float, kPitchTraceSize> pitchTraceConfidence{};") !=
          std::string::npos);
    CHECK(tunerSource->find("tunerProcessor->getDetectedConfidence()") != std::string::npos);
    CHECK(tunerSource->find("kPitchTraceConnectBreakCents") != std::string::npos);
    CHECK(tunerProcessorSource->find("constexpr int kTunerStateVersion = 2;") != std::string::npos);
    CHECK(tunerProcessorSource->find("stream.writeFloat(getReferenceA4Hz());") != std::string::npos);
    CHECK(tunerProcessorSource->find("stream.writeInt(static_cast<int>(getResponseMode()));") !=
          std::string::npos);
    CHECK(tunerSource->find("tunerProcessor->getReferenceA4Hz()") != std::string::npos);
    CHECK(tunerProcessorSource->find("std::this_thread::sleep_for") != std::string::npos);
    CHECK(tunerProcessorSource->find("detectPitchYIN") == std::string::npos);
    CHECK(tunerProcessorSource->find("yinBuffer") == std::string::npos);
    CHECK(tunerProcessorSource->find("contiguousBuffer") == std::string::npos);

    CHECK(stageViewHeader->find("void drawStageTunerTrace(Graphics& g, Rectangle<float> bounds);") !=
          std::string::npos);
    CHECK(stageViewHeader->find("void drawStageStringChecklist(Graphics& g, Rectangle<float> bounds);") !=
          std::string::npos);
    CHECK(stageViewHeader->find("void drawStageTunerRail(Graphics& g, Rectangle<float> bounds);") !=
          std::string::npos);
    CHECK(stageViewHeader->find("std::array<float, kStagePitchTraceSize> stagePitchTraceCents{};") !=
          std::string::npos);
    CHECK(stageViewSource->find("drawStageTunerTrace(g, body);") != std::string::npos);
    CHECK(stageViewSource->find("drawStageStringChecklist(g, stringArea);") != std::string::npos);
    CHECK(stageViewSource->find("drawStageTunerRail(g, railArea);") != std::string::npos);
    CHECK(stageViewSource->find("tunerProcessor->getDetectedConfidence()") != std::string::npos);
    CHECK(stageViewSource->find("tunerProcessor->getGuitarStringInTuneMask()") != std::string::npos);
    CHECK(stageViewSource->find("kStageStringMidiNotes{40, 45, 50, 55, 59, 64}") != std::string::npos);
    CHECK(stageViewSource->find("const bool showPatchNavigation = viewMode != ViewMode::Tuner;") !=
          std::string::npos);
    CHECK(stageViewSource->find("prevButton->setVisible(showPatchNavigation);") != std::string::npos);
    CHECK(stageViewSource->find("nextButton->setVisible(showPatchNavigation);") != std::string::npos);
    CHECK(stageViewSource->find("PITCH HISTORY") != std::string::npos);
    CHECK(stageViewSource->find("STRING CHECK") != std::string::npos);
}

TEST_CASE("Utility scope and tone nodes keep explicit bus and pin footprint contracts",
          "[ui][regression][visual][source][nodes][utility]")
{
    const auto pluginSource = loadSourceFile("src/PluginComponent.cpp");
    const auto oscilloscopeHeader = loadSourceFile("src/OscilloscopeProcessor.h");
    const auto oscilloscopeSource = loadSourceFile("src/OscilloscopeProcessor.cpp");
    const auto oscilloscopeControl = loadSourceFile("src/OscilloscopeControl.cpp");
    const auto toneHeader = loadSourceFile("src/ToneGeneratorProcessor.h");
    const auto toneSource = loadSourceFile("src/ToneGeneratorProcessor.cpp");
    const auto toneControl = loadSourceFile("src/ToneGeneratorControl.cpp");
    const auto reverbSource = loadSourceFile("src/ReverbSCProcessor.cpp");

    REQUIRE(pluginSource.has_value());
    REQUIRE(oscilloscopeHeader.has_value());
    REQUIRE(oscilloscopeSource.has_value());
    REQUIRE(oscilloscopeControl.has_value());
    REQUIRE(toneHeader.has_value());
    REQUIRE(toneSource.has_value());
    REQUIRE(toneControl.has_value());
    REQUIRE(reverbSource.has_value());

    CHECK(pluginSource->find("bool suppressesHostParamPinForUtilityNode(const String& pluginName)") !=
          std::string::npos);
    CHECK(pluginSource->find("return getEmbeddedNodeShellPolicy(pluginName).suppressesUtilityHostParamPin;") !=
          std::string::npos);
    CHECK(pluginSource->find("bool shouldCreateHostMidiOrParamPin(AudioProcessor* plugin, const String& pluginName, int numInputs, int numOutputs)") !=
          std::string::npos);
    CHECK(pluginSource->find("if (shellPolicy.suppressesHostMidiOrParamPin || suppressesHostParamPinForUtilityNode(pluginName))") !=
          std::string::npos);
    CHECK(pluginSource->find("if (shouldCreateHostMidiOrParamPin(plugin, pluginName, numIn, numOut) && pluginName != \"MIDI Input\")") !=
          std::string::npos);

    CHECK(oscilloscopeHeader->find("Point<int> getSize() override { return Point<int>(280, 154); }") !=
          std::string::npos);
    CHECK(oscilloscopeHeader->find("PinLayout getInputPinLayout() const override;") != std::string::npos);
    CHECK(oscilloscopeHeader->find("PinLayout getOutputPinLayout() const override;") != std::string::npos);
    CHECK(oscilloscopeSource->find("setPlayConfigDetails(2, 2, 0, 0);") != std::string::npos);
    CHECK(oscilloscopeSource->find("constexpr int kOscilloscopeUpperAudioPinY = 64;") != std::string::npos);
    CHECK(oscilloscopeSource->find("constexpr int kOscilloscopeLowerAudioPinY = 106;") != std::string::npos);
    CHECK(oscilloscopeSource->find("layout.pinY.push_back(kOscilloscopeUpperAudioPinY);") !=
          std::string::npos);
    CHECK(oscilloscopeSource->find("layout.pinY.push_back(kOscilloscopeLowerAudioPinY);") !=
          std::string::npos);
    CHECK(oscilloscopeControl->find("setSize(280, 154);") != std::string::npos);
    CHECK(oscilloscopeControl->find("g.drawText(\"OSCILLOSCOPE\"") != std::string::npos);
    CHECK(oscilloscopeControl->find("g.drawText(\"NO SIGNAL\"") != std::string::npos);

    CHECK(toneHeader->find("Point<int> getSize() override { return Point<int>(392, 308); }") !=
          std::string::npos);
    CHECK(toneHeader->find("PinLayout getOutputPinLayout() const override;") != std::string::npos);
    CHECK(toneSource->find("setPlayConfigDetails(0, 2, 44100.0, 512);") != std::string::npos);
    CHECK(toneSource->find("constexpr int kToneGeneratorLeftOutputPinY = 70;") != std::string::npos);
    CHECK(toneSource->find("constexpr int kToneGeneratorRightOutputPinY = 92;") != std::string::npos);
    CHECK(toneSource->find("layout.pinY.push_back(kToneGeneratorLeftOutputPinY);") != std::string::npos);
    CHECK(toneSource->find("layout.pinY.push_back(kToneGeneratorRightOutputPinY);") != std::string::npos);
    CHECK(toneControl->find("setSize(392, 308);") != std::string::npos);
    CHECK(toneControl->find("Colour ToneGeneratorControl::getAccentColour() const") != std::string::npos);
    CHECK(toneControl->find("Colour ToneGeneratorControl::getPanelTextColour(Colour surface, float alpha) const") !=
          std::string::npos);
    CHECK(toneControl->find("surface.withAlpha(1.0f).contrasting(0.92f).withAlpha(alpha)") !=
          std::string::npos);
    CHECK(toneControl->find("Colour(0xFF39D3E6)") == std::string::npos);
    CHECK(toneControl->find("Colours::white") == std::string::npos);
    CHECK(toneControl->find("Colours::black") == std::string::npos);
    CHECK(toneControl->find("\"READY\"") == std::string::npos);
    CHECK(toneControl->find("void ToneGeneratorControl::styleEditableSlider(Slider& slider, Colour accent, const String& suffix, int textBoxWidth)") !=
          std::string::npos);
    CHECK(toneControl->find("frequencySlider->setTextBoxStyle(Slider::NoTextBox, false, 0, 0);") !=
          std::string::npos);
    CHECK(toneControl->find("detuneSlider->setTextBoxStyle(Slider::NoTextBox, false, 0, 0);") !=
          std::string::npos);
    CHECK(toneControl->find("amplitudeSlider->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);") !=
          std::string::npos);
    CHECK(toneControl->find("amplitudeSlider->setTextBoxStyle(Slider::TextBoxBelow, false, 54, 16);") !=
          std::string::npos);
    CHECK(toneControl->find("frequencySlider->setAlpha(0.01f);") == std::string::npos);
    CHECK(toneControl->find("detuneSlider->setAlpha(0.01f);") == std::string::npos);
    CHECK(toneControl->find("amplitudeSlider->setAlpha(0.01f);") == std::string::npos);
    CHECK(toneControl->find("void ToneGeneratorControl::drawChromeShell(Graphics& g, Rectangle<float> bounds)") !=
          std::string::npos);
    CHECK(toneControl->find("void ToneGeneratorControl::drawDisplayPanel(Graphics& g, Rectangle<float> bounds)") !=
          std::string::npos);
    CHECK(toneControl->find("void ToneGeneratorControl::drawWaveformGlyph(Graphics& g, Rectangle<float> bounds)") !=
          std::string::npos);
    CHECK(toneControl->find("void ToneGeneratorControl::drawSectionLabel(Graphics& g, Rectangle<float> bounds, const String& text)") !=
          std::string::npos);
    CHECK(toneControl->find("void ToneGeneratorControl::drawOutputKnob(Graphics& g, Rectangle<float> bounds, float normalisedValue, Colour accent)") !=
          std::string::npos);
    CHECK(toneControl->find("void ToneGeneratorControl::drawValueChip(Graphics& g, Rectangle<float> bounds, const String& text, Colour accent)") !=
          std::string::npos);
    CHECK(toneControl->find("button.setColour(TextButton::textColourOffId, getPanelTextColour(buttonBase, active ? 0.98f : 0.76f));") !=
          std::string::npos);
    CHECK(toneControl->find("slider.setColour(Slider::textBoxTextColourId, getPanelTextColour(textBoxBase, 0.90f));") !=
          std::string::npos);
    CHECK(toneControl->find("displayPanel = bounds.removeFromTop(60);") != std::string::npos);
    CHECK(toneControl->find("pitchPanel = bounds.removeFromTop(70);") != std::string::npos);
    CHECK(toneControl->find("pitchFrequencyChipArea = freqRow.removeFromRight(74);") != std::string::npos);
    CHECK(toneControl->find("frequencySlider->setBounds(frequencyRail.withTrimmedLeft(30).expanded(3, 5));") !=
          std::string::npos);
    CHECK(toneControl->find("detuneChipArea = detuneRow.removeFromRight(70);") != std::string::npos);
    CHECK(toneControl->find("detuneSlider->setBounds(detuneRail.withTrimmedLeft(30).expanded(3, 5));") !=
          std::string::npos);
    CHECK(toneControl->find("bottomPanel = bounds.removeFromTop(80);") != std::string::npos);
    CHECK(toneControl->find("bottomInner.removeFromTop(16);") != std::string::npos);
    CHECK(toneControl->find("outputKnobArea = outputLayout.removeFromLeft(82);") != std::string::npos);
    CHECK(toneControl->find("const int btnW = 32;") != std::string::npos);
    CHECK(toneControl->find("drawWaveformGlyph(g, waveformGlyphArea.toFloat());") != std::string::npos);
    CHECK(toneControl->find("g.drawText(\"WAVE\", tile.removeFromBottom(12.0f)") == std::string::npos);
    CHECK(toneControl->find("g.drawText(\"WAVE\", waveformPanel.toFloat().removeFromTop") == std::string::npos);
    CHECK(toneControl->find("g.fillAll();") == std::string::npos);
    CHECK(toneControl->find("g.drawText(\"Freq:\", Rectangle<float>(10, 28, 34, 14)") ==
          std::string::npos);

    CHECK(reverbSource->find("class ReverbSCControl final : public Component, private Timer") !=
          std::string::npos);
    CHECK(reverbSource->find("colours[\"Graph Category Reverb\"]") != std::string::npos);
    CHECK(reverbSource->find("colours[\"Graph Category Delay\"]") != std::string::npos);
    CHECK(reverbSource->find("colours[\"Success Colour\"]") != std::string::npos);
    CHECK(reverbSource->find("Colours::white") == std::string::npos);
    CHECK(reverbSource->find("Colours::black") == std::string::npos);
    CHECK(reverbSource->find("Colour(0x") == std::string::npos);
}

TEST_CASE("VU meter nodes use shared meter semantics and polished direct-painted chrome",
          "[ui][regression][visual][source][nodes][meters]")
{
    const auto processorHeader = loadSourceFile("src/PedalboardProcessors.h");
    const auto processorSource = loadSourceFile("src/VuMeterProcessor.cpp");
    const auto controlSource = loadSourceFile("src/VuMeterEditors.cpp");
    const auto pluginHeader = loadSourceFile("src/PluginComponent.h");
    const auto pluginSource = loadSourceFile("src/PluginComponent.cpp");

    REQUIRE(processorHeader.has_value());
    REQUIRE(processorSource.has_value());
    REQUIRE(controlSource.has_value());
    REQUIRE(pluginHeader.has_value());
    REQUIRE(pluginSource.has_value());

    CHECK(processorHeader->find("#include \"dsp/MeterSource.h\"") != std::string::npos);
    CHECK(processorHeader->find("NodeShellPolicy getNodeShellPolicy() const override { return NodeShellPolicy::directPainted(true); }") !=
          std::string::npos);
    CHECK(processorHeader->find("float getLeftRmsLevel() const") != std::string::npos);
    CHECK(processorHeader->find("float getRightRmsLevel() const") != std::string::npos);
    CHECK(processorHeader->find("float getLeftVuLevel() const") != std::string::npos);
    CHECK(processorHeader->find("float getRightVuLevel() const") != std::string::npos);
    CHECK(processorHeader->find("bool getLeftAndClearClip()") != std::string::npos);
    CHECK(processorHeader->find("PedalboardMeterSource meterSource;") != std::string::npos);
    CHECK(processorHeader->find("std::atomic<float> levelLeft") == std::string::npos);

    CHECK(processorSource->find("meterSource.prepare(sampleRate, 2);") != std::string::npos);
    CHECK(processorSource->find("meterSource.process(channelData.data(), channelCount, buffer.getNumSamples());") !=
          std::string::npos);
    CHECK(processorSource->find("buffer.getWritePointer") == std::string::npos);

    CHECK(controlSource->find("void VuMeterControl::drawChromeShell(Graphics& g, Rectangle<float> bounds)") !=
          std::string::npos);
    CHECK(controlSource->find("void VuMeterControl::drawVuMeterColumn(Graphics& g, Rectangle<float> bounds, const MeterSnapshot& snapshot, const String& label)") !=
          std::string::npos);
    CHECK(controlSource->find("void VuMeterControl::drawVuMeterScale(Graphics& g, Rectangle<float> bounds)") !=
          std::string::npos);
    CHECK(controlSource->find("void VuMeterControl::drawVuMeterValuePill(Graphics& g, Rectangle<float> bounds, const String& text, Colour accent)") !=
          std::string::npos);
    CHECK(controlSource->find("Colour(0x") == std::string::npos);
    CHECK(controlSource->find("Colours::white") == std::string::npos);
    CHECK(controlSource->find("Colours::black") == std::string::npos);

    CHECK(pluginHeader->find("float cachedRmsLevels[16]{};") != std::string::npos);
    CHECK(pluginHeader->find("bool cachedClipState[16]{};") != std::string::npos);
    CHECK(pluginSource->find("drawCompactAudioIoMeter(g,") != std::string::npos);
    CHECK(pluginSource->find("limiter->getInputRmsLevel(ch)") != std::string::npos);
    CHECK(pluginSource->find("limiter->getOutputRmsLevel(ch)") != std::string::npos);
    CHECK(pluginSource->find("limiter->getInputAndClearClip(ch)") != std::string::npos);
    CHECK(pluginSource->find("limiter->getOutputAndClearClip(ch)") != std::string::npos);
}
