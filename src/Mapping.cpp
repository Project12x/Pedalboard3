//	Mapping.cpp - The various mapping classes.
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

#include "Mapping.h"
#include "BypassableInstance.h"
#include "FilterGraph.h"
#include "MidiAppFifo.h"

MidiAppFifo* Mapping::paramFifo = nullptr;

//------------------------------------------------------------------------------
void Mapping::setParamFifo(MidiAppFifo* fifo) { paramFifo = fifo; }

//------------------------------------------------------------------------------
Mapping::Mapping(FilterGraph *graph, uint32 pluginId, int param)
    : filterGraph(graph), plugin(pluginId), parameter(param) {}

//------------------------------------------------------------------------------
Mapping::Mapping(FilterGraph *graph, XmlElement *e) : filterGraph(graph) {
  if (e) {
    // We cheat a bit and don't check the tag name, so we can use MidiMapping
    // and OSCMappings.
    plugin = e->getIntAttribute("pluginId");
    parameter = e->getIntAttribute("parameter");
  }
}

//------------------------------------------------------------------------------
Mapping::~Mapping() {}

//------------------------------------------------------------------------------
void Mapping::updateParameter(float val) {
  // Defer to message thread via lock-free FIFO (RT-safe).
  if (paramFifo) {
    paramFifo->writeParamChange(filterGraph, plugin, parameter, val);
    return;
  }

  // Fallback: direct dispatch (non-RT-safe, only before MainPanel wires the FIFO).
  jassert(paramFifo != nullptr); // If this fires, FIFO was never wired - check init order.
  auto node = filterGraph->getNodeForId(AudioProcessorGraph::NodeID(plugin));
  if (!node)
    return;
  auto* filter = node->getProcessor();

  if (parameter == -1) {
    BypassableInstance *bypassable = dynamic_cast<BypassableInstance *>(filter);

    if (bypassable)
      bypassable->setBypass(val > 0.5f);
  } else
    filter->setParameter(parameter, val);
}

//------------------------------------------------------------------------------
void Mapping::setParameter(int val) { parameter = val; }
