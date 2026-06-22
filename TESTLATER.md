# TESTLATER

Deferred manual QA that needs real hardware or user interaction.

## Instant Scratch Capture Hardware Smoke

Status: not completed in this session.

Build to test:

- Branch: `codex/pedalboard-remix-ui-polish`
- Commit: `c7b30d7 feat: polish scratch capture workflow`
- App: `build\Pedalboard3_artefacts\Release\Pedalboard3.exe`

Steps:

1. Launch Pedalboard3.
2. Select a real guitar/audio-interface input and audible output.
3. Confirm footer `REC` and `Takes` controls are visible.
4. Press `REC`.
5. Play a short idea through an audible processed chain.
6. Press `STOP`.
7. Open `Takes` or use `Reveal`.
8. Confirm the take folder contains `raw.wav`, `wet.wav`, and `take.json`.
9. Confirm `take.json` reports a complete take and matching raw/wet duration samples.
10. Confirm `raw.wav` is pre-chain DI and `wet.wav` is the processed sound heard during recording.
11. Start another recording, switch patches, and confirm capture stops cleanly with `Patch changed during scratch capture`.
12. Start another recording, change audio-device settings, and confirm capture stops cleanly with `Audio device changed during scratch capture`.
13. Confirm no Audio Recorder node is added to the graph.

## Scratch Panel Interaction Check

Status: visual/function smoke still useful after a real take exists.

Steps:

1. Open the Scratch Capture panel.
2. Confirm elapsed time advances while recording.
3. Confirm destination `Choose`, `Reset`, and `Reveal` are disabled while recording and enabled after stop.
4. Confirm recent take rows show date, time, patch context, RAW/WET availability, and duration.
5. Confirm `Play` opens `wet.wav` only when the complete wet file exists.
6. Confirm `Reamp` adds a file-player node for `raw.wav` only when the complete raw file exists.
7. Confirm `Reveal` opens the take folder.

## UI Scale Spot Check

Status: automated and screenshot QA passed; manual spot check remains optional.

Evidence already captured:

- `documentation\qa\2026-06-09-scratch-footer`
- `documentation\qa\2026-06-09-main-footer-scale-v2`

Manual spot check:

1. Set Pedalboard UI Scale to 150%, then 200%.
2. Narrow the window.
3. Confirm patch controls, transport, tempo, Scratch `REC`/`Takes`, input/output gain, `FX`, `Manage`, `Fit`, CPU, and UI Scale remain visible/recoverable.

## Mockup Polish Visual QA

Status: automated screenshot pass completed on the current branch; manual interaction checks remain.

Scope:

- Branch: `codex/rt-hosting-sprint`
- Latest verified commit: `2486d56 fix: polish tone and tuner utility nodes`
- Source reference: `releases/design-handoffs/pedalboard-remix/pedalboard-remix/project/Pedalboard 3 Demo.html`
- Reference screenshot: `documentation\qa\2026-06-09-mockup-reference\nam-browser-mockup-msedge.png`

Evidence captured:

- `documentation\qa\2026-06-21-polish-audit-nodes`
- `documentation\qa\2026-06-21-polish-audit-dialogs`
- `documentation\qa\2026-06-21-tone-tuner-polish`
- `audits\audit-20260621-polish-status.md`

Covered by current screenshots:

- NAM Loader and IR Loader node text roles are readable across node snapshots.
- NAM Library normal-size chrome is centered and local rows/detail card are readable.
- IR Browser normal-size detail card is readable.
- Plugin search visible list no longer shows duplicate internal `Mixer`/`Splitter` variants in the captured view.
- Effect Rack node has one clear `Open` affordance and no redundant `Map` button.
- Notes node shows the empty hint again.
- Tone Generator pitch-row overlap is fixed in `2026-06-21-tone-tuner-polish`.
- Tuner mode row has an integrated segmented control in `2026-06-21-tone-tuner-polish`.

Remaining manual checks:

