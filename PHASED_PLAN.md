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

**Goal:** Launch Pro Edition — Premium UX + killer features

> **Release:** v3.1 Pro — Steam ($15), itch.io ($15), Gumroad ($25)
> **Positioning:** "The Live Rig for Musicians" — Guitarists, Keyboardists, AND Vocalists

### 5A: Pro Foundation (Week 1-2)
| # | Feature | Effort | Status |
|---|---------|--------|--------|
| 5A.1 | **MIDI Learn** | 1 day | ✅ Done |
| 5A.2 | **Notes Node** | 1-2 days | ✅ Done |
| 5A.3 | **Label Node** | 1 day | ✅ Done |
| 5A.4 | **Talkback Mode** | 1 day | ⏳ Planned |
| 5A.5 | **Placeholder Nodes** | 2-3 days | ⏳ Planned |
| 5A.6 | **Full-Screen Lyrics** | 1-2 days | ⏳ Planned |

### 5A-B: Visual Nodes 📊 (Canvas Enhancement)

> **Target:** Visualization and annotation features for premium UX
> **Why:** Differentiates from competitors, enhances live performance workflow

#### Audio Visualizers (pass-through audio)
| # | Feature | Effort | Status | Notes |
|---|---------|--------|--------|-------|
| 5A-B.1 | **Oscilloscope** | 2-3 days | ⏳ Planned | Real-time waveform, stereo overlay, trigger modes |
| 5A-B.2 | **Spectrum Analyzer** | 3-4 days | ⏳ Planned | FFT-based frequency bars/curve, EQ visualization |

#### Purely Visual Nodes (no audio I/O)
| # | Feature | Effort | Status | Notes |
|---|---------|--------|--------|-------|
| 5A-B.3 | **Image Node** | 1-2 days | ⏳ Planned | Display custom logos/images on canvas |
| 5A-B.4 | **Clock/Timer** | 1-2 days | ⏳ Planned | Current time, setlist timer, countdown |
| 5A-B.5 | **Lyrics Sheet** | 3-5 days | ⏳ Planned | Color-coded lyrics, full-screen stage mode, markdown support |
| 5A-B.6 | **3D Visualization** | 5+ days | ⏳ Planned | OpenGL-based audio reactive 3D visuals |

**Implementation Pattern (established):**
- `*Processor`: Inherits PedalboardProcessor, implements `getControls()`, set `numInputChannels=0, numOutputChannels=0` for visual-only
- `*Control`: Inherits Component, renders UI
- Skip `BypassableInstance` wrapping in FilterGraph.cpp
- Add to `PluginComponent.cpp` button skip lists

### 5B: Glitch-Free Switching (Week 3-4) — THE Killer Feature
| # | Feature | Status | Notes |
|---|---------|--------|-------|
| 5B.1 | **Hybrid Plugin Pool** | ✅ Done | PluginPoolManager implemented |
| 5B.2 | **Predictive Loading** | ✅ Done | Part of PluginPoolManager |
| 5B.3 | **Instant Switch** | ✅ Done | Reroute + recall |
| 5B.4 | **Tail Spillover** | ⏳ Planned | Old patch's reverb/delays continue |
| 5B.5 | **Crossfade Fallback** | ✅ Done | CrossfadeMixer implemented |

> **Architecture:** Matches Gig Performer ($169). Sliding window plugin pool scales to 100+ patches.

### 5C: MIDI Processors (Week 5)
| # | Feature | Status | Audience |
|---|---------|--------|----------|
| 5C.1 | **MIDI Transpose** | ✅ Done | Keyboardists |
| 5C.2 | **MIDI Split** | ⏳ Planned | Upper/lower zones |
| 5C.3 | **MIDI Rechannelize** | ✅ Done | Route to different channels |
| 5C.4 | **MIDI Layer Mode** | ⏳ Planned | Stack multiple sounds |

### 5D: Pro UX Polish 🎨
| # | Feature | Priority | Notes |
|---|---------|----------|-------|
| 5D.1 | **Plugin node redesign** | ✅ Done | Rounded corners, shadows, glow |
| 5D.2 | **Curved cable routing** | ✅ Done | Bezier curves, color-coded |
| 5D.3 | **Cable signal animation** | ✅ Done | Glow when audio flows |
| 5D.4 | **Inter font** | ✅ Done | Modern, readable |
| 5D.5 | **Lucide icons** | ✅ Done | Consistent iconography |
| 5D.6 | **Canvas zoom/pan** | ✅ Done | Scroll + Fit to Screen |
| 5D.7 | **Melatonin blur** | ✅ Done | Glassmorphism effects |
| 5D.8 | **Toast notifications** | ✅ Done | "Undo: Removed Reverb" |
| 5D.9 | **Safety Limiter** | ✅ Done | Protect ears from feedback |

### 5E: Setlist & Performance
| # | Feature | From |
|---|---------|------|
| 5E.1 | **Setlist management** | ✅ Already exists |
| 5E.2 | **Predictive loading** | Part of 5B |
| 5E.3 | **Stage Mode** | ✅ Already exists |

### 5F: Advanced Routing
| # | Feature | Notes |
|---|---------|-------|
| 5F.1 | **A/B splitter node** | ✅ Already exists |
| 5F.2 | **Mixer node** | ✅ Already exists |
| 5F.3 | **Wet/dry mix** | Built into routing |

### 5G: Built-In Tools (Already Done!)
| # | Feature | Status |
|---|---------|--------|
| 5G.1 | **Chromatic Tuner** | ✅ Done |
| 5G.2 | **Looper** | ✅ Done |
| 5G.3 | **Audio Recorder** | ✅ Done |
| 5G.4 | **Metronome** | ✅ Done |
| 5G.5 | **Tone Generator** | ✅ Done |

### 5H: Backing Tracks (v1.1)
| # | Feature | Notes |
|---|---------|-------|
| 5H.1 | **Streaming Audio Player** | MP3/WAV/FLAC from disk |
| 5H.2 | **Transport controls** | Play/pause/seek |
| 5H.3 | **Per-song assignment** | Sync with setlist |
| 5H.4 | **Loop regions** | Section practice |

### 5I: Worship Features ⛪ (v1.2 - New Market Segment)

