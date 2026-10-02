#pragma once
#include <JuceHeader.h>
#include <functional>

using ScratchFileReplace = std::function<bool(const juce::File& temporary, const juce::File& target)>;

// Control/worker-only. Never remove the old target before replacement succeeds.
juce::Result writeScratchTextAtomically(const juce::File& target, const juce::String& text,
                                      const ScratchFileReplace& replace = {});
