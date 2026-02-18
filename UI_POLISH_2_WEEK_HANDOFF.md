# 2-Week UI Polish Implementation Plan (Handoff)
> [!WARNING]
> **Deprecated on 2026-02-13.**
> This handoff is retained for historical context only.
> Active UI polish planning has moved to `UI_POLISH_ROADMAP_UPGRADE.md`.

> [!NOTE]
> Historical sequencing from this file is incorporated into the upgrade roadmap.

**Project:** Pedalboard3  
**Window:** Monday, February 9, 2026 to Friday, February 20, 2026 (10 working days)  
**Objective:** Commercial-grade UI polish with minimal behavioral risk and clear file-level ownership.

---

## Scope and Constraints

- Focus on visual consistency, interaction clarity, and premium perceived quality.
- Avoid net-new feature scope except polish-adjacent quality fixes.
- Keep each day as a small PR-sized unit with before/after screenshots.
- Freeze unrelated feature work during this sprint except blocker bug fixes.

---

## Week 1

## Day 1 (Feb 9): Design Tokens + Typography Baseline

**Goals**
- Establish semantic color + typography foundations used across all UI surfaces.

**File-by-file tasks**
- `src/ColourScheme.h`
  - Add typed semantic accessors for role-based colors (surface, border, accent, text-strong, text-muted, danger, success).
- `src/ColourScheme.cpp`
  - Map semantic roles for all built-in themes.
  - Ensure Stage Mode colors can be derived from theme roles (not hardcoded literals).
- `src/FontManager.h`
  - Add helpers for typography tiers: `caption`, `body`, `label`, `title`, `display`.
- `src/FontManager.cpp`
  - Implement helper styles and robust fallback behavior.
- `CHANGELOG.md`
  - Update stale typography note to match current embedded font stack.

**Acceptance**
- No UI surface depends on undocumented color role names for new work.
- Typography helpers are available and used by new polish patches.

---

## Day 2 (Feb 10): Global LookAndFeel Consistency

**Goals**
- Normalize control states and visual language at the LAF layer.

**File-by-file tasks**
- `src/BranchesLAF.h`
  - Add shared paint utility declarations for controls.
- `src/BranchesLAF.cpp`
  - Standardize hover/pressed/focus/disabled rendering.
  - Normalize corner radii, border weights, and contrast for text + controls.
- `src/App.cpp`
  - Ensure theme changes reliably trigger LAF refresh path.
- `src/ColourSchemeEditor.cpp`
  - Trigger LAF refresh and explicit repaint propagation after theme apply/save.

**Acceptance**
- Buttons, menus, toggles, and fields render with coherent state behavior.
- Theme switching updates active UI immediately without stale artifacts.

---

## Day 3 (Feb 11): Main Shell Layout Polish

**Goals**
- Make footer/top-level control layout feel intentional and production-grade.

**File-by-file tasks**
- `src/MainPanel.h`
  - Add layout constants/struct for spacing, sizing, and alignment baselines.
- `src/MainPanel.cpp`
  - Replace magic-number layout values in `resized()` with named constants.
  - Rebalance patch controls, transport cluster, organize/fit actions, CPU meter.
  - Remove legacy hardcoded background remnants in paint path.
  - Migrate shell text/control fonts to `FontManager` helpers.

**Acceptance**
- Footer alignment is consistent at common widths and DPI scales.
- No regressions in transport, patch switching, or command routing.

---

## Day 4 (Feb 12): Canvas Visual Hierarchy + Empty State

**Goals**
- Improve graph readability without reducing information density.

**File-by-file tasks**
- `src/PluginField.h`
  - Add style constants for grid density/opacities.
- `src/PluginField.cpp`
  - Tune background and grid contrast (optionally zoom-aware intensity).
  - Refine empty-state typography and spacing.
- `src/SettingsManager.h`
- `src/SettingsManager.cpp`
  - Add optional persistence for canvas visual toggles if introduced.

**Acceptance**
- Canvas remains readable under dark/light themes and varied zoom levels.
- Empty state is clean, legible, and visually aligned with app style.

---

## Day 5 (Feb 13): Node, Pin, and Cable Polish

**Goals**
- Upgrade node graph craftsmanship (the most visible working surface).

**File-by-file tasks**
- `src/PluginComponent.h`
  - Add concise style constants for node geometry.
