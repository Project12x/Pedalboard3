# Pedalboard3 Architecture

> **For LLMs and Engineers:** This document describes the codebase structure,
> key patterns, and JUCE 8/9 migration details. For current product status and
> the active task, start with [ROADMAP.md](ROADMAP.md) and
> [STATE_OF_PROJECT.md](STATE_OF_PROJECT.md); this is an implementation
> reference, not the release/status source of truth.

---

## Overview

Pedalboard3 is a **JUCE-based audio plugin host** for live performance. It allows:
- Loading audio plugins (VST3) into a signal graph
- MIDI/OSC control of parameters and routing
- Patch management for live performance switching

---

## Directory Structure

```
src/
├── App.cpp/h                 # Application entry, window management
├── MainPanel.cpp/h           # Main UI container, menu bar, commands
├── FilterGraph.cpp/h         # AudioProcessorGraph wrapper
├── PluginField.cpp/h         # Canvas for plugin nodes and connections
├── PluginComponent.cpp/h     # Individual plugin node UI
├── BranchesLAF.cpp/h         # Custom LookAndFeel
├── ColourScheme.cpp/h        # Theme colors
│
├── MidiMappingManager.cpp/h  # MIDI CC → parameter mapping
├── OscMappingManager.cpp/h   # OSC → parameter mapping
├── Mapping.h                 # Base mapping interface
│
├── PedalboardProcessors.cpp/h    # Built-in audio processors
├── PedalboardProcessorEditors.cpp/h  # Editors for built-in processors
├── BypassableInstance.cpp/h  # Wrapper adding bypass to plugins
│
└── [other support files]
```

---

## Core Components

### FilterGraph (Audio Engine)

Wraps JUCE's `AudioProcessorGraph` to manage the plugin signal chain.

```cpp
class FilterGraph : public FileBasedDocument {
    AudioProcessorGraph graph;           // The actual graph
    AudioProcessorGraph::Node::Ptr addFilter(...);
    void removeFilter(uint32 nodeId);
    std::vector<Connection> getConnections() const;  // JUCE 8
};
```

### Infrastructure Nodes (Hidden)

FilterGraph manages hidden "infrastructure" nodes that are always present but never shown to the user or saved to patch XML:

- **SafetyLimiterProcessor** -- Final-stage limiter preventing clipping
- **CrossfadeMixerProcessor** -- Handles glitch-free crossfade during patch switches

These are created by `createInfrastructureNodes()`, which is called both in the constructor and after `graph.clear()`. The cached raw pointers (`safetyLimiter`, `crossfadeMixer`) and NodeIDs are refreshed each time.

**Critical pattern:** Any code that calls `graph.clear()` invalidates all node pointers. After clearing, `createInfrastructureNodes()` must be called to rebuild infrastructure, and any cached pointers (e.g., in `PluginFieldPersistence`) must be reacquired via `getCrossfadeMixer()`.

Infrastructure nodes are excluded from:

- `createXml()` -- not saved to patch files (`isHiddenInfrastructureNode` check)
- User-visible node iteration -- filtered by the same predicate

### PluginField (Canvas)

Component that displays and allows editing of the plugin graph.

- Manages `PluginComponent` children (one per node)
- Handles drag-and-drop plugin loading
- Draws connection cables
- Lives inside a `Viewport` in `MainPanel`

**Canvas sizing:** The PluginField must always be larger than the bounding box of all nodes, with generous padding (500px). Without this, the Viewport cannot scroll to center content or reach nodes near the edges. `changeListenerCallback()` grows the field as nodes are added/moved. `fitToScreen()` also ensures the field is large enough before setting the scroll position.

**Zoom:** `zoomLevel` is applied via `setTransform(AffineTransform::scale(zoomLevel))`. Ctrl+wheel zooms, plain wheel scrolls. `fitToScreen()` computes the zoom to fit all `PluginComponent` instances (excluding connections and hidden nodes) with 50px padding.

**Infrastructure node exclusion pattern:** Several places in `PluginComponent` and `PluginField` check node names to exclude infrastructure nodes from buttons, sizing, and UI features. The canonical set is: `"Audio Input"`, `"Audio Output"`, `"MIDI Input"`, `"Virtual MIDI Input"`, `"OSC Input"`. Note: JUCE 8 uses `"MIDI Input"` (all caps), not `"Midi Input"`.

