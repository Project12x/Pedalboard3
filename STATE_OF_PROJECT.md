# State of the Project

> **For LLMs and Engineers:** This document provides persistent context about the current state of Pedalboard2 modernization. Read this first when joining the project.

---

## Current Focus: JUCE 8 Migration

**Status:** 🔄 In Progress  
**Blocker:** File editing requires closing IDE to apply PowerShell fix script

### What's Happening

We're modernizing an abandoned JUCE 6 audio plugin host (Pedalboard2) to:
1. Build with **JUCE 8.x** and modern C++17
2. Support **VST3 plugins** (instead of deprecated VST2)
3. Add modern **theming and UI**

### Build System

| Component | Status |
|-----------|--------|
| JUCE 8 submodule | ✅ Added |
| CMakeLists.txt | ✅ Created |
| Visual Studio 2022 | ✅ Configured |
| Successful build | ⏳ Pending API fixes |

---

## JUCE 8 API Migration Status

| Pattern | Count | Status |
|---------|-------|--------|
| `NodeID` → `NodeID.uid` | ~10 | 🔄 Fix script ready |
| `Node::Ptr` → `.get()` | ~8 | 🔄 Fix script ready |
| `XmlDocument` unique_ptr | ~6 | 🔄 Fix script ready |
| `ScopedPointer` removal | ~3 | 🔄 Fix script ready |
| `createItemComponent` | 1 | 🔄 Fix script ready |
| `KnownPluginList::sort` | 1 | 🔄 Fix script ready |

### To Apply Fixes

```powershell
# Close any IDE first, then:
cd "c:\Users\estee\Desktop\My Stuff\Code\Antigravity\Pedalboard2"
.\apply_juce8_fixes.ps1
cmake --build build --config Release
```

---

## Key Files Modified

| File | Changes |
|------|---------|
| `PluginField.cpp` | NodeID.uid, Node::Ptr.get() |
| `PluginComponent.cpp` | NodeID.uid |
| `FilterGraph.h/.cpp` | Connection API (vector-based) |
| `ApplicationMappingsEditor.h` | createItemComponent unique_ptr |
| `MainPanel.cpp` | XmlDocument, ScopedPointer, sort API |

---

## Repository Setup

| Remote | URL | Purpose |
|--------|-----|---------|
| `origin` | github.com/Project12x/Pedalboard2 | Your fork |
| `upstream` | github.com/lrq3000/Pedalboard2 | Original repo |

---

## Project Documentation

| Document | Purpose |
|----------|---------|
| `PHASED_PLAN.md` | Roadmap with all phases and legacy TODOs |
| `ARCHITECTURE.md` | Codebase structure and patterns |
| `apply_juce8_fixes.ps1` | PowerShell script for API migration |
| `ToDo.txt` | Original abandoned project TODOs (archived) |

---

## Next Steps

1. Apply JUCE 8 fix script
2. Verify clean build
3. Test basic functionality
4. Commit to fork
5. Begin Phase 1 (VST3 + themes)

---

*Last updated: 2026-01-22*
