# Stage Mode Shared Chrome Polish Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Upgrade Stage Mode's shared chrome so the top bar, view switcher, theme access, safety bar, meters, and panic affordance better match the approved mockup direction without removing any existing live controls.

**Architecture:** Keep the change native and narrow. `StageLayout` owns executable geometry constraints for the header/footer chrome, while `StageView` keeps rendering and component placement. The mockup remains a pattern-only reference; no React, CSS, fonts, or assets are copied.

**Tech Stack:** C++17, JUCE, existing `ColourScheme`, existing `FontManager`, Catch2 tests, MSBuild Release targets.

---

## File Structure

- Modify: `tests/stage_layout_test.cpp`
  - Add regression coverage for centered mode-switcher reachability, right-side utility reachability, theme switcher visibility threshold, and footer meter/slider/panic spacing.
- Modify: `src/StageLayout.h`
  - Add small shared chrome metrics if tests require values that should not be recomputed ad hoc in `StageView`.
- Modify: `src/StageLayout.cpp`
  - Populate any added metrics using the existing viewport-derived metric style.
- Modify: `src/StageView.cpp`
  - Polish `StageButtonLookAndFeel`, `drawStatusBar()`, `drawSafetyBar()`, and `resized()`.
  - Preserve the existing component set and command wiring.
- Do not modify: view-specific bodies such as `drawGridView()`, `drawQueueFocus()`, `drawPatchDisplay()`, or `drawTunerDisplay()` except if a compile-only helper signature demands a mechanical update. This slice is shared chrome only.

## Task 1: Add Stage Chrome Layout Regression

**Files:**
- Modify: `tests/stage_layout_test.cpp`

- [ ] **Step 1: Add a failing Stage chrome reachability test**

Append this test case to `tests/stage_layout_test.cpp`:

```cpp
TEST_CASE("Stage shared chrome metrics keep header and footer controls separated", "[ui][regression][stage][layout]")
{
    const auto compact = StageLayout::calculateMetrics(760, 540, true);
    const auto normal = StageLayout::calculateMetrics(1280, 820, true);
    const auto wide = StageLayout::calculateMetrics(1920, 1080, true);

    auto checkHeader = [](const StageLayout::Metrics& metrics, int width)
    {
        const int modeButtonWidth = metrics.modeButtonWidth;
        const int modeButtonGap = metrics.modeButtonGap;
        const int modeGroupWidth = modeButtonWidth * 4 + modeButtonGap * 3;
        const int modeX = (width - modeGroupWidth) / 2;
        const int rightClusterWidth = metrics.utilityButtonWidth * 2 + metrics.margin * 2;

        CHECK(modeButtonWidth >= 68);
        CHECK(modeButtonWidth <= 112);
        CHECK(modeButtonGap >= 4);
        CHECK(modeGroupWidth < width - metrics.margin * 2);
        CHECK(modeX > metrics.margin + metrics.stageBrandWidth);
        CHECK(modeX + modeGroupWidth < width - rightClusterWidth);
        CHECK(metrics.utilityButtonHeight < metrics.headerHeight);
    };

    checkHeader(compact, 760);
    checkHeader(normal, 1280);
    checkHeader(wide, 1920);

    CHECK_FALSE(compact.showStageThemeSwitcher);
    CHECK(normal.showStageThemeSwitcher);
    CHECK(wide.showStageThemeSwitcher);
    CHECK(normal.stageThemeSwitcherWidth >= 120);
    CHECK(normal.stageThemeSwitcherWidth <= 170);

    auto checkFooter = [](const StageLayout::Metrics& metrics, int width)
    {
        const float firstMeterX = metrics.meterStartX + metrics.meterLabelWidth + 6.0f;
        const float secondMeterX = firstMeterX + metrics.meterWidth + metrics.meterSpacing + metrics.panicButtonWidth +
                                   metrics.meterLabelWidth + 6.0f;
        const float panicX = static_cast<float>(width - metrics.panicButtonWidth - metrics.margin);

        CHECK(metrics.meterTopOffset + metrics.meterHeight * 2.0f + metrics.meterChannelGap * 2.0f <
              metrics.sliderTopOffset);
        CHECK(metrics.sliderTopOffset + metrics.sliderHeight <= metrics.footerHeight);
        CHECK(secondMeterX + metrics.meterWidth < panicX);
        CHECK(metrics.panicButtonHeight < metrics.footerHeight);
    };

    checkFooter(normal, 1280);
    checkFooter(wide, 1920);
}
```

