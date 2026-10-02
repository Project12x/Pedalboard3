# Scratch Capture foundations and UI polish

Date: 2026-10-02

Status: written design approved by the user's `proceed` on 2026-10-02.
The [implementation plan](../plans/2026-10-02-scratch-foundations.md) was approved
for native execution; implementation is in progress.
This is a design, not a claim that the features below exist.

## Intent and scope

The user asked to expand Scratch substantially, update documentation in parallel,
and proceed with the five identified foundations plus modest UI polish. Success
means a saved performance remains findable and useful after the recording session:
find the take, see its actual audio, hear the original sound inside Pedalboard,
and play the dry performance through the current rig without manual wiring.

The accepted foundations are persistent take history, genuine waveforms, in-app
wet playback, easier raw reamping, and capture-time rig context. UI polish must
preserve the existing ColourScheme, FontManager, themes, and prominent REC control.

This increment does not add a DAW timeline, destructive editing, automatic
retrospective recording, AI analysis, cloud storage, a template browser, or a new
UI framework. It does not silently restore an old rig over the current one.
Offline rendering/export of reamp variants and portable asset bundling are later
features, not hidden requirements of this increment.

## Current implementation and gaps

- `ScratchRecorder` owns paired 24-bit WAV writers. Raw is tapped before device
  input gains; wet is tapped after graph, master inserts, and output gains.
- `ScratchTake` writes unversioned JSON containing context and absolute WAV paths.
  It has no loader. The recent list retains eight takes in memory only.
- `ScratchPanel` paints decorative lane traces and seeded pseudo-waveforms.
- `MainPanel::previewScratchWetTake` opens the operating system's file handler.
- `MainPanel::reampScratchRawTake` inserts an unconnected FilePlayer node.
- Patch serialization already captures live graph/plugin state, but does not
  by itself include the separate device-level master insert rack and all gains.

The existing hardware smoke test remains outstanding. Equal file lengths do not
mean dry and processed waveforms are latency-compensated or phase-aligned.

## Approach and alternatives

**Chosen: a disk-backed take library and one app-owned audition service.** Reuse
the recorder, JUCE file/waveform primitives, and Pedalboard's state serializers.
Switch the audition source at the device callback boundary, keeping graph routing
unchanged. This makes the five improvements useful together without making
Scratch a second patch editor.

An alternative is to improve FilePlayer insertion and automate cable edits.
That reuses more existing player UI, but changes the user's graph, needs undo and
routing heuristics, and cannot replay the original wet sound independently of
the rig. Another alternative is a separate preview device callback plus a new
offline reamp graph. That duplicates device/lifetime or plugin-hosting concerns
and exceeds this increment's needs. Neither is the default design.

## 1. Persistent take library

Introduce a `ScratchTakeLibrary` service independent of recorder state. The take
folder is the source of truth; no separate authoritative database is required.

- Scan the configured root's existing day/take layout off the audio and message
  threads. Publish immutable result batches, sorted newest first. Refresh on
  startup, panel open, explicit Refresh, root change, and capture finalization.
- Use the canonical take directory as identity. A copied folder or duplicated
  take ID must not overwrite or merge another entry. Keep selection stable by
  identity when a scan completes.
- Cancel obsolete scans with a root/generation token. Never publish results
  from a previous root into the current library. Do not traverse links outside
  the selected root or recursively crawl arbitrary directory trees.
- Read legacy unversioned metadata without rewriting it. Prefer the sibling
  `raw.wav` and `wet.wav`, so moved take folders remain useful even when legacy
  absolute paths are stale. Do not follow metadata paths outside a take bundle.
- Write new metadata as schema version 2 with bundle-relative asset paths,
  separate raw/wet frame counts, capture status, and snapshot status. Retain
  existing fields that consumers use, including the shared duration and
  `complete` flag. Unknown future schemas are visible as unsupported, not
  silently rewritten as version 2.
- Write initial incomplete metadata before audio recording begins and finalize
  through a sibling temporary file plus replace. A failed update must leave the
  prior metadata intact; do not claim power-loss durability beyond the filesystem
  guarantees. Incomplete files are never relabeled complete just because they exist.
- Bad JSON, missing audio, and unreadable files produce an inspectable row with
  a reason and Reveal action; one damaged take must not hide the rest. A failed
  or interrupted take may expose a readable finalized WAV for audition with a
  warning. It must not be presented as a successful paired capture.