> **Target:** Churches, worship teams, volunteer musicians
> **Why:** Underserved market with budget, strong word-of-mouth, MainStage refugees on Windows

| # | Feature | Effort | Notes |
|---|---------|--------|-------|
| 5I.1 | **IEM Mix Routing** | 1 day | Click to one output, music to another |
| 5I.2 | **Song Sections** | 2 days | Verse/Chorus/Bridge navigation buttons |
| 5I.3 | **Countdown Timer** | 1 day | "30 seconds to service start" |
| 5I.4 | **Ambient Pad Generator** | 3-5 days | Built-in, replaces $50+ plugin |
| 5I.5 | **Planning Center Import** | 2-3 days | Auto-import setlist from their service |
| 5I.6 | **Presentation Output** | 3 days | Lyrics to projector/secondary display |
| 5I.7 | **CCLI Song # field** | 1 hour | Licensing compliance metadata |
| 5I.8 | **Ableton Link** | 2-3 days | Sync tempo with keys player |

**Total worship features:** ~2 weeks of development

**Marketing channels:**
- Worship Tutorials (500K+ YouTube)
- Sunday Sounds (100K+)
- Loop Community (50K+)
- That Worship Sound (100K+)
- Church tech Facebook groups

---

### 5J: NAM Loader 🎸 (Guitarist Game-Changer)

> **Target:** Guitarists who want amp/cab/pedal captures without external plugins
> **Why:** NAM is huge right now, ToneHunt has 10,000+ free models, removes friction

| # | Feature | Effort | Notes |
|---|---------|--------|-------|
| 5J.1 | **NAM Model Loader** | 1-2 days | Use NeuralAmpModelerCore (GPL compatible) |
| 5J.2 | **Model browser** | 1 day | Load .nam files from disk |
| 5J.3 | **ToneHunt integration** | 2-3 days | Browse/download models in-app |
| 5J.4 | **Curated model packs** | Ongoing | Holdsworth, Worship, JHS-style tones |

---

### 5J-B: IR Loader 🔊 (Cabinet Simulation) ✅ COMPLETE

> **Target:** Complete the amp chain — NAM = amp, IR = cabinet
> **Why:** Every guitarist expects cab sim. Without it, NAM sounds harsh/fizzy.

| # | Feature | Status | Notes |
|---|---------|--------|-------|
| 5J-B.1 | **IR Loader node** | ✅ Done | IRLoaderProcessor implemented |
| 5J-B.2 | **IR browser** | ✅ Done | IRLoaderControl implemented |
| 5J-B.3 | **Bundled IR pack** | ⏳ Planned | 10-20 essential cabs |
| 5J-B.4 | **Mix control** | ✅ Done | Built into IRLoaderProcessor |
| 5J-B.5 | **Low/high cut** | ⏳ Planned | Shape the IR response |

**Free IR Sources:**
- Ownhammer free pack
- Redwirez free samples  
- ML Sound Lab free cabs
- 3 Sigma Audio freebies

---

**The "Zero External Plugins" Guitar Rig:**
```
Input → Tuner → NAM Amp → IR Cab → UDS Delay → Looper → Output
```
All built-in. No plugins needed. $29.

---

### 5K: SFZ Sampler 🎹 (MainStage Parity)

> **Target:** Keyboardists who need piano/organ/strings without buying plugins
> **Why:** Closes the biggest gap vs MainStage - built-in instruments

| # | Feature | Effort | Notes |
|---|---------|--------|-------|
| 5K.1 | **sfizz integration** | 2-3 days | BSD license, high-quality SFZ player |
| 5K.2 | **SFZ/SF2 loader** | 1 day | Load any SoundFont |
| 5K.3 | **Curated instrument packs** | Ongoing | Piano, organ, strings, pads |
| 5K.4 | **Instrument browser** | 1 day | Browse loaded SFZ files |

**Free SFZ Libraries Available:**
- Salamander Grand Piano (free, excellent)
- Virtual Playing Orchestra (free)
- VSCO Community Orchestra (free)
- Iowa Piano (public domain)

**The "Zero Plugins" Keyboardist Rig:**
```
MIDI In → Split/Layer → SFZ Piano + SFZ Strings → Output
```
All built-in. No Kontakt needed.

---

### 5L: Virtual QWERTY Keyboard ⌨️ (Steam-Critical)

> **Target:** Users with no hardware — just a laptop
> **Why:** Steam users may not have guitar, interface, or MIDI keyboard. Must make sound immediately.

| # | Feature | Effort | Notes |
|---|---------|--------|-------|
| 5L.1 | **QWERTY → MIDI** | 1-2 days | ASDFGHJKL = notes, like GarageBand |
| 5L.2 | **Octave shift** | 1 hour | Z/X or Shift modifier |
| 5L.3 | **Velocity control** | 1 hour | Based on key hold or fixed |
| 5L.4 | **Visual keyboard** | 1 day | On-screen piano showing pressed keys |
| 5L.5 | **Sustain toggle** | 30 min | Space bar or dedicated key |

**Zero-Hardware Experience:**
```
Open App → Press ASDF → Hear Piano
```
No guitar. No interface. No MIDI controller. Laptop speakers only.

---

### 5M: First-Run Onboarding 🎯 (Steam-Critical)

> **Target:** All new users, especially Steam audience
> **Why:** Empty canvas = bad reviews. Guided setup = instant success.

| # | Feature | Effort | Notes |
|---|---------|--------|-------|
| 5M.1 | **Instrument selector** | 2 days | Guitar / Keys / Vocals / Virtual |
| 5M.2 | **Audio device setup** | 1 day | Auto-detect or simple picker |
| 5M.3 | **Preset auto-load** | 1 hour | Full working chain per choice |
| 5M.4 | **Quick tutorial overlay** | 1-2 days | "This is your signal chain..." |
| 5M.5 | **"Don't show again"** | 30 min | Settings flag |
| 5M.6 | **Re-run from Help menu** | 30 min | "Setup Wizard..." |

**Onboarding Flow:**
```
┌─────────────────────────────────────────────┐
│   Welcome to Pedalboard3!                   │
│                                             │
│   What are you playing?                     │
│                                             │
│   🎸  Guitar                                │
│   🎹  Keys / Synth                          │
│   🎤  Vocals                                │
│   ⌨️  Just exploring (virtual keyboard)    │
└─────────────────────────────────────────────┘
```

