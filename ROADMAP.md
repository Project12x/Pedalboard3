# Pedalboard3 Roadmap

A focused roadmap for current development. For historical completed phases, see [docs/archive/PHASED_PLAN_ARCHIVE.md](docs/archive/PHASED_PLAN_ARCHIVE.md).

---

## Current Status

**Phase 4 (Launch)** - COMPLETE
| # | Feature | Status |
|---|---------|--------|
| 4.1 | Undo/Redo system | ✅ Complete |
| 4.2 | Plugin blacklist + crash protection | ✅ Complete |
| 4.3 | Out-of-process scanner | ✅ Complete |
| 4.4 | Launch prep (donations, announcements) | ✅ Complete |

---

## Phase 5: Pro Features

### 5A: Pro Foundation
| # | Feature | Status |
|---|---------|--------|
| 5A.4 | Talkback Mode | Planned |
| 5A.5 | Placeholder Nodes | Planned |
| 5A.6 | Full-Screen Lyrics | Planned |

### 5A-B: Visual Nodes
| # | Feature | Status |
|---|---------|--------|
| 5A-B.1 | Oscilloscope | Planned |
| 5A-B.2 | Spectrum Analyzer | Planned |
| 5A-B.3 | Image Node | Planned |
| 5A-B.4 | Clock/Timer | Planned |
| 5A-B.5 | Lyrics Sheet | Planned |
| 5A-B.6 | 3D Visualization | Planned |

### 5B: Glitch-Free Switching
| # | Feature | Status |
|---|---------|--------|
| 5B.4 | Tail Spillover | Planned |

### 5C: MIDI Processors
| # | Feature | Status |
|---|---------|--------|
| 5C.2 | MIDI Split | Planned |
| 5C.4 | MIDI Layer Mode | Planned |

### 5H: Backing Tracks
| # | Feature | Status |
|---|---------|--------|
| 5H.1 | Streaming Audio Player | Planned |
| 5H.2 | Transport controls | Planned |
| 5H.3 | Per-song assignment | Planned |
| 5H.4 | Loop regions | Planned |

### 5I: Worship Features
| # | Feature | Status |
|---|---------|--------|
| 5I.1 | IEM Mix Routing | Planned |
| 5I.2 | Song Sections | Planned |
| 5I.3 | Countdown Timer | Planned |
| 5I.4 | Ambient Pad Generator | Planned |
| 5I.5 | Planning Center Import | Planned |
| 5I.6 | Presentation Output | Planned |
| 5I.7 | CCLI Song # field | Planned |
| 5I.8 | Ableton Link | Planned |

### 5J: NAM Loader
| # | Feature | Status |
|---|---------|--------|
| 5J.1 | NAM Model Loader | Planned |
| 5J.2 | Model browser | Planned |
| 5J.3 | ToneHunt integration | Planned |
| 5J.4 | Curated model packs | Planned |

### 5J-B: IR Loader (Remaining)
| # | Feature | Status |
|---|---------|--------|
| 5J-B.3 | Bundled IR pack | Planned |
| 5J-B.5 | Low/high cut | Planned |

### 5K: SFZ Sampler
| # | Feature | Status |
|---|---------|--------|
| 5K.1 | sfizz integration | Planned |
| 5K.2 | SFZ/SF2 loader | Planned |
| 5K.3 | Curated instrument packs | Planned |
| 5K.4 | Instrument browser | Planned |

### 5L: Virtual QWERTY Keyboard
| # | Feature | Status |
|---|---------|--------|
| 5L.1 | QWERTY to MIDI | Planned |
| 5L.2 | Octave shift | Planned |
| 5L.3 | Velocity control | Planned |
| 5L.4 | Visual keyboard | Planned |
| 5L.5 | Sustain toggle | Planned |

### 5M: First-Run Onboarding
| # | Feature | Status |
|---|---------|--------|
| 5M.1 | Instrument selector | Planned |
| 5M.2 | Audio device setup | Planned |
| 5M.3 | Preset auto-load | Planned |
| 5M.4 | Quick tutorial overlay | Planned |
| 5M.5 | "Don't show again" | Planned |
| 5M.6 | Re-run from Help menu | Planned |

### 5N: Studio Essentials Pack
| # | Effect | Status |
|---|--------|--------|
| 5N.1 | Noise Gate | Planned |
| 5N.2 | Compressor | Planned |
| 5N.3 | Parametric EQ | Planned |
| 5N.4 | Reverb | Planned |
| 5N.5 | Chorus | Planned |
| 5N.6 | Tremolo | Planned |
| 5N.7 | Phaser | Planned |