### PluginComponent (Plugin Node)

UI for a single plugin in the graph.

- Shows plugin name, bypass button
- Connection pins for audio/MIDI
- Opens plugin editor on double-click
- Per-channel gain sliders and VU meters on Audio I/O nodes

---

## JUCE 8/9 API Patterns

### NodeID is a Struct

```cpp
// OLD (JUCE 6)
uint32 id = node->nodeID;

// NEW (JUCE 8)
uint32 id = node->nodeID.uid;  // .uid extracts integer
```

### Node::Ptr Smart Pointer

```cpp
// OLD
Node* node = graph.getNode(index);

// NEW
Node* node = graph.getNode(index).get();  // .get() for raw pointer
```

### Connection API

```cpp
// OLD
int count = getNumConnections();
Connection* c = getConnection(index);

// NEW
std::vector<Connection> connections = getConnections();
for (auto& c : connections) { ... }
```

### Ownership (unique_ptr)

```cpp
// OLD
XmlElement* xml = doc.parse(...);
delete xml;

// NEW
auto xml = doc.parse(...);  // unique_ptr, auto-deleted

// OLD
ScopedPointer<Drawable> img;

// NEW
std::unique_ptr<Drawable> img;
```

### Virtual Method Returns

```cpp
// OLD
Component* createItemComponent() override;

// NEW
std::unique_ptr<Component> createItemComponent() override;
```

### Graphics setColour/setFont Order

```cpp
// OLD (JUCE 7 - order didn't matter)
g.setFont(Font(15.0f));
g.setColour(Colours::white);
g.drawText(...);

// NEW (JUCE 8 - setColour MUST come first)
g.setColour(Colours::white);  // Color first!
g.setFont(Font(FontOptions().withHeight(15.0f)));
g.drawText(...);  // Now color is applied
```

### JUCE 9 Integration

JUCE 9 requires CMake projects to enable both `C` and `CXX` languages. Its
SVG parser accepts SVG strings or files rather than `XmlElement` instances;
use `Drawable::createFromSVGString()` for in-memory assets. Plugin editors
must be opened with `AudioProcessor::createEditorAndMakeActive()` so JUCE can
track the active editor and clear the pointer when it is destroyed.
`Font::getStringWidth*()` has moved to
`GlyphArrangement::getStringWidth()` (or `getStringWidthInt()` when integral
layout is required).

---

## Threading Model

| Thread | Components |
|--------|------------|
| **Message Thread** | UI, plugin editors, timer callbacks, FIFO drain |
| **Audio Thread** | `AudioProcessorGraph::processBlock`, `MidiInterceptor`, MIDI CC dispatch |
| **OSC Thread** | Network message receiving |

**RT-Safety Rules (Audio Thread):**

- No memory allocation (`new`, `String`, `std::vector` resize)
- No locks (`CriticalSection`, `ScopedLock`) -- use `ScopedTryLock` if unavoidable
- No file I/O (`LogFile`, `spdlog`, `File::existsAsFile`)
- No message-thread calls (`sendChangeMessage`, `triggerAsyncUpdate`, `MessageManager`)
- Use `std::atomic<>` for cross-thread scalars (float, bool, int, pointers)
- Use lock-free FIFOs (`AbstractFifo`) to pass data to the message thread

### Parameter Dispatch via FIFO

MIDI/OSC-mapped parameter changes are dispatched through `MidiAppFifo` to avoid calling `setParameter()` on the audio thread (many processors do non-RT work in `setParameter`).

```text
Audio Thread                          Message Thread
─────────────────                     ─────────────────
MidiInterceptor::processBlock         MainPanel::timerCallback (5ms)
  → midiCcReceived                      → midiAppFifo.readParamChange()
    → Mapping::updateParameter            → node->getProcessor()->setParameter()
      → midiAppFifo.writeParamChange()
```

**Key files:**

- `MidiAppFifo.h/cpp` -- Lock-free FIFO with channels for CommandID, tempo, patch change, and parameter change
- `Mapping.h/cpp` -- Static `paramFifo` pointer; `updateParameter()` queues to FIFO
- `MainPanel.cpp` -- `MidiAppTimer` case drains parameter FIFO on message thread

