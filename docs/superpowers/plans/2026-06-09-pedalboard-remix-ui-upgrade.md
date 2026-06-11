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
  - Stage mode includes real Hero, Setlist, Grid, and Tuner view modes backed by the native patch list; the Grid bank rail now uses Bank A/B/C language with clickable visible bank selectors for larger sets.
  - Visual QA recorded in `documentation/qa/2026-06-09-stage-remix-polish`.
- Main Graph first polish pass:
  - Native graph canvas now has clearer major/minor grid hierarchy.
  - Cable rendering gained a stable low-alpha bed and wider clipping bounds for readability in dense routing.
  - Audio and parameter pins now have distinct shapes while preserving the existing drag/connect hit targets.
  - Nodes gained mockup-informed accent strips plus hover, drag, and bypass cues without moving edit, mapping, bypass, delete, or plugin editor controls.
  - Visual QA recorded in `documentation/qa/2026-06-09-main-graph-remix-polish`.
- Secondary Mockup Harvest first pass:
  - Installed Playwright-based local mockup rendering support and captured `NAM Browser.html` as a visual reference in `documentation/qa/2026-06-09-mockup-reference`.
  - Upgraded NAM browser, IR browser, plugin search, and IR loader surfaces using clean-room/pattern-only mockup lessons: faceplate headers, list cards, glyph tiles, selection glow, stronger empty states, and status feedback.
  - Preserved existing UX functions: local/online/IR tabs, search, folder refresh/browse, model load/delete, IR load, plugin category tabs, keyboard navigation, and close/recovery paths.
  - Fixed populated NAM/IR browsers opening with blank details by syncing selection and detail preview after scans and filters.
  - Moved NAM Load/Delete actions into the footer action row so Load Model, Delete Model, and Close stay reachable in the 200 percent narrow dialog matrix.
  - Final visual QA recorded in `documentation/qa/2026-06-09-secondary-mockup-harvest-final`.
- Secondary Mockup Harvest aesthetic follow-up:
  - Plugin search now carries the mockup's instrument-panel title signal, framed search/status header, result-count capsule, segmented category controls, row glyphs, and category-aware footer copy while preserving fuzzy search, category tabs, keyboard navigation, format badges, double-click selection, and close behavior.
  - NAM and IR browser detail panels now use backed key-value rows with reserved glyph space, improving preview-card hierarchy without removing any model/IR metadata fields or action buttons.
  - Final visual QA recorded in `documentation/qa/2026-06-09-secondary-mockup-polish-final-v2`.
- Main graph/control polish follow-up:
  - Shared `BranchesLAF` TextButtons and linear sliders now carry mockup-informed depth, sheen, hover/pressed/disabled states, and theme-aware accent handling while preserving existing command targets and layouts.
  - The main graph canvas now has a persisted `Options > Graph Grid` selector for `Dots`, `Lines`, and `Off`, matching the useful mockup grid primitive without replacing graph behavior.
  - Wrapped plugin nodes now have optional compact body parameter controls inspired by `mw2-nodes.jsx`; the feature is controlled by persisted `Options > Node Parameter Controls` and leaves edit, mappings, bypass, delete, pin, and cable workflows intact.
  - Deferred manual visual QA for grid/control polish is recorded in `TESTLATER.md`.
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

Status: First native harvest pass complete for browser/search/loader workflow polish. The app is closer to the mockup, but not yet at the mockup's full density, proportional spacing, or premium detail treatment.

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

## 2026-06-09 Secondary Harvest Verification

- Reference mockup rendered with Playwright/Edge from local handoff sources: `documentation/qa/2026-06-09-mockup-reference/nam-browser-mockup-msedge.png`.
- Final visual QA: `documentation/qa/2026-06-09-secondary-mockup-harvest-final`.
- Follow-up visual QA: `documentation/qa/2026-06-09-secondary-mockup-polish-final-v2`.
- Build: `cmake --build build --config Release --target Pedalboard3 -- /m:1`.
- Tests:
  - `.\build\tests\Release\Pedalboard3_Tests.exe "[plugin-search]"`
  - `.\build\tests\Release\Pedalboard3_Tests.exe "[ui][visual]"`
  - `.\build\tests\Release\Pedalboard3_Tests.exe "[ui][regression]"`
- Reuse mode: pattern-only / clean-room. No prototype code, assets, or fonts were copied into the native app.

## 2026-06-10 Main Graph Control Polish Verification

