# Pedalboard3 How-To

This is the practical entry point for the current Windows checkout. Product
priority and acceptance criteria live in [ROADMAP.md](ROADMAP.md); architecture
and real-time constraints live in [ARCHITECTURE.md](ARCHITECTURE.md).

## Build on Windows

The repository provides CMake presets for Ninja and Visual Studio 2022. For a
standard MSVC Release build:

```powershell
cmake --preset msvc-release
cmake --build --preset msvc-release
```

For this checkout, reuse the existing `build` tree through its cached alias:

```powershell
cmake --build "C:/Users/estee/Desktop/My Stuff/Code/Antigravity/Pedalboard2/build" --config Release
```

Source identity verified 2026-10-02: `C:/Users/estee/Desktop/My Stuff/Code/Antigravity`
is a junction to `D:/Code`, and `src/ScratchTake.h` through both checkout paths
has the same `fsutil` file ID. The C: source/JUCE paths in `build/CMakeCache.txt`
therefore name this same checkout, physically on D:, not an old source copy.
The existing tree and its artifacts can be reused; no cache deletion, new build
tree, or toolchain repair is required for this alias. Builds remain
coordinator-owned, and this path check is not a successful-build claim.

Do not delete or regenerate another active worktree's build directory. The
macOS/AU worktrees are separate and require a Mac to validate.

## Run focused tests

Build the test executable, then use Catch2 tags to keep feedback narrow:

```powershell
cmake --build build --config Release --target Pedalboard3_Tests
.\build\tests\Release\Pedalboard3_Tests.exe "[tuner]"
.\build\tests\Release\Pedalboard3_Tests.exe "[scratch]"
```

Run the configured Release suite when an integration checkpoint needs it:

```powershell
ctest --test-dir build -C Release --output-on-failure
```

The checker's output is evidence for that invocation only. Do not infer a green
full suite from a successful focused tag.

## Use the current Scratch Capture

Press the footer `REC` control (or Ctrl+Shift+R), wait for `Preparing` to become
`Recording`, play, then press `STOP` and wait for `Saved` or a failure reason.
Pressing REC again during preparation cancels it. Open `Takes` for the Scratch panel.
Capture records device input before input gains/patch processing to `raw.wav`
and output after the patch, master bus, and output gains to `wet.wav`.

The default destination is the user application-data directory's
`Pedalboard3/Scratch Ideas`, with a day folder and a timestamp/patch folder for
each take. Use `Choose` / `Reset` in the panel or the corresponding File menu
commands to change the destination for new takes; the chosen root is persisted.

New captures now write schema-version-2 `take.json` with bundle-relative asset
names, separate raw/wet frame counts, capture status, and snapshot status. An
initial incomplete manifest is written before recording; final metadata uses
temporary-file replacement so a failed replacement preserves the prior JSON.
The new read-only loader accepts legacy and v2 bundles without migrating them,
including relocated legacy folders. Task 7 now attempts `rig.xml` before audio
starts: live patch state, separate master-rack state/bypass, master/per-channel
gains, device context, and known NAM/IR/FilePlayer paths. It does not copy assets
or restore that rig. Check `snapshotStatus` and `snapshotMessage` independently
of audio completion: a failed snapshot may accompany a valid RAW/WET pair.
Legacy takes without a snapshot remain valid. Task 7's focused verification
passed; no manual live-plugin/master-rack snapshot integration pass is claimed.

Task 2 adds disk-backed `Takes`. It scans the configured root's day/take folders
on startup, panel open, `Refresh`, destination changes, and capture finalization. Previously saved
takes are discovered without modifying their files, and the panel no longer
uses the recorder's eight-entry session limit. Scanning is fixed-depth, not a
general recursive audio-file search; keep imported bundles in the day/take
layout. Missing roots and damaged/unsupported bundles show diagnostic status
rather than being silently discarded.

Task 6 routing and panel transport controls have a passing focused build/test
run; manual UI/hardware checks are still outstanding.
Current source behavior and boundaries:

- Task 3 provides virtualized rows and real RAW/WET waveform overviews. Select a row to view
  its audio on a shared time scale, and scroll the selected waveform vertically
  for additional channels. Small row previews show RAW channel 1 and WET
  channel 1 only. Selection follows the take folder through refresh/reordering.
