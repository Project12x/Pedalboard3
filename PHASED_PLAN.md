# Pedalboard2 Modernization - Phased Implementation Plan

A roadmap for modernizing Pedalboard2 into a modern VST3 pedalboard host application.

> **Ordering Principle:** Each phase builds on the previous. Complete phases in order; tasks within phases can be parallelized.

---

## Timeline & Estimates

### Phase Duration

| Phase | Name | Part-Time (10-15 hrs/wk) | Full-Time (40 hrs/wk) | Bottleneck |
|-------|------|--------------------------|----------------------|------------|
| **1** | JUCE 8 Migration | 1-2 weeks | 3-5 days | API fixes across many files |
| **2** | Foundation & Tooling | 2-3 weeks | 1 week | Library integration |
| **3** | VST3 & Core Features | 6-8 weeks | 2-3 weeks | Theme system UI iteration |
| **4** | Quality & Stability | 4-6 weeks | 2 weeks | Test coverage |
| **5** | Features & Bug Fixes | 8-12 weeks | 3-4 weeks | Undo/Redo state management |
| **6** | Distribution & Platform | 3-4 weeks | 1-2 weeks | CI/CD + code signing |
| **7** | Advanced & Future | 12+ weeks | 4-6 weeks | CLAP integration |

### Total Estimates

| Scenario | Total Time | MVP (Phases 1-3) |
|----------|------------|------------------|
| **Part-Time** (10-15 hrs/wk) | 9-12 months | 2-3 months |
| **Full-Time** (40 hrs/wk) | 3-4 months | 4-6 weeks |
| **With AI Pair Programming** | ~60-70% of above | — |

### Milestones

| Milestone | Phases | Part-Time | Full-Time | Deliverable |
|-----------|--------|-----------|-----------|-------------|
| **Buildable** | 1 | 1-2 weeks | 3-5 days | Compiles with JUCE 8 |
| **Usable VST3 Host** | 1-3 | 2-3 months | 4-6 weeks | Load VST3, basic UI |
| **Production Quality** | 1-4 | 4-5 months | 2-3 months | Tested, stable |
| **Shippable Product** | 1-6 | 7-9 months | 3-4 months | Installer, auto-update |
| **Feature Complete** | 1-7 | 12+ months | 5-6 months | All features |

---

## Phase 1: JUCE 8 Migration ✅ COMPLETE

**Status:** ✅ Complete  
**Prerequisite:** None  
**Goal:** Get the codebase building with JUCE 8

### Tasks (in order)
| # | Task | Status | Dependency |
|---|------|--------|------------|
| 1.1 | Update JUCE submodule to 8.x | ✅ Done | - |
| 1.2 | Create CMakeLists.txt | ✅ Done | 1.1 |
| 1.3 | Fix NodeID.uid conversions | ✅ Done | 1.2 |
| 1.4 | Fix Node::Ptr.get() calls | ✅ Done | 1.2 |
| 1.5 | Fix unique_ptr ownership | ✅ Done | 1.2 |
| 1.6 | Fix ScopedPointer removal | ✅ Done | 1.2 |
| 1.7 | Build and smoke test | ✅ Done | 1.3-1.6 |
| 1.8 | Remove dead/commented code | ⏳ Deferred to Phase 4 | 1.7 |
| 1.9 | Commit to fork | ✅ Done | 1.7 |

### Documentation
- [x] STATE_OF_PROJECT.md
- [x] ARCHITECTURE.md  
- [ ] Doxygen comments for API changes

---

## Phase 2: Foundation & Tooling ← **CURRENT**

**Status:** 🔄 In Progress  
**Prerequisite:** Phase 1 complete  
**Goal:** Establish modern development foundation

### 2A: Build System (do first)
| # | Task | Library/Tool | Status |
|---|------|--------------|--------|
| 2A.1 | Add dependency manager | **CPM.cmake** | ✅ Done |
| 2A.2 | Add .clang-format | Consistent style | ✅ Done |
| 2A.3 | Add CMakePresets.json | Debug/Release | ✅ Done |
| 2A.4 | Enable compiler warnings | `/W4 /FS` | ✅ Done |

### 2B: Core Libraries (integrate early)
| # | Library | Purpose | Replaces | Status |
|---|---------|---------|----------|--------|
| 2B.1 | **fmt** | String formatting | String::formatted() | ✅ Done |
| 2B.2 | **spdlog** | Async logging | LogFile | ✅ Done |
| 2B.3 | **nlohmann/json** | JSON handling | Custom XML | ✅ Done |
| 2B.4 | **toml++** | Config files | PropertiesFile | ⏳ Optional |
| 2B.5 | **CLI11** | Command-line args | Manual parsing | ⏳ Optional |

### 2C: Testing Framework
| # | Task | Library/Tool | Status |
|---|------|--------------|--------|
| 2C.1 | Add tests/ directory | Structure | ✅ Done |
| 2C.2 | Integrate Catch2 | **Catch2** | ✅ Done |
| 2C.3 | First smoke test | Verify build in test | ✅ Done (11 assertions) |
| 2C.4 | Add mock framework | **FakeIt** (optional) | ⏳ Optional |