- Mockup source inspected: `releases/design-handoffs/pedalboard-remix/pedalboard-remix/project/mw2.css`, `mw-app.jsx`, and `tweaks-panel.jsx`.
- Reuse mode: pattern-only / clean-room. No prototype code, assets, or fonts were copied into the native app.
- Build: `cmake --build build --config Release --target Pedalboard3 -- /m:1`.
- Tests:
  - `.\build\tests\Release\Pedalboard3_Tests.exe "[ui][regression]"`
  - `.\build\tests\Release\Pedalboard3_Tests.exe "[ui][visual]"`
- Deferred manual visual QA: `TESTLATER.md` > `Mockup Polish Visual QA`.

## 2026-06-10 Node Parameter Control Polish Verification

- Mockup source inspected: `releases/design-handoffs/pedalboard-remix/pedalboard-remix/project/mw2-nodes.jsx` and `mw2-prims.jsx`.
- Reuse mode: pattern-only / clean-room. No prototype code, assets, or fonts were copied into the native app.
- Intended scope: existing graph workflow polish only. Compact controls are limited to wrapped plugin parameters and can be disabled from `Options > Node Parameter Controls`.
- Build:
  - `cmake --build build --config Release --target Pedalboard3 -- /m:1` could not run because the existing CMake cache pins a VS Community instance currently marked incomplete/reboot-required by the Visual Studio installer.
  - `MSBuild.exe build\Pedalboard3.sln /t:Pedalboard3 /p:Configuration=Release /m:1` passed.
  - `MSBuild.exe build\Pedalboard3.sln /t:Pedalboard3_Tests /p:Configuration=Release /m:1` passed.
- Tests:
  - `.\build\tests\Release\Pedalboard3_Tests.exe "[ui][regression]"` passed.
  - `.\build\tests\Release\Pedalboard3_Tests.exe "[ui][visual]"` passed.
- Manual visual QA deferred to `TESTLATER.md` > `Mockup Polish Visual QA`.

## 2026-06-10 Stage Grid Bank Rail Verification

- Mockup source inspected: `releases/design-handoffs/pedalboard-remix/pedalboard-remix/project/StageMode.jsx`, `StageGrid.jsx`, and `stage.css`.
- Reuse mode: pattern-only / clean-room. No prototype code, assets, or fonts were copied into the native app.
- Intended scope: existing Stage Grid workflow polish only. Bank selectors jump to the first patch in visible banks and tile hitboxes still switch directly to individual patches.
- `cmd /d /c "set Path=& call ""C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe"" build\Pedalboard3.sln /t:Pedalboard3 /p:Configuration=Release /m:1"` passed with 224 existing warnings and 0 errors. The `set Path=&` prefix is required in this Codex shell because the inherited environment exposes both `PATH` and `Path`, which otherwise makes MSBuild fail before compiling.
- `.\build\tests\Release\Pedalboard3_Tests.exe "[stage][layout]"` passed: 43 assertions in 5 test cases.
- `.\build\tests\Release\Pedalboard3_Tests.exe "[ui][regression]"` passed: 562 assertions in 14 test cases.
- `.\build\tests\Release\Pedalboard3_Tests.exe "[ui][visual]"` passed: 33 assertions in 4 test cases.
- Manual visual QA deferred to `TESTLATER.md` > `Mockup Polish Visual QA`.

## 2026-06-10 IR Loader Chassis Polish Verification

- Mockup source inspected: `releases/design-handoffs/pedalboard-remix/pedalboard-remix/project/nam-browser.jsx`, `nam-browser-data.jsx`, `nam-editor.jsx`, and `nam.css`.
- Native source inspected: `src/IRLoaderControl.cpp`, `src/IRLoaderControl.h`, and `src/IRLoaderProcessor.h`.
- Reuse mode: pattern-only / clean-room. The implementation uses the same native theme-derived palette strategy already present in `NAMModelBrowser.cpp`; no prototype source, assets, fonts, or CSS were copied.
- Intended scope: standalone IR Loader workflow polish only. Load, Browse, Clear, Blend, Mix, Lo Cut, and Hi Cut behavior is unchanged.
- `cmd /d /c "set Path=& call ""C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe"" build\Pedalboard3.sln /t:Pedalboard3 /p:Configuration=Release /m:1"` passed with 218 existing warnings and 0 errors.
- `.\build\tests\Release\Pedalboard3_Tests.exe "[ui][regression]"` passed: 562 assertions in 14 test cases.
- `.\build\tests\Release\Pedalboard3_Tests.exe "[ui][visual]"` passed: 33 assertions in 4 test cases.
- Manual visual QA deferred to `TESTLATER.md` > `Mockup Polish Visual QA`.

