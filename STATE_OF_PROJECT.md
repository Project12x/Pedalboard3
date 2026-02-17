# State of the Project

> **For LLMs and Engineers:** This document provides persistent context about the current state of Pedalboard3 modernization. Read this first when joining the project.

---

## Current Focus: Phase 6 - Stability & Polish

**Status:** In Progress
**Last Updated:** 2026-02-11

### Phase 6 Progress

**Plugin Scan Protection (Complete):**
- Done: PluginBlacklist singleton with SettingsManager persistence
- Done: CrashProtection with SEH wrappers and Watchdog thread
- Done: FilterGraph integration (blocks blacklisted plugins at load)
- Done: BlacklistWindow UI for manual management (Options > Plugin Blacklist)
- Done: Out-of-process scanner (Complete)
- Pending: Timeout logic for hung scans

**Monolith Sharding (Complete):**
- Done: PedalboardProcessors.cpp split into 7 files
- Done: PedalboardProcessorEditors.cpp split into 6 files
- Done: PluginField.cpp extracted persistence
- Done: PluginComponent.cpp extracted connections

**Infrastructure Node Polish (Complete):**
- Done: JUCE 8 "MIDI Input" name mismatch fixed across codebase
- Done: MIDI Input and Virtual MIDI Input nodes sized identically
- Done: Virtual MIDI Input: removed edit/bypass/mappings buttons
- Done: OSC Input: removed spurious audio/MIDI pins
- Done: Default node positions updated to centered layout
- Done: Node positions no longer overwritten by device changes

**Canvas & Viewport (Complete):**
- Done: Fit-to-screen properly centers nodes on startup
- Done: PluginField sized with padding for smooth viewport scrolling
- Done: Ctrl+wheel zoom, plain wheel scroll
- Done: fitToScreen excludes connection objects from bounding box

### Previous Wins (Feb 2026)

- Done: **Effect Rack Feature Complete** - Phase 5Q milestone achieved
- Done: **OSC Position Persistence** - Fixed node drift on patch load
- Done: **Southwest Cable Fix** - Two-Phase Transform pattern
- Done: **Sub-Graph Connection Fix** - Clear before restore pattern

**Master Bus & Mixer (Complete - Feb 17 2026):**
- Done: Bypass persistence in patch XML (forward/backward compatible)
- Done: MasterBusProcessor insert rack (SubGraph-based, device callback level)
- Done: Master bus UI (FX button in footer, dialog window rack editor)
- Done: RT-safe hasPlugins atomic cache via ChangeListener
- Done: Gain smoothing (SmoothedValue, 50ms multiplicative ramp)
- Done: VU meter ballistics (VuMeterDsp, 300ms IEC 60268-17)
- Done: Dual metering UI (VU bar + peak hold indicator)

---

## Effect Rack Status

| Component | Status |
|-----------|--------|
| SubGraphProcessor | ✅ Stable |
| SubGraphEditorComponent | ✅ Functional |
| Add plugins in rack | ✅ Working |
| Cable wiring tests | ✅ Complete (59 assertions) |
| State persistence | ✅ Tested |

**Key Files:**
- `src/SubGraphProcessor.cpp/h`
- `src/SubGraphEditorComponent.cpp/h`
- `tests/subgraph_test.cpp`

---

## Test Suite Status

| Test Category | Tests | Assertions | Status |
|---------------|-------|------------|--------|
| SubGraph [subgraph] | 13 | 148 | ✅ Pass |
| Cables [cables] | 3 | 59 | ✅ Pass |
| Integration | 5 | 37 | ✅ Pass |
| ToneGenerator | 8 | 85 | ✅ Pass |
| PluginPoolManager | 10 | 130+ | ✅ Pass |
| Audio Components | 8 | 42 | ✅ Pass |
| Protection | 12 | 186 | ✅ Pass |
| MIDI Mapping | 18 | 746 | ✅ Pass |
| **Total** | **124** | **2M+** | ✅ All Pass |

---

## Debugging Lessons Learned

1. **Windows crash dumps**: `%LOCALAPPDATA%\CrashDumps\` - open in VS debugger
2. **spdlog logging**: `%APPDATA%\Pedalboard3\debug.log` - use `flush()` before crash points
3. **Critical JUCE gotcha**: `setSize()` triggers `resized()` immediately
4. **Iterator invalidation**: Copy containers before iterating if modifying

---

## Quick Reference

| Component | Status |
|-----------|--------|
| VST3 Loading | ✅ Working |
| Plugin Scanning | ✅ Working |
| Settings Persistence | ✅ JSON |
| Theme System | ✅ Complete |
| Effect Rack | ✅ Phase 5Q Complete |
| Master Bus Insert | ✅ Complete |
| Gain Smoothing | ✅ Complete |
| VU Metering | ✅ Complete |
| MIDI Mapping | ⚠️ Legacy (works) |
| OSC Mapping | ⚠️ Legacy (works) |

---

## Next Steps (Phase 6)

1. ~~**Patch Organiser Sync**~~ - Done (commit 02acb82)
2. ~~**Timeout Logic**~~ - Done (out-of-process scanner with timeout)
3. ~~**Premium Typography**~~ - Done (Inter font family)
4. ~~**6B: Testing**~~ - Done (MIDI mapping tests: 18 cases, 746 assertions)
5. **6A: UI Polish** - blend2d vectors, animations, hover tooltips
6. **6E: Legacy UX** - Zoom out mode, pin hover enlarge, keyboard shortcuts

---

*Last updated: 2026-02-17*