- Expose all discovered takes, with virtualized/paged rows rather than one
  component and waveform decoder for every file. Remove the eight-take UI limit.
  This pass adds no deletion, automatic cleanup, or background migration.

The recorder publishes newly finalized takes to the library; scanning and
waveform loading are not added to `ScratchRecorder::getStatus()` or a UI timer.

## 2. Real waveforms and a selected-take view

Replace synthetic traces with waveforms decoded from the saved raw and wet WAVs.
Use JUCE `AudioThumbnail`/`AudioThumbnailCache` for file-backed overview data,
outside the realtime path. Load the selected take first and visible rows next;
bound the cache and cancel stale requests after selection/root changes.

The selected take has two labelled RAW/WET lanes on one time scale, duration,
playhead, and click-to-seek. Multiple channels must be identified accurately;
an overview must not falsely label a multichannel file as stereo. A loading or
unreadable waveform shows a real status, not fabricated sample activity.

Before a take exists, show a quiet empty state. During capture, show elapsed time
and existing level information; do not imply a decorative trace is a live scope.
Continuous live waveform capture is outside this increment.

## 3. In-app audition

Introduce a `ScratchPlayback` service owned by `MainPanel`, not by the window.
Its public UI states are Idle, Loading, Playing, Paused, Stopped, and Error, with
selected take, position, duration, mode, and an actionable error description.

Controls are Play/Pause, Stop, seek, full-take Loop, and preview volume. Closing
the Scratch panel stops audition. Selecting another take stops the old one;
selection alone does not start audio. Device change, document/patch switch, and
shutdown stop playback safely and do not automatically resume it.

### Original wet mode

Play the recorded wet file through the existing output device, bypassing the
current graph, master inserts, and live master gain settings for the audible
preview. These were already printed into the file. Apply only an independent
preview attenuation and output protection. Default preview level is -12 dB,
adjustable up to unity; never normalize or boost the saved audio automatically.

Suppress live monitoring while auditioning. Keep any necessary graph lifecycle
and MIDI servicing intact, but do not mix its output into the preview. Restore
live monitoring on Stop using short ramps. The UI must clearly say that live
input is temporarily replaced, and Stop must always be reachable.

### Audio and lifetime boundary

Do not put `AudioTransportSource::getNextAudioBlock` or a disk reader directly in
the realtime callback: the pinned JUCE transport locks its callback mutex.
Use a file worker to decode and, when required, resample to the active device
rate. Feed a bounded, preallocated single-producer/single-consumer PCM queue.
Reuse JUCE primitives rather than inventing a resampler or FIFO algorithm.

The callback only reads prepared PCM, applies bounded ramps/gain/protection,
and publishes atomic progress. It never waits for the worker. Underflow fades
to silence and reports buffering; it must not replay stale memory or stale audio.
Pause holds the consumed position; seek/loop changes are generation-tagged so
old queued samples cannot leak into a new position or take.

Seek, source replacement, queue reset, and teardown require an explicit consumer
quiescence acknowledgment before buffers are reset or freed. A FIFO reset is not
assumed thread-safe. Keep owners off the callback; no shared-pointer destruction,
allocation, file I/O, logging, waits, or UI calls there. Device-stop teardown
must also work when no further callback acknowledgment will arrive.

Preview must respect the application's safety mute. The current limiter is a
graph processor with a static instance accessor, so creating another registered
instance is not an acceptable shortcut. Reuse/extract its applicable protection
without replacing the registered instance or changing live-path safety policy.
Test preview finite-value protection, ceiling, and mute behavior explicitly.

For wet playback, map channels by index and silence unused outputs. A mono file
may mirror to a stereo output pair; multichannel files with insufficient device
outputs must show the channel mismatch and require an explicit preview mapping,
not silently discard channels. Sample-rate conversion must preserve time/pitch.

## 4. Reamp through the current rig

`Reamp current rig` uses the same transport and plays the raw file as the graph's
input, before current master/per-channel input gain. The normal graph, master
inserts, and output gains then run once. Hardware input is excluded while reamping.

No FilePlayer is inserted, no cables are edited, and no patch is replaced. The
user can adjust the current rig while the performance plays or loops. Stop
returns control to the instrument. This is audition, not a new saved wet print;
the original take's files and metadata are never overwritten.