- `Play` auditions the saved wet file through Pedalboard's selected output,
  bypassing the current rig and its gains. `Reamp` sends the raw performance
  through the current input gains, rig, master inserts, and output gains once.
  Neither inserts a FilePlayer, edits cables, or restores an old rig. Reamp is
  audition only; it does not export or overwrite a wet file.
- Audition temporarily replaces live input. Both modes use independent preview
  attenuation (default -12 dB, at most unity) and guarded output honoring safety
  mute. REC stops audition and waits for the live-input handoff before recording;
  Play/Reamp is blocked during preparation/capture/saving or a pending capture handoff.
- If the take has more channels than the current destination, choose each
  destination's source (or Off) and explicitly accept dropped channels. The
  dialog does not change device settings. Automatic mono wet mapping uses the
  first two outputs; mono raw uses only the first input.
- `Reveal` opens the take folder, including for incomplete takes if it exists.
  A supported complete or incomplete take can Play its readable wet side and
  Reamp its readable raw side independently. An interrupted-take warning remains
  visible; audition does not mark the capture complete. Invalid/unsupported
  bundles remain Reveal-only.
- For the selected take, use `Play original` or `Reamp current rig` to start audition,
  `Pause`/`Resume`, `Stop`, and full-take `Loop` to control it. Click the matching
  selected waveform to seek; its playhead follows playback. Preview level is
  independent of rig gain: -12 dB by default, at most 0 dB, with -60 dB muted.
  A persistent label identifies the mode and warns when live input is replaced.
  Selecting another take or closing the panel stops audition; selecting alone
  stays silent. A row's Play/Reamp button first selects that row. Mode buttons
  sit above transport and level controls, with the selected patch and snapshot
  status above the waveform. The smaller REC control has a keyboard-focus ring.
  A failed snapshot does not hide playable audio.
  Asset references are saved in `rig.xml`, not listed in the selected-take UI.
- While focus is inside the Scratch panel, bare Space pauses/resumes or starts
  the selected take in the current audition mode. Space is not intercepted in
  text editors or with modifiers. Escape stops audition, including while editing
  the level value. Actual focus traversal and shortcut dispatch remain manually
  unverified; automated helper tests are not proof of working native key routing.
- There are no fake traces: the overviews use cached saved-audio data, with
  explicit loading/unavailable states. They are not live input meters. The full
  keyboard workflow and visual polish are not yet manually verified. Disabled
  row/selected audition actions explain Preparing, Recording, or Saving.

## Scratch foundations — acceptance in progress

The native foundations cover a disk-backed take library, real raw/wet waveforms,
in-app wet audition, current-rig reamp without manual FilePlayer wiring, saved
capture-time rig context, and modest existing-style UI polish. The
[written design](docs/superpowers/specs/2026-10-02-scratch-foundations-design.md)
is approved. Native implementation of the
[implementation plan](docs/superpowers/plans/2026-10-02-scratch-foundations.md)
is now in progress. Storage Task 1 (`11fe41a`) has a successful build and focused
Scratch test result. Task 2 (`c33748f`) final app/test build passed on 2026-10-02; its
focused library/storage run passed 328 assertions in 12 cases, with two
Windows privilege-dependent symlink cases skipped, not passed. Task 3 (`c0d0dee`) final
incremental app/test build passed on 2026-10-02, with 53 assertions passing in
four focused waveform/UI cases. JUCE's silence thumbnail range is `[0, 1/128]`
because of its native quantization; that is not fabricated waveform activity.
Task 4's bounded worker-decoded audition engine had its final app/test build pass on
2026-10-02; the playback/storage/waveform run passed 871 assertions in 22 cases,
with one storage-symlink skip for missing privilege. The engine now stops
non-looping reads at the exact file end, including non-block-aligned lengths.
Task 5 shared preview protection (`3e441cb`) has a successful app/test build and
7,477 assertions passing in 22 safety/RT cases. Task 6 (`b22e1e8`) connects native routing,
record handoff, and panel controls. Its final app/test build exited 0 on
2026-10-04; the routing/playback/safety/storage run passed 5,058 assertions in
30 cases, with one storage-symlink case skipped for missing privilege. Task 7's
final app/test build exited 0 on 2026-10-04; the rig/storage/routing run passed
307 assertions in 16 cases, with one symlink case skipped for missing privilege.
Task 7 is committed as `5563f7f`. Task 8 panel polish and keyboard wiring are
source-integrated; the 2026-10-04 app/test build exited 0 and the focused
Scratch/RT/master-bus run passed 8,916 assertions in 99 cases, with two symlink
cases skipped. The first full CTest run passed 351/358 entries; reported failures
include two privilege skips, three stale ReverbSC assertions against unchanged
baseline files, and two post-success SegFault exits. The latter passed exact
reruns, but their root cause is unconfirmed. With skip reporting corrected,
pre-review full CTest exited 8: 353 passed, two skipped, and three unchanged-baseline
ReverbSC assertion cases failed (358 total), without another post-success SegFault.
The final incremental app/test build and 8,916-assertion focused run passed again;
the full-suite gate is not clean. No manual UI integration,
visual/focus, live-plugin snapshot, or hardware pass is claimed. Native automation
could not initialize, and no screenshot QA script ran. See the handoff for
dated evidence and the remaining acceptance checks.
The review fix enabling readable audio in incomplete takes passed its app/test
build and 962 assertions in 27 UI/playback/storage cases, with one symlink skip
(2026-10-05). Post-fix full CTest exited 8: 353 passed, two skipped, four failed
(359 total): the same baseline assertions plus Connection Dragging State Machine
crashing after nine assertions passed. It was not retried; crash root causes
remain unknown. The full-suite gate remains not clean, and runtime UI/live-plugin,
hardware, and macOS acceptance are still open.

