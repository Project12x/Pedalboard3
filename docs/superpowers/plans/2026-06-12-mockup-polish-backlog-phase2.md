# Mockup Polish Backlog Phase 2 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Close the next mockup-polish backlog without removing existing Pedalboard UX, focusing on graph/node polish, NAM/IR library parity, Stage/Scratch affordances, and Effect Rack surfaces.

**Architecture:** Keep the existing JUCE component ownership and routing behavior. Implement visual polish as small paint/layout/persistence changes inside the current files, with one commit per subsystem slice and visual QA after each visible slice. Do not replace native workflows with mockup-only abstractions.

**Tech Stack:** JUCE C++ desktop app, CMake/MSVC, Catch2 tests, existing `SettingsManager`, existing visual QA script, local mockup files under `releases/design-handoffs/pedalboard-remix/pedalboard-remix/project/`.

---

## Execution Status

Status as of 2026-06-12: implementation slices 1-6 are complete on branch `codex/secondary-mockup-polish`.

Completed commits:

1. `12cb520 style: polish graph cables and node chrome`
2. `4fbe513 style: polish built-in graph nodes`
3. `245e792 feat: add library favorites and ir folder setting`
4. `80b19c3 style: align nam online browser with library`
5. `0b9cda0 style: polish stage tuner and scratch affordances`
6. `52ae1fb style: polish effect rack graph surfaces`

Final verification passed:

- `cmake --build build --config Release --target Pedalboard3 -- /m:1`
- `cmake --build build --config Release --target Pedalboard3_Tests -- /m:1`
- `build\tests\Release\Pedalboard3_Tests.exe "[ui][regression]"`
- `build\tests\Release\Pedalboard3_Tests.exe "[ui][visual]"`
- `build\tests\Release\Pedalboard3_Tests.exe "[ui][scale]"`
- `build\tests\Release\Pedalboard3_Tests.exe "[scratch]"`
- `build\tests\Release\Pedalboard3_Tests.exe "[subgraph]"`
- `build\tests\Release\Pedalboard3_Tests.exe "[nam]"`
- `git -c safe.directory='C:/Users/estee/Desktop/My Stuff/Code/Antigravity/Pedalboard2' diff --check`

What is next for the agent: launch the latest Release build for visual evaluation, collect the user's concrete punch list from the live app, then create the next narrowly scoped polish branch or commit set from those findings. Do not reopen the `TESTLATER` backlog unless the user explicitly asks; the current priority is visual parity and workflow polish against the mockup without losing existing UX depth.

---

## Scope Check

This is a broad UI-polish backlog spanning independent subsystems. Treat this as an umbrella plan with independent, shippable slices:

1. Main graph foundation: cables, ports, node chrome, graphpaper default.
2. Built-in node quality: Tuner, NAM Loader, Mixer, Splitter, Label, Effect Rack node.
3. NAM/IR library parity: clipping, spacing, centered separators, favorites, IR folder setting.
4. NAM Online browser parity with NAM Library.
5. Stage/Scratch affordances: persistent tuners, grid clipping, scratch quick access.
6. Effect Rack nested graph polish.

Every task must preserve existing behavior unless the user explicitly approves a behavioral change.

## References Inspected

- Mockup main graph: `releases/design-handoffs/pedalboard-remix/pedalboard-remix/project/mw2.css`, `mw2-nodes.jsx`, `mw2-data.jsx`
- Mockup NAM/IR browser: `releases/design-handoffs/pedalboard-remix/pedalboard-remix/project/nam-browser.css`, `nam-browser.jsx`, `nam.css`, `nam-editor.jsx`
- Mockup Stage/Scratch: `releases/design-handoffs/pedalboard-remix/pedalboard-remix/project/stage.css`, `StageGrid.jsx`, `StageMode.jsx`, `scratch-panel.jsx`, `scratch.css`
- Current graph/native UI: `src/PluginConnection.cpp`, `src/PluginComponent.cpp`, `src/PluginField.cpp`, `src/MainPanel.cpp`
- Current browsers/settings: `src/NAMModelBrowser.cpp`, `src/NAMModelBrowser.h`, `src/NAMOnlineBrowser.cpp`, `src/NAMOnlineBrowser.h`, `src/PreferencesDialog.cpp`, `src/PreferencesDialog.h`, `src/SettingsManager.cpp`, `src/SettingsManager.h`
- Current Stage/Scratch: `src/StageView.cpp`, `src/StageView.h`, `src/StageLayout.cpp`, `src/StageLayout.h`, `src/ScratchPanel.cpp`, `src/ScratchPanelLayout.h`
- Current built-in nodes: `src/TunerControl.cpp`, `src/TunerControl.h`, `src/NAMControl.cpp`, `src/NAMControl.h`, `src/DawMixerProcessor.cpp`, `src/DawSplitterProcessor.cpp`, `src/LabelControl.cpp`, `src/SubGraphProcessor.cpp`, `src/SubGraphEditorComponent.cpp`
- JUCE current docs checked through Context7: `juce::Graphics`, `juce::Path`, `juce::PathStrokeType`, rounded rectangle, path stroke, and text drawing APIs.

## Reference-Code-First Record

- Primary visual reference: local user-provided mockup handoff in `releases/design-handoffs/pedalboard-remix/pedalboard-remix/project/`.
- Reuse mode: pattern-only visual translation. Do not copy React/CSS directly into JUCE C++; translate the intent into existing semantic colour roles, `FontManager`, `IconManager`, and local JUCE paint/layout patterns.
- External code copied: none.
- External docs: JUCE master documentation via Context7, used only to verify current drawing API signatures.

## File Responsibility Map

