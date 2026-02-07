//	IconManager.cpp - Combined FontAudio + Lucide icons for Pedalboard.
//	----------------------------------------------------------------------------
//	This file is part of Pedalboard3, an audio plugin host.
//	Copyright (c) 2024 Niall Moody.
//
//	FontAudio icons: https://github.com/fefanto/fontaudio (CC BY 4.0)
//	Lucide icons: https://lucide.dev (ISC License)
//	----------------------------------------------------------------------------

#include "IconManager.h"

//------------------------------------------------------------------------------
// FontAudio SVGs (transport/audio controls)
//------------------------------------------------------------------------------
static const char* fad_play_svg = R"SVG(
<svg viewBox="0 0 256 256" xmlns="http://www.w3.org/2000/svg">
    <path d="M59 61.922c0-9.768 13.016-15.432 22.352-11.615 10.695 7.017 101.643 58.238 109.869 65.076 8.226 6.838 10.585 17.695-.559 25.77-11.143 8.074-99.712 60.203-109.31 64.73-9.6 4.526-21.952-1.632-22.352-13.088-.4-11.456 0-121.106 0-130.873zm13.437 8.48c0 2.494-.076 112.852-.216 115.122-.23 3.723 3 7.464 7.5 5.245 4.5-2.22 97.522-57.704 101.216-59.141 3.695-1.438 3.45-5.1 0-7.388C177.488 121.952 82.77 67.76 80 65.38c-2.77-2.381-7.563 1.193-7.563 5.023z" fill-rule="evenodd"/>
</svg>
)SVG";

static const char* fad_pause_svg = R"SVG(
<svg viewBox="0 0 256 256" xmlns="http://www.w3.org/2000/svg">
    <path d="M46.677 64.652c0-9.362 7.132-17.387 16.447-17.394 9.315-.007 24.677.007 34.55.007 9.875 0 17.138 7.594 17.138 16.998 0 9.403-.083 119.094-.083 127.82 0 8.726-7.58 16.895-16.554 16.837-8.975-.058-25.349.115-34.963.058-9.614-.058-16.646-7.74-16.646-17.254 0-9.515.11-117.71.11-127.072zm14.759.818s-.09 118.144-.09 123.691c0 5.547 3.124 5.315 6.481 5.832 3.358.518 21.454.47 24.402.47 2.947 0 7.085-1.658 7.167-6.14.08-4.483-.082-119.507-.082-123.249 0-3.742-4.299-4.264-7.085-4.66-2.787-.395-25.796 0-25.796 0l-4.997 4.056zm76.664-.793c.027-9.804 7.518-17.541 17.125-17.689 9.606-.147 25.283.148 35.004.148 9.72 0 17.397 8.52 17.397 17.77s-.178 117.809-.178 127c0 9.192-7.664 17.12-16.323 17.072-8.66-.05-26.354 0-34.991.048-8.638.05-17.98-8.582-18.007-17.783-.027-9.201-.055-116.763-.027-126.566zm16.917.554s-.089 118.145-.089 123.692c0 5.547 3.123 5.314 6.48 5.832 3.359.518 21.455.47 24.402.47 2.948 0 7.086-1.659 7.167-6.141.081-4.482-.08-119.506-.08-123.248 0-3.742-4.3-4.265-7.087-4.66-2.786-.396-25.796 0-25.796 0l-4.997 4.055z" fill-rule="evenodd"/>
</svg>
)SVG";

static const char* fad_stop_svg = R"SVG(
<svg viewBox="0 0 256 256" xmlns="http://www.w3.org/2000/svg">
    <path d="M48.227 65.473c0-9.183 7.096-16.997 16.762-17.51 9.666-.513 116.887-.487 125.094-.487 8.207 0 17.917 9.212 17.917 17.71 0 8.499.98 117.936.49 126.609-.49 8.673-9.635 15.995-17.011 15.995-7.377 0-117.127-.327-126.341-.327-9.214 0-17.472-7.793-17.192-16.1.28-8.306.28-116.708.28-125.89zm15.951 4.684c-.153 3.953 0 112.665 0 116.19 0 3.524 3.115 5.959 7.236 6.156 4.12.198 112.165.288 114.852 0 2.686-.287 5.811-2.073 5.932-5.456.12-3.383-.609-113.865-.609-116.89 0-3.025-3.358-5.84-6.02-5.924-2.662-.085-110.503 0-114.155 0-3.652 0-7.083 1.972-7.236 5.924z" fill-rule="evenodd"/>
