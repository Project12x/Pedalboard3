# UI Polish Roadmap Upgrade (Supersedes Prior UI Polish Plans)

**Last updated:** 2026-02-18  
**Status:** Active  
**Supersedes:** `UI_POLISH_TASK_BOARD.md`, `UI_POLISH_2_WEEK_HANDOFF.md`

---

## Purpose

Create a premium, modern, and durable visual language for Pedalboard3 that feels "expensive" without depending on trend-driven or dated UI styles.

This roadmap consolidates previous UI polish plans, removes already-completed foundation work, and upgrades remaining work into a production-ready execution plan.

---

## Deprecated Documents

The following are now historical reference only:

1. `UI_POLISH_TASK_BOARD.md`
2. `UI_POLISH_2_WEEK_HANDOFF.md`

Use this document as the single source of truth for UI polish going forward.

---

## Legacy Inputs Incorporated

This roadmap incorporates all relevant items from:

1. UI foundation and theme/LAF goals from `UI_POLISH_TASK_BOARD.md`
2. Day-by-day implementation structure from `UI_POLISH_2_WEEK_HANDOFF.md`
3. Existing app roadmap priorities from `ROADMAP.md` Phase `6A`
4. Completed and current behavior context from `CHANGELOG.md`

---

## Completed Baseline (Excluded from New Work)

The following are treated as baseline capability and are intentionally not repeated as roadmap deliverables:

1. Theme presets and runtime theme switching
2. Modern LAF direction and custom typography integration
3. Canvas pan/zoom/fit navigation
4. Plugin search, favorites, and recent plugin affordances
5. Toast-style notification surface

These remain subject to refinement, but are no longer tracked as net-new deliverables.

---

## Visual Direction (Premium, Timeless, Non-Dated)

Design targets:

1. **Precision over decoration**: tight spacing rhythm, strong alignment, intentional hierarchy.
2. **Depth through layering, not gimmicks**: subtle elevation, restrained contrast, minimal glow.
3. **Motion with purpose**: short, quiet transitions that clarify state changes.
4. **Readable density**: complex patches remain scannable under load.
5. **Theme resilience**: same hierarchy quality across dark/light and custom schemes.

Avoid:

1. Heavy skeuomorphism (chrome, bevel stacks, fake hardware textures)
2. Excessive blur/glass effects as primary style language
3. Neon-heavy accent overuse outside targeted semantic signals
4. Trend-only motifs that age quickly

---

## Success Criteria

Definition of Done for this roadmap:

1. Visual consistency across Main view, Canvas, Stage Mode, and secondary surfaces
2. Stable keyboard/mouse affordance visibility in all interaction states
3. No regressions in patch switching, transport, graph edit, and stage workflows
4. Verified at 100%, 150%, and 200% scaling
5. Screenshot-reviewed before/after evidence for each milestone

---

## Execution Plan

## Track A: Design System Hardening (P0)

### A1. Token Audit and Enforcement
- **Priority:** P0
- **Goal:** eliminate ad-hoc UI constants from core surfaces.
- **Files:** `src/ColourScheme.h`, `src/ColourScheme.cpp`, `src/FontManager.h`, `src/FontManager.cpp`, `src/MainPanel.cpp`, `src/PluginField.cpp`, `src/PluginComponent.cpp`
- **Deliverables:**
  - semantic color role matrix including state variants (default/hover/active/disabled/focus)
  - typography tier matrix with explicit usage map
  - no new hardcoded color/font literals in target files
- **Acceptance:** all modified files consume token/tier accessors only.

### A2. LAF Consistency Contract
- **Priority:** P0
- **Goal:** consistent control rendering behavior across the app.
- **Files:** `src/BranchesLAF.h`, `src/BranchesLAF.cpp`, `src/App.cpp`, `src/ColourSchemeEditor.cpp`
- **Deliverables:**
  - unified interaction-state rendering primitives
  - guaranteed repaint/refresh path on theme changes
- **Acceptance:** no stale style states after theme swap; focus/disabled states visibly coherent.

---

## Track B: Core Surface Premium Pass (P0)

### B1. Main Shell Rhythm and Hierarchy
- **Priority:** P0
- **Goal:** make shell layout look deliberate and production-grade.
- **Files:** `src/MainPanel.h`, `src/MainPanel.cpp`
- **Deliverables:**
  - spacing/sizing constants replacing magic-number layout hotspots
  - improved typography hierarchy for patch/transport/system controls
  - control grouping with clear primary vs secondary actions
