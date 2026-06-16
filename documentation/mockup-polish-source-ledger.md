# Mockup Polish Source Ledger

Last updated: 2026-06-14

This ledger is the guardrail for the active Main Window v2 polish sprint. Future UI changes in this sprint should start here, cite the mockup source selector or component, and preserve existing Pedalboard behavior unless the user explicitly approves removal.

## Source Of Truth

- Claude Design target: `Main Window v2.html`
- Extracted source root: `documentation/mockup-main-window-v2-lXKi/pedalboard-remix/project/`
- Primary files:
  - `mw2-nodes.jsx`
  - `mw2-prims.jsx`
  - `mw2.css`
  - `nam.css`
  - `nam-browser.css`
  - `nam-browser.jsx`
- Reuse mode: pattern-only visual translation into existing JUCE components.
- Feature rule: the mockup is a design reference, not a product feature ceiling. Preserve all existing app function that the mockup omits.

## Current Status

- NAM Loader: accepted as done by user. Do not keep reworking without a new direct request.
- IR Loader: accepted as done by user. Do not keep reworking without a new direct request.
- Main graph grid: source contract now requires mockup graphpaper behavior, meaning uniform graphpaper without heavy major grid lines.
- Tuner node: source contract now requires direct-in-node paint, internal bypass, and `Needle` / `Strobe` / visible `Poly` mode while preserving the existing six-string implementation.
- NAM/IR library close button: source contract now requires a simple transparent 16 px rounded-stroke close mark, not the older boxed/stylized close button.
- NAM online browser: source contract now requires narrower online search width and browser-palette action styling for Search, Download, and Load.

## Verified Source Contracts

These checks are guardrails, not final visual proof. They exist to keep completed source-backed decisions from being reintroduced accidentally.

- `tests/ui_regression_harness_test.cpp`, `[ui][regression][theme][tokens]`
  - Daylight uses layered off-white surfaces and forbids pure white primary UI roles.
- `tests/ui_regression_harness_test.cpp`, `[ui][regression][visual][source][graph]`
  - Main graph grid uses 24 px uniform lines/dots and forbids `majorGridSize`, `majorGridCol`, and `firstMajorX`.
- `tests/ui_regression_harness_test.cpp`, `[ui][regression][visual][source][nodes][routing]`
  - Mixer and splitter visible controls stay directly hosted in the node and forbid `paintRoutingNodeShell`.
- `tests/ui_regression_harness_test.cpp`, `[ui][regression][visual][source][subgraph]`
  - Effect Rack uses the normal host chrome plus a mock subgraph preview and forbids the removed rack shell.
- `tests/ui_regression_harness_test.cpp`, `[ui][regression][visual][source][library]`
  - NAM/IR library and online browser keep favorites, IR folder setting, scrollable details, narrowed online search, shared browser palette action buttons, and plain close mark.
- `tests/ui_regression_harness_test.cpp`, `[ui][regression][visual][source][nodes][tuner]`
  - Tuner keeps the mockup mode schema and visible `POLY` label without renaming or removing the product's six-string behavior.

## Remaining Source-Backed Surfaces

### Effect Rack Node

- Mockup source:
  - `mw2-nodes.jsx`, case `"effect-rack"`
  - `mw2.css`, `.m2-node.t-rack`, `.rack-graph-wrap`, `.rack-badge`, `.rack-foot`, `.rack-open`
- Intended visual anatomy:
  - Host node body contains a single subgraph preview area.
  - Preview area has dot-grid texture, `SUB-GRAPH` badge, unlabeled or lightly labeled nested processor marks.
  - Footer has nested processor count and an `Open` action.
- Preserve app function:
  - Existing subgraph editor/open workflow.
  - Existing bypass and mapping behavior.
  - Existing nested graph processor count, but visual preview does not need to be live or semantically exact.
- Avoid:
  - A second full node shell inside the host node.
  - Rails or rack hardware that fights the mockup topology.
  - Fixed effect-slot labels that imply a non-generic rack.

### Mixer Node

- Mockup source:
  - `mw2-nodes.jsx`, case `"mixer"`
  - `mw2.css`, `.m2-node.t-mixer`, `.mix-container`, `.mix-strip`, `.mix-fader-area`, `.mix-vu`, `.mix-fader-fill`, `.mix-mute`
- Intended visual anatomy:
  - Generic numbered strips plus master strip.
  - Pan rail, VU-backed fader, dB readout, mute control per strip.
  - No semantic channel names like NAM, DI, Verb.
