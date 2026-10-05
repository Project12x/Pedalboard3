# Instant Scratch Capture Handoff - 2026-06-05

This handoff preserves the original Scratch Capture implementation evidence
and manual acceptance procedure. Current priorities belong to `ROADMAP.md` and
`STATE_OF_PROJECT.md`; current usage and architecture are in `HOWTO.md` and
`ARCHITECTURE.md`.

Source reconciliation 2026-10-02: the inspected checkout is
`D:\Code\Pedalboard2`, branch `codex/rt-hosting-sprint`. The initial source-only
documentation pass did not build, run tests, or verify a real audio interface.
Task 1 implementation evidence is recorded separately below; the June results
remain historical.

Accepted expansion in progress: a disk-backed take library, real
raw/wet waveforms, in-app wet audition, current-rig reamp without manual
FilePlayer wiring, saved capture-time rig context, and modest existing-style UI
polish. The
[Scratch foundations design](docs/superpowers/specs/2026-10-02-scratch-foundations-design.md)
is approved. Native implementation of the
[implementation plan](docs/superpowers/plans/2026-10-02-scratch-foundations.md)
is now in progress. Storage Task 1 is committed as `11fe41a` with focused
verification below. Task 2 disk discovery (`c33748f`) has a successful app/test
build and focused verification below. Task 3 real waveforms and virtualized
selection (`c0d0dee`) also have a successful app/test build and focused result.
Task 4's audition engine has a successful final app/test build and focused
result below. Task 5 shared preview protection is committed as `3e441cb`.
Task 6 native routing, recorder handoff, and panel transport controls are committed
as `b22e1e8`, with final focused verification recorded below. Task 7 rig snapshots
and cancellable preparation are committed as `5563f7f`. Task 8 panel polish and
keyboard wiring have focused build/test evidence below; the full-suite gate is
not clean (first and final runs below), and visual/manual UI, live snapshot integration, and hardware QA
are not yet verified.

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
- `src/ScratchBundleIO.h`
- `src/ScratchBundleIO.cpp`
- `tests/scratch_recorder_test.cpp`
- `tests/scratch_take_storage_test.cpp`
- `CMakeLists.txt`
- `tests/CMakeLists.txt`

Behavior:

- Creates take folders under the scratch root.
- Uses date/timestamp and sanitized patch names.
- Handles folder collisions by appending numeric suffixes.
- Writes schema-version-2 `take.json` with bundle-relative `raw.wav`, `wet.wav`,
  and `rig.xml` names. Task 7 now attempts the rig sidecar before recording.
- Metadata includes patch/document/device context, sample rate, channel counts, gain values, raw/wet file paths, duration, completion state, and failure reason.
- `take.json` gain metadata is the two master gain values at start, not per-channel
  values or a history. Task 7's separate rig sidecar contains live patch state,
  master-rack state and per-channel gains; names/paths alone are not a snapshot.
- `take.json` now stores independent raw/wet frame counts, capture/snapshot
  status, and the retained shared minimum duration. Initial incomplete metadata
  is written before Recording; sibling temporary replacement preserves old
  metadata on failure without claiming power-loss durability.
- `loadScratchTake()` is a bounded, read-only v1/v2 loader. It accepts relocated
  legacy bundles without rewriting them or following their old absolute paths;
  missing legacy counts remain unknown. Invalid/unsupported results retain the
  directory for inspection. JSON size/type/range/path checks and WAV chunk
  preflight reject malformed or truncated data before constructing a reader.
  Task 2 consumes this loader through a background library worker; see its
  dated build/focused-test result below.

### Scratch Recorder Core

Files:

- `src/ScratchRecorder.h`
- `src/ScratchRecorder.cpp`
- `tests/scratch_recorder_test.cpp`

Behavior:

- `ScratchRecorder` owns state, current take metadata, recent take list, raw/wet sinks, and writer thread.
- The recorder's recent list remains in-memory, newest-first, capped at eight
  for existing callers. The Task 2 panel uses the disk library instead, so this
  recorder limit no longer limits panel history.
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

### Task 2 Disk Library

`src/ScratchTakeLibrary.{h,cpp}` adds one app-owned scan worker. MainPanel
requests discovery at startup, panel open, explicit Refresh, root changes, and
capture finalization (notified after the recorder lock is released).

- Scan only root/day/take depth; skip linked day/take directories and leave
  bundle contents unchanged. Missing/inaccessible roots produce status.