- **Acceptance:** stable and visually balanced at multiple window sizes and DPI scales.

### B2. Graph Canvas Readability Under Density
- **Priority:** P0
- **Goal:** improve parseability in large live-performance patch graphs.
- **Files:** `src/PluginField.h`, `src/PluginField.cpp`, `src/PluginComponent.h`, `src/PluginComponent.cpp`, `src/PluginConnection.cpp`
- **Deliverables:**
  - cleaner node cards with stronger title/body hierarchy
  - pin affordance improvements (size, hit clarity, type contrast)
  - cable styling with stronger selected/hovered/active differentiation
  - reduced background/grid competition with signal paths
- **Acceptance:** dense graphs remain scannable without zoom dependency.

### B3. Stage Mode Premium Clarity
- **Priority:** P0
- **Goal:** "at-a-distance" confidence and premium look for live use.
- **Files:** `src/StageView.h`, `src/StageView.cpp`, `src/MainPanel.cpp`
- **Deliverables:**
  - semantic-theme-only colors (no hardcoded palette drift)
  - responsive typography and spacing for long patch names
  - stronger cueing for current patch, next patch, and critical status
- **Acceptance:** stage readability validated from distance and under multiple themes.

---

## Track C: Interaction Craft and Accessibility (P1)

### C1. State Feedback and Motion
- **Priority:** P1
- **Goal:** make interaction feel intentional and high-end.
- **Files:** `src/BranchesLAF.cpp`, `src/PluginField.cpp`, `src/PluginComponent.cpp`, `src/MainPanel.cpp`
- **Deliverables:**
  - subtle transitions for selection, hover, and active states
  - stronger drag targets and connection feedback
  - consistent focus-ring behavior for keyboard navigation
- **Acceptance:** users can reliably identify interaction state without ambiguity.

### C2. Secondary Surface Alignment
- **Priority:** P1
- **Goal:** remove style drift between core and utility surfaces.
- **Files:** `src/PresetBar.cpp`, `src/ToastOverlay.cpp`, `src/PreferencesDialog.cpp`, `src/ColourSchemeEditor.cpp`
- **Deliverables:**
  - shared spacing/typography/color rules
  - consistent corner radii, border weights, and elevation language
- **Acceptance:** dialogs/utilities feel like one product with the main shell.

---

## Track D: Quality Gates and Release Readiness (P0)

### D1. UI Regression Harness
- **Priority:** P0
- **Files:** `tests/smoke_test.cpp`, `tests/integration_test.cpp`
- **Deliverables:**
  - add/extend checks for theme switching, stage toggle, and patch switching around polish changes
- **Acceptance:** no regressions in patching, transport, and stage flows.

### D2. Visual Review Discipline
- **Priority:** P0
- **Files:** `documentation/style.css`, `documentation/*.html` (as needed)
- **Deliverables:**
  - before/after screenshots for every milestone
  - UI checklist sign-off record (density, contrast, focus, scaling, stage readability)
- **Acceptance:** each milestone has visual evidence and QA notes.

---

## Track E: Legacy 6A Carry-Forward (Modernized) (P1/P2)

These are unresolved UI polish items migrated from `ROADMAP.md` Phase `6A`, reframed to match the premium/timeless direction.

### E1. Motion System and Interaction Feel
- **Priority:** P1
- **Legacy source:** `6A.5`, `6A.10`
- **Deliverables:**
  - purposeful animation specs for panel/canvas transitions
  - drag/drop feedback quality pass for perceived responsiveness
- **Acceptance:** motion improves clarity and quality perception without visual noise.

### E2. Connection and Bypass Signal Cues
- **Priority:** P1
- **Legacy source:** `6A.8`, `6A.18`
- **Deliverables:**
  - stateful cable highlighting (normal/hover/selected/invalid)
  - stronger bypass-state visual communication at node level
- **Acceptance:** routing and bypass state are legible at a glance in dense graphs.

### E3. Utility Discoverability Cues
- **Priority:** P2
- **Legacy source:** `6A.7`, `6A.11`, `6A.12`, `6A.13`
- **Deliverables:**
  - modern tooltip behavior pass (timing, density, readability)
  - keyboard shortcut overlay refresh aligned with design system
  - adaptive theme policy (manual + optional OS-follow mode)
  - welcome/onboarding surface visual refresh
- **Acceptance:** helper UI improves discoverability without looking intrusive or outdated.

