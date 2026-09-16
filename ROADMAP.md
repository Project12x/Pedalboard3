# Pedalboard3 Roadmap

- **Last updated:** 2026-06-20
- **Status:** Active source of truth for current product work

This roadmap is intentionally narrow. Historical phase plans and completed long-form
tracking belong in archive/status documents, not in the active work queue.

Reference documents:

- Historical completed phases: [docs/archive/PHASED_PLAN_ARCHIVE.md](docs/archive/PHASED_PLAN_ARCHIVE.md)
- UI polish execution detail: [UI_POLISH_ROADMAP_UPGRADE.md](UI_POLISH_ROADMAP_UPGRADE.md)
- Gig-speed feature detail: [P0_GIG_SPEED_FEATURES.md](P0_GIG_SPEED_FEATURES.md)
- Bugfix/gap tracking: [BUGFIX_ROADMAP.md](BUGFIX_ROADMAP.md)
- Current scratch-capture handoff: [INSTANT_SCRATCH_CAPTURE_HANDOFF_2026-06-05.md](INSTANT_SCRATCH_CAPTURE_HANDOFF_2026-06-05.md)
- Embedded editor reference: Kushview Element source study, `https://github.com/kushview/element`, commit `c2d7e0d61def46cb50f0fa8eabddceeb0f9fdb3d`, license `GPL-3.0-or-later`, files inspected: `src/ui/block.cpp`, `src/ui/block.hpp`, `src/ui/blockutils.hpp`, `src/ui/nodeeditorfactory.cpp`, `src/ui/pluginwindow.cpp`, `src/ui/windowmanager.cpp`, `src/ui/grapheditorcomponent.cpp`, `src/ui/nodeeditorview.cpp`, `src/engine/clapprovider.cpp`. Reuse mode: pattern-only / clean-room.

---

## Product Direction

Pedalboard3 should prioritize immediate, local, musician-facing utility:

1. Plug in a guitar or instrument.
2. Start Pedalboard.
3. Get useful sound quickly.
4. Verify signal confidence before playing.
5. Record scratch ideas immediately, with both raw DI and wet output preserved.
6. Switch stage-ready sounds confidently from GUI, keyboard, and hardware controllers.

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

## Scratch Mode Next: One-Click Scratch Templates

**Status:** Next product layer after manual raw/wet hardware QA.

Scratch Mode should become the fastest path from instrument to recorded idea:
choose one of a handful of useful sounds, plug in, play, and capture both the
heard tone and the dry DI for future reamping.

| Item | Status | Notes |
|---|---|---|
| Scratch template manifest | Planned | Small curated set: clean DI, edge-of-breakup, high gain, bass, ambient/lead. Keep metadata minimal and local. |
| Template-backed rig loading | Planned | Reuse the Starter Rig Browser/template work instead of adding a separate scratch-only loader. |
| Record-ready validation | Planned | Before recording, surface missing input, missing output, or unavailable required plugin/model in one clear state. |
| Raw/wet invariant | Required | Every guitar scratch template must preserve synchronized `raw.wav`, `wet.wav`, and `take.json`. |
| Reamp action hardening | Planned | Raw reamp should load into a predictable reamp path, not just add an ambiguous file-player node. |
| Tuner integration | Later | Add a fast tune/check affordance near Scratch, but do not block template recording on tuner UI work. |
| Looper integration | Later | Useful for practice, but keep separate from the raw/wet take bundle in the first template pass. |
| MIDI scratch | Later | Keyboard-oriented scratch capture should use the same take/history model, with MIDI capture added deliberately. |

Acceptance criteria for the first template pass:

1. A new user can open Pedalboard, choose a scratch template, and record a usable guitar idea in under 60 seconds.
2. The take folder contains synchronized raw and wet files plus metadata.
3. Missing optional assets fail gracefully with an explicit action, not a crash or silent empty rig.
4. The workflow does not require adding an Audio Recorder node or manually wiring a DI split.

---

## Stage Mode Next: Live Action Map

**Status:** Active direction; implementation still planned.

Stage currently has setlists, queue/grid views, and GUI patch switching. The next
stage-facing step is to route all live operations through one action layer so GUI,
keyboard shortcuts, MIDI pedals, and later OSC/hardware surfaces behave the same.

| Item | Status | Notes |
|---|---|---|
| `StageAction` command model | Planned | Define stable actions: previous/next patch, select patch index, grid bank up/down, tuner view, panic, scratch record toggle. |
| MIDI pedal mapping | Planned | Build on existing MIDI mapping/FIFO patterns; controller input should trigger actions, not reach into `StageView` internals. |
| Grid/setlist action feedback | Planned | Current patch, queued next patch, selected bank, and blocked actions need large live-safe feedback. |
| Controller-safe patch switching | Planned | Patch changes must preserve existing crossfade/preload behavior and stop scratch capture cleanly when required. |
| Stage layout persistence | Planned | Preserve chosen Stage view and grid/setlist preferences per app or per set, after action routing is stable. |
| Looper/tuner/scratch hooks | Later | Add as actions after the core patch/grid/controller path is reliable. |

