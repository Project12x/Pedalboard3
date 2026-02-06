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

#include "InternalFilters.h"

#include "FilterGraph.h"
#include "IRLoaderProcessor.h"
#include "NAMProcessor.h"
#include "LabelProcessor.h"
#include "MidiFilePlayer.h"
#include "MidiMappingManager.h"
#include "MidiUtilityProcessors.h"
#include "NotesProcessor.h"
#include "OscMappingManager.h"
#include "PedalboardProcessors.h"
#include "RoutingProcessors.h"
#include "SubGraphProcessor.h"
#include "ToneGeneratorProcessor.h"
#include "TunerProcessor.h"

//==============================================================================
InternalPluginFormat::InternalPluginFormat()
{
    {
        AudioProcessorGraph::AudioGraphIOProcessor p(AudioProcessorGraph::AudioGraphIOProcessor::audioOutputNode);
        p.fillInPluginDescription(audioOutDesc);
        audioOutDesc.category = "Built-in";
    }

    {
        AudioProcessorGraph::AudioGraphIOProcessor p(AudioProcessorGraph::AudioGraphIOProcessor::audioInputNode);
        p.fillInPluginDescription(audioInDesc);
        audioInDesc.category = "Built-in";
    }

    {
        AudioProcessorGraph::AudioGraphIOProcessor p(AudioProcessorGraph::AudioGraphIOProcessor::midiInputNode);
        p.fillInPluginDescription(midiInDesc);
        midiInDesc.category = "Built-in";
    }

    {
        MidiInterceptor p;
        p.fillInPluginDescription(midiInterceptorDesc);
        midiInterceptorDesc.category = "Built-in";
    }

    {
        OscInput p;
        p.fillInPluginDescription(oscInputDesc);
        oscInputDesc.category = "Built-in";
    }

    {
        LevelProcessor p;
        p.fillInPluginDescription(levelProcDesc);
        levelProcDesc.category = "Built-in";
    }

    {
        FilePlayerProcessor p;
        p.fillInPluginDescription(filePlayerProcDesc);
        filePlayerProcDesc.category = "Built-in";
    }

    {
        OutputToggleProcessor p;
        p.fillInPluginDescription(outputToggleProcDesc);
        outputToggleProcDesc.category = "Built-in";
    }

    {
        VuMeterProcessor p;
        p.fillInPluginDescription(vuMeterProcDesc);
        vuMeterProcDesc.category = "Built-in";
    }

    {
        RecorderProcessor p;
        p.fillInPluginDescription(recorderProcDesc);
        recorderProcDesc.category = "Built-in";
    }

    {
        MetronomeProcessor p;
        p.fillInPluginDescription(metronomeProcDesc);
        metronomeProcDesc.category = "Built-in";
    }

    {
        LooperProcessor p;
        p.fillInPluginDescription(looperProcDesc);
        looperProcDesc.category = "Built-in";
    }

    {
        TunerProcessor p;
        p.fillInPluginDescription(tunerProcDesc);
        tunerProcDesc.category = "Built-in";
    }

    {
        ToneGeneratorProcessor p;
        p.fillInPluginDescription(toneGenProcDesc);
        toneGenProcDesc.category = "Built-in";
    }

    {
        SplitterProcessor p;
        p.fillInPluginDescription(splitterProcDesc);
        splitterProcDesc.category = "Built-in";
    }

    {
        MixerProcessor p;
        p.fillInPluginDescription(mixerProcDesc);
        mixerProcDesc.category = "Built-in";
    }

    {
        IRLoaderProcessor p;
        p.fillInPluginDescription(irLoaderProcDesc);
        irLoaderProcDesc.category = "Effects";
    }

    {
        NAMProcessor p;
        p.fillInPluginDescription(namProcDesc);
        namProcDesc.category = "Built-in";
    }

    {
        MidiTransposeProcessor p;
        p.fillInPluginDescription(midiTransposeProcDesc);
        midiTransposeProcDesc.category = "MIDI Utility";
    }

    {
        MidiRechannelizeProcessor p;
        p.fillInPluginDescription(midiRechannelizeProcDesc);
        midiRechannelizeProcDesc.category = "MIDI Utility";
    }

    {
        KeyboardSplitProcessor p;
        p.fillInPluginDescription(keyboardSplitProcDesc);
        keyboardSplitProcDesc.category = "MIDI Utility";
    }

    {
        NotesProcessor p;
        p.fillInPluginDescription(notesProcDesc);
        notesProcDesc.category = "Built-in";
    }

    {
        LabelProcessor p;
        p.fillInPluginDescription(labelProcDesc);
        labelProcDesc.category = "Built-in";
    }

    {
        MidiFilePlayerProcessor p;
        p.fillInPluginDescription(midiFilePlayerProcDesc);
        midiFilePlayerProcDesc.category = "Built-in";
    }

    {
        SubGraphProcessor p;
        p.fillInPluginDescription(subGraphProcDesc);
        subGraphProcDesc.category = "Built-in";
    }
}

