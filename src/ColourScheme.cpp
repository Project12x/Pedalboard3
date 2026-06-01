//	ColourScheme.cpp - Singleton struct handling colour schemes.
//	----------------------------------------------------------------------------
//	This file is part of Pedalboard3, an audio plugin host.
//	Copyright (c) 2012 Niall Moody.
//
//	This program is free software: you can redistribute it and/or modify
//	it under the terms of the GNU General Public License as published by
//	the Free Software Foundation, either version 3 of the License, or
//	(at your option) any later version.
//
//	This program is distributed in the hope that it will be useful,
//	but WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//	GNU General Public License for more details.
//
//	You should have received a copy of the GNU General Public License
//	along with this program.  If not, see <http://www.gnu.org/licenses/>.
//	----------------------------------------------------------------------------

#include "ColourScheme.h"

#ifndef PEDALBOARD3_TESTS
#include "JuceHelperStuff.h"
#endif

using namespace std;

namespace
{
File getColourSchemeAppDataFolder()
{
#ifdef PEDALBOARD3_TESTS
    auto folder = File::getSpecialLocation(File::tempDirectory).getChildFile("Pedalboard3_ColourSchemeTests");
    folder.createDirectory();
    return folder;
#else
    return JuceHelperStuff::getAppDataFolder();
#endif
}
} // namespace

//------------------------------------------------------------------------------
ColourScheme& ColourScheme::getInstance()
{
    static ColourScheme retval;

    return retval;
}

//------------------------------------------------------------------------------
const StringArray ColourScheme::getPresets() const
{
    int i;
    Array<File> files;
    StringArray retval;
    File settingsDir = getColourSchemeAppDataFolder();

    // Add built-in presets first
    retval.addArray(getBuiltInPresets());

    // Add user-saved presets from filesystem
    settingsDir.findChildFiles(files, File::findFiles, false, "*.colourscheme");
    for (i = 0; i < files.size(); ++i)
    {
        String presetName = files[i].getFileNameWithoutExtension();
        // Avoid duplicates with built-in names
        if (!retval.contains(presetName))
            retval.add(presetName);
    }

    return retval;
}

//------------------------------------------------------------------------------
void ColourScheme::loadPreset(const String& name)
{
    String filename;
    File settingsDir = getColourSchemeAppDataFolder();

    filename << name << ".colourscheme";

    File presetFile = settingsDir.getChildFile(filename);

    if (presetFile.existsAsFile())
    {
        std::unique_ptr<XmlElement> rootXml(XmlDocument::parse(presetFile)); // JUCE 8: unique_ptr

        if (rootXml)
        {
            if (rootXml->hasTagName("Pedalboard3ColourScheme"))
            {
                forEachXmlChildElement(*rootXml, colour)
                {
                    if (colour->hasTagName("Colour"))
                    {
                        String colName;
                        String tempstr;

                        colName = colour->getStringAttribute("name", "NoName");
                        tempstr = colour->getStringAttribute("value", "FFFFFFFF");
                        if (colName != "NoName")
                            colours[colName] = Colour(tempstr.getHexValue32());
                    }
                }
                presetName = name;
            }
        }
    }
    else
    {
        // Try loading as built-in preset
        loadBuiltInPreset(name);
    }
}

//------------------------------------------------------------------------------
void ColourScheme::savePreset(const String& name)
{
    String filename;
    map<String, Colour>::iterator it;
    XmlElement rootXml("Pedalboard3ColourScheme");
    File settingsDir = getColourSchemeAppDataFolder();

    filename << name << ".colourscheme";

    File presetFile = settingsDir.getChildFile(filename);

    for (it = colours.begin(); it != colours.end(); ++it)
    {
        XmlElement* colour = new XmlElement("Colour");

        colour->setAttribute("name", it->first);
        colour->setAttribute("value", it->second.toString());

        rootXml.addChildElement(colour);
    }

    presetName = name;
    rootXml.writeToFile(presetFile, "");
}

