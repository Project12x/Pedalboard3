# Pedalboard3 Modernization - Phased Implementation Plan

A roadmap for modernizing Pedalboard2 into **Pedalboard3** - a modern VST3/CLAP pedalboard host application.

> **Ordering Principle:** Each phase builds on the previous. Complete phases in order; tasks within phases can be parallelized.

---

## Business Model (Aseprite-style, GPL-compliant)

| Tier | Description | Price |
|------|-------------|-------|
| **Community** | Phase 4 completion (modernized Pedalboard2) | Free on GitHub |
| **Pro** | Phase 5+ features (competitive with Gig Performer) | $15-25 on Steam/itch.io |

**GPL v3 allows:** Selling binaries, charging for support/updates, keeping source public.

**Competitors:** Gig Performer (~$170), Cantabile (~$199), MainStage ($40), LiveProfessor ($100)

---

## Status Overview

| Phase | Name | Status | Release |
|-------|------|--------|---------|
| **1** | JUCE 8 Migration | ✅ Complete | — |
| **2** | Foundation & Tooling | ✅ Complete | — |
| **3** | Core Features & UI | ✅ Complete | — |
| **4** | Launch & Competitive | 🎯 **NEXT** | **v3.0 Community** |
| **5** | Pro Features | ⏳ Future | v3.1+ Pro |
| **6** | Distribution | ⏳ Future | — |
| **7** | Advanced Features | ⏳ Future | — |

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

**Themes:** Midnight (default), Daylight, Synthwave, Deep Ocean, Forest

---

## Phase 4: Launch & Competitive Features 🎯 NEXT

**Goal:** Close gaps with competitors, prepare v3.0 Community release

> **Release:** v3.0 Community Edition (FREE) — Announce on KVR, Reddit, GitHub

### 4A: Undo/Redo System (Priority)
| # | Task | Notes |
|---|------|-------|
| 4A.1 | Implement UndoManager | JUCE built-in |
| 4A.2 | Track plugin add/remove | Undoable actions |
| 4A.3 | Track connection changes | Undoable actions |
| 4A.4 | Panic button (all notes off) | Quick win, Gig Performer has this |

### 4B: CLAP Plugin Support
| # | Task | Notes |
|---|------|-------|
| 4B.1 | Add clap-juce-extensions | FetchContent |
| 4B.2 | Enable CLAPPluginFormat | Similar to VST3 |
| 4B.3 | Test CLAP plugin loading | Verify compatibility |

### 4C: Launch Prep (Pedalboard3)
| # | Task | Notes |
|---|------|-------|
| 4C.1 | Rename to Pedalboard3 | CMakeLists, App.cpp, docs |
| 4C.2 | Update README.md | Features, screenshots, badges |
| 4C.3 | Create CHANGELOG.md | v3.0 release notes |
| 4C.4 | Set up donations | GitHub Sponsors / Ko-fi |
| 4C.5 | Draft announcement | KVR, Reddit, r/synthesizers, r/guitarpedals |

### 4D: Cross-Platform (Stretch Goal)
| # | Task | Notes |
|---|------|-------|
| 4D.1 | macOS build attempt | CMake + Xcode |
| 4D.2 | Linux build attempt | CMake + GCC |
| 4D.3 | Platform-specific fixes | As needed |

### 4E: Professional Polish
| # | Task | License | Notes |
|---|------|---------|-------|
| 4E.1 | Inter font (UI) | SIL OFL | Modern, clean sans-serif |
| 4E.2 | JetBrains Mono (values) | SIL OFL | Numbers, parameter values |
| 4E.3 | Lucide icons | ISC | Clean SVG icon set |
| 4E.4 | Font embedding in binary | JUCE | BinaryData::fonts |

---

## Phase 5: Pro Features 💰

**Goal:** Compete with Gig Performer, Cantabile, LiveProfessor

> **Release:** v3.1+ Pro — Sell binaries on Steam/itch.io ($15-25)