- `src/PluginConnection.cpp` and `src/PluginComponent.h`: cable path rendering, cable hit area, gradient endpoints, and path-local state.
- `src/PluginComponent.cpp` and `src/PluginComponent.h`: node shell, header gradients, ports, built-in node special casing, Label node control buttons.
- `src/PluginField.cpp`: main graph background/grid style.
- `src/MainPanel.cpp` and `src/MainPanel.h`: footer Scratch affordance, graph grid menu default, command routing.
- `src/NAMControl.cpp` and `src/NAMControl.h`: NAM Loader node/editor polish.
- `src/TunerControl.cpp` and `src/TunerControl.h`: tuner node surface.
- `src/DawMixerProcessor.cpp` and `src/DawSplitterProcessor.cpp`: Mixer/Splitter control polish and pin-spacing coordination.
- `src/LabelControl.cpp`, `src/LabelControl.h`, and `src/LabelProcessor.cpp`: bespoke Label node behavior and display.
- `src/SubGraphProcessor.cpp`, `src/SubGraphEditorComponent.cpp`, and `src/SubGraphEditorComponent.h`: Effect Rack node/editor/nested graph polish.
- `src/NAMModelBrowser.cpp` and `src/NAMModelBrowser.h`: NAM Library and embedded IR Library layout, favorites, centered separators, folder settings.
- `src/NAMOnlineBrowser.cpp` and `src/NAMOnlineBrowser.h`: online browser visual parity with NAM Library.
- `src/PreferencesDialog.cpp` and `src/PreferencesDialog.h`: persistent IR folder setting in Settings.
- `src/SettingsManager.cpp` and `src/SettingsManager.h`: use existing string/string-array helpers for favorites and paths; add no new storage system.
- `src/StageView.cpp`, `src/StageView.h`, `src/StageLayout.cpp`, and `src/StageLayout.h`: Stage grid/setlist tuner behavior and grid tile clipping.
- `tests/ui_regression_harness_test.cpp`, `tests/stage_layout_test.cpp`, `tests/ui_scale_test.cpp`, `tests/scratch_recorder_test.cpp`, `tests/subgraph_test.cpp`: regression coverage gates.

## Global Implementation Rules

- Keep all existing commands, right-click menus, keyboard shortcuts, footer controls, panel buttons, and dialogs reachable.
- Do not remove `M/E/B` behavior globally. Only Label nodes get bespoke construction without those buttons.
- Do not replace the NAM Library design. Treat it as the visual target for IR Library and NAM Online.
- Do not add AI features or speculative primitives.
- Use existing theme tokens and add semantic roles only if a surface cannot be expressed with current roles.
- Use `git -c safe.directory='C:/Users/estee/Desktop/My Stuff/Code/Antigravity/Pedalboard2' ...` for git commands.
- MSVC builds in this repo may require unsandboxed execution because sandboxed builds have hit object/lib write permission failures.

---

### Task 1: Main Graph Cable, Port, Chrome, And Grid Polish

**Files:**
- Modify: `src/PluginConnection.cpp`
- Modify: `src/PluginComponent.cpp`
- Modify: `src/PluginComponent.h`
- Modify: `src/PluginField.cpp`
- Modify: `src/MainPanel.cpp`
- Test: `tests/ui_regression_harness_test.cpp`

- [ ] **Step 1: Add stable cable gradient endpoints**

In `src/PluginComponent.h`, store local bezier endpoints inside `PluginConnection` so the cable gradient is source-to-destination, not bounding-box diagonal:

```cpp
Point<float> gradientStart;
Point<float> gradientEnd;
```

Place these private members next to `drawnCurve`, `hitCurve`, and `glowPath`.

- [ ] **Step 2: Set endpoints when rebuilding the bezier**

In `PluginConnection::updateBounds`, after `p1 -= pos; p2 -= pos;`, add:

```cpp
gradientStart = p1;
gradientEnd = p2;
```

Keep `glowPath = tempPath;`, `drawnCurve`, and `hitCurve` intact.

- [ ] **Step 3: Replace oversized cable glow with path-following strokes**

In `PluginConnection::paint`, remove the `melatonin::DropShadow cableGlow` call for cables and replace the broad halo stack with two direct strokes on `glowPath`:

```cpp
const float outerGlowWidth = selected ? 14.0f : 10.0f;
const float innerGlowWidth = selected ? 8.0f : 6.0f;

g.setColour(cableColour.withAlpha(selected ? 0.16f : 0.075f));
g.strokePath(glowPath, PathStrokeType(outerGlowWidth, PathStrokeType::mitered, PathStrokeType::rounded));

g.setColour(cableColour.withAlpha(selected ? 0.22f : 0.11f));
g.strokePath(glowPath, PathStrokeType(innerGlowWidth, PathStrokeType::mitered, PathStrokeType::rounded));
```

Then create the wire gradient from the stored endpoints:

```cpp
ColourGradient wireGrad(startCol, gradientStart.x, gradientStart.y, endCol, gradientEnd.x, gradientEnd.y, false);
g.setGradientFill(wireGrad);
g.fillPath(drawnCurve);
```

Expected result: glow follows the curved bezier path, is softer than the current build, and hit testing remains unchanged because `hitCurve` is untouched.

- [ ] **Step 4: Polish pin sockets without changing hitboxes**

In `PluginPinComponent::paint`, keep `getWidth()`/`getHeight()` and mouse behavior unchanged, but draw a socket backing before the current pin body:

```cpp
auto socketBounds = pinBounds.expanded(1.0f);
g.setColour(colours["Window Background"].withAlpha(0.58f));
if (audioPin)
    g.fillRoundedRectangle(socketBounds, pinCorner + 1.0f);
else
    g.fillEllipse(socketBounds);

g.setColour(colours["Plugin Border"].interpolatedWith(baseColour, 0.22f).withAlpha(0.72f));
if (audioPin)
    g.drawRoundedRectangle(socketBounds.reduced(0.5f), pinCorner + 1.0f, 1.0f);
else
    g.drawEllipse(socketBounds.reduced(0.5f), 1.0f);
```

Keep the current chevron and hover behavior, but reduce hover glow alpha to `0.32f` so adjacent NAM Loader pins do not visually merge.

- [ ] **Step 5: Make node chrome closer to mockup gradients**

In `PluginComponent::paint`, adjust the body/header mix to match mockup `.m2-node` and `.m2-head`:

```cpp
Colour bgBase = colours["Plugin Background"].interpolatedWith(accentColour, isAudioIONode() ? 0.10f : 0.065f);
Colour bgTop = bgBase.brighter(0.11f);
Colour bgBottom = bgBase.darker(0.13f);

Colour base = colours["Plugin Background"].interpolatedWith(accentColour, isAudioIONode() ? 0.34f : 0.24f);
Colour headerTop = base.brighter(0.16f);
Colour headerBottom = base.darker(0.08f);
```

Keep existing bypass, selected, header dot, delete/edit/mapping buttons, node parameter controls, and Audio I/O meters.

- [ ] **Step 6: Change graph grid default to graphpaper lines**

In both `src/PluginField.cpp::getGraphGridStyle()` and `src/MainPanel.cpp` where `kGraphGridStyleSettingsKey` is read for the menu, keep `"Lines"` as the default and verify no code path defaults to `"Dots"` or `"Off"`.

Expected check:

```powershell
rg -n "GraphGridStyle.*Dots|GraphGridStyle.*Off|getString\\(kGraphGridStyleSettingsKey" src
```

Only `"Lines"` should appear as the default argument for this setting.

- [ ] **Step 7: Build and run graph/UI verification**

Run:

```powershell
cmake --build build --config Release --target Pedalboard3 -- /m:1
cmake --build build --config Release --target Pedalboard3_Tests -- /m:1
.\build\tests\Release\Pedalboard3_Tests.exe '[ui][regression]'
.\build\tests\Release\Pedalboard3_Tests.exe '[ui][visual]'
.\build\tests\Release\Pedalboard3_Tests.exe '[subgraph][ui]'
git -c safe.directory='C:/Users/estee/Desktop/My Stuff/Code/Antigravity/Pedalboard2' diff --check
```

Expected: all commands pass. Visual inspection confirms softer curved cable glow, better ports, no NAM Loader port clipping, graphpaper lines by default.

- [ ] **Step 8: Commit Task 1**

```powershell
git -c safe.directory='C:/Users/estee/Desktop/My Stuff/Code/Antigravity/Pedalboard2' add src/PluginConnection.cpp src/PluginComponent.h src/PluginComponent.cpp src/PluginField.cpp src/MainPanel.cpp
git -c safe.directory='C:/Users/estee/Desktop/My Stuff/Code/Antigravity/Pedalboard2' commit -m "style: polish graph cables and node chrome"
```

---

### Task 2: Built-In Node Quality Pass

**Files:**
- Modify: `src/PluginComponent.cpp`
- Modify: `src/PluginComponent.h`
- Modify: `src/TunerControl.cpp`
- Modify: `src/TunerControl.h`
- Modify: `src/NAMControl.cpp`
- Modify: `src/NAMControl.h`
- Modify: `src/DawMixerProcessor.cpp`
- Modify: `src/DawSplitterProcessor.cpp`
- Modify: `src/LabelControl.cpp`
- Modify: `src/LabelControl.h`
- Test: `tests/mixer_splitter_test.cpp`
- Test: `tests/tone_generator_test.cpp`
- Test: `tests/ui_regression_harness_test.cpp`

- [ ] **Step 1: Add Label-node special casing in `PluginComponent`**

Add a helper in the anonymous namespace of `src/PluginComponent.cpp`:

```cpp
bool isLabelNodeName(const String& name)
{
    return name.equalsIgnoreCase("Label") || name.equalsIgnoreCase("Label Node");
}
```

Use it in the constructor where edit/mappings/bypass buttons are created:

```cpp
const bool labelNode = isLabelNodeName(pluginName);
if (!labelNode)
{
    editButton = new TextButton("e", "Open plugin editor (right-click for options)");
    mappingsButton = new TextButton("m", "Open mappings editor");
    bypassButton = new DrawableButton("BypassFilterButton", DrawableButton::ImageOnButtonBackground);
}
```

Do not skip delete button creation. Label nodes must still be movable, editable through their bespoke control, and deletable.

Also guard every later use of these optional controls. At minimum:

```cpp
if (bypassable != nullptr && bypassButton != nullptr)
    bypassButton->setToggleState(bypassable->getBypass(), false);

if (editButton != nullptr)
    editButton->setBounds(10, getHeight() - 30, 20, 20);

if (mappingsButton != nullptr)
    mappingsButton->setBounds(32, getHeight() - 30, 24, 20);

if (bypassButton != nullptr)
    bypassButton->setBounds(getWidth() - 30, getHeight() - 30, 20, 20);
```

Search before finishing:

```powershell
rg -n "bypassButton->|editButton->|mappingsButton->" src\PluginComponent.cpp
```

Every dereference outside construction must either be inside a matching null check or be in code that cannot run for Label nodes.

- [ ] **Step 2: Give Label node a bespoke paint surface**

In `LabelControl::paint`, replace the flat label panel with a mockup-compatible note plate:

```cpp
auto bounds = getLocalBounds().toFloat().reduced(1.0f);
auto& colours = ColourScheme::getInstance().colours;
const auto accent = colours["Graph Category Source"];
ColourGradient fill(colours["Plugin Background"].brighter(0.08f), bounds.getX(), bounds.getY(),
                    colours["Plugin Background"].darker(0.10f), bounds.getX(), bounds.getBottom(), false);
g.setGradientFill(fill);
g.fillRoundedRectangle(bounds, 7.0f);
g.setColour(accent.withAlpha(0.42f));
g.drawRoundedRectangle(bounds.reduced(0.5f), 7.0f, 1.1f);
g.setColour(accent.withAlpha(0.48f));
g.fillRoundedRectangle(bounds.getX() + 4.0f, bounds.getY() + 5.0f, 2.0f, bounds.getHeight() - 10.0f, 1.0f);
```

Keep double-click-to-edit and the existing `TextEditor` flow.

- [ ] **Step 3: Polish Tuner node surface**

In `TunerControl::paint`, keep the note, needle, strobe, LED, and frequency helpers. Add a stronger chassis behind them:

```cpp
auto panel = getLocalBounds().toFloat().reduced(1.0f);
auto& colours = ColourScheme::getInstance().colours;
auto tunerAccent = colours["Tuner Active Colour"];
ColourGradient panelFill(colours["Plugin Background"].interpolatedWith(tunerAccent, 0.06f).brighter(0.06f),
                         panel.getX(), panel.getY(),
                         colours["Plugin Background"].interpolatedWith(tunerAccent, 0.04f).darker(0.12f),
                         panel.getX(), panel.getBottom(), false);
g.setGradientFill(panelFill);
g.fillRoundedRectangle(panel, 8.0f);
g.setColour(tunerAccent.withAlpha(0.36f));
g.drawRoundedRectangle(panel.reduced(0.5f), 8.0f, 1.2f);
```

Keep the current tuner math untouched.

- [ ] **Step 4: Bring NAM Loader control up to browser quality**

In `NAMControl::paint`, preserve all controls and the current layout. Strengthen the header and signal-chain sections using existing `NAMLookAndFeel` fields:

```cpp
ColourGradient headerGrad(namLookAndFeel.ampHeaderBg.brighter(0.13f), header.getX(), header.getY(),
                          namLookAndFeel.ampHeaderBg.darker(0.12f), header.getX(), header.getBottom(), false);
g.setGradientFill(headerGrad);
g.fillRoundedRectangle(header, 8.0f);
g.setColour(namLookAndFeel.ampAccent.withAlpha(0.55f));
g.fillRoundedRectangle(header.getX() + 8.0f, header.getBottom() - 3.0f, header.getWidth() - 16.0f, 2.0f, 1.0f);
```

Make model/IR chips use ellipsis in the LookAndFeel `drawLabel` call:

```cpp
g.drawText(label.getText(), bounds.reduced(8.0f, 0.0f), label.getJustificationType(), true);
```

Expected: long model and IR names never draw outside chip bounds.

- [ ] **Step 5: Polish Mixer and Splitter controls**

In `DawMixerProcessor.cpp` and `DawSplitterProcessor.cpp`, update the strip-row paint methods to use the graph category palette:

```cpp
auto& colours = ColourScheme::getInstance().colours;
const auto accent = colours["Graph Category Dynamics"];
ColourGradient stripFill(colours["Plugin Background"].brighter(0.07f), bounds.getX(), bounds.getY(),
                         colours["Plugin Background"].darker(0.10f), bounds.getX(), bounds.getBottom(), false);
g.setGradientFill(stripFill);
g.fillRoundedRectangle(bounds, 6.0f);
g.setColour(accent.withAlpha(0.42f));
g.drawRoundedRectangle(bounds.reduced(0.5f), 6.0f, 1.0f);
```

Do not change channel counts, audio processing, strip add/remove behavior, gain ranges, mute/solo behavior, or state XML.

- [ ] **Step 6: Verify built-in node behavior**

Run:

```powershell
cmake --build build --config Release --target Pedalboard3 -- /m:1
cmake --build build --config Release --target Pedalboard3_Tests -- /m:1
.\build\tests\Release\Pedalboard3_Tests.exe '[tonegen]'
.\build\tests\Release\Pedalboard3_Tests.exe '[mixer]'
.\build\tests\Release\Pedalboard3_Tests.exe '[splitter]'
.\build\tests\Release\Pedalboard3_Tests.exe '[ui][regression]'
git -c safe.directory='C:/Users/estee/Desktop/My Stuff/Code/Antigravity/Pedalboard2' diff --check
```

Expected: tests pass; Label nodes have no M/E/B buttons; normal plugin nodes still have edit, mappings, and bypass affordances.

- [ ] **Step 7: Commit Task 2**

```powershell
git -c safe.directory='C:/Users/estee/Desktop/My Stuff/Code/Antigravity/Pedalboard2' add src/PluginComponent.cpp src/PluginComponent.h src/TunerControl.cpp src/TunerControl.h src/NAMControl.cpp src/NAMControl.h src/DawMixerProcessor.cpp src/DawSplitterProcessor.cpp src/LabelControl.cpp src/LabelControl.h
git -c safe.directory='C:/Users/estee/Desktop/My Stuff/Code/Antigravity/Pedalboard2' commit -m "style: polish built-in graph nodes"
```

---

### Task 3: NAM And IR Library Parity, Favorites, And IR Folder Setting