//------------------------------------------------------------------------------
bool ColourScheme::doesColoursMatchPreset(const String& name)
{
    String tempstr;
    File presetFile;
    bool retval = true;
    File settingsDir = getColourSchemeAppDataFolder();

    tempstr << name << ".colourscheme";
    presetFile = settingsDir.getChildFile(tempstr);

    if (presetFile.existsAsFile())
    {
        std::unique_ptr<XmlElement> rootXml(XmlDocument::parse(presetFile)); // JUCE 8: unique_ptr

        if (rootXml)
        {
            if (rootXml->hasTagName("Pedalboard3ColourScheme"))
            {
                forEachXmlChildElement(*rootXml, colour)
                {
                    if (colour->hasTagName("Colour"))
                    {
                        String colName;
                        String value;

                        colName = colour->getStringAttribute("name", "NoName");
                        value = colour->getStringAttribute("value", "FFFFFFFF");

                        if (colours[colName] != Colour(value.getHexValue32()))
                        {
                            retval = false;
                            break;
                        }
                    }
                }
                presetName = name;
            }
        }
    }
    else
        retval = false;

    return retval;
}

//------------------------------------------------------------------------------
const StringArray ColourScheme::getBuiltInPresets()
{
    return {"Midnight", "Daylight", "Synthwave", "Deep Ocean", "Forest"};
}

//------------------------------------------------------------------------------
const std::vector<ColourRoleSpec>& ColourScheme::getSemanticColourRoles()
{
    static const std::vector<ColourRoleSpec> roles{
        {"Window Background", "app-shell", "default", "Root frame and primary empty-space fill."},
        {"Field Background", "graph-canvas", "default", "Patch canvas and large editable work surfaces."},
        {"Text Colour", "typography", "default", "Primary readable text."},
        {"Text Colour", "typography", "disabled", "Disabled text derives from primary text by alpha."},
        {"Plugin Border", "node-card", "focus", "Node outlines, focused edges, and subtle separators."},
        {"Plugin Background", "node-card", "default", "Plugin/node body surfaces and internal processor panels."},
        {"Audio Connection", "routing-cable", "active", "Audio signal paths, active routing affordances, and audio pins."},
        {"Parameter Connection", "routing-cable", "active", "Parameter/control paths and parameter pins."},
        {"Button Colour", "control", "default", "Default button and toolbar control fill."},
        {"Button Highlight", "control", "hover", "Hovered or pressed button fill and raised control emphasis."},
        {"Text Editor Colour", "input", "default", "Text-entry and editable-value backgrounds."},
        {"Menu Selection Colour", "menu", "selection", "Selected menu row and menu focus affordance."},
        {"Accent Colour", "control", "focus", "Primary accent, keyboard focus ring, and selected-control emphasis."},
        {"CPU Meter Colour", "meter", "active", "CPU load meter and system activity indicator."},
        {"Dialog Inner Background", "dialog", "default", "Nested dialog panels, lists, and utility surface interiors."},
        {"Slider Colour", "control", "active", "Slider tracks, fills, and primary continuous-control emphasis."},
        {"List Selected Colour", "list", "selection", "Selected rows in lists, browsers, and searchable menus."},
        {"VU Meter Lower Colour", "meter", "active-low", "Nominal low/mid meter range."},
        {"VU Meter Upper Colour", "meter", "warning", "Elevated meter range before clipping."},
        {"VU Meter Over Colour", "meter", "danger", "Over/clip meter range."},
        {"Vector Colour", "iconography", "default", "Drawable icons, strokes, and legacy vector glyphs."},
        {"Waveform Colour", "meter", "active", "Waveform and scope traces."},
        {"Level Dial Colour", "control", "active", "Rotary level control fill and emphasis."},
        {"Tick Box Colour", "control", "active", "Checked checkbox and binary setting emphasis."},
        {"Stage Background Top", "stage", "default", "Stage Mode gradient start."},
        {"Stage Background Bottom", "stage", "default", "Stage Mode gradient end."},
        {"Stage Panel Background", "stage", "default", "Stage Mode panel surface and status block fill."},
        {"Dialog Background", "dialog", "default", "Top-level utility/dialog window background."},
        {"Tuner Active Colour", "stage", "active", "In-tune and tuner-active signal state."},
        {"Danger Colour", "semantic", "danger", "Destructive, panic, error, clip, and invalid states."},
        {"Warning Colour", "semantic", "warning", "Caution, transitional, and elevated-risk states."},
        {"Success Colour", "semantic", "success", "Successful, ready, armed, and valid states."},
    };

    return roles;
}

