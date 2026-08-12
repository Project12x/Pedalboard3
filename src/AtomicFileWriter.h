#pragma once

#include <JuceHeader.h>

#include <functional>

using AtomicTextWriter = std::function<bool(const juce::File&, juce::StringRef)>;

/** Replaces a text file only after its complete replacement has been written. */
bool replaceTextAtomically(const juce::File& target, juce::StringRef text);

/**
 * Replaces a text file using a supplied temporary-file writer.
 * This overload lets callers test write failures without risking the target.
 */
bool replaceTextAtomically(const juce::File& target, juce::StringRef text, const AtomicTextWriter& writeTemporary);