**Auto-Loaded Presets:**

| Choice | Preset Chain |
|--------|--------------|
| **Guitar** | Input → Tuner → NAM Clean → Delay → Reverb → Looper → Output |
| **Keys** | MIDI In → SFZ Piano → Reverb → Output |
| **Vocals** | Mic In → EQ → Comp → Reverb → Talkback → Output |
| **Virtual** | QWERTY → Built-in Synth → Reverb → Output |

---

### 5N: Studio Essentials Pack 🎛️ (Steam-Critical)

> **Target:** Complete rig without external VST plugins
> **Why:** Steam users have NO plugins. Without built-in effects, NAM sounds unfinished.

**Current State:** Tools exist (tuner, looper, recorder) but NO audio processing effects.

#### Core Effects (Must-Have for Steam)

| # | Effect | JUCE DSP | Effort | Priority |
|---|--------|----------|--------|----------|
| 5N.1 | **Noise Gate** | `juce::dsp::NoiseGate` | 1 day | 🔴 Critical |
| 5N.2 | **Compressor** | `juce::dsp::Compressor` | 1 day | 🔴 Critical |
| 5N.3 | **Parametric EQ** | `juce::dsp::IIR::Filter` | 1-2 days | 🔴 Critical |
| 5N.4 | **Reverb** | `juce::Reverb` | 1 day | 🔴 Critical |
| 5N.5 | **Chorus** | LFO + delay line | 1-2 days | 🟡 High |
| 5N.6 | **Tremolo** | LFO × gain | 0.5 day | 🟢 Medium |
| 5N.7 | **Phaser** | AllPass chain + LFO | 1-2 days | 🟢 Medium |

#### Effect Specifications

**Noise Gate:**
- Threshold, Attack, Release, Range
- Side-chain filter option

**Compressor:**
- Threshold, Ratio, Attack, Release, Makeup Gain
- Knee control (soft/hard)
- Meter showing gain reduction

**Parametric EQ:**
- 4-band parametric (Low, Low-Mid, High-Mid, High)
- Q/bandwidth per band
- Bypass per band
- Simple visual curve

**Reverb:**
- Room Size, Damping, Width, Mix
- Pre-delay optional
- Plate/Hall/Room presets

**Total effort:** ~1-2 weeks for Core (Gate, Comp, EQ, Reverb)

---

### Steam-Ready Checklist ✅

> **Steam audience ≠ plugin enthusiasts.** They need batteries included.

| Requirement | Section | Status |
|-------------|---------|--------|
| NAM integration | 5J | Planned |
| IR Loader | 5J-B | ✅ Done |
| SFZ sampler | 5K | Planned |
| QWERTY keyboard | 5L | **NEW** |
| Onboarding wizard | 5M | **NEW** |
| **Studio Essentials** | 5N | **NEW** |
| **GPL Plugin Bundle** | 5O | **NEW** |
| Bundled NAM models | 5J.4 | Planned |
| Bundled IRs | 5J-B.3 | **NEW** |
| Bundled SFZ piano | 5K.3 | Planned |
| Default presets (4) | 5M.3 | **NEW** |


**Platform Launch Strategy:**

| Platform | Audience | When |
|----------|----------|------|
| Gumroad/itch/KVR | Plugin enthusiasts | Now |
| **Steam** | Mainstream users | After 5J-5N complete |

---

### 5O. GPL Leverage Strategy 🔓

> **GPL is a strategic advantage, not a limitation.** Since Pedalboard3 is GPLv3, we can bundle other GPL software and take code from GPL projects.

#### Bundleable GPL Plugins (Ship with Installer)

**Major Instruments:**

| Plugin | Category | License | Value Add |
|--------|----------|---------|-----------|
| **Cardinal** | Modular Synth | GPL-3.0 | Complete VCV Rack as a plugin |
| **Surge XT** | Synth | GPL-3.0 | World-class wavetable/subtractive synth |
| **Vital** | Wavetable Synth | GPL-3.0 | Serum competitor |
| **Dexed** | FM Synth | GPL-3.0 | Yamaha DX7 emulator |
| **Odin 2** | Synth | GPL-3.0 | Modern hybrid synth |
| **OB-Xd** | Analog Synth | GPL-3.0 | Oberheim emulator |
| **Helm** | Synth | GPL-3.0 | Polyphonic synth |
| **DrumGizmo** | Drums | GPL-3.0 | Acoustic drum sampler |
| **sfizz** | Sampler | BSD-2 | SFZ player (can also bundle) |

**DISTRHO Ecosystem (falkTX):**

| Plugin | Category | License | Value Add |
|--------|----------|---------|-----------|
| **Ildaeil** | Plugin Host | ISC | Mini plugin host *as a plugin* — runs LV2/VST2/VST3/CLAP/LADSPA inside! |
| **MVerb** | Reverb | GPL-3.0 | Studio-quality Dattorro reverb (VST3/LV2/CLAP) |
| **Mini-Series** | Utilities | GPL-3.0 | 3-Band EQ, 3-Band Splitter, Ping Pong Pan |
| **FluidPlug** | SoundFonts | GPL-2.0 | SoundFonts as LV2 plugins via FluidSynth |
| **Wolf Shaper** | Distortion | GPL-3.0 | Waveshaper with spline graph editor (LV2/VST/CLAP) |

**Bundled Plugin Strategy:**
- Create installer option: "All-In-One Studio" (includes above plugins)
- Separate download to keep base installer small
- Include curated presets showcasing each synth
- **Ildaeil** is particularly strategic — users can load LV2/LADSPA plugins inside Pedalboard3!

#### GPL Code Borrowing Opportunities

**Host Infrastructure (Steal the Hard Stuff):**

| Project | License | What to Borrow |
|---------|---------|----------------|
| **Carla** | GPL-2.0+ | LV2 hosting, LADSPA hosting, JACK transport, plugin discovery, patchbay logic |
| **Element** | GPL-3.0 | Node graph logic, session persistence, MIDI routing |
| **Ardour** | GPL-2.0 | Transport system, mixer routing, session management |
| **Audacity** | GPL-2.0 | Audio utilities, effects algorithms, file format handling |
| **LMMS** | GPL-2.0 | Instrument hosting, MIDI processing, automation |
| **Hydrogen** | GPL-2.0 | Drum machine patterns, swing/humanize algorithms |

