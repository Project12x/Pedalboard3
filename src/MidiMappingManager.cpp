//	MidiMappingManager.cpp - Class which dispatches MIDI CC messages to
//						     MidiMappings.
//	----------------------------------------------------------------------------
//	This file is part of Pedalboard3, an audio plugin host.
//	Copyright (c) 2011 Niall Moody.
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

#include "MidiMappingManager.h"

#include "LogFile.h"
#include "MainPanel.h"
#include "SettingsManager.h"

#include <spdlog/spdlog.h>


using namespace std;

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
MidiMapping::MidiMapping(MidiMappingManager* manager, FilterGraph* graph, uint32 pluginId, int param, int midiCc,
                         bool latch, int chan, float lower, float upper)
    : Mapping(graph, pluginId, param), mappingManager(manager), cc(midiCc), latched(latch), channel(chan),
      lowerBound(lower), upperBound(upper), latchVal(0.0f), latchToggle(false)
{
    // mappingManager->registerMapping(midiCc, this);
}

//------------------------------------------------------------------------------
MidiMapping::MidiMapping(MidiMappingManager* manager, FilterGraph* graph, XmlElement* e)
    : Mapping(graph, e), mappingManager(manager), channel(0), latchVal(0.0f), latchHi(1.0f), latchLo(0.0f),
      latchToggle(false)
{
    if (e)
    {
        cc = e->getIntAttribute("cc");
        latched = e->getBoolAttribute("latch");
        channel = e->getIntAttribute("channe");
        lowerBound = (float)e->getDoubleAttribute("lowerBound");
        upperBound = (float)e->getDoubleAttribute("upperBound");
    }
    // mappingManager->registerMapping(cc, this);
}

//------------------------------------------------------------------------------
MidiMapping::~MidiMapping()
{
    mappingManager->unregisterMapping(this);
}

//------------------------------------------------------------------------------
void MidiMapping::ccReceived(int val)
{
    float tempf;

    if (latched)
    {
        if (val == 0)
            return;

        latchToggle = !latchToggle;

        /*if(latchToggle)
        {
            latchHi = (float)val/127.0f;

            if(latchVal > latchLo)
                latchVal = latchLo;
            else
                latchVal = latchHi;
        }
        else
            latchLo = (float)val/127.0f;

        tempf = latchVal;*/
        if (latchToggle)
            tempf = 1.0f;
        else
            tempf = 0.0f;
    }
    else
        tempf = (float)val / 127.0f;

    if (upperBound > lowerBound)
    {
        tempf *= upperBound - lowerBound;
        tempf += lowerBound;
    }
    else
    {
        tempf = 1.0f - tempf;
        tempf *= lowerBound - upperBound;
        tempf += upperBound;
    }

    updateParameter(tempf);
}

//------------------------------------------------------------------------------
XmlElement* MidiMapping::getXml() const
{
    XmlElement* retval = new XmlElement("MidiMapping");

    retval->setAttribute("pluginId", (int)getPluginId());
    retval->setAttribute("parameter", getParameter());
    retval->setAttribute("cc", cc);
    retval->setAttribute("latch", latched);
    retval->setAttribute("channe", channel);
    retval->setAttribute("lowerBound", lowerBound);
    retval->setAttribute("upperBound", upperBound);

    return retval;
}

//------------------------------------------------------------------------------
void MidiMapping::setCc(int val)
{
    cc = val;
    mappingManager->unregisterMapping(this);
    mappingManager->registerMapping(cc, this);
}

//------------------------------------------------------------------------------
void MidiMapping::setLatched(bool val)
{
    latched = val;
}

//------------------------------------------------------------------------------
void MidiMapping::setChannel(int val)
{
    channel = val;
}

//------------------------------------------------------------------------------
void MidiMapping::setLowerBound(float val)
{
    lowerBound = val;
}