**Latency:** Up to 5ms for MIDI CC to parameter change. Audio-rate MIDI (note on/off) is unaffected.

**FIFO capacity:** 1024 entries. Writes silently drop when full (burst CC floods). For continuous expression pedal use, consider increasing `BufferSize` or decreasing timer interval.

### Double-Buffering Pattern (Display Data)

For data written by the audio thread and read by the UI (e.g., oscilloscope waveforms):

```cpp
std::array<float, N> displayBuffers[2];
std::atomic<int> frontBuffer{0};   // UI reads this index
int backBuffer = 1;                // Audio writes (audio-thread-only)

// Audio thread: write to back, swap on complete
displayBuffers[backBuffer][i] = sample;
if (captureComplete) {
    frontBuffer.store(backBuffer);
    backBuffer = 1 - backBuffer;
}

// UI thread: read from front
output = displayBuffers[frontBuffer.load()];
```

### Atomic Scalars Pattern

For simple cross-thread values (levels, flags):

```cpp
// Header
std::atomic<float> level{0.0f};

// Audio thread
float cur = level.load();
// ... compute ...
level.store(cur);

// UI thread
float display = level.load();
```

---

## Key Singletons

| Singleton | Purpose |
|-----------|---------|
| `MainTransport` | Global play/stop, tempo |
| `AudioSingletons` | Device manager, format manager |
| `SettingsManager` | **NEW** JSON-based settings (`settings.json`) |
| `PropertiesSingleton` | ~~Legacy~~ (deprecated, being removed) |
| `ColourScheme` | Current theme colors |
| `MasterGainState` | Atomic per-channel gain (dB), SmoothedValue gain ramps, master bus insert rack |
| `SafetyLimiterProcessor` | Output/input peak + VU level metering, safety limiting |

---

## Scratch Capture

Source baseline reconciled through Task 6 on 2026-10-04; focused build/tests
passed, with Task 8 acceptance still pending. Scratch Capture is app-level manual
raw/wet recording, independent of Audio Recorder nodes in the patch graph.

```text
Live capture: device input -> RAW tap -> input gains -> patch graph -> master bus
             -> output gains -> WET tap -> device output
RAW/WET taps -> ScratchRecorder -> buffered WAV writers -> take folder
MainPanel commands -> start/stop -> message-thread finalization -> take.json
ScratchPanel <- 10 Hz recorder status + immutable disk-library snapshots
```

- `MainPanel` owns the recorder and builds start-time context from the current
  patch, document, device, channel counts, sample rate, and master gains.
  `MeteringProcessorPlayer` observes it through an atomic, non-owning pointer.
- `ScratchRecorder::start()` creates the take directory, writes initial
  incomplete metadata, and opens two 24-bit WAV sinks before publishing
  `Recording`. Each sink uses a 32,768-sample buffer serviced by the recorder's
  writer thread. Missing/unsupported channels, invalid sample rate, metadata
  failure, or writer setup failure prevent a successful start.
- The live router acquires one recorder lease with `beginDeviceBlock()` before
  RAW and releases it with `endDeviceBlock()` after WET. A successful begin
  requires exactly one end, even when stop is requested between taps. File setup,
  writer teardown, JSON serialization, and UI
  work stay off the callback. `requestStop()` queues message-thread
  finalization; `finishStop()` gates new writes and waits for in-flight writes
  before destroying sinks. A 250 ms drain warning marks failure but does not
  bypass that lifetime wait.
- Finalization checks matching raw/wet sample counts and write/interruption
  errors, then publishes `Saved` or `Failed`. Patch/device changes request an
  incomplete take with an explicit reason. Shutdown requests stop, clears the
  observer pointer, and removes the audio callback before recorder destruction.
- `ScratchPanel` uses `Component::SafePointer<MainPanel>` for owner access.
  Status copies and file-availability checks belong to the message thread, not
  the audio callback.

