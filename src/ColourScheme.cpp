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

#include "JuceHelperStuff.h"

using namespace std;

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
    File settingsDir = JuceHelperStuff::getAppDataFolder();

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
    File settingsDir = JuceHelperStuff::getAppDataFolder();

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
    File settingsDir = JuceHelperStuff::getAppDataFolder();

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
    File settingsDir = JuceHelperStuff::getAppDataFolder();

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
    File defaultFile = JuceHelperStuff::getAppDataFolder().getChildFile("default.colourscheme");

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
