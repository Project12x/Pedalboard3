# State of the Project

> **For LLMs and Engineers:** This document provides persistent context about the current state of Pedalboard3 modernization. Read this first when joining the project.

---

## Current Focus: Effect Rack GUI Stabilization

**Status:** ✅ Functional - Core Features Working  
**Last Updated:** 2026-02-03

### Current Session Status

Effect Rack editor is now functional:
- ✅ Editor opens and closes without crash
- ✅ Editor can be reopened multiple times without crash
- ✅ Nodes visible (Audio Input, Output, MIDI Input)
- ✅ Can load external VST3/AU plugins into rack
- ✅ ToneGenerator sliders restore correctly when editor reopened
- ⚠️ Pin-to-pin connection wiring not yet implemented

### Previous Wins (Feb 2026)

- ✅ **Effect Rack Re-entry Fixed** - Editor can now be reopened multiple times:
  - Switched to fresh editor creation each time
  - Pin bounds checking prevents out-of-range access
  - Fixed iterator invalidation when rebuilding graph
- ✅ **ToneGenerator State Restoration** - Sliders read from processor state, not defaults
- ✅ **Effect Rack Crash Fixes** - Three bugs resolved:
  - `resized()` timing: viewport/canvas had zero bounds before components existed
  - Double-delete in `SubGraphCanvas` destructor: OwnedArray already manages lifetime
  - Null pointer in `PluginPinComponent`: parent is SubGraphCanvas, not PluginField
- ✅ **Test Suite Complete** - 337 assertions, 39 test cases (Phase 4)
- ✅ SubGraphProcessor implemented (nested AudioProcessorGraph)
- ✅ XML serialization for Effect Racks in patches
- ✅ Effect Rack appears in Pedalboard menu

---

## Effect Rack Status

| Component | Status |
|-----------|--------|
| SubGraphProcessor | ✅ Stable |
| SubGraphEditorComponent | ✅ Functional |
| Add plugins in rack | ✅ Working |
| Connection management | ⚠️ Not implemented |
| State persistence | ✅ Tested (234 assertions) |

**Key Files:**
- `src/SubGraphProcessor.cpp/h`
- `src/SubGraphEditorComponent.cpp/h`
- `src/PluginField.cpp/h` (reference for canvas implementation)

---

## Test Suite Status

| Test File | Assertions | Status |
|-----------|------------|--------|
| integration_test.cpp | 42 | ✅ Pass |
| subgraph_processor_test.cpp | 67 | ✅ Pass |
| subgraph_filterGraph_test.cpp | 54 | ✅ Pass |
| plugin_pool_manager_test.cpp | 89 | ✅ Pass |
| tone_generator_test.cpp | 85 | ✅ Pass |
| **Total** | **337** | ✅ All Pass |

---

## Debugging Lessons Learned

1. **Windows crash dumps**: `%LOCALAPPDATA%\CrashDumps\` - open in VS debugger
2. **spdlog logging**: `%APPDATA%\Pedalboard3\debug.log` - use `flush()` before crash points
3. **Critical JUCE gotcha**: `setSize()` triggers `resized()` immediately

---

## Quick Reference

| Component | Status |
|-----------|--------|
| VST3 Loading | ✅ Working |
| Plugin Scanning | ✅ Working |
| Settings Persistence | ✅ JSON |
| Theme System | ✅ Complete |
| Effect Rack | ⚠️ GUI reimplementation in progress |
| MIDI Mapping | ⚠️ Legacy (works) |
| OSC Mapping | ⚠️ Legacy (works) |

---

## Next Steps

1. **Implement cable wiring** - Pin-to-pin connection in SubGraph editor
2. **Test full SubGraph workflow** - Add/remove plugins, save/load racks
3. **Cleanup** - Remove debug logging from various files

---

*Last updated: 2026-02-03*