## Verify Scratch Capture with a real interface

This manual acceptance check remains pending in the recorded handoff; source
inspection and the historical June test results do not establish it as passed.

Use a real guitar and selected audio input/output. Start capture with the footer
`REC` control, wait for `Recording`, play through an audible chain, then stop. In the recorded take
folder, verify all of the following:

1. `raw.wav`, `wet.wav`, and `take.json` exist.
2. `raw.wav` is pre-chain DI; `wet.wav` is the heard post-chain output.
3. `take.json` reports `complete: true` and `captureStatus: "complete"`; inspect
   both WAV files to confirm their sample-frame counts and recorded sample rate.
   New v2 `rawDurationSamples` and `wetDurationSamples` should match their WAVs
   and each other for a complete pair. The retained `durationSamples` is their
   minimum; legacy manifests may lack the independent counters.
4. A patch change stops capture with the expected incomplete-take reason.
5. An audio-device change does the same.
6. Play/Reamp are available independently for readable wet/raw sides of supported
   complete or incomplete takes; interruption warnings remain visible. Invalid/
   unsupported bundles are Reveal-only when their directory exists. For Task 6 acceptance,
   verify wet bypass/current-rig reamp, no graph edits or live-input leakage,
   explicit channel-loss mapping, safety mute, and REC's return-to-live handoff.
7. Destination choose/reset works between takes and is disabled in the panel
   during preparation/recording/saving. Confirm a new take uses the selected destination.
8. For Task 2 acceptance, restart and reopen Takes to verify disk discovery;
   check Refresh, more than eight saved bundles, destination changes, and a
   damaged test bundle's diagnostic row. Do not modify the user's recordings
   to create that fixture. This manual check is still outstanding.
9. For Task 7 acceptance, change an unsaved plugin parameter and a master insert
   before REC. Verify `snapshotStatus: "saved"` and the corresponding `rig.xml`
   context, including gains and asset references. Separately check cancellation
   during Preparing and snapshot-failure warnings without conflating them with
   audio failure. Snapshot availability does not imply automatic rig restoration.
10. For Task 8 acceptance, check default/narrow sizes, light/dark themes, and
    150% scale. Verify actual Tab focus, Space/ESC dispatch (including the level
    text editor), row-play selection, disabled reasons, and reachable controls.
    These visual/focus checks remain unverified; no new screenshots were captured.

Record the tested commit, interface/driver, channel selection, sample rate,
buffer size, and actual result; do not substitute old evidence for a new run.
The detailed procedure and dated historical automated evidence are in
[INSTANT_SCRATCH_CAPTURE_HANDOFF_2026-06-05.md](INSTANT_SCRATCH_CAPTURE_HANDOFF_2026-06-05.md).

## Before making a change

1. Check `git status --short --branch` and preserve unrelated work.
2. Read the relevant roadmap acceptance criteria and architecture/RT boundary.
3. Keep user-visible behavior changes accompanied by an `Unreleased` entry in
   [CHANGELOG.md](CHANGELOG.md) that explains the change and why it matters.
4. Commit and publish only when the current task authorizes it.
