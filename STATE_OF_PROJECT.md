# State of the Project

> **For LLMs and Engineers:** This document provides persistent context about the current state of Pedalboard2 modernization. Read this first when joining the project.

---

## Current Focus: Phase 4 - Plugin Management

**Status:** ✅ Phase 3 Complete  
**Last Updated:** 2026-01-24

### Recent Wins (Phase 3)

- ✅ VST3 plugin format enabled and working
- ✅ Settings migrated to JSON (`SettingsManager`)
- ✅ UI visibility fixed (dark theme, high contrast text)
- ✅ VST3 I/O bus layouts enforced (stereo)
- ✅ 5 built-in themes: Midnight, Daylight, Synthwave, Deep Ocean, Forest
- ✅ Theme switching with live LookAndFeel refresh
- ✅ Delete confirmation for custom presets

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

## Key Files Modified (Phase 3)

| File | Changes |
|------|---------|
| `SettingsManager.h/cpp` | **NEW** - JSON-based settings |
| `ColourScheme.h/cpp` | Built-in theme presets |
| `BranchesLAF.h/cpp` | `refreshColours()` for live updates |
| `ColourSchemeEditor.cpp` | Theme dropdown, delete protection |
| `FilterGraph.cpp` | VST3 stereo I/O enforcement |

---

## Settings System

**Location:** `%APPDATA%\Pedalboard3\settings.json`  
**Engine:** `SettingsManager` → `nlohmann/json`

---

## Next Steps

1. **Phase 4:** Plugin Management (better scanning, organize, favorites)
2. **Phase 5:** Deferred features (zoom, snap-to-grid, libremidi)
3. Remove deprecated `PropertiesSingleton` code

---

## Quick Reference

| Component | Status |
|-----------|--------|
| VST3 Loading | ✅ Working |
| Plugin Scanning | ✅ Working |
| Settings Persistence | ✅ JSON |
| Theme System | ✅ Complete |
| MIDI Mapping | ⚠️ Legacy (works) |
| OSC Mapping | ⚠️ Legacy (works) |

---

*Last updated: 2026-01-24*
