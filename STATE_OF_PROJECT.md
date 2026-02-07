# State of the Project

> **For LLMs and Engineers:** This document provides persistent context about the current state of Pedalboard3 modernization. Read this first when joining the project.

---

## Current Focus: Phase 6 - Stability & Polish

**Status:** 🚀 In Progress  
**Last Updated:** 2026-02-05

### Phase 6 Progress

**Plugin Scan Protection (Complete):**
- ✅ PluginBlacklist singleton with SettingsManager persistence
- ✅ CrashProtection with SEH wrappers and Watchdog thread
- ✅ FilterGraph integration (blocks blacklisted plugins at load)
- ✅ BlacklistWindow UI for manual management (Options → Plugin Blacklist)
- ✅ Out-of-process scanner (Complete)
- ⏳ Timeout logic for hung scans (pending)

**Monolith Sharding (Complete):**
- ✅ PedalboardProcessors.cpp → 7 files
- ✅ PedalboardProcessorEditors.cpp → 6 files
- ✅ PluginField.cpp → extracted persistence
- ✅ PluginComponent.cpp → extracted connections

### Previous Wins (Feb 2026)

- ✅ **Effect Rack Feature Complete** - Phase 5Q milestone achieved
- ✅ **OSC Position Persistence** - Fixed node drift on patch load
- ✅ **Southwest Cable Fix** - Two-Phase Transform pattern
- ✅ **Sub-Graph Connection Fix** - Clear before restore pattern

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
| **Total** | **59+** | **700+** | ✅ All Pass |

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
| MIDI Mapping | ⚠️ Legacy (works) |
| OSC Mapping | ⚠️ Legacy (works) |

---

## Next Steps (Phase 6)

1. **Patch Organiser Sync** - Fix combobox text update on rename
2. **Timeout Logic** - Detect and terminate hung plugin scans
3. **6B: Testing** - Add more unit tests (FilterGraph, MIDI mapping)
4. **6A: UI Polish** - Premium typography/icons

---

*Last updated: 2026-02-05*