### 5A: Setlist & Performance (from Gig Performer/Cantabile)
| # | Feature | Competes With |
|---|---------|---------------|
| 5A.1 | **Setlist management** | Gig Performer, Cantabile |
| 5A.2 | **Predictive loading** | Gig Performer (preload next song) |
| 5A.3 | **Song states/variations** | Cantabile (verse/chorus) |
| 5A.4 | **Stage Mode** (large UI) | Cantabile Live Mode |
| 5A.5 | Concert/Set/Patch hierarchy | MainStage |

### 5B: Live Performance (from LiveProfessor/MainStage)
| # | Feature | Competes With |
|---|---------|---------------|
| 5B.1 | **Cue list** (scripted shows) | LiveProfessor |
| 5B.2 | **Backing track player** | MainStage, Gig Performer |
| 5B.3 | Wire visualization | Gig Performer (beautiful flow diagrams) |
| 5B.4 | Click track / metronome improvements | All |

### 5C: Plugin Management
| # | Feature | Notes |
|---|---------|-------|
| 5C.1 | Plugin preset navigation | Next/prev via MIDI/OSC |
| 5C.2 | SQLite plugin cache | Fast scanning |
| 5C.3 | Plugin favorites & categories | Organization |
| 5C.4 | MIDI Output support | Full support |
| 5C.5 | Router processor | 4-in/4-out toggle |
| 5C.6 | Global plugin field | Persist across patches |

### 5D: Testing & Stability
| # | Task | Notes |
|---|------|-------|
| 5D.1 | FilterGraph unit tests | Node creation, connections |
| 5D.2 | MIDI mapping tests | CC assignment, ranges |
| 5D.3 | Plugin loading tests | VST3/CLAP scan |
| 5D.4 | Tracy profiler | Performance |

### 5E: Bug Fixes (Legacy)
| # | Issue |
|---|-------|
| 5E.1 | Connection drawing backwards |
| 5E.2 | Patch Organiser sync |
| 5E.3 | AudioRecorder MIDI trigger |
| 5E.4 | MIDI/OSC reliability |
| 5E.5 | Start minimized |
| 5E.6 | Home key mapping |
| 5E.7 | Document in titlebar |

### 5F: Legacy UX (from Niall's ToDo.txt)
| # | Task | Notes |
|---|------|-------|
| 5F.1 | Snap to grid | Grid layout for plugins |
| 5F.2 | Zoom out mode | Reduce plugin component size |
| 5F.3 | Plugin pins enlarge on hover | Better UX |
| 5F.4 | Audio Output anchored right | Always at right |
| 5F.5 | No-setup record button | Quick recording |
| 5F.6 | Hotkey to bypass all | Global bypass |
| 5F.7 | Tempo display improvements | Make hidable |
| 5F.8 | Custom sliders/toggles in LAF | Polish |

### 5G: Documentation
| # | Task | Notes |
|---|------|-------|
| 5G.1 | Set up Doxygen | Integrate with CMake |
| 5G.2 | Generate API docs | HTML output |
| 5G.3 | Update ARCHITECTURE.md | Final structure |
| 5G.4 | CONTRIBUTING.md | For contributors |

---

## Phase 6: Distribution

**Goal:** Production distribution

### 6A: CI/CD Pipeline
| # | Task | Tool |
|---|------|------|
| 6A.1 | GitHub Actions workflow | Automated builds |
| 6A.2 | Windows/macOS/Linux builds | Multi-platform |
| 6A.3 | Artifact publishing | Release binaries |
| 6A.4 | Test automation | CI test runs |

### 6B: Installers & Sales
| # | Task | Tool |
|---|------|------|
| 6B.1 | Windows installer | NSIS or WiX |
| 6B.2 | macOS DMG | create-dmg |
| 6B.3 | Steam integration | Steamworks |
| 6B.4 | itch.io page | Alternative store |
| 6B.5 | Auto-update (Win) | WinSparkle |
| 6B.6 | Code signing | Platform certs |

