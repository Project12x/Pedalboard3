# Pedalboard3 Roadmap

- **Last updated:** 2026-06-09
- **Status:** Active source of truth for current product work

This roadmap is intentionally narrow. Historical phase plans and completed long-form
tracking belong in archive/status documents, not in the active work queue.

Reference documents:

- Historical completed phases: [docs/archive/PHASED_PLAN_ARCHIVE.md](docs/archive/PHASED_PLAN_ARCHIVE.md)
- UI polish execution detail: [UI_POLISH_ROADMAP_UPGRADE.md](UI_POLISH_ROADMAP_UPGRADE.md)
- Gig-speed feature detail: [P0_GIG_SPEED_FEATURES.md](P0_GIG_SPEED_FEATURES.md)
- Bugfix/gap tracking: [BUGFIX_ROADMAP.md](BUGFIX_ROADMAP.md)
- Current scratch-capture handoff: [INSTANT_SCRATCH_CAPTURE_HANDOFF_2026-06-05.md](INSTANT_SCRATCH_CAPTURE_HANDOFF_2026-06-05.md)

---

## Product Direction

Pedalboard3 should prioritize immediate, local, musician-facing utility:

1. Plug in a guitar or instrument.
2. Start Pedalboard.
3. Get useful sound quickly.
4. Verify signal confidence before playing.
5. Record scratch ideas immediately, with both raw DI and wet output preserved.

Do not treat speculative primitives as roadmap items unless they are attached to a
clear musician workflow, acceptance criteria, and implementation path.

---

## Current Ship Gate: Instant Scratch Capture

**Status:** Implemented on `codex/pedalboard-remix-ui-polish`; automated verification passed; manual hardware QA pending.

This is the active branch gate because it directly supports the desired workflow:
plug in, start Pedalboard, and capture an idea within moments.

| Item | Status | Notes |
|---|---|---|
| App-level scratch recorder | Done | `ScratchRecorder` writes synchronized raw and wet WAV files. |
| Raw + wet simultaneous capture | Done | Raw tap is pre-chain; wet tap is post-chain/master output. |
| Take folders and metadata | Done | `take.json`, `raw.wav`, and `wet.wav` are created per take. |
| Footer controls | Done | `REC`/`STOP`, status, and `Takes` affordances are wired. |
| File menu commands | Done | Start/stop, open panel, reveal folder. |
| Patch/device interruption handling | Done | Capture stops with explicit incomplete-take reasons. |
| Focused tests | Done | Scratch tests pass; full Release CTest passed in handoff evidence. |
| Manual guitar/interface smoke test | Pending | Must verify real raw/wet capture before calling user-ready. |
| Fresh footer scale screenshots | Done | Captured at `documentation/qa/2026-06-09-scratch-footer`; narrow 125%-200% keeps scratch controls recoverable. |
| Main footer scale follow-up | Done | Captured at `documentation/qa/2026-06-09-main-footer-scale-v2`; high-scale footer keeps existing controls visible. |
| Scratch panel elapsed timer | Done | Active recording elapsed label is covered by a focused scratch regression. |
| Scratch panel remix polish | Done | Hero record control, RAW/WET context, destination display, date labels, and recent take actions are implemented. |
| Scratch destination controls | Done | Scratch folder choose/reset is available from the scratch panel and app menu, with persisted path setting. |

Immediate next agent sequence:

1. Run a real audio-interface/guitar capture smoke test.
2. Confirm the take contains `raw.wav`, `wet.wav`, and `take.json`.
3. Confirm raw is pre-chain DI and wet is what the user heard.
4. Verify patch change and audio-device change both stop capture cleanly.
5. Confirm `Play`, `Reamp`, and `Reveal` take actions enable only when their files exist.
6. Review `documentation/qa/2026-06-09-scratch-footer` and `documentation/qa/2026-06-09-main-footer-scale-v2` if UI-scale evidence is needed before PR.

---

## P0 Gig-Speed Work

**Status:** Active; partially complete.

These features make Pedalboard feel fast enough for real use without turning the
roadmap into a broad feature grab-bag.

| Feature | Status | Current Evidence / Next Step |
|---|---|---|
| Device-level meter tap | Done | `src/DeviceMeterTap.h/.cpp` exists and is wired through `MainPanel`. |
| Built-in VU on Audio I/O nodes | Done | `PluginComponent` renders input/output node meters from `DeviceMeterTap`. |
| One-Click Soundcheck | Next P0 | `SoundcheckDialog` does not exist yet. Build on `DeviceMeterTap`. |
| Starter Rig Browser | Next P0 | `StarterRig*` files and starter rig content do not exist yet. |

Recommended order after scratch capture:

1. One-Click Soundcheck.
2. Starter Rig Browser and starter content pack.
3. First-run entry point only after starter rigs are real and useful.

---

## P0 UI Polish

**Status:** P0 closed; P1/P2 polish remains.

