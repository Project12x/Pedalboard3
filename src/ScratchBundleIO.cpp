#include "ScratchBundleIO.h"

juce::Result writeScratchTextAtomically(const juce::File& target, const juce::String& text,
                                      const ScratchFileReplace& replace)
{
    if (target == juce::File() || target.isSymbolicLink() || target.getParentDirectory().isSymbolicLink())
        return juce::Result::fail("Unsafe scratch metadata destination");

    // JUCE TemporaryFile owns sibling cleanup; replaceFileIn uses ReplaceFile on
    // Windows and rename on POSIX. This is replacement safety, not a power-loss guarantee.
    juce::TemporaryFile temporary(target);
    {
        juce::FileOutputStream stream(temporary.getFile(), 16);
        const auto bytes = static_cast<size_t>(text.getNumBytesAsUTF8());
        if (!stream.openedOk() || !stream.write(text.toRawUTF8(), bytes))
            return juce::Result::fail("Could not write scratch metadata temporary file");
        stream.flush();
        if (stream.getStatus().failed() || stream.getPosition() != static_cast<juce::int64>(bytes))
            return juce::Result::fail("Could not flush scratch metadata");
    }

    const bool replaced = replace ? replace(temporary.getFile(), target)
                                  : temporary.getFile().replaceFileIn(target);
    return replaced ? juce::Result::ok() : juce::Result::fail("Could not replace scratch metadata");
}
