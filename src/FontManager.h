//  FontManager.h - Singleton for custom font management
//  ----------------------------------------------------------------------------
//  This file is part of Pedalboard3, an audio plugin host.
//  Copyright (c) 2024 Pedalboard3 Project.
//
//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//  ----------------------------------------------------------------------------

#ifndef FONTMANAGER_H_
#define FONTMANAGER_H_

#include <JuceHeader.h>

/// @brief Singleton managing custom embedded fonts (Space Grotesk + JetBrains Mono)
class FontManager
{
  public:
    /// Returns the singleton instance.
    static FontManager& getInstance();

    // -- Semantic typography API (preferred) --

    /// @brief Panel/window titles, major sections (Inter Bold 18px)
    Font getHeadingFont() const;

    /// @brief Section headers, dialog group labels (Inter Bold 15px)
    Font getSubheadingFont() const;

    /// @brief Default text, descriptions, search boxes (Inter Regular 13px)
    Font getBodyFont() const;

    /// @brief Emphasized body, list primary text (Inter Bold 13px)
    Font getBodyBoldFont() const;

    /// @brief Form labels, detail keys, knob labels (Inter Regular 12px)
    Font getLabelFont() const;

    /// @brief Status bars, secondary list text, hints (Inter Regular 11px)
    Font getCaptionFont() const;

    /// @brief Badges, tags, tiny indicators (Inter Bold 9px)
    Font getBadgeFont() const;

    /// @brief Large display text, hero numbers (Inter Bold, caller specifies size)
    Font getDisplayFont(float height) const;

    /// @brief Large mono display (metronome digits etc.), caller specifies size
    Font getMonoDisplayFont(float height) const;

    // -- Low-level API (use semantic methods above when possible) --

    /// @brief Get the main UI font with explicit size
    /// @param height Font height in pixels
    /// @param bold Whether to use bold weight
    Font getUIFont(float height, bool bold = false) const;

    /// @brief Get the monospace font for numbers/code (JetBrains Mono)
    /// @param height Font height in pixels
    Font getMonoFont(float height) const;

    /// @brief Check if custom fonts loaded successfully
    bool areFontsAvailable() const { return fontsLoaded; }

  private:
    FontManager();
    ~FontManager() = default;

    // Prevent copying
    FontManager(const FontManager&) = delete;
    FontManager& operator=(const FontManager&) = delete;

    Typeface::Ptr spaceGroteskTypeface;
    Typeface::Ptr spaceGroteskBoldTypeface;
    Typeface::Ptr ibmPlexSansTypeface;
    Typeface::Ptr ibmPlexSansBoldTypeface;
    Typeface::Ptr interTypeface;
    Typeface::Ptr interBoldTypeface;
    Typeface::Ptr jetBrainsMonoTypeface;
    bool fontsLoaded = false;
};

#endif