- `src/PluginComponent.cpp`
  - Improve node card hierarchy: header/body contrast, spacing, text clipping.
  - Improve pin visual affordance and audio vs parameter differentiation.
- `src/PluginConnection.cpp`
  - Improve cable stroke/readability, selected/hover highlight behavior.

**Acceptance**
- Node cards feel coherent and legible at typical session densities.
- Cable interactions are clearer under overlap and dense routing.

---

## Week 2

## Day 6 (Feb 16): Stage Mode Premium Pass

**Goals**
- Make Stage Mode performance UI visually premium and theme-consistent.

**File-by-file tasks**
- `src/StageView.h`
  - Add layout scale/typography tier constants for responsive behavior.
- `src/StageView.cpp`
  - Replace hardcoded palette literals with semantic theme roles.
  - Refine patch/tuner/status spacing and alignment.
  - Improve responsive behavior for long patch names and smaller displays.
- `src/MainPanel.cpp`
  - Validate StageView lifecycle, focus transfer, and repaint behavior.

**Acceptance**
- Stage mode is readable from distance and consistent across themes.
- Enter/exit stage mode behavior remains stable and predictable.

---

## Day 7 (Feb 17): Secondary Surfaces (Dialogs, Presets, Toasts)

**Goals**
- Ensure non-canvas surfaces match premium baseline.

**File-by-file tasks**
- `src/PresetBar.cpp`
  - Migrate fonts/colors to `FontManager` + semantic colors.
- `src/ToastOverlay.cpp`
  - Align radius/shadow/text specs with updated LAF primitives.
- `src/PreferencesDialog.cpp`
  - Normalize spacing and section hierarchy.
- `src/ColourSchemeEditor.cpp`
  - Improve form alignment and preview readability.

**Acceptance**
- Dialog and utility surfaces no longer look visually disconnected.

---

## Day 8 (Feb 18): Interaction and Accessibility Pass

**Goals**
- Improve focus visibility and interaction feedback quality.

**File-by-file tasks**
- `src/BranchesLAF.cpp`
  - Strengthen keyboard focus ring treatment and disabled state contrast.
- `src/PluginField.cpp`
  - Improve drag feedback and connection target highlighting.
- `src/PluginComponent.cpp`
  - Improve hover/focus cues for edit/bypass/delete affordances.
- `src/MainPanel.cpp`
  - Confirm keyboard navigation order and shortcut discoverability.

**Acceptance**
- Keyboard and mouse interaction feedback is clear and consistent.

---

## Day 9 (Feb 19): Cleanup and Targeted Debt

**Goals**
- Remove polish blockers and reduce noise before QA.

**File-by-file tasks**
- `src/PluginComponent.cpp`
  - Remove high-volume `std::cerr` debug spam; keep gated `spdlog` only.
- `CMakeLists.txt`
  - Remove duplicate `src/StageView.cpp` source entry.
- `src/MainPanel.cpp`
- `src/PresetBar.cpp`
  - Remove lingering legacy font/paint paths.
- `CHANGELOG.md`
  - Add unreleased notes for this polish sprint.

**Acceptance**
- Clean logs, cleaner build sources, and reduced UI technical debt.

---

## Day 10 (Feb 20): QA, Regression, and Sign-off

**Goals**
- Verify polish outcomes and protect core workflows.

**File-by-file tasks**
- `tests/smoke_test.cpp`
  - Add/extend checks for stage toggle/theme switch critical paths.
- `tests/integration_test.cpp`
  - Validate patch switching stability under polished UI code paths.
- `documentation/style.css`
- `documentation/*.html` (as needed)
  - Update screenshots/text if labels or visible UI changed.

**Acceptance**
- Manual QA pass complete at 100%, 150%, 200% scaling.
- Theme switch, long patch names, and dense routing scenarios verified.

---

## Delivery Artifacts

- Daily PRs with:
  - Before/after screenshots
  - Short change summary
  - Risk/regression notes
- End-of-sprint:
  - Updated changelog entry
  - UI consistency checklist completion
  - Test status summary

---

## Recommended Sequence for Implementation

1. Foundation first (tokens, typography, LAF).
2. Main shell + core canvas surfaces.
3. Stage mode + secondary surfaces.
4. Interaction/accessibility pass.
5. Cleanup + QA.

This sequence minimizes rework and keeps visual decisions centralized.

