#pragma once

#include <JuceHeader.h>

#include <array>
#include <optional>

namespace UiScale
{
inline constexpr const char* settingsKey = "UiScalePercent";
inline constexpr int minimumPercent = 75;
inline constexpr int defaultPercent = 100;
inline constexpr int maximumPercent = 200;
inline constexpr int footerControlBaseMinimumWidth = 960;
inline constexpr int singleRowFooterBaseMinimumWidth = 1300;
inline constexpr int twoRowFooterBaseMinimumWidth = 620;
inline constexpr int singleRowFooterHeight = 40;
inline constexpr int compactFooterHeight = 72;
inline constexpr int stackedFooterHeight = 104;

std::array<int, 6> supportedPercents();
bool isSupportedPercent(int percent);
int normalisePercent(int percent);
float toScaleFactor(int percent);
int footerLayoutWidth(int componentWidth, int percent);
int footerControlMinimumWidth(int percent);
bool shouldShowFooterControl(int componentWidth, int percent);
bool shouldUseSingleRowFooter(int componentWidth, int percent);
bool shouldUseStackedFooter(int componentWidth, int percent);
int footerHeight(int componentWidth, int percent);
std::optional<int> parseVisualQaOverride(const juce::String& commandLine);
} // namespace UiScale
