# State of the Project

> **For LLMs and Engineers:** This document provides persistent context about the current state of Pedalboard3 modernization. Read this first when joining the project.

---

## Current Focus: Phase 6 - Stability & Polish

**Status:** 🚀 Starting Phase 6  
**Last Updated:** 2026-02-03

### Phase 5Q Complete ✅

Effect Rack (SubGraph) is now feature-complete:
- ✅ Editor opens and closes without crash
- ✅ Editor can be reopened multiple times without crash
- ✅ Nodes visible (Audio Input, Output, MIDI Input)
- ✅ Can load external VST3/AU plugins into rack
- ✅ Cable wiring tests complete (59 assertions)
- ✅ All internal plugins restore UI state correctly

### Previous Wins (Feb 2026)

- ✅ **Cable Wiring Tests** - 3 new test cases covering connection creation/deletion
- ✅ **Effect Rack Re-entry Fixed** - Editor can now be reopened multiple times
- ✅ **ToneGenerator State Restoration** - Sliders read from processor state
- ✅ **Effect Rack Crash Fixes** - Three bugs resolved
- ✅ **Test Suite Expanded** - 396 assertions, 42 test cases

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
| PluginPoolManager | 6 | 89 | ✅ Pass |
| Audio Components | 8 | 42 | ✅ Pass |
| **Total** | **42** | **396** | ✅ All Pass |

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

1. **6B: Testing** - Add more unit tests (FilterGraph, MIDI mapping)
2. **6D.3: Thread safety** - Atomic cross-thread variables
3. **6C: Bug fixes** - Legacy issues from ToDo.txt
4. **6A: UI Polish** - Consider premium typography/icons

---

*Last updated: 2026-02-03*

