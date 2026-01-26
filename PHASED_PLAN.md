# Pedalboard3 Modernization - Phased Implementation Plan

A roadmap for modernizing Pedalboard3 into **Pedalboard3** - a modern VST3/CLAP pedalboard host.

> **Ordering Principle:** Each phase builds on the previous. Complete phases in order.

---

## Business Model (Aseprite-style, GPL v3)

| Tier | Release | Price | Content |
|------|---------|-------|---------|
| **Community** | Phase 4 | Free | Modernized Pedalboard3 + Undo + CLAP |
| **Pro** | Phase 5+ | $15-25 | Setlist, Stage Mode, Input devices |

---

## Status Overview

| Phase | Name | Status | Release |
|-------|------|--------|---------|
| 1 | JUCE 8 Migration | ✅ Complete | — |
| 2 | Foundation & Tooling | ✅ Complete | — |
| 3 | Core Features & UI | ✅ Complete | — |
| **4** | **Launch** | 🎯 NEXT | **v3.0 Community** |
| 5 | Pro Features | ⏳ Future | v3.1 Pro |
| 6 | Stability & Polish | ⏳ Future | v3.2 |
| 7 | Distribution | ⏳ Future | v3.3 |
| 8 | Advanced | ⏳ Future | v4.0+ |

---

## Phase 1: JUCE 8 Migration ✅ COMPLETE

**Goal:** Get the codebase building with JUCE 8

| # | Task | Status |
|---|------|--------|
| 1.1 | Update JUCE submodule to 8.x | ✅ Done |
| 1.2 | Create CMakeLists.txt | ✅ Done |
| 1.3 | Fix NodeID.uid conversions | ✅ Done |
| 1.4 | Fix Node::Ptr.get() calls | ✅ Done |
| 1.5 | Fix unique_ptr ownership | ✅ Done |
| 1.6 | Fix ScopedPointer removal | ✅ Done |
| 1.7 | Build and smoke test | ✅ Done |

---

## Phase 2: Foundation & Tooling ✅ COMPLETE

**Goal:** Establish modern development foundation

### 2A: Build System
| # | Task | Status |
|---|------|--------|
| 2A.1 | CPM.cmake dependency manager | ✅ Done |
| 2A.2 | .clang-format | ✅ Done |
| 2A.3 | CMakePresets.json | ✅ Done |
| 2A.4 | Compiler warnings `/W4` | ✅ Done |

### 2B: Core Libraries
| # | Library | Purpose | Status |
|---|---------|---------|--------|
| 2B.1 | fmt | String formatting | ✅ Done |
| 2B.2 | spdlog | Async logging | ✅ Done |
| 2B.3 | nlohmann/json | JSON handling | ✅ Done |

### 2C: Testing Framework
| # | Task | Status |
|---|------|--------|
| 2C.1 | Catch2 integration | ✅ Done |
| 2C.2 | tests/ directory | ✅ Done |
| 2C.3 | First smoke test | ✅ Done (11 assertions) |

---

## Phase 3: Core Features & UI ✅ COMPLETE

**Goal:** Working VST3 host with essential features

### 3A: Plugin System
| # | Task | Status |
|---|------|--------|
| 3A.1 | Enable VST3PluginFormat | ✅ Done |
| 3A.2 | 64-bit native build | ✅ Done |
| 3A.3 | Background plugin scanning | ✅ Working |
| 3A.4 | Settings → JSON (SettingsManager) | ✅ Done |

### 3B: Theme System
| # | Task | Status |
|---|------|--------|
| 3B.1 | 5 built-in themes | ✅ Done |
| 3B.2 | Theme dropdown selector | ✅ Done |
| 3B.3 | Live LookAndFeel refresh | ✅ Done |
| 3B.4 | Delete protection for built-ins | ✅ Done |
| 3B.5 | Menu text visibility fix | ✅ Done |
| 3B.6 | JUCE 8 LookAndFeel fixes (setColour order) | ✅ Done |

**Themes:** Midnight (default), Daylight, Synthwave, Deep Ocean, Forest

---

## Phase 4: Launch 🎯 NEXT

**Goal:** v3.0 Community Edition release

> **Release:** Free on GitHub, announce on KVR/Reddit

### 4A: Undo/Redo ✅ COMPLETE
| # | Task | Status |
|---|------|--------|
| 4A.1 | Implement UndoManager | ✅ Done |
| 4A.2 | Track plugin add/remove | ✅ Done |
| 4A.3 | Track connection changes | ✅ Done |
| 4A.4 | Panic button | ✅ Done |

