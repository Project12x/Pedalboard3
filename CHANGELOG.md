# Changelog

All notable changes to Pedalboard3 will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

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

- CLAP plugin support deferred (waiting for stable JUCE 8 integration)
- macOS/Linux builds not yet available (Windows only for v3.0)

---

*For the full development roadmap, see [PHASED_PLAN.md](PHASED_PLAN.md)*