## 2026-06-10 IR Browser Preview Polish Verification

- Mockup source inspected: `releases/design-handoffs/pedalboard-remix/pedalboard-remix/project/nam-browser.jsx` and `nam-browser.css`.
- Native source inspected: `src/NAMModelBrowser.cpp`.
- Reuse mode: pattern-only / clean-room. No prototype source, assets, fonts, CSS, or React code were copied.
- Intended scope: standalone IR Browser workflow polish only. Search, folder browse, refresh, row selection, double-click load, Load IR, Close, status text, and empty-state behavior are preserved.
- `git diff --check` passed.
- `cmd /d /c "set Path=& call ""C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe"" build\Pedalboard3.sln /t:Pedalboard3_Tests /p:Configuration=Release /m:1"` passed with 20 existing warnings and 0 errors.
- `cmd /d /c "set Path=& call ""C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe"" build\Pedalboard3.sln /t:Pedalboard3 /p:Configuration=Release /m:1"` compiled the touched UI source, then failed only at final link with `LNK1104` because the launched visual-evaluation `build\Pedalboard3_artefacts\Release\Pedalboard3.exe` process was still running.
- `.\build\tests\Release\Pedalboard3_Tests.exe "[ui][regression]"` passed: 562 assertions in 14 test cases.
- `.\build\tests\Release\Pedalboard3_Tests.exe "[ui][visual]"` passed: 33 assertions in 4 test cases.
- Manual visual QA deferred to `TESTLATER.md` > `Mockup Polish Visual QA`.

## 2026-06-10 Local NAM Browser Preview Polish Verification

- Mockup source inspected: `releases/design-handoffs/pedalboard-remix/pedalboard-remix/project/nam-browser.jsx` and `nam-browser.css`.
- Native source inspected: `src/NAMModelBrowser.cpp`.
- Reuse mode: pattern-only / clean-room. No prototype source, assets, fonts, CSS, or React code were copied.
- Intended scope: local NAM Browser workflow polish only. Folder browse, refresh, search, row selection, double-click load, Load Model, Delete Model, Close, online/community tabs, status text, and empty-state behavior are preserved.
- Implementation notes: local model search now includes metadata and file path text; selected preview state uses READY/MISSING/EMPTY chips; Load/Delete are disabled when the selected `.nam` file no longer exists.
- `git diff --check` passed.
- `cmd /d /c "set Path=& call ""C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe"" build\Pedalboard3.sln /t:Pedalboard3 /p:Configuration=Release /m:1"` passed with 219 existing warnings and 0 errors.
- `.\build\tests\Release\Pedalboard3_Tests.exe "[ui][regression]"` passed: 562 assertions in 14 test cases.
- `.\build\tests\Release\Pedalboard3_Tests.exe "[ui][visual]"` passed: 33 assertions in 4 test cases.
- Manual visual QA deferred to `TESTLATER.md` > `Mockup Polish Visual QA`.

## 2026-06-10 Stage Grid Tuner Strip Polish Verification

- Superpowers flow: used the existing approved mockup-harvest spec/plan as the design gate, then executed this bounded Stage view slice with fresh verification before commit.
- Mockup source inspected: `releases/design-handoffs/pedalboard-remix/pedalboard-remix/project/StageMode.jsx`, `StageGrid.jsx`, and `stage.css`.
- Native source inspected: `src/StageView.cpp`, `src/StageLayout.cpp`, `src/StageLayout.h`, and `tests/stage_layout_test.cpp`.
- Reuse mode: pattern-only / clean-room. No prototype source, assets, fonts, CSS, or React code were copied.
- Intended scope: Stage view schema polish only. Grid and Setlist no longer reserve the large tuner content strip; Hero/Patch still can, and the full tuner remains available through the dedicated Tune view and existing tuner button.
- Visual QA: `documentation/qa/2026-06-10-stage-grid-tuner-strip-polish/workflow-stage-mode-grid.png` confirms the Stage Grid tiles now use the main content area instead of clipping above a large tuner strip. The capture script was timeboxed because the existing full visual-QA flow still hangs later around the Scratch panel capture.
- `git diff --check` passed.
- `cmd /d /c "set Path=& call ""C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe"" build\Pedalboard3.sln /t:Pedalboard3_Tests /p:Configuration=Release /m:1"` passed with 20 existing warnings and 0 errors.
- `cmd /d /c "set Path=& call ""C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe"" build\Pedalboard3.sln /t:Pedalboard3 /p:Configuration=Release /m:1"` passed with 219 existing warnings and 0 errors.
- `.\build\tests\Release\Pedalboard3_Tests.exe "[stage][layout]"` passed: 47 assertions in 6 test cases.
- `.\build\tests\Release\Pedalboard3_Tests.exe "[ui][regression]"` passed: 566 assertions in 15 test cases.
- `.\build\tests\Release\Pedalboard3_Tests.exe "[ui][visual]"` passed: 33 assertions in 4 test cases.

