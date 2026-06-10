/*
  ==============================================================================

    StageLayout.cpp
    Responsive layout metrics for Stage Mode

  ==============================================================================
*/

#include "StageLayout.h"

namespace StageLayout
{
Metrics calculateMetrics(int width, int height, bool showTuner)
{
    const auto safeWidth = juce::jmax(1, width);
    const auto safeHeight = juce::jmax(1, height);
    const auto shortEdge = juce::jmin(safeWidth, safeHeight);

    Metrics metrics;
    metrics.margin = juce::roundToInt(juce::jlimit(12.0f, 28.0f, shortEdge * 0.032f));
    metrics.gridSpacing = juce::roundToInt(juce::jlimit(32.0f, 56.0f, shortEdge * 0.065f));

    metrics.headerHeight = juce::roundToInt(juce::jlimit(58.0f, 76.0f, safeHeight * 0.075f));
    metrics.footerHeight = juce::roundToInt(juce::jlimit(86.0f, 116.0f, safeHeight * 0.105f));
    metrics.tunerHeight = showTuner ? juce::roundToInt(juce::jlimit(118.0f, 198.0f, safeHeight * 0.19f)) : 0;
    metrics.patchAreaMinHeight =
        juce::jmax(0, safeHeight - metrics.headerHeight - metrics.footerHeight - metrics.tunerHeight);

    metrics.patchNameFontHeight = juce::jlimit(42.0f, 86.0f, juce::jmin(safeWidth * 0.055f, safeHeight * 0.13f));
    metrics.nextPatchFontHeight = juce::jlimit(22.0f, 38.0f, metrics.patchNameFontHeight * 0.45f);
    metrics.positionFontHeight = juce::jlimit(16.0f, 28.0f, shortEdge * 0.035f);
    metrics.statusFontHeight = juce::jlimit(13.0f, 18.0f, shortEdge * 0.024f);
    metrics.timeFontHeight = juce::jlimit(12.0f, 16.0f, shortEdge * 0.022f);
    metrics.modeChipFontHeight = juce::jlimit(12.0f, 16.0f, shortEdge * 0.021f);
    metrics.eyebrowFontHeight = juce::jlimit(13.0f, 18.0f, shortEdge * 0.024f);
    metrics.safetyFontHeight = juce::jlimit(11.0f, 14.0f, shortEdge * 0.018f);
    metrics.tunerNoteFontHeight = juce::jlimit(42.0f, 72.0f, shortEdge * 0.095f);
    metrics.tunerCentsFontHeight = juce::jlimit(20.0f, 32.0f, shortEdge * 0.045f);
    metrics.tunerWaitingFontHeight = juce::jlimit(24.0f, 36.0f, shortEdge * 0.045f);
    metrics.queueLabelFontHeight = juce::jlimit(11.0f, 14.0f, shortEdge * 0.018f);
    metrics.queueTitleFontHeight = juce::jlimit(12.0f, 15.0f, shortEdge * 0.019f);

    metrics.topBarChipWidth = juce::roundToInt(juce::jlimit(128.0f, 188.0f, safeWidth * 0.095f));
    metrics.topBarChipHeight = juce::roundToInt(juce::jlimit(34.0f, 44.0f, safeHeight * 0.043f));
    metrics.utilityButtonWidth = juce::roundToInt(juce::jlimit(104.0f, 148.0f, safeWidth * 0.07f));
    metrics.utilityButtonHeight = juce::roundToInt(juce::jlimit(36.0f, 48.0f, safeHeight * 0.047f));
    metrics.navButtonWidth = juce::roundToInt(juce::jlimit(104.0f, 168.0f, safeWidth * 0.082f));
    metrics.navButtonHeight = juce::roundToInt(juce::jlimit(52.0f, 72.0f, safeHeight * 0.07f));
    metrics.panicButtonWidth = juce::roundToInt(juce::jlimit(136.0f, 192.0f, safeWidth * 0.085f));
    metrics.panicButtonHeight = juce::roundToInt(juce::jlimit(50.0f, 64.0f, safeHeight * 0.058f));
    metrics.heroHorizontalInset =
        juce::roundToInt(juce::jlimit(170.0f, 360.0f, safeWidth * 0.17f));
    metrics.heroEyebrowHeight = juce::roundToInt(juce::jlimit(30.0f, 42.0f, safeHeight * 0.04f));
    metrics.heroNextCueHeight = juce::roundToInt(juce::jlimit(42.0f, 58.0f, safeHeight * 0.055f));
    if (safeWidth >= 1120 && safeHeight >= 640)
    {
        metrics.liveQueueRailWidth = juce::roundToInt(juce::jlimit(210.0f, 318.0f, safeWidth * 0.155f));
        metrics.liveQueueHeaderHeight = juce::roundToInt(juce::jlimit(36.0f, 46.0f, safeHeight * 0.045f));
        metrics.liveQueueRowHeight = juce::roundToInt(juce::jlimit(64.0f, 82.0f, safeHeight * 0.074f));
    }

    metrics.meterLabelWidth = juce::jlimit(34.0f, 44.0f, safeWidth * 0.025f);
    metrics.meterWidth = juce::jlimit(128.0f, 210.0f, safeWidth * 0.084f);
    metrics.meterHeight = juce::jlimit(9.0f, 14.0f, safeHeight * 0.012f);
    metrics.meterSpacing = juce::jlimit(22.0f, 40.0f, safeWidth * 0.018f);
    metrics.meterStartX = juce::jlimit(18.0f, 42.0f, safeWidth * 0.02f);
    metrics.meterChannelGap = juce::jlimit(3.0f, 6.0f, safeHeight * 0.004f);
    metrics.meterTopOffset = juce::jlimit(24.0f, 34.0f, metrics.footerHeight * 0.28f);
    metrics.sliderHeight = juce::jlimit(24.0f, 30.0f, safeHeight * 0.032f);
    metrics.sliderTopOffset = juce::jlimit(54.0f, 76.0f, metrics.footerHeight * 0.63f);

    metrics.tunerBarWidth = juce::jlimit(240.0f, 460.0f, safeWidth * 0.42f);
    metrics.tunerBarHeight = juce::jlimit(10.0f, 16.0f, safeHeight * 0.014f);
    metrics.liveDotSize = juce::jlimit(7.0f, 11.0f, shortEdge * 0.012f);
    metrics.progressDotSize = juce::jlimit(8.0f, 13.0f, shortEdge * 0.014f);
    metrics.progressActiveWidth = metrics.progressDotSize * 3.1f;
    metrics.progressDotGap = juce::jlimit(8.0f, 14.0f, shortEdge * 0.014f);

    metrics.patchNameMaxChars = juce::jlimit(18, 36, safeWidth / 42);
    metrics.nextPatchMaxChars = juce::jlimit(18, 44, safeWidth / 36);
    metrics.maxProgressDots = juce::jlimit(6, 14, safeWidth / 120);

    return metrics;
}

bool shouldReserveTunerStrip(bool showTuner, bool tunerFocus, bool patchView)
{
    return showTuner && !tunerFocus && patchView;
}

juce::String elideLabel(const juce::String& text, int maxChars)
{
    if (maxChars <= 0)
        return {};

    if (text.length() <= maxChars)
        return text;

    if (maxChars <= 3)
        return text.substring(0, maxChars);

    auto prefix = text.substring(0, maxChars - 3).trimEnd();
    while (prefix.length() + 3 > maxChars && prefix.isNotEmpty())
        prefix = prefix.dropLastCharacters(1);

    return prefix + "...";
}

juce::String formatBankLabel(int bankIndex)
{
    if (bankIndex >= 0 && bankIndex < 26)
        return "Bank " + juce::String::charToString((juce_wchar)('A' + bankIndex));

    return "Bank " + juce::String(bankIndex + 1);
}

std::vector<int> collectVisibleBankIndices(int activeBank, int totalBanks, int maxVisible)
{
    std::vector<int> banks;
    if (totalBanks <= 0 || maxVisible <= 0)
        return banks;

    const int visibleCount = juce::jlimit(1, totalBanks, maxVisible);
    const int clampedActive = juce::jlimit(0, totalBanks - 1, activeBank);
    const int start = juce::jlimit(0, juce::jmax(0, totalBanks - visibleCount), clampedActive - visibleCount / 2);

    banks.reserve((size_t)visibleCount);
    for (int i = 0; i < visibleCount; ++i)
        banks.push_back(start + i);

    return banks;
}
} // namespace StageLayout
