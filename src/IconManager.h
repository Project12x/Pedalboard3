//	IconManager.h - Combined FontAudio + Lucide icons for Pedalboard.
//	----------------------------------------------------------------------------
//	This file is part of Pedalboard3, an audio plugin host.
//	Copyright (c) 2024 Niall Moody.
//
//	FontAudio icons: https://github.com/fefanto/fontaudio (CC BY 4.0)
//	Lucide icons: https://lucide.dev (ISC License)
//
//	This program is free software: you can redistribute it and/or modify
//	it under the terms of the GNU General Public License as published by
//	the Free Software Foundation, either version 3 of the License, or
//	(at your option) any later version.
//	----------------------------------------------------------------------------

#ifndef ICONMANAGER_H_
#define ICONMANAGER_H_

#include "JuceHeader.h"

/**
 * Provides icons from FontAudio (audio-specific) and Lucide (general UI).
 * Use IconManager::getInstance() to access icons.
 */
class IconManager
{
  public:
    static IconManager& getInstance();

    // === FontAudio: Transport Controls ===
    std::unique_ptr<Drawable> getPlayIcon(Colour colour = Colours::white);
    std::unique_ptr<Drawable> getPauseIcon(Colour colour = Colours::white);
    std::unique_ptr<Drawable> getStopIcon(Colour colour = Colours::white);
    std::unique_ptr<Drawable> getRecordIcon(Colour colour = Colours::white);
    std::unique_ptr<Drawable> getBackwardIcon(Colour colour = Colours::white);
    std::unique_ptr<Drawable> getLoopIcon(Colour colour = Colours::white);
    std::unique_ptr<Drawable> getMetronomeIcon(Colour colour = Colours::white);

    // === Lucide: File Operations ===
    std::unique_ptr<Drawable> getSaveIcon(Colour colour = Colours::white);
    std::unique_ptr<Drawable> getNewFileIcon(Colour colour = Colours::white);
    std::unique_ptr<Drawable> getOpenFolderIcon(Colour colour = Colours::white);
    std::unique_ptr<Drawable> getTrashIcon(Colour colour = Colours::white);

    // === Lucide: UI Controls ===
    std::unique_ptr<Drawable> getCloseIcon(Colour colour = Colours::white);
    std::unique_ptr<Drawable> getPlusIcon(Colour colour = Colours::white);
    std::unique_ptr<Drawable> getSearchIcon(Colour colour = Colours::white);
    std::unique_ptr<Drawable> getCheckIcon(Colour colour = Colours::white);
    std::unique_ptr<Drawable> getDownloadIcon(Colour colour = Colours::white);
    std::unique_ptr<Drawable> getRefreshIcon(Colour colour = Colours::white);

  private:
    IconManager() = default;
    ~IconManager() = default;

    std::unique_ptr<Drawable> createFromSvg(const String& svgData, Colour colour);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(IconManager)
};

#endif // ICONMANAGER_H_
