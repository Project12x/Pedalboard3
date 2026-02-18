# P0 Gig-Speed Features

**Product intent:** Fast, practical gig utility for local performers.  
**Focus:** Time-to-sound, confidence before playing, and always-visible levels.

---

## Objective

Ship the smallest set of high-impact features that make first use and weekly gig use feel instant:

1. Starter Rig Browser
2. One-Click Soundcheck
3. Built-in VU meters on Audio Input/Output nodes

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

## 2) One-Click Soundcheck (P0)

## Outcome
User can verify input signal, output signal, clipping/headroom, and basic readiness before playing.

## Scope
- Device-level meter tap independent from graph UI.
- One simple dialog with per-channel in/out activity.
- Clear pass/warn states for silence and clipping.

## Files to add
- `src/DeviceMeterTap.h`
- `src/DeviceMeterTap.cpp`
- `src/SoundcheckDialog.h`
- `src/SoundcheckDialog.cpp`

## Files to modify
- `src/MainPanel.h`
- `src/MainPanel.cpp`
- `src/ColourScheme.cpp` (only if additional meter semantic colors are needed)
- `CMakeLists.txt`

## Integration notes
- Register `DeviceMeterTap` with `AudioDeviceManager` alongside existing callbacks.
- Keep meter values thread-safe and UI-polled on timer.

## Acceptance criteria
- Soundcheck opens from menu/toolbar and updates live.
- Detects and surfaces: no input, no output, clipping.
- Adds negligible CPU overhead during normal operation.

---

## 3) Built-in VU on Audio I/O Nodes (P0)

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

1. `DeviceMeterTap` foundation (enables both Soundcheck and node VU)
2. Audio I/O node VU rendering (fastest visible value)
3. Soundcheck dialog
4. Starter Rig Browser + starter content pack

---

## Non-Goals (for this phase)

- Full plugin bundling/packaging of third-party binaries.
- Universal plugin substitution engine.
- Touring/enterprise reliability programs.
- Controller profile system overhaul.

---

## Definition of Done

- All three P0 features shipped and accessible from main UX.
- First-time user can get sound quickly from a starter rig.
- Pre-gig signal confidence is available without opening extra plugin nodes.
- No regressions in patch load/switch and transport behavior.