**Files:**
- Modify: `src/NAMModelBrowser.h`
- Modify: `src/NAMModelBrowser.cpp`
- Modify: `src/PreferencesDialog.h`
- Modify: `src/PreferencesDialog.cpp`
- Modify: `src/SettingsManager.cpp`
- Modify: `src/SettingsManager.h`
- Test: `tests/ui_regression_harness_test.cpp`

- [ ] **Step 1: Define persistent settings keys**

In `src/NAMModelBrowser.cpp`, add anonymous namespace constants:

```cpp
constexpr const char* kNamFavoritesSettingsKey = "NAMModelFavorites";
constexpr const char* kIrFavoritesSettingsKey = "IRFavorites";
constexpr const char* kIrLibraryDirectorySettingsKey = "IRLibraryDirectory";
```

Use the existing `SettingsManager::getStringArray()` and `setStringArray()` helpers for favorites.

- [ ] **Step 2: Add browser component favorite state**

In `NAMModelBrowserComponent` in `src/NAMModelBrowser.h`, add:

```cpp
StringArray favouriteModelPaths;
StringArray favouriteIRPaths;
std::unique_ptr<TextButton> favoriteButton;
std::unique_ptr<TextButton> irFavoriteButton;
bool isFavouriteModel(const NAMModelInfo& model) const;
bool isFavouriteIR(const IRFileInfo& ir) const;
void toggleFavouriteModel();
void toggleFavouriteIR();
void refreshFavouriteButtons();
const NAMModelInfo* getSelectedModel() const;
const IRFileInfo* getSelectedIR() const;
```

Create both buttons in the constructor, style them like other browser action buttons, and show text as `Star`/`Starred` or use an existing `IconManager` star if one already exists. Do not introduce a new icon dependency in this task.

- [ ] **Step 3: Load and save favorites**

In the `NAMModelBrowserComponent` constructor, load:

```cpp
favouriteModelPaths = SettingsManager::getInstance().getStringArray(kNamFavoritesSettingsKey);
favouriteIRPaths = SettingsManager::getInstance().getStringArray(kIrFavoritesSettingsKey);
```

In `toggleFavouriteModel()`:

```cpp
if (const auto* model = getSelectedModel())
{
    const String path(model->filePath);
    if (favouriteModelPaths.contains(path))
        favouriteModelPaths.removeString(path);
    else
        favouriteModelPaths.add(path);

    SettingsManager::getInstance().setStringArray(kNamFavoritesSettingsKey, favouriteModelPaths);
    refreshFavouriteButtons();
    modelList->repaint();
}
```

Implement the IR equivalent with `irList`, `getSelectedIR()`, and `kIrFavoritesSettingsKey`.

Implement the helpers directly against the current list models:

```cpp
const NAMModelInfo* NAMModelBrowserComponent::getSelectedModel() const
{
    return modelList != nullptr && modelList->getSelectedRow() >= 0 ? listModel.getModelAt(modelList->getSelectedRow())
                                                                    : nullptr;
}

const IRFileInfo* NAMModelBrowserComponent::getSelectedIR() const
{
    return irList != nullptr && irList->getSelectedRow() >= 0 ? irListModel.getFileAt(irList->getSelectedRow())
                                                              : nullptr;
}
```

- [ ] **Step 4: Fix detail panel filename clipping**

For `nameValue`, `filePathValue`, `irNameValue`, and `irFilePathValue`, keep labels inside their bounds and force ellipses:

```cpp
value->setMinimumHorizontalScale(0.72f);
value->setJustificationType(Justification::centredLeft);
```

In paint-backed detail cards, use `g.drawText(..., true)` for file/path values. If a `Label` still clips due LookAndFeel, replace only that value with a manual `drawText` region inside `paint()` and hide the label.

- [ ] **Step 5: Add padding above search/action row**

In `NAMModelBrowserComponent::resized()`, before laying out local and IR search rows, increase top separation:

```cpp
bounds.removeFromTop(compactLayout ? 8 : 12);
auto searchRow = bounds.removeFromTop(compactLayout ? 30 : 32);
```

Apply this to both Local NAM and IR tabs so the search pill, Browse Folder, and Refresh controls stop touching the header/tab area.

- [ ] **Step 6: Center detail separators inside the card**

Replace the separator paint block that currently uses `nameLabel`/`nameValue` global positions with `detailsPanelBounds`:

```cpp
const auto separatorBounds = detailsPanelBounds.toFloat().reduced(18.0f, 0.0f);
for (auto y : detailsSeparatorPositions)
{
    const auto yf = static_cast<float>(y);
    g.setColour(palette.edge.withAlpha(0.46f));
    g.drawLine(separatorBounds.getX(), yf, separatorBounds.getRight(), yf, 1.0f);
}
```

Apply the same centered-card rule to IR detail separators if IR adds its own positions in this task.

- [ ] **Step 7: Make IR Library visually match NAM Library**

Use NAM Library as the target. In the IR tab layout:

```cpp
const bool showRail = !compactLayout && bounds.getWidth() >= 720;
const int desiredDetailsWidth = compactLayout ? jlimit(170, 230, roundToInt(bounds.getWidth() * 0.44f))
                                              : jlimit(250, 350, roundToInt(bounds.getWidth() * 0.34f));
```

Mirror Local NAM card widths, rail behavior, search row padding, status row spacing, and detail-card paint style. Keep IR-specific fields: duration, sample rate, channels, file size, file path.

- [ ] **Step 8: Add IR folder setting to Preferences**

In `PreferencesDialog.h`, add:

```cpp
Label* irDirLabel;
Label* irDirValue;
TextButton* irDirBrowseButton;
```

In `PreferencesDialog.cpp`, create these under NAM Options or rename the section to `Library Folders`. Add the same settings key to the anonymous namespace near other local constants:

