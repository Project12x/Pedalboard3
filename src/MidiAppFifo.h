//	MidiAppFifo.h - A lock-free FIFO used to pass messages from the audio
//					thread to the message thread.
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

#ifndef MIDIAPPFIFO_H_
#define MIDIAPPFIFO_H_

#include <JuceHeader.h>

class FilterGraph;

///	A lock-free FIFO used to pass messages from the audio thread to the message thread.
class MidiAppFifo
{
  public:
	///	A deferred parameter change queued from the audio thread.
	struct PendingParamChange
	{
		FilterGraph* graph;
		uint32 pluginId;
		int paramIndex;   // -1 = bypass
		float value;
	};

	///	Constructor.
	MidiAppFifo();
	///	Destructor.
	~MidiAppFifo();

	///	Writes a CommandID to the FIFO.
	void writeID(CommandID id);
	///	Reads a CommandID from the FIFO.
	CommandID readID();
	///	Returns the number of IDs waiting in the FIFO.
	int getNumWaitingID() const {return idFifo.getNumReady();};

	///	Writes a new tempo to the FIFO.
	void writeTempo(double tempo);
	///	Reads a tempo from the FIFO.
	double readTempo();
	///	Returns the number of tempos waiting in the FIFO.
	int getNumWaitingTempo() const {return tempoFifo.getNumReady();};

	///	Writes a patch change to the FIFO.
	void writePatchChange(int index);
	///	Reads a patch change from the FIFO.
	int readPatchChange();
	///	Returns the number of patch changes waiting in the FIFO.
	int getNumWaitingPatchChange() const {return patchChangeFifo.getNumReady();};

	///	Writes a deferred parameter change to the FIFO (audio thread).
	void writeParamChange(FilterGraph* graph, uint32 pluginId, int paramIndex, float value);
	///	Reads a deferred parameter change from the FIFO (message thread).
	bool readParamChange(PendingParamChange& out);
	///	Returns the number of parameter changes waiting in the FIFO.
	int getNumWaitingParamChange() const {return paramChangeFifo.getNumReady();};

  private:
	///	The size of the buffers.
	enum
	{
		BufferSize = 1024
	};

	///	Protects all write operations for multi-producer safety.
	///	AbstractFifo is SPSC; multiple threads (MIDI audio + OSC network) may
	///	write concurrently, so we serialise the producer side with a SpinLock.
	///	SpinLock is RT-safe (busy-wait, no OS blocking).
	juce::SpinLock writeLock;

	///	The CommandID fifo.
	AbstractFifo idFifo;
	///	The CommandID buffer.
	CommandID idBuffer[BufferSize];

	///	The tempo fifo.
	AbstractFifo tempoFifo;
	///	The tempo buffer.
	double tempoBuffer[BufferSize];

	///	The patch change fifo.
	AbstractFifo patchChangeFifo;
	///	The patch change buffer.
	int patchChangeBuffer[BufferSize];

	///	The parameter change fifo.
	AbstractFifo paramChangeFifo;
	///	The parameter change buffer.
	PendingParamChange paramChangeBuffer[BufferSize];
};

#endif