- [ ] **Step 2: Run the focused test and verify RED**

Run:

```powershell
.\build\tests\Release\Pedalboard3_Tests.exe "[stage][layout]"
```

Expected: compile or test failure because `StageLayout::Metrics` does not yet contain `modeButtonWidth`, `modeButtonGap`, `stageBrandWidth`, `showStageThemeSwitcher`, or `stageThemeSwitcherWidth`.

## Task 2: Add Shared Chrome Metrics

**Files:**
- Modify: `src/StageLayout.h`
- Modify: `src/StageLayout.cpp`
- Modify: `tests/stage_layout_test.cpp`

- [ ] **Step 1: Add metric fields to `StageLayout::Metrics`**

In `src/StageLayout.h`, add these fields after `utilityButtonHeight`:

```cpp
    int stageBrandWidth = 0;
    int modeButtonWidth = 0;
    int modeButtonGap = 0;
    int stageThemeSwitcherWidth = 0;
    bool showStageThemeSwitcher = false;
```

- [ ] **Step 2: Populate the new metrics**

In `src/StageLayout.cpp`, immediately after `metrics.utilityButtonHeight = ...`, add:

```cpp
    metrics.stageBrandWidth = juce::roundToInt(juce::jlimit(148.0f, 210.0f, safeWidth * 0.13f));
    metrics.modeButtonWidth = juce::roundToInt(juce::jlimit(68.0f, 104.0f, safeWidth * 0.066f));
    metrics.modeButtonGap = juce::roundToInt(juce::jlimit(4.0f, 8.0f, safeWidth * 0.005f));
    metrics.stageThemeSwitcherWidth = juce::roundToInt(juce::jlimit(124.0f, 158.0f, safeWidth * 0.105f));
    metrics.showStageThemeSwitcher = safeWidth >= 1120 && safeHeight >= 620;
```

- [ ] **Step 3: Run the focused test and verify GREEN**

Run:

```powershell
.\build\tests\Release\Pedalboard3_Tests.exe "[stage][layout]"
```

Expected: all Stage layout tests pass. The new test proves shared chrome metrics are deterministic and keep the header/footer controls separated.

- [ ] **Step 4: Commit the layout metric contract**

Run:

```powershell
git add src/StageLayout.h src/StageLayout.cpp tests/stage_layout_test.cpp
git commit -m "test: cover stage chrome layout metrics"
```

Expected: commit succeeds with only Stage layout metric/test files staged.

## Task 3: Polish Stage Button Rendering

**Files:**
- Modify: `src/StageView.cpp`

- [ ] **Step 1: Update `StageButtonLookAndFeel::drawButtonBackground()`**

Replace the body of `drawButtonBackground()` in `src/StageView.cpp` with:

```cpp
    {
        auto& palette = ::ColourScheme::getInstance().colours;
        auto bounds = button.getLocalBounds().toFloat().reduced(2.0f);
        const bool active = button.getToggleState();
        const bool panic = button.getName().containsIgnoreCase("panic");
        const bool nav = button.getButtonText().contains("PREV") || button.getButtonText().contains("NEXT");
        const bool utility = button.getButtonText().equalsIgnoreCase("EXIT") ||
                             button.getButtonText().equalsIgnoreCase("TUNER");

        if (isButtonDown)
            bounds = bounds.translated(0.0f, 1.0f);

        const auto radius = juce::jlimit(9.0f, 15.0f, bounds.getHeight() * 0.28f);
        const auto shadow = palette["Window Background"].darker(0.72f).withAlpha(isButtonDown ? 0.10f : 0.30f);
        g.setColour(shadow);
        g.fillRoundedRectangle(bounds.translated(0.0f, isButtonDown ? 1.0f : 2.5f), radius);

        if (panic)
        {
            ColourGradient panicFill(palette["Danger Colour"].brighter(isMouseOverButton ? 0.18f : 0.08f),
                                     bounds.getX(), bounds.getY(), palette["Danger Colour"].darker(0.18f),
                                     bounds.getX(), bounds.getBottom(), false);
            g.setGradientFill(panicFill);
            g.fillRoundedRectangle(bounds, radius);
            g.setColour(palette["Danger Colour"].brighter(0.35f).withAlpha(0.78f));
            g.drawRoundedRectangle(bounds.reduced(0.5f), radius, 1.4f);
            g.setColour(juce::Colours::white.withAlpha(0.20f));
            g.drawLine(bounds.getX() + 8.0f, bounds.getY() + 3.0f, bounds.getRight() - 8.0f,
                       bounds.getY() + 3.0f, 1.0f);
            return;
        }

        auto base = active ? palette["Accent Colour"].withAlpha(0.22f)
                           : palette["Stage Panel Background"].withAlpha(utility ? 0.58f : 0.48f);
        if (nav)
            base = palette["Plugin Border"].withAlpha(active ? 0.52f : 0.42f);
        if (isMouseOverButton)
            base = base.brighter(0.10f);

        ColourGradient fill(base.brighter(active ? 0.18f : 0.10f), bounds.getX(), bounds.getY(),
                            base.darker(0.16f), bounds.getX(), bounds.getBottom(), false);
        g.setGradientFill(fill);
        g.fillRoundedRectangle(bounds, radius);

        if (active)
        {
            g.setColour(palette["Accent Colour"].withAlpha(0.14f));
            g.fillRoundedRectangle(bounds.expanded(2.0f), radius + 2.0f);
        }

        g.setColour(palette["Text Colour"].withAlpha(0.10f));
        g.drawLine(bounds.getX() + 7.0f, bounds.getY() + 2.0f, bounds.getRight() - 7.0f, bounds.getY() + 2.0f, 1.0f);
        g.setColour((active ? palette["Accent Colour"] : palette["Plugin Border"]).withAlpha(active ? 0.82f : 0.46f));
        g.drawRoundedRectangle(bounds.reduced(0.5f), radius, active ? 1.5f : 1.0f);
    }
```

- [ ] **Step 2: Update `StageButtonLookAndFeel::drawButtonText()`**

Replace the body of `drawButtonText()` with:

```cpp
    {
        auto& palette = ::ColourScheme::getInstance().colours;
        const bool active = button.getToggleState();
        const bool panic = button.getName().containsIgnoreCase("panic");
        const float fontHeight = juce::jlimit(11.0f, 16.5f, button.getHeight() * 0.31f);

        g.setFont(::FontManager::getInstance().getDisplayFont(fontHeight));
        g.setColour(panic ? juce::Colours::white.withAlpha(button.isEnabled() ? 0.96f : 0.40f)
                          : palette["Text Colour"].withAlpha(button.isEnabled() ? (active ? 0.98f : 0.78f) : 0.36f));
        g.drawFittedText(button.getButtonText(), button.getLocalBounds().reduced(9, 2), Justification::centred, 1);
    }
```

- [ ] **Step 3: Build tests target to catch compile errors**

Run:

```powershell
cmd /d /c "set Path=& call ""C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe"" build\Pedalboard3.sln /t:Pedalboard3_Tests /p:Configuration=Release /m:1"
```

Expected: build succeeds with the existing warning load and 0 errors.

## Task 4: Polish Top Bar And View Switcher

**Files:**
- Modify: `src/StageView.cpp`

- [ ] **Step 1: Update `StageView::drawStatusBar()` left brand and rail drawing**

In `StageView::drawStatusBar()`, keep the existing function signature and replace the first drawing block through the mode rail drawing block with this code. Leave the time drawing block at the end in place.