```cpp
constexpr const char* kIrLibraryDirectorySettingsKey = "IRLibraryDirectory";
```

Use `SettingsManager::getInstance().getString(kIrLibraryDirectorySettingsKey, "")` to populate `irDirValue`. On browse:

```cpp
FileChooser chooser("Select IR Library Folder", File(irDirValue->getText()), "", true);
if (chooser.browseForDirectory())
{
    auto selectedDir = chooser.getResult();
    SettingsManager::getInstance().setValue(kIrLibraryDirectorySettingsKey, selectedDir.getFullPathName());
    irDirValue->setText(selectedDir.getFullPathName(), dontSendNotification);
}
```

Then in `IRBrowserComponent` and the embedded IR tab of `NAMModelBrowserComponent`, prefer this setting before falling back to the current documents/default directory.

- [ ] **Step 9: Verify library behavior**

Run:

```powershell
cmake --build build --config Release --target Pedalboard3 -- /m:1
cmake --build build --config Release --target Pedalboard3_Tests -- /m:1
.\build\tests\Release\Pedalboard3_Tests.exe '[ui][regression][theme]'
.\build\tests\Release\Pedalboard3_Tests.exe '[ui][regression][visual]'
.\build\tests\Release\Pedalboard3_Tests.exe '[ui][scale]'
git -c safe.directory='C:/Users/estee/Desktop/My Stuff/Code/Antigravity/Pedalboard2' diff --check
```

Manual QA:

```powershell
.\scripts\run_d2_visual_qa.ps1 -OutputName 2026-06-12-nam-ir-library-parity -CaptureScaledDialogMatrix
```

Expected: NAM and IR library tabs have near-identical structure; long names elide; search row has breathing room; separators stay within the detail card; favorites persist after closing/reopening; IR folder can be set in Preferences.

- [ ] **Step 10: Commit Task 3**

```powershell
git -c safe.directory='C:/Users/estee/Desktop/My Stuff/Code/Antigravity/Pedalboard2' add src/NAMModelBrowser.h src/NAMModelBrowser.cpp src/PreferencesDialog.h src/PreferencesDialog.cpp src/SettingsManager.h src/SettingsManager.cpp
git -c safe.directory='C:/Users/estee/Desktop/My Stuff/Code/Antigravity/Pedalboard2' commit -m "feat: add library favorites and ir folder setting"
```

---

### Task 4: NAM Online Browser Visual Parity

**Files:**
- Modify: `src/NAMOnlineBrowser.h`
- Modify: `src/NAMOnlineBrowser.cpp`
- Test: `tests/ui_regression_harness_test.cpp`

- [ ] **Step 1: Reuse NAM Library palette structure**

In `NAMOnlineBrowser.cpp`, align local palette derivation with `makeBrowserPalette()` from `NAMModelBrowser.cpp`. If direct sharing would require moving helpers, keep this as a local close-port in `NAMOnlineBrowser.cpp` with identical token intent and no behavior sharing.

Use the same values for:

```cpp
face
face2
inset
edge
edgeHi
accent
accent2
led
text
```

- [ ] **Step 2: Rework online toolbar spacing**

In `NAMOnlineBrowserComponent::resized()`, reserve a header gap above `searchBox` and action controls:

```cpp
bounds.removeFromTop(compactLayout ? 8 : 12);
auto searchRow = bounds.removeFromTop(compactLayout ? 30 : 34);
searchBox->setBounds(searchRow.removeFromLeft(jmax(180, searchRow.getWidth() - 220)));
```

Keep existing auth, filter, download, and selection behavior.

- [ ] **Step 3: Match online details panel to NAM Library**

In `NAMOnlineBrowserComponent::paint`, keep the current details content but paint the card like NAM Library:

```cpp
Path detailsPath;
detailsPath.addRoundedRectangle(detailsBounds, 8.0f);
melatonin::DropShadow shadow{Colours::black.withAlpha(0.28f), 10, {0, 4}};
shadow.render(g, detailsPath);
ColourGradient cardGrad(palette.face2, detailsBounds.getX(), detailsBounds.getY(),
                        palette.face, detailsBounds.getX(), detailsBounds.getBottom(), false);
g.setGradientFill(cardGrad);
g.fillPath(detailsPath);
g.setColour(palette.edgeHi.withAlpha(0.36f));
g.strokePath(detailsPath, PathStrokeType(1.0f));
```

Long names and metadata must use `drawText(..., true)` or `Label::setMinimumHorizontalScale(0.72f)`.

- [ ] **Step 4: Verify online visual parity**

Run:

```powershell
cmake --build build --config Release --target Pedalboard3 -- /m:1
cmake --build build --config Release --target Pedalboard3_Tests -- /m:1
.\build\tests\Release\Pedalboard3_Tests.exe '[ui][regression][theme]'
.\build\tests\Release\Pedalboard3_Tests.exe '[ui][visual]'
git -c safe.directory='C:/Users/estee/Desktop/My Stuff/Code/Antigravity/Pedalboard2' diff --check
```

Manual QA: open NAM Library, switch Local, Online, and IR tabs. Online must feel like the same product surface as Local NAM.

- [ ] **Step 5: Commit Task 4**

```powershell
git -c safe.directory='C:/Users/estee/Desktop/My Stuff/Code/Antigravity/Pedalboard2' add src/NAMOnlineBrowser.h src/NAMOnlineBrowser.cpp
git -c safe.directory='C:/Users/estee/Desktop/My Stuff/Code/Antigravity/Pedalboard2' commit -m "style: align nam online browser with library"
```

---

### Task 5: Stage And Scratch Main-Window Affordances

