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

std::array<int, 6> supportedPercents();
bool isSupportedPercent(int percent);
int normalisePercent(int percent);
float toScaleFactor(int percent);
int footerControlMinimumWidth(int percent);
bool shouldShowFooterControl(int componentWidth, int percent);
std::optional<int> parseVisualQaOverride(const juce::String& commandLine);
} // namespace UiScale
