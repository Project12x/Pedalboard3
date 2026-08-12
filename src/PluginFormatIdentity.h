#pragma once

#include <JuceHeader.h>

/**
 * Returns a cross-platform persistent plugin key that distinguishes plug-ins
 * with the same JUCE identifier when they come from different formats.
 */
juce::String makeExactPluginKey(const juce::PluginDescription& description);

/**
 * Returns the short UI label for a registered plugin format.
 */
juce::String pluginFormatBadge(const juce::String& formatName);
