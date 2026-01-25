# State of the Project

> **For LLMs and Engineers:** This document provides persistent context about the current state of Pedalboard2 modernization. Read this first when joining the project.

---

## Current Focus: Phase 3 - VST3 & Core Features

**Status:** ✅ Core Complete (Needs UI Polish)  
**Last Updated:** 2026-01-24

### Recent Wins
- ✅ VST3 plugin format enabled and working
- ✅ Settings migrated to JSON (`SettingsManager`)
- ✅ UI visibility fixed (dark theme, high contrast text)
- ✅ VST3 I/O bus layouts enforced (stereo)
- ✅ App launches and scans plugins successfully

---

## Key Files Modified (Phase 3)

| File | Changes |
|------|---------|
| `SettingsManager.h/cpp` | **NEW** - JSON-based settings (replaces `PropertiesSingleton`) |
| `FilterGraph.cpp` | VST3 stereo I/O enforcement |
| `BranchesLAF.cpp` | High contrast UI colors |
| `App.cpp` | SettingsManager initialization |
| `MainPanel.cpp` | Settings migration |
| `PluginField.cpp` | Settings migration |

---

## Settings System

**Old:** `PropertiesSingleton` → JUCE `PropertiesFile` (XML/binary)  
**New:** `SettingsManager` → `nlohmann/json` → `settings.json`

Settings file location: `%APPDATA%\Pedalboard3\settings.json`

---

## Next Steps

1. **Phase 3 Remaining:** UI polish (theme system, zoom controls)
2. **Phase 4:** Quality & Stability (tests, profiling, refactoring)
3. Remove deprecated `PropertiesSingleton` (currently commented out)

---

## Quick Reference

| Component | Status |
|-----------|--------|
| VST3 Loading | ✅ Working |
| Plugin Scanning | ✅ Working |
| Settings Persistence | ✅ JSON |
| MIDI Mapping | ⚠️ Legacy (works) |
| OSC Mapping | ⚠️ Legacy (works) |
| Theme System | ⏳ Phase 3C |

---

*Last updated: 2026-01-24*
