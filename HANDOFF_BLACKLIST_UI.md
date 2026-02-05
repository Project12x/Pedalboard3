# Plugin Blacklist UI Handoff

**Date:** 2026-02-05  
**Feature:** BlacklistWindow UI for Plugin Management  
**Status:** ✅ Complete

---

## Summary

Implemented a dedicated UI window (`BlacklistWindow`) for viewing and managing the plugin blacklist. Users can now access this via **Options → Plugin Blacklist** from the main menu.

---

## Architecture

### New Files

| File | Purpose |
|------|---------|
| `src/BlacklistWindow.h` | Header with `BlacklistWindow`, `BlacklistComponent`, `BlacklistListModel` |
| `src/BlacklistWindow.cpp` | Implementation of blacklist management UI |

### Class Hierarchy

```
BlacklistWindow (DocumentWindow)
└── BlacklistComponent (Component)
    ├── titleLabel
    ├── infoLabel  
    ├── ListBox + BlacklistListModel
    ├── removeButton
    ├── clearAllButton
    └── closeButton
```

### Integration Points

1. **MainPanel.cpp** - Added `OptionsPluginBlacklist` command handler
2. **MainCommandIDs.h** - Registered command ID (already existed)
3. **CMakeLists.txt** - Added source files to build

---

## Key Implementation Details

### BlacklistListModel

Displays entries from `PluginBlacklist::getInstance()`:
- Combines paths (`getBlacklistedPaths()`) and IDs (`getBlacklistedIds()`)
- Paths prefixed with "[Path]", IDs with "[ID]"
- Supports multi-selection for batch removal

### BlacklistComponent

- **Remove Selected**: Calls `removeFromBlacklist()` or `removeFromBlacklistById()`
- **Clear All**: Calls `clearBlacklist()` 
- **Refresh**: Updates list from singleton after modifications
- Auto-sizes to 400x500 pixels

### BlacklistWindow

- Singleton pattern via `static std::unique_ptr<BlacklistWindow> instance`
- `showWindow()` creates or brings existing window to front
- Closes via close button or window decoration

---

## Commit Details

```
feat: add BlacklistWindow UI for plugin blacklist management

- Created BlacklistWindow.h/cpp with dedicated management UI
- Integrated into MainPanel Options menu
- Added to CMakeLists.txt build system
- Users can view, remove individual, or clear all blacklisted plugins
```

---

## Current Plugin Protection Status

### Completed ✅

| Component | Description |
|-----------|-------------|
| `PluginBlacklist` | Thread-safe singleton with SettingsManager persistence |
| `CrashProtection` | Windows SEH wrappers for safe plugin operations |
| Watchdog Thread | 15-second timeout detection for hung operations |
| FilterGraph Integration | Blocks blacklisted plugins at load time |
| `BlacklistWindow` | UI for manual blacklist management |

### Pending ⏳

| Component | Description |
|-----------|-------------|
| Timeout Logic | Terminate scans exceeding 30 seconds |
| Out-of-Process Scanner | Separate executable for crash isolation |

---

## Testing Notes

The blacklist UI was verified in a Release build:
1. Open Pedalboard3
2. Navigate to Options → Plugin Blacklist
3. Window appears with list of blacklisted entries
4. Remove/Clear All buttons function correctly
5. Window persists correctly across show/hide cycles

---

## Next Steps

1. **Implement Timeout Logic**
   - Add timer to plugin scan operations
   - Auto-blacklist plugins exceeding threshold
   - Show user notification

2. **Out-of-Process Scanner**
   - Create `Pedalboard3Scanner.exe` 
   - IPC via named pipes or shared memory
   - Main app monitors scanner process health

---

## Related Files

- `src/PluginBlacklist.h/cpp` - Core blacklist singleton
- `src/CrashProtection.h/cpp` - SEH wrappers and watchdog
- `src/FilterGraph.cpp` - `addFilterRaw()` blacklist check
- `src/MainPanel.cpp` - Menu integration

---

*Last updated: 2026-02-05*
