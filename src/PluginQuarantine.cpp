#include "PluginQuarantine.h"

#include "AtomicFileWriter.h"

#include <algorithm>

namespace
{
juce::String reasonToString(PluginQuarantineReason reason)
{
    switch (reason)
    {
        case PluginQuarantineReason::crash: return "crash";
        case PluginQuarantineReason::timeout: return "timeout";
        case PluginQuarantineReason::malformedResponse: return "malformedResponse";
        case PluginQuarantineReason::loadFailure: return "loadFailure";
    }

    return "loadFailure";
}

PluginQuarantineReason reasonFromString(const juce::String& reason)
{
    if (reason == "crash") return PluginQuarantineReason::crash;
    if (reason == "timeout") return PluginQuarantineReason::timeout;
    if (reason == "malformedResponse") return PluginQuarantineReason::malformedResponse;
    return PluginQuarantineReason::loadFailure;
}
} // namespace

PluginQuarantine::PluginQuarantine(juce::File persistenceFile) : file(std::move(persistenceFile))
{
    load();
}

bool PluginQuarantine::contains(juce::StringRef formatName, juce::StringRef identifier) const
{
    std::lock_guard<std::mutex> lock(mutex);
    return std::any_of(storedEntries.begin(), storedEntries.end(), [&] (const auto& entry) {
        return entry.formatName == formatName && entry.identifier == identifier;
    });
}

bool PluginQuarantine::record(const PluginQuarantineEntry& entry)
{
    std::lock_guard<std::mutex> lock(mutex);
    const auto existing = std::find_if(storedEntries.begin(), storedEntries.end(), [&] (const auto& current) {
        return current.formatName == entry.formatName && current.identifier == entry.identifier;
    });

    if (existing == storedEntries.end())
        storedEntries.push_back(entry);
    else
        *existing = entry;

    return saveLocked();
}

bool PluginQuarantine::remove(juce::StringRef formatName, juce::StringRef identifier)
{
    std::lock_guard<std::mutex> lock(mutex);
    const auto previousSize = storedEntries.size();
    storedEntries.erase(std::remove_if(storedEntries.begin(), storedEntries.end(), [&] (const auto& entry) {
                            return entry.formatName == formatName && entry.identifier == identifier;
                        }),
                        storedEntries.end());

    return previousSize == storedEntries.size() || saveLocked();
}

bool PluginQuarantine::clear()
{
    std::lock_guard<std::mutex> lock(mutex);
    storedEntries.clear();
    return saveLocked();
}

std::vector<PluginQuarantineEntry> PluginQuarantine::entries() const
{
    std::lock_guard<std::mutex> lock(mutex);
    return storedEntries;
}

void PluginQuarantine::load()
{
    std::lock_guard<std::mutex> lock(mutex);
    storedEntries.clear();

    if (! file.existsAsFile())
        return;

    const auto document = juce::XmlDocument::parse(file);
    if (document == nullptr || ! document->hasTagName("PLUGIN_QUARANTINE"))
        return;

    forEachXmlChildElementWithTagName(*document, entry, "ENTRY")
    {
        storedEntries.push_back({ entry->getStringAttribute("format"),
                                  entry->getStringAttribute("identifier"),
                                  reasonFromString(entry->getStringAttribute("reason")),
                                  juce::Time(entry->getStringAttribute("timestamp").getLargeIntValue()),
                                  entry->getStringAttribute("detail") });
    }
}

bool PluginQuarantine::saveLocked() const
{
    auto root = std::make_unique<juce::XmlElement>("PLUGIN_QUARANTINE");
    root->setAttribute("version", 1);

    for (const auto& entry : storedEntries)
    {
        auto* element = root->createNewChildElement("ENTRY");
        element->setAttribute("format", entry.formatName);
        element->setAttribute("identifier", entry.identifier);
        element->setAttribute("reason", reasonToString(entry.reason));
        element->setAttribute("timestamp", juce::String(entry.timestamp.toMilliseconds()));
        element->setAttribute("detail", entry.detail);
    }

    return replaceTextAtomically(file, root->toString());
}
