# Pedalboard Remix UI Upgrade Plan

Date: 2026-06-09
Status: Active
Branch policy: direct local branch/main workflow; do not create pull requests unless explicitly requested.

## Purpose

Implement the best parts of the Pedalboard 3 mockup as native Pedalboard UI polish without replacing existing working UX.

The mockup is an upgrade reference, not a new product shell. Existing controls, routing behavior, patch workflows, stage safety controls, recorder behavior, and recovery paths must remain available.

## Source Material

- Design handoff: `F:/Downloads/pedalboard (Remix)-handoff.zip`
- Extracted copy: `releases/design-handoffs/pedalboard-remix/pedalboard-remix`
- Primary open file: `project/Pedalboard 3 Demo.html`
- Relevant mockup files inspected:
  - `project/demo-app.jsx`
  - `project/mw2-app.jsx`
  - `project/mw2-chrome.jsx`
  - `project/mw2-nodes.jsx`
  - `project/mw2.css`
  - `project/scratch-panel.jsx`
  - `project/scratch.css`
  - `project/StageMode.jsx`
  - `project/StageHud.jsx`
  - `project/StageSetlist.jsx`
  - `project/StageGrid.jsx`
  - `project/stage-shared.jsx`
  - `project/stage.css`

## Reuse And Attribution

- Reuse mode: pattern-only / clean-room native JUCE implementation.
- License status: no license file found in the handoff.
- Prototype source and assets must not be copied into the application.
- Acceptable reuse: layout hierarchy, interaction ideas, visual relationships, terminology, and behavior lessons.
- Required implementation target: existing C++/JUCE UI architecture.

## Non-Goals

- Do not treat `TESTLATER.md` as the next implementation queue. It is a deferred manual QA parking lot.
- Do not remove or hide existing UX functions while matching the mockup.
- Do not replace the graph, stage mode, or scratch recorder with a prototype-style rewrite.
- Do not add novelty features merely because the mockup leaves open space.
- Do not open PRs as part of normal workflow.

## Completed Mockup-Informed Work

- Scratch Capture initial polish:
  - Larger record-focused scratch surface.
  - Recent take metadata including dates.
  - RAW/WET capture context represented in UI.
  - Scratch destination chooser added.
  - Play/reveal clipping fixed.
- Scratch Capture second polish pass:
  - Footer and panel status now share one formatter for Ready, Recording, Saving, Saved, and Failed states.
  - Recording status exposes elapsed time plus RAW/WET channel context.
  - Footer REC button shows STOP while recording, SAVE while saving, and disables during the saving state.
  - Full scratch status is available as a footer tooltip when scaled layouts wrap or clip.
  - Scaled footer visual QA recorded in `documentation/qa/2026-06-09-scratch-status-polish`.
- Footer/UI scale recovery:
  - In-app UI scale control added to settings/footer surface.
  - Default scale starts at 75 percent.
  - Reset-to-default path added.
  - Footer layout improved for larger UI scale.
- Stage Mode first polish pass:
  - Native live strip added with current performance mode and clock hierarchy.
  - Current patch, position, next/end cue, and progress dots now match the mockup's stronger stage hierarchy.
  - Tuner and safety-bar surfaces were visually upgraded while preserving tuner, exit, prev/next, meters, gain sliders, and panic controls.
  - Visual QA recorded in `documentation/qa/2026-06-09-stage-remix-polish`.
- Main Graph first polish pass:
  - Native graph canvas now has clearer major/minor grid hierarchy.
  - Cable rendering gained a stable low-alpha bed and wider clipping bounds for readability in dense routing.
  - Audio and parameter pins now have distinct shapes while preserving the existing drag/connect hit targets.
  - Nodes gained mockup-informed accent strips plus hover, drag, and bypass cues without moving edit, mapping, bypass, delete, or plugin editor controls.
  - Visual QA recorded in `documentation/qa/2026-06-09-main-graph-remix-polish`.
- Roadmap and research:
  - Non-AI feature research recorded.
  - Scratch capture direction selected: manual, fast capture, using existing recorder backbones, capturing raw and wet simultaneously.

## Active Track

### 1. Stage Mode Mockup Polish

Goal: bring the Stage mockup's stronger performance hierarchy into the native stage view while preserving all existing live safety and navigation affordances.

