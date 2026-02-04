# Bundled Products Strategy

> **Purpose:** In-house plugins bundled exclusively with Pedalboard3.
> **Advantage:** No licensing concerns — you own the code. This is the honest bundling opportunity.

---

## Existing Products (Commercial)

| Product | Type | Status | Bundle Value |
|---------|------|--------|--------------|
| **LoopDeLoop** | Looper | Active development | Core workflow tool — instant multi-track looping without leaving P3 |
| **UDS Delay** | Effect | Active development | Allan Holdsworth-style modulated delay — unique selling point |

---

## Planned Chip Synthesizers

| Synth | Chip | Effort | Reference Code | Priority |
|-------|------|--------|----------------|----------|
| **SID Synth** | Commodore 64 (6581/8580) | 3-4 weeks | reSIDfp (GPL-2.0+) — study filter math | 🔥 High |
| **GB Synth** | Game Boy APU | 1 week | blargg's gb_apu (LGPL) or SameBoy (MIT) | Medium |
| **NES Synth** | 2A03 APU | 1-2 weeks | blargg's nes_apu (LGPL) | Medium |

### SID Synth Architecture

```
┌─────────────────────────────────────────┐
│ SID Synth VST3                          │
├─────────────────────────────────────────┤
│ 3 Oscillators (saw, tri, pulse, noise)  │
│ Ring modulation + hard sync             │
│ 6581/8580 analog filter emulation       │
│ ADSR envelopes                          │
│ Preset library: C64 game sounds         │
└─────────────────────────────────────────┘
```

**Key insight:** The SID filter is what makes it unique. Study reSIDfp's filter curves, implement clean-room.

---

## Planned Utility Processors

| Processor | Type | Effort | Notes |
|-----------|------|--------|-------|
| **Tone Generator** | Utility | ✅ Done | Already in P3 |
| **MIDI Monitor** | Utility | 1-2 days | Debug tool, useful for users |
| **Audio Scope** | Utility | 2-3 days | Waveform + spectrum |
| **Tuner** | Utility | 1 day | Chromatic tuner node |
| **Metronome** | Utility | 1 day | Click track with tap tempo |

---

## Bundle Strategy

### "Pedalboard3 Studio" Bundle

| Category | Included |
|----------|----------|
| **Host** | Pedalboard3 |
| **Looper** | LoopDeLoop |
| **Effects** | UDS Delay |
| **Synths** | SID Synth, GB Synth, NES Synth |
| **Utilities** | Tone Gen, MIDI Monitor, Tuner, Metronome |
| **GPL Synths** | Dexed, OPNplug, ADLplug, Surge XT (optional) |

**Price:** $25-30 for the full bundle (your plugins add real value over free GPL synths alone)

### Steam Positioning

"Instant music studio with pro-quality bundled instruments. No plugins to buy. No subscription."

---

## Development Priority

| Phase | Products | Timeline |
|-------|----------|----------|
| **Now** | LoopDeLoop, UDS Delay | Active |
| **Q1 2026** | SID Synth | 3-4 weeks |
| **Q2 2026** | GB Synth, NES Synth | 2-3 weeks |
| **Ongoing** | Utilities, presets | As needed |

---

## Revenue Model

| Channel | Your Plugins | GPL Synths |
|---------|--------------|------------|
| **Standalone sales** | ✅ Sell individually | ❌ Can't charge for source |
| **P3 bundle** | ✅ Adds premium value | ✅ Adds volume |
| **Steam** | ✅ Differentiator | ✅ "Free synths included" marketing |

**Key insight:** Your in-house plugins (LoopDeLoop, UDS, chip synths) justify the price. GPL synths add volume and legitimacy. Together = compelling bundle.