**Requirement:** Maintain GPL notices, provide source to recipients.

---

#### Additional Bundleable GPL Plugins

**Guitar/Bass Ecosystem (Via Ildaeil):**

| Plugin | Category | License | Value Add |
|--------|----------|---------|-----------|
| **AIDA-X** | Neural Amp | GPL-3.0 | AI amp modeler (like NAM) with community models |
| **Guitarix LV2** | Amp Suite | GPL-2.0 | 50+ amp sims, 100+ pedal effects as LV2 |
| **gxplugins.lv2** | Pedals | GPL-2.0 | Individual LV2 versions of Guitarix effects |
| **Tamgamp.lv2** | Amp Sim | GPL-3.0 | Fender, Mesa, Marshall, VOX amp models |
| **XPlugs.lv2** | Amps/FX | GPL-3.0 | FatFrog, XDarkTerror, CollisionDrive |
| **MetalTone.lv2** | Distortion | GPL-3.0 | Metal distortion with advanced EQ |

**Pro-Quality Effects Suites:**

| Plugin | Category | License | Value Add |
|--------|----------|---------|-----------|
| **LSP Plugins** | Everything | GPL-3.0 | 100+ plugins: compressors, EQs, gates, limiters, chorus, flanger, etc. |
| **x42 Meters** | Metering | GPL-2.0+ | VU, K-system, EBU R128, goniometer, spectrum analyzer |
| **x42 LV2** | Utils | GPL-2.0+ | Delay compensator, test tone, tuner, midifilter |
| **Dragonfly Reverb** | Reverb | GPL-3.0 | Hall, Room, Plate, Early — rivals Valhalla |
| **ZamAudio** | Dynamics | GPL-2.0 | Compressors, limiters, EQ, saturation |
| **Calf Studio Gear** | Suite | LGPL-2.1 | Vintage Delay, Reverb, Filters, Modulation, Limiter |

**ChowDSP (Premium Quality GPL-3.0):**

| Plugin | Category | Description |
|--------|----------|-------------|
| **ChowTapeModel** | Tape Sat | Analog tape emulation with wow/flutter — Sony TC-260 |
| **ChowMatrix** | Delay | Infinite matrix delay with feedback routing |
| **BYOD** | Modular | Build-Your-Own-Distortion with chorus, spring reverb |

**More Synthesizers:**

| Plugin | Category | License | Value Add |
|--------|----------|---------|-----------|
| **ZynAddSubFX** | Synth | GPL-2.0 | Legendary 3-engine synth (Add/Sub/Pad), 1100+ presets |
| **Yoshimi** | Synth | GPL-2.0 | ZynAddSubFX fork with vector control, LV2 |
| **Nekobi** | Bass Synth | GPL-3.0 | DISTRHO TB-303 clone |
| **Kars** | Karplus-Strong | GPL-3.0 | String synthesis |

**MIDI Tools:**

| Plugin | Category | License | Value Add |
|--------|----------|---------|-----------|
| **Arpligner** | Arpeggiator | GPL-3.0 | Multi-track polyphonic arpeggiator (VST3/LV2) |

---

#### All-In-One Studio Concept

**Steam pitch:** "Complete music studio for $15. Zero plugins to buy."

| Category | Bundled Content | Count |
|----------|-----------------|-------|
| **Synths** | Surge XT, Vital, Dexed, Helm, Odin 2, OB-Xd, ZynAddSubFX | 7 |
| **Modular** | Cardinal (VCV Rack) | 1 |
| **Amp Sims** | NAM Loader + bundled models, AIDA-X, Guitarix | 3+ |
| **Cab Sims** | IR Loader + bundled IRs | 1 |
| **Effects** | Built-in (Gate, Comp, EQ, Reverb) + LSP + Dragonfly + ChowDSP | 100+ |
| **Metering** | x42 Meters (VU, K-system, EBU R128, Goniometer) | 1 suite |
| **Sampler** | sfizz + bundled piano | 1 |
| **Looper** | LoopDeLoop integration | 1 |
| **Meta** | Ildaeil (loads LV2/LADSPA/VST2 inside!) | 1 |

**Total Bundleable Plugins: 150+**

---

#### Competitive Position

| Feature | Pedalboard3 | MOD Desktop | MainStage | Gig Performer |
|---------|-------------|-------------|-----------|---------------|
| **Price** | $15 | Free | $30 | $149 |
| **VST3 Support** | ✅ | ❌ | ✅ | ✅ |
| **LV2 Support** | Via Ildaeil | ✅ Native | ❌ | ❌ |
| **Bundled Synths** | 7+ | 0 | 40+ (loops) | 0 |
| **Bundled Amps** | NAM + AIDA-X | ❌ | ✅ (Amp Designer) | ❌ |
| **Modular** | Cardinal | ✅ | ❌ | ❌ |
| **Pro Metering** | x42 | ❌ | Basic | ❌ |
| **Node Graph** | ✅ | ✅ | ❌ | ✅ |
| **Open Source** | GPL-3.0 | AGPL | ❌ | ❌ |

**Why We Win:**
1. **Price:** $15 vs $149 (Gig Performer) — 90% cheaper
2. **Plugin Ecosystem:** VST3 native + LV2 via Ildaeil = access to everything
3. **Bundled Content:** 150+ plugins vs 0 plugins for most competitors
4. **Open Source:** Users can verify, modify, trust
5. **Self-Contained:** Works without internet, no subscriptions
6. **Guitar Focus:** NAM + AIDA-X + Guitarix = best-in-class amp modeling


- MainStage: Mac only, $30
- Gig Performer: $169, no bundled instruments
- **Pedalboard3: $15, everything included, cross-platform**

#### GPL Compliance Checklist

| Requirement | Implementation |
|-------------|----------------|
| Source availability | GitHub repo link in About dialog |
| License files | COPYING.txt in installer, LICENSE folder for each bundled plugin |
| Attribution | Credits page in app listing all GPL components |
| No DRM | Steam build has no copy protection (GPL compliant) |

