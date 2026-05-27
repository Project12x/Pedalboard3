#include "UiScale.h"

#include <algorithm>
#include <cstdlib>

namespace
{
constexpr std::array<int, 6> supportedUiScalePercents{75, 100, 125, 150, 175, 200};

juce::String extractOverrideValue(const juce::String& commandLine)
{
    const juce::String prefix{"--visual-qa-ui-scale="};
    const auto prefixIndex = commandLine.indexOf(prefix);
    if (prefixIndex < 0)
        return {};

    auto value = commandLine.substring(prefixIndex + prefix.length());
    value = value.upToFirstOccurrenceOf(" ", false, false);
    return value.trimCharactersAtStart("\"'").trimCharactersAtEnd("\"'");
}
} // namespace

namespace UiScale
{
std::array<int, 6> supportedPercents()
{
    return supportedUiScalePercents;
}

bool isSupportedPercent(int percent)
{
    return std::find(supportedUiScalePercents.begin(), supportedUiScalePercents.end(), percent) !=
           supportedUiScalePercents.end();
}

int normalisePercent(int percent)
{
    if (percent <= minimumPercent)
        return minimumPercent;

    if (percent >= maximumPercent)
        return maximumPercent;

    return *std::min_element(supportedUiScalePercents.begin(), supportedUiScalePercents.end(),
                             [percent](int lhs, int rhs) {
                                 return std::abs(lhs - percent) < std::abs(rhs - percent);
                             });
}

float toScaleFactor(int percent)
{
    return static_cast<float>(normalisePercent(percent)) / 100.0f;
}

int footerControlMinimumWidth(int percent)
{
    return juce::roundToInt(static_cast<float>(footerControlBaseMinimumWidth) * toScaleFactor(percent));
}

bool shouldShowFooterControl(int componentWidth, int percent)
{
    return componentWidth >= footerControlMinimumWidth(percent);
}

bool shouldUseSingleRowFooter(int componentWidth, int percent)
{
    return componentWidth >= footerControlMinimumWidth(percent);
}

int footerHeight(int componentWidth, int percent)
{
    return shouldUseSingleRowFooter(componentWidth, percent) ? singleRowFooterHeight : compactFooterHeight;
}

std::optional<int> parseVisualQaOverride(const juce::String& commandLine)
{
    const auto value = extractOverrideValue(commandLine);
    if (value.isEmpty() || value.containsOnly("0123456789") == false)
        return std::nullopt;

    const auto percent = value.getIntValue();
    if (!isSupportedPercent(percent))
        return std::nullopt;

    return percent;
}
} // namespace UiScale
