//	MidiAppFifo.cpp - A lock-free FIFO used to pass messages from the audio
//					  thread to the message thread.
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

#include "MidiAppFifo.h"

//------------------------------------------------------------------------------
MidiAppFifo::MidiAppFifo()
{
}

//------------------------------------------------------------------------------
MidiAppFifo::~MidiAppFifo()
{

}

//------------------------------------------------------------------------------
bool MidiAppFifo::writeID(CommandID id)
{
	const bool success = idFifo.tryWrite(id);
	return recordWriteResult(success, idFifo.getNumReady());
}

//------------------------------------------------------------------------------
CommandID MidiAppFifo::readID()
{
	CommandID retval = -1;

	idFifo.tryRead(retval);

	return retval;
}

//------------------------------------------------------------------------------
bool MidiAppFifo::writeTempo(double tempo)
{
	const bool success = tempoFifo.tryWrite(tempo);
	return recordWriteResult(success, tempoFifo.getNumReady());
}

//------------------------------------------------------------------------------
double MidiAppFifo::readTempo()
{
	double retval = 120.0;

	tempoFifo.tryRead(retval);

	return retval;
}

//------------------------------------------------------------------------------
bool MidiAppFifo::writePatchChange(int tempo)
{
	const bool success = patchChangeFifo.tryWrite(tempo);
	return recordWriteResult(success, patchChangeFifo.getNumReady());
}

//------------------------------------------------------------------------------
int MidiAppFifo::readPatchChange()
{
	int retval = 0;

	patchChangeFifo.tryRead(retval);

	return retval;
}

//------------------------------------------------------------------------------
bool MidiAppFifo::writeParamChange(FilterGraph* graph, uint32 pluginId, int paramIndex, float value)
{
	const PendingParamChange change{graph, pluginId, paramIndex, value};
	const bool success = paramChangeFifo.tryWrite(change);
	return recordWriteResult(success, paramChangeFifo.getNumReady());
}

//------------------------------------------------------------------------------
bool MidiAppFifo::readParamChange(PendingParamChange& out)
{
	return paramChangeFifo.tryRead(out);
}

//------------------------------------------------------------------------------
void MidiAppFifo::resetDiagnostics() noexcept
{
	droppedEvents.store(0, std::memory_order_relaxed);
	lastOverflowTick.store(0, std::memory_order_relaxed);
	maxDepth.store(getCurrentMaxDepth(), std::memory_order_relaxed);
}

//------------------------------------------------------------------------------
bool MidiAppFifo::recordWriteResult(bool success, int depth) noexcept
{
	const auto tick = writeAttemptTick.fetch_add(1, std::memory_order_relaxed) + 1;

	if (!success)
	{
		updateMaxDepth(depth);
		droppedEvents.fetch_add(1, std::memory_order_relaxed);
		lastOverflowTick.store(tick, std::memory_order_relaxed);
		return false;
	}

	updateMaxDepth(depth);
	return true;
}

//------------------------------------------------------------------------------
void MidiAppFifo::updateMaxDepth(int depth) noexcept
{
	int observed = maxDepth.load(std::memory_order_relaxed);

	while (depth > observed
		   && !maxDepth.compare_exchange_weak(observed, depth, std::memory_order_relaxed,
											  std::memory_order_relaxed))
	{
	}
}

//------------------------------------------------------------------------------
int MidiAppFifo::getCurrentMaxDepth() const noexcept
{
	int depth = idFifo.getNumReady();
	depth = juce::jmax(depth, tempoFifo.getNumReady());
	depth = juce::jmax(depth, patchChangeFifo.getNumReady());
	depth = juce::jmax(depth, paramChangeFifo.getNumReady());
	return depth;
}
