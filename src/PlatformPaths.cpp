#include "PlatformPaths.h"

namespace pedalboard3::platform
{
juce::File userDataDirectoryForBase(const juce::File& applicationDataBase, bool macOS)
{
    auto root = applicationDataBase;
    if (macOS) root = root.getChildFile("Application Support");
    return root.getChildFile("Pedalboard3");
}
juce::File getUserDataDirectory()
{
    const auto base = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory);
#if JUCE_MAC
    return userDataDirectoryForBase(base, true);
#else
    return userDataDirectoryForBase(base, false);
#endif
}
juce::File documentationDirectoryForApplication(const juce::File& currentApplication, bool macOSBundle)
{
    if (macOSBundle) return currentApplication.getChildFile("Contents").getChildFile("Resources").getChildFile("documentation");
    return currentApplication.getParentDirectory().getChildFile("documentation");
}
juce::File getDocumentationDirectory()
{
    const auto application = juce::File::getSpecialLocation(juce::File::currentApplicationFile);
#if JUCE_MAC
    return documentationDirectoryForApplication(application, true);
#else
    return documentationDirectoryForApplication(application, false);
#endif
}
}