1. Launch Pedalboard3 and open a patch with several plugin nodes and visible cables.
2. In `Options > Graph Grid`, switch between `Dots`, `Lines`, and `Off`.
3. Confirm the grid mode changes immediately, persists after app restart, and does not affect node dragging, cable hit testing, or double-click add-plugin.
4. Check shared TextButton polish in the footer, plugin search, NAM browser, IR browser, Scratch panel, and dialog action rows.
5. Check shared linear slider polish on CPU, master input/output gain, and Audio I/O per-channel gain sliders.
6. Add a wrapped plugin with automatable parameters and confirm its compact node parameter strips are readable, draggable, mouse-wheel adjustable, and double-click resettable.
7. Toggle `Options > Node Parameter Controls` off and on, then confirm edit, mappings, bypass, delete, pins, cable hit testing, and graph connections remain intact.
8. In Stage Grid with a set larger than eight patches, confirm visible Bank A/B/C selectors jump to the first patch in the selected bank and do not interfere with tile clicks.
9. Add or open a standalone IR Loader node, then confirm empty IR slots, loaded IR slots, the footer status, Load/Browse/Clear buttons, Blend/Mix sliders, and Lo/Hi Cut sliders remain readable and functional.
10. Open the standalone IR Browser, select an IR, and confirm the right preview card shows the cabinet glyph, selected name, READY chip, local source chip, duration, sample rate, channel count, and file size without clipping.
11. If a listed IR file has been moved or deleted, confirm the preview shows MISSING and `Load IR` stays disabled.
12. Open the local NAM browser, select a `.nam` model, and confirm the right preview card shows the architecture glyph, selected model name, author, READY chip, model-type chip, tone chip, architecture, sample rate, loudness, file size/path, and footer status without clipping.
13. Search the local NAM browser by file name, model type, metadata author/maker, and folder path, confirming matching rows remain selectable and double-click/load behavior is unchanged.
14. If a listed local NAM file has been moved or deleted, confirm the preview shows MISSING and `Load Model` / `Delete Model` stay disabled.
15. Repeat the visual pass in dark, light/daylight, and synthwave themes.
16. Repeat at realistic 75%, 100%, and 150% Pedalboard UI Scale window sizes, confirming no recovery controls are clipped.

Note: the fixed-size 200% dialog captures in `2026-06-21-polish-audit-dialogs` are stress cases, not realistic acceptance targets. Revisit high-scale dialog layout only after defining realistic high-DPI window sizes.

## NAM A1/A2 Browser Smoke

Status: local visual pills/detail cards are covered by current screenshots; online/authenticated behavior still needs one manual pass after native launch.

Evidence already captured:

- `documentation\qa\2026-06-21-polish-audit-dialogs\dialog-nam-browser-daylight.png`
- `documentation\qa\2026-06-21-polish-audit-dialogs\dialog-nam-browser-midnight.png`
- `documentation\qa\2026-06-21-polish-audit-dialogs\workflow-scaled-dialog-nam-browser-150-normal.png`

Remaining manual steps:

1. Open the NAM Library local browser with at least one A1 `.nam` model and one A2 `.nam` container model available.
2. Confirm local rows show the correct A1 or A2 architecture pill without clipping.
3. Select and load an A1 model, then confirm the loaded NAM node/control still identifies the model as A1.
4. Select and load an A2 model, then confirm the loaded NAM node/control still identifies the model as A2.
5. Open the online browser while authenticated, select a model, and confirm a visible download action is available.
6. Confirm the online A1/A2 architecture filter changes the result set sensibly and does not label every result as A2.
7. Download one online model, refresh local results, and confirm the new local row shows the correct architecture pill.
8. Reopen the NAM detail card at realistic 75%, 100%, and 150% UI Scale window sizes and confirm labels, pills, and footer status do not clip.

## ReverbSC Direct Node Visual Smoke

Status: still needs manual interaction pass. The June 21 automated node snapshot script does not include ReverbSC.

Steps:

1. Add a ReverbSC internal node from the plugin browser.
2. Confirm the node shows direct-painted Mix, Feedback, Damping, Width, and Output controls.
3. Drag each direct control and confirm the visible value updates immediately.
4. Confirm dragging the controls does not move the graph node unless the drag starts outside a control.
5. Save the patch, reload it, and confirm the five ReverbSC values restore.
6. Toggle bypass and mappings from the node footer and confirm the direct-painted control surface remains readable.
7. Check dark, light/daylight, and synthwave themes.
8. Repeat at 75%, 100%, 150%, and 200% Pedalboard UI Scale, confirming the control text and pills do not clip.

## Tuner And Meter Instrument Smoke

Status: automated node/source/RT smoke is done for the June 22 tuner UI pass; real instrument and Stage/global verification remain planned.

Reference plan:

- `docs\superpowers\plans\2026-06-22-tuner-meter-upgrade.md`

Steps:

1. Launch Pedalboard3 with a real guitar/audio-interface input and audible output.
2. Add a Tuner node and also open Stage/global tuner if available.
3. Compare node tuner and Stage/global tuner on low E, A, D, G, B, and high E.
4. Test quiet signal near the no-signal threshold and confirm stale notes do not flicker as valid detections.
5. Test deliberate sharp/flat offsets against a trusted external tuner and record rough cent agreement.
6. Confirm `Needle`, `DRIFT`, and string views are labeled truthfully for the backend currently implemented.
7. Confirm mute-output still silences the tuner path when expected.
8. Confirm Stage View remains responsive while tuner is active.
9. Confirm Audio I/O node meters remain live while tuning.
10. Confirm Soundcheck/device meters distinguish silence, normal signal, and clipping once the meter upgrade lands.
11. Repeat core visual checks at 75%, 100%, and 150% Pedalboard UI Scale.
