#include "PluginFormatIdentity.h"

juce::String makeExactPluginKey(const juce::PluginDescription& description)
{
    return description.pluginFormatName + "|" + description.createIdentifierString();
}

juce::String pluginFormatBadge(const juce::String& formatName)
{
    return formatName == "AudioUnit" ? "AU" : formatName;
}