The P0 UI polish backlog in `UI_POLISH_ROADMAP_UPGRADE.md` is effectively closed:
A1, A2, B1, B2, B3, D1, and D2 are Done. This includes the Pedalboard UI scale
work, the 75% scale floor, footer/menu/preferences scale controls, and scaled
visual QA evidence from the May/June passes.

Remaining UI work should stay subordinate to musician workflows and bug risk:

| Area | Priority | Status |
|---|---|---|
| State feedback and focused motion | P1 | Planned |
| Secondary surface alignment | P1 | Planned |
| Connection and bypass signal cues | P1 | Planned |
| Internal editor consistency rollout | P2 | Planned |
| SVG/icon and visual asset pass | P2 | Planned |
| CPU meter redesign | P2 | Planned |

Do not reopen broad "premium polish" as an undefined P0 bucket. Any new polish work
needs a concrete workflow, affected surfaces, and verification path.

---

## Release Hardening

**Status:** Important, but behind current ship gate and remaining P0 gig-speed work.

| Area | Status | Notes |
|---|---|---|
| GitHub Actions | Planned | Should build app and tests consistently. |
| Multi-platform build matrix | Planned | Windows first; macOS/Linux only when explicitly prioritized. |
| Windows installer | Planned | NSIS or equivalent. |
| Code signing | Planned | Required for credible user distribution. |
| Crash dumps/reporting | Planned | Crashpad/Sentry class work; decide privacy posture first. |
| Performance profiling | Planned | Tracy remains useful engineering tooling, not product feature work. |

---

## P1 Product Polish

These are useful after the current ship gate and P0 gig-speed items.

| Feature | Status | Notes |
|---|---|---|
| Scratch folder preference | Done for V1 | Scratch panel and app menu expose choose/reset; no Preferences mirror unless later requested. |
| Scratch take playback/reamp | Partial | Wet preview opens the saved wet file; raw reamp adds a file-player node for the raw capture. No timeline/editor. |
| Recent scratch take management | Partial | Recent list shows date/time, patch context, RAW/WET metadata, and reveal/play/reamp actions. Keep small; avoid DAW/library bloat. |
| Quick onboarding entry point | Planned | Should point to real starter rigs once they exist. |
| Focused keyboard shortcut overlay | Planned | Only if it improves discoverability of existing workflows. |

---

## Completed Foundation Summary

See [CHANGELOG.md](CHANGELOG.md) and archived plans for details.

| Phase / Area | Status |
|---|---|
| JUCE 8 migration | Complete |
| Build system, logging, and test framework | Complete |
| VST3 hosting, themes, settings | Complete |
| Undo/redo system | Complete |
| Plugin blacklist and crash protection | Complete |
| Out-of-process plugin scanner | Complete |
| NAM loader, model browser, and ToneHunt integration | Complete |
| IR Loader, dual IR loading/blend, low/high cut filters | Complete |
| Virtual MIDI input/keyboard enhancements | Complete |
| Mixer/Splitter processors and master bus insert rack | Complete |
| Thread/lifetime hardening tracked in `BUGFIX_ROADMAP.md` | Complete |

---

## Parking Lot / Future Bets

These are not active roadmap commitments. Keep them parked until there is a
specific workflow, user need, and implementation plan.

| Area | Parked Ideas |
|---|---|
| Pro/live utilities | Talkback mode, placeholder nodes, full-screen lyrics |
| Visual nodes | Oscilloscope, spectrum analyzer, image node, clock/timer, lyrics sheet, 3D visualization |
| Switching | Tail spillover beyond existing crossfade mixer infrastructure |
| MIDI processing | MIDI split, MIDI layer mode |
| Backing tracks | Streaming player, transport, per-song assignment, loop regions |
| Worship-specific workflows | IEM mix routing, song sections, countdown, ambient pads, Planning Center import, presentation output, CCLI field, Ableton Link |
| Content packs | Curated NAM models, bundled IR pack, bundled instruments |
| Sampler | sfizz/SFZ/SF2 integration and browser |
| Plugin ecosystem | GPL plugin bundle, LV2, CLAP, Ildaeil bridge, scanner expansion |
| Cross-platform/headless | macOS, Linux, `--no-gui`, JSON/OSC API, systemd |
| Networking/cloud | WebSocket remote, preset cloud sync, Zeroconf, mobile remote |
| Hardware appliance | ARM Linux, JACK backend, GPIO, LCD/OLED, minimal memory mode |
| Marketing/revenue | Launch channels, influencer outreach, content strategy, affiliate/supporter/lifetime tiers |

---

## Roadmap Rules

1. P0 work must map to a concrete musician workflow.
2. A feature is not active just because a dependency or primitive is interesting.
3. Research docs stay separate until an idea is approved for implementation.
4. Manual audio hardware QA is required for capture, soundcheck, and live signal features.
5. UI-scale verification uses Pedalboard's app-level scale controls; OS display scale is only a compatibility check.
6. Any implementation based on researched prior art must follow the reference-code-first policy in `AGENTS.md`.
