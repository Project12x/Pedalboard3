# Polish Status Audit - 2026-06-21

## Context

- Branch: `codex/rt-hosting-sprint`
- HEAD: `df3eeda fix: split IR loader text roles by surface`
- Request: audit the remaining polish list because many of the previously reported issues are now done.
- Audit stance: do not reopen old polish reports without current evidence. Classify each area as done, needs proof, or still open.
- Working tree caveat: `P0_GIG_SPEED_FEATURES.md` and `ROADMAP.md` already have unstaged changes and were not treated as part of this audit.

## Evidence Gathered

- Recent commit history shows focused follow-up commits for ReverbSC, mixer/splitter, Tone Generator, NAM Loader, IR Loader, Effect Rack, Notes, MIDI labels, and NAM browser polish.
- Source/test contracts inspected:
  - `tests/reverbsc_processor_test.cpp`
  - `tests/ui_regression_harness_test.cpp`
  - `tests/nam_processor_test.cpp`
  - `scripts/run_d2_visual_qa.ps1`
  - `TESTLATER.md`
  - `ROADMAP.md`
- Latest committed visual QA artifacts under `documentation/qa` are from 2026-06-16. They predate the latest June 21 NAM/IR Loader text-role fixes, so current screenshots are stale.
- Focused Release tests run during this audit:
  - `.\\build\\tests\\Release\\Pedalboard3_Tests.exe "[ui][regression][visual]"` passed: 837 assertions in 16 test cases.
  - `.\\build\\tests\\Release\\Pedalboard3_Tests.exe "[reverbsc]"` passed: 304 assertions in 17 test cases.
  - `.\\build\\tests\\Release\\Pedalboard3_Tests.exe "[nam][a2][ui]"` passed: 63 assertions in 4 test cases.

## Done or Contract-Covered

### ReverbSC Direct Node

Status: done at source/test-contract level; needs current screenshot/manual confirmation.

Evidence:
- `tests/reverbsc_processor_test.cpp` covers stable metadata, compact stereo graph pins, embedded controls for every parameter, suppression of redundant editor/param-pin affordances, no generic host bottom padding, no nested node shell, no duplicate title chrome, polished direct-surface primitives, compact footer spacing, state round-trip, DSP impulse behavior, and internal plugin registration.
- `tests/ui_regression_harness_test.cpp` also checks ReverbSC theme-colour use and rejects hardcoded `Colours::white`, `Colours::black`, and `Colour(0x...)`.

Residual risk:
- The current visual QA screenshots are stale, so actual theme/scale rendering still needs a fresh screenshot pass.

### Internal Plugin Duplicate Prevention

Status: done at source-contract level.

Evidence:
- `tests/ui_regression_harness_test.cpp` includes `Internal plugin descriptions are runtime catalog entries, not persisted scan results`.
- That contract verifies runtime internal plugin XML is removed from persisted plugin-list XML before restore/save, which addresses the duplicate internal-node list failure at the source.

Residual risk:
- A live plugin-search screenshot after a clean app restart is still useful, but this is no longer an obvious open implementation item.

### Mixer and Splitter Consolidation

Status: done at source/test-contract level; needs current visual confirmation.

Evidence:
- Recent commits merged the dynamic Mixer/Splitter paths and corrected orientation/pin follow-up issues.
- `tests/mixer_splitter_test.cpp` covers DSP/state behavior.
- `tests/ui_regression_harness_test.cpp` covers visible routing Mixer/Splitter polish, compact host pin labels, direct-painted embedding, and retention of controls.

Residual risk:
- The exact live horizontal/vertical strip presentation should be rechecked in screenshots because this area had repeated wording/interpretation mismatch.

### Tone Generator Chrome and Layout

Status: done at source-contract level; needs current visual confirmation.

Evidence:
- `tests/ui_regression_harness_test.cpp` checks the Tone Generator size, output pin footprint, theme-derived colour helpers, removal of hardcoded black/white/hex colours, removal of the `READY` pill, output rotary knob, text-box sizing, visible sliders, waveform glyph, display/pitch/bottom panel sizing, and removal of the old clipped wave labels.

Residual risk:
- This was a visually sensitive node. A fresh node snapshot is still required before calling it visually final.

### NAM Loader Theme/Text Roles

Status: done at source-contract level.

Evidence:
- `tests/ui_regression_harness_test.cpp` checks the NAM Loader uses separate host text and dark-panel text roles: `ampHostTextBright`, `ampPanelTextBright`, panel-dim text, and embedded graph-node text role selection.
- The latest HEAD specifically splits loader text roles by surface.