Storage is `<root>/<YYYY-MM-DD>/<timestamp>-<sanitized-patch>/` with numeric
collision suffixes and `raw.wav`, `wet.wav`, `take.json`. The root defaults to
the user application-data directory's `Pedalboard3/Scratch Ideas`; a custom
root is persisted under `scratchRootDirectory`. New `take.json` files use schema
version 2: bundle-relative `raw.wav`/`wet.wav`/`rig.xml` names, independent
`rawDurationSamples`/`wetDurationSamples`, capture/snapshot status, and the
retained shared `durationSamples` (the smaller count). Naming `rig.xml` does not
create a snapshot: capture-time rig serialization is still upcoming.

`ScratchBundleIO` writes metadata through a sibling temporary file and replaces
the destination only after a successful write; replacement failure preserves
the old JSON. This is not a power-loss durability guarantee. `loadScratchTake()`
runs only on control/worker threads and reads legacy unversioned and v2 metadata without
rewriting it, resolves audio inside the bundle rather than following legacy
absolute paths, and leaves missing legacy frame counts unknown. It caps JSON
at 1 MiB, validates types/ranges/paths, and preflights WAV chunk bounds before
reader construction (rejecting truncation and oversized non-audio chunks).
Invalid/unsupported takes retain an inspectable result and directory; readable
audio does not relabel an interrupted take complete.

Task 2 background library: `MainPanel` owns
`ScratchTakeLibrary`, whose single worker scans exactly root/day/take depth
without recursively traversing arbitrary subtrees or following linked day/take
directories. Startup, panel open, Refresh, destination changes, and capture
finalization request scans. The finalization callback runs after the recorder's
state lock is released and is cleared during owner teardown.

The library publishes immutable, newest-first snapshots with root/generation,
entries, scanning state, and a message. A new generation invalidates old scan
results; publication rechecks that generation. Partial snapshots publish every
128 discovered entries. Corrupt/unsupported takes remain diagnostic entries;
duplicate take IDs in separate directories are not merged. Missing/inaccessible
roots produce status without scan-side directory creation. Disk reads and worker
joining stay off the audio callback. Snapshot access is a control-thread API.

Task 3 waveform/selection integration: `ScratchPanel`
uses a virtualized `ListBox` with recycled rows and actual Play/Reamp/Reveal
buttons. Selection is keyed by take directory, not row index, and survives
reordering and partial scan batches. Each panel shares a 32-entry
`AudioThumbnailCache`; raw and wet overviews use 512-sample buckets. A worker
revalidates bundles and opens WAV readers, with per-source generations rejecting
stale results and path/size/modification-time cache identities. Painting reads
cached approximate min/max values only, not WAV samples.

JUCE's native min/max quantization expands equal bounds: digital silence has
the thumbnail range `[0, 1/128]`, not an invented audio signal. Tests distinguish
that representation from a real impulse and its time region.

The selected-take view uses one RAW/WET time range and individually labelled
channels in a vertically scrollable area. Compact row previews show channel 1
of each file only, not a mixed-down or complete multichannel overview. Empty,
loading, and unavailable states are textual; synthetic traces are removed.
Task 6 connects waveform seek/playhead to the selected audition, with Original
WET/Current rig mode buttons, Pause/Resume, Stop, Loop, preview level, and a
persistent mode/live-input-replacement status label. Choosing another take or
closing the panel stops audition; selecting a row alone does not start it.

The Task 4 engine (with its own build/focused verification) separates
`ScratchPlayback` control/worker state from its single audio consumer. It is
now owned by MainPanel and connected to the device callback in Task 6 source.
`ScratchPcmQueue` uses
16 channels and 65,537 allocated frames (65,536 usable, about 4 MiB), not a
whole-file buffer. WAV decoding and JUCE resampling run only on the worker;
source/device rates are limited to 8–384 kHz and callback blocks to 8,192 frames.

Requested/published/acknowledged generations retire old audio before the worker
resets the queue. The consumer fades its held sample and acknowledges retirement;
the worker stops production before resetting and primes two maximum blocks (or
EOF) before publishing. `audioDeviceStopped()` supplies acknowledgment only
after callbacks have ceased, so teardown does not depend on another callback.
Owners must remove the consumer before preparing or destroying the engine.