### 4B: CLAP Plugin Support ⏸️ DEFERRED
> **Note:** clap-juce-extensions not yet stable with JUCE 8. Will revisit when mature.

| # | Task | Status |
|---|------|--------|
| 4B.1 | Add clap-juce-extensions | ⏸️ Deferred |
| 4B.2 | Enable CLAPPluginFormat | ⏸️ Deferred |
| 4B.3 | Test CLAP loading | ⏸️ Deferred |

### 4C: Launch Prep
| # | Task | Status |
|---|------|--------|
| 4C.1 | Rename to Pedalboard3 | ✅ Done |
| 4C.2 | Update README.md | ✅ Done |
| 4C.3 | Create CHANGELOG.md | ✅ Done |
| 4C.4 | Code hygiene cleanup | ✅ Done |
| 4C.5 | Email draft to Niall | ✅ Done |
| 4C.6 | Set up donations | ⏳ Planned |
| 4C.7 | Draft announcement | ⏳ Planned |

---

## Phase 5: Pro Features 💰

**Goal:** Compete with Gig Performer, Cantabile

> **Release:** v3.1 Pro — Sell on Steam/itch.io ($15-25)

### 5A: Setlist & Performance
| # | Feature | From |
|---|---------|------|
| 5A.1 | **Setlist management** | Gig Performer |
| 5A.2 | **Predictive loading** | Gig Performer |
| 5A.3 | **Song states/variations** | Cantabile |
| 5A.4 | **Stage Mode** (large UI) | Cantabile |

### 5B: Input Devices
| # | Device | Library |
|---|--------|---------|
| 5B.1 | Gamepad/joystick | SDL2 |
| 5B.2 | USB footswitches | hidapi |
| 5B.3 | Serial port | libserialport |
| 5B.4 | Stream Deck | hidapi |
| 5B.5 | OSC controllers | liblo |

### 5C: Live Performance & Built-In Tools
| # | Feature | Notes |
|---|---------|-------|
| 5C.1 | **Chromatic Tuner node** | Built-in, no plugin needed |
| 5C.2 | **Virtual Keyboard node** | On-screen MIDI keyboard |
| 5C.3 | **MIDI File Player node** | Play .mid files through chain |
| 5C.4 | Cue list | Scripted shows |
| 5C.5 | Backing track player | Audio files |
| 5C.6 | Wire visualization | Beautiful graphs |
| 5C.7 | Plugin preset nav | MIDI/OSC next/prev |
| 5C.8 | **Ableton Link** | Sync with other musicians |

### 5D: Database & Cache
| # | Library | Purpose |
|---|---------|---------|
| 5D.1 | SQLite | Plugin cache |
| 5D.2 | sqlite_orm | C++ wrapper |

### 5E: Advanced Routing
| # | Feature | Notes |
|---|---------|-------|
| 5E.1 | **A/B splitter node** | Send to two parallel chains |
| 5E.2 | **Y merger node** | Combine two chains back |
| 5E.3 | **Wet/dry mix node** | Blend parallel paths |
| 5E.4 | **Crossfade node** | Smooth transition between A/B |

---

## Phase 6: Stability & Polish

**Goal:** Production-ready quality

### 6A: Visual Polish
| # | Task | License |
|---|------|---------|
| 6A.1 | Inter font | SIL OFL |
| 6A.2 | JetBrains Mono | SIL OFL |
| 6A.3 | Lucide icons | ISC |
| 6A.4 | blend2d vectors | zlib |

### 6B: Testing
| # | Task | Notes |
|---|------|-------|
| 6B.1 | FilterGraph tests | Unit tests |
| 6B.2 | MIDI mapping tests | CC assignment |
| 6B.3 | Plugin loading tests | VST3/CLAP |
| 6B.4 | Tracy profiler | Performance |

### 6C: Bug Fixes (Legacy)
| # | Issue |
|---|-------|
| 6C.1 | Connection drawing backwards |
| 6C.2 | Patch Organiser sync |
| 6C.3 | AudioRecorder MIDI trigger |
| 6C.4 | Start minimized |
| 6C.5 | Home key mapping |

### 6D: Code Refactoring
| # | Task | Notes |
|---|------|-------|
| 6D.1 | Split PedalboardProcessors | One file each |
| 6D.2 | Global loadSVGFromMemory() | Remove duplication |
| 6D.3 | Atomic cross-thread vars | Thread safety |

