# Instant Scratch Capture Handoff - 2026-06-05

This handoff lets the next agent resume the Instant Scratch Capture branch without replaying the session. It documents what is implemented, what evidence exists, where the risks are, and what should happen next.

## Current Baseline

- Branch: `codex/instant-scratch-capture`
- Head commit at handoff creation: `ed2532d fix: stop scratch capture on disruptive changes`
- Tracked worktree status at handoff creation: clean before this document was added.
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
- Adds default shortcut for capture toggle:
  - Command/Ctrl + Shift + R
- Adds compact footer controls:
  - `REC` / `STOP`
  - status label
  - `Takes` button
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
- Shows record/stop button, reveal button, status, elapsed field, and recent takes.
- Uses `juce::Component::SafePointer<MainPanel>` instead of holding a raw recorder reference.
- If the owning `MainPanel` is gone, the timer stops and the panel disables its buttons instead of dereferencing stale state.
- Record button routes through `MainPanel::toggleScratchCapture()`.
- Reveal button routes through `MainPanel::revealScratchFolder()`.

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

`ScratchPanel` currently computes elapsed time from `status.elapsedSamples` only when `status.lastTake` has a sample rate. During an active recording `lastTake` is reset, so the panel may show `00:00` until the take is finalized.

Suggested fix:

- Add `currentSampleRate` or `sampleRate` to `ScratchRecorderStatus`, populate it at start, and compute elapsed from that while recording.
- Add a focused test if status shape changes.

### Footer Visual QA

The footer layout compiles and existing UI scale regression tests pass, but no fresh screenshot matrix was captured after adding scratch controls.

Suggested follow-up:

```powershell
powershell -ExecutionPolicy Bypass -File scripts\run_d2_visual_qa.ps1 -OutputName 2026-06-05-scratch-footer -CaptureScaledFooterMatrix
```

Review normal and narrow screenshots at `125%`, `150%`, `175%`, and `200%` to ensure `REC` remains recoverable and does not crowd patch/transport/CPU controls.

### No Playback In Scratch Panel

The design allowed optional wet playback if it stayed small. That is not implemented.

Do not add this unless the user asks; immediate capture and reampable raw/wet files are the core value.

### Scratch Folder Setting

The recorder has `setScratchRoot()`, but no Preferences UI for a custom scratch folder is wired yet.

Current default behavior is acceptable for v1:

- user application data directory
- `Pedalboard3/Scratch Ideas`

Only add a setting if the user asks or manual QA shows the default is hard to find.

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
4. Fix the panel elapsed display if it shows `00:00` during recording.
5. Capture scaled footer screenshots if UI confidence is needed before PR.
6. Push the branch and open a draft PR after manual verification.

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
- Adds a non-modal ScratchPanel with safe MainPanel lifetime handling
- Stops/marks takes incomplete on patch switch and audio device changes
- Covers scratch take/recorder behavior with focused tests
```

## Do Not Forget

- Do not stage `documentation/research/` unless explicitly requested.
- Do not claim real guitar capture works until manual audio-interface verification is done.
- Do not convert this into a graph node unless the user explicitly changes the product direction.
- Keep future audio callback changes RT-safe: no locks, no allocation, no file work, no UI calls.