---

### 5P: Plugin Format Expansion 🔌 (Universal Compatibility)

> **Goal:** Support ALL plugin formats — native where possible, wrapped for legacy.
> **Strategy:** Native LV2/CLAP + Ildaeil auto-wrapping for VST2/32-bit/LADSPA

#### Native Format Support

| # | Format | Source | Status |
|---|--------|--------|--------|
| 5P.1 | **VST3 64-bit** | JUCE native | ✅ Done |
| 5P.2 | **LV2 native** | Element patterns + Lilv | ⏳ Planned (2-3 weeks) |
| 5P.3 | **CLAP native** | JUCE 9 / juce_clap_hosting | ⏸️ Deferred |

**LV2 Implementation:**
- Adapt `LV2PluginFormat.cpp` patterns from Kushview Element (GPL-3.0)
- Direct Lilv integration for plugin discovery/instantiation (ISC license)
- Suil for UI embedding (ISC license)
- Full control, no Element code debt

#### Ildaeil Auto-Wrapping (Legacy Format Bridge)

> **Bundled Ildaeil = instant access to VST2, 32-bit, LADSPA** without native implementation

| # | Feature | Effort | Notes |
|---|---------|--------|-------|
| 5P.4 | **Bundle Ildaeil with installer** | 1 day | ISC license, uses Carla engine |
| 5P.5 | **Extended plugin scanner** | 2-3 days | Detect VST2/32-bit/LADSPA formats |
| 5P.6 | **Wrapper description system** | 2-3 days | Create virtual entries for legacy plugins |
| 5P.7 | **Transparent load via Ildaeil** | 2-3 days | Load Ildaeil + inject target plugin path |
| 5P.8 | **Fallback popup** | 1 day | "Load via Ildaeil?" if transparent fails |

**Auto-Wrap Flow:**
```
Plugin Scan
    │
    ├── VST3/LV2/CLAP → Add to native list
    │
    └── VST2/32-bit/LADSPA → Create Ildaeil wrapper entry
                              │
                              ▼
                         Plugin appears in list like any other
                         When loaded → P3 loads Ildaeil + injects target path
                         User sees plugin UI transparently
```

**Result:**

| Format | Native | Via Ildaeil | User Sees |
|--------|--------|-------------|-----------|
| VST3 64-bit | ✅ | — | Plugin |
| LV2 | ✅ | — | Plugin |
| CLAP | ✅ | ✅ (fallback) | Plugin |
| VST2 | ❌ | ✅ Auto-wrap | Plugin |
| 32-bit (any) | ❌ | ✅ Auto-wrap | Plugin |
| LADSPA | ❌ | ✅ Auto-wrap | Plugin |

**Total Effort:** ~1-2 weeks for transparent flow

**Why This Wins:**
- Zero friction for legacy plugin users
- 32-bit plugins work without separate bridging code
- VST2 access without Steinberg SDK licensing headaches
- Users don't know/care about the wrapping — it just works

### Pro Target Audiences

| Audience | Key Features |
|----------|-------------|
| **Guitarists** | NAM Loader, Looper, Tuner, UDS, Placeholder templates |
| **Keyboardists** | SFZ Sampler, MIDI Split, Transpose, Layer mode, Rechannelize |
| **Vocalists** | Talkback, Full-Screen Lyrics, Backing tracks, Click track |
| **Worship Teams** | IEM routing, Song sections, Ambient pads, Planning Center |
| **Beginners (Steam)** | Virtual Keyboard, Onboarding Wizard, Zero-config presets |

---

### Pro vs Competition

| Feature | Pedalboard3 | Gig Performer ($169) | MainStage ($30) |
|---------|-------------|---------------------|-----------------|
| Instant Switching | ✅ Hybrid Pool | ✅ | ⚠️ Basic |
| MIDI Learn | ✅ | ✅ | ✅ |
| Built-in Tuner | ✅ | ❌ (+$20 plugin) | ✅ |
| Built-in Looper | ✅ | ❌ (+$50 plugin) | ❌ |
| **Built-in NAM Loader** | ✅ | ❌ | ❌ |
| **Built-in Sampler** | ✅ SFZ | ❌ | ✅ EXS24 |
| **Virtual QWERTY Keyboard** | ✅ | ❌ | ❌ |
| **First-Run Onboarding** | ✅ | ❌ | ⚠️ Basic |
| Notes/Lyrics on Canvas | ✅ | ❌ | ❌ |
| Talkback Mode | ✅ | ❌ | ❌ |
| Ambient Pad Generator | ✅ | ❌ | ❌ |
| Planning Center Import | ✅ | ❌ | ❌ |
| Windows Support | ✅ | ✅ | ❌ Mac only |
| Open Source | ✅ GPL | ❌ | ❌ |

| **Price** | **$29** | **$169** | **$30** |

---

### 5-Week MVP Build Order

| Week | Features | Deliverable |
|------|----------|-------------|
| 1 | MIDI Learn + Notes Node + Talkback | Core workflow + unique features |
| 2 | Placeholder Nodes + Full-Screen Lyrics | Templates + vocalist tools |
| 3-4 | Hybrid Plugin Pool | Instant switching (GP-level) |
| 5 | MIDI Processors + NAM Loader | Split + Transpose + Amp modeling |

> **After MVP:** Ship v3.1 Pro, then add Backing Tracks + Worship Features in v3.2.


---

### 5Q: Effect Rack (SubGraph) 🧱 ✅ COMPLETE

> **Target:** Power users who need complex routing within a single node
> **Status:** Functional — Core editor stabilized

| # | Feature | Status | Notes |
|---|---------|--------|-------|
| 5Q.1 | **SubGraphProcessor** | ✅ Done | Nested AudioProcessorGraph within a node |
| 5Q.2 | **SubGraphEditorComponent** | ✅ Done | Viewport + canvas editor |
| 5Q.3 | **Editor re-entry** | ✅ Done | Fresh editor on each open (createEditor pattern) |
| 5Q.4 | **Pin bounds checking** | ✅ Done | Prevent out-of-range access in rebuildGraph |
| 5Q.5 | **Internal plugin state restore** | ✅ Done | ToneGenerator reads from processor |
| 5Q.6 | **Cable wiring** | ✅ Done | Pin-to-pin connections in SubGraph |
| 5Q.7 | **Delete nodes/cables** | ✅ Done | Keyboard shortcuts + menu actions |

