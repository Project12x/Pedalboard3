/*
  ==============================================================================

    StageLayout.h
    Responsive layout metrics for Stage Mode

  ==============================================================================
*/

#pragma once

#include "JuceHeader.h"

namespace StageLayout
{
struct Metrics
{
    int headerHeight = 0;
    int footerHeight = 0;
    int tunerHeight = 0;
    int patchAreaMinHeight = 0;

    int margin = 0;
    int utilityButtonWidth = 0;
    int utilityButtonHeight = 0;
    int navButtonWidth = 0;
    int navButtonHeight = 0;
    int panicButtonWidth = 0;

    float patchNameFontHeight = 0.0f;
    float nextPatchFontHeight = 0.0f;
    float positionFontHeight = 0.0f;
    float statusFontHeight = 0.0f;
    float timeFontHeight = 0.0f;
    float tunerNoteFontHeight = 0.0f;
    float tunerCentsFontHeight = 0.0f;
    float tunerWaitingFontHeight = 0.0f;

    float meterLabelWidth = 0.0f;
    float meterWidth = 0.0f;
    float meterHeight = 0.0f;
    float meterSpacing = 0.0f;
    float meterStartX = 0.0f;
    float meterChannelGap = 0.0f;
    float sliderHeight = 0.0f;

    float tunerBarWidth = 0.0f;
    float tunerBarHeight = 0.0f;

    int patchNameMaxChars = 0;
    int nextPatchMaxChars = 0;
};

Metrics calculateMetrics(int width, int height, bool showTuner);
juce::String elideLabel(const juce::String& text, int maxChars);
} // namespace StageLayout
