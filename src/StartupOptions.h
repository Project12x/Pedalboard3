#pragma once

#include <JuceHeader.h>

namespace pedalboard3
{
struct StartupOptions
{
    bool startupSmokeTest = false;
    static StartupOptions parse(const juce::String& commandLine);
};
}