- Publish immutable root/generation snapshots, sorted newest-first, with partial
  publication every 128 discovered entries. Discard obsolete-generation results.
- Retain corrupt/unsupported bundles with reasons; separate directories remain
  separate entries even when take IDs match.
- The `Takes` panel consumes all entries rather than the eight-item
  recorder list, exposes Refresh and scanning/error status, and keeps diagnostic
  rows revealable. Task 3 replaces its interim list/trace presentation below.

Coordinator-reported verification on 2026-10-02: final app/test build exited 0;
`[scratch-library],[scratch-storage]` passed **328 assertions in 12 passed cases**,
with **two skipped cases** (storage symlink sections and the library linked-day
test) because Windows lacks symlink creation privilege. Those link-boundary
checks were unavailable, not passed. Manual UI/restart and hardware checks remain
unverified. At the Task 2 checkpoint, external wet playback and manual FilePlayer
reamp were unchanged; Task 6 replaces that routing as described below.

### Task 3 Waveforms And Selection

Source: `src/ScratchWaveformView.{h,cpp}`, `src/ScratchPanel.{h,cpp}`, and
`src/ScratchPanelPresentation.h`.

- RAW/WET overviews are decoded audio, sharing a panel-owned 32-entry cache and
  512-sample thumbnail buckets. Worker jobs revalidate bundles and open readers;
  generation checks reject stale selection results. Cache identity includes
  path, size, and modification time. Paint uses cached min/max data without
  reading WAV samples.
- The selected take shows individually labelled RAW/WET channels on a common
  time range, with vertical scrolling for extra channels. Compact row previews
  show channel 1 of each file only. Missing/loading states use text; synthetic
  lane and thumbnail traces are removed.
- A virtualized ListBox recycles rows with actual Play/Reamp/Reveal buttons.
  Selection follows the take directory across ordering/scan changes, including
  partial batches that have not reached the selected folder yet.
- Task 3 exposed seek/playhead hooks without connecting transport; its Play and
  Reamp still used the system player/manual FilePlayer at that checkpoint.
  Task 6 connects native routing and the panel controls.

Coordinator-reported final verification on 2026-10-02: incremental app/test
build exited 0; `[scratch-waveform],[scratch-ui]` passed **53 assertions in four
cases**. The first successful build exposed a test expectation mismatch: JUCE
`MinMaxValue::setFloat` deliberately expands equal bounds, so true silence has
thumbnail bounds `[0, 1/128]`. The regression was corrected against that source
behavior; it also checks a 0.75 impulse within 0.02 and in the correct time
region. This is native overview quantization, not synthetic waveform content.
No visual/manual UI/restart or hardware verification is claimed; those checks
remain outstanding.

### Task 4 Audition Engine — Engine Verification Complete

Source: `src/ScratchPlayback.{h,cpp}`, `src/ScratchPlaybackTypes.h`, and
`src/ScratchPcmQueue.{h,cpp}`. This task delivered the engine without MainPanel,
device-callback, or waveform-control integration; Task 6 adds those connections.

- Bounded SPSC PCM queue: 65,536 usable frames, 16 channels, about 4 MiB. WAV
  decoding/resampling is worker-only, supporting 8–384 kHz source/device rates.
- Consumer generation retirement and acknowledgment precede queue reset;
  `audioDeviceStopped()` acknowledges once callbacks have ceased. Owners remove
  the consumer before preparation/destruction. Starts/stops/seeks and held-sample
  underflow transitions use 5 ms fades; progress tracks consumed frames.
- Published preview gain defaults to -12 dB, caps at unity, and treats -60 dB
  as mute. Future routing/protection must apply it. Implicit channel loss is
  rejected; explicit mappings permit approved drops. Mono wet mirrors to only
  the first two destinations; mono raw uses only the first, with others silent.

Coordinator-reported final verification on 2026-10-02: app/test build exited 0;
`[scratch-playback],[scratch-storage],[scratch-waveform]` passed **871 assertions
in 22 cases**, with **one storage-symlink case skipped** for Windows privilege.
This supersedes the initial 866-assertion/21-case run. A non-128-aligned
500-frame fixture first exposed a loop-off end overshoot (524 frames consumed).
The consumer now caps non-looping reads to the exact remaining frame count;
the final passing run includes this regression.

