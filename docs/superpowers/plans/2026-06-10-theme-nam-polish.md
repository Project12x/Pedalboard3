# Theme And NAM Polish Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Align Daylight, Synthwave, and Forest with the mockup tokens while polishing the NAM Loader editor and TONE3000 online tab without changing NAM behavior.

**Architecture:** Keep the existing JUCE component structure. Theme changes live in `ColourScheme.cpp` and swatches in `ThemeSwitcherComponent.cpp`; NAM visual changes stay in `NAMControl.cpp/.h`; online browser visual changes stay in `NAMOnlineBrowser.cpp/.h`. All behavior remains behind existing button, list, auth, download, and processor callbacks.

**Tech Stack:** JUCE C++17, existing `ColourScheme`, `FontManager`, `melatonin_blur`, Catch2 regression tests, MSBuild Release targets.

---

## File Map

- `src/ColourScheme.cpp`: update built-in token values for Daylight, Synthwave, Forest; preserve all required semantic roles.
- `src/ThemeSwitcherComponent.cpp`: keep swatches consistent with adjusted mockup tokens.
- `src/NAMControl.h`: add small helper palette fields only if needed by `NAMLookAndFeel`; do not add processor state.
- `src/NAMControl.cpp`: polish NAM Loader LookAndFeel, section panel drawing, header, model/IR chip treatment, and compact layout spacing.
- `src/NAMOnlineBrowser.h`: add visual helper state only if necessary; do not add API/auth/search state.
- `src/NAMOnlineBrowser.cpp`: polish result-row paint, toolbar/detail paint, status/action button colors, and layout spacing.
- `docs/superpowers/plans/2026-06-09-pedalboard-remix-ui-upgrade.md`: append verification note after implementation.

## Task 1: Align Requested Theme Tokens

**Files:**
- Modify: `src/ColourScheme.cpp`
- Modify: `src/ThemeSwitcherComponent.cpp`
- Test: `tests/ui_regression_harness_test.cpp`

- [ ] **Step 1: Update `Daylight` token assignments in `ColourScheme.cpp`**

Replace only the Daylight built-in values that differ from the mockup token source:

```cpp
colours["Window Background"] = Colour(0xFFEEF0F2);
colours["Field Background"] = Colour(0xFFFFFFFF);
colours["Text Colour"] = Colour(0xFF1A1F24);
colours["Plugin Border"] = Colour(0xFFC2C6CC);
colours["Plugin Background"] = Colour(0xFFEAECEE);
colours["Button Colour"] = Colour(0xFFE4E6E9);
colours["Button Highlight"] = Colour(0xFFF3F4F6);
colours["Stage Background Top"] = Colour(0xFFEEF0F2);
colours["Stage Background Bottom"] = Colour(0xFFDADDE1);
colours["Stage Panel Background"] = Colour(0xFFCFD3D8);
colours["Tuner Active Colour"] = Colour(0xFF0A8F5A);
```

- [ ] **Step 2: Update `Synthwave` token assignments in `ColourScheme.cpp`**

Keep magenta as the accent, but make body text/border match the mockup:

```cpp
colours["Text Colour"] = Colour(0xFFF4E6FF);
colours["Plugin Border"] = Colour(0xFF5A2D82);
colours["Accent Colour"] = Colour(0xFFFF2BFF);
colours["Menu Selection Colour"] = Colour(0xFFFF2BFF);
colours["Slider Colour"] = Colour(0xFFFF2BFF);
colours["CPU Meter Colour"] = Colour(0xFF00FF88);
colours["Tuner Active Colour"] = Colour(0xFF00FF88);
colours["Success Colour"] = Colour(0xFF00FF88);
```

- [ ] **Step 3: Update `Forest` token assignments in `ColourScheme.cpp`**

Keep the current identity but match mockup text/warm/status tones:

```cpp
colours["Text Colour"] = Colour(0xFFDCECD2);
colours["Parameter Connection"] = Colour(0xFFCCAA44);
colours["Warning Colour"] = Colour(0xFFCCAA44);
colours["Success Colour"] = Colour(0xFF44BB44);
```

