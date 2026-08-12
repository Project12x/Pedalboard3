#include "AtomicFileWriter.h"

namespace
{
bool writeText(const juce::File& file, juce::StringRef text)
{
    if (auto stream = file.createOutputStream())
    {
        stream->writeText(juce::String(text), false, false, "\n");
        stream->flush();
        return stream->getStatus().wasOk();
    }

    return false;
}
} // namespace

bool replaceTextAtomically(const juce::File& target, juce::StringRef text)
{
    return replaceTextAtomically(target, text, writeText);
}

bool replaceTextAtomically(const juce::File& target, juce::StringRef text, const AtomicTextWriter& writeTemporary)
{
    if (target == juce::File() || ! target.getParentDirectory().createDirectory())
        return false;

    juce::TemporaryFile temporary(target);
    if (! writeTemporary(temporary.getFile(), text))
        return false;

    return temporary.overwriteTargetFileWithTemporary();
}