The targeted stale-generation negative control survived removal of the generation
guard alone because the independent buffering gate still blocked consumption.
Bypassing both gates made the stalled-seek test consume 128 retired frames and
fail. Both guards were restored. This records the tested protection combination,
not a claim that the first mutation failed. No UI, visual, or hardware result
is implied by these engine tests.

### Tasks 5–6 Protection And Native Routing — Focused Verification Complete

Task 5 (`3e441cb`) shares the existing `dsp/SafetyLimiterCore` through an
independent `ScratchPreviewGuard`, without replacing the graph limiter singleton.
The guard honors safety mute, latches trips, rejects non-finite output, and
bounds preview samples. Explicit user unlock resets DSP on the processing
thread; source/device changes do not silently clear mute.

Task 5 coordinator-reported final verification: app/test build exited 0;
`[scratch-safety],[rt]` passed **7,477 assertions in 22 cases**.

Task 6 source: `ScratchAudioRouter.*`, recorder block-lease methods, and
MainPanel's callback hooks/`playScratchTake()`:

- Original WET bypasses audible current-rig/master-gain processing. The graph
  still receives zero input and processes MIDI/state into discarded output.
- Current RAW replaces hardware input and traverses current input gains, graph,
  master inserts, and output gains once. Neither mode edits nodes/cables or
  replaces a patch. The router applies preview gain/protection in both modes.
- Live capture alone obtains one RAW/WET writer lease per block. A successful
  begin must finish WET and release the lease even if stop arrives between taps.
  Audition cannot record itself; REC waits for playback quiescence before start.
- Ready library entries are required. Channel loss requires explicit
  destination/source approval, without device reconfiguration. Root/patch/device
  changes cancel pending audition/handoff; callback removal precedes teardown.
- Panel controls are wired: selected Original WET/Current rig actions,
  Pause/Resume, Stop, Loop, independent preview level, waveform seek/playhead,
  and persistent mode/live-input-replacement status. Selecting another take or
  closing the panel stops audition; selection alone does not start it.
- Manual UI, full integration, and real-interface checks remain pending; none
  are inferred from engine/helper tests. Task 7 adds rig snapshots below.

Task 6 final coordinator-reported verification (2026-10-04): app/test build
exited 0; `[scratch-routing],[scratch-playback],[scratch-safety],[scratch-storage]`
passed **5,058 assertions in 30 cases**, with **one storage-symlink case skipped**
for missing Windows privilege. This includes the added no-recapture tests and
supersedes the initial 5,046-assertion/29-case run. The production wet-route negative control
doubled output to 0.15071319 versus expected 0.07535660 and failed as intended.
Correct source was restored before the final passing build. Visual checks,
the full-suite checkpoint, and real-interface/hardware acceptance remain Task 8;
no such pass is claimed by this focused run.

### Task 7 Rig Context And Cancellable Preparation — Focused Verification Complete

Source: `ScratchRigSnapshot.*`, recorder preparation methods, MainPanel's
`prepareScratchCapture()`/`gatherScratchAssets()`, and panel Preparing labels.

- `prepareTake()` creates one pending bundle/manifest and enters Preparing;
  `startPreparedTake()` opens WAV writers only after snapshot completion.
  The legacy `start(context)` convenience API records without a snapshot.
- Live `PluginField::getXml()` is gathered once on the control thread, not from
  cached document XML. `ScratchRig` schema 1 stores that Patch, separate base64
  master-insert state and bypass, master gains, all 16 input/output channel
  gains, app version, and device/patch context.
- Known NAM, IR/IR2, and FilePlayer references include nested racks and NAM
  effects loops. Paths are recorded, assets are not copied, opaque plugin
  dependencies remain explicitly unenumerated, and no rig restore is added.
- A worker writes detached `rig.xml` using sibling temporary replacement. The
  message-thread completion checks the monotonic preparation ID plus context
  and the actual device-callback epoch before starting. That epoch increments
  in both `audioDeviceStopped()` and `audioDeviceAboutToStart()`, rather than
  relying only on later UI notification. Cancellation or a newer request
  cannot be restarted by an old completion; cancelled bundles stay interrupted.
- Snapshot failure retains its reason and may still accompany a complete,
  matched RAW/WET pair. A missing saved sidecar becomes a load-time snapshot
  warning without rejecting otherwise valid audio; legacy snapshot state is None.
- Preparing has a cancel action and disables audition/destination controls.
  Task 8 integrates presentation polish below.