Match recorded channel indices to graph input indices and zero unused inputs.
Do not duplicate mono raw input onto multiple graph inputs automatically. If
the current graph cannot accept the recorded channels, show the mismatch and
do not start until the user selects an explicit mapping. Do not silently change
the audio device configuration to make a take fit.

Playback and recording are mutually exclusive in this increment. Pressing REC
stops audition, waits for the safe live-input handoff, then starts a new capture.
Play/Reamp is unavailable during Preparing, Recording, or Saving, with a reason.
One transport mode is latched for a complete callback block; mode changes cannot
produce a raw hardware block paired with wet audition audio.

Reamping uses the **current** rig and gains, not necessarily the capture-time rig.
The UI must state this. It is not bit-exact reproduction: plugin latency, random
modulation, external hardware, generators, and parameter changes remain relevant.

## 5. Capture-time rig context

Save a versioned `rig.xml` sidecar at capture start, using a fresh live
`PluginField::getXml()` snapshot, not the last saved document or cached patch.
Include graph routing, serialized plugin state, mappings, and patch tempo, plus
the separately serialized master insert rack, its bypass state, master gains,
and all active per-channel gains. Include app/schema version and original device
channel/rate context for interpretation, not automatic device reconfiguration.

Gather plugin state on its existing supported control-thread path before the
recorder becomes active; write the detached snapshot off the audio thread. Show
Preparing while needed. Never call plugin serialization from a callback or paint.
If snapshot capture/write fails, allow audio capture with a visible warning and
record `snapshotStatus` and reason. Do not equate a saved audio pair with a saved rig.

Snapshot scope is the state at the beginning of the take, not recorded automation.
Record known external NAM/IR/file references; do not copy plugin binaries, model
libraries, or arbitrary referenced files. Opaque third-party plugin state may
contain additional external dependencies that Pedalboard cannot enumerate.

The selected-take view shows snapshot availability and known asset references,
and Reveal exposes the bundle. This is durable saved context, not a promise of
portable exact recall. A dedicated restore/import workflow, including missing
plugin/asset handling and device-global state restoration, is a later increment.
No new button in this pass silently applies a snapshot to the user's current rig.

## UI polish

Retain the existing themed chassis and REC identity. Use the current screenshot
and `ScratchPanelLayout` as the baseline, not a wholesale visual replacement.

- Reduce duplicated Ready/RAW/WET badges and decorative empty space. Keep one
  clear recording status, elapsed timer, and channel summary beside REC.
- Give the selected take's genuine RAW/WET waveforms and transport useful space.
  Distinguish `Play original` from `Reamp current rig` in text, not color alone.
- Rename Recent Takes to Takes; show recorded date, duration, patch name, and
  capture/snapshot warnings. Maintain a visible selected row after refresh.
- Use real focusable controls for actions instead of paint-only mouse hit areas.
  Include hover, pressed, selected, disabled, and keyboard-focus states. Explain
  disabled actions without relying on a transient toast.
- Keep Ctrl+Shift+R. Space controls selected-take playback only when Scratch has
  focus and no text input consumes it; Escape stops audition. Do not hijack global
  performance controls or plugin-editor typing.
- Keep destination choose/reset/reveal available but visually secondary. At
  narrow sizes, stack transport controls; do not clip actions or long paths.
  Use existing semantic font/color accessors in every theme.

## Component boundaries

| Component | Responsibility | Must not own |
|---|---|---|
| `ScratchTake` | Versioned take data, bounded parsing, bundle paths | UI or audio callbacks |
| `ScratchTakeLibrary` | Background discovery and immutable library results | Capture writers or plugin instances |
| `ScratchRecorder` | Raw/wet writer lifecycle and finalization | Disk library scanning or playback |
| `ScratchPlayback` | Worker decoding, bounded PCM handoff, transport status | Graph edits or live plugin state |
| Rig snapshot helper | Detached live-state serialization and sidecar | Audio processing or automatic restoration |
| `MeteringProcessorPlayer` | Block-consistent live/wet/reamp selection | File access or worker lifetime ownership |
| `ScratchPanel` | Selection, waveforms, controls, status presentation | Musical rules or storage work in paint/timers |

Library, snapshot, and transport state must be separable from UI so deterministic
tests do not require a running window or a real audio device. Keep new services
in focused files; do not append all of this to the already-large MainPanel.

## Reference source and reuse record