Start/stop/seek, pause/resume, loop boundaries, and underflow use 5 ms fades;
underflow holds no stale queue audio. Playback position follows consumption,
not read-ahead. Non-looping reads stop at the exact remaining frame count, even
after disabling Loop or when file length is not block-aligned. The engine
publishes preview gain separately from its faded
PCM: default -12 dB, maximum unity, with -60 dB represented as mute. The Task 6
router applies it with 20 ms smoothing, then preview protection. Implicit mapping rejects
channel loss; an explicit map permits approved drops/rerouting. Mono wet mirrors
only to the first two destinations; mono raw uses only the first. Other unmapped
destinations are silent.

Task 5 (`3e441cb`) extracts the existing limiter algorithm into
`dsp/SafetyLimiterCore`; `ScratchPreviewGuard` owns independent DSP state without
replacing the registered graph limiter. It honors the graph safety mute, latches
trips, clears non-finite output, and bounds preview samples to [-1, 1]. Device
or source changes do not unlock it; explicit reset serial changes are consumed
on the processing thread.

Task 6 routing (focused verification passed): `ScratchAudioRouter`
latches one playback block/mode and uses preallocated PCM/zero/discard buffers:

| Mode | Audible path | Recording |
|---|---|---|
| Live | Hardware input → gains/graph/master bus/output gains | Paired RAW/WET lease allowed |
| Original WET | Saved wet PCM → preview gain/protection; live path receives zeros and its output is discarded | No capture lease |
| Current-rig RAW | Saved raw PCM → current input gains/graph/master bus/output gains once → preview gain/protection | No capture lease |

Both audition modes exclude hardware input and add no nodes, cables, or patch
replacement. Wet mode still advances graph/MIDI state without audible tails or
generators; the router also gates post-rig output during pause/loading/underflow.
Stopping restores live input with a short ramp. REC requests audition stop and
waits for quiescence before capture; audition is blocked while capture, saving,
or that handoff is pending. Root/patch/device changes cancel pending activity.
`playScratchTake()` requires a ready library entry and explicitly prompts for
destination-to-source mapping before dropping channels, without changing device
settings. Callback removal precedes playback teardown. Panel transport controls
are wired in source; manual UI, full integration, and hardware results are not
claimed yet.

Current boundaries:

- The recorder retains its eight-entry session history for existing callers,
  but the panel consumes all library entries with no eight-take limit. It
  retains damaged entries with reasons and Reveal.
- MainPanel Play/Reamp now route to native wet/current-rig audition rather than
  an OS player or inserted FilePlayer. They do not restore the recorded rig or
  save a reamp print. `Reveal` needs the take directory, not a complete recording.
- The waveform view describes saved audio, not a live input scope. Equal sample
  counts are checked, but there is no separate latency-alignment pass for raw
  and processed signals.

Key files: `src/ScratchRecorder.{h,cpp}`, `src/ScratchTake.{h,cpp}`,
`src/ScratchBundleIO.{h,cpp}`, `src/ScratchTakeLibrary.{h,cpp}`,
`src/ScratchWaveformView.{h,cpp}`, `src/ScratchPlayback.{h,cpp}`,
`src/ScratchPlaybackTypes.h`, `src/ScratchPcmQueue.{h,cpp}`,
`src/ScratchAudioRouter.{h,cpp}`, `src/ScratchPreviewGuard.{h,cpp}`,
`src/ScratchPanel.{h,cpp}`, and the Scratch helpers/taps in
`src/MainPanel.{h,cpp}`. Real-interface acceptance remains separate from source
inspection and historical automated results; see [HOWTO.md](HOWTO.md).

### Accepted Expansion — Native Implementation In Progress

The accepted foundations are a persistent disk-backed take library, real raw/wet
waveforms, in-app wet audition, reamp through the current rig without manually
wiring a FilePlayer, saved capture-time rig context, and modest UI polish within
the existing visual style. The written
[Scratch foundations design](docs/superpowers/specs/2026-10-02-scratch-foundations-design.md)
is approved. Native implementation of the
[implementation plan](docs/superpowers/plans/2026-10-02-scratch-foundations.md)
is now in progress. Storage Task 1 is committed as `11fe41a` with focused
verification. Task 2 (`c33748f`) final app/test build passed on 2026-10-02; the focused
library/storage run passed 328 assertions in 12 cases, with two Windows
privilege-dependent symlink cases skipped, not passed. Task 3 (`c0d0dee`) incremental
app/test build passed on 2026-10-02, followed by 53 assertions in four passed
waveform/UI cases. Task 4's final app/test build passed on 2026-10-02; its
playback/storage/waveform run passed 871 assertions in 22 cases, with one
storage-symlink case skipped for missing privilege. Task 5 shared protection is
committed as `3e441cb`, with 7,477 safety/RT assertions passing in 22 cases.
Task 6 native routing/record handoff and panel controls are source-integrated;
its final app/test build exited 0 on 2026-10-04, with 5,058 assertions in 30
passed routing/playback/safety/storage cases and one privilege-related
storage-symlink skip. Task 7 rig
snapshots and integrated UI polish remain upcoming. See the handoff for
dated evidence; manual restart/UI and hardware QA remain open.