//------------------------------------------------------------------------------
const std::vector<LookAndFeelColourSpec>& ColourScheme::getLookAndFeelColourSpecs()
{
    static const std::vector<LookAndFeelColourSpec> specs{
        {"TextButton", static_cast<int>(TextButton::buttonColourId), "Button Colour", 1.0f},
        {"TextButton", static_cast<int>(TextButton::buttonOnColourId), "Button Colour", 1.0f},
        {"TextButton", static_cast<int>(TextButton::textColourOnId), "Text Colour", 1.0f},
        {"TextButton", static_cast<int>(TextButton::textColourOffId), "Text Colour", 1.0f},
        {"PopupMenu", static_cast<int>(PopupMenu::backgroundColourId), "Window Background", 1.0f},
        {"PopupMenu", static_cast<int>(PopupMenu::textColourId), "Text Colour", 1.0f},
        {"PopupMenu", static_cast<int>(PopupMenu::highlightedBackgroundColourId), "Menu Selection Colour", 1.0f},
        {"PopupMenu", static_cast<int>(PopupMenu::highlightedTextColourId), "Text Colour", 1.0f},
        {"ComboBox", static_cast<int>(ComboBox::backgroundColourId), "Text Editor Colour", 1.0f},
        {"ComboBox", static_cast<int>(ComboBox::buttonColourId), "Button Colour", 1.0f},
        {"ComboBox", static_cast<int>(ComboBox::arrowColourId), "Text Colour", 1.0f},
        {"ComboBox", static_cast<int>(ComboBox::outlineColourId), "Plugin Border", 1.0f},
        {"ComboBox", static_cast<int>(ComboBox::focusedOutlineColourId), "Menu Selection Colour", 1.0f},
        {"TextEditor", static_cast<int>(TextEditor::backgroundColourId), "Text Editor Colour", 1.0f},
        {"TextEditor", static_cast<int>(TextEditor::textColourId), "Text Colour", 1.0f},
        {"TextEditor", static_cast<int>(TextEditor::outlineColourId), "Plugin Border", 1.0f},
        {"TextEditor", static_cast<int>(TextEditor::focusedOutlineColourId), "Menu Selection Colour", 1.0f},
        {"TextEditor", static_cast<int>(TextEditor::highlightColourId), "Button Highlight", 1.0f},
        {"TextEditor", static_cast<int>(TextEditor::highlightedTextColourId), "Text Colour", 1.0f},
        {"Label", static_cast<int>(Label::textColourId), "Text Colour", 1.0f},
        {"ToggleButton", static_cast<int>(ToggleButton::textColourId), "Text Colour", 1.0f},
        {"ToggleButton", static_cast<int>(ToggleButton::tickColourId), "Vector Colour", 1.0f},
        {"ToggleButton", static_cast<int>(ToggleButton::tickDisabledColourId), "Tick Box Colour", 1.0f},
        {"Slider", static_cast<int>(Slider::thumbColourId), "Slider Colour", 1.0f},
        {"Slider", static_cast<int>(Slider::trackColourId), "Slider Colour", 0.4f},
        {"Slider", static_cast<int>(Slider::rotarySliderFillColourId), "Slider Colour", 1.0f},
        {"Slider", static_cast<int>(Slider::rotarySliderOutlineColourId), "Plugin Border", 1.0f},
        {"Slider", static_cast<int>(Slider::textBoxTextColourId), "Text Colour", 1.0f},
        {"Slider", static_cast<int>(Slider::textBoxBackgroundColourId), "Text Editor Colour", 1.0f},
        {"Slider", static_cast<int>(Slider::textBoxHighlightColourId), "Button Highlight", 1.0f},
        {"Slider", static_cast<int>(Slider::textBoxOutlineColourId), "Plugin Border", 1.0f},
        {"ScrollBar", static_cast<int>(ScrollBar::thumbColourId), "Button Highlight", 1.0f},
        {"ScrollBar", static_cast<int>(ScrollBar::trackColourId), "Field Background", 1.0f},
        {"ListBox", static_cast<int>(ListBox::backgroundColourId), "Field Background", 1.0f},
        {"ListBox", static_cast<int>(ListBox::textColourId), "Text Colour", 1.0f},
        {"ProgressBar", static_cast<int>(ProgressBar::backgroundColourId), "Window Background", 1.0f},
        {"ProgressBar", static_cast<int>(ProgressBar::foregroundColourId), "CPU Meter Colour", 1.0f},
        {"DirectoryContentsDisplayComponent",
         static_cast<int>(DirectoryContentsDisplayComponent::highlightColourId), "List Selected Colour", 1.0f},
    };

    return specs;
}