### E4. High-Fidelity Visual Assets
- **Priority:** P2
- **Legacy source:** `6A.4`, `6A.9`, `6A.17`
- **Note:** blend2d was evaluated and dropped — JUCE 8 Direct2D on Windows makes a CPU-based vector engine redundant. SVG icons via `Drawable::createFromSVG()` + `BinaryData` provide crisp rendering at any zoom with zero new dependencies.
- **Deliverables:**
  - SVG icon system using JUCE Drawable + BinaryData for all toolbar/node icons
  - plugin thumbnail strategy with restrained, non-cluttered presentation
  - CPU meter redesign aligned with premium shell hierarchy
- **Acceptance:** visual upgrades improve perceived quality without harming clarity or performance.

---

## Milestones

1. **M1 - System Lock (Week 1):** A1, A2 complete
2. **M2 - Core Premium (Week 2):** B1, B2 complete
3. **M3 - Stage Excellence (Week 3):** B3 complete
4. **M4 - Interaction/Surface Finish (Week 4):** C1, C2, E1 complete
5. **M5 - Legacy Carry-Forward (Week 5):** E2, E3, E4 complete
6. **M6 - Final QA Gate (Week 6):** D1, D2 complete

---

## Priority Backlog (Active)

| ID | Item | Priority | Status |
|---|---|---|---|
| A1 | Token audit and enforcement | P0 | Planned |
| A2 | LAF consistency contract | P0 | Planned |
| B1 | Main shell rhythm and hierarchy | P0 | In Progress |
| B2 | Graph canvas readability under density | P0 | In Progress |
| B3 | Stage mode premium clarity | P0 | In Progress |
| C1 | State feedback and motion | P1 | Planned |
| C2 | Secondary surface alignment | P1 | Planned |
| D1 | UI regression harness | P0 | Planned |
| D2 | Visual review discipline | P0 | Planned |
| E1 | Motion system and interaction feel | P1 | Planned |
| E2 | Connection and bypass signal cues | P1 | Planned |
| E3 | Utility discoverability cues | P2 | Planned |
| E4 | High-fidelity visual assets | P2 | Planned |

---

## Progress Log

### 2026-02-13 — Master Gain Controls + VU Meter Upgrade

**Tracks affected:** B1, B2, B3

Completed work:

- **Master gain sliders** added to footer toolbar (B1), Audio I/O node components (B2), and Stage View (B3)
- Footer layout refactored with responsive breakpoints (full labels / compact / hidden) for varying window widths
- Self-labeling sliders via `textFromValueFunction` ("IN 0.0 dB" / "OUT 0.0 dB") with tooltips
- **Professional VU meters** across all surfaces: gradient fill, peak hold indicators, dB tick marks, glow effects, thicker bars
- Audio I/O nodes enforced to consistent minimum width (140px)
- `MasterGainState` singleton for RT-safe gain state shared across all UI and audio thread
- `MeteringProcessorPlayer` applies gain at device callback level (pre-graph input, post-graph output)
- `SafetyLimiterProcessor` extended with input level metering

**Commits:** `72ca2e8`, `9ee5629`

### 2026-02-18 — DAW Mixer/Splitter + Thread Safety Hardening

**Tracks affected:** B2

Completed work:

- **DAW Mixer plugin** — N-strip mixer with per-strip gain/pan/mute/solo/phase, stereo/mono toggle, SmoothedValue gain ramps, VU metering per channel
- **DAW Splitter plugin** — 1-to-N splitter with matching per-strip controls
- **Pin-to-strip alignment** via PinLayout virtual on PedalboardProcessor
- **MIDI pin placement** fixed to bottom-left for PedalboardProcessors
- **Button clipping fix** — e/b/m buttons reposition on strip add/remove
- **Tech debt pass** — duplicate mute.store bug, comment cleanup, loop hoist, thread safety audit
- **Thread/lifetime safety hardening** across 5 subsystems: CrashProtection, MainPanel OSC, Tone3000DownloadManager, Tone3000Auth, SafePluginScanner
- **blend2d dropped** from roadmap — JUCE 8 Direct2D makes it redundant; SVG via Drawable replaces it

**Commits:** `2c8e61a`, `1fddbdf`, `f856a2a`, `14b8c86`

---

## Working Rules

1. One P0 item in active implementation at a time.
2. No milestone closes without screenshot evidence and test notes.
3. No style changes merged without theme-switch and DPI checks.
4. Prefer timeless clarity over trend-heavy styling.

---

## Notes on Positioning

The target aesthetic is **high-end live-performance software**:

1. calm, controlled, and technical
2. high information density with low cognitive noise
3. durable style that will still look current after multiple release cycles
