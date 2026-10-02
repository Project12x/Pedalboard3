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

Source baseline reconciled 2026-10-02. Scratch Capture is app-level manual
raw/wet recording, independent of Audio Recorder nodes in the patch graph.

```text
Device input -> RAW tap -> input gains -> patch graph -> master bus
             -> output gains -> WET tap -> device output
RAW/WET taps -> ScratchRecorder -> buffered WAV writers -> take folder
MainPanel commands -> start/stop -> message-thread finalization -> take.json
ScratchPanel <- 10 Hz status snapshot + session recent-take list
```

- `MainPanel` owns the recorder and builds start-time context from the current
  patch, document, device, channel counts, sample rate, and master gains.
  `MeteringProcessorPlayer` observes it through an atomic, non-owning pointer.
- `ScratchRecorder::start()` creates the take directory, writes initial
  incomplete metadata, and opens two 24-bit WAV sinks before publishing
  `Recording`. Each sink uses a 32,768-sample buffer serviced by the recorder's
  writer thread. Missing/unsupported channels, invalid sample rate, metadata
  failure, or writer setup failure prevent a successful start.
- Audio entry points forward existing channel pointers to the buffered writers
  and update atomics. File setup, writer teardown, JSON serialization, and UI
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
audio does not relabel an interrupted take complete. This loader is not yet
wired to a persistent panel library.

Current boundaries:

- Recent takes are an in-memory, newest-first list capped at eight; the panel
  does not yet scan disk or use the new loader. Files remain on disk when the
  list rolls over or the app restarts. Its counts describe this list, not a
  library.
- `Play` opens a complete take's existing `wet.wav` through the OS file handler;
  it is not an internal transport. `Reamp` adds a `FilePlayerProcessor` for a
  complete take's existing `raw.wav` to the current graph, without wiring it or
  restoring the recorded rig. `Reveal` needs the take directory, not a complete
  recording.
- The panel's lane curves and take thumbnails are decorative, not live meters
  or decoded audio waveforms. Equal sample counts are checked, but there is no
  separate latency-alignment pass for raw and processed signals.

Key files: `src/ScratchRecorder.{h,cpp}`, `src/ScratchTake.{h,cpp}`,
`src/ScratchBundleIO.{h,cpp}`, `src/ScratchPanel.{h,cpp}`, and the Scratch helpers/taps in
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
is now in progress. The storage-only first slice above has built and passed its
focused Scratch run; persistent browsing, real waveforms, in-app audition,
easy reamp, rig snapshots, and UI polish remain upcoming. See the handoff for
the dated result and skipped Windows symlink check; hardware QA remains open.

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
  2. Scratch RAW tap (before input gains)
  3. updateSmoothedTargets() — sync atomic dB → SmoothedValue targets
  4. Pre-compute smoothedInputRamp[numSamples] (one value per sample)
  5. Copy input to inputGainBuffer, scale by smoothed gain * channel gain
  6. Call parent (graph processes)
  7. Process master bus insert rack (if has plugins and not bypassed)
  8. Pre-compute smoothedOutputRamp[numSamples]
  9. Scale output buffers in-place by smoothed gain * channel gain
 10. Scratch WET tap (after output gains)
 11. Tap levels → SafetyLimiterProcessor (peak + VU metering)
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