---

## Plugin Formats

| Format | Status |
|--------|--------|
| VST2 | ❌ Deprecated, removed |
| VST3 | ✅ Primary format |
| AU | ⏳ macOS future |
| CLAP | ⏳ Future consideration |
| Internal | ✅ Built-in processors |

---

## Build System

```powershell
# Configure
cmake -B build -G "Visual Studio 17 2022"

# Build
cmake --build build --config Release
```

---

## Master Gain and Metering Architecture

### MasterGainState Singleton

Thread-safe gain state shared between audio thread and all UI surfaces.

```cpp
class MasterGainState {
public:
    static constexpr int MaxChannels = 16;
    std::atomic<float> inputChannelGainDb[MaxChannels];   // Per-channel, default 0
    std::atomic<float> outputChannelGainDb[MaxChannels];
    std::atomic<float> masterInputGainDb{0.0f};           // Master bus
    std::atomic<float> masterOutputGainDb{0.0f};

    // Gain smoothing (50ms multiplicative ramp, zipper-free)
    SmoothedValue<float, ValueSmoothingTypes::Multiplicative> smoothedInputGain;
    SmoothedValue<float, ValueSmoothingTypes::Multiplicative> smoothedOutputGain;
    void prepareSmoothing(double sampleRate);  // Called from audioDeviceAboutToStart
    void updateSmoothedTargets();              // Called once per audio block

    // Master bus insert rack (global, not per-patch)
    MasterBusProcessor& getMasterBus();
};
```

**Key files:** `MasterGainState.h`, `MasterGainState.cpp`

### MasterBusProcessor

Global insert rack at the device output level, between graph output and output gain.
Wraps a `SubGraphProcessor` for recursive plugin hosting.

- **Audio path:** Processes in `MeteringProcessorPlayer` callback, after graph, before output gain
- **RT safety:** `hasPluginsCached()` reads `std::atomic<bool>`, updated via `ChangeListener` on message thread
- **Bypass:** `setBypass()`/`isBypassed()` — empty rack is automatic passthrough
- **State:** Persisted globally via `SettingsManager`, not per-patch

**Key files:** `MasterBusProcessor.h`, `MasterBusProcessor.cpp`

### MeteringProcessorPlayer

Subclasses `AudioProcessorPlayer` to apply smoothed gain, master bus insert, and tap real device buffers.

```text
audioDeviceIOCallbackWithContext:
  1. Validate callback bounds; reject unsupported buffers before processing
  2. Router latches live/original-WET/current-RAW mode for the block
  3. Live only: begin RAW/WET recorder lease before input gains
  4. processLivePath: smoothed input gains -> graph -> master bus -> output gains
     WET mode services this path with zeros and discards its output
  5. Live: restore-live ramp, then finish WET write/release recorder lease
     Audition: apply output gate/preview gain, then independent preview guard
  6. Publish device-output levels; input levels come from processLivePath
```

**Key file:** `MainPanel.h` (inner class `MeteringProcessorPlayer`)

### VU Metering

Dual metering system: **peak** (existing) + **VU ballistic** (IEC 60268-17).

| Meter Type | Class | Integration Time | Use |
|------------|-------|-------------------|-----|
| Peak | Direct `std::abs` with decay | ~300ms decay | Peak hold indicator (thin line) |
| VU | `VuMeterDsp` (2-pole lowpass) | 300ms rise to 99% | Meter bar body (smooth) |

