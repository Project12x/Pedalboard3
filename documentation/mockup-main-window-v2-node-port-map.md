# Main Window v2 Mockup Port Map

This document pins the active polish work to the actual mockup source instead of ad-hoc visual approximation.

Primary mockup files:
- `documentation/mockup-main-window-v2-lXKi/pedalboard-remix/project/Main Window v2.html`
- `documentation/mockup-main-window-v2-lXKi/pedalboard-remix/project/mw2-nodes.jsx`
- `documentation/mockup-main-window-v2-lXKi/pedalboard-remix/project/mw2.css`
- `documentation/mockup-main-window-v2-lXKi/pedalboard-remix/project/mw2-data.jsx`
- `documentation/mockup-main-window-v2-lXKi/pedalboard-remix/project/mw2-prims.jsx`

Design intent from the bundle chats:
- Main Window v2 must remain recognizably the real Pedalboard main window: OS chrome, menu bar, graph canvas, varied nodes, labeled ports where the real app has them, keyboard/transport footer.
- The remix is an upgrade layer, not a replacement. Existing UX and processor function must remain.
- NAM Loader and IR Loader are hero graph nodes. They should share the standalone NAM/IR library quality and complementary amp/cab token scheme.
- The mockup's loader nodes are graph-node renderers, not miniaturized full standalone editors.

## Core Port Rule

For NAM Loader and IR Loader, stop treating the graph node as an embedded `NAMControl` / `IRLoaderControl` editor with a decorative shell.

The real Pedalboard app remains the behavior source of truth. The mockup is a visual/layout reference only. Any port that removes an existing reachable function is wrong, even if it looks closer to the mockup.

Target structure is:

1. Host node chrome draws `ChassisFrame`.
2. Host node body draws mockup sections (`nc-sect`, `nc-slot`, `nc-field`, `nc-track`, `nc-chip`, `nc-eq-wrap`, `nc-wave-wrap`).
3. Existing functions remain available through mapped hit targets:
   - edit
   - map
   - bypass
   - load model
   - browse NAM library
   - clear model
   - load/clear IR 1
   - load/clear IR 2
   - IR enable toggles
   - blend, low cut, high cut
   - FX loop enable/edit
   - gain/gate controls
   - stack/param EQ controls
4. The standalone editor window may remain richer, but the graph node should be a bespoke node surface, matching the mockup.

## Functionality That Must Survive

These controls and behaviors exist in the real app and must remain reachable after every mockup slice. If the mockup omits one, keep it integrated into the graph node, preserve it in the full editor path, or expose it through a clear advanced/detail affordance. Do not delete it.

NAM Loader:
- Load model from disk.
- Browse NAM library / online browser.
- Clear model.
- Show current model name and architecture/type badge.
- Load IR 1 and IR 2.
- Clear IR 1 and IR 2.
- Enable/disable IR 1 and IR 2.
- Blend IR slots.
- Low-cut and high-cut cabinet filters.
- Enable FX loop and open the FX-loop editor.
- Input gain, output gain, and noise gate controls.
- Enable EQ, switch PRE/POST, switch STACK/PARAM modes, and normalize output.
- STACK mode bass/mid/treble controls.
- PARAM mode four-band frequency/gain/Q controls.

IR Loader:
- Load IR 1 and IR 2 from disk.
- Browse IR library for both slots.
- Clear IR 1 and IR 2.
- Show current IR file names.
- Blend/crossfade, mix, low cut, and high cut controls.

Node host:
- Edit/open editor.
- Map/mappings editor.
- Bypass.
- Delete/remove.
- Existing node routing/ports and connection behavior.

## Mockup Elements To Port

| Mockup source | Exact construct | JUCE target |
|---|---|---|
| `mw2-nodes.jsx:56` | `ChassisFrame(...)` | New host-drawn loader node branch in `PluginComponent.cpp`, not full embedded editor chrome |
| `mw2.css:290-294` | per-theme `.n-chassis --amp-*` tokens | Current `makeHeroChassisPalette()` plus loader-specific NAM/IR palette functions |
| `mw2.css:314-325` | chassis shell, selected state, screws, brushed faceplate | `drawHeroChassisNodeChrome()` and loader body helpers |
| `mw2.css:342-371` | `nc-sect`, `nc-field`, `nc-slot`, `nc-tog` | Reusable JUCE draw helpers for section cards, file fields, slot cards, on/off pills |
| `mw2.css:377-390` | `nc-track`, `nc-fill`, `nc-thumb`, `nc-chip` | Reusable JUCE slider/readout visuals for graph-node controls |
| `mw2-nodes.jsx:91` | `NcKnob` | Graph-node-specific compact knob primitive, not full editor rotary control |
| `mw2-nodes.jsx:121` | `NcGainRow` | NAM graph node gain/gate rows |
| `mw2-nodes.jsx:154` | `ToneCurve` | NAM stack mode visual curve |
| `mw2-nodes.jsx:171` | `IRWave` | IR slot waveform preview |
| `mw2-nodes.jsx:201` | `EqCurve` | NAM parametric EQ mode curve and handles |
| `mw2-nodes.jsx:230` | `RackGraph` | Effect Rack nested graph preview |
| `mw2-nodes.jsx:334` | NAM Loader case | NAM graph-node layout source of truth |
| `mw2-nodes.jsx:428` | IR Loader case | IR graph-node layout source of truth |
| `mw2-nodes.jsx:477` | Effect Rack case | Effect Rack subgraph node source of truth |
| `mw2-nodes.jsx:500` | Mixer case | Generic numbered channel strips |
| `mw2-nodes.jsx:533` | Splitter case | Generic fan-out router |
| `mw2-nodes.jsx:569` | Tuner case | Node-level tuner modes and readout |
| `mw2-nodes.jsx:575` | Note case | Sticky note without generic plugin controls |
| `mw2.css:88-94` | grid background and grid-lines default | Main graph background/grid rendering |
| `mw2.css:98-99` | cable wire/glow using same path | `PluginConnection.cpp` glow path behavior |

