# Tuner closeout visual sign-off — 2026-09-16

App: `build/Pedalboard3_artefacts/Release/Pedalboard3.exe` (built 2026-09-16 16:10)
Capture: `scripts/run_d2_visual_qa.ps1 -NodeSnapshotsOnly -UiScalePercent 100`
Host: Windows, OS scale 175% (DPI 168), Pedalboard UI scale 100%

`app-node-tuner.png` is 430 x 350, matching `TunerControl` / `TunerProcessor::getSize()`.

Observed on the snapshot:

- Header, NEEDLE/DRIFT/STRINGS/BYPASS, coarse cents strip, needle meter, and audio pins are present and unclipped.
- The node is in the waiting/no-signal state, so the note glyph is empty by design; the reserved note area is not overlapped by the meter or controls.
- Close control and rounded chassis remain inside the 430 x 350 bounds.

This is a node-bitmap capture, not a substitute for playing a note in the running app. Live confirmation of a detected-note glyph is still useful.
