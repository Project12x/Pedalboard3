# Pedalboard3 UI Scale Handoff - 2026-05-28

This document is the handoff for the recent Pedalboard3 app-owned UI scale work. It is intended to let the next session resume without replaying the conversation or guessing which scale issues are fixed, which evidence exists, and where the remaining risk is.

## Current Baseline

- Branch: `main`
- Implementation baseline before this handoff document: `7643bee fix: keep scaled dialogs usable`
- Prior UI scale commit sequence:
  - `5588ca8 feat: add Pedalboard UI scale control`
  - `c315d93 fix: expose UI scale in footer`
  - `ef47483 fix: add recoverable UI scale menu`
  - `e5bb149 fix: preserve footer controls at scaled layouts`
  - `2ec391d fix: make scaled footer layout responsive`
  - `beb1241 test: cover scaled footer visual QA`
  - `07f5512 test: add scaled footer QA evidence`
  - `7643bee fix: keep scaled dialogs usable`
- Git state before creating this document was clean.

## Problem Statement

Pedalboard needed a per-program UI scale control, independent of Windows Display scale. The important distinction is:

- Pedalboard UI scale is an app-owned setting applied through JUCE global scale.
- Windows Display scale is only the operating-system DPI environment around the app.
- Users must be able to recover from an oversized app scale without editing settings files or changing Windows Display scale.

The user specifically required:

- Scale choices starting at `75%` so the app can scale both down and up.
- An obvious UI control in Pedalboard itself, not a reliance on Windows display settings.
- The control available in both the footer and Options menu.
- A reset-to-default command in the menu.
- Footer controls must remain reachable at high app scale.
- Secondary dialogs, especially Preferences, must remain usable at high app scale.

## What Is Implemented

### App UI Scale Model

Files:

- `src/UiScale.h`
- `src/UiScale.cpp`

Behavior:

- Supported app scale choices are `75`, `100`, `125`, `150`, `175`, and `200`.
- Default remains `100%`.
- Persisted settings use `UiScale::settingsKey`, currently `"UiScalePercent"`.
- Invalid persisted values are normalized to the nearest supported choice.
- Visual QA can drive app scale with the command-line flag `--visual-qa-ui-scale=`.
- Footer layout helpers are scale-aware, so available layout width is evaluated against app scale instead of raw physical pixels.

### Footer Control And Options Recovery

Files:

- `src/MainPanel.h`
- `src/MainPanel.cpp`

Behavior:

- The footer has a visible `UI Scale` combo box when layout space permits.
- The Options menu has a `UI Scale` submenu with all supported choices.
- Options also includes `Reset to Default (100%)`.
- `MainPanel::setUiScalePercent()` persists the setting, applies the JUCE global scale, syncs the footer combo box, and triggers resized/repaint behavior.
- High-scale footer layout now leaves single-row mode sooner and can fall back to a stacked arrangement before controls become clipped.
- The intended recovery path is: `Options > UI Scale > Reset to Default (100%)`.

### Preferences Dialog High-Scale Recovery

Files:

- `src/PreferencesDialog.h`
- `src/PreferencesDialog.cpp`

Behavior:

- Preferences now has its own `UI Scale` combo box near the top of the dialog.
- The dialog content is hosted inside a vertical `Viewport`, so lower settings remain reachable when app scale or window size reduces visible space.
- The OSC multicast hint was shortened to `optional` to avoid local clipping at high scale.
- The top of Preferences exposes enough recovery surface at `200%` narrow capture size to switch back or reset elsewhere via Options.

Important maintenance note:

- `PreferencesDialog.cpp` contains older Projucer-style generated sections. Avoid opening this dialog in Projucer or regenerating it unless you are prepared to preserve the hand edits around the viewport/content component and UI Scale controls.

### Visual QA Harness

Files:

- `scripts/run_d2_visual_qa.ps1`
- `tests/ui_regression_harness_test.cpp`
- `tests/ui_scale_test.cpp`
- `documentation/ui-polish-qa.htm`