## 2026-06-10 Stage Shared Chrome Polish Verification

- Superpowers flow: implemented the approved Stage shared chrome slice from `docs/superpowers/specs/2026-06-10-stage-mode-shared-chrome-polish.md` with subagent-driven workers and spec/quality reviews for metrics, buttons, header chrome, and safety bar polish.
- Mockup source inspected: `releases/design-handoffs/pedalboard-remix/pedalboard-remix/project/StageMode.jsx`, `StageGrid.jsx`, and `stage.css`.
- Native source modified: `src/StageView.cpp`, `src/StageLayout.h`, `src/StageLayout.cpp`, and `tests/stage_layout_test.cpp`.
- Reuse mode: pattern-only / clean-room. No prototype source, assets, fonts, CSS, or React code were copied.
- Intended scope: shared Stage chrome only. Hero, Setlist, Grid, and Tuner view bodies keep existing behavior.
- Visual QA: broad `scripts/run_d2_visual_qa.ps1 -OutputName 2026-06-10-stage-shared-chrome-polish` produced captures but timed out later, and desktop `CopyFromScreen` captures were contaminated by an unrelated `LoopDeLoop_Tests.exe` error dialog. Clean Stage evidence was captured with targeted `PrintWindow` runs in `documentation/qa/2026-06-10-stage-shared-chrome-polish-targeted-printwindow-wide/` for Hero, Setlist, Grid, and Tuner.
- `git diff --check` passed.
- `cmd /d /c "set Path=& call ""C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe"" build\Pedalboard3.sln /t:Pedalboard3_Tests /p:Configuration=Release /m:1"` passed with 20 existing warnings and 0 errors.
- `cmd /d /c "set Path=& call ""C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe"" build\Pedalboard3.sln /t:Pedalboard3 /p:Configuration=Release /m:1"` passed with 217 existing warnings and 0 errors.
- `.\build\tests\Release\Pedalboard3_Tests.exe "[stage][layout]"` passed: 81 assertions in 7 test cases.
- `.\build\tests\Release\Pedalboard3_Tests.exe "[ui][regression]"` passed: 610 assertions in 17 test cases.
- `.\build\tests\Release\Pedalboard3_Tests.exe "[ui][visual]"` passed: 43 assertions in 5 test cases.
- Final fix: replaced hardcoded panic white with a `Danger Colour` contrast-derived label and highlight after the token audit caught forbidden `Colours::` usage and final review caught weak `Text Colour` contrast in Synthwave.

## Next For Agent

Continue with aesthetic polish from the actual mockup, but only where it improves existing workflows:

1. Fix or bypass the visual-QA Scratch-panel hang so `-CaptureScaledDialogMatrix` can complete and produce `capture-summary.json` again.
2. Run `-CaptureScaledDialogMatrix` against the latest browser/search polish surfaces and check plugin search, NAM browser, IR browser, and Preferences at 150 and 200 percent app scale.
3. Compare the new final NAM/plugin-search screenshots against `nam-browser-mockup-msedge.png`; focus on density, hierarchy, state chips, and button finish, not wholesale replacement.
4. Continue with real workflow polish: plugin search button/state finish, remaining NAM browser density, IR browser empty/folder states, and Stage safety-bar/tuner compactness.
5. Keep Load/Close/Delete/Folders reachable at high scale before making any further aesthetic changes.
6. Preserve all existing UX functions; the mockup remains an upgrade reference, not a replacement spec.

## Immediate Next Step

Continue the Secondary Mockup Harvest aesthetic polish:

1. Re-run the scaled dialog matrix for the latest browser/search polish.
2. Inspect high-scale screenshots for clipping or hidden recovery controls.
3. Continue only with real workflow polish: NAM browser density, IR browser state clarity, selected preview hierarchy, Stage view mode polish, and screenshot comparison against the actual mockup.
4. Preserve all existing UX functions and keep implementation clean-room/pattern-only.
