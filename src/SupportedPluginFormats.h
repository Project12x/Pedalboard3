#pragma once
#include <JuceHeader.h>
#include <functional>

enum class PluginFormatRole { application, scanner };
using InternalFormatRegistrar = std::function<void(juce::AudioPluginFormatManager&)>;
void registerSupportedPluginFormats(juce::AudioPluginFormatManager&, PluginFormatRole,
                                    InternalFormatRegistrar = {});
juce::StringArray getRegisteredFormatNames(const juce::AudioPluginFormatManager&);