Internal baseline inspected: Pedalboard HEAD `a15ae4f`; `ScratchTake.*`,
`ScratchRecorder.*`, `ScratchPanel.*`, `MainPanel.h/.cpp`,
`PluginFieldPersistence.cpp`, `FilterGraph.cpp`, `MasterBusProcessor.cpp`,
`MasterGainState.h`, and `SafetyLimiter.h/.cpp`. Reuse existing serializers and
capture patterns; do not create a competing patch format. Read the June Scratch
design and non-AI feature/library research before choosing this approach.

Upstream repository: https://github.com/juce-framework/JUCE

Pinned commit: `f8f8864172464b9adf9eba6101e1f784838d1597` (local submodule).
No source clone or new dependency was created for this design.

| Inspected files under JUCE | License | Reuse mode and finding |
|---|---|---|
| `examples/Audio/AudioPlaybackDemo.h` (thumbnail setup) | ISC, Raw Material Software Limited | Pattern-only: selected-file waveform and playhead; not its RT transport architecture |
| `modules/juce_audio_utils/gui/juce_AudioThumbnail.cpp` (source setup/drawing) | AGPLv3 / commercial | Existing dependency: file-backed waveform/cache, never used by audio callback |
| `modules/juce_audio_devices/sources/juce_AudioTransportSource.cpp` (`getNextAudioBlock`) | AGPLv3 / commercial | Pattern-only audit: callback lock makes direct use unsuitable for the added RT path |
| `modules/juce_core/containers/juce_AbstractFifo.h` (contract/read-write pattern) | AGPLv3 / commercial | Existing dependency: SPSC indices over owned bounded PCM storage; reset only when quiescent |

The repository's `THIRD_PARTY_LICENSES.md` declares JUCE use under AGPLv3. Keep
that existing licensing record; do not characterize JUCE modules as permissive.
No upstream source is copied by this document. If the implementation copies or
closely ports example code, retain its ISC copyright/permission notice and
identify the exact source and changes. Prefer API reuse over copying modules.

## Delivery and verification boundaries

Product phase: **active development**.

Durable contracts: saved take compatibility and non-destructive behavior (lost or
misidentified performances); audio ownership/RT boundaries (glitches, deadlock,
use-after-free); source routing and output protection (feedback or unsafe levels).
Volatile surfaces: panel arrangement, visual density, and wording. Do not freeze
unfinished UI with exhaustive snapshots.

Deliver library plus real waveforms first, transport plus source switching next,
then capture-time rig context and integrated polish. These are coherent slices of
one approved feature direction, not separate speculative product bets. Each
behavior-changing commit must carry a matching Unreleased CHANGELOG entry and
update current-state docs; planned features stay labelled planned until verified.

Focused acceptance evidence must cover:

- Legacy and version-2 round trips, relocated bundles, duplicate IDs, invalid
  types/paths/schema, incomplete takes, failed metadata replacement, and stale
  root scan cancellation. Existing recordings must never be rewritten by scan.
- Actual nonzero and silent WAV fixtures produce different truthful waveforms;
  loading/missing states and selection survive refresh without fake traces.
- Deterministic PCM tests distinguish wet bypass from reamp (use non-unity test
  gains/effect); exclude live input during audition; prohibit preview recapture.
- Underflow, EOF, loop, seek, rapid source/mode changes, pause/stop, and shutdown
  cannot reuse stale samples or retire a buffer still in use. Exercise differing
  block sizes and device rates. Add targeted mutation checks only where a test
  might otherwise pass despite incorrect routing or generation handling.
- Rig state includes unsaved plugin changes and master inserts/gains, and handles
  sidecar failure without falsely reporting complete rig context. Original patch,
  audio files, and device configuration remain unchanged by audition.
- One non-clean incremental build and focused test run per implementation slice;
  one broader integration checkpoint for callback/graph/master-bus changes. The
  coordinator alone owns builds. Diagnose a toolchain failure once against the
  known-good tree; do not create alternate build trees opportunistically.
- Manual guitar/interface verification: start/stop capture, close/reopen app,
  audition original, loop reamp while changing tone, return to live input, switch
  device/patch, and verify original files plus matched raw/wet frame counts.
  Check minimum/default panel sizes, keyboard operation, and light/dark themes.

No compile or audio test was run for this documentation-only design pass. No
temporary downloads, clones, or external scratch resources were created. Future
generated fixtures/build scratch follow the task-manifest rule under
`D:\tmp\codex\<task-id>`, with explicit budgets and coordinator cleanup.
