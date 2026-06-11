# Theme And NAM Polish Design

## Goal

Bring the Daylight, Synthwave, and Forest themes closer to the actual Pedalboard 3 mockup while raising the NAM Loader editor and TONE3000 online tab to the same visual standard as the recent browser, Stage, and graph polish.

## References Inspected

- Mockup token source: `releases/design-handoffs/pedalboard-remix/pedalboard-remix/project/main.css`
- Mockup graph/NAM node source: `releases/design-handoffs/pedalboard-remix/pedalboard-remix/project/mw2.css`, `mw2-nodes.jsx`, `mw2-data.jsx`
- Native theme source: `src/ColourScheme.cpp`, `src/ThemeSwitcherComponent.cpp`
- Native NAM loader source: `src/NAMControl.cpp`, `src/NAMControl.h`
- Native online tab source: `src/NAMOnlineBrowser.cpp`, `src/NAMOnlineBrowser.h`
- Existing verification gates: `tests/ui_regression_harness_test.cpp`, `tests/nam_processor_test.cpp`

Reuse mode: pattern-only / clean-room. The mockup supplies exact colour-token targets and component hierarchy lessons. No React, CSS, font, or asset code is copied into the JUCE implementation.

## Scope

### In Scope

- Update built-in Daylight, Synthwave, and Forest token values where the current app diverges from the mockup palette.
- Update matching theme switcher swatches if token changes affect visible swatch identity.
- Improve the NAM Loader control surface with stronger amp-node hierarchy: category-accent header, better model state treatment, more premium section panels, and clearer Load/Browse/Clear/IR/Gain/Tone affordances.
- Improve the TONE3000 online tab with a mockup-aligned browser chassis: richer result rows, selected/hover states, status chips, search/filter surface polish, details panel polish, and button treatment.
- Preserve every existing NAM Loader and online-tab workflow.

### Out Of Scope

- No new online features, account-flow changes, search API changes, download behavior changes, or cache behavior changes.
- No DSP, parameter, state-serialization, plugin-pool, or Tone3000 client changes.
- No full redesign of the local NAM browser in this slice.
- No new font/assets.

## Design Decisions

### Theme Tokens

Use the mockup token values for the three requested themes as the source of truth:

- Daylight should become a clearer studio-light palette: `bg-top #eef0f2`, `bg-bottom #dadde1`, `panel #cfd3d8`, `plugin-bg #eaecee`, `border #c2c6cc`, `text #1a1f24`, `button #e4e6e9`, `button-hi #f3f4f6`, `tuner #0a8f5a`.
- Synthwave should stop using pure magenta as body text. Body text becomes readable lavender `#f4e6ff`; magenta stays as accent `#ff2bff`; border becomes `#5a2d82`; CPU/tuner/success use neon green `#00ff88`.
- Forest should preserve the current green identity but match the mockup’s brighter readable text `#dcecd2`, warm parameter/warning `#ccaa44`, and stable green success `#44bb44`.

### NAM Loader

The NAM loader should feel like the native equivalent of the mockup’s amp-category node/editor, not a generic JUCE panel. It keeps the current compact editor footprint and all controls. Polish focuses on:

- Header: stronger amp-category accent and readable loaded/empty model state.
- Signal Chain: model/IR/IR2 rows read as connected chain modules, with model and IR labels acting like inset chips.
- Gain/Tone: clearer section separation, improved slider/knob contrast, and less flat button rendering.
- Theme behavior: all colors derive from semantic tokens, with no hardcoded `juce::Colours::white` or similar forbidden token-audit patterns in audited files.

### NAM Online Tab

The online tab should match the newer browser standard without changing the TONE3000 workflow:

- Search and filters live in a clear toolbar with token-derived field and button treatments.
- Result rows become denser cards with an accent rail, gear badge, cache/download status, title, author, and size/progress.
- Details panel gets stronger hierarchy and empty/selection states.
- Download, Load, Login, Logout, and paging buttons remain reachable and keep their current actions.

## Verification

- `git diff --check`
- `.\build\tests\Release\Pedalboard3_Tests.exe "[ui][regression][theme]"`
- `.\build\tests\Release\Pedalboard3_Tests.exe "[ui][regression]"`
- `.\build\tests\Release\Pedalboard3_Tests.exe "[nam]"`
- Release build target for `Pedalboard3_Tests`
- Release build target for `Pedalboard3`
- Visual QA capture for Daylight, Synthwave, and Forest main/NAM browser surfaces if the harness is not blocked by the existing Scratch capture hang.

## Risks

- Existing saved custom themes may keep old values; this slice updates built-in presets only.
- TONE3000 online behavior may require network/auth to fully exercise manually, so automated verification should focus on paint/layout and existing regression gates.
- NAM Loader is compact; visual polish must avoid clipping at small node/editor sizes.
