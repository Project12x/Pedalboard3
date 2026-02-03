# Changelog

All notable changes to Pedalboard3 will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

---

## [Unreleased]

### Added
- **Effect Rack (SubGraphProcessor)** – Nested plugin hosting within a single node
  - `SubGraphProcessor` wraps internal `AudioProcessorGraph`
  - `SubGraphEditorComponent` provides rack editor UI with viewport/canvas
  - Available via right-click → Pedalboard → Effect Rack
  - Can load external VST3/AU plugins into the rack
  - Double-click canvas to add plugins (same menu as main PluginField)

### Fixed
- **Effect Rack Editor Crash** – Multiple crash fixes:
  - `resized()` timing: viewport/canvas had zero bounds because `setSize()` triggered `resized()` before components existed. Fix: explicit `resized()` call at end of constructor.
  - Double-delete in `SubGraphCanvas` destructor: removed redundant `deleteAllChildren()` since `OwnedArray` manages component lifetime.
  - Null pointer in `PluginPinComponent`: added null checks when parent is `SubGraphCanvas` (not `PluginField`).
- **SubGraph Editor Re-entry Crash** – Fixed crash when reopening Effect Rack editor:
  - Switched from `createEditorIfNeeded()` to `createEditor()` to ensure fresh editor instance on each open.
  - Added pin bounds checking before `getInputPin`/`getOutputPin` calls in `rebuildGraph()` to prevent out-of-range access.
  - Fixed iterator invalidation in `rebuildGraph()` by copying connections to `std::vector` before iterating.
- **ToneGenerator State Restoration** – Fixed sliders resetting to defaults when reopening editor:
  - Constructor now reads frequency, detune, and amplitude from processor instead of hardcoded values.
- **SubGraphProcessor XML Serialization** – Special handling in `createNodeXml()` since SubGraphProcessor is not `AudioPluginInstance`
- **BypassableInstance Exclusion** – SubGraphProcessor excluded from bypass wrapper to prevent bus layout issues
- **VSTi Audio Output Pins Not Displaying** – Root cause: `BypassableInstance` wrapper hid real plugin's bus state. Solution: Unwrap wrapper before querying buses.
- **Mixer Node Pins Missing** – Fallback to `getTotalNumChannels()` for internal processors without bus configuration.

### Known Issues
- **Effect Rack Connections** – Pin-to-pin wiring not yet functional (no crash, just ignored)
- **Internal Node Editors** – Special nodes (Tone Generator, etc.) may show generic UI

### Technical
- Added `getUnwrappedProcessor()` pattern for safe bus queries through wrapper classes
- Debug logging for bus state during plugin loading (retained for development)
- Documented critical JUCE pattern: `setSize()` must be called LAST in component constructors
- Windows crash dump analysis workflow documented in ARCHITECTURE.md

---


## [3.1.0-dev] - 2026-01-30

### 🎹 MIDI Enhancements

- **MIDI File Player** – Load and play .mid/.midi files through synth plugins
  - Transport controls (play/pause/stop/rewind)
  - Tempo/BPM control with sync support
  - Loop mode for seamless playback
- **MIDI Transpose** – Shift MIDI notes ±48 semitones
- **MIDI Rechannelizer** – Remap MIDI input/output channels
- **Keyboard Split** – Split keyboard with configurable split point and channel routing

### 🎛️ Live Performance

- **Stage Mode** – Fullscreen performance view (F11)
  - Large patch display
  - Next patch preview
  - High-contrast colors for stage visibility
  - Quick patch switching via keyboard/foot controller
- **Setlist Management** – Organize patches for live performance
  - Drag-and-drop reordering
  - PatchOrganiser integration
- **Glitch-Free Patch Switching** – Crossfade mixer for silent transitions
- **Plugin Pool Manager** – Background preloading for instant patch switches

### 🎸 New Processors

- **Chromatic Tuner** – Dual-mode display (needle + strobe), YIN pitch detection
- **Tone Generator** – Sine/square/saw/triangle waveforms
- **A/B Splitter** – Split signal into two parallel paths with mute controls
- **A/B Mixer** – Mix two paths back together with level controls
- **Notes Node** – Text display for patch documentation
- **Label Node** – Simple themed text labels
- **IR Loader** – Impulse response cabinet simulation (placeholder)

### 🎨 Visual Polish

- **Canvas Navigation** – Pan (left-click drag), zoom (scroll wheel), fit-to-screen
- **Premium Node Design** – Metallic gradients, rounded corners, shadow effects
- **Modern LAF Updates** – Button hover effects, progress bars, tick boxes
- **Custom Fonts** – Inter/Roboto typography
- **Toast Notifications** – With Melatonin Blur shadows

### 🔧 Bug Fixes

- **Cable Connection Loss** – Fixed connections breaking when switching patches
- **Audio Settings Crash** – Suspend audio before patch reload
- **Looper Hang** – Fixed file load hang and sample rate resampling
- **Plugin Menu Crash** – Fixed dangling reference in categorized menu
- **Undo/Redo Crash** – Safe UID access via PluginPinComponent

### 🔌 Plugin Management

- **Plugin Search** – Filter plugins by name in menu
- **Favorites System** – Star plugins, "★ Edit Favorites..." submenu
- **Recent Plugins** – Quick access to recently used
- **Categorized Menu** – VST3s organized by manufacturer/category

---

## [3.0.0] - 2026-01-XX

### 🎉 First Release as Pedalboard3

This release marks the modernization of Niall Moody's original Pedalboard2 (2011) to work with
modern audio plugins and development practices.

### Added

- **VST3 Plugin Support** – Native 64-bit VST3 hosting (replaces legacy VST2)
- **Undo/Redo System** – Full undo support for:
  - Adding/removing plugins
  - Creating/deleting connections
  - Moving plugins
- **Panic Button** – Instantly stop all audio (Edit → Panic or Ctrl+Shift+P)
- **Theme System** – 5 built-in color schemes:
  - Midnight (default dark theme)
  - Daylight (light theme)
  - Synthwave (neon/retro)
  - Deep Ocean (blue/cyan)
  - Forest (green/nature)
- **Background Plugin Scanning** – Non-blocking plugin discovery
- **JSON Settings** – Modern settings storage via SettingsManager
- **Modern Logging** – spdlog-based logging system
- **CMake Build System** – Modern CMake with presets

### Changed

- **JUCE 8 Migration** – Updated from JUCE 1.x to JUCE 8.0.6
- **64-bit Only** – Single 64-bit build (no 32-bit version)
- **Application Name** – Renamed to "Pedalboard 3"
- **About Page** – Updated credits and links

### Removed

- **VST2 Support** – Removed due to Steinberg SDK license restrictions
- **32-bit Build** – Modern plugins are 64-bit only
- **Deprecated JUCE APIs** – Removed all legacy JUCE patterns:
  - `ScopedPointer` → `std::unique_ptr`
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
2. **64-bit plugins only** – 32-bit plugins will not load
3. **Settings file location unchanged** – Your preferences migrate automatically

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