Task 7 final coordinator-reported verification (2026-10-04): app/test build
exited 0; `[scratch-rig],[scratch-storage],[scratch-routing]` passed **307
assertions in 16 cases**, with **one symlink case skipped** for missing Windows
privilege. The missing-sidecar regression first failed with Saved (2) instead
of Failed (3), then passed after correction; intact complete audio stays Ready
with a SnapshotFailed warning. Live `getXml()` gathering was source-inspected,
but runtime unsaved-plugin/master-rack snapshot acceptance remains unverified.
No visual, full-suite, or hardware result is implied by the focused run.

Coordinator-reported manual-access limitation (2026-10-04): native computer-use
initialization failed twice, including after reset, while writing kernel assets
(OS error 3, path not found). No QA script ran; screenshot/focus checks and manual
hardware acceptance remain unavailable/unverified, not passed.

### Task 8 Panel Polish And Acceptance Checkpoint

Source: `ScratchPanel.{h,cpp}`, `ScratchPanelLayout.h`, and
`ScratchPanelPresentation.h`.

- Reduced REC hero with a keyboard-focus ring; selected patch/snapshot status above the real waveform;
  `Play original`/`Reamp current rig` mode row stacked above Pause/Resume, Stop,
  Loop, and level controls. Existing theme colors and FontManager remain in use.
- Row Play/Reamp first select that row so the waveform and transport follow it.
  Row/selected actions are disabled with phase-specific preparation/capture/saving
  reasons, and Preparing's record control reads CANCEL.
- Panel/descendant key listeners scope bare Space to pause/resume or start the
  selected take in the current mode. Text editors and modifier combinations are
  excluded from Space handling. Escape stops preview within the panel, including
  text-editor focus. Closing or selecting another take still stops playback.
- Automated production-helper tests cover Space arbitration, phase reasons,
  and transport geometry at 360/568/648/948-pixel widths. They do not exercise
  actual native focus traversal, key dispatch, theme rendering, or screenshots.

Coordinator-reported verification (2026-10-04): app/test build exited 0;
`[scratch],[rt],[masterbus]` passed **8,916 assertions in 99 cases**, with **two
symlink cases skipped** for missing Windows privilege.

The first full Release CTest run passed **351/358 entries**, with seven reported
failures requiring separate accounting:

- Two Windows privilege-dependent symlink cases were all-skipped, incorrectly
  reported as Failed. CTest reporting was corrected to match the one-case
  all-skipped summary, not exit code 4 (which can also mean four failed assertions).
- Three stale ReverbSC source-string assertions remain. `src/PluginComponent.cpp`
  and `tests/reverbsc_processor_test.cpp` are unchanged from baseline `47cfb47`;
  these unrelated baseline assertions have not been fixed by the Scratch work.
- NAM Boolean Threshold and Scratch destination compacts long paths reported
  post-success SegFault exits. Both passed exact reruns, but the root cause is
  unconfirmed; reruns do not establish a fix.

Final coordinator-reported checkpoint (2026-10-04): incremental app/test build
exited 0; the combined focused run again passed **8,916 assertions in 99 cases**,
with **two skips**. Final full Release CTest exited **8**: **353 passed, two
correctly skipped (227, 233), and three failed (289, 290, 294), out of 358**.
The failures remain the unchanged-baseline ReverbSC assertions above. No
post-success SegFault occurred in this final run; the first-run cause remains
unconfirmed. The full-suite gate is **not clean**. Native
automation failed to initialize as recorded above, so visual/focus and manual
UI/hardware acceptance remain unverified. No screenshot QA script ran.

### Audio Callback Taps

File:

- `src/MainPanel.h`

Behavior:

- The callback delegates to `ScratchAudioRouter`; its live-only recorder lease
  taps RAW before input gains and WET after graph/master bus/output gains and
  the return-to-live ramp. Audition routes never obtain a capture lease.
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
- Task 6 routes wet Play and raw Reamp to native audition, replacing the old
  OS-player/manual-FilePlayer actions. Reamp uses the current rig without graph
  edits or snapshot restoration and does not export a new wet file. Panel
  transport is wired with focused verification above; manual acceptance remains
  pending.
- Task 3 replaces synthetic lane/thumbnail traces with real cached overviews
  and folder-stable virtualized selection, with the focused verification above;
  in-app transport is not part of that change.

Additional layout/presentation helpers:

- `src/ScratchPanelLayout.h`
- `src/ScratchPanelPresentation.h`
- Focused tests cover non-overlapping row/destination actions and active elapsed display.

## Task 1 Storage Verification — 2026-10-02

Coordinator-reported final incremental app/test build passed after warning
cleanup and removal of the duplicate `/FS` flag; it rebuilt the changed
`ScratchTake.cpp` and reused existing dependencies. The focused `[scratch]`
rerun passed **420 assertions in 32 passed cases**, with **one symlink case
skipped** because Windows symlink creation privilege was unavailable. That
boundary is not recorded as a passing automated check.

This verifies the storage slice, not later library/transport/UI tasks, a fresh
full-suite run, macOS, or real-interface capture. No hardware smoke test result
is added by this update.

## Historical Verification (June 2026)

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
4. Press `REC` and wait for Preparing to become Recording.
5. Play guitar through an audible chain.
6. Press `STOP`.
7. Open `Takes` or use `Reveal Scratch Ideas Folder`.
8. Confirm the take folder contains:
   - `raw.wav`
   - `wet.wav`
   - `take.json`
9. Confirm `take.json` reports `complete: true` and, for v2,
   `captureStatus: "complete"`. Inspect both WAVs for equal frame counts and the recorded sample
   rate; v2 `rawDurationSamples`/`wetDurationSamples` must match their files.
   The shared `durationSamples` remains the minimum; legacy bundles may lack
   independent counters and require direct WAV inspection.
10. Confirm `raw.wav` is pre-chain DI and `wet.wav` is what was heard.
11. Start another recording, switch patches, and confirm capture stops with `Patch changed during scratch capture`.
12. Start another recording, change audio device settings, and confirm capture stops with `Audio device changed during scratch capture`.
13. Confirm no Audio Recorder node is added to the graph.
14. For Task 6 acceptance, confirm native wet bypass and current-rig raw reamp,
    live-input exclusion, safety mute, explicit channel-loss approval, and no
    graph edits. Verify Stop/REC restores live input without preview recapture.
15. For Task 7 acceptance, verify a changed unsaved plugin parameter, separate
    master insert/bypass/gains, and known assets in `rig.xml`; check the manifest's
    snapshot result separately from audio completion. Check Preparing cancellation
    and failure warnings without editing the user's existing takes.
16. For Task 8 acceptance, check default/narrow widths, light/dark themes, and
    150% scale. Exercise actual Tab focus and Space/ESC from buttons, list rows,
    waveform, and level text editor; verify row-play selection and disabled
    reasons. Helper tests do not substitute for these running-panel checks.
17. Record commit, interface/driver, channels, sample rate, buffer size, and
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

Task 6 source update, 2026-10-04: native wet/current-rig raw audition replaces
the June external-player/FilePlayer actions. Original take files and graph
routing are unchanged. Reveal still opens the bundle. Panel transport controls
are wired with focused verification; full integration and manual UI verification
remain pending. Task 7 now saves capture-time rig context with focused verification,
but raw reamp still uses the current rig; automatic restoration remains out of scope.

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
   requires the directory. Check native wet/current-rig routing and handoff
   behavior against the Task 6 manual acceptance items above.
6. Use June 9 QA folders as historical references; capture new UI evidence only
   when needed for the current change.
7. Follow `AGENTS.md`: this is a single-developer repository, with no PR by
   default. Commit/push/merge only as authorized by the current task; create a
   PR only if explicitly requested.

Suggested focused verification against the existing build (coordinator-owned;
no clean or alternate build). Run the full suite at an explicit integration
checkpoint or for a broad-reaching change, not automatically for a doc update:

The build-location prerequisite is resolved: the cached C: source path reaches
`D:/Code` through the `Antigravity` junction, and matching `ScratchTake.h` file
IDs confirm the same checkout. Reuse the existing tree via its cached alias;
see [HOWTO.md](HOWTO.md#build-on-windows). This is source-identity evidence, not
a build/test result.

```powershell
git -c safe.directory=D:/Code/Pedalboard2 status --short --branch
cmake --build "C:/Users/estee/Desktop/My Stuff/Code/Antigravity/Pedalboard2/build" --config Release --target Pedalboard3 -- /m:1
cmake --build "C:/Users/estee/Desktop/My Stuff/Code/Antigravity/Pedalboard2/build" --config Release --target Pedalboard3_Tests -- /m:1
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