Residual risk:
- Needs screenshots in light/daylight, dark, and synthwave because the failure was black/dark or low-contrast text in real rendering.

### IR Loader Theme/Text Roles

Status: done at source-contract level.

Evidence:
- `tests/ui_regression_harness_test.cpp` checks `IRLoaderControl.cpp` has separate `hostText`, `hostTextDim`, `panelText`, and `panelTextDim` roles, derives panel text from the inset surface, applies panel text to slider text boxes, and rejects prior hardcoded text patterns.

Residual risk:
- Same as NAM Loader: the code contract is green, but the visual pass needs current screenshots.

### NAM A1/A2 UI Labelling

Status: done at source/test-contract level; online behavior still needs manual/API proof.

Evidence:
- `tests/nam_processor_test.cpp` covers TONE3000 architecture helper mapping, architecture-specific requests/cache keys, online browser architecture labels, local browser architecture labels, preview badge repainting, loaded model architecture badge, A2 runtime routing, and A2 UI/state boundaries.
- Focused `[nam][a2][ui]` test run passed.

Residual risk:
- `TESTLATER.md` still correctly calls for a manual online/local smoke pass: authenticated download visibility, A1/A2 filter sanity, downloaded local pill correctness, and scale/clipping checks.

### Effect Rack, Notes, MIDI Labels, Tuner, NAM Browser Chrome

Status: mostly done at source-contract level; needs current visual proof.

Evidence:
- `tests/ui_regression_harness_test.cpp` covers Effect Rack nested graph chrome, utility/node polish guardrails, MIDI source labels and bottom keyboard hint, NAM/IR browser chrome centering contracts, and Notes/Label polish contracts.
- The focused `[ui][regression][visual]` test run passed.

Residual risk:
- The audit has not yet generated live screenshots after the latest fixes. This matters for small alignment problems such as browser chrome centering, clipped list rows, tuner pill rendering, and detail-card text wrapping.

## Needs Proof, Not New Implementation Yet

These should not be treated as open bugs until the current build is visually checked:

1. Current node snapshots after the June 21 commits.
2. NAM and IR browser dialog matrix at 150% and 200% scale, including local/online/IR tabs.
3. NAM online authenticated flow: visible download action, A1/A2 filter sanity, and downloaded-model local architecture pill.
4. ReverbSC live interaction: dragging controls does not move the node, values update immediately, save/reload restores values, bypass/mappings remain readable.
5. Mixer and Splitter live layout/orientation, especially because this area had repeated interpretation churn.
6. Theme sweep for dark, daylight/light, and synthwave after the loader text-role changes.
7. Scratch Capture hardware smoke remains manual/hardware-gated.

## Still Open Work

### Shared Direct-Painted Node Shell

Status: open architecture cleanup.

The current code has better helper contracts than before, but it is still mostly name-driven host logic plus per-node direct-painted controls. A reusable direct-painted node shell abstraction has not been completed. This is the real long-term fix for the repeated padding, footer, host button, nested-shell, pin-label, and drag-suppression workarounds.

Recommended direction:
- Define a small direct-node contract owned by processors/controls rather than scattered plugin-name checks.
- Centralize shell sizing, footer affordance policy, pin-label policy, drag suppression, host padding, and theme role handoff.
- Keep NAM/IR hero chassis nodes as their own category if they do not fit the direct-node shell.

### Roadmap P1/P2 Polish

Status: open, but not part of the old bug list.

`ROADMAP.md` no longer frames broad "premium polish" as undefined P0. Remaining polish should be handled as concrete P1/P2 work:
- State feedback and focused motion.
- Secondary surface alignment.
- Connection and bypass signal cues.
- Internal editor consistency rollout.
- SVG/icon and visual asset pass.
- CPU meter redesign.

## Recommended Next Audit Steps

1. Generate current node snapshots:

   ```powershell
   .\scripts\run_d2_visual_qa.ps1 -Configuration Release -OutputName 2026-06-21-polish-audit-nodes -NodeSnapshotsOnly
   ```

2. Generate current scaled browser/dialog screenshots:

   ```powershell
   .\scripts\run_d2_visual_qa.ps1 -Configuration Release -OutputName 2026-06-21-polish-audit-dialogs -CaptureScaledDialogMatrix
   ```

3. Run the manual `TESTLATER.md` smoke passes that depend on authenticated online state, local model files, user interaction, or hardware.

4. Only convert screenshot/manual failures into implementation tasks. Do not reopen broad polish buckets or already contract-covered issues without new evidence.
