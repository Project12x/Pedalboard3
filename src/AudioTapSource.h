//	AudioTapSource.h - Mixin giving a graph node the ability to publish its own
//					   rendered audio as a per-node Ableton Link Audio sink.
//	----------------------------------------------------------------------------
//	This file is part of Pedalboard3, an audio plugin host.
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

#ifndef AUDIOTAPSOURCE_H_
#define AUDIOTAPSOURCE_H_

#include "LinkAudioService.h"

#include <JuceHeader.h>
#include <atomic>

/// Mixin for graph nodes that can be individually opted in to publish their
/// own rendered audio as a named Ableton Link Audio sink, alongside the
/// existing master-bus sink. Implemented by BypassableInstance (covers every
/// VST3/AU plugin and built-in effect processor) and
/// TappableAudioGraphIOProcessor (covers the two device I/O nodes). Not every
/// graph node implements this - MIDI-only nodes and the nested-graph
/// SubGraphProcessor (Effect Rack) are out of scope.
class AudioTapSource
{
  public:
    virtual ~AudioTapSource() = default;

    /// Set by the message thread (under the owning graph's callback lock when
    /// clearing an active slot - see FilterGraph::removeFilterRaw /
    /// PluginComponent's Link Audio toggle). Read every block by the audio
    /// thread. nullptr means "not opted in" - the tap is a no-op.
    void setLinkAudioSinkSlot(LinkAudioService::NodeSinkGroup* slot) noexcept
    {
        linkAudioSinkSlot.store(slot, std::memory_order_release);
    }

    LinkAudioService::NodeSinkGroup* getLinkAudioSinkSlot() const noexcept
    {
        return linkAudioSinkSlot.load(std::memory_order_acquire);
    }

  protected:
    /// Call from the end of processBlock() with the node's own final rendered
    /// buffer. RT-safe: a single atomic load plus (when opted in) writes into
    /// pre-allocated Link Audio sink buffers - no allocation, no locking.
    void publishToLinkAudioIfTapped(const AudioSampleBuffer& buffer) noexcept
    {
        auto* slot = linkAudioSinkSlot.load(std::memory_order_acquire);
        if (slot == nullptr)
            return;
        // Same atomic-singleton-load pattern MeteringProcessorPlayer and
        // LinkAudioInputProcessor already use to reach the one LinkAudioService
        // instance from the audio thread.
        if (auto* service = LinkAudioService::getActiveInstance())
            service->publishNodeAudio(slot, buffer.getArrayOfReadPointers(), buffer.getNumChannels(),
                                      buffer.getNumSamples());
    }

  private:
    std::atomic<LinkAudioService::NodeSinkGroup*> linkAudioSinkSlot{nullptr};
};

#endif
