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
- Initial committed visual QA artifacts under `documentation/qa` were from 2026-06-16 and predated the latest June 21 NAM/IR Loader text-role fixes.
- Fresh visual QA artifacts were generated during this audit:
  - `documentation/qa/2026-06-21-polish-audit-nodes`
  - `documentation/qa/2026-06-21-polish-audit-dialogs`
- Visual acceptance note: the fixed-size 200% dialog captures are stress cases, not a realistic use case at those small window dimensions. Treat them as graceful-degradation evidence only, not product-blocking bugs.
- Focused Release tests run during this audit:
  - `.\\build\\tests\\Release\\Pedalboard3_Tests.exe "[ui][regression][visual]"` passed: 837 assertions in 16 test cases.
  - `.\\build\\tests\\Release\\Pedalboard3_Tests.exe "[reverbsc]"` passed: 304 assertions in 17 test cases.
  - `.\\build\\tests\\Release\\Pedalboard3_Tests.exe "[nam][a2][ui]"` passed: 63 assertions in 4 test cases.

## Fresh Visual QA Results

### Looks Current-Good

- NAM Loader node: current snapshot shows the prior black/dark and single-text-role issue is fixed. Host text and dark panel text now separate cleanly.
- IR Loader node: current snapshot shows the same text-role fix is applied; dark slot cards and outer shell labels are readable.
- NAM Library normal-size dialog: chrome is centered, local rows show A1/A2 pills, detail text is readable, and the selected model panel no longer has the original pill rendering failure.
- IR Browser normal-size dialog: local detail card is readable and visually aligned.
- Mixer/Splitter nodes: current snapshots show the consolidated dynamic UI is usable and no duplicate DAW/internal variants are visible in the captured plugin search list.
- Effect Rack node: current snapshot has one clear `Open` affordance and no redundant `Map` button.
- Notes node: empty hint is present again in the captured snapshot.

### Fixed After Follow-Up

- Tone Generator node: `documentation/qa/2026-06-21-tone-tuner-polish/app-node-tone-generator.png` shows the pitch row no longer has the prior normal-size overlap. The node was widened slightly, pitch-row values were moved into direct-painted chips, and hidden slider hit targets now sit over the rails instead of painting text boxes over the row.
- Tuner node: `documentation/qa/2026-06-21-tone-tuner-polish/app-node-tuner.png` shows the mode row is now an integrated segmented control with clearer separation and a roomier bypass pill.

### Still Open From Current Evidence

- No current normal-size node snapshot failure remains from the Tone Generator/Tuner items. The remaining checks are interaction-level/manual, not obvious rendering defects.

### Stress-Only / Do Not Over-Prioritize

- NAM/IR Browser at 200% in the fixed QA capture sizes clips detail content. That resolution/window combination is not a realistic acceptance target. Keep it as stress evidence only.
- At 150%, the NAM detail card starts to lose lower metadata in the scaled matrix. This is a lower-priority responsive/compact-mode issue unless it appears in a realistic user window.

## Done or Contract-Covered

### ReverbSC Direct Node

Status: done at source/test-contract level; needs current screenshot/manual confirmation.

Evidence:
- `tests/reverbsc_processor_test.cpp` covers stable metadata, compact stereo graph pins, embedded controls for every parameter, suppression of redundant editor/param-pin affordances, no generic host bottom padding, no nested node shell, no duplicate title chrome, polished direct-surface primitives, compact footer spacing, state round-trip, DSP impulse behavior, and internal plugin registration.
- `tests/ui_regression_harness_test.cpp` also checks ReverbSC theme-colour use and rejects hardcoded `Colours::white`, `Colours::black`, and `Colour(0x...)`.

Residual risk:
- Current screenshots are now available. The remaining ReverbSC gap is manual interaction: drag controls, save/reload, bypass/mappings readability, and user-facing feel.

### Internal Plugin Duplicate Prevention

Status: done at source-contract level.

Evidence:
- `tests/ui_regression_harness_test.cpp` includes `Internal plugin descriptions are runtime catalog entries, not persisted scan results`.
- That contract verifies runtime internal plugin XML is removed from persisted plugin-list XML before restore/save, which addresses the duplicate internal-node list failure at the source.

Residual risk:
- Current plugin-search capture shows no obvious duplicate internal plugin entries in the visible list.

### Mixer and Splitter Consolidation

Status: done at source/test-contract level; needs current visual confirmation.

