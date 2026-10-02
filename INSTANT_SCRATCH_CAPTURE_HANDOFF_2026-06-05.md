# Instant Scratch Capture Handoff - 2026-06-05

This handoff preserves the original Scratch Capture implementation evidence
and manual acceptance procedure. Current priorities belong to `ROADMAP.md` and
`STATE_OF_PROJECT.md`; current usage and architecture are in `HOWTO.md` and
`ARCHITECTURE.md`.

Source reconciliation 2026-10-02: the inspected checkout is
`D:\Code\Pedalboard2`, branch `codex/rt-hosting-sprint`. This documentation pass
did not build, run tests, or verify a real audio interface. The June evidence
below is historical, not a claim about this checkout's current test results.

Accepted expansion, not yet implemented: a disk-backed take library, real
raw/wet waveforms, in-app wet audition, current-rig reamp without manual
FilePlayer wiring, saved capture-time rig context, and modest existing-style UI
polish. The
[Scratch foundations design](docs/superpowers/specs/2026-10-02-scratch-foundations-design.md)
is approved; the
[implementation plan](docs/superpowers/plans/2026-10-02-scratch-foundations.md)
awaits review and an execution choice. Implementation has not started. This
handoff continues to describe the current baseline and its historical evidence.

## Historical Baseline (June 2026)

- Original branch: `codex/instant-scratch-capture`
- June 9 continuation branch: `codex/pedalboard-remix-ui-polish`
- Original handoff head commit: `ed2532d fix: stop scratch capture on disruptive changes`
- June 9 pre-polish checkpoint: `5aec6aa docs: reconcile active roadmap`
- Tracked worktree status at original handoff creation: clean before this document was added.
- At the original handoff, `documentation/research/` was unrelated untracked
  work. Inspect current Git status rather than assuming that old status still
  applies; preserve unrelated edits.

Original branch commits:

- `ed2532d fix: stop scratch capture on disruptive changes`
- `8469e70 feat: add scratch capture controls`
- `7015cb4 feat: tap scratch recorder in audio callback`
- `f29fa6e feat: add scratch recorder core`
- `600f097 feat: wire scratch take tests`
- `c89e27d fix: harden scratch take metadata`
- `3e33080 docs: plan instant scratch capture implementation`
- `72c6b15 docs: design instant scratch capture`

Primary planning docs:

- `docs/superpowers/specs/2026-06-04-instant-scratch-capture-design.md`
- `docs/superpowers/plans/2026-06-04-instant-scratch-capture.md`
- `docs/superpowers/specs/2026-06-09-pedalboard-remix-ui-upgrade.md`

## Product Intent

The user wants Pedalboard to support this workflow:

1. Plug in guitar.
2. Start Pedalboard.
3. Press an obvious record control.
4. Capture a scratch idea immediately.
5. Save both what the user heard and the raw DI so the take can be reamped later.

Important product constraints from the conversation:

- This is not an AI feature.
- It should not be bloat or a vague primitive.
- It should reuse the existing recording backbone where practical.
- V1 is manual start/stop, not retrospective capture.
- Capture must be global/app-level, not an Audio Recorder graph node.
- Every take should simultaneously capture raw pre-chain input and wet post-chain output.

## What Is Implemented

### Scratch Take Model

Files:

- `src/ScratchTake.h`
- `src/ScratchTake.cpp`
- `tests/scratch_recorder_test.cpp`
- `CMakeLists.txt`
- `tests/CMakeLists.txt`

Behavior:

- Creates take folders under the scratch root.
- Uses date/timestamp and sanitized patch names.
- Handles folder collisions by appending numeric suffixes.
- Writes `take.json`.
- Metadata includes patch/document/device context, sample rate, channel counts, gain values, raw/wet file paths, duration, completion state, and failure reason.
- Gain metadata is the two master gain values at start, not per-channel values
  or a history. Patch name/index/document path do not constitute a rig snapshot.
- `take.json` stores one duration (the smaller raw/wet count), not independent
  raw/wet sample counters. There is no metadata reader/library reload path.

### Scratch Recorder Core

Files:

