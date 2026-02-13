# UI Polish Task Board (2-Week Sprint)
> [!WARNING]
> **Deprecated on 2026-02-13.**
> This document is retained for historical context only.
> Active UI polish planning has moved to `UI_POLISH_ROADMAP_UPGRADE.md`.

> [!NOTE]
> Completed baseline items from this board were folded into the upgrade roadmap and are not duplicated there.

**Sprint:** UI Polish Sprint  
**Dates:** Feb 9, 2026 to Feb 20, 2026  
**Definition of Done:** All acceptance criteria met, no critical regressions in patching/transport/stage flows.

---

## Backlog Structure

- **Status lanes:** `Todo`, `In Progress`, `Review`, `Done`, `Blocked`
- **Priority labels:** `P0`, `P1`, `P2`
- **Area labels:** `LAF`, `Theme`, `Typography`, `Shell`, `Canvas`, `Node`, `Stage`, `Dialogs`, `Cleanup`, `Tests`, `Docs`

---

## Week 1 Cards

## Card 1: Semantic Color Roles and Typography Tiers
- **ID:** UI-001
- **Priority:** P0
- **Area:** Theme, Typography
- **Estimate:** 1 day
- **Files:**
  - `src/ColourScheme.h`
  - `src/ColourScheme.cpp`
  - `src/FontManager.h`
  - `src/FontManager.cpp`
  - `CHANGELOG.md`
- **Tasks:**
  - Add semantic color role accessors.
  - Map semantic roles for all built-in themes.
  - Add typography tier helpers.
  - Correct changelog font-stack note.
- **Acceptance Criteria:**
  - New polish code consumes semantic roles and tiered fonts.
  - No behavior regressions on theme load.

## Card 2: Global LAF State Normalization
- **ID:** UI-002
- **Priority:** P0
- **Area:** LAF, Theme
- **Estimate:** 1 day
- **Files:**
  - `src/BranchesLAF.h`
  - `src/BranchesLAF.cpp`
  - `src/App.cpp`
  - `src/ColourSchemeEditor.cpp`
- **Tasks:**
  - Normalize hover/pressed/focus/disabled visuals.
  - Add shared LAF paint helpers.
  - Ensure theme apply triggers immediate LAF refresh.
- **Acceptance Criteria:**
  - UI controls render consistently across states.
  - Theme switching updates visible UI without stale styles.

## Card 3: MainPanel Layout and Typography Cleanup
- **ID:** UI-003
- **Priority:** P0
- **Area:** Shell, Typography
- **Estimate:** 1 day
- **Files:**
  - `src/MainPanel.h`
  - `src/MainPanel.cpp`
- **Tasks:**
  - Introduce layout constants.
  - Remove magic-number-heavy layout hotspots.
  - Replace legacy font usage with `FontManager` tiers.
  - Remove legacy hardcoded background paint remnants.
- **Acceptance Criteria:**
  - Footer/top shell alignment remains stable at multiple widths/scales.

## Card 4: PluginField Visual Hierarchy Pass
- **ID:** UI-004
- **Priority:** P1
- **Area:** Canvas
- **Estimate:** 1 day
- **Files:**
  - `src/PluginField.h`
  - `src/PluginField.cpp`
  - `src/SettingsManager.h`
  - `src/SettingsManager.cpp`
- **Tasks:**
  - Adjust grid/background intensity strategy.
  - Refine empty-state composition and readability.
  - Persist optional visual toggles if introduced.
- **Acceptance Criteria:**
  - Canvas readability improved under dense and sparse layouts.

## Card 5: Node, Pin, Cable Craftsmanship Pass
- **ID:** UI-005
- **Priority:** P0
- **Area:** Node, Canvas
- **Estimate:** 1 day
- **Files:**
  - `src/PluginComponent.h`
  - `src/PluginComponent.cpp`
  - `src/PluginConnection.cpp`
- **Tasks:**
  - Improve node spacing/contrast/text handling.
  - Improve pin affordance and differentiation.
  - Improve cable readability and selection highlighting.