**Key Files:**
- `src/SubGraphProcessor.cpp/h`
- `src/SubGraphEditorComponent.cpp/h`
- `src/ToneGeneratorControl.cpp`

---

## Phase 6: Stability & Polish

**Goal:** Production-ready quality with premium UI/UX

> **Competitive Edge:** Carla and Element are powerful but have developer-focused UIs.
> Pedalboard3's differentiator is an **intuitive, polished GUI** that feels premium.

### 6A: Premium UI/UX Polish 🎨
| # | Task | Notes |
|---|------|-------|
| 6A.1 | **Inter font** | Modern, readable UI font (SIL OFL) |
| 6A.2 | **JetBrains Mono** | Monospace for values/logs (SIL OFL) |
| 6A.3 | **Lucide icons** | Consistent modern iconography (ISC) |
| 6A.4 | **blend2d vectors** | Smooth vector graphics (zlib) |
| 6A.5 | **Smooth animations** | Connection drawing, plugin add/remove |
| 6A.6 | **Cable glow effects** | Audio flowing = subtle pulse/glow |
| 6A.7 | **Hover tooltips** | Descriptive tooltips on all controls |
| 6A.8 | **Connection highlighting** | Highlight path on hover |
| 6A.9 | **Plugin thumbnails** | Visual preview in plugin list |
| 6A.10 | **Drag & drop polish** | Smooth ghost preview when dragging |
| 6A.11 | **Keyboard shortcuts overlay** | Press `?` to show all shortcuts |
| 6A.12 | **Dark/light mode auto-switch** | Match OS preference |
| 6A.13 | **Welcome screen** | First-run tutorial/quick start |
| 6A.14 | **Plugin search with fuzzy match** | Type-to-filter with smart matching |
| 6A.15 | **Recent files quick access** | File > Recent with previews |
| 6A.16 | **Undo/redo toast notifications** | "Removed Reverb" with undo link |
| 6A.17 | **CPU meter redesign** | Animated, gradient bar |
| 6A.18 | **Plugin bypass visual feedback** | Dimmed + strikethrough when bypassed |

### 6B: Testing
| # | Task | Notes |
|---|------|-------|
| 6B.1 | FilterGraph tests | Unit tests |
| 6B.2 | MIDI mapping tests | CC assignment |
| 6B.3 | Plugin loading tests | VST3/CLAP |
| 6B.4 | Tracy profiler | Performance |
| 6B.5 | **Plugin load crash protection** | Try-catch + error dialog |
| 6B.6 | **Incompatible plugin detection** | Scan on startup, warn user |

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
| 6E.6 | Bidirectional connection dragging (input→output) |

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

### 8B: Plugin Formats (Advanced)
> **Note:** LV2 native + 32-bit/VST2/LADSPA bridging moved to **5P: Plugin Format Expansion** (via Ildaeil)

| # | Format | Notes |
|---|--------|-------|
| 8B.1 | Plugin sandboxing | Crash isolation for problematic plugins |
| 8B.2 | SoundFont (SF2/SFZ) | Built-in instruments (sfizz) |

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
| 5 | SDL2, hidapi, libserialport, liblo, SQLite, NeuralAmpModelerCore |
| 6 | Inter, JetBrains Mono, Lucide, blend2d, Tracy |
| 7 | WinSparkle, Crashpad, Sentry, Steamworks |
| 8 | websocketpp, libcurl, mdns |

---

## Distribution & Licensing Strategy 💰

### The Aseprite Model

> **Source is public (GPL compliant), binaries are sold.**
> Most users can't/won't compile. They'll pay for convenience.

| Tier | What They Get | Price |
|------|---------------|-------|
| **Free** | Modernized Pedalboard2 base binary | $0 |
| **Source** | Full Pedalboard3 source (GPL) | $0 (build it yourself) |
| **Pro** | Source access + presets + NAM models + support | $29 |

### Product Licensing

| Product | License | Reason |
|---------|---------|--------|
| **Pedalboard3** | GPL v3 | Inherited from Pedalboard2 (viral) |
| **LoopDeLoop** | GPL (JUCE free tier) | Until JUCE license paid |
| **Phantasm** | GPL | Possible GPL dependencies |
| **SporeEngine** | **Your choice** | MIT deps + your code only |
| **UDS** | **Your choice** | MIT deps + your code only |

> **Note:** SporeEngine and UDS can remain proprietary - valuable flexibility.

### Bundle Offerings

| Bundle | Contents | Individual Value | Bundle Price |
|--------|----------|------------------|--------------|
| **Essential** | Pedalboard3 + LoopDeLoop | $58 | $39 |
| **Producer** | Essential + UDS + Phantasm | $136 | $79 |
| **Complete** | All 5 products | $185 | $99 |
| **Holdsworth Pack** | UDS + delay presets only | $59 | $39 |

### Content Packs (Recurring Revenue)

| Pack | Contents | Price |
|------|----------|-------|
| **NAM Starter** | 20 curated amp/cab models | $9 |
| **Worship Tones** | Clean + ambient NAM models | $9 |
| **Holdsworth UDS Presets** | His actual delay settings | $9 |
| **Complete Content Bundle** | All packs | $29 |

### Sales Channels

| Platform | Cut | Best For |
|----------|-----|----------|
| **Gumroad** | 10% | Direct sales, best margins |
| **Steam** | 30% | SporeEngine (gamers love weird synths) |
| **Plugin Boutique** | 30% | Serious audio buyers |
| **itch.io** | You choose | Experimental/indie audience |
| **KVR Marketplace** | Free | Discovery, links to Gumroad |

### The 94% Rule

| User Type | Behavior | % |
|-----------|----------|---|
| Can compile C++ audio code | Builds from source | ~1% |
| Tech-savvy but busy | "I could, but $29 is easier" | ~5% |
| Normal musicians | Click buy | **~94%** |

**You're selling to the 94%.** The source being public is compliance, not competition.

---

## Phase 9: Marketing & Launch 📣 (The $3K+ Gap)