- Preserve app function:
  - Existing fader, pan, mute, solo, phase, state save/load, and audio routing behavior.
  - Existing channel count and parameter semantics.
- Avoid:
  - Nested node shell/chassis inside the host node.
  - Removing solo or phase merely because the mockup omits them. If space is tight, treat them as compact controls.

### Splitter Node

- Mockup source:
  - `mw2-nodes.jsx`, case `"splitter"`
  - `mw2.css`, `.m2-node.t-splitter`, `.spl-in-row`, `.spl-fan`, `.spl-outs`, `.spl-out-row`
- Intended visual anatomy:
  - Input badge and meter.
  - Fanout curve.
  - Numbered output rows with meters and dB/value text.
- Preserve app function:
  - Existing mute toggles and routing behavior.
  - Existing dynamic state serialization.
- Avoid:
  - Nested node shell/chassis inside the host node.
  - Semantic output labels not present in the product model.

### Tuner Node

- Mockup source:
  - `mw2-nodes.jsx`, case `"tuner"`
  - `mw2-prims.jsx`, `M2Tuner`, `TunerNeedle`, `TunerPoly`
  - `mw2.css`, `.t-tuner`, `.m2-tuner-v2`, `.tn-modeseg`, `.tn-needle-view`, `.tn-poly`
- Intended visual anatomy:
  - Painted directly inside the node, not as a parameter node.
  - Mode segmented control: `Needle`, `Strobe`, `Poly`.
  - Internal bypass belongs inside tuner UI, not as a host footer `B` button.
  - Poly mode is the six-string view.
- Preserve app function:
  - Existing tuning engine and six-string behavior.
  - Existing reference pitch and tuner status behavior.
- Avoid:
  - Restoring the host parameter node or footer-only bypass.
  - Renaming the product behavior away from six-string internally; `Poly` is a visible label only.

### Note Node

- Mockup source:
  - `mw2-nodes.jsx`, case `"note"`
  - `mw2.css`, `.m2-node.t-note`, `.note-body`
- Intended visual anatomy:
  - Bespoke sticky-note construction.
  - Warm cream paper, amber header, subtle rotation.
  - No M/E/B plugin controls.
- Preserve app function:
  - Existing note editing and persistence.
- Avoid:
  - Generic plugin footer controls.
  - Treating notes as a normal plugin node.

### NAM Library And Online Browser

- Mockup source:
  - `nam-browser.jsx`
  - `nam-browser.css`, `.nb-head`, `.nb-tabs`, `.nb-toolbar`, `.nb-search`, `.nb-body`, `.nb-list`, `.nb-preview`, `.nb-audition`, `.nb-foot-act .nk-btn`
- Intended visual anatomy:
  - Three-column local library layout where appropriate: filter rail, list, preview/detail.
  - Segmented centered tabs matching the mockup style.
  - Preview/detail panel scrolls independently.
  - Search pill has enough top padding and should not clip headers.
  - Online actions should use the same amp browser palette and button language as `nk-btn`/`nb-audition`.
- Preserve app function:
  - Local tab always visible.
  - Favorites.
  - IR folder setting.
  - Existing online search/download/load behavior.
- Avoid:
  - New tab styling that diverges from the mockup.
  - Over-wide online search field.
  - Decorative close button stronger than the mockup's simple `nb-x`.

### Themes

- Mockup source:
  - `mw2.css`, `.mw2.theme-daylight`, `.mw2.theme-synthwave`, `.mw2.theme-forest`
  - `nam.css`, theme-scoped amp tokens
- Intended visual anatomy:
  - Daylight is not plain white. It uses soft off-white and gray-blue surfaces with readable contrast.
  - NAM/IR browser palettes use complementary amp tokens, not a flat recolor of the main theme.
- Preserve app function:
  - Existing runtime theme switching.
  - Existing theme names and saved preferences.
- Avoid:
  - Pure white primary surfaces.
  - Single-hue recolors that ignore complementary accent tokens.

## Required Process For Next Slices

1. Pick one surface from this ledger.
2. Read the cited mockup selectors/components before editing.
3. Read the current JUCE implementation file for that same surface.
4. Patch only the smallest visible delta.
5. Run focused source/visual regression tests or build.
6. If visual fidelity is the point, compare isolated node screenshots from both mockup and app before claiming improvement.