```cpp
    g.setColour(colours["Stage Panel Background"].withAlpha(0.40f));
    g.fillRect(bounds);

    g.setColour(colours["Plugin Border"].withAlpha(0.48f));
    g.drawHorizontalLine(juce::roundToInt(bounds.getBottom()) - 1, bounds.getX(), bounds.getRight());

    auto leftArea = bounds.reduced((float)metrics.margin, 0.0f).withWidth((float)metrics.stageBrandWidth);
    const auto dotSize = metrics.liveDotSize;
    const auto dotArea =
        Rectangle<float>(leftArea.getX(), leftArea.getCentreY() - dotSize * 0.5f, dotSize, dotSize);

    g.setColour(colours["Accent Colour"].withAlpha(0.15f));
    g.fillEllipse(dotArea.expanded(dotSize * 0.85f));
    g.setColour(colours["Accent Colour"]);
    g.fillEllipse(dotArea);

    auto brandText = leftArea.withTrimmedLeft(dotSize + 12.0f);
    g.setColour(colours["Text Colour"].withAlpha(0.62f));
    g.setFont(fonts.getDisplayFont(metrics.statusFontHeight));
    g.drawText("STAGE MODE", brandText, Justification::centredLeft);

    if (themeSwitcher != nullptr && themeSwitcher->isVisible())
    {
        auto switcherRail = themeSwitcher->getBounds().toFloat().expanded(5.0f, 4.0f);
        g.setColour(colours["Stage Panel Background"].withAlpha(0.46f));
        g.fillRoundedRectangle(switcherRail, 11.0f);
        g.setColour(colours["Plugin Border"].withAlpha(0.34f));
        g.drawRoundedRectangle(switcherRail.reduced(0.5f), 11.0f, 1.0f);
    }

    if (patchViewButton != nullptr && queueViewButton != nullptr && gridViewButton != nullptr &&
        tunerViewButton != nullptr)
    {
        auto modeRail = patchViewButton->getBounds()
                            .getUnion(queueViewButton->getBounds())
                            .getUnion(gridViewButton->getBounds())
                            .getUnion(tunerViewButton->getBounds())
                            .expanded(5, 4)
                            .toFloat();
        g.setColour(colours["Window Background"].darker(0.36f).withAlpha(0.34f));
        g.fillRoundedRectangle(modeRail, 13.0f);
        g.setColour(colours["Stage Panel Background"].withAlpha(0.52f));
        g.fillRoundedRectangle(modeRail.reduced(1.0f), 12.0f);
        g.setColour(colours["Plugin Border"].withAlpha(0.52f));
        g.drawRoundedRectangle(modeRail.reduced(0.5f), 13.0f, 1.0f);
    }
```

- [ ] **Step 2: Update header component placement in `StageView::resized()`**

In `StageView::resized()`, replace the theme switcher and mode button placement block with:

```cpp
    if (themeSwitcher != nullptr)
    {
        const int switcherH = juce::jmin(32, utilityButtonHeight);
        const int switcherX = margin + metrics.stageBrandWidth + margin;
        themeSwitcher->setBounds(switcherX, headerButtonY + juce::roundToInt((utilityButtonHeight - switcherH) * 0.5f),
                                 metrics.stageThemeSwitcherWidth, switcherH);
        themeSwitcher->setVisible(metrics.showStageThemeSwitcher);
        themeSwitcher->toFront(false);
    }

    exitButton->setBounds(bounds.getWidth() - utilityButtonWidth - margin, headerButtonY, utilityButtonWidth,
                          utilityButtonHeight);
    tunerToggleButton->setBounds(bounds.getWidth() - utilityButtonWidth * 2 - margin * 2, headerButtonY,
                                 utilityButtonWidth, utilityButtonHeight);

    const int modeButtonW = metrics.modeButtonWidth;
    const int modeButtonGap = metrics.modeButtonGap;
    const int modeGroupW = modeButtonW * 4 + modeButtonGap * 3;
    const int modeX = (bounds.getWidth() - modeGroupW) / 2;
    patchViewButton->setBounds(modeX, headerButtonY, modeButtonW, utilityButtonHeight);
    queueViewButton->setBounds(modeX + modeButtonW + modeButtonGap, headerButtonY, modeButtonW, utilityButtonHeight);
    gridViewButton->setBounds(modeX + (modeButtonW + modeButtonGap) * 2, headerButtonY, modeButtonW,
                              utilityButtonHeight);
    tunerViewButton->setBounds(modeX + (modeButtonW + modeButtonGap) * 3, headerButtonY, modeButtonW,
                               utilityButtonHeight);
```

- [ ] **Step 3: Run focused layout tests**

