/*
  ==============================================================================

   This file is part of the JUCE library - "Jules' Utility Class Extensions"
   Copyright 2004-7 by Raw Material Software ltd.

  ------------------------------------------------------------------------------

   JUCE can be redistributed and/or modified under the terms of the
   GNU General Public License, as published by the Free Software Foundation;
   either version 2 of the License, or (at your option) any later version.

   JUCE is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with JUCE; if not, visit www.gnu.org/licenses or write to the
   Free Software Foundation, Inc., 59 Temple Place, Suite 330,
   Boston, MA 02111-1307 USA

  ------------------------------------------------------------------------------

   If you'd like to release a closed-source product which uses JUCE, commercial
   licenses are also available: visit www.rawmaterialsoftware.com/juce for
   more information.

  ==============================================================================
*/

#ifndef __JUCE_MAINHOSTWINDOW_JUCEHEADER__xxxxx
#define __JUCE_MAINHOSTWINDOW_JUCEHEADER__xxxxx

#include "FilterGraph.h"

#include <JuceHeader.h>
using juce::uint32;

//==============================================================================
/**
    Manages the internal plugin types.
*/
class InternalPluginFormat : public AudioPluginFormat
{
  public:
    //==============================================================================
    InternalPluginFormat();
    ~InternalPluginFormat() {}

    //==============================================================================
    enum InternalFilterType
    {
        audioInputFilter = 0,
        audioOutputFilter,
        midiInputFilter,

        midiInterceptorFilter,
        oscInputFilter,
        levelProcFilter,
        filePlayerProcFilter,
        outputToggleProcFilter,
        vuMeterProcFilter,
        recorderProcFilter,
        metronomeProcFilter,
        looperProcFilter,
        tunerProcFilter,
        toneGenProcFilter,
        splitterProcFilter,
        mixerProcFilter,
        irLoaderProcFilter,
        namProcFilter,
        midiTransposeProcFilter,
        midiRechannelizeProcFilter,
        keyboardSplitProcFilter,
        notesProcFilter,
        labelProcFilter,
        midiFilePlayerProcFilter,
        subGraphProcFilter,
        channelInputProcFilter,
        channelOutputProcFilter,

        endOfFilterTypes
    };

    const PluginDescription* getDescriptionFor(const InternalFilterType type);

    void getAllTypes(OwnedArray<PluginDescription>& results);

    /// Returns only user-facing internal plugins (excludes Audio I/O, MIDI Input, etc.)
    void getUserFacingTypes(OwnedArray<PluginDescription>& results);

    bool canScanForPlugins() const { return false; };

    //==============================================================================
    String getName() const { return "Internal"; }
    bool fileMightContainThisPluginType(const String&) { return true; }
    FileSearchPath getDefaultLocationsToSearch() { return FileSearchPath(); }
    void findAllTypesForFile(OwnedArray<PluginDescription>&, const String&) {}
    AudioPluginInstance* createInstanceFromDescription(const PluginDescription& desc);

    // JUCE 8: isTrivialToScan is now a required pure virtual
    bool isTrivialToScan() const override { return true; }

    String getNameOfPluginFromIdentifier(const String& fileOrIdentifier) override { return "Internal"; }
    bool doesPluginStillExist(const PluginDescription& desc) override { return true; }
    // JUCE 8: pluginNeedsRescanning is now pure virtual
    bool pluginNeedsRescanning(const PluginDescription&) override { return false; }
    StringArray searchPathsForPlugins(const FileSearchPath& directoriesToSearch, bool recursive, bool) override
    {
        StringArray retval;
        return retval;
    }
    bool requiresUnblockedMessageThreadDuringCreation(const PluginDescription&) const override { return false; }

  protected:
    // JUCE 8: Override the protected createPluginInstance (not the public
    // createPluginInstanceAsync)
    void createPluginInstance(const PluginDescription& desc, double initialSampleRate, int initialBufferSize,
                              PluginCreationCallback callback) override
    {
        auto* instance = createInstanceFromDescription(desc);
        callback(std::unique_ptr<AudioPluginInstance>(instance), instance ? String() : "Could not create plugin");
    }

  private:
    PluginDescription audioInDesc;
    PluginDescription audioOutDesc;
    PluginDescription midiInDesc;

    PluginDescription midiInterceptorDesc;
    PluginDescription oscInputDesc;
    PluginDescription levelProcDesc;
    PluginDescription filePlayerProcDesc;
    PluginDescription outputToggleProcDesc;
    PluginDescription vuMeterProcDesc;
    PluginDescription recorderProcDesc;
    PluginDescription metronomeProcDesc;
    PluginDescription looperProcDesc;
    PluginDescription tunerProcDesc;
    PluginDescription toneGenProcDesc;
    PluginDescription splitterProcDesc;
    PluginDescription mixerProcDesc;
    PluginDescription irLoaderProcDesc;
    PluginDescription namProcDesc;
    PluginDescription midiTransposeProcDesc;
    PluginDescription midiRechannelizeProcDesc;
    PluginDescription keyboardSplitProcDesc;
    PluginDescription notesProcDesc;
    PluginDescription labelProcDesc;
    PluginDescription midiFilePlayerProcDesc;
    PluginDescription subGraphProcDesc;
    PluginDescription channelInputProcDesc;
    PluginDescription channelOutputProcDesc;
};

#endif