### 5O: GPL Plugin Bundle
| # | Feature | Status |
|---|---------|--------|
| 5O.1 | Bundle GPL plugins with installer | Planned |
| 5O.2 | Curated presets for bundled synths | Planned |
| 5O.3 | Ildaeil integration (LV2/VST2 bridge) | Planned |

### 5P: Plugin Format Expansion
| # | Format | Status |
|---|--------|--------|
| 5P.2 | LV2 native | Planned |
| 5P.3 | CLAP native | Deferred |
| 5P.4 | Bundle Ildaeil with installer | Planned |
| 5P.5 | Extended plugin scanner | Planned |
| 5P.6 | Wrapper description system | Planned |
| 5P.7 | Transparent load via Ildaeil | Planned |
| 5P.8 | Fallback popup | Planned |

---

## Phase 6: Stability & Polish

### 6A: Premium UI/UX Polish
| # | Task | Status |
|---|------|--------|
| 6A.4 | blend2d vectors | Planned |
| 6A.5 | Smooth animations | Planned |
| 6A.7 | Hover tooltips | Planned |
| 6A.8 | Connection highlighting | Planned |
| 6A.9 | Plugin thumbnails | Planned |
| 6A.10 | Drag & drop polish | Planned |
| 6A.11 | Keyboard shortcuts overlay | Planned |
| 6A.12 | Dark/light mode auto-switch | Planned |
| 6A.13 | Welcome screen | Planned |
| 6A.14 | Plugin search with fuzzy match | Planned |
| 6A.15 | Recent files quick access | Planned |
| 6A.17 | CPU meter redesign | Planned |
| 6A.18 | Plugin bypass visual feedback | Planned |

### 6B: Testing & Protection
| # | Task | Status |
|---|------|--------|
| 6B.2 | MIDI mapping tests | Planned |
| 6B.4 | Tracy profiler | Planned |

### 6C: Bug Fixes (Legacy)
| # | Issue | Status |
|---|-------|--------|
| 6C.2 | Patch Organiser sync | Planned |
| 6C.3 | AudioRecorder MIDI trigger | Planned |
| 6C.4 | Start minimized | Planned |
| 6C.5 | Home key mapping | Planned |
| 6C.6 | Stale CrossfadeMixer pointer on patch switch (use-after-free) | Done |
| 6C.7 | Infrastructure nodes leaking into patch XML | Done |
| 6C.8 | FIFO param dispatch safety during patch transitions | Done |
| 6C.9 | Duplicate getXml() memory leak in savePatch | Done |

### 6D: Code Refactoring
| # | Task | Status |
|---|------|--------|
| 6D.1 | Split PedalboardProcessors | Planned |
| 6D.2 | Global loadSVGFromMemory() | Planned |
| 6D.3 | Atomic cross-thread vars | Done |
| 6D.4 | RT-safe MIDI/OSC parameter dispatch (FIFO) | Done |
| 6D.5 | Remove non-RT logging from audio path | Done |
| 6D.6 | Oscilloscope/VuMeter thread-safe display | Done |

### 6E: Legacy UX (Niall's ToDo.txt)
| # | Task | Status |
|---|------|--------|
| 6E.1 | Snap to grid | Deferred |
| 6E.2 | Zoom out mode | Planned |
| 6E.3 | Plugin pins enlarge on hover | Planned |
| 6E.4 | Hotkey to bypass all | Planned |
| 6E.5 | Tempo display improvements | Planned |
| 6E.6 | Bidirectional connection dragging | Planned |

### 6F: Cross-Platform
| # | Task | Status |
|---|------|--------|
| 6F.1 | macOS build | Planned |
| 6F.2 | Linux build | Planned |

### 6G: Headless Mode
| # | Task | Status |
|---|------|--------|
| 6G.1 | --no-gui execution | Planned |
| 6G.2 | JSON/OSC control API | Planned |
| 6G.3 | Systemd service file | Planned |

---

## Phase 7: Distribution

### 7A: CI/CD
| # | Task | Status |
|---|------|--------|
| 7A.1 | GitHub Actions | Planned |
| 7A.2 | Multi-platform builds | Planned |
| 7A.3 | Test automation | Planned |

