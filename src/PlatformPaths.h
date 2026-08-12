#pragma once

#include <JuceHeader.h>

namespace pedalboard3::platform
{
juce::File userDataDirectoryForBase(const juce::File& applicationDataBase, bool macOS);
juce::File getUserDataDirectory();
juce::File documentationDirectoryForApplication(const juce::File& currentApplication, bool macOSBundle);
juce::File getDocumentationDirectory();
}
