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

Status: deferred manual/visual pass after the next native build.

Scope:

- Branch: `codex/secondary-mockup-polish`
- Source reference: `releases/design-handoffs/pedalboard-remix/pedalboard-remix/project/Pedalboard 3 Demo.html`
- Reference screenshot: `documentation\qa\2026-06-09-mockup-reference\nam-browser-mockup-msedge.png`

Steps:

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
12. Repeat the visual pass in dark, light/daylight, and synthwave themes.
13. Repeat at 75%, 100%, 150%, and 200% Pedalboard UI Scale, confirming no recovery controls are clipped.