- `src/ScratchRecorder.h`
- `src/ScratchRecorder.cpp`
- `tests/scratch_recorder_test.cpp`

Behavior:

- `ScratchRecorder` owns state, current take metadata, recent take list, raw/wet sinks, and writer thread.
- The recent list is in-memory only, newest-first, capped at eight. Restarting
  clears the panel list, not the saved files; its `total` label is not a disk
  library count.
- `ThreadedWavSinkFactory` creates `AudioFormatWriter::ThreadedWriter` backed WAV sinks.
- Refuses start when raw input or wet output channel counts are missing.
- Writes raw and wet blocks through audio-callback entry points.
- Stops through an async/message-thread finalization path.
- Marks takes failed if raw/wet sample counts do not match or a write fails.
- Marks device-change and patch-change interruptions as incomplete with explicit failure reasons:
  - `Audio device changed during scratch capture`
  - `Patch changed during scratch capture`

Real-time safety notes:

- Audio callback work is limited to atomic state checks, existing channel pointer forwarding, `ThreadedWriter::write()`, and atomic counters.
- File creation, writer opening, writer closing, metadata write, and UI updates are outside the audio callback.
- Finalization gates new writes and waits for active writes before destroying
  sinks; the 250 ms drain warning records failure but still waits for safety.
- `MeteringProcessorPlayer` holds an atomic raw observer pointer to `ScratchRecorder`; it does not own the recorder.

### Audio Callback Taps

File:

- `src/MainPanel.h`

Behavior:

- Raw tap is inside `MeteringProcessorPlayer::audioDeviceIOCallbackWithContext()` before master input gain and before graph processing.
- Wet tap is after graph processing, master bus insert, and master output gain.
- `MainPanel` clears the recorder pointer before removing the audio callback during destruction.

### MainPanel Commands And Footer Controls

Files:

- `src/MainPanel.h`
- `src/MainPanel.cpp`

Behavior:

- Adds app command IDs appended to the existing enum:
  - `ScratchCaptureToggle`
  - `ScratchPanelOpen`
  - `ScratchRevealFolder`
- Adds File menu entries:
  - `Start/Stop Scratch Capture`
  - `Open Scratch Panel`
  - `Reveal Scratch Ideas Folder`
  - `Choose Scratch Ideas Folder`
  - `Reset Scratch Ideas Folder`
- Adds default shortcut for capture toggle:
  - Command/Ctrl + Shift + R
- Adds compact footer controls:
  - `REC` / `STOP`
  - status label
  - `Takes` button
- Persists custom scratch destination via `scratchRootDirectory`.
- Adds wet preview, raw reamp, and take reveal helpers for the scratch panel.
- Footer layout has progressive fallback:
  - full scratch strip when space allows
  - record + takes when tighter
  - record only when very tight
- Scratch capture is stopped before patch loads and when the audio device changes.

### Scratch Panel

Files:

- `src/ScratchPanel.h`
- `src/ScratchPanel.cpp`
- `CMakeLists.txt`

Behavior:

- Non-modal utility panel.
- Shows a hero record/stop button, reveal button, choose/reset destination controls, status, elapsed field, RAW/WET context, and recent takes.
- Uses `juce::Component::SafePointer<MainPanel>` instead of holding a raw recorder reference.
- If the owning `MainPanel` is gone, the timer stops and the panel disables its buttons instead of dereferencing stale state.
- Record button routes through `MainPanel::toggleScratchCapture()`.
- Reveal button routes through `MainPanel::revealScratchFolder()`.
- Recent take rows show date/time, patch context, duration, RAW/WET availability, and action buttons.
- Wet `Play` opens `wet.wav` with the OS handler when a complete wet file exists.
- Raw `Reamp` adds a `FilePlayerProcessor` loaded with `raw.wav` when a complete raw file exists.
- Reamp does not connect the node, restore the recorded patch, or export a new
  wet file; the user must route and control the file player in the current graph.
- Panel lane traces and take thumbnails are decorative (the latter derived
  from the take ID), not measurements or decoded audio waveforms.

Additional layout/presentation helpers:

- `src/ScratchPanelLayout.h`
- `src/ScratchPanelPresentation.h`
- Focused tests cover non-overlapping row/destination actions and active elapsed display.