> **Reality check:** A great product with no marketing = $30.
> A good product with great marketing = $3,000+.

### 9A: Pre-Launch (Start Now)

| # | Task | Effort | Impact |
|---|------|--------|--------|
| 9A.1 | **Pick ONE niche** (Worship OR NAM guitarists) | 1 hour | Critical |
| 9A.2 | **Landing page** (Gumroad/itch.io) | 2 hours | Critical |
| 9A.3 | **Email signup** (Buttondown free tier) | 1 hour | High |
| 9A.4 | **Discord server** | 30 min | High |
| 9A.5 | **Beta tester recruitment** (target: 50 people) | Ongoing | High |
| 9A.6 | **Teaser posts** on Reddit/forums | 2 hours | Medium |

**Niche Decision Matrix:**

| Niche | Market Size | Competition | Your Advantage |
|-------|-------------|-------------|----------------|
| **Worship teams** | Medium | Low | Planning Center, IEM routing, pads |
| **NAM guitarists** | Large | Medium | ToneHunt integration, one-click rig |
| **Holdsworth fans** | Small | None | UDS is literally made for this |

> **Recommendation:** Launch as "The NAM Pedalboard" — largest market, overlap with ToneHunt community.

---

### 9B: Content Creation (Non-Negotiable)

| # | Content | Effort | Platform |
|---|---------|--------|----------|
| 9B.1 | **"5-Minute Guitar Rig" video** | 2 hours | YouTube |
| 9B.2 | **Full demo/walkthrough** | 4 hours | YouTube |
| 9B.3 | **Comparison video** (vs Gig Performer/MainStage) | 2 hours | YouTube |
| 9B.4 | **Preset showcase** (Holdsworth tones) | 1 hour | YouTube |
| 9B.5 | **ToneHunt integration demo** | 1 hour | YouTube |
| 9B.6 | **GIF demos** for Reddit/forums | 1 hour | Reddit |

**Video Script Template:**
```
0:00 - Hook: "Build a complete guitar rig in 60 seconds"
0:15 - Problem: "Gig Performer costs $169, MainStage is Mac-only"
0:30 - Solution: "Pedalboard3 is free/open-source with built-in NAM"
1:00 - Demo: Add NAM amp, tuner, looper
2:00 - Result: Playing through the rig
3:00 - CTA: "Link in description, $29 for Pro features"
```

---

### 9C: Launch Channels

| Channel | Action | When | Effort |
|---------|--------|------|--------|
| **r/guitarpedals** | Demo post | Launch day | 1 hour |
| **r/NAMmodeler** | ToneHunt integration post | Launch day | 1 hour |
| **r/WeAreTheMusicMakers** | "I built a free alternative to GP" | Launch day | 1 hour |
| **r/mainstage** | "Windows alternative" post | Day 2 | 1 hour |
| **KVR Forum** | Product announcement thread | Launch day | 30 min |
| **Gearspace** | Demo thread | Launch day | 30 min |
| **ToneHunt Discord** | Integration announcement | Launch day | 30 min |
| **ProductHunt** | Launch submission | Day 3 | 2 hours |
| **Hacker News** | "Show HN" post | Day 5 | 30 min |

---

### 9D: Post-Launch (Sustaining Sales)

| # | Task | Frequency | Impact |
|---|------|-----------|--------|
| 9D.1 | Reply to all comments/questions | Daily | High |
| 9D.2 | Post update changelogs | Per release | Medium |
| 9D.3 | Share user rigs/presets | Weekly | High |
| 9D.4 | New content pack announcements | Monthly | Revenue |
| 9D.5 | YouTube Shorts/TikToks | Weekly | Visibility |
| 9D.6 | Affiliate program setup | Month 2 | Long-term |

---

### 9E: Revenue Optimization

| Strategy | Implementation | Revenue Impact |
|----------|----------------|----------------|
| **Upsell content packs** | Checkout page | +$9-27/customer |
| **Lifetime deal** ($49) | "Never pay for updates" | +$20/customer |
| **Supporter tier** ($99) | Name in credits, early access | +$70/superfan |
| **Affiliate program** (30%) | YouTubers promote | Free marketing |
| **Bundle deals** | Black Friday, etc. | Volume spikes |

**Target Average Order Value:**

| Tier | Base | Upsells | Total |
|------|------|---------|-------|
| Basic | $29 | +$9 (1 pack) | $38 |
| Enthusiast | $29 | +$27 (3 packs) | $56 |
| Superfan | $99 | +$29 (all packs) | $128 |

---

### 9F: The Math

| Scenario | Email List | Conversion | Price | Revenue |
|----------|------------|------------|-------|---------|
| **Minimum** | 100 | 10% | $29 | **$290** |
| **Target** | 300 | 15% | $38 avg | **$1,710** |
| **Good** | 500 | 20% | $45 avg | **$4,500** |
| **Great** | 1000 | 25% | $50 avg | **$12,500** |

**The Path to $3,000:**
1. Build email list of 200+ pre-launch
2. Launch with 5+ videos
3. Post on 5+ forums/subreddits
4. Offer upsells (content packs)
5. Respond to every comment (builds trust)

**The Path to $30,000:**
- All above, plus:
- Affiliate program with 5+ YouTubers
- Monthly content releases
- 6+ months of consistent posting
- Steam release for SporeEngine

---

### 9G: Influencer Outreach (The Chords of Orion Strategy)

> **Key insight:** Don't ask for reviews. Solve problems they've already expressed.
> "I built the thing you asked for" beats "Please review my plugin" every time.

#### Target YouTubers by Niche

| YouTuber | Subs | Niche | What You Send | Why It Works |
|----------|------|-------|---------------|--------------|
| **Chords of Orion** | ~50K | Holdsworth/jazz | UDS + Pedalboard3 + Holdsworth rig | He asked for UD Stomp software |
| **Rick Beato** | 4M+ | Music theory | UDS + technical explainer | Loves deep-dive gear content |
| **Martin Miller** | ~200K | Jazz/fusion | UDS + clean delay presets | Uses complex delay setups |
| **ToneHunt creators** | Varies | NAM community | Pedalboard3 + integration | Cross-promotion opportunity |
| **Sunday Sounds** | ~30K | Worship | Pedalboard3 + worship features | Planning Center integration |
| **Hillsong team members** | Varies | Worship | Pedalboard3 + IEM routing | Church-specific features |
| **FluffyAudio / Alex Ball** | ~50K | Synths | SporeEngine | Loves weird granular stuff |

