# State of the Project

> **For LLMs and Engineers:** This document provides persistent context about
> Pedalboard3 modernization. Use [ROADMAP.md](ROADMAP.md) for the active product
> queue and this document for the current restart snapshot and retained history.

---

## Current restart snapshot — 2026-10-02

### Active ship gate

**Instant Scratch Capture** is implemented in code and has recorded automated
verification. Its remaining release gate is a real guitar/audio-interface smoke
test: confirm synchronized pre-chain `raw.wav`, post-chain `wet.wav`, and
`take.json`; then confirm patch and device changes stop capture cleanly. The
full procedure and prior build/test evidence are in
[INSTANT_SCRATCH_CAPTURE_HANDOFF_2026-06-05.md](INSTANT_SCRATCH_CAPTURE_HANDOFF_2026-06-05.md).

### Current capabilities and recent closeout

Current request (2026-10-02): substantially expand Scratch Capture while updating
the docs in parallel. The user accepted persistent take history, genuine
waveforms, in-app playback, easier reamping, capture-time rig context, and a
modest UI polish pass. The [written design](docs/superpowers/specs/2026-10-02-scratch-foundations-design.md)
is approved. The [implementation plan](docs/superpowers/plans/2026-10-02-scratch-foundations.md)
awaits review and execution-method choice; no expansion code has been implemented.
Current limitations
and the accepted direction are recorded in
[ROADMAP.md](ROADMAP.md#scratch-expansion--foundations-accepted-2026-10-02).

- The primary checkout is on the JUCE 9 migration line (`1e0b4c3`); Windows
  continues to host VST3 and internal processors.
- Tuner closeout added a quick-read 430 x 350 node and fixed persistent
  guitar-string checklist state for overlapping analysis windows. The focused
  Release result and visual capture are recorded in
  [TUNER_CLOSEOUT_HANDOFF_2026-09-16.md](TUNER_CLOSEOUT_HANDOFF_2026-09-16.md).
- Device VU meters and Scratch Capture form the implemented foundation for the
  previously proposed P0 sequence: one-click Soundcheck, followed by Starter Rigs
  and Scratch Templates. These are planned, not partially implemented. The
  accepted Scratch foundations are the current design priority ahead of adding
  those previously proposed product layers.
- A separate macOS/AU hosting worktree contains Apple-Silicon and safe
  VST3/AU-scanning work. It is not merged into this Windows checkout and still
  needs macOS integration and real-plugin validation before it can be called a
  usable AU-hosting beta.

### Working rules

- The tuner's checklist persistence is a durable behavior contract; its layout
  remains product-polish territory.
- Scratch Capture, Soundcheck, and live-signal features require manual audio
  hardware QA in addition to automated tests.
- Preserve real-time boundaries: the audio callback may not allocate, lock,
  access files, or invoke UI/message-thread work.

### Documentation map

- [ROADMAP.md](ROADMAP.md): active product direction, explicit next work, and
  acceptance criteria.
- [ARCHITECTURE.md](ARCHITECTURE.md): engine, graph, metering, and RT patterns.
- [HOWTO.md](HOWTO.md): local build, test, and hardware-QA entry points.
- [CHANGELOG.md](CHANGELOG.md): behavior-changing history.

---

## Historical snapshot — Phase 6 stability & polish (last updated 2026-02-18)

The retained material below describes the February Phase 6 checkpoint. It is
useful for background but is not the current work queue; in particular, its
JUCE 8-era scanner and test-suite status does not supersede the restart snapshot
or roadmap above.

**Status:** In Progress
**Last Updated:** 2026-02-11

### Phase 6 Progress

**Plugin Scan Protection (Complete):**
- Done: PluginBlacklist singleton with SettingsManager persistence
- Done: CrashProtection with SEH wrappers and Watchdog thread
- Done: FilterGraph integration (blocks blacklisted plugins at load)
- Done: BlacklistWindow UI for manual management (Options > Plugin Blacklist)
- Done: Out-of-process scanner (Complete)
- Pending: Timeout logic for hung scans

**Monolith Sharding (Complete):**
- Done: PedalboardProcessors.cpp split into 7 files
- Done: PedalboardProcessorEditors.cpp split into 6 files
- Done: PluginField.cpp extracted persistence
- Done: PluginComponent.cpp extracted connections

**Infrastructure Node Polish (Complete):**
- Done: JUCE 8 "MIDI Input" name mismatch fixed across codebase
- Done: MIDI Input and Virtual MIDI Input nodes sized identically
- Done: Virtual MIDI Input: removed edit/bypass/mappings buttons
- Done: OSC Input: removed spurious audio/MIDI pins
- Done: Default node positions updated to centered layout
- Done: Node positions no longer overwritten by device changes

**Canvas & Viewport (Complete):**
- Done: Fit-to-screen properly centers nodes on startup
- Done: PluginField sized with padding for smooth viewport scrolling
- Done: Ctrl+wheel zoom, plain wheel scroll
- Done: fitToScreen excludes connection objects from bounding box

### Previous Wins (Feb 2026)

- Done: **Effect Rack Feature Complete** - Phase 5Q milestone achieved
- Done: **OSC Position Persistence** - Fixed node drift on patch load
- Done: **Southwest Cable Fix** - Two-Phase Transform pattern
- Done: **Sub-Graph Connection Fix** - Clear before restore pattern

**Master Bus & Mixer (Complete - Feb 17 2026):**
- Done: Bypass persistence in patch XML (forward/backward compatible)
- Done: MasterBusProcessor insert rack (SubGraph-based, device callback level)
- Done: Master bus UI (FX button in footer, dialog window rack editor)
- Done: RT-safe hasPlugins atomic cache via ChangeListener
- Done: Gain smoothing (SmoothedValue, 50ms multiplicative ramp)
- Done: VU meter ballistics (VuMeterDsp, 300ms IEC 60268-17)
- Done: Dual metering UI (VU bar + peak hold indicator)

**DAW Mixer & Splitter (Complete - Feb 18 2026):**
- Done: DawMixerProcessor — N-strip mixer with per-strip gain/pan/mute/solo/phase
- Done: DawSplitterProcessor — 1-to-N splitter with per-strip gain/mute/solo/phase
- Done: Stereo/mono toggle per strip with dynamic channel config
- Done: Pin-to-strip alignment via PinLayout virtual
- Done: MIDI pin bottom-left placement for PedalboardProcessors
- Done: Button clipping fix (e/b/m buttons reposition on resize)
- Done: Tech debt pass (duplicate mute.store bug, comment cleanup, loop hoist)
- Done: Thread safety audit — no high-risk issues found

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
| PluginPoolManager | 10 | 130+ | ✅ Pass |
| Audio Components | 8 | 42 | ✅ Pass |
| Protection | 12 | 186 | ✅ Pass |
| MIDI Mapping | 18 | 746 | ✅ Pass |
| **Total** | **124** | **2M+** | ✅ All Pass |

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
| Master Bus Insert | ✅ Complete |
| Gain Smoothing | ✅ Complete |
| VU Metering | ✅ Complete |
| DAW Mixer | ✅ Complete |
| DAW Splitter | ✅ Complete |
| MIDI Mapping | ⚠️ Legacy (works) |
| OSC Mapping | ⚠️ Legacy (works) |

---

## Next Steps (Phase 6)

1. ~~**Patch Organiser Sync**~~ - Done (commit 02acb82)
2. ~~**Timeout Logic**~~ - Done (out-of-process scanner with timeout)
3. ~~**Premium Typography**~~ - Done (Inter font family)
4. ~~**6B: Testing**~~ - Done (MIDI mapping tests: 18 cases, 746 assertions)
5. **6A: UI Polish** - blend2d vectors, animations, hover tooltips
6. **6E: Legacy UX** - Zoom out mode, pin hover enlarge, keyboard shortcuts

---

*Last updated: 2026-02-18*