Acceptance criteria for the first action-map pass:

1. GUI buttons, Stage keyboard shortcuts, and MIDI pedal events call the same action dispatcher.
2. Next/previous patch and direct grid tile selection behave identically across GUI and controller input.
3. Existing Stage view rendering and patch-switch tests remain valid.
4. Controller actions are ignored or clearly rejected when they would be unsafe during patch load or capture stop.

---

## Embedded Node Editors

**Status:** Planned P1 after Scratch Templates and Stage Action Map foundations.

Element's useful pattern is an opt-in node display mode, not default embedding:
the graph block owns the same editor component that would normally live in a
floating plugin window, closes any floating window for that node, sizes the node
around the editor, and tears the editor down before returning to normal display.

Pedalboard3 should adapt that behavior clean-room, using its existing
`PluginComponent` and `PluginEditorWindow` lifecycle:

| Item | Status | Notes |
|---|---|---|
| Node display mode metadata | Planned | Persist `normal` / `compact` / `embedded` per node in patch XML. |
| `EmbeddedPluginEditorHost` helper | Planned | Own editor creation, fallback, child sizing, focus policy, and teardown outside the already-large `PluginComponent.cpp`. |
| Floating/embedded exclusivity | Required | A node may have either a floating editor or an embedded editor, never both. |
| Safe editor creation | Required | Reuse current crash-protected custom-editor creation path; fall back to `NiallsGenericEditor`. |
| Internal node first pass | Planned | Validate on NAM, IR Loader, tuner, mixer/splitter, and generic editors before enabling arbitrary external plugin UIs. |
| External plugin allowlist/opt-in | Planned | External custom UIs should be opt-in per node/plugin because DPI, native child windows, keyboard focus, and popups vary by vendor. |
| Graph interaction contract | Planned | Embedded editors must not break node drag, pin hit testing, cable drawing, zoom, fit-to-screen, or deletion. |

Non-goals for the first pass:

1. Do not make embedded custom UIs the default for all third-party plugins.
2. Do not support simultaneous floating and embedded editors for the same plugin instance.
3. Do not copy Element source; use the pattern only, with the inspected source recorded above.

Acceptance criteria for the first pass:

1. A supported node can switch between normal, compact, and embedded display modes.
2. Embedded mode survives save/load and patch switching.
3. Closing/deleting a node destroys the editor without leaving stale processor/editor pointers.
4. A plugin that fails editor creation falls back to the generic editor or normal node display with a visible message.
5. Layout remains usable at supported UI scales.

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
| Starter Rig Browser / Scratch Templates | Next P0 | `StarterRig*` files and starter rig content do not exist yet. This should also power the first Scratch Template presets. |

Recommended order after scratch capture:

1. One-Click Soundcheck.
2. Starter Rig Browser and starter content pack.
3. Scratch Templates built from the same template/manifest system.
4. First-run entry point only after starter rigs are real and useful.

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
| Embedded node editors | Planned | Opt-in node display mode based on clean-room Element pattern study; internal/generic editors first, arbitrary external custom UIs later. |
| Stage MIDI pedal control | Planned | Build via the Stage Action Map, not one-off `StageView` shortcuts. |
| Quick onboarding entry point | Planned | Should point to real starter rigs once they exist. |
| Focused keyboard shortcut overlay | Planned | Only if it improves discoverability of existing workflows. |

---

## Plugin Format Expansion Track

**Status:** Opportunistic infrastructure, not the main product wedge.

The product direction remains live utility and fast capture. Format expansion is
valuable when it directly increases usable rigs and compatibility.

| Format | Roadmap Position |
|---|---|
| CLAP | Revisit when JUCE 9 has official, stable host support. Avoid carrying a custom CLAP host unless it becomes strategically necessary. |
| AU | Enable on macOS builds when a Mac build/test path exists. JUCE support is not the blocker; hardware/CI validation is. |
| VST2 | Treat as legacy/compatibility work with explicit SDK/license handling. Do not make it a release blocker. |
| LADSPA | Feasible as a utility format if Linux support becomes active; not a Windows-first differentiator. |
| LV2 | Keep as a Linux/open ecosystem expansion candidate. Existing experiments/research are useful, but ship only with concrete user workflow coverage. |

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
| Plugin ecosystem | GPL plugin bundle, Ildaeil bridge, scanner expansion beyond approved format work |
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
