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

Press the footer `REC` control (or Ctrl+Shift+R), play, then press `STOP` and
wait for `Saved` or a failure reason. Open `Takes` for the Scratch panel.
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
including relocated legacy folders. `rig.xml` is a reserved sidecar name,
not a saved rig yet.

Task 2 adds disk-backed `Takes`. It scans the configured root's day/take folders
on startup, panel
open, `Refresh`, destination changes, and capture finalization. Previously saved
takes are discovered without modifying their files, and the panel no longer
uses the recorder's eight-entry session limit. Scanning is fixed-depth, not a
general recursive audio-file search; keep imported bundles in the day/take
layout. Missing roots and damaged/unsupported bundles show diagnostic status
rather than being silently discarded.

Current limitations remain:

- The interim list is not virtualized yet. Real waveforms and selected-take
  transport are later tasks, not part of disk discovery.
- `Play` opens a complete take's wet file in the OS-associated player, which
  may use a different output device from Pedalboard. There are no internal
  Scratch playback controls.
- `Reamp` adds a raw-file player to the current patch. Connect its outputs to
  the desired processing chain and use that player's controls to play it; the
  action does not restore the original rig, wire the node, or export a reamp.
- `Reveal` opens the take folder, including for incomplete takes if it exists.
  Play/Reamp require both a complete take and their respective WAV file.
- The drawn traces are decorative, not an audio waveform or signal meter.

## Scratch foundations — native implementation in progress

The accepted next scope is a disk-backed take library, real raw/wet waveforms,
in-app wet audition, current-rig reamp without manual FilePlayer wiring, saved
capture-time rig context, and modest existing-style UI polish. The
[written design](docs/superpowers/specs/2026-10-02-scratch-foundations-design.md)
is approved. Native implementation of the
[implementation plan](docs/superpowers/plans/2026-10-02-scratch-foundations.md)
is now in progress. Storage Task 1 (`11fe41a`) has a successful build and focused
Scratch test result. Task 2's final app/test build passed on 2026-10-02; its
focused library/storage run passed 328 assertions in 12 cases, with two
Windows privilege-dependent symlink cases skipped, not passed. Waveform,
transport/reamp, rig-context, and polish tasks are upcoming. See the handoff
for dated evidence; manual restart/UI and hardware QA remain outstanding.

## Verify Scratch Capture with a real interface

This manual acceptance check remains pending in the recorded handoff; source
inspection and the historical June test results do not establish it as passed.

Use a real guitar and selected audio input/output. Start capture with the footer
`REC` control, play through an audible chain, then stop. In the recorded take
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
6. Play/Reamp are available only for complete takes with the target WAV present;
   Reveal is available when the take directory exists. Verify the external
   playback and manual-routing reamp behaviors described above.
7. Destination choose/reset works between takes and is disabled in the panel
   during recording/saving. Confirm a new take uses the selected destination.
8. For Task 2 acceptance, restart and reopen Takes to verify disk discovery;
   check Refresh, more than eight saved bundles, destination changes, and a
   damaged test bundle's diagnostic row. Do not modify the user's recordings
   to create that fixture. This manual check is still outstanding.

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