### 6C: Crash Reporting
| # | Task | Tool |
|---|------|------|
| 6C.1 | Crash dumps | Crashpad |
| 6C.2 | Cloud reporting | Sentry (optional) |

---

## Phase 7: Advanced Features

**Goal:** Competitive feature set for the future

### 7A: Plugin Formats
| # | Format | Library |
|---|--------|---------|
| 7A.1 | LV2 support | Linux focus |
| 7A.2 | Plugin sandboxing | Out-of-process (crash isolation) |

### 7B: Networking
| # | Feature | Library |
|---|---------|---------|
| 7B.1 | WebSocket remote | websocketpp |
| 7B.2 | Preset cloud sync | libcurl |
| 7B.3 | Zeroconf discovery | mdns |

### 7C: Input Devices (Expanded)
| # | Feature | Library | Notes |
|---|---------|---------|-------|
| 7C.1 | Gamepad/joystick | SDL2 (zlib) | Expression pedal via analog sticks |
| 7C.2 | USB footswitches | hidapi (BSD) | PageFlip, AirTurn, etc. |
| 7C.3 | Serial port | libserialport (LGPL) | Arduino, DIY controllers |
| 7C.4 | Expression pedals | MIDI native | Already supported |
| 7C.5 | Stream Deck | hidapi | Elgato buttons |
| 7C.6 | OSC controllers | liblo | TouchOSC, Lemur |
| 7C.7 | MIDI controllers | JUCE | Already supported |
| 7C.8 | Custom HID devices | hidapi | DIY footswitches |

### 7D: UI Enhancements
| # | Feature | Library |
|---|---------|---------|
| 7D.1 | GPU vector graphics | blend2d / NanoVG |
| 7D.2 | Animations | Lottie / rive-cpp |
| 7D.3 | Additional themes | Industrial, Stage, Blueprint |

### 7E: Future
| # | Feature | Notes |
|---|---------|-------|
| 7E.1 | Accessibility | Screen reader, keyboard nav |
| 7E.2 | Localization (i18n) | Multi-language |
| 7E.3 | Touch interface | Large hit targets |
| 7E.4 | OSC timeline | Generalized sequencing |
| 7E.5 | Mobile remote app | Phone/tablet control |
| 7E.6 | Scripting language | Like GP Script |

---

## Competitive Comparison

| Feature | Gig Performer | Cantabile | **Pedalboard3** |
|---------|---------------|-----------|-----------------|
| **Price** | $170+ | $199 + sub | Free / $20 |
| VST3 + CLAP | ✅ | ✅ | ✅ Phase 4 |
| Rapid patch queue | ❌ | Limited | ✅ **Unique** |
| Setlist mgmt | ✅ | ✅ | ✅ Phase 5 |
| Stage Mode | ❌ | ✅ | ✅ Phase 5 |
| Open Source | ❌ | ❌ | ✅ GPL v3 |
| Themes | ❌ | ❌ | ✅ 5 built-in |

---

## Library Quick Reference

| Phase | Libraries |
|-------|-----------|
| **2** | CPM.cmake, fmt, spdlog, nlohmann/json, Catch2 |
| **4** | clap-juce-extensions, Inter font, JetBrains Mono, Lucide icons |
| **5** | Tracy, SQLite, sqlite_orm, blend2d/NanoVG |
| **6** | Crashpad, Sentry, WinSparkle, Sparkle, Steamworks |
| **7** | websocketpp, libcurl, SDL2, hidapi, libserialport, liblo |

---

## Document Index

| Document | Purpose |
|----------|---------|
| `PHASED_PLAN.md` | This roadmap |
| `STATE_OF_PROJECT.md` | Current state for LLMs/engineers |
| `ARCHITECTURE.md` | Codebase structure and patterns |
| `ToDo.txt` | Original TODOs (archived - integrated here) |

---

*Last updated: 2026-01-24*