- [ ] **Step 4: Update visible swatches in `ThemeSwitcherComponent.cpp`**

Change only affected swatch values:

```cpp
{"Synthwave", swatchColour("ffff2bff"), swatchColour("ff0d0221")},
{"Daylight", swatchColour("ff0077cc"), swatchColour("ffeef0f2")},
```

- [ ] **Step 5: Run theme regression gate**

Run:

```powershell
.\build\tests\Release\Pedalboard3_Tests.exe "[ui][regression][theme]"
```

Expected: all theme regression tests pass. If the test binary is stale or missing, build `Pedalboard3_Tests` Release first.

- [ ] **Step 6: Commit theme token alignment**

```powershell
git add src/ColourScheme.cpp src/ThemeSwitcherComponent.cpp
git commit -m "style: align daylight synthwave forest tokens"
```

## Task 2: Polish NAM Loader Node/Editor

**Files:**
- Modify: `src/NAMControl.cpp`
- Modify: `src/NAMControl.h` only if helper palette fields are needed
- Test: `tests/ui_regression_harness_test.cpp`, `tests/nam_processor_test.cpp`

- [ ] **Step 1: Improve `NAMLookAndFeel::refreshColours()` palette derivation**

Derive the loader surface from semantic tokens and the amp category token:

```cpp
const auto ampCategory = cs.colours.count("Graph Category Amp") != 0 ? cs.colours["Graph Category Amp"]
                                                                     : cs.colours["Warning Colour"];
ampAccent = ampCategory;
ampAccentSecondary = cs.colours["Parameter Connection"];
ampBackground = cs.colours["Plugin Background"].darker(0.35f);
ampSurface = cs.colours["Plugin Background"].interpolatedWith(cs.colours["Field Background"], 0.22f);
ampHeaderBg = cs.colours["Plugin Background"].interpolatedWith(ampCategory, 0.16f);
ampBorder = cs.colours["Plugin Border"].interpolatedWith(ampCategory, 0.16f);
ampButtonBg = cs.colours["Button Colour"];
ampButtonHover = cs.colours["Button Highlight"];
ampInsetBg = cs.colours["Field Background"].interpolatedWith(cs.colours["Plugin Background"], 0.35f);
```

- [ ] **Step 2: Polish NAM header paint**

In `NAMControl::paint`, keep the current collapse behavior and LED state, but make the header read like the mockup node header: accent-tinted gradient, small category dot, `NAM LOADER` kicker, loaded/empty model name, LED and chevron on the right. Use existing `namProcessor->isModelLoaded()` and `namProcessor->getModelName()`.

- [ ] **Step 3: Polish section panel paint**

Update `drawSectionPanel` only. It should use an inset panel fill, accent micro-rail, section title, subtle highlight line, and token-derived border. Do not change section names or call sites.

- [ ] **Step 4: Improve model/IR chip colors**

Keep `modelNameLabel`, `modelArchLabel`, `irNameLabel`, and `ir2NameLabel` as existing labels, but tune their background/text/outline colors to read as model chips. Empty labels use dim text; loaded labels use bright text and a faint accent outline.

- [ ] **Step 5: Run NAM and theme regression checks**

Run:

```powershell
.\build\tests\Release\Pedalboard3_Tests.exe "[nam]"
.\build\tests\Release\Pedalboard3_Tests.exe "[ui][regression]"
```

Expected: all selected tests pass.

- [ ] **Step 6: Commit NAM loader polish**

```powershell
git add src/NAMControl.cpp src/NAMControl.h
git commit -m "style: polish nam loader surface"
```

## Task 3: Polish NAM Online Tab

**Files:**
- Modify: `src/NAMOnlineBrowser.cpp`
- Modify: `src/NAMOnlineBrowser.h` only if helper constants/state are needed
- Test: `tests/ui_regression_harness_test.cpp`

- [ ] **Step 1: Replace result-row paint with card row treatment**

In `Tone3000ResultsListModel::paintListBoxItem`, preserve all displayed data and status states. Update row paint to draw card background, selected accent rail/glow, gear badge, cached/downloading/failed/status chips, title, author, and size/progress. Avoid hardcoded `juce::Colours::white`; use `contrastColour = base.contrasting(0.96f)` for filled status chips.