## Historical Verification (Not Rerun 2026-10-02)

Verification recorded by the original 2026-06-05 handoff:

```powershell
cmake --build build --config Release --target Pedalboard3 -- /m:1
cmake --build build --config Release --target Pedalboard3_Tests -- /m:1
ctest --test-dir build -C Release --output-on-failure -R Scratch
ctest --test-dir build -C Release --output-on-failure
```

Observed results:

- `Pedalboard3` Release app target built successfully.
- `Pedalboard3_Tests` Release test target built successfully.
- Scratch tests: 9/9 passed.
- Full Release CTest suite: 200/200 passed.
- `git diff --check` was clean before Task 7 commit.

Consolidation verification recorded on 2026-06-09:

- `git diff --check`: passed; only normal CRLF working-copy warnings were reported.
- `cmake --build build --config Release --target Pedalboard3_Tests -- /m:1`: passed.
- `.\build\tests\Release\Pedalboard3_Tests.exe "[scratch]"`: passed, 126 assertions in 17 test cases.
- `.\build\tests\Release\Pedalboard3_Tests.exe "[ui][scale]"`: passed, 55 assertions in 9 test cases.
- `cmake --build build --config Release --target Pedalboard3 -- /m:1`: passed and produced `build\Pedalboard3_artefacts\Release\Pedalboard3.exe`.

Expected warning noise:

- The app and test builds emit existing JUCE deprecation, hidden-member, and unused-variable warnings across unrelated files.
- No new compile or linker errors were observed.

## Manual Verification Not Yet Done

The inherited June handoff records no real audio-interface/guitar smoke test;
the 2026-10-02 source reconciliation did not perform one either. This remains
pending unless a later dated hardware result supersedes it.

Next agent should run this before calling the feature user-ready:

1. Launch Pedalboard from the branch build.
2. Select a real guitar/audio input and output.
3. Confirm the footer `REC` control is visible.
4. Press `REC`.
5. Play guitar through an audible chain.
6. Press `STOP`.
7. Open `Takes` or use `Reveal Scratch Ideas Folder`.
8. Confirm the take folder contains:
   - `raw.wav`
   - `wet.wav`
   - `take.json`
9. Confirm `take.json` reports `complete: true`; inspect both WAV files for
   equal sample-frame counts and sample rate. JSON's single `durationSamples`
   value should match each completed WAV, but cannot independently prove both.
10. Confirm `raw.wav` is pre-chain DI and `wet.wav` is what was heard.
11. Start another recording, switch patches, and confirm capture stops with `Patch changed during scratch capture`.
12. Start another recording, change audio device settings, and confirm capture stops with `Audio device changed during scratch capture`.
13. Confirm no Audio Recorder node is added to the graph.
14. Confirm `Play` launches the wet WAV externally; confirm `Reamp` adds an
    unconnected raw-file player and verify it after routing it manually.
15. Record commit, interface/driver, channels, sample rate, buffer size, and
    pass/fail observations with a date.

## Known Gaps And Risks

### Panel Elapsed Display

Updated 2026-06-09: fixed.

`ScratchPanelPresentation::formatElapsedLabel()` now uses the active take sample rate while recording, falling back to the last take after capture. `tests/scratch_recorder_test.cpp` includes a focused regression proving active recording elapsed labels advance from `00:01` to `00:02` at 48 kHz.

### Footer Visual QA

Historical 2026-06-09 evidence: screenshots were captured after adding scratch
controls. These are not fresh captures of the current checkout.

Command used:

```powershell
& 'C:\Program Files\PowerShell\7\pwsh.exe' -ExecutionPolicy Bypass -File scripts\run_d2_visual_qa.ps1 -OutputName 2026-06-09-scratch-footer -CaptureScaledFooterMatrix
```

Evidence lives at `documentation/qa/2026-06-09-scratch-footer`. Narrow captures at `125%`, `150%`, `175%`, and `200%` keep `REC`, `Takes`, patch selection, transport, UI Scale, CPU meter, FX, and IN/OUT gain controls visible and reachable.