- **Acceptance Criteria:**
  - Dense patches remain visually parsable.

---

## Week 2 Cards

## Card 6: Stage Mode Theme and Layout Upgrade
- **ID:** UI-006
- **Priority:** P0
- **Area:** Stage, Theme, Typography
- **Estimate:** 1 day
- **Files:**
  - `src/StageView.h`
  - `src/StageView.cpp`
  - `src/MainPanel.cpp`
- **Tasks:**
  - Replace hardcoded palette with semantic theme colors.
  - Add responsive layout/typography constants.
  - Verify stage lifecycle, focus, and repaint paths.
- **Acceptance Criteria:**
  - Stage mode is readable at distance and consistent across themes.

## Card 7: Secondary UI Surfaces Alignment
- **ID:** UI-007
- **Priority:** P1
- **Area:** Dialogs, Shell
- **Estimate:** 1 day
- **Files:**
  - `src/PresetBar.cpp`
  - `src/ToastOverlay.cpp`
  - `src/PreferencesDialog.cpp`
  - `src/ColourSchemeEditor.cpp`
- **Tasks:**
  - Unify fonts/colors/spacing with LAF rules.
  - Improve readability of utility surfaces.
- **Acceptance Criteria:**
  - Secondary surfaces no longer visually diverge from core shell.

## Card 8: Interaction and Accessibility Polish
- **ID:** UI-008
- **Priority:** P1
- **Area:** LAF, Canvas, Shell
- **Estimate:** 1 day
- **Files:**
  - `src/BranchesLAF.cpp`
  - `src/PluginField.cpp`
  - `src/PluginComponent.cpp`
  - `src/MainPanel.cpp`
- **Tasks:**
  - Improve keyboard focus visibility.
  - Improve drag/hover/target highlights.
  - Verify keyboard navigation coherence.
- **Acceptance Criteria:**
  - Input feedback is clear for mouse and keyboard users.

## Card 9: Cleanup and Build Hygiene
- **ID:** UI-009
- **Priority:** P0
- **Area:** Cleanup
- **Estimate:** 1 day
- **Files:**
  - `src/PluginComponent.cpp`
  - `CMakeLists.txt`
  - `src/MainPanel.cpp`
  - `src/PresetBar.cpp`
  - `CHANGELOG.md`
- **Tasks:**
  - Remove `std::cerr` debug spam.
  - Remove duplicate StageView source entry.
  - Remove residual legacy visual paths.
  - Add unreleased sprint notes.
- **Acceptance Criteria:**
  - Clean runtime logs and cleaner build config.

## Card 10: QA and Regression Validation
- **ID:** UI-010
- **Priority:** P0
- **Area:** Tests, Docs
- **Estimate:** 1 day
- **Files:**
  - `tests/smoke_test.cpp`
  - `tests/integration_test.cpp`
  - `documentation/style.css`
  - `documentation/*.html` (as needed)
- **Tasks:**
  - Add/extend UI-adjacent regression checks.
  - Update docs/screenshots if visible UI changed.
- **Acceptance Criteria:**
  - Critical flows verified at 100/150/200% scaling.
  - No regressions in patching, transport, or stage operations.

---

## Suggested Board Initialization

1. Create all cards as `Todo`.
2. Mark `UI-001` as first `In Progress`.
3. Keep only one `P0` card in `In Progress` at a time.
4. Require screenshots in every card before moving to `Review`.
5. Require test notes before moving to `Done`.

---

## Dependencies and Order

1. `UI-001` -> `UI-002` -> `UI-003`
2. `UI-003` + `UI-004` -> `UI-005`
3. `UI-001` + `UI-002` -> `UI-006`
4. `UI-006` + `UI-007` -> `UI-008`
5. `UI-008` -> `UI-009` -> `UI-010`

---

## Risk Watchlist

- Theme regression risk from semantic role migration.
- Layout regressions across DPI/scaling configurations.
- Stage mode readability regressions in long-name scenarios.
- Hidden behavior regressions if visual refactors touch command flow.

Mitigation: keep daily PRs small, screenshot-backed, and test-gated.

