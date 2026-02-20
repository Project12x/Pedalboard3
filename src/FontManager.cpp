//  FontManager.cpp - Singleton for custom font management
//  ----------------------------------------------------------------------------
//  This file is part of Pedalboard3, an audio plugin host.
//  Copyright (c) 2024 Pedalboard3 Project.
//
//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//  ----------------------------------------------------------------------------

#include "FontManager.h"

#include "FontData.h"

//------------------------------------------------------------------------------
FontManager& FontManager::getInstance()
{
    static FontManager instance;
    return instance;
}

//------------------------------------------------------------------------------
FontManager::FontManager()
{
    // Load Space Grotesk Regular + Bold
    spaceGroteskTypeface =
        Typeface::createSystemTypefaceFor(FontData::SpaceGroteskRegular_ttf, FontData::SpaceGroteskRegular_ttfSize);
    spaceGroteskBoldTypeface =
        Typeface::createSystemTypefaceFor(FontData::SpaceGroteskBold_ttf, FontData::SpaceGroteskBold_ttfSize);

    // Load IBM Plex Sans Regular + Bold
    ibmPlexSansTypeface =
        Typeface::createSystemTypefaceFor(FontData::IBMPlexSansRegular_ttf, FontData::IBMPlexSansRegular_ttfSize);
    ibmPlexSansBoldTypeface =
        Typeface::createSystemTypefaceFor(FontData::IBMPlexSansBold_ttf, FontData::IBMPlexSansBold_ttfSize);

    // Load Inter Regular + Bold
    interTypeface = Typeface::createSystemTypefaceFor(FontData::InterRegular_ttf, FontData::InterRegular_ttfSize);
    interBoldTypeface = Typeface::createSystemTypefaceFor(FontData::InterBold_ttf, FontData::InterBold_ttfSize);

    // Load JetBrains Mono (numbers/code)
    jetBrainsMonoTypeface =
        Typeface::createSystemTypefaceFor(FontData::JetBrainsMonoRegular_ttf, FontData::JetBrainsMonoRegular_ttfSize);

    fontsLoaded = (interTypeface != nullptr && interBoldTypeface != nullptr && jetBrainsMonoTypeface != nullptr);
}

//------------------------------------------------------------------------------
// Semantic typography API
//------------------------------------------------------------------------------

Font FontManager::getHeadingFont() const
{
    return getUIFont(18.0f, true);
}
Font FontManager::getSubheadingFont() const
{
    return getUIFont(15.0f, true);
}
Font FontManager::getBodyFont() const
{
    return getUIFont(13.0f, false);
}
Font FontManager::getBodyBoldFont() const
{
    return getUIFont(13.0f, true);
}
Font FontManager::getLabelFont() const
{
    return getUIFont(12.0f, false);
}
Font FontManager::getCaptionFont() const
{
    return getUIFont(11.0f, false);
}
Font FontManager::getBadgeFont() const
{
    return getUIFont(9.0f, true);
}
Font FontManager::getDisplayFont(float height) const
{
    return getUIFont(height, true);
}
Font FontManager::getMonoDisplayFont(float height) const
{
    return getMonoFont(height);
}

//------------------------------------------------------------------------------
// Low-level API
//------------------------------------------------------------------------------

Font FontManager::getUIFont(float height, bool bold) const
{
    // Currently using Inter; swap to ibmPlexSans* or spaceGrotesk* to revert
    Typeface::Ptr face = bold ? interBoldTypeface : interTypeface;

    if (face != nullptr)
    {
        FontOptions options;
        options = options.withTypeface(face);
        options = options.withHeight(height);
        return Font(options);
    }
    // Fallback to system font
    return Font(FontOptions().withHeight(height));
}

//------------------------------------------------------------------------------
Font FontManager::getMonoFont(float height) const
{
    if (jetBrainsMonoTypeface != nullptr)
    {
        FontOptions options;
        options = options.withTypeface(jetBrainsMonoTypeface);
        options = options.withHeight(height);
        return Font(options);
    }
    // Fallback to system monospace
    return Font(FontOptions(Font::getDefaultMonospacedFontName(), height, Font::plain));
}