### 7B: Installers & Sales
| # | Task | Status |
|---|------|--------|
| 7B.1 | Windows installer (NSIS) | Planned |
| 7B.2 | macOS DMG | Planned |
| 7B.3 | Steam integration | Planned |
| 7B.4 | itch.io page | Planned |
| 7B.5 | Auto-update (WinSparkle) | Planned |
| 7B.6 | Code signing | Planned |

### 7C: Crash Reporting
| # | Task | Status |
|---|------|--------|
| 7C.1 | Crash dumps (Crashpad) | Planned |
| 7C.2 | Cloud reports (Sentry) | Planned |

---

## Phase 8: Advanced Features

### 8A: Networking
| # | Feature | Status |
|---|---------|--------|
| 8A.1 | WebSocket remote | Planned |
| 8A.2 | Preset cloud sync | Planned |
| 8A.3 | Zeroconf | Planned |

### 8B: Plugin Formats (Advanced)
| # | Format | Status |
|---|--------|--------|
| 8B.1 | Plugin sandboxing | Planned |
| 8B.2 | SoundFont (SF2/SFZ) | Planned |

### 8C: Future
| # | Feature | Status |
|---|---------|--------|
| 8C.1 | Accessibility | Planned |
| 8C.2 | Localization | Planned |
| 8C.3 | Touch interface | Planned |
| 8C.4 | OSC timeline | Planned |
| 8C.5 | Mobile remote | Planned |
| 8C.6 | Scripting | Planned |

### 8D: Hardware Integration
| # | Feature | Status |
|---|---------|--------|
| 8D.1 | ARM Linux build | Planned |
| 8D.2 | JACK audio backend | Planned |
| 8D.3 | GPIO control | Planned |
| 8D.4 | LCD/OLED display | Planned |
| 8D.5 | Minimal memory mode | Planned |

---

## Phase 9: Marketing & Launch

### 9A: Pre-Launch
| # | Task | Status |
|---|------|--------|
| 9A.1 | Pick ONE niche | Planned |
| 9A.2 | Landing page | Planned |
| 9A.3 | Email signup | Planned |
| 9A.4 | Discord server | Planned |
| 9A.5 | Beta tester recruitment | Planned |
| 9A.6 | Teaser posts | Planned |

### 9B: Content Creation
| # | Content | Status |
|---|---------|--------|
| 9B.1 | "5-Minute Guitar Rig" video | Planned |
| 9B.2 | Full demo/walkthrough | Planned |
| 9B.3 | Comparison video | Planned |
| 9B.4 | Preset showcase | Planned |
| 9B.5 | ToneHunt integration demo | Planned |
| 9B.6 | GIF demos | Planned |

### 9C: Launch Channels
| # | Channel | Status |
|---|---------|--------|
| 9C.1 | r/guitarpedals | Planned |
| 9C.2 | r/NAMmodeler | Planned |
| 9C.3 | r/WeAreTheMusicMakers | Planned |
| 9C.4 | r/mainstage | Planned |
| 9C.5 | KVR Forum | Planned |
| 9C.6 | Gearspace | Planned |
| 9C.7 | ToneHunt Discord | Planned |
| 9C.8 | ProductHunt | Planned |
| 9C.9 | Hacker News | Planned |

### 9D: Post-Launch
| # | Task | Status |
|---|------|--------|
| 9D.1 | Reply to all comments | Planned |
| 9D.2 | Post update changelogs | Planned |
| 9D.3 | Share user rigs/presets | Planned |
| 9D.4 | New content pack announcements | Planned |
| 9D.5 | YouTube Shorts/TikToks | Planned |
| 9D.6 | Affiliate program setup | Planned |

### 9E: Revenue Optimization
| # | Strategy | Status |
|---|----------|--------|
| 9E.1 | Upsell content packs | Planned |
| 9E.2 | Lifetime deal | Planned |
| 9E.3 | Supporter tier | Planned |
| 9E.4 | Affiliate program | Planned |
| 9E.5 | Bundle deals | Planned |

### 9G: Influencer Outreach
| # | Task | Status |
|---|------|--------|
| 9G.1 | Research target channels | Planned |
| 9G.2 | Prepare outreach bundles | Planned |
| 9G.3 | Send personalized emails | Planned |
| 9G.4 | Follow up on responses | Planned |

---

## Completed (Summary)

See [CHANGELOG.md](CHANGELOG.md) for details.

- **Phase 1**: JUCE 8 migration
- **Phase 2**: Build system, logging, testing framework
- **Phase 3**: VST3 hosting, themes, settings
- **Phase 4**: Undo/redo, blacklist UI, out-of-process scanner

---

*Last updated: 2026-02-11*