AudioPluginInstance* InternalPluginFormat::createInstanceFromDescription(const PluginDescription& desc)
{
    if (desc.name == audioOutDesc.name)
    {
        return new AudioProcessorGraph::AudioGraphIOProcessor(
            AudioProcessorGraph::AudioGraphIOProcessor::audioOutputNode);
    }
    else if (desc.name == audioInDesc.name)
    {
        return new AudioProcessorGraph::AudioGraphIOProcessor(
            AudioProcessorGraph::AudioGraphIOProcessor::audioInputNode);
    }
    else if (desc.name == midiInDesc.name)
    {
        return new AudioProcessorGraph::AudioGraphIOProcessor(
            AudioProcessorGraph::AudioGraphIOProcessor::midiInputNode);
    }
    else if (desc.name == midiInterceptorDesc.name)
    {
        return new MidiInterceptor();
    }
    else if (desc.name == oscInputDesc.name)
    {
        return new OscInput();
    }
    else if (desc.name == levelProcDesc.name)
    {
        return new LevelProcessor();
    }
    else if (desc.name == filePlayerProcDesc.name)
    {
        return new FilePlayerProcessor();
    }
    else if (desc.name == outputToggleProcDesc.name)
    {
        return new OutputToggleProcessor();
    }
    else if (desc.name == vuMeterProcDesc.name)
    {
        return new VuMeterProcessor();
    }
    else if (desc.name == recorderProcDesc.name)
    {
        return new RecorderProcessor();
    }
    else if (desc.name == metronomeProcDesc.name)
    {
        return new MetronomeProcessor();
    }
    else if (desc.name == looperProcDesc.name)
    {
        return new LooperProcessor();
    }
    else if (desc.name == tunerProcDesc.name)
    {
        return new TunerProcessor();
    }
    else if (desc.name == toneGenProcDesc.name)
    {
        return new ToneGeneratorProcessor();
    }
    else if (desc.name == splitterProcDesc.name)
    {
        return new SplitterProcessor();
    }
    else if (desc.name == mixerProcDesc.name)
    {
        return new MixerProcessor();
    }
    else if (desc.name == irLoaderProcDesc.name)
    {
        return new IRLoaderProcessor();
    }
    else if (desc.name == namProcDesc.name)
    {
        return new NAMProcessor();
    }
    else if (desc.name == midiTransposeProcDesc.name)
    {
        return new MidiTransposeProcessor();
    }
    else if (desc.name == midiRechannelizeProcDesc.name)
    {
        return new MidiRechannelizeProcessor();
    }
    else if (desc.name == keyboardSplitProcDesc.name)
    {
        return new KeyboardSplitProcessor();
    }
    else if (desc.name == notesProcDesc.name)
    {
        return new NotesProcessor();
    }
    else if (desc.name == labelProcDesc.name)
    {
        return new LabelProcessor();
    }
    else if (desc.name == midiFilePlayerProcDesc.name)
    {
        return new MidiFilePlayerProcessor();
    }
    else if (desc.name == subGraphProcDesc.name)
    {
        return new SubGraphProcessor();
    }

    return 0;
}

const PluginDescription* InternalPluginFormat::getDescriptionFor(const InternalFilterType type)
{
    switch (type)
    {
    case audioInputFilter:
        return &audioInDesc;
    case audioOutputFilter:
        return &audioOutDesc;
    case midiInputFilter:
        return &midiInDesc;
    case midiInterceptorFilter:
        return &midiInterceptorDesc;
    case oscInputFilter:
        return &oscInputDesc;
    case levelProcFilter:
        return &levelProcDesc;
    case filePlayerProcFilter:
        return &filePlayerProcDesc;
    case outputToggleProcFilter:
        return &outputToggleProcDesc;
    case vuMeterProcFilter:
        return &vuMeterProcDesc;
    case recorderProcFilter:
        return &recorderProcDesc;
    case metronomeProcFilter:
        return &metronomeProcDesc;
    case looperProcFilter:
        return &looperProcDesc;
    case tunerProcFilter:
        return &tunerProcDesc;
    case toneGenProcFilter:
        return &toneGenProcDesc;
    case splitterProcFilter:
        return &splitterProcDesc;
    case mixerProcFilter:
        return &mixerProcDesc;
    case irLoaderProcFilter:
        return &irLoaderProcDesc;
    case namProcFilter:
        return &namProcDesc;
    case midiTransposeProcFilter:
        return &midiTransposeProcDesc;
    case midiRechannelizeProcFilter:
        return &midiRechannelizeProcDesc;
    case keyboardSplitProcFilter:
        return &keyboardSplitProcDesc;
    case notesProcFilter:
        return &notesProcDesc;
    case labelProcFilter:
        return &labelProcDesc;
    case midiFilePlayerProcFilter:
        return &midiFilePlayerProcDesc;
    case subGraphProcFilter:
        return &subGraphProcDesc;
    default:
        return 0;
    }
}

void InternalPluginFormat::getAllTypes(OwnedArray<PluginDescription>& results)
{
    for (int i = 0; i < (int)endOfFilterTypes; ++i)
        results.add(new PluginDescription(*getDescriptionFor((InternalFilterType)i)));
}

void InternalPluginFormat::getUserFacingTypes(OwnedArray<PluginDescription>& results)
{
    // Only include processors that users can add from the plugin menu
    // Excludes: audioInputFilter, audioOutputFilter, midiInputFilter, midiInterceptorFilter, oscInputFilter
    static const InternalFilterType userFacingTypes[] = {
        levelProcFilter,         filePlayerProcFilter,       outputToggleProcFilter,  vuMeterProcFilter,
        recorderProcFilter,      metronomeProcFilter,        looperProcFilter,        tunerProcFilter,
        toneGenProcFilter,       splitterProcFilter,         mixerProcFilter,         irLoaderProcFilter,
        namProcFilter,           midiTransposeProcFilter,    midiRechannelizeProcFilter, keyboardSplitProcFilter,
        notesProcFilter,         labelProcFilter,            midiFilePlayerProcFilter,   subGraphProcFilter};

    for (auto type : userFacingTypes)
        results.add(new PluginDescription(*getDescriptionFor(type)));
}
