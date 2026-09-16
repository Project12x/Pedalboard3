# P0 Gig-Speed Features

- **Product intent:** Fast, practical gig utility for local performers.
- **Focus:** Time-to-sound, confidence before playing, and always-visible levels.
- **Last updated:** 2026-06-20
- **Status:** Active; partially complete

---

## Objective

Ship the smallest set of high-impact features that make first use and weekly gig use feel instant:

1. Starter Rig Browser
2. Scratch Templates for one-click raw/wet idea capture
3. One-Click Soundcheck
4. Built-in VU meters on Audio Input/Output nodes

---

## Current Status

| Item | Status | Notes |
|---|---|---|
| DeviceMeterTap foundation | Done | `src/DeviceMeterTap.h/.cpp` exists and is wired through `MainPanel`. |
| Built-in VU meters on Audio I/O nodes | Done | `PluginComponent` renders input/output node levels from `DeviceMeterTap`. |
| One-Click Soundcheck | Planned / next P0 | `SoundcheckDialog` has not been implemented. |
| Starter Rig Browser | Planned / next P0 | `StarterRigManager`, `StarterRigBrowser`, and starter rig content do not exist yet. |
| Scratch Templates | Planned / next P0 | Should reuse the starter rig template system so guitar scratch presets do not become a parallel loader. |

---

## 1) Starter Rig Browser (P0)

## Outcome
A user can open Pedalboard3, choose a starter rig, and play immediately.

## Scope
- Curated starter rigs as real `.pd` templates.
- Lightweight browser dialog with search/tag filter.
- Menu entry + optional first-run entry point.

## Files to add
- `src/StarterRigManager.h`
- `src/StarterRigManager.cpp`
- `src/StarterRigBrowser.h`
- `src/StarterRigBrowser.cpp`

## Files to modify
- `src/MainPanel.h`
- `src/MainPanel.cpp`
- `src/App.cpp`
- `CMakeLists.txt`

## Data/content
- New folder: `starter_rigs/`
- Manifest format (JSON or simple metadata file) containing:
  - `name`
  - `tags`
  - `description`
  - `plugin_requirements`
  - `template_file`

## Acceptance criteria
- User can open browser from menu and load a rig in 2 clicks.
- First-run user can reach a playable rig in under 60 seconds.
- Missing optional third-party plugins do not crash rig load; rig still opens.

---

## 2) Scratch Templates (P0)

## Outcome
A user can pick a small preset such as clean DI, edge-of-breakup, high gain,
bass, or ambient lead, plug in a guitar, and record a scratch take with dry DI
and wet output preserved for later reamping.

## Scope
- Curated scratch presets implemented as starter rig templates, not a separate system.
- A scratch-focused entry point that loads the template and arms the existing raw/wet recorder workflow.
- Template metadata that identifies required plugins, NAM models, IRs, and fallback behavior.
- Clear record-ready states for missing input, missing output, missing model, or missing plugin.

## Files to add
- Reuse `src/StarterRigManager.h`
- Reuse `src/StarterRigManager.cpp`
- Reuse `src/StarterRigBrowser.h`
- Reuse `src/StarterRigBrowser.cpp`
- Optional later: `src/ScratchTemplateBrowser.h`
- Optional later: `src/ScratchTemplateBrowser.cpp`

## Files to modify
- `src/MainPanel.h`
- `src/MainPanel.cpp`
- `src/ScratchPanel.h`
- `src/ScratchPanel.cpp`
- `src/ScratchTake.h` only if additional template metadata belongs in `take.json`
- `CMakeLists.txt`

## Data/content
- Use `starter_rigs/` for template files and metadata.
- Add scratch-specific metadata fields only where useful:
  - `scratch_ready`
  - `captures_raw_wet`
  - `instrument`
  - `required_models`
  - `required_irs`
  - `fallback_template`

## Acceptance criteria
- User can choose a scratch preset and start recording without manually adding or wiring recorder nodes.
- Every guitar scratch preset preserves synchronized `raw.wav`, `wet.wav`, and `take.json`.
- Missing optional assets produce a clear message and safe fallback.
- The take metadata records the patch/template context well enough to reamp later.
- The existing Scratch hardware smoke remains valid.

---

## 3) One-Click Soundcheck (P0)

## Outcome
User can verify input signal, output signal, clipping/headroom, and basic readiness before playing.

## Scope
- Device-level meter tap independent from graph UI.
- One simple dialog with per-channel in/out activity.
- Clear pass/warn states for silence and clipping.

## Files to add
- `src/SoundcheckDialog.h`
- `src/SoundcheckDialog.cpp`

## Files to modify
- `src/MainPanel.h`
- `src/MainPanel.cpp`
- `src/ColourScheme.cpp` (only if additional meter semantic colors are needed)
- `CMakeLists.txt`

## Integration notes
- Reuse the existing `DeviceMeterTap` registered by `MainPanel`.
- Keep meter values thread-safe and UI-polled on timer.

## Acceptance criteria
- Soundcheck opens from menu/toolbar and updates live.
- Detects and surfaces: no input, no output, clipping.
- Adds negligible CPU overhead during normal operation.

---

## 4) Built-in VU on Audio I/O Nodes (P0)

## Outcome
Audio Input and Audio Output nodes always show live per-channel level activity directly on canvas.

## Scope
- Draw compact per-channel bars on Audio Input and Audio Output node cards.
- Reuse existing theme colors (`VU Meter Lower/Upper/Over`).
- Zero extra graph nodes required.

## Files to modify
- `src/PluginComponent.h`
- `src/PluginComponent.cpp`
- `src/MainPanel.cpp` (to route meter source/state as needed)

## Implementation notes
- Feed meter data from `DeviceMeterTap`.
- For Audio Input node: display device input channel levels.
- For Audio Output node: display device output channel levels.
- Keep repaint frequency controlled (timer-based) to avoid UI churn.

## Acceptance criteria
- Audio I/O nodes display live movement when signal is present.
- Channel count display follows active device channel config.
- Meter rendering remains readable in all built-in themes.

---

## Build Order (Recommended)

1. `DeviceMeterTap` foundation (done)
2. Audio I/O node VU rendering (done)
3. Soundcheck dialog (next)
4. Starter Rig Browser + starter content pack
5. Scratch Templates built on the starter rig template system

---

## Non-Goals (for this phase)

- Full plugin bundling/packaging of third-party binaries.
- Universal plugin substitution engine.
- Touring/enterprise reliability programs.
- Controller profile system overhaul.
- DAW-style timeline, comping, or scratch library management.
- A separate scratch-template format that duplicates starter rig loading.

---

## Definition of Done

- All P0 features shipped and accessible from main UX.
- First-time user can get sound quickly from a starter rig.
- Guitar scratch capture can start from a useful preset and preserves raw/wet files.
- Pre-gig signal confidence is available without opening extra plugin nodes.
- No regressions in patch load/switch and transport behavior.
