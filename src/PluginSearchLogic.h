//  PluginSearchLogic.h - Search matching helpers for plugin selection
//  ----------------------------------------------------------------------------
//  This file is part of Pedalboard3, an audio plugin host.
//  Copyright (c) 2026 Pedalboard3 Project.
//  ----------------------------------------------------------------------------

#ifndef PLUGINSEARCHLOGIC_H_
#define PLUGINSEARCHLOGIC_H_

#include <JuceHeader.h>

namespace PluginSearchLogic
{

enum class Category
{
    All,
    Effects,
    Instruments,
    Internal
};

inline int fuzzyScore(const juce::String& query, const juce::String& target)
{
    juce::String lowerQuery = query.trim().toLowerCase();
    juce::String lowerTarget = target.toLowerCase();

    if (lowerQuery.isEmpty() || lowerTarget.isEmpty())
        return 0;

    if (lowerTarget == lowerQuery)
        return 1000;

    if (lowerTarget.startsWith(lowerQuery))
        return 800;

    if (lowerTarget.contains(lowerQuery))
        return 600;

    {
        juce::String initials;
        bool nextIsStart = true;
        for (auto ch : lowerTarget)
        {
            if (ch == ' ' || ch == '-' || ch == '_')
            {
                nextIsStart = true;
            }
            else if (nextIsStart)
            {
                initials += ch;
                nextIsStart = false;
            }
        }

        if (initials.startsWith(lowerQuery))
            return 500;
        if (initials.contains(lowerQuery))
            return 400;
    }

    {
        int qi = 0;
        int matched = 0;
        for (int ti = 0; ti < lowerTarget.length() && qi < lowerQuery.length(); ++ti)
        {
            if (lowerTarget[ti] == lowerQuery[qi])
            {
                ++qi;
                ++matched;
            }
        }

        if (qi == lowerQuery.length())
        {
            int density = (matched * 100) / lowerTarget.length();
            return 100 + density;
        }
    }

    return 0;
}

inline int scorePlugin(const juce::String& query, const juce::PluginDescription& type)
{
    juce::String normalizedQuery = query.trim().toLowerCase();
    if (normalizedQuery.isEmpty())
        return 100;

    return juce::jmax(fuzzyScore(normalizedQuery, type.name), fuzzyScore(normalizedQuery, type.manufacturerName));
}

inline bool matchesCategory(const juce::PluginDescription& type, Category category)
{
    switch (category)
    {
    case Category::All:
        return true;

    case Category::Effects:
        return type.pluginFormatName != "Internal" && type.category != "Built-in" && !type.isInstrument;

    case Category::Instruments:
        return type.isInstrument;

    case Category::Internal:
        return type.pluginFormatName == "Internal" || type.category == "Built-in";
    }

    return true;
}

} // namespace PluginSearchLogic

#endif
