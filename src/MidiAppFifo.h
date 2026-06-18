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
#include <atomic>
#include <cstdint>

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

	///	Writes a CommandID to the FIFO. Returns false if the FIFO is full.
	bool writeID(CommandID id);
	///	Reads a CommandID from the FIFO.
	CommandID readID();
	///	Returns the number of IDs waiting in the FIFO.
	int getNumWaitingID() const {return idFifo.getNumReady();};

	///	Writes a new tempo to the FIFO. Returns false if the FIFO is full.
	bool writeTempo(double tempo);
	///	Reads a tempo from the FIFO.
	double readTempo();
	///	Returns the number of tempos waiting in the FIFO.
	int getNumWaitingTempo() const {return tempoFifo.getNumReady();};

	///	Writes a patch change to the FIFO. Returns false if the FIFO is full.
	bool writePatchChange(int index);
	///	Reads a patch change from the FIFO.
	int readPatchChange();
	///	Returns the number of patch changes waiting in the FIFO.
	int getNumWaitingPatchChange() const {return patchChangeFifo.getNumReady();};

	///	Writes a deferred parameter change to the FIFO (audio thread). Returns false if the FIFO is full.
	bool writeParamChange(FilterGraph* graph, uint32 pluginId, int paramIndex, float value);
	///	Reads a deferred parameter change from the FIFO (message thread).
	bool readParamChange(PendingParamChange& out);
	///	Returns the number of parameter changes waiting in the FIFO.
	int getNumWaitingParamChange() const {return paramChangeFifo.getNumReady();};

	///	Returns each FIFO's fixed event capacity.
	static constexpr int getCapacity() noexcept {return BufferSize;};
	///	Returns the total number of events dropped because a FIFO was full.
	std::uint64_t getDroppedEventCount() const noexcept {return droppedEvents.load(std::memory_order_relaxed);};
	///	Returns the highest observed queued depth across all FIFO lanes.
	int getMaxDepth() const noexcept {return maxDepth.load(std::memory_order_relaxed);};
	///	Returns the write-attempt sequence number of the last overflow, or 0 if none.
	std::uint64_t getLastOverflowTick() const noexcept {return lastOverflowTick.load(std::memory_order_relaxed);};
	///	Resets diagnostic counters without clearing queued events.
	void resetDiagnostics() noexcept;

  private:
	///	The size of the buffers.
	enum
	{
		BufferSize = 1024
	};

	template <typename Value>
	class BoundedQueue
	{
	  public:
		BoundedQueue() noexcept {reset();};

		void reset() noexcept
		{
			writePosition.store(0, std::memory_order_relaxed);
			readPosition.store(0, std::memory_order_relaxed);

			for (std::size_t i = 0; i < static_cast<std::size_t>(BufferSize); ++i)
			{
				slots[i].value = {};
				slots[i].sequence.store(i, std::memory_order_relaxed);
			}
		}

		bool tryWrite(const Value& value) noexcept
		{
			std::size_t position = writePosition.load(std::memory_order_relaxed);
			Slot* slot = nullptr;

			for (;;)
			{
				slot = &slots[position & Mask];
				const auto sequence = slot->sequence.load(std::memory_order_acquire);
				const auto diff = static_cast<std::intptr_t>(sequence) - static_cast<std::intptr_t>(position);

				if (diff == 0)
				{
					if (writePosition.compare_exchange_weak(position, position + 1, std::memory_order_relaxed,
															std::memory_order_relaxed))
						break;
				}
				else if (diff < 0)
				{
					return false;
				}
				else
				{
					position = writePosition.load(std::memory_order_relaxed);
				}
			}

			slot->value = value;
			slot->sequence.store(position + 1, std::memory_order_release);
			return true;
		}

		bool tryRead(Value& value) noexcept
		{
			std::size_t position = readPosition.load(std::memory_order_relaxed);
			Slot* slot = nullptr;

			for (;;)
			{
				slot = &slots[position & Mask];
				const auto sequence = slot->sequence.load(std::memory_order_acquire);
				const auto diff = static_cast<std::intptr_t>(sequence) - static_cast<std::intptr_t>(position + 1);

				if (diff == 0)
				{
					if (readPosition.compare_exchange_weak(position, position + 1, std::memory_order_relaxed,
														   std::memory_order_relaxed))
						break;
				}
				else if (diff < 0)
				{
					return false;
				}
				else
				{
					position = readPosition.load(std::memory_order_relaxed);
				}
			}

			value = slot->value;
			slot->sequence.store(position + static_cast<std::size_t>(BufferSize), std::memory_order_release);
			return true;
		}

		int getNumReady() const noexcept
		{
			const auto read = readPosition.load(std::memory_order_acquire);
			int ready = 0;

			while (ready < BufferSize)
			{
				const auto position = read + static_cast<std::size_t>(ready);
				const auto sequence = slots[position & Mask].sequence.load(std::memory_order_acquire);

				if (sequence != position + 1)
					break;

				++ready;
			}

			return ready;
		}

	  private:
		static_assert((BufferSize & (BufferSize - 1)) == 0, "BufferSize must be a power of two");
		static constexpr std::size_t Mask = static_cast<std::size_t>(BufferSize - 1);

		struct Slot
		{
			std::atomic<std::size_t> sequence{0};
			Value value{};
		};

		Slot slots[BufferSize];
		std::atomic<std::size_t> writePosition{0};
		std::atomic<std::size_t> readPosition{0};
	};

	bool recordWriteResult(bool success, int depth) noexcept;
	void updateMaxDepth(int depth) noexcept;
	int getCurrentMaxDepth() const noexcept;

	///	The CommandID fifo.
	BoundedQueue<CommandID> idFifo;
	///	The tempo fifo.
	BoundedQueue<double> tempoFifo;
	///	The patch change fifo.
	BoundedQueue<int> patchChangeFifo;
	///	The parameter change fifo.
	BoundedQueue<PendingParamChange> paramChangeFifo;

	std::atomic<std::uint64_t> droppedEvents{0};
	std::atomic<int> maxDepth{0};
	std::atomic<std::uint64_t> lastOverflowTick{0};
	std::atomic<std::uint64_t> writeAttemptTick{0};
};

#endif