**Files:**
- Modify: `src/MainPanel.cpp`
- Modify: `src/MainPanel.h`
- Modify: `src/StageView.cpp`
- Modify: `src/StageView.h`
- Modify: `src/StageLayout.cpp`
- Modify: `src/StageLayout.h`
- Test: `tests/stage_layout_test.cpp`
- Test: `tests/ui_scale_test.cpp`
- Test: `tests/scratch_recorder_test.cpp`

- [ ] **Step 1: Make Scratch panel access more obvious in footer**

In `MainPanel::resized`, keep `scratchRecordButton`, `scratchStatusLabel`, and `scratchPanelButton`, but change `scratchPanelButton` text/tooltip during construction to read as an obvious panel opener:

```cpp
scratchPanelButton->setButtonText("Scratch");
scratchPanelButton->setTooltip("Open Scratch Mode");
```

Update footer layout widths:

```cpp
const int panelW = 76;
```

Where `layoutScratchControls` uses `areaW`, preserve the fallback behavior:

1. Record + status + Scratch button when wide.
2. Record + Scratch button when medium.
3. Record only when narrow.

- [ ] **Step 2: Add a compact Scratch status affordance**

In `refreshScratchControls`, make recording/saving states visually obvious on the footer button:

```cpp
scratchPanelButton->setColour(TextButton::buttonColourId,
    stateColour.withAlpha(status.state == ScratchRecorderState::Ready ? 0.12f : 0.30f));
scratchPanelButton->setColour(TextButton::textColourOffId,
    text.withAlpha(status.state == ScratchRecorderState::Ready ? 0.82f : 0.98f));
```

Do not remove the existing record button or scratch menu commands.

- [ ] **Step 3: Keep Stage tuners persistent in Setlist and Grid**

In `StageLayout::shouldReserveTunerStrip`, change the condition to reserve a compact strip for Patch, Queue, and Grid when `showTuner` is true and the focused Tuner view is not active:

```cpp
bool shouldReserveTunerStrip(bool showTuner, bool tunerFocus, bool patchView)
{
    return showTuner && !tunerFocus;
}
```

Rename the third parameter in a follow-up cleanup only if needed, but keep the function signature in this task to reduce blast radius.

In `StageView::paint`, when `reserveTunerStrip` is true for Queue/Grid, call `drawTunerDisplay(g, tunerArea)` after the main view, just like Patch view.

- [ ] **Step 4: Fix Stage grid tile bottom bar clipping**

In `StageView::drawGridView`, any active/next bottom accent strip must be clipped to the same rounded tile path:

```cpp
Path tileClip;
tileClip.addRoundedRectangle(tile, 16.0f);
g.saveState();
g.reduceClipRegion(tileClip);
g.setColour(accent.withAlpha(isActive ? 0.78f : 0.45f));
g.fillRect(tile.getX(), tile.getBottom() - 4.0f, tile.getWidth(), 4.0f);
g.restoreState();
```

Do not draw bottom bars with raw rectangles outside that clipping region.

- [ ] **Step 5: Update Stage layout tests**

In `tests/stage_layout_test.cpp`, update the tuner-strip test to reflect persistent tuner strips:

```cpp
TEST_CASE("Stage tuner strip persists outside focused tuner view", "[ui][regression][stage][layout]")
{
    CHECK(StageLayout::shouldReserveTunerStrip(true, false, true));
    CHECK(StageLayout::shouldReserveTunerStrip(true, false, false));
    CHECK_FALSE(StageLayout::shouldReserveTunerStrip(false, false, true));
    CHECK_FALSE(StageLayout::shouldReserveTunerStrip(true, true, true));
}
```

Ensure header/footer tests still pass with the reserved strip.

- [ ] **Step 6: Verify Stage/Scratch**

Run:

```powershell
cmake --build build --config Release --target Pedalboard3 -- /m:1
cmake --build build --config Release --target Pedalboard3_Tests -- /m:1
.\build\tests\Release\Pedalboard3_Tests.exe '[ui][regression][stage][layout]'
.\build\tests\Release\Pedalboard3_Tests.exe '[ui][scale]'
.\build\tests\Release\Pedalboard3_Tests.exe '[scratch]'
.\build\tests\Release\Pedalboard3_Tests.exe '[ui][regression]'
git -c safe.directory='C:/Users/estee/Desktop/My Stuff/Code/Antigravity/Pedalboard2' diff --check
```

Manual QA:

```powershell
.\scripts\run_d2_visual_qa.ps1 -OutputName 2026-06-12-stage-scratch-affordances -CaptureScratchPanel
```

Expected: Scratch panel is obvious from the footer; Setlist and Grid have persistent tuner strips when tuner is enabled; grid accent bars stay inside rounded tile edges.

- [ ] **Step 7: Commit Task 5**

```powershell
git -c safe.directory='C:/Users/estee/Desktop/My Stuff/Code/Antigravity/Pedalboard2' add src/MainPanel.cpp src/MainPanel.h src/StageView.cpp src/StageView.h src/StageLayout.cpp src/StageLayout.h tests/stage_layout_test.cpp
git -c safe.directory='C:/Users/estee/Desktop/My Stuff/Code/Antigravity/Pedalboard2' commit -m "style: polish stage tuner and scratch affordances"
```

---

### Task 6: Effect Rack Node And Nested Graph Polish

**Files:**
- Modify: `src/SubGraphProcessor.cpp`
- Modify: `src/SubGraphProcessor.h`
- Modify: `src/SubGraphEditorComponent.cpp`
- Modify: `src/SubGraphEditorComponent.h`
- Modify: `src/PluginComponent.cpp`
- Modify: `src/PluginConnection.cpp`
- Test: `tests/subgraph_test.cpp`
- Test: `tests/master_bus_test.cpp`
- Test: `tests/integration_test.cpp`

