# Stage Mode Shared Chrome Polish

## Status

Approved direction: implement Stage Mode polish in three slices:

1. Shared Stage chrome.
2. Grid and setlist polish.
3. Tuner overlay polish.

This spec covers slice 1 only.

## Source References

- Existing native code: `src/StageView.cpp`, `src/StageView.h`, `src/StageLayout.cpp`, `src/StageLayout.h`, `tests/stage_layout_test.cpp`.
- Existing upgrade spec: `docs/superpowers/specs/2026-06-09-pedalboard-remix-ui-upgrade.md`.
- Existing plan notes: `docs/superpowers/plans/2026-06-09-pedalboard-remix-ui-upgrade.md`.
- Mockup reference: `releases/design-handoffs/pedalboard-remix/pedalboard-remix/project/Pedalboard 3 Demo.html`.
- Mockup Stage files inspected: `StageMode.jsx`, `StageGrid.jsx`, `stage.css`.
- Reuse mode: pattern-only / clean-room. No prototype source, assets, fonts, or CSS are copied into the JUCE app.
- License note: the design handoff has no license file, so it remains a visual and behavioral reference only.

## Goal

Make Stage Mode's shared shell feel closer to the mockup and more performance-ready without changing the user's existing live workflow. The first slice should improve the always-visible chrome that wraps every Stage view: top bar, view switcher, theme access, safety bar, meters, and panic affordance.

## Non-Goals

- Do not redesign individual Grid, Setlist, Hero, or Tuner content in this slice.
- Do not remove any existing Stage controls.
- Do not replace the native JUCE implementation with mockup code.
- Do not add new audio behavior, tuner muting behavior, setlist storage, or new patch-management semantics.
- Do not make Stage Mode dependent on browser/React assets.

## Current Behavior To Preserve

- `F11` toggles Stage Mode through `MainPanel`.
- `EXIT` leaves Stage Mode.
- `<< PREV` and `NEXT >>` switch patches.
- `HERO`, `SETLIST`, `GRID`, and `TUNE` view buttons remain available.
- `TUNER` toggle remains available.
- Theme switching remains available when the viewport has enough room.
- Input/output master gain sliders remain live and synchronized with `MasterGainState`.
- IN/OUT meters and peak hold continue rendering in the footer.
- `PANIC` continues routing through the existing all-notes-off path.
- Existing keyboard navigation stays intact.

## Design

### Shared Top Bar

The top bar should read as a single intentional control surface instead of separate generic buttons.

- Keep the left `STAGE MODE` live indicator, but make it more like the mockup's live brand: small glowing live dot, clear uppercase label, restrained text alpha.
- Keep the clock on the right.
- Keep `EXIT` and `TUNER` in the right utility cluster.
- Keep the theme switcher visible when there is room, but visually align it with the top bar rather than letting it feel like a stray app control.
- Keep all header geometry responsive through `StageLayout` metrics or local metrics derived from those values.

### View Switcher

The view switcher should feel like the mockup's segmented control while preserving native button behavior.

- Keep the existing four mode buttons and radio group.
- Polish the rail behind the buttons: rounded shared container, subtle panel fill, border, and active mode emphasis.
- Buttons should have clearer active state, calmer inactive state, and less generic beveling.
- Labels stay text-first for now: `HERO`, `SETLIST`, `GRID`, `TUNE`. Icon drawing can be a later slice if it requires new shared primitives.

### Safety Bar

The footer should feel like a live safety strip, not a regular app footer.

- Keep IN/OUT meters, panic, and master gain sliders.
- Improve grouping: the safety label, master bus label, meter groups, and panic glow should align better with footer geometry.
- Panic should remain visually dominant but not consume layout space at smaller widths.
- The tuner strip behavior remains controlled by existing view/tuner state. This slice should not introduce the mockup's full tuner overlay.

### Theme Consistency

All polish should use existing `ColourScheme` roles and `FontManager` semantic fonts.

- No hardcoded palette literals in Stage source.
- No copied CSS color tokens.
- Theme-specific complementary colors should come from existing roles such as accent, warning, tuner, panel, border, text, and danger.

## Implementation Shape

Prefer a narrow patch in `StageView.cpp`, with `StageLayout` changes only if geometry needs executable regression coverage.

Likely code areas:

- `StageButtonLookAndFeel::drawButtonBackground()`
- `StageButtonLookAndFeel::drawButtonText()`
- `StageView::drawStatusBar()`
- `StageView::drawSafetyBar()`
- `StageView::resized()`
- `StageView::paint()` only if footer or header background treatment needs shared visual context

## Testing

Automated:

- Add or update `tests/stage_layout_test.cpp` if the slice changes layout metrics.
- Run `Pedalboard3_Tests.exe "[stage][layout]"`.
- Run `Pedalboard3_Tests.exe "[ui][regression]"`.
- Run `Pedalboard3_Tests.exe "[ui][visual]"`.
- Build `Pedalboard3` Release after tests.

Visual:

- Capture Stage Hero, Setlist, Grid, and Tune states after the patch.
- If the full visual harness hits the known `CopyFromScreen`/Scratch-panel issue, use targeted Stage screenshots and record the harness limitation in the implementation summary.

## Acceptance Criteria

- Shared Stage chrome is visibly closer to the mockup's top-bar/view-switcher/safety-bar language.
- No existing Stage control disappears or changes behavior.
- The four Stage view modes still render.
- Footer controls remain reachable at the normal QA viewport.
- Stage layout tests and UI regression/visual tests pass.
- Commit summary records pattern-only mockup use and files inspected.