#### Channel Size Strategy

| Size | Response Rate | Best For |
|------|---------------|----------|
| 1M+ | ~1% | Credibility if you land one |
| 100K-500K | ~5% | Good reach, selective |
| **10K-100K** | **~20%** | **Best ROI — niche authority** |
| 1K-10K | ~40% | Responsive, limited reach |

> **Focus on 10K-100K channels.** They're big enough to matter, small enough to respond.

#### The "No Ask" Approach

**Traditional pitch (bad):**
> "Hi, I made a plugin. Would you review it?"

**Your pitch (good):**
> "Hi, you mentioned wanting [X]. I built it. Here it is. No strings attached."

**Why this works:**
- Removes obligation/pressure
- Shows you did research
- Makes them *want* to help you
- Builds genuine relationship

#### Email Template

```
Subject: I built the [specific thing] you mentioned

Hey [Name],

A while back you mentioned [specific video/comment where they expressed 
a need]. I'm a developer and thought — I should just build that.

So I did.

**[Product Name]** is [one-sentence description]. I've attached:
- The plugin/installer
- A preset that does [specific thing they wanted]
- A quick guide (1 page)

No strings attached — just thought you'd appreciate having the tools.

If you find it useful and want to mention it, great. If not, enjoy.

[Your name]
[Optional: link to your site]
```

#### Bundle Preparation Checklist

**For Holdsworth/Jazz outreach:**
- [ ] UDS.vst3 (Windows + Mac builds)
- [ ] Pedalboard3 installer
- [ ] NAM model: JC-120 clean (Holdsworth's amp)
- [ ] Preset: "Holdsworth Delays" (pre-configured rig)
- [ ] PDF: "Recreating Holdsworth Tones" (1-2 pages)
- [ ] Quick start guide

**For Worship outreach:**
- [ ] Pedalboard3 installer
- [ ] LoopDeLoop plugin
- [ ] NAM model: Clean Fender-style (worship tone)
- [ ] Preset: "Sunday Morning" rig
- [ ] PDF: "Worship Setup Guide" (IEM routing, Planning Center)

**For NAM community outreach:**
- [ ] Pedalboard3 installer
- [ ] ToneHunt integration demo video (60 sec)
- [ ] 3 curated NAM models (amp + cab)
- [ ] Quick start guide

#### Research Before Pitching

For each target, document:
- [ ] Watch 3-5 of their videos
- [ ] Note gear they use
- [ ] Find comments where they want something
- [ ] Find the specific video/timestamp to reference
- [ ] Draft personalized email

#### Outreach Timeline

| Week | Action |
|------|--------|
| 1 | Research 10 targets, draft emails |
| 2 | Send first 5 emails (staggered) |
| 3 | Send remaining 5, follow up on week 2 |
| 4 | Send thank-you to anyone who responded |
| Ongoing | Add new targets as you find them |

#### Expected Results

| Emails Sent | Responses | Reviews | Sales Impact |
|-------------|-----------|---------|--------------|
| 10 | 2-3 | 1 | $200-500 |
| 25 | 5-8 | 2-3 | $500-1500 |
| 50 | 10-15 | 5-7 | $1500-4000 |

> **One good YouTube mention = 50-200 sales.**
> Chords of Orion mentioning UDS could be worth $1,000+ in direct sales.

---

#### Holdsworth Bundle Outreach (Pedalboard3 + UDS)

> **The Bundle Pitch:** "A complete Holdsworth rig — JC-120 amp sim, UD Stomp delay, and looper — all in one app. No plugins to configure."

**Why Bundle Beats Single Product:**

| Approach | Friction | Value Prop |
|----------|----------|------------|
| "Here's a delay plugin" | Need DAW, amp sim, setup | Low |
| "Here's a complete rig, ready to play" | Open app, plug in, go | **High** |

**The Holdsworth Bundle:**

```
Pedalboard3 + NAM JC-120 + UDS Delay = Complete Rig
```

All built-in. No external plugins. $29.

---

**Target Channels (Bundle Focus)**

| Name | Subs | Pitch Angle |
|------|------|-------------|
| **Chords of Orion** | ~50K | "Complete Holdsworth rig in one app" |
| **Tom Quayle** | ~100K | "Legato practice rig with built-in looper" |
| **r/jazzguitar** | ~80K | "Free/open-source jazz guitarist's pedalboard" |
| **Holdsworth FB Group** | ~5K | "UD Stomp + JC-120 in software form" |

**Example Email (Bundle Angle):**

```
Subject: A complete Holdsworth rig — amp, delay, looper — in one app

Hey [name],

You mentioned looking for a UD Stomp alternative. I built one — but I 
went further and put it inside a complete pedalboard app.

**Pedalboard3** is a free/open-source VST host with:
- Built-in NAM amp sim (JC-120 clean loaded)
- Built-in UDS delay (UD Stomp emulation)
- Built-in looper
- A pre-configured "Holdsworth Rig" preset

Plug in your guitar, open the app, and play. No configuration needed.

I've attached everything. No strings attached — just thought you'd 
appreciate the complete package.

[Your name]
```

**Why This Works for Pedalboard3:**

| Single Plugin | Complete Rig |
|---------------|--------------|
| They already have a DAW | They might not |
| Competes with other delays | Competes with nothing |
| $9-29 value | $50+ perceived value |
| "Another plugin" | "My whole setup" |

---



### Pre-Launch Checklist

- [ ] Niche chosen (NAM guitarists recommended)
- [ ] Landing page live
- [ ] Email signup working
- [ ] Discord server created
- [ ] 5+ beta testers recruited
- [ ] Demo video recorded
- [ ] 3 preset packs created
- [ ] Reddit posts drafted
- [ ] Forum accounts created (KVR, Gearspace)
- [ ] Gumroad/itch.io store set up
- [ ] Influencer research completed (10 targets)
- [ ] Outreach bundles prepared

---

*Last updated: 2026-01-28*