//------------------------------------------------------------------------------
StringArray ColourScheme::getRequiredColourRoles()
{
    StringArray requiredRoles;
    for (const auto& role : getSemanticColourRoles())
        requiredRoles.addIfNotAlreadyThere(role.name);

    return requiredRoles;
}

//------------------------------------------------------------------------------
bool ColourScheme::hasRequiredColourRoles(StringArray* missingRoles) const
{
    if (missingRoles != nullptr)
        missingRoles->clear();

    bool hasAllRoles = true;
    for (const auto& role : getSemanticColourRoles())
    {
        if (colours.find(role.name) == colours.end())
        {
            hasAllRoles = false;
            if (missingRoles != nullptr)
                missingRoles->addIfNotAlreadyThere(role.name);
        }
    }

    return hasAllRoles;
}

//------------------------------------------------------------------------------
bool ColourScheme::loadBuiltInPreset(const String& name)
{
    if (name == "Midnight")
    {
        // Default dark theme - professional and easy on eyes
        colours["Window Background"] = Colour(0xFF1A1A2E);
        colours["Field Background"] = Colour(0xFF16213E);
        colours["Text Colour"] = Colour(0xFFE8E8E8);
        colours["Plugin Border"] = Colour(0xFF3A3A5C);
        colours["Plugin Background"] = Colour(0xFF252545);
        colours["Audio Connection"] = Colour(0xFF00D9FF);
        colours["Parameter Connection"] = Colour(0xFFFFAA00);
        colours["Button Colour"] = Colour(0xFF2D2D50);
        colours["Button Highlight"] = Colour(0xFF4A4A70);
        colours["Text Editor Colour"] = Colour(0xFF0F0F23);
        colours["Menu Selection Colour"] = Colour(0xFF00D9FF);
        colours["Accent Colour"] = Colour(0xFF00D9FF);
        colours["CPU Meter Colour"] = Colour(0xFF00FF88);
        colours["Dialog Inner Background"] = Colour(0xFF252545);
        colours["Slider Colour"] = Colour(0xFF6366F1);
        colours["List Selected Colour"] = Colour(0xFF3A3A8C);
        colours["VU Meter Lower Colour"] = Colour(0x7F00BF00);
        colours["VU Meter Upper Colour"] = Colour(0x7FFFFF00);
        colours["VU Meter Over Colour"] = Colour(0x7FFF0000);
        colours["Vector Colour"] = Colour(0x80000000);
        colours["Waveform Colour"] = Colour(0xFF6366F1);
        colours["Level Dial Colour"] = Colour(0xFF4F46E5);
        colours["Tick Box Colour"] = Colour(0x806366F1);
        colours["Stage Background Top"] = Colour(0xFF1a1a2e);
        colours["Stage Background Bottom"] = Colour(0xFF0f0f1a);
        colours["Stage Panel Background"] = Colour(0xFF2a2a3e);
        colours["Dialog Background"] = Colour(0xFFEEECE1);
        colours["Tuner Active Colour"] = Colour(0xFF00AA55);
        colours["Danger Colour"] = Colour(0xFFDC2626);
        colours["Warning Colour"] = Colour(0xFFF59E0B);
        colours["Success Colour"] = Colour(0xFF16A34A);
    }
    else if (name == "Daylight")
    {
        // Light theme for bright environments
        colours["Window Background"] = Colour(0xFFF5F5F5);
        colours["Field Background"] = Colour(0xFFFFFFFF);
        colours["Text Colour"] = Colour(0xFF1A1A1A);
        colours["Plugin Border"] = Colour(0xFFCCCCCC);
        colours["Plugin Background"] = Colour(0xFFE8E8E8);
        colours["Audio Connection"] = Colour(0xFF0077CC);
        colours["Parameter Connection"] = Colour(0xFFCC6600);
        colours["Button Colour"] = Colour(0xFFDDDDDD);
        colours["Button Highlight"] = Colour(0xFFBBBBBB);
        colours["Text Editor Colour"] = Colour(0xFFFFFFFF);
        colours["Menu Selection Colour"] = Colour(0xFF0077CC);
        colours["Accent Colour"] = Colour(0xFF0077CC);
        colours["CPU Meter Colour"] = Colour(0xFF00AA00);
        colours["Dialog Inner Background"] = Colour(0xFFFFFFFF);
        colours["Slider Colour"] = Colour(0xFF0077CC);
        colours["List Selected Colour"] = Colour(0xFFCCE5FF);
        colours["VU Meter Lower Colour"] = Colour(0x7F00AA00);
        colours["VU Meter Upper Colour"] = Colour(0x7FCCCC00);
        colours["VU Meter Over Colour"] = Colour(0x7FCC0000);
        colours["Vector Colour"] = Colour(0x40000000);
        colours["Waveform Colour"] = Colour(0xFF0077CC);
        colours["Level Dial Colour"] = Colour(0xFF005599);
        colours["Tick Box Colour"] = Colour(0x800077CC);
        colours["Stage Background Top"] = Colour(0xFFE8E8E8);
        colours["Stage Background Bottom"] = Colour(0xFFD0D0D0);
        colours["Stage Panel Background"] = Colour(0xFFCCCCCC);
        colours["Dialog Background"] = Colour(0xFFF0F0F0);
        colours["Tuner Active Colour"] = Colour(0xFF00AA00);
        colours["Danger Colour"] = Colour(0xFFDC2626);
        colours["Warning Colour"] = Colour(0xFFD97706);
        colours["Success Colour"] = Colour(0xFF16A34A);
    }
    else if (name == "Synthwave")
    {
        // Retro neon 80s aesthetic
        colours["Window Background"] = Colour(0xFF0D0221);
        colours["Field Background"] = Colour(0xFF1A0533);
        colours["Text Colour"] = Colour(0xFFFF00FF);
        colours["Plugin Border"] = Colour(0xFFFF00AA);
        colours["Plugin Background"] = Colour(0xFF2D0A4E);
        colours["Audio Connection"] = Colour(0xFF00FFFF);
        colours["Parameter Connection"] = Colour(0xFFFF6B00);
        colours["Button Colour"] = Colour(0xFF3D1A6D);
        colours["Button Highlight"] = Colour(0xFF5A2D82);
        colours["Text Editor Colour"] = Colour(0xFF0A0015);
        colours["Menu Selection Colour"] = Colour(0xFFFF00FF);
        colours["Accent Colour"] = Colour(0xFFFF00FF);
        colours["CPU Meter Colour"] = Colour(0xFF00FF00);
        colours["Dialog Inner Background"] = Colour(0xFF1A0533);
        colours["Slider Colour"] = Colour(0xFFFF00FF);
        colours["List Selected Colour"] = Colour(0xFF5A2D82);
        colours["VU Meter Lower Colour"] = Colour(0x7F00FFFF);
        colours["VU Meter Upper Colour"] = Colour(0x7FFF00FF);
        colours["VU Meter Over Colour"] = Colour(0x7FFF0000);
        colours["Vector Colour"] = Colour(0x80FF00FF);
        colours["Waveform Colour"] = Colour(0xFF00FFFF);
        colours["Level Dial Colour"] = Colour(0xFFFF00AA);
        colours["Tick Box Colour"] = Colour(0x80FF00FF);
        colours["Stage Background Top"] = Colour(0xFF0D0221);
        colours["Stage Background Bottom"] = Colour(0xFF060112);
        colours["Stage Panel Background"] = Colour(0xFF2D0A4E);
        colours["Dialog Background"] = Colour(0xFF1A0533);
        colours["Tuner Active Colour"] = Colour(0xFF00FF88);
        colours["Danger Colour"] = Colour(0xFFFF0055);
        colours["Warning Colour"] = Colour(0xFFFF6600);
        colours["Success Colour"] = Colour(0xFF00FF88);
    }
    else if (name == "Deep Ocean")
    {
        // Calm blue underwater theme
        colours["Window Background"] = Colour(0xFF0A1628);
        colours["Field Background"] = Colour(0xFF0D1F3C);
        colours["Text Colour"] = Colour(0xFFB8D4E8);
        colours["Plugin Border"] = Colour(0xFF1E4976);
        colours["Plugin Background"] = Colour(0xFF142D4C);
        colours["Audio Connection"] = Colour(0xFF00C8FF);
        colours["Parameter Connection"] = Colour(0xFF7DD3FC);
        colours["Button Colour"] = Colour(0xFF1A3A5C);
        colours["Button Highlight"] = Colour(0xFF2A5A8C);
        colours["Text Editor Colour"] = Colour(0xFF081420);
        colours["Menu Selection Colour"] = Colour(0xFF00C8FF);
        colours["Accent Colour"] = Colour(0xFF00C8FF);
        colours["CPU Meter Colour"] = Colour(0xFF00DDAA);
        colours["Dialog Inner Background"] = Colour(0xFF0D1F3C);
        colours["Slider Colour"] = Colour(0xFF0EA5E9);
        colours["List Selected Colour"] = Colour(0xFF1E4976);
        colours["VU Meter Lower Colour"] = Colour(0x7F00AACC);
        colours["VU Meter Upper Colour"] = Colour(0x7F00DDFF);
        colours["VU Meter Over Colour"] = Colour(0x7FFF6666);
        colours["Vector Colour"] = Colour(0x8000C8FF);
        colours["Waveform Colour"] = Colour(0xFF7DD3FC);
        colours["Level Dial Colour"] = Colour(0xFF0284C7);
        colours["Tick Box Colour"] = Colour(0x800EA5E9);
        colours["Stage Background Top"] = Colour(0xFF0A1628);
        colours["Stage Background Bottom"] = Colour(0xFF060E18);
        colours["Stage Panel Background"] = Colour(0xFF142D4C);
        colours["Dialog Background"] = Colour(0xFF0D1F3C);
        colours["Tuner Active Colour"] = Colour(0xFF00DDAA);
        colours["Danger Colour"] = Colour(0xFFEF4444);
        colours["Warning Colour"] = Colour(0xFFF59E0B);
        colours["Success Colour"] = Colour(0xFF00DDAA);
    }
    else if (name == "Forest")
    {
        // Natural green and earth tones
        colours["Window Background"] = Colour(0xFF1A2F1A);
        colours["Field Background"] = Colour(0xFF0F1F0F);
        colours["Text Colour"] = Colour(0xFFD4E8C8);
        colours["Plugin Border"] = Colour(0xFF3A5A3A);
        colours["Plugin Background"] = Colour(0xFF244024);
        colours["Audio Connection"] = Colour(0xFF66CC66);
        colours["Parameter Connection"] = Colour(0xFFCCAA44);
        colours["Button Colour"] = Colour(0xFF2A4A2A);
        colours["Button Highlight"] = Colour(0xFF3A6A3A);
        colours["Text Editor Colour"] = Colour(0xFF0A150A);
        colours["Menu Selection Colour"] = Colour(0xFF66CC66);
        colours["Accent Colour"] = Colour(0xFF66CC66);
        colours["CPU Meter Colour"] = Colour(0xFF88EE88);
        colours["Dialog Inner Background"] = Colour(0xFF1A2F1A);
        colours["Slider Colour"] = Colour(0xFF4ADE80);
        colours["List Selected Colour"] = Colour(0xFF2A5A2A);
        colours["VU Meter Lower Colour"] = Colour(0x7F22BB22);
        colours["VU Meter Upper Colour"] = Colour(0x7FAADD22);
        colours["VU Meter Over Colour"] = Colour(0x7FDD4444);
        colours["Vector Colour"] = Colour(0x8066CC66);
        colours["Waveform Colour"] = Colour(0xFF86EFAC);
        colours["Level Dial Colour"] = Colour(0xFF22C55E);
        colours["Tick Box Colour"] = Colour(0x804ADE80);
        colours["Stage Background Top"] = Colour(0xFF1A2F1A);
        colours["Stage Background Bottom"] = Colour(0xFF0F1F0F);
        colours["Stage Panel Background"] = Colour(0xFF244024);
        colours["Dialog Background"] = Colour(0xFF1A2F1A);
        colours["Tuner Active Colour"] = Colour(0xFF66CC66);
        colours["Danger Colour"] = Colour(0xFFDD4444);
        colours["Warning Colour"] = Colour(0xFFCCAA44);
        colours["Success Colour"] = Colour(0xFF44BB44);
    }
    else
    {
        return false; // Unknown preset
    }

    presetName = name;
    return true;
}

//------------------------------------------------------------------------------
ColourScheme::ColourScheme()
{
    File defaultFile = getColourSchemeAppDataFolder().getChildFile("default.colourscheme");

    if (defaultFile.existsAsFile())
        loadPreset("default");
    else
    {
        // Load built-in Midnight theme as default
        loadBuiltInPreset("Midnight");
        savePreset("default");
    }
}

//------------------------------------------------------------------------------
ColourScheme::~ColourScheme() {}
