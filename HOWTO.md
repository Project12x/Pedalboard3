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

The existing local `build` directory can also be rebuilt without reconfiguring:

```powershell
cmake --build build --config Release
```

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

The current panel is not a persistent take library:

- `Recent Takes` contains at most eight takes from this app session. Older
  files remain on disk, but the panel does not reload them after restarting.
- `Play` opens a complete take's wet file in the OS-associated player, which
  may use a different output device from Pedalboard. There are no internal
  Scratch playback controls.
- `Reamp` adds a raw-file player to the current patch. Connect its outputs to
  the desired processing chain and use that player's controls to play it; the
  action does not restore the original rig, wire the node, or export a reamp.
- `Reveal` opens the take folder, including for incomplete takes if it exists.
  Play/Reamp require both a complete take and their respective WAV file.
- The drawn traces are decorative, not an audio waveform or signal meter.

## Planned Scratch foundations

The accepted next scope is a disk-backed take library, real raw/wet waveforms,
in-app wet audition, current-rig reamp without manual FilePlayer wiring, saved
capture-time rig context, and modest existing-style UI polish. These are planned
features, not controls available in the current build. The
[written design](docs/superpowers/specs/2026-10-02-scratch-foundations-design.md)
awaits review before implementation; the current-use instructions above remain
applicable.

## Verify Scratch Capture with a real interface

This manual acceptance check remains pending in the recorded handoff; source
inspection and the historical June test results do not establish it as passed.

Use a real guitar and selected audio input/output. Start capture with the footer
`REC` control, play through an audible chain, then stop. In the recorded take
folder, verify all of the following:

1. `raw.wav`, `wet.wav`, and `take.json` exist.
2. `raw.wav` is pre-chain DI; `wet.wav` is the heard post-chain output.
3. `take.json` reports `complete: true`; inspect both WAV files to confirm equal
   sample-frame counts and the recorded sample rate. JSON has only one
   `durationSamples` value, which should match each completed WAV.
4. A patch change stops capture with the expected incomplete-take reason.
5. An audio-device change does the same.
6. Play/Reamp are available only for complete takes with the target WAV present;
   Reveal is available when the take directory exists. Verify the external
   playback and manual-routing reamp behaviors described above.
7. Destination choose/reset works between takes and is disabled in the panel
   during recording/saving. Confirm a new take uses the selected destination.

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
