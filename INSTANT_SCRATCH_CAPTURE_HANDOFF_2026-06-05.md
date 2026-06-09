# Instant Scratch Capture Handoff - 2026-06-05

This handoff lets the next agent resume the Instant Scratch Capture branch without replaying the session. It documents what is implemented, what evidence exists, where the risks are, and what should happen next.

Updated 2026-06-09: the active continuation branch is `codex/pedalboard-remix-ui-polish`, which layers mockup-informed Scratch Capture and footer polish on top of the original scratch-capture branch.

## Current Baseline

- Original branch: `codex/instant-scratch-capture`
- Current continuation branch: `codex/pedalboard-remix-ui-polish`
- Original handoff head commit: `ed2532d fix: stop scratch capture on disruptive changes`
- Current pre-polish checkpoint: `5aec6aa docs: reconcile active roadmap`
- Tracked worktree status at original handoff creation: clean before this document was added.
- Unrelated untracked directory present: `documentation/research/`
  - Do not stage it unless the user explicitly asks to include those research notes.

Recent branch commits:

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

### Scratch Recorder Core

Files:

- `src/ScratchRecorder.h`
- `src/ScratchRecorder.cpp`
- `tests/scratch_recorder_test.cpp`

Behavior:

- `ScratchRecorder` owns state, current take metadata, recent take list, raw/wet sinks, and writer thread.
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

Additional layout/presentation helpers:

- `src/ScratchPanelLayout.h`
- `src/ScratchPanelPresentation.h`
- Focused tests cover non-overlapping row/destination actions and active elapsed display.

## Verification Already Run

Fresh verification at the end of implementation:

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

Latest consolidation verification on 2026-06-09:

- `git diff --check`: passed; only normal CRLF working-copy warnings were reported.
- `cmake --build build --config Release --target Pedalboard3_Tests -- /m:1`: passed.
- `.\build\tests\Release\Pedalboard3_Tests.exe "[scratch]"`: passed, 126 assertions in 17 test cases.
- `.\build\tests\Release\Pedalboard3_Tests.exe "[ui][scale]"`: passed, 55 assertions in 9 test cases.
- `cmake --build build --config Release --target Pedalboard3 -- /m:1`: passed and produced `build\Pedalboard3_artefacts\Release\Pedalboard3.exe`.

Expected warning noise:

- The app and test builds emit existing JUCE deprecation, hidden-member, and unused-variable warnings across unrelated files.
- No new compile or linker errors were observed.

## Manual Verification Not Yet Done

No real audio-interface/guitar capture smoke test has been performed in this session.

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
9. Confirm `take.json` reports complete true and equal raw/wet duration samples.
10. Confirm `raw.wav` is pre-chain DI and `wet.wav` is what was heard.
11. Start another recording, switch patches, and confirm capture stops with `Patch changed during scratch capture`.
12. Start another recording, change audio device settings, and confirm capture stops with `Audio device changed during scratch capture`.
13. Confirm no Audio Recorder node is added to the graph.

## Known Gaps And Risks

### Panel Elapsed Display

Updated 2026-06-09: fixed.

`ScratchPanelPresentation::formatElapsedLabel()` now uses the active take sample rate while recording, falling back to the last take after capture. `tests/scratch_recorder_test.cpp` includes a focused regression proving active recording elapsed labels advance from `00:01` to `00:02` at 48 kHz.

### Footer Visual QA

Updated 2026-06-09: fresh screenshot evidence was captured after adding scratch controls.

Command used:

```powershell
& 'C:\Program Files\PowerShell\7\pwsh.exe' -ExecutionPolicy Bypass -File scripts\run_d2_visual_qa.ps1 -OutputName 2026-06-09-scratch-footer -CaptureScaledFooterMatrix
```

Evidence lives at `documentation/qa/2026-06-09-scratch-footer`. Narrow captures at `125%`, `150%`, `175%`, and `200%` keep `REC`, `Takes`, patch selection, transport, UI Scale, CPU meter, FX, and IN/OUT gain controls visible and reachable.

Updated 2026-06-09: an additional main-footer scale follow-up was captured at `documentation/qa/2026-06-09-main-footer-scale-v2`. The two-row breakpoint now engages earlier, the CPU label is shortened to `CPU`, and narrow captures keep the existing footer functions visible at high Pedalboard UI scale.

### Playback And Reamp Scope

Updated 2026-06-09: small take actions are implemented.

- `Play` opens a completed `wet.wav` externally.
- `Reamp` adds a `FilePlayerProcessor` node for a completed `raw.wav`.
- `Reveal` opens the take folder.

Do not expand this into a DAW-style timeline or library unless the user explicitly asks; immediate capture and reampable raw/wet files are the core value.

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

## Agent Next Steps

Recommended next sequence:

1. Read this file, the spec, and the plan.
2. Run `git status --short --branch` and confirm only intended work is present.
3. Do the manual hardware capture verification above.
4. Confirm scratch destination choose/reset behaves correctly outside a recording and is disabled while recording.
5. Confirm wet `Play`, raw `Reamp`, and `Reveal` actions enable only when their files exist.
6. Review the two June 9 footer QA folders if UI-scale evidence is needed before PR.
7. Push the branch and open a draft PR after manual verification.

Suggested commands:

```powershell
git -c safe.directory="C:/Users/estee/Desktop/My Stuff/Code/Antigravity/Pedalboard2" status --short --branch
cmake --build build --config Release --target Pedalboard3 -- /m:1
ctest --test-dir build -C Release --output-on-failure -R Scratch
ctest --test-dir build -C Release --output-on-failure
```

Suggested PR summary:

```text
Adds app-level Instant Scratch Capture for manual raw/wet take recording.

- Creates ScratchTake metadata/take folders with raw.wav, wet.wav, take.json
- Adds ScratchRecorder with ThreadedWriter-backed raw/wet sinks
- Taps raw pre-chain input and wet post-chain output in MeteringProcessorPlayer
- Adds footer REC/Takes controls and File menu commands
- Adds a non-modal ScratchPanel with hero record control, RAW/WET context, destination controls, and safe MainPanel lifetime handling
- Adds recent take date labels and scoped Play/Reamp/Reveal actions
- Stops/marks takes incomplete on patch switch and audio device changes
- Covers scratch take/recorder behavior with focused tests
```

## Do Not Forget

- Do not stage `documentation/research/` unless explicitly requested.
- Do not claim real guitar capture works until manual audio-interface verification is done.
- Do not convert this into a graph node unless the user explicitly changes the product direction.
- Keep future audio callback changes RT-safe: no locks, no allocation, no file work, no UI calls.
