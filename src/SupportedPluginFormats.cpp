#include "SupportedPluginFormats.h"

juce::StringArray getRegisteredFormatNames(const juce::AudioPluginFormatManager& manager)
{
    juce::StringArray names;

    for (int index = 0; index < manager.getNumFormats(); ++index)
        names.add(manager.getFormat(index)->getName());

    return names;
}

void registerSupportedPluginFormats(juce::AudioPluginFormatManager& manager, PluginFormatRole role,
                                    InternalFormatRegistrar registerInternal)
{
    auto hasFormatNamed = [&] (const juce::String& name) { return getRegisteredFormatNames(manager).contains(name); };

    if (role == PluginFormatRole::application && registerInternal && ! hasFormatNamed("Internal"))
        registerInternal(manager);

#if JUCE_PLUGINHOST_VST3
    if (! hasFormatNamed("VST3"))
        manager.addFormat(new juce::VST3PluginFormat());
#endif

#if JUCE_PLUGINHOST_AU && JUCE_MAC
    if (! hasFormatNamed("AudioUnit"))
        manager.addFormat(new juce::AudioUnitPluginFormat());
#endif
}