- [ ] **Step 2: Polish online browser toolbar and background**

In `NAMOnlineBrowserComponent::paint`, keep the same layout regions but improve:

- top toolbar field surface behind search and filters
- list panel fill/border
- details panel fill/border
- empty and unselected detail states
- footer/status background

Use `Window Background`, `Dialog Inner Background`, `Plugin Background`, `Button Colour`, `Button Highlight`, `Text Colour`, `Accent Colour`, `Success Colour`, `Danger Colour`, and `Warning Colour`.

- [ ] **Step 3: Tune button and ComboBox colors in constructor**

Keep all buttons and combo boxes. Set button text colors with `buttonColour.contrasting(0.96f)` for filled primary buttons and `Text Colour` for neutral buttons. Keep Download disabled until auth/selection allows it and Load disabled until cached.

- [ ] **Step 4: Preserve layout reachability**

In `resized`, keep the current search/filter/status/list/details regions but adjust gaps and button widths only if needed to prevent clipping. Download, Load, Login/Logout, previous/next, and page label must remain visible in the existing component size.

- [ ] **Step 5: Run UI regression checks**

Run:

```powershell
.\build\tests\Release\Pedalboard3_Tests.exe "[ui][regression]"
.\build\tests\Release\Pedalboard3_Tests.exe "[ui][visual]"
```

Expected: all selected tests pass.

- [ ] **Step 6: Commit NAM online polish**

```powershell
git add src/NAMOnlineBrowser.cpp src/NAMOnlineBrowser.h
git commit -m "style: polish nam online browser"
```

## Task 4: Final Verification, Documentation, Push

**Files:**
- Modify: `docs/superpowers/plans/2026-06-09-pedalboard-remix-ui-upgrade.md`

- [ ] **Step 1: Run final checks**

Run:

```powershell
git diff --check
.\build\tests\Release\Pedalboard3_Tests.exe "[ui][regression][theme]"
.\build\tests\Release\Pedalboard3_Tests.exe "[ui][regression]"
.\build\tests\Release\Pedalboard3_Tests.exe "[ui][visual]"
.\build\tests\Release\Pedalboard3_Tests.exe "[nam]"
cmd /d /c "set Path=& call ""C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe"" build\Pedalboard3.sln /t:Pedalboard3_Tests /p:Configuration=Release /m:1"
cmd /d /c "set Path=& call ""C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe"" build\Pedalboard3.sln /t:Pedalboard3 /p:Configuration=Release /m:1"
```

Expected: no diff whitespace errors, selected tests pass, both Release targets build with only existing warnings.

- [ ] **Step 2: Attempt visual QA if practical**

Run a focused capture for Daylight, Synthwave, and Forest main/NAM surfaces using the existing visual QA scripts if they can complete without the known Scratch-panel hang. If the broad script hangs again, record the exact blocker and use targeted/manual launch evidence.

- [ ] **Step 3: Append roadmap verification note**

Add a section to `docs/superpowers/plans/2026-06-09-pedalboard-remix-ui-upgrade.md` listing:

- mockup files inspected
- native files modified
- reuse mode
- preserved workflows
- commands run and results
- visual QA path or blocker

- [ ] **Step 4: Commit final verification note**

```powershell
git add docs/superpowers/plans/2026-06-09-pedalboard-remix-ui-upgrade.md
git commit -m "docs: record theme nam polish verification"
```

- [ ] **Step 5: Push branch**

```powershell
git push
```

Expected: `codex/secondary-mockup-polish` pushed to origin.

## Self-Review

- Spec coverage: theme alignment, NAM loader polish, NAM online polish, workflow preservation, verification, and documentation all have explicit tasks.
- Placeholder scan: no TBD/TODO/fill-in-later requirements are present.
- Type consistency: all named files and functions exist in the current codebase; implementation uses existing `ColourScheme`, `FontManager`, `NAMControl`, and `NAMOnlineBrowserComponent` boundaries.