</svg>
)SVG";

static const char* fad_record_svg = R"SVG(
<svg viewBox="0 0 256 256" xmlns="http://www.w3.org/2000/svg">
    <path d="M128 209c-44.735 0-81-36.265-81-81s36.265-81 81-81 81 36.265 81 81-36.265 81-81 81zm.5-33c26.51 0 48-21.49 48-48s-21.49-48-48-48-48 21.49-48 48 21.49 48 48 48z" fill-rule="evenodd"/>
</svg>
)SVG";

static const char* fad_backward_svg = R"SVG(
<svg viewBox="0 0 256 256" xmlns="http://www.w3.org/2000/svg">
    <path d="M117.027 60.858c0 14.184.118 120.734.118 134.66 0 13.925-7.798 16.307-14.543 10.568-6.745-5.74-46.666-60.003-52.014-67.201-5.349-7.198-4.45-12.951.086-20.28 4.535-7.328 47.284-62.224 52.46-68.2 5.175-5.977 13.893-3.73 13.893 10.453zM65.424 132.65l32.874 43.167c1.336 1.754 2.425 1.393 2.424-.814l-.005-92.006c0-2.197-1.072-2.562-2.395-.792l-32.927 44.057c-1.327 1.776-1.31 4.63.03 6.388zm142.603-71.792c0 14.184.118 120.734.118 134.66 0 13.925-7.798 16.307-14.543 10.568-6.745-5.74-46.666-60.003-52.014-67.201-5.349-7.198-4.45-12.951.086-20.28 4.535-7.328 47.284-62.224 52.46-68.2 5.175-5.977 13.893-3.73 13.893 10.453zm-51.603 71.792l32.874 43.167c1.336 1.754 2.425 1.393 2.424-.814l-.005-92.006c0-2.197-1.072-2.562-2.395-.792l-32.927 44.057c-1.327 1.776-1.31 4.63.03 6.388z" fill-rule="evenodd"/>
</svg>
)SVG";

static const char* fad_loop_svg = R"SVG(
<svg viewBox="0 0 256 256" xmlns="http://www.w3.org/2000/svg">
    <path d="M24.849 72.002a8 8 0 0 1 8.027-7.983l189.787.462c4.42.01 8.004 3.61 8.004 8.026v111.986c0 4.422-3.587 8.04-8.008 8.08l-42.661.39A4.04 4.04 0 0 0 176 197v22.002c0 2.208-1.39 2.878-3.115 1.488l-35.31-28.463c-5.157-4.156-5.11-10.846.099-14.935l35.174-27.616c1.74-1.367 3.152-.685 3.152 1.534v21.482a4 4 0 0 0 3.992 4.009h30.683a4.02 4.02 0 0 0 4.013-3.991l.437-83.012a7.98 7.98 0 0 0-7.952-8.02l-158.807-.454c-4.415-.013-8.008 3.56-8.025 7.975l-.31 79.504a7.962 7.962 0 0 0 7.967 7.998H108a4.003 4.003 0 0 1 3.999 3.994v8.512a3.968 3.968 0 0 1-4.001 3.971l-75.502-.431c-4.417-.026-7.987-3.634-7.974-8.048l.326-112.496z" fill-rule="evenodd"/>
</svg>
)SVG";

static const char* fad_metronome_svg = R"SVG(
<svg viewBox="0 0 256 256" xmlns="http://www.w3.org/2000/svg">
    <g fill-rule="evenodd">
        <path d="M64.458 228.867c-.428 2.167 1.007 3.91 3.226 3.893l121.557-.938c2.21-.017 3.68-1.794 3.284-3.97l-11.838-64.913c-.397-2.175-1.626-2.393-2.747-.487l-9.156 15.582c-1.12 1.907-1.71 5.207-1.313 7.388l4.915 27.03c.395 2.175-1.072 3.937-3.288 3.937H88.611c-2.211 0-3.659-1.755-3.233-3.92L114.85 62.533l28.44-.49 11.786 44.43c.567 2.139 2.01 2.386 3.236.535l8.392-12.67c1.22-1.843 1.73-5.058 1.139-7.185l-9.596-34.5c-1.184-4.257-5.735-7.677-10.138-7.638l-39.391.349c-4.415.039-8.688 3.584-9.544 7.912L64.458 228.867z"/>
        <path d="M118.116 198.935c-1.182 1.865-.347 3.377 1.867 3.377h12.392c2.214 0 4.968-1.524 6.143-3.39l64.55-102.463c1.18-1.871 3.906-3.697 6.076-4.074l9.581-1.667c2.177-.379 4.492-2.38 5.178-4.496l4.772-14.69c.683-2.104-.063-5.034-1.677-6.555L215.53 54.173c-1.609-1.517-4.482-1.862-6.4-.78l-11.799 6.655c-1.925 1.086-3.626 3.754-3.799 5.954l-.938 11.967c-.173 2.202-1.27 5.498-2.453 7.363l-72.026 113.603z"/>
    </g>
