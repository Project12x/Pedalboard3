# Pedalboard3 Architecture

> **For LLMs and Engineers:** This document describes the codebase structure, key patterns, and JUCE 8 API changes.

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

### PluginComponent (Plugin Node)

UI for a single plugin in the graph.
- Shows plugin name, bypass button
- Connection pins for audio/MIDI
- Opens plugin editor on double-click

---

## JUCE 8 API Patterns

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
| `MasterGainState` | Atomic per-channel gain (dB) for Audio I/O |
| `SafetyLimiterProcessor` | Output/input level metering + safety limiting |

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

    float getInputGainLinear(int ch) const;   // Decibels::decibelsToGain
    float getOutputGainLinear(int ch) const;
    void loadFromSettings();  // SettingsManager on startup
    void saveToSettings();    // SettingsManager on shutdown
};
```

**Key files:** `MasterGainState.h`, `MasterGainState.cpp`

### MeteringProcessorPlayer

Subclasses `AudioProcessorPlayer` to apply gain and tap real device buffers.

```text
audioDeviceIOCallbackWithContext:
  1. Read per-channel input gain from MasterGainState
  2. Copy input to inputGainBuffer, scale by gain (inputChannelData is const)
  3. Update input levels → SafetyLimiterProcessor::updateInputLevelsFromDevice
  4. Call parent (graph processes)
  5. Read per-channel output gain, scale output buffers in-place
  6. Update output levels → SafetyLimiterProcessor::updateOutputLevelsFromDevice
```

**Key file:** `MainPanel.h` (inner class `MeteringProcessorPlayer`)

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

*Last updated: 2026-02-13*