Run:

```powershell
.\build\tests\Release\Pedalboard3_Tests.exe "[stage][layout]"
```

Expected: all Stage layout tests pass.

## Task 5: Polish Safety Bar Grouping

**Files:**
- Modify: `src/StageView.cpp`

- [ ] **Step 1: Update `StageView::drawSafetyBar()`**

Inside `StageView::drawSafetyBar()`, replace the function body with:

```cpp
{
    auto& fonts = FontManager::getInstance();
    auto& colours = ColourScheme::getInstance().colours;
    const auto metrics = StageLayout::calculateMetrics(getWidth(), getHeight(), showTuner);

    g.setColour(colours["Window Background"].darker(0.12f).withAlpha(0.78f));
    g.fillRect(bounds);
    g.setColour(colours["Plugin Border"].withAlpha(0.50f));
    g.drawHorizontalLine(juce::roundToInt(bounds.getY()), bounds.getX(), bounds.getRight());

    auto labelArea = bounds.reduced((float)metrics.margin, 0.0f).removeFromTop(metrics.meterTopOffset - 2.0f);
    g.setFont(fonts.getDisplayFont(metrics.safetyFontHeight));
    g.setColour(colours["Text Colour"].withAlpha(0.50f));
    g.drawText("SAFETY", labelArea.withWidth(70.0f), Justification::centredLeft);

    const auto meterGroupX = metrics.meterStartX + metrics.meterLabelWidth + 6.0f;
    auto meterLabelArea = Rectangle<float>(meterGroupX + 72.0f, labelArea.getY(),
                                           metrics.meterWidth * 2.0f + metrics.meterSpacing, labelArea.getHeight());
    g.setColour(colours["Accent Colour"].withAlpha(0.48f));
    g.drawText("MASTER BUS", meterLabelArea, Justification::centredLeft);

    auto meterBack = Rectangle<float>(metrics.meterStartX - 8.0f, bounds.getY() + metrics.meterTopOffset - 7.0f,
                                      metrics.meterLabelWidth * 2.0f + metrics.meterWidth * 2.0f +
                                          metrics.meterSpacing + metrics.panicButtonWidth + 28.0f,
                                      metrics.sliderTopOffset + metrics.sliderHeight - metrics.meterTopOffset + 16.0f);
    g.setColour(colours["Stage Panel Background"].withAlpha(0.26f));
    g.fillRoundedRectangle(meterBack, 12.0f);
    g.setColour(colours["Plugin Border"].withAlpha(0.24f));
    g.drawRoundedRectangle(meterBack.reduced(0.5f), 12.0f, 1.0f);

    auto panicGlow = panicButton != nullptr ? panicButton->getBounds().toFloat().expanded(5.0f)
                                            : Rectangle<float>();
    if (!panicGlow.isEmpty())
    {
        g.setColour(colours["Danger Colour"].withAlpha(0.16f));
        g.fillRoundedRectangle(panicGlow, 15.0f);
        g.setColour(colours["Danger Colour"].withAlpha(0.28f));
        g.drawRoundedRectangle(panicGlow.reduced(0.5f), 15.0f, 1.0f);
    }
}
```

- [ ] **Step 2: Build tests target to catch compile errors**

Run:

```powershell
cmd /d /c "set Path=& call ""C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe"" build\Pedalboard3.sln /t:Pedalboard3_Tests /p:Configuration=Release /m:1"
```

Expected: build succeeds with the existing warning load and 0 errors.

## Task 6: Verify, Capture, Commit, Push

**Files:**
- Modify: `docs/superpowers/plans/2026-06-09-pedalboard-remix-ui-upgrade.md`
  - Add a short verification note after successful implementation.

- [ ] **Step 1: Run focused checks**

Run:

```powershell
git diff --check
.\build\tests\Release\Pedalboard3_Tests.exe "[stage][layout]"
.\build\tests\Release\Pedalboard3_Tests.exe "[ui][regression]"
.\build\tests\Release\Pedalboard3_Tests.exe "[ui][visual]"
```

Expected:

- `git diff --check` reports no whitespace errors.
- Stage layout tests pass.
- UI regression tests pass.
- UI visual regression tests pass.