</svg>
)SVG";

//------------------------------------------------------------------------------
// Lucide SVGs (file operations & UI controls)
//------------------------------------------------------------------------------
static const char* lucide_save_svg = R"SVG(
<svg viewBox="0 0 24 24" xmlns="http://www.w3.org/2000/svg" fill="none" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
    <path d="M15.2 3a2 2 0 0 1 1.4.6l3.8 3.8a2 2 0 0 1 .6 1.4V19a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2z"/>
    <path d="M17 21v-7a1 1 0 0 0-1-1H8a1 1 0 0 0-1 1v7"/>
    <path d="M7 3v4a1 1 0 0 0 1 1h7"/>
</svg>
)SVG";

static const char* lucide_file_plus_svg = R"SVG(
<svg viewBox="0 0 24 24" xmlns="http://www.w3.org/2000/svg" fill="none" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
    <path d="M6 22a2 2 0 0 1-2-2V4a2 2 0 0 1 2-2h8a2.4 2.4 0 0 1 1.704.706l3.588 3.588A2.4 2.4 0 0 1 20 8v12a2 2 0 0 1-2 2z"/>
    <path d="M14 2v5a1 1 0 0 0 1 1h5"/>
    <path d="M9 15h6"/>
    <path d="M12 18v-6"/>
</svg>
)SVG";

static const char* lucide_folder_open_svg = R"SVG(
<svg viewBox="0 0 24 24" xmlns="http://www.w3.org/2000/svg" fill="none" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
    <path d="m6 14 1.5-2.9A2 2 0 0 1 9.24 10H20a2 2 0 0 1 1.94 2.5l-1.54 6a2 2 0 0 1-1.95 1.5H4a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2h3.9a2 2 0 0 1 1.69.9l.81 1.2a2 2 0 0 0 1.67.9H18a2 2 0 0 1 2 2v2"/>
</svg>
)SVG";

static const char* lucide_trash_svg = R"SVG(
<svg viewBox="0 0 24 24" xmlns="http://www.w3.org/2000/svg" fill="none" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
    <path d="M10 11v6"/>
    <path d="M14 11v6"/>
    <path d="M19 6v14a2 2 0 0 1-2 2H7a2 2 0 0 1-2-2V6"/>
    <path d="M3 6h18"/>
    <path d="M8 6V4a2 2 0 0 1 2-2h4a2 2 0 0 1 2 2v2"/>
</svg>
)SVG";

static const char* lucide_x_svg = R"SVG(
<svg viewBox="0 0 24 24" xmlns="http://www.w3.org/2000/svg" fill="none" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
    <path d="M18 6 6 18"/>
    <path d="m6 6 12 12"/>
</svg>
)SVG";

static const char* lucide_plus_svg = R"SVG(
<svg viewBox="0 0 24 24" xmlns="http://www.w3.org/2000/svg" fill="none" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
    <path d="M5 12h14"/>
    <path d="M12 5v14"/>
</svg>
)SVG";

static const char* lucide_search_svg = R"SVG(
<svg viewBox="0 0 24 24" xmlns="http://www.w3.org/2000/svg" fill="none" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
    <circle cx="11" cy="11" r="8"/>
    <path d="m21 21-4.35-4.35"/>
</svg>
)SVG";

static const char* lucide_check_svg = R"SVG(
<svg viewBox="0 0 24 24" xmlns="http://www.w3.org/2000/svg" fill="none" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
    <path d="M20 6 9 17l-5-5"/>
</svg>
)SVG";

static const char* lucide_download_svg = R"SVG(
<svg viewBox="0 0 24 24" xmlns="http://www.w3.org/2000/svg" fill="none" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
    <path d="M21 15v4a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2v-4"/>
    <polyline points="7 10 12 15 17 10"/>
    <line x1="12" x2="12" y1="15" y2="3"/>
</svg>
)SVG";

