# State of the Project

> **For LLMs and Engineers:** This document provides persistent context about the current state of Pedalboard3 modernization. Read this first when joining the project.

---

## Current Focus: Effect Rack Stabilization

**Status:** ✅ Crash Fixed, Functionality Placeholder  
**Last Updated:** 2026-02-01

### Recent Wins (Feb 2026)

- ✅ **Effect Rack Crash Fix** - Root cause: `setSize()` in constructor triggered `resized()` before child components existed
- ✅ SubGraphProcessor implemented (nested AudioProcessorGraph)
- ✅ SubGraphEditorComponent UI with canvas and viewport
- ✅ XML serialization for Effect Racks in patches
- ✅ Effect Rack appears in Pedalboard menu
- ✅ Documented critical JUCE pattern: call `setSize()` LAST in constructors

### Previous Wins (Phase 3)

- ✅ VST3 plugin format enabled and working
- ✅ Settings migrated to JSON (`SettingsManager`)
- ✅ UI visibility fixed (dark theme, high contrast text)
- ✅ 5 built-in themes: Midnight, Daylight, Synthwave, Deep Ocean, Forest
- ✅ JUCE 8 LookAndFeel fixes (setColour before setFont order)

---

## Effect Rack Status

| Component | Status |
|-----------|--------|
| SubGraphProcessor | ✅ Stable |
| SubGraphEditorComponent | ✅ Stable (crash fixed) |
| Add plugins in rack | ⏳ Not implemented |
| Connection management | ⏳ Not implemented |
| State persistence | ⏳ Not implemented |

**Key Files:**
- `src/SubGraphProcessor.cpp/h`
- `src/SubGraphEditorComponent.cpp/h`

---

## Debugging Lessons Learned

1. **Windows crash dumps**: `%LOCALAPPDATA%\CrashDumps\` - open in VS debugger
2. **spdlog logging**: `%APPDATA%\Pedalboard3\debug.log` - use `flush()` before crash points
3. **Critical JUCE gotcha**: `setSize()` triggers `resized()` immediately

---

## Theme System

| Theme | Description |
|-------|-------------|
| Midnight | Default dark theme (indigo/purple) |
| Daylight | Light mode for bright environments |
| Synthwave | Retro neon 80s aesthetic |
| Deep Ocean | Calm blue underwater theme |
| Forest | Natural green and earth tones |

**Files:**
- `ColourScheme.h/cpp` - Theme definitions and persistence
- `ColourSchemeEditor.cpp` - Theme picker UI
- `BranchesLAF.cpp` - LookAndFeel integration

---

## Settings System

**Location:** `%APPDATA%\Pedalboard3\settings.json`  
**Engine:** `SettingsManager` → `nlohmann/json`

---

## Next Steps

1. **Effect Rack Functionality** - Add plugin picker, connection management
2. **Cleanup** - Remove debug logging from PluginComponent, PluginField, FilterGraph
3. **Phase 4:** Plugin Management (better scanning, organize, favorites)
4. Remove deprecated `PropertiesSingleton` code

---

## Quick Reference

| Component | Status |
|-----------|--------|
| VST3 Loading | ✅ Working |
| Plugin Scanning | ✅ Working |
| Settings Persistence | ✅ JSON |
| Theme System | ✅ Complete |
| Effect Rack | ⚠️ Crash fixed, functionality placeholder |
| MIDI Mapping | ⚠️ Legacy (works) |
| OSC Mapping | ⚠️ Legacy (works) |

---

*Last updated: 2026-02-01*