- [ ] **Step 2: Build the Release app**

Run:

```powershell
cmd /d /c "set Path=& call ""C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe"" build\Pedalboard3.sln /t:Pedalboard3 /p:Configuration=Release /m:1"
```

Expected: build succeeds with the existing warning load and 0 errors.

- [ ] **Step 3: Capture Stage visual evidence**

Preferred command:

```powershell
powershell -ExecutionPolicy Bypass -File scripts\run_d2_visual_qa.ps1 -OutputName 2026-06-10-stage-shared-chrome-polish
```

Expected: visual QA produces Stage hero, queue, grid, and tuner screenshots.

If the existing broad harness fails with the known Windows `CopyFromScreen` or Scratch-panel issue, run a targeted Stage capture instead and document that limitation in the verification note. Use the existing command-line hooks:

```powershell
build\Pedalboard3_artefacts\Release\Pedalboard3.exe --visual-qa-stage
build\Pedalboard3_artefacts\Release\Pedalboard3.exe --visual-qa-stage-queue
build\Pedalboard3_artefacts\Release\Pedalboard3.exe --visual-qa-stage-grid
build\Pedalboard3_artefacts\Release\Pedalboard3.exe --visual-qa-stage-tuner
```

- [ ] **Step 4: Add verification note to the existing implementation plan**

Append this section to `docs/superpowers/plans/2026-06-09-pedalboard-remix-ui-upgrade.md`, replacing the screenshot path and warning counts with the observed results:

```markdown
## 2026-06-10 Stage Shared Chrome Polish Verification

- Superpowers flow: implemented the approved Stage shared chrome slice from `docs/superpowers/specs/2026-06-10-stage-mode-shared-chrome-polish.md`.
- Mockup source inspected: `releases/design-handoffs/pedalboard-remix/pedalboard-remix/project/StageMode.jsx`, `StageGrid.jsx`, and `stage.css`.
- Native source modified: `src/StageView.cpp`, `src/StageLayout.h`, `src/StageLayout.cpp`, and `tests/stage_layout_test.cpp`.
- Reuse mode: pattern-only / clean-room. No prototype source, assets, fonts, CSS, or React code were copied.
- Intended scope: shared Stage chrome only. Hero, Setlist, Grid, and Tuner view bodies keep their existing behavior.
- Visual QA: `documentation/qa/2026-06-10-stage-shared-chrome-polish/` contains Stage hero, setlist, grid, and tuner evidence.
- `git diff --check` passed.
- `cmd /d /c "set Path=& call ""C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe"" build\Pedalboard3.sln /t:Pedalboard3_Tests /p:Configuration=Release /m:1"` passed with existing warnings and 0 errors.
- `cmd /d /c "set Path=& call ""C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe"" build\Pedalboard3.sln /t:Pedalboard3 /p:Configuration=Release /m:1"` passed with existing warnings and 0 errors.
- `.\build\tests\Release\Pedalboard3_Tests.exe "[stage][layout]"` passed.
- `.\build\tests\Release\Pedalboard3_Tests.exe "[ui][regression]"` passed.
- `.\build\tests\Release\Pedalboard3_Tests.exe "[ui][visual]"` passed.
```

- [ ] **Step 5: Commit implementation**

Run:

```powershell
git add src/StageView.cpp src/StageLayout.h src/StageLayout.cpp tests/stage_layout_test.cpp docs/superpowers/plans/2026-06-09-pedalboard-remix-ui-upgrade.md
git commit -m "feat: polish stage shared chrome"
```

Expected: commit succeeds with only the Stage chrome implementation and verification note staged.

- [ ] **Step 6: Push the branch**

Run:

```powershell
git push
```

Expected: `codex/secondary-mockup-polish` pushes successfully.

## Self-Review

- Spec coverage: the plan covers shared top bar, view switcher, safety bar, theme consistency, current control preservation, tests, visual capture, and commit notes.
- Scope check: view-specific Grid, Setlist, Hero, and Tuner content polish is deferred to later slices B and C.
- Placeholder scan: no `TBD`, `TODO`, or open implementation placeholders remain.
- Type consistency: all new metric names are introduced in Task 2 before use in `StageView.cpp` and tests.
