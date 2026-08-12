#include "StartupOptions.h"

namespace pedalboard3
{
StartupOptions StartupOptions::parse(const juce::String& commandLine)
{
    StartupOptions options;
    options.startupSmokeTest = commandLine.contains("--startup-smoke-test");
    return options;
}
}
