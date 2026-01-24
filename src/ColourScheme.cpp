//	ColourScheme.cpp - Singleton struct handling colour schemes.
//	----------------------------------------------------------------------------
//	This file is part of Pedalboard2, an audio plugin host.
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

    settingsDir.findChildFiles(files, File::findFiles, false, "*.colourscheme");
    for (i = 0; i < files.size(); ++i)
        retval.add(files[i].getFileNameWithoutExtension());

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
            if (rootXml->hasTagName("Pedalboard2ColourScheme"))
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
}

//------------------------------------------------------------------------------
void ColourScheme::savePreset(const String& name)
{
    String filename;
    map<String, Colour>::iterator it;
    XmlElement rootXml("Pedalboard2ColourScheme");
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
            if (rootXml->hasTagName("Pedalboard2ColourScheme"))
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
ColourScheme::ColourScheme()
{
    File defaultFile = JuceHelperStuff::getAppDataFolder().getChildFile("default.colourscheme");

    if (defaultFile.existsAsFile())
        loadPreset("default");
    else
    {
        presetName = "default";

        colours["Window Background"] = Colour(0xFF333333);
        colours["Field Background"] = Colour(0xFF222222);
        colours["Text Colour"] = Colour(0xFFEEEEEE);
        colours["Plugin Border"] = Colour(0xFF555555);
        colours["Plugin Background"] = Colour(0xFF444444);
        colours["Audio Connection"] = Colour(0xFF00AAFF);
        colours["Parameter Connection"] = Colour(0xFFFFAA00);
        colours["Button Colour"] = Colour(0xFF444444);
        colours["Button Highlight"] = Colour(0xFF666666);
        colours["Text Editor Colour"] = Colour(0xFF222222);
        colours["Menu Selection Colour"] = Colour(0xFF00AAFF); // New entry for menu highlights
        colours["CPU Meter Colour"] = Colour(0xFF00FF00);
        colours["Dialog Inner Background"] = Colour(0xFFFFFFFF);
        colours["CPU Meter Colour"] = Colour(0xB0B0B0FF);
        colours["Slider Colour"] = Colour(0xFF9A9181);
        colours["List Selected Colour"] = Colour(0xFFB0B0FF);
        colours["VU Meter Lower Colour"] = Colour(0x7F00BF00);
        colours["VU Meter Upper Colour"] = Colour(0x7FFFFF00);
        colours["VU Meter Over Colour"] = Colour(0x7FFF0000);
        colours["Vector Colour"] = Colour(0x80000000);
        colours["Menu Selection Colour"] = Colour(0x40000000);
        colours["Waveform Colour"] = Colour(0xFFB0B0FF);
        colours["Level Dial Colour"] = Colour(0xFFB0B0FF).darker(0.25f);
        colours["Tick Box Colour"] = Colour(0x809A9181);

        savePreset("default");
    }
}

//------------------------------------------------------------------------------
ColourScheme::~ColourScheme() {}