Behavior:

- `scripts/run_d2_visual_qa.ps1` accepts `-UiScalePercent`.
- It records `uiScalePercent` in `capture-summary.json`.
- `-CaptureScaledFooterMatrix` captures footer evidence at `125%`, `150%`, `175%`, and `200%`.
- `-CaptureScaledDialogMatrix` captures secondary dialog evidence at `150%` and `200%`.
- The OS display scale guard remains only as `-ExpectedOsScalePercent` for compatibility checks. It is not the mechanism for app scaling.
- The regression harness now treats scaled footer and scaled dialog evidence as documented visual QA requirements.

## Evidence Captured

### Scaled Footer Evidence

Folder:

- `documentation/qa/2026-05-27-d2-scaled-footer/`

Key files:

- `README.md`
- `capture-summary.json`
- `workflow-scaled-footer-125-normal.png`
- `workflow-scaled-footer-125-narrow.png`
- `workflow-scaled-footer-150-normal.png`
- `workflow-scaled-footer-150-narrow.png`
- `workflow-scaled-footer-175-normal.png`
- `workflow-scaled-footer-175-narrow.png`
- `workflow-scaled-footer-200-normal.png`
- `workflow-scaled-footer-200-narrow.png`

Manual review result:

- Footer controls remain reachable in normal and narrow captures across the high-scale matrix.
- The stacked layout at high scale preserves the recovery path and core transport/patch/CPU controls.

### Scaled Dialog Evidence

Folder:

- `documentation/qa/2026-05-27-d2-scaled-dialogs/`

Key files:

- `README.md`
- `capture-summary.json`
- `workflow-scaled-dialog-plugin-search-150-normal.png`
- `workflow-scaled-dialog-plugin-search-150-narrow.png`
- `workflow-scaled-dialog-plugin-search-200-normal.png`
- `workflow-scaled-dialog-plugin-search-200-narrow.png`
- `workflow-scaled-dialog-preferences-150-normal.png`
- `workflow-scaled-dialog-preferences-150-narrow.png`
- `workflow-scaled-dialog-preferences-200-normal.png`
- `workflow-scaled-dialog-preferences-200-narrow.png`
- `workflow-scaled-dialog-nam-browser-150-normal.png`
- `workflow-scaled-dialog-nam-browser-150-narrow.png`
- `workflow-scaled-dialog-nam-browser-200-normal.png`
- `workflow-scaled-dialog-nam-browser-200-narrow.png`
- `workflow-scaled-dialog-ir-browser-150-normal.png`
- `workflow-scaled-dialog-ir-browser-150-narrow.png`
- `workflow-scaled-dialog-ir-browser-200-normal.png`
- `workflow-scaled-dialog-ir-browser-200-narrow.png`

Manual review result:

- Preferences at `200%` narrow shows the `UI Scale` control near the top.
- Preferences at `200%` narrow shows a visible vertical scrollbar.
- Plugin search, NAM browser, and IR browser retain primary controls and close paths at `200%` narrow.

## Verification Already Run

These checks passed during the implementation sequence:

```powershell
cmake --build build --config Release --target Pedalboard3 -- /m:1
cmake --build build --config Release --target Pedalboard3_Tests -- /m:1
.\build\tests\Release\Pedalboard3_Tests.exe "[ui][regression][visual]"
.\build\tests\Release\Pedalboard3_Tests.exe "[ui][regression]"
.\build\tests\Release\Pedalboard3_Tests.exe "[ui][scale]"
powershell -ExecutionPolicy Bypass -File scripts\run_d2_visual_qa.ps1 -OutputName 2026-05-27-d2-scaled-dialogs -CaptureScaledDialogMatrix
```

Observed test result highlights:

- `[ui][regression][visual]`: 27 assertions, 3 test cases passed.
- `[ui][regression]`: 225 assertions, 7 test cases passed.
- `[ui][scale]`: 53 assertions, 9 test cases passed.
- The final scaled-dialog visual QA command completed successfully and produced the evidence folder listed above.

Build notes:

- Release build completed with existing warnings. No new blocker was identified from those warnings during this work.
- PowerShell parser validation for `scripts/run_d2_visual_qa.ps1` passed before the final visual QA capture.

## Exact Resume Commands

Start with repository state:

```powershell
git -c safe.directory='C:/Users/estee/Desktop/My Stuff/Code/Antigravity/Pedalboard2' status --short
git -c safe.directory='C:/Users/estee/Desktop/My Stuff/Code/Antigravity/Pedalboard2' log --oneline -8
```

Build and test:

```powershell
cmake --build build --config Release --target Pedalboard3 -- /m:1
cmake --build build --config Release --target Pedalboard3_Tests -- /m:1
.\build\tests\Release\Pedalboard3_Tests.exe "[ui][scale]"
.\build\tests\Release\Pedalboard3_Tests.exe "[ui][regression]"
.\build\tests\Release\Pedalboard3_Tests.exe "[ui][regression][visual]"
```

Regenerate footer evidence:

```powershell
powershell -ExecutionPolicy Bypass -File scripts\run_d2_visual_qa.ps1 -OutputName 2026-05-28-d2-scaled-footer -CaptureScaledFooterMatrix
```

Regenerate secondary dialog evidence:

```powershell
powershell -ExecutionPolicy Bypass -File scripts\run_d2_visual_qa.ps1 -OutputName 2026-05-28-d2-scaled-dialogs -CaptureScaledDialogMatrix
```

Run both visual QA matrices together if a longer capture pass is acceptable:

```powershell
powershell -ExecutionPolicy Bypass -File scripts\run_d2_visual_qa.ps1 -OutputName 2026-05-28-d2-scaled-full -CaptureScaledFooterMatrix -CaptureScaledDialogMatrix
```

## Known Risks And Gaps

- Visual QA is screenshot evidence, not automated pixel comparison. Human review is still required.
- Dialog automation depends on current window titles and button flows. If dialog labels or launch flows change, the visual QA script may need updates.
- Preferences is viewport-backed now, but not every older secondary dialog has been converted to a viewport layout.
- At `200%` narrow, Preferences is intentionally scrollable; not every setting is visible at once.
- The evidence was captured on a Windows desktop reporting `175%` OS display scale. The app-owned scale path is independent, but final release QA should still sample other OS DPI environments if practical.
- Footer behavior is materially better than before, but dense new footer controls could reopen crowding if added without updating `UiScale` layout thresholds and tests.
- The Options menu reset is the primary guaranteed recovery path if the footer combo box is hidden by compact layout.

## Recommended Next Work

1. Run one manual launch pass and exercise `75%`, `100%`, `125%`, `150%`, `175%`, and `200%` from both the footer and Options menu.
2. Add a quicker visual QA mode if repeated scale evidence captures become too slow. Today the D2 script still performs the broader baseline sequence around the matrix captures.
3. Convert any remaining secondary dialog to a viewport-backed layout only after screenshot evidence shows high-scale clipping.
4. Consider adding a small automated assertion that all supported UI scale choices appear in both the footer combo and Options submenu.
5. If preparing a PR, include the two evidence folders and call out that app UI scale is separate from Windows Display scale.

## Upstream Source And Reuse Record

No upstream source code was copied, closely ported, or adapted for the UI scale implementation. The work is local JUCE application layout code, local visual QA scripting, and local regression-test coverage.

Reuse mode: `clean-room`.

Reason no permissive reference code was inspected or reused:

- The implementation is specific to Pedalboard3's existing `MainPanel`, `PreferencesDialog`, settings layer, and D2 visual QA harness.
- The bug was an app-local layout/recovery issue, not a researched algorithm or domain component with a suitable external reference implementation.