//------------------------------------------------------------------------------
void MidiMapping::setUpperBound(float val)
{
    upperBound = val;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
MidiAppMapping::MidiAppMapping(MidiMappingManager* manager, int midiCc, CommandID commandId)
    : midiManager(manager), cc(midiCc), id(commandId)
{
    // midiManager->registerAppMapping(this);
}

//------------------------------------------------------------------------------
MidiAppMapping::MidiAppMapping(MidiMappingManager* manager, XmlElement* e) : midiManager(manager)
{
    if (e)
    {
        cc = e->getIntAttribute("cc");
        id = e->getIntAttribute("commandId");
    }
    // midiManager->registerAppMapping(this);
}

//------------------------------------------------------------------------------
MidiAppMapping::~MidiAppMapping()
{
    midiManager->unregisterAppMapping(this);
}

//------------------------------------------------------------------------------
XmlElement* MidiAppMapping::getXml() const
{
    XmlElement* retval = new XmlElement("MidiAppMapping");

    retval->setAttribute("cc", cc);
    retval->setAttribute("commandId", id);

    return retval;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
MidiMappingManager::MidiMappingManager(ApplicationCommandManager* manager) : appManager(manager), midiLearnCallback(0)
{
}

//------------------------------------------------------------------------------
MidiMappingManager::~MidiMappingManager()
{
    unsigned int i;
    std::vector<MidiMapping*> tempMappings;
    std::vector<MidiAppMapping*> tempAppMappings;
    std::multimap<int, MidiMapping*>::iterator it;
    std::multimap<int, MidiAppMapping*>::iterator it2;

    for (it = mappings.begin(); it != mappings.end(); ++it)
        tempMappings.push_back(it->second);
    for (it2 = appMappings.begin(); it2 != appMappings.end(); ++it2)
        tempAppMappings.push_back(it2->second);

    for (i = 0; i < tempMappings.size(); ++i)
        delete tempMappings[i];
    for (i = 0; i < tempAppMappings.size(); ++i)
        delete tempAppMappings[i];
}

//------------------------------------------------------------------------------
void MidiMappingManager::midiCcReceived(const MidiMessage& message, double secondsSinceStart)
{
    if (LogFile::getInstance().getIsLogging())
    {
        String tempstr;

        if (message.isController())
        {
            tempstr << "MIDI CC message received: CC=" << message.getControllerNumber();
            tempstr << " val=" << message.getControllerValue();
            tempstr << " chan=" << message.getChannel();
        }
        else if (message.isNoteOn())
        {
            tempstr << "MIDI Note On message received: note=" << message.getNoteNumber();
            tempstr << " vel=" << (int)message.getVelocity();
            tempstr << " chan=" << message.getChannel();
        }
        else if (message.isNoteOff())
        {
            tempstr << "MIDI Note Off message received: note=" << message.getNoteNumber();
            tempstr << " vel=" << (int)message.getVelocity();
            tempstr << " chan=" << message.getChannel();
        }
        else if (message.isProgramChange())
        {
            tempstr << "MIDI Program Change message received: prog=" << message.getProgramChangeNumber();
        }
        else
        {
            int i;

            tempstr << "MIDI message received: ";
            for (i = 0; i < message.getRawDataSize(); ++i)
                tempstr << String::toHexString(message.getRawData()[i]);
        }

        LogFile::getInstance().logEvent("MIDI", tempstr);
    }

    if (message.isController())
    {
        int mappingChan;
        multimap<int, MidiMapping*>::iterator it;
        multimap<int, MidiAppMapping*>::iterator it2;
        int cc = message.getControllerNumber();
        int value = message.getControllerValue();
        int messageChan = message.getChannel();

        if (midiLearnCallback)
        {
            midiLearnCallback->midiCcReceived(cc);
            midiLearnCallback = 0;
        }

        // Check if it matches any MidiMappings.
        for (it = mappings.lower_bound(cc); it != mappings.upper_bound(cc); ++it)
        {
            mappingChan = it->second->getChannel();
            if ((mappingChan == 0) || (mappingChan == messageChan))
                it->second->ccReceived(value);
        }

        if (value > 64)
        {
            // Check if it matches any MidiAppMappings.
            spdlog::debug("[MIDI] CC{} value {} - checking appMappings (count={})",
                          cc, value, appMappings.count(cc));
            for (it2 = appMappings.lower_bound(cc); it2 != appMappings.upper_bound(cc); ++it2)
            {
                CommandID id = it2->second->getId();
                MainPanel* panel =
                    dynamic_cast<MainPanel*>(appManager->getFirstCommandTarget(MainPanel::TransportPlay));

                spdlog::debug("[MIDI] Found appMapping CC{} -> CommandID={}, panel={}",
                              cc, (int)id, panel != nullptr);

                if (panel)
                {
                    if (id != MainPanel::TransportTapTempo)
                    {
                        spdlog::info("[MIDI] Invoking command {} from CC{}", (int)id, cc);
                        panel->invokeCommandFromOtherThread(id);
                    }
                    else
                    {
                        double tempo = tapHelper.updateTempo(secondsSinceStart);

                        if (tempo > 0.0)
                            panel->updateTempoFromOtherThread(tempo);
                    }
                }
            }
        }
    }
    else if (message.isMidiMachineControlMessage())
    {
        if (SettingsManager::getInstance().getBool("mmcTransport", false))
        {
            CommandID id = -1;
            MainPanel* panel = dynamic_cast<MainPanel*>(appManager->getFirstCommandTarget(MainPanel::TransportPlay));

            switch (message.getMidiMachineControlCommand())
            {
            case MidiMessage::mmc_stop:
                id = MainPanel::TransportPlay;
                break;
            case MidiMessage::mmc_play:
                id = MainPanel::TransportPlay;
                break;
            case MidiMessage::mmc_rewind:
                id = MainPanel::TransportRtz;
                break;
            case MidiMessage::mmc_pause:
                id = MainPanel::TransportPlay;
                break;
            default:
                break;
            }
            if ((id > -1) && panel)
                panel->invokeCommandFromOtherThread(id);
        }
    }
    else if (message.isProgramChange())
    {
        if (SettingsManager::getInstance().getBool("midiProgramChange", false))
        {
            int newPatch;
            MainPanel* panel = dynamic_cast<MainPanel*>(appManager->getFirstCommandTarget(MainPanel::TransportPlay));

            newPatch = message.getProgramChangeNumber();

            if (panel)
                panel->switchPatchFromProgramChange(newPatch);
        }
    }
}

//------------------------------------------------------------------------------
void MidiMappingManager::registerMapping(int midiCc, MidiMapping* mapping)
{
    jassert(mapping);

    mappings.insert(make_pair(midiCc, mapping));
}

//------------------------------------------------------------------------------
void MidiMappingManager::unregisterMapping(MidiMapping* mapping)
{
    multimap<int, MidiMapping*>::iterator it;

    jassert(mapping);

    for (it = mappings.begin(); it != mappings.end();)
    {
        if (it->second == mapping)
            mappings.erase(it++); // Pass the current iterator to erase, then increment it before erase() is executed,
                                  // so it's not invalidated by the erase() call.
        else
            ++it; // Pre-increment because it should be more efficient.
    }
}

//------------------------------------------------------------------------------
void MidiMappingManager::registerAppMapping(MidiAppMapping* mapping)
{
    jassert(mapping);

    appMappings.insert(make_pair(mapping->getCc(), mapping));
}

//------------------------------------------------------------------------------
void MidiMappingManager::unregisterAppMapping(MidiAppMapping* mapping)
{
    multimap<int, MidiAppMapping*>::iterator it;

    jassert(mapping);

    for (it = appMappings.begin(); it != appMappings.end();)
    {
        if (it->second == mapping)
            appMappings.erase(it++); // Pass the current iterator to erase, then increment it before erase() is
                                     // executed, so it's not invalidated by the erase() call.
        else
            ++it; // Pre-increment because it should be more efficient.
    }
}

//------------------------------------------------------------------------------
MidiAppMapping* MidiMappingManager::getAppMapping(int index)
{
    int i = 0;
    MidiAppMapping* retval = 0;
    multimap<int, MidiAppMapping*>::iterator it;

    for (it = appMappings.begin(); it != appMappings.end(); ++it)
    {
        if (i == index)
        {
            retval = it->second;
            break;
        }
        ++i;
    }

    return retval;
}

//------------------------------------------------------------------------------
void MidiMappingManager::registerMidiLearnCallback(MidiLearnCallback* callback)
{
    midiLearnCallback = callback;
}

//------------------------------------------------------------------------------
void MidiMappingManager::unregisterMidiLearnCallback(MidiLearnCallback* callback)
{
    midiLearnCallback = 0;
}

//------------------------------------------------------------------------------
const StringArray MidiMappingManager::getCCNames()
{
    StringArray retval;

    retval.add("0: Bank Select");
    retval.add("1: Mod Wheel");
    retval.add("2: Breath");
    retval.add("3:");
    retval.add("4: Foot Pedal");
    retval.add("5: Portamento");
    retval.add("6: Data Entry");
    retval.add("7: Volume");
    retval.add("8: Balance");
    retval.add("9:");
    retval.add("10: Pan");
    retval.add("11: Expression");
    retval.add("12: Effect Control 1");
    retval.add("13: EffectControl 2");
    retval.add("14:");
    retval.add("15:");
    retval.add("16: General Purpose 1");
    retval.add("17: General Purpose 2");
    retval.add("18: General Purpose 3");
    retval.add("19: General Purpose 4");
    retval.add("20:");
    retval.add("21:");
    retval.add("22:");
    retval.add("23:");
    retval.add("24:");
    retval.add("25:");
    retval.add("26:");
    retval.add("27:");
    retval.add("28:");
    retval.add("29:");
    retval.add("30:");
    retval.add("31:");
    retval.add("32: Bank Select (fine)");
    retval.add("33: Mod Wheel (fine)");
    retval.add("34: Breath (fine)");
    retval.add("35:");
    retval.add("36: Foot Pedal (fine)");
    retval.add("37: Portamento (fine)");
    retval.add("38: Data Entry (fine)");
    retval.add("39: Volume (fine)");
    retval.add("40: Balance (fine)");
    retval.add("41:");
    retval.add("42: Pan (fine)");
    retval.add("43: Expression (fine)");
    retval.add("44: Effect Control 1 (fine)");
    retval.add("45: Effect Control 2 (fine)");
    retval.add("46:");
    retval.add("47:");
    retval.add("48:");
    retval.add("49:");
    retval.add("50:");
    retval.add("51:");
    retval.add("52:");
    retval.add("53:");
    retval.add("54:");
    retval.add("55:");
    retval.add("56:");
    retval.add("57:");
    retval.add("58:");
    retval.add("59:");
    retval.add("60:");
    retval.add("61:");
    retval.add("62:");
    retval.add("63:");
    retval.add("64: Hold Pedal");
    retval.add("65: Portamento (on/off)");
    retval.add("66: Sustenuto Pedal");
    retval.add("67: Soft Pedal");
    retval.add("68: Legato Pedal");
    retval.add("69: Hold 2 Pedal");
    retval.add("70: Sound Variation");
    retval.add("71: Sound Timbre");
    retval.add("72: Sound Release Time");
    retval.add("73: Sound Attack Time");
    retval.add("74: Sound Brightness");
    retval.add("75: Sound Control 6");
    retval.add("76: Sound Control 7");
    retval.add("77: Sound Control 8");
    retval.add("78: Sound Control 9");
    retval.add("79: Sound Control 10");
    retval.add("80: General Purpose Button 1");
    retval.add("81: General Purpose Button 2");
    retval.add("82: General Purpose Button 3");
    retval.add("83: General Purpose Button 4");
    retval.add("84:");
    retval.add("85:");
    retval.add("86:");
    retval.add("87:");
    retval.add("88:");
    retval.add("89:");
    retval.add("90:");
    retval.add("91: Effects Level");
    retval.add("92: Tremolo Level");
    retval.add("93: Chorus Level");
    retval.add("94: Celeste Level");
    retval.add("95: Phaser Level");
    retval.add("96: Data Button Inc");
    retval.add("97: Data Button Dec");
    retval.add("98: NRPN (fine)");
    retval.add("99: NRPN (coarse)");
    retval.add("100: RPN (fine)");
    retval.add("101: RPN (coarse)");
    retval.add("102:");
    retval.add("103:");
    retval.add("104:");
    retval.add("105:");
    retval.add("106:");
    retval.add("107:");
    retval.add("108:");
    retval.add("109:");
    retval.add("110:");
    retval.add("111:");
    retval.add("112:");
    retval.add("113:");
    retval.add("114:");
    retval.add("115:");
    retval.add("116:");
    retval.add("117:");
    retval.add("118:");
    retval.add("119:");
    retval.add("120: All Sound Off");
    retval.add("121: All Controllers Off");
    retval.add("122: Local Keyboard");
    retval.add("123: All Notes Off");
    retval.add("124: Omni Mode Off");
    retval.add("125: Omni Mode On");
    retval.add("126: Mono Operation");
    retval.add("127: Poly Operation");

    return retval;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
MidiInterceptor::MidiInterceptor() : midiManager(0), samplesSinceStart(0) {}

//------------------------------------------------------------------------------
MidiInterceptor::~MidiInterceptor() {}

//------------------------------------------------------------------------------
void MidiInterceptor::setManager(MidiMappingManager* manager)
{
    midiManager = manager;
}

//------------------------------------------------------------------------------
void MidiInterceptor::fillInPluginDescription(PluginDescription& description) const
{
    description.name = "Midi Interceptor";
    description.descriptiveName = "Hidden Midi Interceptor plugin for mapping MIDI CCs to parameters.";
    description.pluginFormatName = "Internal";
    description.category = "Internal";
    description.manufacturerName = "Niall Moody";
    description.version = "1.00";
    description.uniqueId = description.name.hashCode();
    description.isInstrument = true; //?
    description.numInputChannels = 0;
    description.numOutputChannels = 0;
}

//------------------------------------------------------------------------------
void MidiInterceptor::processBlock(AudioSampleBuffer& buffer, MidiBuffer& midiMessages)
{
    int samplePos;
    double seconds;
    MidiMessage tempMess(0xf0);
    MidiBuffer::Iterator it(midiMessages);
    const double sampleRate = getSampleRate();

    jassert(sampleRate > 0.0);

    if (midiManager)
    {
        int numMess = midiMessages.getNumEvents();

        if (numMess > 0)
            numMess = numMess;

        while (it.getNextEvent(tempMess, samplePos))
        {
            seconds = (double)(samplesSinceStart + samplePos) / sampleRate;
            midiManager->midiCcReceived(tempMess, seconds);
        }
    }

    samplesSinceStart += buffer.getNumSamples();

    midiMessages.clear();
}
