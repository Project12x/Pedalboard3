//	ColourScheme.h - Singleton struct handling colour schemes.
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

#ifndef COLOURSCHEME_H_
#define COLOURSCHEME_H_

#include <JuceHeader.h>
#include <map>
#include <vector>

struct ColourRoleSpec
{
    const char* name;
    const char* surface;
    const char* state;
    const char* usage;
};

struct LookAndFeelColourSpec
{
    const char* component;
    int colourId;
    const char* role;
    float alpha;
};

///	Singleton struct handling colour schemes.
struct ColourScheme
{
  public:
    ///	The map of all the available colours.
    std::map<String, Colour> colours;
    ///	The name of the current colour scheme preset.
    String presetName;

    ///	Returns the sole instance of the singleton.
    static ColourScheme& getInstance();

    ///	Returns a StringArray of all the available colour scheme presets.
    const StringArray getPresets() const;
    ///	Loads a colour scheme preset.
    void loadPreset(const String& name);
    ///	Saves a colour scheme preset.
    void savePreset(const String& name);

    ///	Returns true if colours == the named preset.
    bool doesColoursMatchPreset(const String& name);

    /// @brief Returns a StringArray of built-in theme names.
    /// @return StringArray containing: Midnight, Daylight, Synthwave, Deep Ocean, Forest
    static const StringArray getBuiltInPresets();

    /// @brief Loads a built-in preset by name.
    /// @param name The preset name (e.g., "Midnight", "Synthwave")
    /// @return true if the preset was found and loaded, false otherwise
    bool loadBuiltInPreset(const String& name);

    /// @brief Returns the semantic colour role matrix required by built-in themes.
    static const std::vector<ColourRoleSpec>& getSemanticColourRoles();

    /// @brief Returns the shared LookAndFeel colour-id to semantic-role contract.
    static const std::vector<LookAndFeelColourSpec>& getLookAndFeelColourSpecs();

    /// @brief Returns the unique colour role keys required by built-in themes.
    static StringArray getRequiredColourRoles();

    /// @brief Returns true if the current colour map contains every required role.
    bool hasRequiredColourRoles(StringArray* missingRoles = nullptr) const;

  private:
    ///	Constructor.
    ColourScheme();
    ///	Destructor.
    ~ColourScheme();
};

#endif