### 6E: Legacy UX (Niall's ToDo.txt)
| # | Task |
|---|------|
| 6E.1 | Snap to grid |
| 6E.2 | Zoom out mode |
| 6E.3 | Plugin pins enlarge on hover |
| 6E.4 | Hotkey to bypass all |
| 6E.5 | Tempo display improvements |

### 6F: Cross-Platform
| # | Task | Notes |
|---|------|-------|
| 6F.1 | macOS build | CMake + Xcode |
| 6F.2 | Linux build | CMake + GCC |

### 6G: Headless Mode (Hardware Prep)
| # | Task | Notes |
|---|------|-------|
| 6G.1 | `--no-gui` execution | Run without display |
| 6G.2 | JSON/OSC control API | Remote preset switching |
| 6G.3 | Systemd service file | Auto-start on Linux boot |

---

## Phase 7: Distribution

**Goal:** Production distribution

### 7A: CI/CD
| # | Task | Tool |
|---|------|------|
| 7A.1 | GitHub Actions | Automated builds |
| 7A.2 | Multi-platform builds | Win/Mac/Linux |
| 7A.3 | Test automation | CI runs |

### 7B: Installers & Sales
| # | Task | Tool |
|---|------|------|
| 7B.1 | Windows installer | NSIS |
| 7B.2 | macOS DMG | create-dmg |
| 7B.3 | Steam integration | Steamworks |
| 7B.4 | itch.io page | Alt store |
| 7B.5 | Auto-update (Win) | WinSparkle |
| 7B.6 | Code signing | Certs |

### 7C: Crash Reporting
| # | Task | Tool |
|---|------|------|
| 7C.1 | Crash dumps | Crashpad |
| 7C.2 | Cloud reports | Sentry |

---

## Phase 8: Advanced Features

**Goal:** Future competitive features

### 8A: Networking
| # | Feature | Library |
|---|---------|---------|
| 8A.1 | WebSocket remote | websocketpp |
| 8A.2 | Preset cloud sync | libcurl |
| 8A.3 | Zeroconf | mdns |

### 8B: Plugin Formats
| # | Format | Notes |
|---|--------|-------|
| 8B.1 | LV2 | Linux standard |
| 8B.2 | Plugin sandboxing | Crash isolation |
| 8B.3 | 32↔64-bit bridging | Run legacy plugins |
| 8B.4 | SoundFont (SF2/SFZ) | Built-in instruments |

### 8C: Future
| # | Feature | Notes |
|---|---------|-------|
| 8C.1 | Accessibility | Screen reader |
| 8C.2 | Localization | i18n |
| 8C.3 | Touch interface | Large targets |
| 8C.4 | OSC timeline | Sequencing |
| 8C.5 | Mobile remote | Phone control |
| 8C.6 | Scripting | Like GP Script |

### 8D: Hardware Integration
| # | Feature | Notes |
|---|---------|-------|
| 8D.1 | ARM Linux build | Raspberry Pi 4/5 |
| 8D.2 | JACK audio backend | Pro Linux audio |
| 8D.3 | GPIO control | Physical buttons/LEDs |
| 8D.4 | LCD/OLED display | Simple hardware UI |
| 8D.5 | Minimal memory mode | Optimize for 2GB RAM |

---

## Competitive Comparison

| Feature | Gig Performer | Cantabile | **Pedalboard3** |
|---------|---------------|-----------|-----------------|
| Price | $170+ | $199 + sub | Free / $20 |
| VST3 + CLAP | ✅ | ✅ | ✅ Phase 4 |
| Setlist | ✅ | ✅ | ✅ Phase 5 |
| Input devices | Limited | Limited | ✅ Phase 5 |
| Open Source | ❌ | ❌ | ✅ GPL v3 |

---

## Library Quick Reference

| Phase | Libraries |
|-------|-----------|
| 2 | CPM, fmt, spdlog, nlohmann/json, Catch2 |
| 4 | clap-juce-extensions |
| 5 | SDL2, hidapi, libserialport, liblo, SQLite |
| 6 | Inter, JetBrains Mono, Lucide, blend2d, Tracy |
| 7 | WinSparkle, Crashpad, Sentry, Steamworks |
| 8 | websocketpp, libcurl, mdns |

---

*Last updated: 2026-01-25*