### Documentation
- [ ] Update ARCHITECTURE.md with new libs
- [ ] Add CONTRIBUTING.md

---

## Phase 3: VST3 & Core Features

**Status:** 🔄 In Progress  
**Prerequisite:** Phase 2 complete  
**Goal:** Working VST3 host with essential features

### 3A: Plugin System
| # | Task | Notes | Status |
|---|------|-------|--------|
| 3A.1 | Enable VST3PluginFormat | Add to format manager | ✅ Done |
| 3A.2 | 64-bit build | Native x64 | ✅ Done |
| 3A.3 | Plugin scanning improvements | Background, caching | ✅ Working |
| 3A.4 | Plugin state persistence | Save/restore parameters | ⏳ Pending |
| 3A.5 | Settings migration to JSON | SettingsManager | ✅ Done |

### 3B: MIDI/OSC Modernization
| # | Task | Library |
|---|------|---------|
| 3B.1 | Replace MIDI handling | **libremidi** |
| 3B.2 | Replace OSC handling | **liblo** |
| 3B.3 | MIDI learn improvements | Visual feedback |
| 3B.4 | Expression curve editor | Velocity curves |

### 3C: UI Foundation
| # | Task | Notes |
|---|------|-------|
| 3C.1 | Theme system architecture | ThemeManager |
| 3C.2 | Midnight theme (default) | Dark modern |
| 3C.3 | Update BranchesLAF | Modern controls |
| 3C.4 | Zoom controls | Canvas zoom |
| 3C.5 | Snap to grid | Plugin alignment |

### Documentation
- [ ] Theme system README
- [ ] Plugin format support guide

---

## Phase 4: Quality & Stability

**Status:** ⏳ Future  
**Prerequisite:** Phase 3 functional  
**Goal:** Production-ready quality

### 4A: Testing Expansion
| # | Task | Notes |
|---|------|-------|
| 4A.1 | FilterGraph unit tests | Node creation, connections |
| 4A.2 | MIDI mapping tests | CC assignment, ranges |
| 4A.3 | Plugin loading tests | VST3 scan, instantiate |
| 4A.4 | Performance benchmarks | **Google Benchmark** |

### 4B: Profiling & Debugging
| # | Tool | Purpose |
|---|------|---------|
| 4B.1 | **Tracy** | Frame profiler |
| 4B.2 | AddressSanitizer | Memory bugs |
| 4B.3 | ThreadSanitizer | Race conditions |
| 4B.4 | Self-test mode | `--self-test` CLI |

### 4C: Static Analysis
| # | Tool | Purpose |
|---|------|---------|
| 4C.1 | clang-tidy | Linting |
| 4C.2 | cppcheck | Static analysis |
| 4C.3 | include-what-you-use | Header cleanup |

### 4D: Code Refactoring
| # | Task | Notes |
|---|------|-------|
| 4D.1 | Split PedalboardProcessors | One file per processor |
| 4D.2 | Global loadSVGFromMemory() | Remove duplication |
| 4D.3 | BypassableInstance → ChangeBroadcaster | Better pattern |
| 4D.4 | Atomic cross-thread vars | Thread safety |
| 4D.5 | Replace Event Log TextEditor | Use ListBox |

### Documentation
- [ ] Complete Doxygen coverage
- [ ] API reference generation

---

## Phase 5: Features & Bug Fixes

**Status:** ⏳ Future  
**Prerequisite:** Phase 4 stable  
**Goal:** Feature-complete application

### 5A: Critical Features
| # | Feature | Priority |
|---|---------|----------|
| 5A.1 | **Undo/Redo system** | High |
| 5A.2 | **Plugin preset nav** | High |
| 5A.3 | **SQLite plugin cache** | High |

### 5B: State Management Libraries
| # | Library | Purpose |
|---|---------|---------|
| 5B.1 | **SQLite** | Plugin cache, presets |
| 5B.2 | **sqlite_orm** | C++ wrapper |
| 5B.3 | **cereal** | Fast binary serialization |

### 5C: Audio Processing (optional)
| # | Library | Purpose |
|---|---------|---------|
| 5C.1 | **RubberBand** | Time-stretch |
| 5C.2 | **libsamplerate** | Resampling |
| 5C.3 | **CHOC** | Audio utilities |

### 5D: Additional Features
| # | Feature | Notes |
|---|---------|-------|
| 5D.1 | MIDI Output | Full support |
| 5D.2 | Router processor | 4-in/4-out |
| 5D.3 | Global plugin field | Persist across patches |
| 5D.4 | Document in titlebar | Show .pdl filename |

### 5E: Bug Fixes (Legacy)
| # | Issue |
|---|-------|
| 5E.1 | Connection drawing backwards |
| 5E.2 | Patch Organiser sync |
| 5E.3 | AudioRecorder MIDI trigger |
| 5E.4 | MIDI/OSC reliability |
| 5E.5 | Start minimized |
| 5E.6 | Home key mapping |