## Current Misalignment

Current loader work still largely embeds `NAMControl` and `IRLoaderControl` into the graph node, then tries to fit/chrome around them. That preserves function, but it does not match the mockup because:

- The mockup's hero nodes are purpose-built graph cards with only the expected node controls visible.
- The current graph node duplicates editor-level density and still reads busy/flat.
- Size tweaks do not solve the structure mismatch.
- The source/function-contract tests are only behavior guardrails. They must not be treated as visual approval while the node design is still moving.

2026-06-13 node-only screenshots show the host-layout fix is active, but the loader bodies still diverge from the mockup in one structural way: the app paints large section cards around Signal Chain/Gain/Tone and Impulse/Mix/Filter, while the mockup's `nc-sect` sections are mostly bare body rhythm with only nested `nc-field`, `nc-slot`, `nc-track`, `nc-chip`, `nc-wave-wrap`, and `nc-eq-wrap` surfaces receiving chrome.

Expansion rule for app-only functions:
- If the mockup omits a real Pedalboard function, add the function as a compact child of the closest mockup construct.
- Do not add another enclosing card to hold it.
- Prefer an additional `nc-row`, `nc-field` action chip, or `nc-slot` footer over expanding the node with editor-style panels.
- Keep live JUCE controls for behavior, but place them on top of the mockup-derived rectangles.
- If a live control cannot fit without clipping, expand that mockup section vertically before changing the visual grammar.

## Correct Implementation Slices

1. **Chassis Primitive Slice**
   - Build reusable graph-node drawing helpers from the `nc-*` mockup vocabulary.
   - Keep `PluginComponent` footer edit/map/bypass behavior.
   - Remove the left colored bar and thin borders for all nodes.

2. **NAM Loader Direct-Port Slice**
   - Replace embedded-editor layout in the graph with a bespoke NAM node body:
     - header glyph + eyebrow + title + status
     - Capture/Signal Chain section
     - model field + NAM tag
     - Cabinet IR dual slot grid with on pills
     - Gain rows
     - Tone section with Stack/Param mode
     - Tone curve or parametric EQ curve
   - Map existing buttons/sliders/toggles to hit targets on this body.

3. **IR Loader Direct-Port Slice**
   - Replace embedded-editor layout in the graph with a bespoke IR node body:
     - header glyph + eyebrow + title + status
     - Primary/Secondary IR slot cards
     - waveform preview per slot
     - A/B crossfade
     - filter chips/curve
   - Map existing load/browse/clear/blend/filter controls to hit targets.

4. **Supporting Node Slice**
   - Effect Rack: port `RackGraph`, not fixed slots.
   - Mixer/Splitter: port generic numbered strips/fan-out, no semantic labels.
   - Tuner: reconcile real `TunerControl` modes with mockup node structure.
   - Note: sticky note, no generic M/E/B row.

5. **Browser/Theme Slice**
   - Continue NAM online browser padding/action button/detail polish.
   - Bring Daylight theme to the mockup standard: no plain white cards, readable warm/cool surfaces.

## Verification Required

Every implementation slice must include:
- a focused function contract that checks existing Pedalboard controls and routes remain reachable,
- a Release build,
- focused tests,
- node-only visual QA screenshots:
  - mockup NAM Loader node vs. app NAM Loader node,
  - mockup IR Loader node vs. app IR Loader node,
  - supporting mockup node vs. app node for tuner/effect rack/mixer/splitter work,
- only then full-window QA screenshots for footer clipping, routing context, stage mode, and theme regressions.

Headless/source checks are not design gates. They exist to prevent mockup ports from deleting app-only functions such as mapping, bypass, FX loop, dual IR slots, gain/gate, EQ modes, and parametric EQ.

## Node-Only Visual Gate

For loader/rack/tuner polish, the required comparison artifact is the node alone, not the whole main window.

Required captures:
- `documentation/qa/<run>/mockup-node-nam-loader.png`
- `documentation/qa/<run>/app-node-nam-loader.png`
- `documentation/qa/<run>/mockup-node-ir-loader.png`
- `documentation/qa/<run>/app-node-ir-loader.png`

For each later node slice, add the same pair, for example `mockup-node-effect-rack.png` and `app-node-effect-rack.png`.

Rules:
- capture the node at a readable design-review scale,
- crop to the node bounds plus a small shadow/glow margin,
- compare mockup node to app node before using any full-window screenshot,
- do not use source/headless paint checks to claim visual progress.

Capture command:

```powershell
.\scripts\run_d2_visual_qa.ps1 -OutputName 2026-06-13-node-polish -CaptureNodeSnapshots
```
