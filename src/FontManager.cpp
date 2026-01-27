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
    // Load Space Grotesk (main UI font)
    spaceGroteskTypeface =
        Typeface::createSystemTypefaceFor(FontData::SpaceGroteskRegular_ttf, FontData::SpaceGroteskRegular_ttfSize);

    // Load JetBrains Mono (numbers/code)
    jetBrainsMonoTypeface =
        Typeface::createSystemTypefaceFor(FontData::JetBrainsMonoRegular_ttf, FontData::JetBrainsMonoRegular_ttfSize);

    fontsLoaded = (spaceGroteskTypeface != nullptr && jetBrainsMonoTypeface != nullptr);
}

//------------------------------------------------------------------------------
Font FontManager::getUIFont(float height, bool bold) const
{
    if (spaceGroteskTypeface != nullptr)
    {
        FontOptions options;
        options = options.withTypeface(spaceGroteskTypeface);
        options = options.withHeight(height);
        if (bold)
            options = options.withStyle("Bold");
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
