# Changelog

All notable changes to Pedalboard3 will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

---

## [Unreleased]

### Added

- **Semantic Colour Tokens** — Added `Danger Colour`, `Warning Colour`, and `Success Colour` tokens to all 5 built-in themes (Midnight, Daylight, Synthwave, Deep Ocean, Forest). Theme-appropriate palette per scheme (e.g. Synthwave uses neon pink/orange/green).
- **NAM Collapsible Editor (F1)** — Click the NAM header bar to collapse to a 40px header-only view showing model name, LED, and chevron. Children hidden; PluginComponent relayouts automatically. Pins and E/M/B buttons remain visible.
- **NAM Multi-Font Typography (F2)** — Slider numeric readouts now use JetBrains Mono. Button text uses Inter. Section headers remain Space Grotesk Bold.

### Changed

- **NAM Interface Premium Polish** — Complete visual rewrite of `NAMControl`. Theme-derived 16-colour palette from `ColourScheme` (replaces all hardcoded hex colours). Value arcs on rotary knobs with glow effect and tick marks. Inner-recessed knob rendering with metallic outer ring. LED pulse animation for model-loaded status. Section panels (Signal Chain / Gain / Tone) with inner shadow and accent dots. `FontManager` integration for all typography. Architecture badge label. Larger 64px knobs. Improved linear slider rendering with gradient-filled tracks and shadowed thumbs. Toggle buttons rendered as LED indicators with glow.

### Fixed