static const char* lucide_refresh_svg = R"SVG(
<svg viewBox="0 0 24 24" xmlns="http://www.w3.org/2000/svg" fill="none" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
    <path d="M3 12a9 9 0 0 1 9-9 9.75 9.75 0 0 1 6.74 2.74L21 8"/>
    <path d="M21 3v5h-5"/>
    <path d="M21 12a9 9 0 0 1-9 9 9.75 9.75 0 0 1-6.74-2.74L3 16"/>
    <path d="M8 16H3v5"/>
</svg>
)SVG";

//------------------------------------------------------------------------------
IconManager& IconManager::getInstance()
{
    static IconManager instance;
    return instance;
}

//------------------------------------------------------------------------------
std::unique_ptr<Drawable> IconManager::createFromSvg(const String& svgData, Colour colour)
{
    // Replace stroke and fill with the desired colour
    String colourHex = colour.toDisplayString(false);
    String modifiedSvg = svgData;

    // For FontAudio icons (which use fill)
    if (!modifiedSvg.contains("stroke="))
    {
        // Add fill attribute
        modifiedSvg = modifiedSvg.replace("<path", String("<path fill=\"#") + colourHex + "\"");
        modifiedSvg = modifiedSvg.replace("<g fill-rule", String("<g fill=\"#") + colourHex + "\" fill-rule");
    }
    else
    {
        // For Lucide icons (which use stroke)
        modifiedSvg = modifiedSvg.replace("stroke-width=", String("stroke=\"#") + colourHex + "\" stroke-width=");
    }

    auto drawable = Drawable::createFromSVG(*XmlDocument::parse(modifiedSvg));
    return drawable;
}

//------------------------------------------------------------------------------
// FontAudio Transport Icons
//------------------------------------------------------------------------------
std::unique_ptr<Drawable> IconManager::getPlayIcon(Colour colour)
{
    return createFromSvg(fad_play_svg, colour);
}

std::unique_ptr<Drawable> IconManager::getPauseIcon(Colour colour)
{
    return createFromSvg(fad_pause_svg, colour);
}

std::unique_ptr<Drawable> IconManager::getStopIcon(Colour colour)
{
    return createFromSvg(fad_stop_svg, colour);
}

std::unique_ptr<Drawable> IconManager::getRecordIcon(Colour colour)
{
    return createFromSvg(fad_record_svg, colour);
}

std::unique_ptr<Drawable> IconManager::getBackwardIcon(Colour colour)
{
    return createFromSvg(fad_backward_svg, colour);
}

std::unique_ptr<Drawable> IconManager::getLoopIcon(Colour colour)
{
    return createFromSvg(fad_loop_svg, colour);
}

std::unique_ptr<Drawable> IconManager::getMetronomeIcon(Colour colour)
{
    return createFromSvg(fad_metronome_svg, colour);
}

//------------------------------------------------------------------------------
// Lucide File/UI Icons
//------------------------------------------------------------------------------
std::unique_ptr<Drawable> IconManager::getSaveIcon(Colour colour)
{
    return createFromSvg(lucide_save_svg, colour);
}

std::unique_ptr<Drawable> IconManager::getNewFileIcon(Colour colour)
{
    return createFromSvg(lucide_file_plus_svg, colour);
}

std::unique_ptr<Drawable> IconManager::getOpenFolderIcon(Colour colour)
{
    return createFromSvg(lucide_folder_open_svg, colour);
}

std::unique_ptr<Drawable> IconManager::getTrashIcon(Colour colour)
{
    return createFromSvg(lucide_trash_svg, colour);
}

std::unique_ptr<Drawable> IconManager::getCloseIcon(Colour colour)
{
    return createFromSvg(lucide_x_svg, colour);
}

std::unique_ptr<Drawable> IconManager::getPlusIcon(Colour colour)
{
    return createFromSvg(lucide_plus_svg, colour);
}

std::unique_ptr<Drawable> IconManager::getSearchIcon(Colour colour)
{
    return createFromSvg(lucide_search_svg, colour);
}

std::unique_ptr<Drawable> IconManager::getCheckIcon(Colour colour)
{
    return createFromSvg(lucide_check_svg, colour);
}

std::unique_ptr<Drawable> IconManager::getDownloadIcon(Colour colour)
{
    return createFromSvg(lucide_download_svg, colour);
}

std::unique_ptr<Drawable> IconManager::getRefreshIcon(Colour colour)
{
    return createFromSvg(lucide_refresh_svg, colour);
}