Evidence:
- Recent commits merged the dynamic Mixer/Splitter paths and corrected orientation/pin follow-up issues.
- `tests/mixer_splitter_test.cpp` covers DSP/state behavior.
- `tests/ui_regression_harness_test.cpp` covers visible routing Mixer/Splitter polish, compact host pin labels, direct-painted embedding, and retention of controls.

Residual risk:
- The node snapshots look usable, but manual confirmation is still useful for horizontal/vertical interaction because this area had repeated wording/interpretation mismatch.

### Tone Generator Chrome and Layout

Status: done at source-contract level; needs current visual confirmation.

Evidence:
- `tests/ui_regression_harness_test.cpp` checks the Tone Generator size, output pin footprint, theme-derived colour helpers, removal of hardcoded black/white/hex colours, removal of the `READY` pill, output rotary knob, text-box sizing, visible sliders, waveform glyph, display/pitch/bottom panel sizing, and removal of the old clipped wave labels.

Residual risk:
- The follow-up snapshot in `documentation/qa/2026-06-21-tone-tuner-polish` shows the pitch-row clipping fixed. Manual interaction remains useful because the row uses direct-painted chips with hidden slider hit targets.

### NAM Loader Theme/Text Roles

Status: done at source-contract level.

Evidence:
- `tests/ui_regression_harness_test.cpp` checks the NAM Loader uses separate host text and dark-panel text roles: `ampHostTextBright`, `ampPanelTextBright`, panel-dim text, and embedded graph-node text role selection.
- The latest HEAD specifically splits loader text roles by surface.

Residual risk:
- Fresh node and dialog screenshots show the text-role issue is materially improved. Keep theme sweep as release QA, not as an open implementation claim.

### IR Loader Theme/Text Roles

Status: done at source-contract level.

Evidence:
- `tests/ui_regression_harness_test.cpp` checks `IRLoaderControl.cpp` has separate `hostText`, `hostTextDim`, `panelText`, and `panelTextDim` roles, derives panel text from the inset surface, applies panel text to slider text boxes, and rejects prior hardcoded text patterns.

Residual risk:
- Fresh node and dialog screenshots show the text-role issue is materially improved. Keep theme sweep as release QA, not as an open implementation claim.

### NAM A1/A2 UI Labelling

Status: done at source/test-contract level; online behavior still needs manual/API proof.

Evidence:
- `tests/nam_processor_test.cpp` covers TONE3000 architecture helper mapping, architecture-specific requests/cache keys, online browser architecture labels, local browser architecture labels, preview badge repainting, loaded model architecture badge, A2 runtime routing, and A2 UI/state boundaries.
- Focused `[nam][a2][ui]` test run passed.

Residual risk:
- `TESTLATER.md` still correctly calls for a manual online/local smoke pass: authenticated download visibility, A1/A2 filter sanity, downloaded local pill correctness, and downloaded-model local architecture pill.

### Effect Rack, Notes, MIDI Labels, Tuner, NAM Browser Chrome

Status: mostly done at source-contract level; needs current visual proof.

Evidence:
- `tests/ui_regression_harness_test.cpp` covers Effect Rack nested graph chrome, utility/node polish guardrails, MIDI source labels and bottom keyboard hint, NAM/IR browser chrome centering contracts, and Notes/Label polish contracts.
- The focused `[ui][regression][visual]` test run passed.

Residual risk:
- Current screenshots now cover browser chrome centering and common dialog rendering. The follow-up node snapshot shows the Tuner mode row polished. Extreme 200% fixed-size clipping should not be treated as a realistic blocker.

## Needs Proof, Not New Implementation Yet

These should not be treated as open bugs until the current build is checked in realistic conditions:

1. NAM online authenticated flow: visible download action, A1/A2 filter sanity, and downloaded-model local architecture pill.
2. ReverbSC live interaction: dragging controls does not move the node, values update immediately, save/reload restores values, bypass/mappings remain readable.
3. Mixer and Splitter live layout/orientation, especially because this area had repeated wording/interpretation churn.
4. Theme sweep for dark, daylight/light, and synthwave remains release QA, though the latest node/dialog screenshots are encouraging.
5. Scratch Capture hardware smoke remains manual/hardware-gated.
6. High-scale browser/dialog behavior should be tested in realistic high-DPI window sizes, not the cramped fixed 200% stress captures.

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

1. Run the manual `TESTLATER.md` smoke passes that depend on authenticated online state, local model files, user interaction, or hardware.
2. If high-scale dialog polish is revisited, define realistic high-DPI window sizes first. Do not use the fixed 200% stress captures as a product-blocking acceptance target.
3. Only convert screenshot/manual failures into implementation tasks. Do not reopen broad polish buckets or already contract-covered issues without new evidence.