- [ ] **Step 1: Give Effect Rack node an explicit category identity**

In `getNodeVisualStyle()` in `src/PluginComponent.cpp`, classify Effect Rack/SubGraph as a module/rack with a stable accent:

```cpp
if (containsAnyToken(text, {"effect rack", "subgraph", "rack"}))
    return {"rack", graphCategoryColour("Graph Category Modulation")};
```

Place this before the generic module fallback.

- [ ] **Step 2: Polish SubGraph editor chrome**

In `SubGraphEditorComponent::paint`, replace flat background with the same Stage/main graph visual language:

```cpp
auto bounds = getLocalBounds().toFloat();
auto& colours = ColourScheme::getInstance().colours;
ColourGradient bg(colours["Window Background"].brighter(0.05f), bounds.getX(), bounds.getY(),
                  colours["Window Background"].darker(0.12f), bounds.getX(), bounds.getBottom(), false);
g.setGradientFill(bg);
g.fillRoundedRectangle(bounds.reduced(1.0f), 8.0f);
g.setColour(colours["Plugin Border"].withAlpha(0.70f));
g.drawRoundedRectangle(bounds.reduced(0.5f), 8.0f, 1.0f);
```

Keep viewport, canvas, toolbar, plugin search, nested rack insertion, and routing behavior unchanged.

- [ ] **Step 3: Reuse graph cable/port polish inside SubGraphCanvas**

Because `PluginConnection` and `PluginComponent` are used by both main canvas and `SubGraphCanvas`, Task 1 visual improvements should already apply. Verify `SubGraphCanvas` does not paint an old background over the improved grid. If it does, change only its `paint()` background to the same field gradient/grid pattern used in `PluginField.cpp`.

- [ ] **Step 4: Verify Effect Rack graph behavior**

Run:

```powershell
cmake --build build --config Release --target Pedalboard3 -- /m:1
cmake --build build --config Release --target Pedalboard3_Tests -- /m:1
.\build\tests\Release\Pedalboard3_Tests.exe '[subgraph]'
.\build\tests\Release\Pedalboard3_Tests.exe '[masterbus][subgraph]'
.\build\tests\Release\Pedalboard3_Tests.exe '[integration][subgraph]'
.\build\tests\Release\Pedalboard3_Tests.exe '[ui][regression]'
git -c safe.directory='C:/Users/estee/Desktop/My Stuff/Code/Antigravity/Pedalboard2' diff --check
```

Expected: nested graph routing/state tests pass; editor looks integrated with the polished main graph; no nested-rack workflow is removed.

- [ ] **Step 5: Commit Task 6**

```powershell
git -c safe.directory='C:/Users/estee/Desktop/My Stuff/Code/Antigravity/Pedalboard2' add src/SubGraphProcessor.cpp src/SubGraphProcessor.h src/SubGraphEditorComponent.cpp src/SubGraphEditorComponent.h src/PluginComponent.cpp src/PluginConnection.cpp
git -c safe.directory='C:/Users/estee/Desktop/My Stuff/Code/Antigravity/Pedalboard2' commit -m "style: polish effect rack graph surfaces"
```

---

## Final Verification

After all tasks:

```powershell
cmake --build build --config Release --target Pedalboard3 -- /m:1
cmake --build build --config Release --target Pedalboard3_Tests -- /m:1
.\build\tests\Release\Pedalboard3_Tests.exe '[ui][regression]'
.\build\tests\Release\Pedalboard3_Tests.exe '[ui][visual]'
.\build\tests\Release\Pedalboard3_Tests.exe '[ui][scale]'
.\build\tests\Release\Pedalboard3_Tests.exe '[scratch]'
.\build\tests\Release\Pedalboard3_Tests.exe '[subgraph]'
.\build\tests\Release\Pedalboard3_Tests.exe '[nam]'
git -c safe.directory='C:/Users/estee/Desktop/My Stuff/Code/Antigravity/Pedalboard2' diff --check
```

Visual QA:

```powershell
.\scripts\run_d2_visual_qa.ps1 -OutputName 2026-06-12-mockup-polish-phase2 -CaptureScaledFooterMatrix -CaptureScaledDialogMatrix -CaptureScratchPanel
```

Manual sign-off checklist:

- Cable glow is subtle and follows the bezier path.
- NAM Loader ports do not visually clip each other.
- Node chrome gradients feel closer to the mockup.
- Graphpaper line grid is the default visible grid.
- Tuner node, NAM Loader, Mixer, Splitter, Label node, and Effect Rack node feel like first-class nodes.
- Label node has bespoke display/edit behavior and no M/E/B buttons.
- Scratch panel has an obvious main-window button and existing scratch commands still work.
- NAM Library and IR Library tabs look nearly identical, with NAM Library as the target.
- Long file/model names elide inside detail pills and cards.
- Search/action row has enough top padding.
- Detail separators stay inside detail cards.
- Favorites persist for NAM and IR libraries.
- IR folder can be set in Preferences and affects IR browser startup.
- NAM Online browser matches the NAM Library design language.
- Stage Grid bottom bars are clipped inside rounded tiles.
- Stage Setlist and Grid keep persistent tuner strips when tuner is enabled.

## Push Policy

This repository is single-developer. Do not open pull requests. After each successful task commit, push only when the user asks to back up or publish work:

```powershell
git -c safe.directory='C:/Users/estee/Desktop/My Stuff/Code/Antigravity/Pedalboard2' push
```