- **Eigen MSVC Build Error** — Pinned Eigen dependency from `master` to `5.0.0`. The `master` branch had GCC-specific `__attribute__((always_inline))` on lambdas in `GeneralBlockPanelKernel.h` causing MSVC C3260. Version 3.4.x was too old (lacked `Eigen::placeholders::lastN` required by NAM's `lstm.h`). 5.0.0 is the stable release with both features.

- **Token Audit Pass 1** — Replaced hardcoded `Colours::black` label text in MainPanel (3 labels) and `Colours::darkgrey` dialog background with `ColourScheme` tokens. Replaced 13 `Colours::white` text callsites in StageView with `"Text Colour"` token — white text was invisible on Daylight theme where stage backgrounds are light grey.
- **Token Audit Pass 2** — Replaced hardcoded `Colours::red`, `Colours::green`, `Colours::orange`, `Colours::darkred`, `Colours::darkgreen` with semantic tokens (`Danger Colour`, `Warning Colour`, `Success Colour`) across 11 files: DawSplitterProcessor, RoutingProcessors (A/B Splitter mute buttons, old Mixer mute buttons), StageView (panic button, VU glow), TunerControl (needle ticks, strobe glow, LED indicators), ToneGeneratorControl (play/stop button), NAMOnlineBrowser (cached/done/failed status), NotesControl (bold markdown text), MarkdownTokeniser (bold token), Tone3000Auth (error outlines).
- **Mixer Strips Beyond 2 Non-Functional** — VU meters, mute, fader, and pan controls did not work on mixer strips beyond the first two. Root cause: `BypassableInstance` cached initial channel counts and never updated when `DawMixerProcessor` added strips. Added `BypassableInstance::resyncChannelCount()` to resize `tempBuffer` and call `setPlayConfigDetails()`, invoked from `PluginComponent::refreshPins()` for dynamic-channel processors.
- **Stereo Toggle Cable Crash** — Toggling a mixer strip between mono/stereo with cables connected caused a crash. Root cause: `refreshPins()` deleted all `PluginPinComponent` objects while `PluginConnection` objects still held raw pointers to them. Fix: `refreshPins()` now removes all `PluginConnection` objects and their graph connections before destroying pins.

### Changed

- **Tech Debt Cleanup** — Fixed duplicate `mute.store()` bug in `StripState::resetDefaults()` (both mixer and splitter) where `solo` was never reset. Removed 28-line deliberation comment block from `DawSplitterProcessor::processBlock`. Fixed duplicate doc comment in `PedalboardProcessors.h`. Hoisted `numStrips_` atomic load out of `prepareToPlay` loop in both processors (was redundantly loaded 32x per call).
- **Code Formatting Pass** — clang-format applied across NAMOnlineBrowser, Tone3000Auth, ToneGeneratorControl, and other files for consistent brace/indent style.

- **Gain Smoothing** — Master input/output gain now uses `SmoothedValue<float, Multiplicative>` with a 50ms ramp to eliminate zipper noise during gain changes. Pre-computed ramp buffers ensure correct smoothing rate independent of channel count.
- **VU Meter Ballistics** — New `VuMeterDsp` class provides standard 300ms VU integration (IEC 60268-17) using a 2-pole cascaded lowpass filter. Integrated into `SafetyLimiterProcessor` alongside existing peak metering. VU levels exposed via `getOutputVuLevel()`/`getInputVuLevel()`.
- **VU Meter UI Wiring** — Meter bars in PluginComponent (Audio I/O nodes) and StageView now display VU ballistic levels for smooth response, while peak hold indicators retain instantaneous peak detection. Professional DAW-style metering: smooth VU body + sharp peak pip.
- **Master Bus Insert Rack** — Global effect rack at the device output level, powered by SubGraphProcessor. Accessible via the "FX" button in the footer toolbar between the input/output gain sliders. Opens in a dialog window for plugin management. State persists globally (not per-patch).
- **Master Bus Processor** — New `MasterBusProcessor` wraps a `SubGraphProcessor` for processing all audio at the device callback level, between the graph output and output gain stage.
- **Mixer Plugin Upgrade** — Complete rewrite of the internal Mixer plugin:
  - Per-channel gain (dB), pan (-3dB equal-power law), mute, solo-in-place, phase invert
  - `SmoothedValue` gain ramps (50ms) with multiplicative smoothing to eliminate zipper noise
  - VU metering per channel (L/R) using filtered RMS with peak hold indicators
  - Channel strip editor with vertical gain faders (-60 to +12 dB), rotary pan knobs, mute/solo/phase toggle buttons with color feedback, and dual VU meter bars with dB scale ticks
  - Backward-compatible state serialization (reads old `levelA`/`levelB` format)
- **DAW Splitter Plugin** — New `DawSplitterProcessor`: 1 stereo input to N mono outputs with per-strip gain, mute, solo, phase invert, and mono VU metering. Mirror of `DawMixerProcessor` with dynamic strip add/remove via editor UI.
- **Stereo Strip Toggle** — Mixer and Splitter strips can now be toggled between mono (1 channel) and stereo (2 channels) via the "ST" button. Stereo strips use balance panning and dual-channel VU meters. Channel config updates the audio graph on toggle. State persists in project serialization.

### Fixed

- **CrashProtection Detached-Thread Lifetime** — Detached timeout workers captured stack locals by reference, causing use-after-free if the calling function returned before the worker completed. Moved timeout coordination state to heap-shared storage (`std::shared_ptr`) so detached threads never touch stack locals.
- **MainPanel OSC Thread Shutdown Ordering** — OSC listener thread was not explicitly stopped before destructor teardown, risking callbacks into destroyed objects. Added `stopThread(2000)` in destructor and when disabling OSC input.
- **Tone3000DownloadManager Queue/Thread Safety** — Worker thread held raw pointers into `downloadQueue` while the message thread could mutate it. Worker now processes copied task objects. Added `cancelCurrentDownload` atomic flag. `getTask()` returns `std::optional<DownloadTask>` by value instead of raw pointer.
- **Tone3000Auth Callback UAF/Double-Callback** — Async completion lambdas captured `[this]` and could fire after the auth object was destroyed, or fire multiple times. Added one-shot `dispatchCompletion()` with mutex-protected callback copy. Startup-timeout path now stops thread before dispatch.
- **SafePluginScanner Timeout Stack-Capture** — Scanner timeout detach path captured stack references in its lambda. Replaced with heap-shared coordination state to match `CrashProtection` pattern.

- **Mixer/Splitter Pin Count Not Scaling** — Input/output pins on DawMixerProcessor and DawSplitterProcessor nodes were stuck at the initial channel count because `BypassableInstance` caches channel counts at construction time. `countInputChannelsFromBuses`/`countOutputChannelsFromBuses` now detect `PedalboardProcessor` inside the wrapper and query the inner plugin's current `getTotalNumChannels()` directly.
- **Mixer/Splitter VU Meters Showing Stereo for Mono Strips** — Strip VU bars were using the stereo `paintHorizontalVU` function. Replaced with `paintMonoVU` for mono strips. Master/input rows correctly retain stereo VU display.
- **Mixer/Splitter Pins Not Aligned with Strip Rows** — Pins used a uniform 22px spacing that didn't match the 44px strip row height. Added `PinLayout` virtual to `PedalboardProcessor` with overrides in `DawMixerProcessor` and `DawSplitterProcessor`. `PluginComponent::createPins()` now queries the inner processor's layout for custom startY/spacing, aligning each pin with its corresponding channel strip.
- **Stereo Toggle Not Updating Channel Config** — Toggling the ST button changed the atomic flag but did not call `updateChannelConfig()`, causing channel "robbing" where a stereo strip would steal the next strip's pin. Fixed by calling `processor->updateChannelConfig()` in the onClick handler. Made `updateChannelConfig()` public for UI access.
- **Stereo Pin Alignment** — Increased strip row height from 44px to 52px for better stereo pin spacing. Pin Y coordinates now derived from first principles (PluginComponent title + control header + row position) with mono pins centered and stereo L/R at 1/3 and 2/3 of row height. Output pins (mixer master, splitter strips) are now dynamic based on strip count.
- **ST Button Text Truncated** — The stereo toggle button was 24px wide, causing "ST" to display as "...". Widened to 28px.
- **MIDI Pin Floats After Audio Pins** — On mixer/splitter nodes, the MIDI In pin was placed immediately after the last audio input pin, landing mid-component. Now placed at a fixed bottom-left position (`getHeight() - 40`) for PedalboardProcessors.
- **Edit/Bypass/Mappings Buttons Clipped by Growing Controls** — The e/b/m buttons were positioned once in the constructor at `getHeight() - 30` and never updated when strips were added. Added button repositioning in `refreshPins()` after `determineSize()` recalculates the component height.

- **Bypass Not Saved in Patches** — Plugin bypass state was never serialized to patch XML. Toggling bypass, saving, and reloading a patch would reset all plugins to un-bypassed. Added `bypass` attribute to `createNodeXml()` and restore in `createNodeFromXml()`. Backward-compatible (missing attribute defaults to false).
- **JUCE 8 MIDI Input Name Mismatch** — JUCE 8 renamed the internal processor from "Midi Input" to "MIDI Input" (all caps). The codebase still compared against the old name, causing MIDI node sizing, toggle duplicate detection, and delete-on-disable to silently fail. Fixed across PluginComponent, PluginField, FilterGraph, and PluginFieldPersistence.
- **MIDI Node Size Mismatch** — "MIDI Input" and "Virtual MIDI Input" nodes had different dimensions. Both now use a shared width computed from the longer name's font metrics, with matching height (92px).
- **MIDI Toggle Spawning Duplicates** — Options > MIDI Input toggle created a new MIDI Input node each time because the duplicate check compared against the wrong name string. Root cause: same JUCE 8 name mismatch.
- **Node Positions Overwritten on Device Change** — `setDeviceChannelCounts()` called `repositionDefaultInputNodes()` which overwrote saved node positions from patches. Removed that call so patch positions persist.
- **OSC Input Showing Audio/MIDI Pins** — `OscInput` had JUCE's default stereo bus layout because its constructor was empty. Fixed by setting an empty bus layout and adding `isBusesLayoutSupported()` override.
- **Virtual MIDI Input Showing Edit/Bypass Buttons** — The node wasn't in the infrastructure exclusion list. Added to all 3 exclusion conditions in PluginComponent.
- **Fit-to-Screen Not Centering Properly** — `fitToScreen()` set the zoom and scroll position, but the PluginField was sized exactly to the node edges with no padding. The viewport couldn't scroll far enough to center the content. Fix: `fitToScreen()` now enlarges the PluginField before positioning, and `changeListenerCallback()` adds 500px padding beyond nodes.
- **Fit-to-Screen on Startup Timing** — The initial fit was triggered from `resized()` before the default patch loaded. Moved to `MessageManager::callAsync` after patch load so the viewport has its final dimensions.
- **Canvas Scroll Jitter** — Scrolling right/down caused jitter because the viewport hit the PluginField boundary. Same root cause as the fit-to-screen issue: field was sized to exactly the node edges. Fixed by adding padding in `changeListenerCallback()`.
- **fitToScreen Including Connection Objects** — `fitToScreen()` included all child components (including `PluginConnection` objects) in the bounding box. Fixed by filtering to only `PluginComponent` instances.
- **Audio I/O Node Refresh on Device Change** — `refreshPins()` only recreated pins, not gain sliders, so switching to a device with different channel count left the node without VU meters or gain controls. Fix: `refreshPins()` now tears down and recreates gain sliders (mirroring constructor logic). Also fixed `timerUpdate()` hardcoding `numChannels = 2` — now derives from processor bus layout.
- **Audio Output VU Meter Not Responding** — Output metering was broken because `DeviceMeterTap` (JUCE callback[1]) receives a temp buffer for output, not the real device output. And `SafetyLimiterProcessor` was a hidden infrastructure node with no audio connections. Fix: `MeteringProcessorPlayer` subclasses `AudioProcessorPlayer` and taps the real device output buffers after graph processing, writing peak levels to `SafetyLimiterProcessor`'s atomics (RT-safe). The Audio Output VU now responds correctly for all sources including synth plugins.
- **RT-Safety: MIDI/OSC Parameter Dispatch** — All mapping-dispatched `setParameter()` calls now go through a lock-free FIFO (`MidiAppFifo`) and are drained on the message thread via the 5ms `MidiAppTimer`. This eliminates `sendChangeMessage`, `transportSource` ops, `triggerAsyncUpdate`, and other non-RT calls from the audio thread for FilePlayerProcessor, LooperProcessor, MetronomeProcessor, MidiFilePlayerProcessor, and all future processors.
- **RT-Safety: MIDI CC Logging Removed from Audio Thread** — Removed `LogFile::logEvent` (CriticalSection + file I/O + `sendChangeMessage`) and `spdlog` calls from `MidiMappingManager::midiCcReceived`. Made `midiLearnCallback` pointer `std::atomic` for thread-safe access.
- **RT-Safety: Metronome Staging Guard** — Moved `files[]` assignment after the `pendingClickReady` guard in `setAccentFile`/`setClickFile`, preventing state desync when the audio thread hasn't consumed the previous pending buffer.
- **RT-Safety: VuMeter Atomic Floats** — Changed `levelLeft`/`levelRight` from plain `float` to `std::atomic<float>` with proper `.load()`/`.store()` in `processBlock` and UI getters.
- **RT-Safety: Oscilloscope Double-Buffering** — Replaced unsynchronized `displaySnapshot` with a double-buffer scheme. Audio writes to the back buffer and atomically swaps the front index on capture complete; UI reads from the front buffer.
- **RT-Safety: Null Node Dereference** — Added null check on `getNodeForId` return in `Mapping::updateParameter` fallback path.
- **Crash: Stale CrossfadeMixer Pointer on Patch Switch** — `PluginField::loadFromXml` cached a `CrossfadeMixer*` then cleared/restored the graph (destroying all nodes), then called `startFadeIn` on the now-dangling pointer. Fix: `FilterGraph::clear()` now calls `createInfrastructureNodes()` to rebuild hidden infrastructure after `graph.clear()`, and `PluginFieldPersistence` reacquires the crossfader pointer after restore.
- **Crash: Infrastructure Nodes Persisted to Patch XML** — Hidden runtime-only nodes (SafetyLimiter, CrossfadeMixer) and their connections were being saved into patch XML, causing duplicate/ghost nodes on reload. Fix: `FilterGraph::createXml` now skips `isHiddenInfrastructureNode` nodes and connections.
- **Crash: FIFO Parameter Dispatch During Patch Transition** — Deferred parameter events from the old graph could dispatch to the new graph's nodes after a patch switch, hitting wrong processors or out-of-range parameter indices. Fix: FIFO drain now checks `pc.graph == &signalPath` and bounds-checks `paramIndex < numParams`.
- **Memory Leak: Duplicate `getXml()` Call in Patch Save** — `MainPanel::savePatch` called `field->getXml()` twice, leaking one `XmlElement`. Removed the redundant call.

### Known Issues

- **FIFO Silent Drop** — FIFO writes silently drop when full (1024 entries). In practice this requires ~200K+ entries/sec to trigger, far beyond MIDI bandwidth.
- **FIFO Fallback Path** — `Mapping::updateParameter` retains a direct `setParameter` fallback when the FIFO is not wired (before `MainPanel` constructor runs). Guarded by `jassert` in debug builds. This is an init-order guard, not a runtime path.

### Added
- **Master Gain Controls** — RT-safe per-channel gain for Audio Input/Output nodes
  - `MasterGainState` singleton with atomic gain values (dB) for up to 16 channels
  - Gain applied in `MeteringProcessorPlayer::audioDeviceIOCallbackWithContext` (pre-graph for input, post-graph for output)
  - Per-channel gain sliders on Audio I/O node components
  - Master IN/OUT gain sliders in footer toolbar (responsive layout with breakpoints)
  - Master IN/OUT gain sliders in Stage View (performance mode)
  - All slider instances sync via `MasterGainState` atomics (~30fps timer)
  - Double-click to reset to 0 dB; range -60 to +12 dB
  - Gain persisted via `SettingsManager` across sessions
- **Professional VU Meters** — Upgraded metering across all views
  - Green-to-yellow-to-red horizontal gradient fill
  - Peak hold indicators with ~2s hold then multiplicative decay
  - dB scale tick marks at -48, -24, -12, -6, -3, 0 dB
  - Glow/bloom effect when levels exceed -6 dB
  - Thicker meter bars (8px on nodes, 10px in Stage View)
  - Input metering via `SafetyLimiterProcessor::updateInputLevelsFromDevice`
- **Effect Rack (SubGraphProcessor)** â€" Nested plugin hosting within a single node
  - `SubGraphProcessor` wraps internal `AudioProcessorGraph`
  - `SubGraphEditorComponent` provides rack editor UI with viewport/canvas
  - Available via right-click â†’ Pedalboard â†’ Effect Rack
  - Can load external VST3/AU plugins into the rack
  - Double-click canvas to add plugins (same menu as main PluginField)
- **Cable Wiring/Deletion Tests** â€“ Comprehensive headless test coverage for Effect Rack connections
  - 3 new test cases: wiring creation, connection removal, mutation testing
  - 59 assertions covering stereo, chaining, self-connection rejection, iterator stability
- **FilterGraph Unit Tests** â€“ Phase 6B testing expansion
  - 6 test cases: node management, connection management, position, infrastructure detection
  - 71 assertions covering add/remove/query operations and mutation testing
- **MIDI Mapping Unit Tests** — Phase 6B testing expansion
  - 15 test cases: CC normalization, custom/inverted range mapping, latch/toggle, channel filtering, multi-mapping dispatch, XML round-trip, MidiAppFifo, MIDI learn, register/unregister lifecycle, edge cases
  - 746 assertions (including existing MIDI integration tests)
  - Tests real MidiAppFifo for parameter, CommandID, tempo, and patch change FIFOs
- **Expanded Integration Tests** â€“ End-to-end and mutation test coverage
  - Signal path: effect chain processing, bypass behavior
  - Plugin lifecycle: load/unload, editor reopen
  - MIDI routing: channel filtering, omni mode
  - MIDI mapping: CC-to-parameter, min/max ranges
  - 10 mutation test cases covering boundaries, returns, conditions
- **Plugin Protection Infrastructure** â€“ Crash resilience for plugin operations
  - `PluginBlacklist` â€“ Thread-safe singleton for blocking problematic plugins with SettingsManager persistence
  - `CrashProtection` â€“ SEH exception wrappers (Windows), watchdog thread (15s timeout), auto-save triggers, crash context logging
  - `BlacklistWindow` â€“ UI for viewing/removing blacklisted plugins (Options â†’ Plugin Blacklist)
  - `FilterGraph` integration â€“ Blocks blacklisted plugins at load time (main graph + sub-graphs)
- **Generic Editor Context Menu** â€“ Right-click Edit button to choose:
  - "Open Custom Editor" â€“ Plugin's native GUI
  - "Open Generic Editor" â€“ Internal parameter view (NiallsGenericEditor)

### Refactored
- **loadSVGFromMemory Consolidation** â€“ Removed duplicate implementations from `PresetBar`, `MetronomeControl`, `MainPanel`, and `ColourSchemeEditor`. All classes now use global `JuceHelperStuff::loadSVGFromMemory()`.
- **Plugin Editor Creation** â€“ Extracted `openPluginEditor(bool forceGeneric)` helper for cleaner code reuse

### Fixed
- **Plugin Editor Reopen Bug** â€“ Fixed custom GUI not reopening after closing:
  - `EditorWrapper` destructor was removing but not deleting editor, causing memory leak
  - Plugin's cached editor pointer prevented new custom GUI creation
  - Now correctly deletes editor, allowing fresh instance on next open
- **Effect Rack Editor Crash** â€“ Multiple crash fixes:
  - `resized()` timing: viewport/canvas had zero bounds because `setSize()` triggered `resized()` before components existed. Fix: explicit `resized()` call at end of constructor.
  - Double-delete in `SubGraphCanvas` destructor: removed redundant `deleteAllChildren()` since `OwnedArray` manages component lifetime.
  - Null pointer in `PluginPinComponent`: added null checks when parent is `SubGraphCanvas` (not `PluginField`).
- **Mixer Node Pins Missing** â€“ Fallback to `getTotalNumChannels()` for internal processors without bus configuration.
- **Cable Connection Invisible Collision** â€“ Fixed cable dragging backwards (outputâ†’input) causing collision with invisible object at origin. Root cause: `PluginConnection::updateBounds` could set negative bounds. Fix: clamp bounds to non-negative using `jmax(0, left - 5)`.

### Known Issues
- **Effect Rack Connections** â€“ Pin-to-pin wiring not yet functional (no crash, just ignored)
- **Internal Node Editors** â€“ Special nodes (Tone Generator, etc.) may show generic UI

### Technical
- Added `getUnwrappedProcessor()` pattern for safe bus queries through wrapper classes
- Debug logging for bus state during plugin loading (retained for development)
- Documented critical JUCE pattern: `setSize()` must be called LAST in component constructors
- Windows crash dump analysis workflow documented in ARCHITECTURE.md

---


## [3.1.0-dev] - 2026-01-30

### MIDI Enhancements

- **MIDI File Player** â€“ Load and play .mid/.midi files through synth plugins
  - Transport controls (play/pause/stop/rewind)
  - Tempo/BPM control with sync support
  - Loop mode for seamless playback
- **MIDI Transpose** â€“ Shift MIDI notes Â±48 semitones
- **MIDI Rechannelizer** â€“ Remap MIDI input/output channels
- **Keyboard Split** â€“ Split keyboard with configurable split point and channel routing

### Live Performance

- **Stage Mode** â€“ Fullscreen performance view (F11)
  - Large patch display
  - Next patch preview
  - High-contrast colors for stage visibility
  - Quick patch switching via keyboard/foot controller
- **Setlist Management** - Organize patches for live performance
  - Drag-and-drop reordering
  - PatchOrganiser integration
- **Glitch-Free Patch Switching** â€“ Crossfade mixer for silent transitions
- **Plugin Pool Manager** â€“ Background preloading for instant patch switches

### New Processors

- **Chromatic Tuner** â€“ Dual-mode display (needle + strobe), YIN pitch detection
- **Tone Generator** â€“ Sine/square/saw/triangle waveforms
- **A/B Splitter** â€“ Split signal into two parallel paths with mute controls
- **A/B Mixer** â€“ Mix two paths back together with level controls
- **Notes Node** â€“ Text display for patch documentation
- **Label Node** â€“ Simple themed text labels
- **IR Loader** â€“ Impulse response cabinet simulation (placeholder)

### Visual Polish

- **Canvas Navigation** â€“ Pan (left-click drag), zoom (scroll wheel), fit-to-screen
- **Premium Node Design** â€“ Metallic gradients, rounded corners, shadow effects
- **Modern LAF Updates** â€“ Button hover effects, progress bars, tick boxes
- **Custom Fonts** â€“ Inter/Roboto typography
- **Toast Notifications** â€“ With Melatonin Blur shadows

### Bug Fixes

- **Cable Connection Loss** â€“ Fixed connections breaking when switching patches
- **Audio Settings Crash** â€“ Suspend audio before patch reload
- **Looper Hang** â€“ Fixed file load hang and sample rate resampling
- **Plugin Menu Crash** â€“ Fixed dangling reference in categorized menu
- **Undo/Redo Crash** â€“ Safe UID access via PluginPinComponent

### Plugin Management

- **Plugin Search** â€“ Filter plugins by name in menu
- **Favorites System** â€“ Star plugins, "â˜… Edit Favorites..." submenu
- **Recent Plugins** â€“ Quick access to recently used
- **Categorized Menu** â€“ VST3s organized by manufacturer/category

---

## [3.0.0] - 2026-01-XX

### First Release as Pedalboard3

This release marks the modernization of Niall Moody's original Pedalboard2 (2011) to work with
modern audio plugins and development practices.

### Added

- **VST3 Plugin Support** â€“ Native 64-bit VST3 hosting (replaces legacy VST2)
- **Undo/Redo System** â€“ Full undo support for:
  - Adding/removing plugins
  - Creating/deleting connections
  - Moving plugins
- **Panic Button** â€“ Instantly stop all audio (Edit â†’ Panic or Ctrl+Shift+P)
- **Theme System** â€“ 5 built-in color schemes:
  - Midnight (default dark theme)
  - Daylight (light theme)
  - Synthwave (neon/retro)
  - Deep Ocean (blue/cyan)
  - Forest (green/nature)
- **Background Plugin Scanning** â€“ Non-blocking plugin discovery
- **JSON Settings** â€“ Modern settings storage via SettingsManager
- **Modern Logging** â€“ spdlog-based logging system
- **CMake Build System** â€“ Modern CMake with presets

### Changed

- **JUCE 8 Migration** â€“ Updated from JUCE 1.x to JUCE 8.0.6
- **64-bit Only** â€“ Single 64-bit build (no 32-bit version)
- **Application Name** â€“ Renamed to "Pedalboard 3"
- **About Page** â€“ Updated credits and links

### Removed

- **VST2 Support** â€“ Removed due to Steinberg SDK license restrictions
- **32-bit Build** â€“ Modern plugins are 64-bit only
- **Deprecated JUCE APIs** â€“ Removed all legacy JUCE patterns:
  - `ScopedPointer` â†’ `std::unique_ptr`
  - `juce_UseDebuggingNewOperator` macros
  - Old `NodeID.uid` direct access

### Technical

- **Dependencies:**
  - JUCE 8.0.6
  - fmt 11.1.4
  - spdlog 1.15.1
  - nlohmann/json 3.11.3
  - Catch2 3.8.0
  - Melatonin Blur
- **Compiler:** MSVC 2022 (C++17)
- **Build:** CMake 3.24+ with CPM.cmake

### Credits

- **Original Author:** Niall Moody (2011)
- **Modernization:** Eric Steenwerth (2024-2026)

---

## [2.14] - 2013-04-22

*Final release by Niall Moody*

- Last version of the original Pedalboard2
- See original documentation for full history

---

## Migration Notes

### For Pedalboard2 Users

Your existing `.pdl` patch files should load in Pedalboard3, but:

1. **VST2 plugins must be replaced with VST3 versions**
2. **64-bit plugins only** â€“ 32-bit plugins will not load
3. **Settings file location unchanged** â€“ Your preferences migrate automatically

### Known Limitations

- CLAP plugin support planned (Phase 3 of roadmap)
- macOS/Linux builds not yet available (Windows only for v3.0)
- VST2 bridge deferred due to licensing

---

## Technical Fixes Reference

This section documents significant bug fixes with technical details for developer reference.

### VSTi Audio Output Pins (2026-01-30)

**Symptom:** VST instruments (SurgeXT, etc.) displayed 0 audio output pins on canvas.

**Root Cause:** `BypassableInstance` wrapper hid real plugin's bus state. JUCE's `getBusCount()`, `getBus()`, `getTotalNumInputChannels()` are **NOT virtual** - calling on wrapper returns empty bus state.

**Solution:**
1. Add `getUnwrappedProcessor()` helper to detect/unwrap `BypassableInstance`
2. Call `enableAllBuses()` before wrapping in `addFilter()` and `createNodeFromXml()`
3. Fallback to `getTotalNumChannels()` for internal processors

**Files Changed:** `PluginComponent.cpp`, `BypassableInstance.h`, `FilterGraph.cpp`

---

*For the full development roadmap, see [PHASED_PLAN.md](PHASED_PLAN.md)*


