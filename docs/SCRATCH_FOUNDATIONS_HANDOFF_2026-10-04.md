# Scratch foundations — implementation handoff, 2026-10-04

Branch: `codex/rt-hosting-sprint`; implementation baseline: `47cfb47`.
Product phase: **active development**. Saved bundles, RT/ownership boundaries,
capture preparation and source routing are **durable**: defects can lose takes,
play stale audio, record the wrong source, or block the callback. Panel layout is
**volatile**; helper tests are not pixel snapshots or proof of focus dispatch.

## Implemented

- Schema-2 portable take bundles, independent RAW/WET counts, bounded read-only
  validation, and interrupted/damaged bundle inspection without migration.
- Worker-based persistent discovery; virtualized, folder-stable take selection.
- Real cached RAW/WET waveforms and selected playback position/seek.
- Worker decoding/resampling, bounded PCM queue, pause/stop/loop/level, channel-loss
  approval. Original WET bypasses current processing; RAW runs through the current
  input gain/graph/master/output gain once. Preview replaces live input and cannot
  enter recording taps. Neither mode edits the graph.
- Independent preview safety; existing normal live limiter behavior preserved.
- REC preparation snapshots live unsaved graph/plugins, master inserts/bypass,
  channel/master gains, device context and known asset references. Cancelled or
  obsolete preparation cannot start a new recording. Audio can succeed while
  snapshot writing fails, with that distinction retained in metadata.
- Stacked mode/transport controls, selected snapshot context, phase-specific
  unavailable reasons, row-action selection, and REC focus ring. Space controls
  preview only inside Scratch and outside text entry (no modifiers); Escape stops
  preview anywhere within panel focus, including text entry.

No automatic rig restore, external asset copying, offline reamp export, cloud
features or timeline. Opaque plugin dependencies are not enumerated.

## Evidence and open gates

- Existing non-clean Release app/test build succeeded before final polish.
- Combined `[scratch],[rt],[masterbus]`: **8,916 assertions**, **99 passed cases**,
  **two Windows symlink cases skipped**. Tests exercise production storage,
  workers, transport, routing, preview safety, controller and serialization.
- Routing tests distinguish hardware/RAW/WET and non-unity nonlinear graph/master
  gains. A deliberate WET double-gain mutation failed, then was restored. Readiness
  tests reject stale PCM with both redundant publication protections disabled.
- Initial full CTest: **351/358 passed**, seven entries flagged. Two are privilege
  skips (reporting corrected); three ReverbSC source-string tests are stale against
  the shell-policy refactor. `src/PluginComponent.cpp` and
  `tests/reverbsc_processor_test.cpp` are byte-unchanged from baseline `47cfb47`.
  NAM Boolean Parameter Threshold and Scratch destination compaction printed
  successful assertions then exited with SegFault; both passed an exact rerun.
  Their intermittent process-exit cause is unconfirmed, not declared fixed.
- Final non-clean Release app/test build: exit 0. Combined focused run again:
  8,916 assertions, 99 passed cases, two privilege skips. Final full CTest:
  **353 passed, two skipped, three failed / 358 total**, exit 8. Only the unchanged
  baseline ReverbSC source-string assertions fail; the earlier process-exit
  crashes did not recur. Full suite remains non-green. Independent review pending.
- **OPEN:** dark/light, narrow/default, 150% screenshots and actual keyboard/focus
  traversal. Native UI runtime failed to initialize twice with kernel-assets path
  OS error 3 (including one reset); no screenshots or application input were made.
- **OPEN:** live unsaved-plugin/master-insert snapshot check. Fixture XML and
  source inspection do not establish actual plugin runtime serialization.
- **OPEN:** real guitar/interface capture, restart discovery, original audition,
  RAW loop while adjusting tone, REC handoff, patch/device removal, snapshot
  availability, and matching RAW/WET headers/counts. No macOS build claimed.

The feature is **not yet user-ready**. Full-suite failures and manual acceptance
must remain visible; historical screenshot evidence is not this build's QA.

## Reference use and implementation tradeoffs

Inspected the existing JUCE source at
`https://github.com/juce-framework/JUCE`, commit
`f8f8864172464b9adf9eba6101e1f784838d1597` (AGPLv3 dependency), including
TemporaryFile/File/FileOutputStream/Windows file operations, WavAudioFormat,
AudioThumbnail/Cache/FileInputSource/ThreadPool, AudioFormatReaderSource,
ResamplingAudioSource/AudioFormatReader and ComponentPeer keyboard dispatch.
Reuse mode: existing dependency/API use; no upstream source copied. Context7 was
not available. Existing local Scratch/limiter/MainPanel/live graph/master serializers
were inspected and reused; normal limiter policy was not replaced by another library.

Bounded WAV preflight is original boundary validation because JUCE can zero-fill
truncated reads: max 4,096 chunks and 1 MiB non-audio data. Audition supports
8–384 kHz, 16 channels; waveforms use a 32-entry cache and 512-sample buckets.
These intentional bounds reject unusual external files rather than risking
unbounded work. Snapshot assets remain references. One scan worker can delay
new-root completion behind a slow old-root read, but cannot publish stale results.

## Resources

Task logs/TEMP live under `D:\tmp\codex\scratch-foundations-20261002` with a
manifest and 1 GiB budget; cleanup audit pending final review. No alternate build
tree, dependency clone or macOS worktree edits were created.