Status: First native polish pass complete. Further Stage work should only add real setlist/grid switching if backed by the app's actual patch/setlist model; do not add decorative inactive mockup tabs.

Tasks:

- Audit current native stage components against `StageMode.jsx`, `StageHud.jsx`, `StageSetlist.jsx`, `StageGrid.jsx`, and `stage.css`.
- Preserve current working controls before adding visual polish:
  - Fullscreen/performance view.
  - Patch navigation.
  - Tuner and meter visibility.
  - Panic/safety controls.
  - Setlist/current/next context.
- Add the useful mockup concepts:
  - Clearer top performance strip.
  - Stronger current patch and next patch hierarchy.
  - More legible safety/status bar.
  - View switching that feels deliberate, not decorative.
  - Better scan rhythm for stage tiles and setlist entries.
- Verify across normal and scaled UI sizes.

Done when:

- Existing stage workflows still work.
- No critical controls clip at common UI scales.
- The stage view looks meaningfully closer to the mockup's density, contrast, and hierarchy.
- A visual QA note or screenshot set records the result.

### 2. Main Graph And Chrome Polish

Goal: improve the main pedalboard surface using the mockup's visual language while keeping the existing graph UX intact.

Status: First native polish pass complete. Further graph/chrome work should focus on real remaining usability gaps, not prototype-only decoration.

Tasks:

- Audit native main graph/chrome against `mw2-app.jsx`, `mw2-chrome.jsx`, `mw2-nodes.jsx`, and `mw2.css`.
- Preserve existing functions:
  - Add/edit/remove devices.
  - Routing and cable interaction.
  - Footer controls and recovery controls.
  - Settings/options access.
  - Scratch and stage access.
- Add the useful mockup concepts:
  - Clearer footer priority model at constrained widths.
  - Stronger node state affordances.
  - Better pin labeling and cable readability.
  - More intentional active/selected/bypassed visual states.
  - Refined chrome spacing and contrast.
- Keep all UI scale recovery paths reachable.

Done when:

- Footer controls remain reachable above 100 percent scale.
- Graph interaction remains unchanged or improved.
- Node/cable state is easier to read at a glance.
- No existing workflow is displaced by visual polish.

### 3. Scratch Capture Second Pass

Goal: finish the scratch workflow as the user's "plug in, start Pedalboard, record an idea within moments" path.

Status: Second native polish pass complete for status clarity and scaled footer visibility. Further scratch work should be driven by manual capture friction, not mockup-only decoration.

Tasks:

- Audit current scratch UI against `scratch-panel.jsx` and `scratch.css`.
- Preserve existing recorder backbones and simultaneous RAW/WET capture direction.
- Improve:
  - One-action readiness.
  - Destination clarity.
  - Recent take metadata density.
  - Raw/wet reamp affordance.
  - Empty, armed, recording, saved, and error states.

Done when:

- A user can identify destination, arm/record, stop, and locate the take with minimal UI travel.
- Raw and wet capture status is visible without reading documentation.
- Scratch takes remain date-stamped and recoverable.

### 4. Secondary Mockup Harvest

Goal: only after stage, graph, and scratch are solid, review the remaining mockup files for targeted polish ideas.

Candidate areas:

- NAM browser and loader polish if they improve existing browsing/loading workflows.
- Shared typography and spacing cleanup.
- Motion/transition tuning where it clarifies state.

Constraints:

- Skip anything that adds surface area without a clear user workflow.
- Skip features that are only attractive in the mockup but not useful in the native app.

## Verification

Minimum checks for implementation slices:

- Build the native app.
- Run focused existing tests touched by the change.
- Perform visual QA at 75 percent, 100 percent, and above 100 percent UI scale.
- Confirm options/settings still expose UI scale and reset-to-default.
- Confirm `TESTLATER.md` only receives deferred manual QA notes, not active implementation tasks.

## Immediate Next Step

Start with Secondary Mockup Harvest:

1. Review the remaining mockup files for high-signal native polish candidates.
2. Reject anything that adds surface area without a real existing workflow.
3. Prefer small improvements to existing browser, loader, typography, spacing, and state feedback surfaces.
4. Preserve all existing UX functions and keep implementation clean-room/pattern-only.