### 5F: Documentation
| # | Task | Notes |
|---|------|-------|
| 5F.1 | Set up Doxygen | Create Doxyfile, integrate with CMake |
| 5F.2 | Generate API docs | HTML output in `/docs/api/` |
| 5F.3 | Update ARCHITECTURE.md | Reflect final structure |
| 5F.4 | Add CONTRIBUTING.md | For contributors |

---

## Phase 6: Distribution & Platform

**Status:** ⏳ Future  
**Prerequisite:** Phase 5 feature-complete  
**Goal:** Production distribution

### 6A: CI/CD Pipeline
| # | Task | Tool |
|---|------|------|
| 6A.1 | GitHub Actions workflow | Automated builds |
| 6A.2 | Windows build | MSVC |
| 6A.3 | macOS build | Xcode |
| 6A.4 | Linux build | GCC/Clang |
| 6A.5 | Artifact publishing | Release binaries |
| 6A.6 | Test automation | CI test runs |

### 6B: Installers & Updates
| # | Task | Tool |
|---|------|------|
| 6B.1 | Windows installer | NSIS or WiX |
| 6B.2 | macOS DMG | create-dmg |
| 6B.3 | Auto-update (Win) | **WinSparkle** |
| 6B.4 | Auto-update (Mac) | **Sparkle** |
| 6B.5 | Code signing | Windows/macOS certs |

### 6C: Crash Reporting
| # | Task | Tool |
|---|------|------|
| 6C.1 | Crash dumps | **Crashpad** |
| 6C.2 | Cloud reporting | **Sentry** (optional) |

---

## Phase 7: Advanced & Future

**Status:** ⏳ Future  
**Prerequisite:** Phase 6 shipping  
**Goal:** Competitive feature set

### 7A: Additional Plugin Formats
| # | Format | Library |
|---|--------|---------|
| 7A.1 | **CLAP** | clap-juce-extensions |
| 7A.2 | LV2 | Linux focus |
| 7A.3 | Plugin sandboxing | Out-of-process |

### 7B: Networking
| # | Feature | Library |
|---|---------|---------|
| 7B.1 | WebSocket remote | **websocketpp** |
| 7B.2 | Preset cloud sync | **libcurl** |
| 7B.3 | Zeroconf discovery | **mdns** |

### 7C: Platform & Input
| # | Feature | Library |
|---|---------|---------|
| 7C.1 | Gamepad/joystick | **SDL2** |
| 7C.2 | Custom footswitches | **hidapi** |
| 7C.3 | Serial port input | Native |

### 7D: UI Enhancements
| # | Feature | Library |
|---|---------|---------|
| 7D.1 | GPU vector graphics | **blend2d** / **NanoVG** |
| 7D.2 | Animations | **Lottie** / **rive-cpp** |
| 7D.3 | Flexbox layout | **Yoga** |
| 7D.4 | Additional themes | Industrial, Stage, Blueprint |

### 7E: User Experience
| # | Feature | Notes |
|---|---------|-------|
| 7E.1 | Accessibility | Screen reader, keyboard nav |
| 7E.2 | Localization (i18n) | Multi-language |
| 7E.3 | Touch interface | Large hit targets |
| 7E.4 | OSC timeline | Generalized sequencing |
| 7E.5 | Mobile remote app | Phone/tablet control |

---

## Library Quick Reference

| Phase | Libraries |
|-------|-----------|
| **2** | CPM.cmake, fmt, spdlog, nlohmann/json, toml++, CLI11, Catch2 |
| **3** | libremidi, liblo |
| **4** | Tracy, Google Benchmark, GSL, clang-tidy |
| **5** | SQLite, sqlite_orm, cereal, RubberBand, libsamplerate, CHOC |
| **6** | Crashpad, Sentry, WinSparkle, Sparkle |
| **7** | CLAP, websocketpp, libcurl, SDL2, hidapi, blend2d, NanoVG |

---

## Ongoing: Code Hygiene Checklist

Apply at each phase completion:

- [ ] Doxygen comments on public APIs
- [ ] No unused `#include` statements
- [ ] Warning-free build (`/W4`)
- [ ] clang-tidy clean
- [ ] .clang-format enforced
- [ ] No dead/commented code
- [ ] All TODOs tracked or resolved
- [ ] Named constants (no magic numbers)
- [ ] Error handling complete
- [ ] Unit tests for new code
- [ ] ARCHITECTURE.md updated

---

## Document Index

| Document | Purpose |
|----------|---------|
| `PHASED_PLAN.md` | This roadmap |
| `STATE_OF_PROJECT.md` | Current state for LLMs/engineers |
| `ARCHITECTURE.md` | Codebase structure and patterns |
| `apply_juce8_fixes.ps1` | JUCE 8 migration script |
| `ToDo.txt` | Original TODOs (archived) |

---

*Last updated: 2026-01-24*