Both run in `SafetyLimiterProcessor::updateInputLevelsFromDevice()`/`updateOutputLevelsFromDevice()`, called from the device audio callback. Results stored in `std::atomic<float>` arrays read by UI.

**Key files:** `VuMeterDsp.h`, `SafetyLimiter.h`, `SafetyLimiter.cpp`

### UI Slider Synchronization

All gain slider instances read from `MasterGainState` atomics on their timers:

| Surface | Timer | Rate | Guard |
|---------|-------|------|-------|
| PluginComponent | `timerUpdate()` | ~30fps | `!slider->isMouseButtonDown()` |
| MainPanel footer | `CpuTimer` | ~10fps | `!slider->isMouseButtonDown()` |
| StageView | `timerCallback()` | 30fps | `!slider->isMouseButtonDown()` |

When a slider is dragged, it writes to `MasterGainState`. On next tick, all other sliders pick up the new value.

---

## Effect Rack Architecture (NEW - Feb 2026)

The Effect Rack feature allows nesting plugins within a single node using `SubGraphProcessor`.

### SubGraphProcessor

Wraps an internal `AudioProcessorGraph` to enable recursive plugin hosting.

```cpp
class SubGraphProcessor : public AudioPluginInstance {
    AudioProcessorGraph internalGraph;  // Nested graph
    AudioProcessorGraph::Node::Ptr rackAudioInput;   // Internal I/O
    AudioProcessorGraph::Node::Ptr rackAudioOutput;
    AudioProcessorGraph::Node::Ptr rackMidiInput;
    String rackName;
};
```

**Key Files:**
- `SubGraphProcessor.cpp/h` - The processor wrapping internal graph
- `SubGraphEditorComponent.cpp/h` - Rack editor UI (canvas, nodes, viewport)

### Integration Points

| Location | Modification |
|----------|-------------|
| `InternalFilters.cpp` | Registers Effect Rack via `SubGraphInternalPlugin` |
| `FilterGraph.cpp` | Special XML serialization for SubGraphProcessor |
| `FilterGraph.cpp` | **Excludes** SubGraphProcessor from `BypassableInstance` wrapping |

### SubGraphProcessor is NOT an AudioPluginInstance

SubGraphProcessor inherits from `AudioPluginInstance` but its internal I/O nodes are just `AudioProcessor`. Code that casts to `AudioPluginInstance*` will fail for these nodes. Handle explicitly:

```cpp
SubGraphProcessor* subGraph = dynamic_cast<SubGraphProcessor*>(processor);
if (subGraph != nullptr) {
    // Special handling - it's a rack, not a normal plugin
}
```

---

## Critical JUCE Patterns

### setSize() MUST Be Called LAST in Constructors

`setSize()` immediately triggers `resized()`. If child components aren't created yet, null pointer crash.

```cpp
// ❌ WRONG - crashes!
MyComponent::MyComponent() {
    setSize(800, 600);           // Triggers resized() NOW
    button = make_unique<TextButton>();  // Not created yet!
}

// ✅ CORRECT
MyComponent::MyComponent() {
    button = make_unique<TextButton>();
    addAndMakeVisible(*button);
    setSize(800, 600);           // Call LAST
}
```

### Viewport Cleanup in Destructors

Always detach Viewport's content before destruction:

```cpp
~MyEditor() {
    viewport->setViewedComponent(nullptr, false);
}
```

### CallbackLock for Graph Iteration

When iterating graph nodes from UI thread while audio may be processing:

```cpp
const ScopedLock sl(graph.getCallbackLock());
for (int i = 0; i < graph.getNumNodes(); ++i) { ... }
```

---

## Debugging

### Windows Crash Dumps

Location: `%LOCALAPPDATA%\CrashDumps\`

When app crashes, Windows creates `.dmp` files. Open in Visual Studio → "Debug with Native Only" for stack trace.

### Logging

Uses `spdlog` → `%APPDATA%\Pedalboard3\debug.log`

```cpp
#include <spdlog/spdlog.h>
spdlog::debug("[FunctionName] Message: {}", value);
spdlog::default_logger()->flush();  // Force write before crash
```

---

*Core architecture consolidated 2026-02-17. Current product state is maintained
in [ROADMAP.md](ROADMAP.md) and [STATE_OF_PROJECT.md](STATE_OF_PROJECT.md).*