Historical 2026-06-09 follow-up: `documentation/qa/2026-06-09-main-footer-scale-v2`
recorded the earlier two-row breakpoint, shortened `CPU` label, and high-scale
footer visibility at that checkpoint.

### Playback And Reamp Scope

Updated 2026-06-09: small take actions are implemented.

- `Play` opens a completed `wet.wav` externally.
- `Reamp` adds an unconnected `FilePlayerProcessor` node for a completed `raw.wav`.
- `Reveal` opens the take folder.

This describes the implemented scope. The expansion design is approved, with
implementation-plan review and execution choice pending as linked above; do
not treat planned audition/reamp behavior as already available.

### Scratch Folder Setting

Updated 2026-06-09: V1 destination selection is implemented in the scratch panel and app menu.

Current default behavior remains:

- user application data directory
- `Pedalboard3/Scratch Ideas`

The chosen folder is persisted through `SettingsManager` key `scratchRootDirectory`. Reset returns to the app-data default. There is no Preferences mirror yet; add one only if the user asks or manual QA shows the menu/panel path is not discoverable enough.

### Subagent Review Note

Two attempted lightweight code-review subagents timed out and were closed. Local review found and fixed one important lifetime issue before commit:

- The initial `ScratchPanel` held a raw `ScratchRecorder&`.
- It now holds a `Component::SafePointer<MainPanel>` and calls MainPanel helper methods.

## Reference Code And Reuse Notes

Internal source inspected and used:

- `src/RecorderProcessor.cpp`
  - Reused the existing JUCE WAV + `ThreadedWriter` recording pattern conceptually.
- `src/LooperProcessor.cpp`
  - Confirmed existing app recording patterns and warning baseline.
- `src/MainPanel.h`
  - Located `MeteringProcessorPlayer` callback points for raw/wet taps.
- `src/MainPanel.cpp`
  - Reused app command/menu/footer/toast conventions.
- `src/JuceHelperStuff.cpp`
  - Verified `showNonModalDialog()` takes ownership of the content component.

External source reuse:

- No third-party implementation code was copied.
- No new dependency was added.

Library documentation checked:

- Context7 was used for current JUCE writer ownership/API behavior around `WavAudioFormat::createWriterFor()` and `AudioFormatWriterOptions`.

## Resume And Verification Guidance

Recommended next sequence:

1. Read current status/roadmap and this file; treat the June spec/plan as the
   baseline, not approval for an expanded feature set.
2. Run `git status --short --branch` and confirm only intended work is present.
3. Do the manual hardware capture verification above.
4. Confirm scratch destination choose/reset behaves correctly outside a recording and is disabled while recording.
5. Confirm wet `Play`/raw `Reamp` require a complete take and their WAV; `Reveal`
   requires the directory. Check the actual external-player/manual-routing flow.
6. Use June 9 QA folders as historical references; capture new UI evidence only
   when needed for the current change.
7. Follow `AGENTS.md`: this is a single-developer repository, with no PR by
   default. Commit/push/merge only as authorized by the current task; create a
   PR only if explicitly requested.

Suggested focused verification against the existing build (coordinator-owned;
no clean or alternate build). Run the full suite at an explicit integration
checkpoint or for a broad-reaching change, not automatically for a doc update:

```powershell
git -c safe.directory=D:/Code/Pedalboard2 status --short --branch
cmake --build build --config Release --target Pedalboard3 -- /m:1
cmake --build build --config Release --target Pedalboard3_Tests -- /m:1
.\build\tests\Release\Pedalboard3_Tests.exe "[scratch]"
```

Verification classification: active development; saved audio/metadata and
real-time/lifetime boundaries are durable because defects can lose or
misrepresent takes, race writer destruction, or disrupt audio. Panel layout is
volatile; do not freeze it with exhaustive visual snapshots while redesigning.

## Do Not Forget

- Preserve unrelated local edits and stage only work authorized by the task.
- Do not claim real guitar capture works until manual audio-interface verification is done.
- Do not convert this into a graph node unless the user explicitly changes the product direction.
- Keep future audio callback changes RT-safe: no locks, no allocation, no file work, no UI calls.
