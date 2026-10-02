# Scratch Foundations Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make saved Scratch takes findable, visibly truthful, internally playable, and useful for current-rig reamping, with saved rig context and focused UI polish.

**Architecture:** Keep recording, library discovery, audition transport, and rig serialization in separate app-owned services. The existing device callback selects one source mode for an entire block; file decoding and waveform work remain outside it. Reuse Pedalboard serializers and the pinned JUCE dependency rather than adding a second graph editor or storage database.

**Tech Stack:** C++17, JUCE 9.0.0, existing nlohmann/json, Catch2, CMake/MSVC.

**Spec:** [Approved Scratch foundations design](../specs/2026-10-02-scratch-foundations-design.md).

**Status:** Written design approved by the user's `proceed` on 2026-10-02. This implementation plan awaits review and execution-method choice. No task below has been implemented or tested by this planning pass.

## Global Constraints

- Product phase: **active development**.
- Durable: saved take compatibility/non-destructive behavior, audio ownership/RT boundaries, source routing/output protection. Defects can lose performances, deadlock/crash audio, or emit unsafe output.
- Volatile: panel arrangement, visual density, and wording; no exhaustive visual snapshots of unfinished UI.
- UI polish must preserve the existing ColourScheme, FontManager, themes, and prominent REC control.
- Default preview level is -12 dB, adjustable up to unity; never normalize or boost the saved audio automatically.
- Playback and recording are mutually exclusive in this increment.
- No allocation, file I/O, waits, locks, logging, ownership destruction, or UI calls in the new audio callback path.
- No FilePlayer is inserted, no cables are edited, and no patch is replaced by audition.
- No automatic rig restoration, offline reamp export, asset copying, cloud/AI, retrospective capture, or DAW timeline.
- Each behavior-changing commit includes CHANGELOG.md [Unreleased]: what changed, why, and important limitations. Update current-state docs in that same commit; never mark planned behavior implemented before verification.
- One coordinator owns all builds. Follow the scratch manifest/lifecycle rules under `D:\tmp\codex\<task-id>`; no speculative dependency clones or alternative build trees.
- Do not touch the separate macOS/AU worktrees. Keep source portable; macOS validation remains a separate platform gate.

## Review Focus

1. Legacy metadata can contain stale absolute paths or huge/wrong-typed numbers: load a relocated bundle safely without following external paths or rewriting it (Task 1).
2. A root/selection change can finish before an old scan/thumbnail job: old results must never replace the new selection or reopen an old file (Tasks 2–3).
3. A decoder can stall during seek, loop, stop, or device removal: silence is acceptable; stale samples, buffer reuse races, and waiting on the audio thread are not (Task 4).
4. A normal-looking unity-gain test can conceal wet double-processing or leaked live input: use distinct non-unity graph/master/input/output sentinels and real production routing (Task 6).
5. A take can save audio successfully while rig state fails, or finish preparation after a patch changes: distinguish partial context and reject stale capture starts (Task 7).

## Checkout, build, and reference baseline

Planning baseline: `D:\Code\Pedalboard2`, `codex/rt-hosting-sprint`, commit
`279ba00`. Preserve any later unrelated edits. At execution, inspect Git/worktree
identity before choosing isolation; reuse a suitable existing checkout, do not
create a worktree or move these changes merely because the template mentions one.

**Build-location prerequisite:** the current `build/CMakeCache.txt` names the old
`C:/Users/estee/Desktop/My Stuff/Code/Antigravity/Pedalboard2` as its source and
JUCE location. That old source path still exists. Before compiling, prove the
chosen build actually consumes this checkout's changed sources. Do not report
an old-C: build as verification of D: edits. Diagnose this once against the
known-good tree; if recovery needs a new build tree, present its D: path/size and
obtain any required authority rather than deleting caches or building multiple trees.

Once the existing tree is verified/repaired in scope, the canonical commands are:

```powershell
cmake --build build --config Release --target Pedalboard3 Pedalboard3_Tests -- /m:1
.\build\tests\Release\Pedalboard3_Tests.exe "[scratch]"
```

Each task specifies a narrower Catch2 tag. Use the minimum test-target compile
needed to establish its failing regression, then one non-clean green app/test
build and focused run. Do not rerun the whole suite per task. Missing symbols
can establish the initial red compile for a new interface; unrelated toolchain
errors cannot. Run the full suite only at Task 8's explicit integration checkpoint.
In task steps below, `Pedalboard3_Tests.exe` is shorthand for the verified
`.\build\tests\Release\Pedalboard3_Tests.exe` path shown above, not a PATH lookup.

Reference-first record: use the spec's pinned-source table, JUCE repository
`https://github.com/juce-framework/JUCE`, SHA
`f8f8864172464b9adf9eba6101e1f784838d1597`. Reuse modules as the existing
AGPLv3 dependency; example code is ISC and requires its notice if copied.
Before implementing worker resampling, inspect that same pin's
`juce_ResamplingAudioSource.cpp` and `juce_AudioFormatReaderSource.cpp`; their
locks/reads are acceptable only on the worker. Record newly inspected files and
reuse mode in the implementation handoff. No new third-party library is planned.
Context7 is not exposed in this planning session; rediscover it at execution and
use it for API verification when available, otherwise record the gap and verify
against pinned local source/official docs. Do not install a connector implicitly.

## File and dependency map

Paths are relative to the repository root. `{h,cpp}` means a header/source pair.
Every new production pair is registered in root `CMakeLists.txt`; register the
testable implementation and test in `tests/CMakeLists.txt` in its owning task.
Do not create empty scaffolds in a separate commit.

| Task | Owned production changes | Test file / tag | Depends on |
|---|---|---|---|
| 1 | `ScratchTake.*`, `ScratchRecorder.*`, new `ScratchBundleIO.{h,cpp}` | `scratch_recorder_test.cpp`, new `scratch_take_storage_test.cpp` / `[scratch-storage]` | Build-location check |
| 2 | New `ScratchTakeLibrary.{h,cpp}`, `MainPanel.*` library glue | New `scratch_take_library_test.cpp` / `[scratch-library]` | 1 |
| 3 | New `ScratchWaveformView.{h,cpp}`, `ScratchPanel.*`, layout/presentation helpers | New `scratch_waveform_test.cpp`, `scratch_panel_test.cpp` / `[scratch-waveform]`, `[scratch-ui]` | 2 |
| 4 | New `ScratchPlayback.{h,cpp}`, `ScratchPlaybackTypes.h`, `ScratchPcmQueue.{h,cpp}` | New `scratch_playback_test.cpp` / `[scratch-playback]` | 1 |
| 5 | Extract `dsp/SafetyLimiterCore.{h,cpp}`, adapt `SafetyLimiter.*`, new `ScratchPreviewGuard.{h,cpp}` | New `scratch_preview_guard_test.cpp`, existing `rt_hosting_sprint_test.cpp` / `[scratch-safety]`, `[rt]` | 4 |
| 6 | New `ScratchAudioRouter.{h,cpp}`, `MainPanel.h/.cpp`, `ScratchRecorder.*` block lease | New `scratch_audio_routing_test.cpp`, existing recorder tests / `[scratch-routing]` | 3–5 |
| 7 | New `ScratchRigSnapshot.{h,cpp}`, `MainPanel.*`, `ScratchRecorder.*`, `ScratchTake.*` | New `scratch_rig_snapshot_test.cpp` / `[scratch-rig]` | 1, 6 |
| 8 | `ScratchPanel.*`, `ScratchPanelLayout.h`, `ScratchPanelPresentation.h`, docs | `scratch_panel_test.cpp` / `[scratch-ui]`; integration/manual evidence | All |

Tasks 1–3 make a useful library/waveform slice; Tasks 4–6 integrate audition;
Tasks 7–8 finish context/polish. Keep one plan because these slices share take
identity, service ownership, and the same recording/audition boundary. Code
tasks should run in dependency order; documentation can continue in parallel
with file ownership agreed in advance.

## Task 1: Durable take bundles and truthful capture metadata

**Files:** `src/ScratchTake.{h,cpp}`, `src/ScratchRecorder.{h,cpp}`;
create `src/ScratchBundleIO.{h,cpp}`, `tests/scratch_take_storage_test.cpp`;
modify `tests/scratch_recorder_test.cpp`, both CMake lists, CHANGELOG.md.

**Interfaces:** retain `ScratchTake::createPending`, `writeMetadata() const -> bool`,
and existing recorder start/stop APIs. Add:

```cpp
enum class ScratchTakeLoadState { Ready, Incomplete, Invalid, Unsupported };
enum class ScratchSnapshotState { None, Pending, Saved, Failed };
// In ScratchTake: int schemaVersion; optional<uint64_t> rawDurationSamples,
// wetDurationSamples; String captureStatus, snapshotMessage;
// ScratchSnapshotState snapshotState; File rigFile.
struct ScratchTakeLoadResult {
    ScratchTake take;
    ScratchTakeLoadState state;
    juce::String message;
    bool rawReadable = false, wetReadable = false;
};
ScratchTakeLoadResult loadScratchTake(const juce::File& takeDirectory);
using ScratchFileReplace = std::function<bool(const juce::File&, const juce::File&)>;
juce::Result writeScratchTextAtomically(const juce::File& target,
    const juce::String& text, const ScratchFileReplace& replace = {});
```

The replace seam accepts `(temporary, target)` and is control/worker-only;
production uses sibling temporary-file replacement. It enables a small,
deterministic failed-replace test without a platform-specific permission harness.

- [ ] Add `[scratch][scratch-storage]` tests with these assertions:
  `schemaVersion == 2`; raw/wet counts are distinct when unequal;
  legacy missing counts remain unknown, not fabricated;
  `rawFile.getParentDirectory() == movedTakeDirectory` after relocation;
  metadata bytes are unchanged after every load; failed replacement preserves
  previous bytes and leaves no temporary file. Reject wrong types, negative or
  overflowing counts, non-finite/nonpositive rates, >16 channels, traversal,
  absolute external paths, and JSON larger than 1 MiB. Version >2 returns
  Unsupported and remains revealable. Invalid JSON produces one invalid row.
  Name the central case `Scratch storage preserves relocated legacy bundles`;
  after arranging a legacy bundle and moving it, its core assertions are:

  ```cpp
  const auto loaded = loadScratchTake(movedDirectory);
  REQUIRE(loaded.take.rawFile == movedDirectory.getChildFile("raw.wav"));
  REQUIRE_FALSE(loaded.take.rawDurationSamples.has_value());
  REQUIRE(loaded.take.metadataFile.loadFileAsString() == originalMetadata);
  ```
- [ ] Establish RED with the storage tag (or missing new declarations), not a
  broad suite. Existing recorder tests stay intact except abandoned fake-UI tests.
- [ ] Implement bounded schema parsing and v2 writing. Retain legacy fields,
  `complete`, shared minimum duration, and capture context. New `rawFile`,
  `wetFile`, `rigFile` paths are relative; legacy absolute paths are ignored in
  favor of canonical siblings. Reject symlink/reparse escapes on metadata/audio
  as well as directories. Capture statuses are `recording`, `complete`,
  `interrupted`, `failed`; missing legacy schema means version 1. Inspect WAV
  headers off-thread; complete metadata alone cannot certify a readable file.
- [ ] Persist initial incomplete metadata before publishing Recording. On stop,
  close both writers, persist actual independent frame counts and the existing
  completion/failure result. Metadata failure prevents a successful capture state;
  never destroy an earlier valid manifest to report the failure. Permit readable
  interrupted WAV audition with a warning, not complete-pair status. Unknown
  schemas and invalid metadata expose Reveal only.
- [ ] Run the green incremental build and `Pedalboard3_Tests.exe "[scratch]"`.
  Require zero failed assertions; inspect the write/drop/lifetime regressions,
  not only new parser tests. Update CHANGELOG and HOWTO's current metadata facts.
- [ ] Commit exact task files: `feat: persist versioned scratch take bundles`.

## Task 2: Background discovery and persistent library identity

**Files:** create `src/ScratchTakeLibrary.{h,cpp}` and
`tests/scratch_take_library_test.cpp`; modify `MainPanel.{h,cpp}`, both CMake
lists, CHANGELOG.md, ROADMAP.md, ARCHITECTURE.md.

**Interfaces:** consumes `loadScratchTake`; produces:

```cpp
struct ScratchLibrarySnapshot {
    juce::File root;
    uint64_t generation = 0;
    bool scanning = false;
    std::vector<ScratchTakeLoadResult> entries;
    juce::String message;
};
// ScratchTakeLibrary public control-thread API:
void setRoot(const juce::File&);
void refresh();
void upsertFinalizedTake(const ScratchTake&);
std::shared_ptr<const ScratchLibrarySnapshot> snapshot() const;
```

The immutable snapshot pointer is for workers/message thread only, never audio.
`MainPanel::getScratchLibrarySnapshot() const` forwards the same return type.

- [ ] Test `[scratch][scratch-library]`: 12 bundles survive service recreation;
  two directories with the same `takeId` produce two rows; corrupt JSON does not
  hide neighbors; newest-first order is deterministic on equal timestamps;
  root A completing after root B cannot publish A; refresh cannot remove a
  newly finalized take from a later generation; excluded links are not followed.
  Assert root/metadata/audio contents are unchanged after discovery.
  In `Scratch library rejects obsolete root results`, release the blocked A
  scan only after B publishes, then assert against the actual service:

  ```cpp
  const auto result = library.snapshot();
  REQUIRE(result->root == rootB);
  REQUIRE(result->entries.size() == 12);
  REQUIRE_FALSE(result->scanning);
  ```
- [ ] Establish RED, then implement one cancellable scan worker over exactly
  root/day/take depth. Canonical directory is identity; use platform-correct path
  case handling. Publish sorted immutable batches of at most 128 new entries per
  update. Missing/inaccessible roots produce a visible status without creating
  directories. Include malformed bundles as diagnostic entries.
- [ ] Wire startup, panel-open, explicit Refresh, root changes, and finalization.
  Add `ScratchRecorder::onTakeFinalized` of type
  `std::function<void(const ScratchTake&)>`, invoked on the message thread after
  releasing the state lock. Preserve the recorder's small recent list for current
  callers, but stop using it as the panel's library. Own and join the scanner
  outside the callback; cancel callbacks before MainPanel destruction.
- [ ] Run green build and `Pedalboard3_Tests.exe "[scratch-library],[scratch-storage]"`.
  Record scanning/decode thread identities in focused tests, not timing sleeps.
  Update docs to distinguish persistent library behavior from capture state.
- [ ] Commit `feat: reload scratch takes from disk` with task files and changelog.

## Task 3: Real waveforms and virtualized take selection

**Files:** create `src/ScratchWaveformView.{h,cpp}`,
`tests/scratch_waveform_test.cpp`, `tests/scratch_panel_test.cpp`;
modify `ScratchPanel.{h,cpp}`, `ScratchPanelLayout.h`,
`ScratchPanelPresentation.h`, CMake lists, CHANGELOG.md, HOWTO.md.

**Interfaces:** consumes library snapshots. `ScratchWaveformView` is a component:

```cpp
void setTake(const ScratchTakeLoadResult&);
void clear();
void setPlayheadSeconds(double);
std::function<void(double)> onSeek;
```

Its two AudioThumbnails share a cache capped at 32 entries and use 512 samples
per overview bucket. Store selected canonical directory independently of row
index. Introduce `ScratchPanel::selectedTakeDirectory`, not a global selected row.

- [ ] Test `[scratch][scratch-waveform]` using small actual WAV fixtures: silence
  yields zero min/max; an impulse appears only in its expected time region;
  distinct raw/wet contents stay distinct; channels and duration match headers;
  replacing selection while loading cannot republish the first take. Test the
  production thumbnail/view data path, not a synthetic waveform algorithm.
  In `Scratch waveforms reflect decoded raw and wet samples`, use the real
  thumbnail min/max results from a silent raw fixture and 0.75 wet impulse:

  ```cpp
  REQUIRE(rawMinimum == 0.0f);
  REQUIRE(rawMaximum == 0.0f);
  REQUIRE(wetMaximum == Catch::Approx(0.75f).margin(0.02f));
  REQUIRE(wetDurationSeconds == Catch::Approx(1.0).margin(1.0 / 48000.0));
  ```
- [ ] Test `[scratch][scratch-ui]` selection by directory survives newest-first
  insertion and generation replacement; invalid/unsupported entries show their
  reason and Reveal; >8 entries remain reachable through recycled list rows.
  Put `int findScratchTakeRow(const ScratchLibrarySnapshot&, const juce::File&)`
  in ScratchPanelPresentation.h and use it in production for stable selection;
  test this shared helper, not a reimplemented test-only selection algorithm.
- [ ] Establish RED, then replace nested paint-only RecentTakesList with a JUCE
  virtualized ListBox and recycled row components. Use real focusable action
  buttons. Load selected waveforms first, then only visible row thumbnails;
  destroy stale source requests and invalidate cache identity when a finalized
  file's size/modification time changes. Do not load all WAVs during a UI tick.
- [ ] Remove `drawThumbnailTrace` and decorative lane curves. Label every
  displayed channel; use a shared time range for RAW/WET. Empty/loading/missing
  states show text, not invented signal. Expose seek/playhead integration points;
  until Task 6, do not claim that the old external player can seek in-app.
- [ ] Run green build and `Pedalboard3_Tests.exe "[scratch-waveform],[scratch-ui]"`.
  Keep screenshot checks proportional; do not snapshot the whole panel yet.
- [ ] Commit `feat: show real scratch waveforms and persistent take selection`.

## Task 4: Bounded worker-decoded audition transport

**Files:** create `src/ScratchPlaybackTypes.h`,
`src/ScratchPlayback.{h,cpp}`, `src/ScratchPcmQueue.{h,cpp}`,
`tests/scratch_playback_test.cpp`; modify CMake lists and CHANGELOG.md.

**Interfaces:** all transport commands are message-thread calls; readBlock is
the sole audio consumer. Fixed-size result types contain no strings/ownership.

```cpp
enum class ScratchPlaybackMode { OriginalWet, CurrentRigRaw };
enum class ScratchPlaybackState { Idle, Loading, Playing, Paused, Stopped, Error };
// Worker-only decoder seam. Production default uses pinned JUCE primitives.
class ScratchPcmDecoder {
public:
    virtual ~ScratchPcmDecoder() = default;
    virtual juce::Result open(const juce::File&, double deviceRate) = 0;
    virtual int channels() const = 0;
    virtual int64_t lengthFrames() const = 0; // destination-rate frames
    virtual void seek(int64_t destinationFrame) = 0;
    virtual int read(float* const* destination, int frames) = 0;
};
using ScratchDecoderFactory = std::function<std::unique_ptr<ScratchPcmDecoder>()>;
struct ScratchChannelMap {
    std::array<int, 16> sourceForDestination; // -1 = silence, never sum sources
    int destinationChannels = 0;
    bool explicitMapping = false;
};
struct ScratchPlaybackDevice {
    double sampleRate = 0;
    int maxBlockSize = 0, graphInputChannels = 0, outputChannels = 0;
};
struct ScratchPlaybackStatus {
    ScratchPlaybackState state;
    ScratchPlaybackMode mode;
    juce::File takeDirectory;
    double positionSeconds = 0, durationSeconds = 0, volumeDb = -12;
    bool looping = false, buffering = false, replacingLive = false;
    juce::String message;
};
struct ScratchPlaybackBlock {
    ScratchPlaybackMode mode;
    uint64_t generation = 0;
    int validFrames = 0;
    float previewGain = 0.25118864f; // -12 dB; no UI status read on the callback
    bool replacingLive = false;
};
// ScratchPlayback:
explicit ScratchPlayback(ScratchDecoderFactory factory = {});
void prepare(const ScratchPlaybackDevice&); // device stopped, allocates
juce::Result play(const ScratchTakeLoadResult&, ScratchPlaybackMode,
                  const ScratchChannelMap&);
void pause();
void resume();
void stop();
void seek(double seconds);
void setLooping(bool);
void setVolumeDb(double); // clamp [-60, 0], -60 acts as silence
ScratchPlaybackStatus status() const;
ScratchPlaybackBlock readBlock(float* const* pcm, int channels, int samples) noexcept;
bool isQuiescent() const noexcept;
void audioDeviceStopped(); // only after the consumer can no longer run
```

- [ ] Test `[scratch][scratch-playback]` with production queue/transport and an
  injected decoder source: FIFO wraparound; exact frame order; underflow clears
  unwritten destinations; pause holds position; stop/EOF releases live replacement;
  loop returns to frame zero; seek clamps to the file; generation changes discard
  queued old samples; a stalled decoder cannot stall readBlock or acknowledge
  old data as new. Test stop/device removal without another callback, bounded
  rapid command coalescing, and repeated prepare/shutdown without use-after-free.
- [ ] Add a real-reader test: 1-second 440 Hz WAV at 44.1 kHz auditioned at
  48 kHz retains 1-second duration within one destination frame and frequency
  within 1 Hz after excluding converter warmup. Same-rate playback preserves
  samples before gain/ramping. Test 1/2/16-channel maps, mono wet mirroring, mono
  raw non-duplication, and explicit drop/reroute versus rejected implicit loss.
  Empty/zero-frame audio must fail with a reason before replacing live input.
  In `Scratch seek never consumes the retired generation`, queue only -0.25
  in the old generation and +0.5 in the new one; exclude the documented fade:

  ```cpp
  REQUIRE(block.generation == requestedGeneration);
  REQUIRE(block.validFrames == 128);
  for (int i = 0; i < block.validFrames; ++i)
      REQUIRE(destination[i] == Catch::Approx(0.5f));
  REQUIRE_FALSE(playback.status().buffering);
  ```
- [ ] Establish RED and inspect the pinned JUCE reader/resampling source. Use
  JUCE decoding/resampling only on the worker. Preallocate a 65,537-frame SPSC
  queue for at most 16 channels (65,536 usable frames, about 4 MiB PCM), plus
  fixed callback scratch no larger than 16 x 8,192 samples. Prime two maximum
  callback blocks before Playing; short files prime up to EOF. No whole-file load.
- [ ] Implement a requested/acknowledged generation protocol: consumer stops
  reading the old queue and acknowledges at a block boundary; worker stops
  producing before resetting it. Publish the new generation only after priming.
  Commands are coalesced off-thread; queue reset/free requires both sides idle.
  Never call `AbstractFifo::reset` concurrently with a read/write. Device stop
  explicitly supplies quiescence when callbacks have ceased. Do not force-kill
  a worker or destroy state it may still access.
- [ ] Clear every destination block before reading; report actual consumed
  position, not decoded-ahead position. Use a 5 ms fade for starts/stops/seeks
  and underflow, including continuity from the last sample on underflow. While
  Paused/Loading after an audition request, keep live input excluded until Stop;
  failed initial loading returns to live. EOF stops unless Loop is enabled.
- [ ] Run green build and `Pedalboard3_Tests.exe "[scratch-playback]"`.
  Add a targeted negative control that removes the generation check and prove
  the stale-sample test fails; restore the implementation immediately afterward.
- [ ] Commit `feat: add bounded scratch audition transport`.

## Task 5: Preview protection without replacing the safety singleton

**Files:** create `src/dsp/SafetyLimiterCore.{h,cpp}`,
`src/ScratchPreviewGuard.{h,cpp}`, `tests/scratch_preview_guard_test.cpp`;
modify `src/SafetyLimiter.{h,cpp}`, `tests/rt_hosting_sprint_test.cpp`, CMake
lists, CHANGELOG.md. Read juce-dsp safety/testing references before this task.

**Interfaces:** extract the existing algorithm, not a new limiter policy.
`SafetyLimiterCore` owns DSP state only and exposes
`prepare(double sampleRate)`, `reset() noexcept`, and
`bool process(float* const* data, int channels, int samples) noexcept`
(true means a safety trip). Max channels is 16; existing graph wrapper continues
to pass its existing two-channel limit. No singleton registration in the core.
`ScratchPreviewGuard` owns an independent core and exposes those same prepare/
reset methods plus `bool process(float* const*, int, int, bool muted) noexcept`.

- [ ] Establish characterization tests against the existing production limiter:
  normal signals, DC, overload, non-finite values, silence and release preserve
  current results/thresholds. Test that preparing/using/destroying a preview guard
  never changes `SafetyLimiterProcessor::getInstance()`.
- [ ] Add `[scratch][scratch-safety]` cases for a corrupt sample on channel 16,
  linked protection across all preview channels, finite output bounded to
  [-1, 1], mute producing silence, and no automatic unmute after a source switch.
  Use actual protection core/guard, not copied test logic.
  In `Scratch preview guard does not steal the limiter instance`, retain the
  graph instance pointer before preparing an independent guard and assert:

  ```cpp
  REQUIRE(SafetyLimiterProcessor::getInstance() == graphLimiter);
  REQUIRE(guard.process(channels, 16, 128, true) == false); // already muted
  for (int ch = 0; ch < 16; ++ch)
      for (int i = 0; i < 128; ++i)
          REQUIRE(channels[ch][i] == 0.0f);
  ```
- [ ] Extract exact existing coefficient/timing/limiting behavior. Keep meters
  and atomics in the wrapper. Add
  `SafetyLimiterProcessor::requestMute() noexcept` to publish a preview trip
  through the existing mute/notification state. Consume reset/unmute requests
  on the processing thread so UI calls cannot race DSP state reset. Do not
  reset a latched mute merely to prepare a new take or device rate.
- [ ] Run green build and
  `Pedalboard3_Tests.exe "[scratch-safety],[rt]"`. Require characterization and
  preview cases both green. Document the narrow extraction, not a safety-policy
  upgrade or a claim that unrelated live-output weaknesses are fixed.
- [ ] Commit `refactor: share scratch preview protection without singleton changes`.

## Task 6: Block-consistent wet/reamp routing and recording handoff

**Files:** create `src/ScratchAudioRouter.{h,cpp}`,
`tests/scratch_audio_routing_test.cpp`; modify `MainPanel.{h,cpp}`,
`ScratchRecorder.{h,cpp}`, recorder tests, panel transport glue, CMake lists,
CHANGELOG.md, ARCHITECTURE.md, HOWTO.md.

**Interfaces:** ScratchAudioRouter owns preallocated PCM/zero/discard buffers,
not plugin instances. Its hooks are function pointers, not allocating closures:

```cpp
struct ScratchRenderHooks {
    void* context = nullptr;
    void (*processLivePath)(void*, const float* const*, int,
                           float* const*, int, int) = nullptr;
    void (*protectPreview)(void*, float* const*, int, int) = nullptr;
};
// ScratchAudioRouter:
void prepare(const ScratchPlaybackDevice&);
void process(ScratchPlayback&, ScratchRecorder* recorder,
             const float* const* hardwareInput, int inputChannels,
             float* const* outputs, int outputChannels, int samples,
             ScratchRenderHooks) noexcept;
```

`processLivePath` is the extracted existing input-gain -> graph -> master-insert
-> output-gain code in MeteringProcessorPlayer. The existing callback context is
carried in the synchronous hook context. Protection uses Task 5; preview volume
is applied after the selected render path, in both audition modes. Read its
latched gain from ScratchPlaybackBlock, never from status() on the audio thread.

- [ ] Test `[scratch][scratch-routing]` through the production router with fixed
  hook probes: hardware input 0.1, raw file 0.2, wet file 0.3; non-unity gains
  and a nonlinear graph make all routes distinguishable. Assert wet output
  equals wet PCM times preview gain (apart from protection/ramp), raw traverses
  the full live path once, and live hardware never leaks into either mode.
  Assert no nodes/connections are added and no settings are changed.
  In `Scratch wet bypasses the current processing path`, inspect the audible
  output after the fade and before the independent protection changes anything:

  ```cpp
  REQUIRE(output[127] == Catch::Approx(0.3f * block.previewGain));
  REQUIRE(probe.lastInputPeak == 0.0f); // graph serviced with zero input
  REQUIRE(rawSink.samplesWritten == 0);
  REQUIRE(wetSink.samplesWritten == 0);
  ```
- [ ] Test that wet mode still services the graph with zero input into discarded
  output, preserving MIDI consumption while suppressing generators/tails from
  the audible preview. Test Stop restores live input after its ramp; channel
  maps zero every unmapped destination; safety mute also suppresses wet playback.
- [ ] Add a recorder block lease: `bool beginDeviceBlock(const float* const*, int,
  int) noexcept` acquires one in-flight writer lease and writes raw;
  `void endDeviceBlock(float* const*, int, int) noexcept` writes wet and releases
  it. A true begin requires exactly one end, even if stop is requested between
  taps. A false begin requires none. Keep old individual writer methods for
  existing tests/callers but use the lease in production. Test stop between taps
  keeps the admitted pair together and cannot retire either sink mid-block.
  The router's optional recorder pointer is non-owning; it acquires/releases this
  lease around the live render only after latching mode. Hooks themselves do not
  invoke recorder taps, including the discarded wet-mode graph render.
- [ ] Latch source mode once per callback. The live path alone may begin a
  capture lease. REC during audition requests stop and starts capture only after
  quiescence/live handoff; reject playback during Preparing/Recording/Saving.
  Never publish recorder start midway through a block's raw/wet eligibility.
  Register app-owned playback/router before adding the audio callback; remove
  the callback before releasing them. Stop on patch/document/device changes.
- [ ] Replace external wet launch/unconnected FilePlayer insertion with the
  service. Add MainPanel forwarding methods `pauseScratchPlayback()`,
  `resumeScratchPlayback()`, `stopScratchPlayback()`,
  `seekScratchPlayback(double)`, `setScratchPlaybackLooping(bool)`,
  `setScratchPlaybackVolumeDb(double)`, and
  `getScratchPlaybackStatus() const -> ScratchPlaybackStatus`.
  Extend play/reamp actions to accept a `ScratchChannelMap`; on mismatch show
  destination-to-source selectors (-1 = Off) before starting, requiring explicit
  acceptance of dropped channels. Never change device configuration.
- [ ] Connect selected waveform seek and Play/Pause/Stop/Loop/volume controls.
  Selection change or panel close stops audition; selection alone is silent.
  Show original/current-rig mode and temporary live-input replacement persistently.
- [ ] Run green build and
  `Pedalboard3_Tests.exe "[scratch-routing],[scratch-playback],[scratch-safety],[scratch-storage]"`.
  Deliberately route wet through the probe gain once and assert the negative
  control fails; restore correct routing. Existing master-bus simulations are
  not evidence of real callback integration; inspect the actual hook wiring.
- [ ] Commit `feat: audition and reamp scratch takes without graph edits`.

## Task 7: Capture-time rig snapshot and cancellable preparation

**Files:** create `src/ScratchRigSnapshot.{h,cpp}`,
`tests/scratch_rig_snapshot_test.cpp`; modify `MainPanel.{h,cpp}`,
`ScratchRecorder.{h,cpp}`, `ScratchTake.{h,cpp}`, panel state labels,
CMake lists, CHANGELOG.md, HOWTO.md, ARCHITECTURE.md.

**Interfaces:** `ScratchRigSnapshotData` contains `unique_ptr<XmlElement> patch`,
`MemoryBlock masterInsertState`, `bool masterInsertBypassed`, arrays of 16
input/output channel dB values, master input/output dB values, appVersion,
`ScratchTakeContext context`, and `vector<ScratchAssetReference> assets`.
`ScratchAssetReference` contains `String nodePath, kind; File file`.
Pure serializer: `juce::String serializeScratchRig(const ScratchRigSnapshotData&)`.
Worker write: `juce::Result writeScratchRig(const File& takeDirectory, const String&)`.

- [ ] Test `[scratch][scratch-rig]`: an unsaved probe plugin state is preserved in
  serialized live Patch XML; master rack bytes round-trip separately; gains,
  bypass, channel/rate context and known NAM/IR/file references survive. Failed
  sidecar replacement leaves snapshot Failed plus reason without falsely failing
  a valid audio pair. Legacy takes show None, not a broken-snapshot error.
- [ ] Build `ScratchRig schemaVersion="1"` around the existing live Patch XML,
  master rack state, gains and asset references. Do not call cached patch/document
  save APIs. MainPanel gathers plugin state on the existing control-thread path;
  a worker writes detached XML atomically using Task 1's helper. Enumerate known
  NAM/IR/FilePlayer references through their existing accessors/state, including
  nested racks. Opaque plugin dependencies remain explicitly unenumerated.
  Fix sidecar names now: direct child `Patch`; `MasterInsert` with `bypassed`
  attribute and base64 text; `Gains` with `masterInputDb`/`masterOutputDb` and
  `Input`/`Output` children carrying `channel`/`gainDb`; `Assets/Asset` entries
  carrying `nodePath`/`kind`/`path`; root `appVersion` plus `Device` context.
  In `Scratch snapshot distinguishes audio success from context failure`, after
  a failed sidecar write followed by matched successful WAV writes, assert:

  ```cpp
  const auto finalTake = recorder.getStatus().lastTake.value();
  REQUIRE(finalTake.complete);
  REQUIRE(finalTake.snapshotState == ScratchSnapshotState::Failed);
  REQUIRE_FALSE(finalTake.snapshotMessage.isEmpty());
  REQUIRE_FALSE(finalTake.rigFile.existsAsFile());
  ```
- [ ] Introduce Preparing to ScratchRecorderState and control-thread APIs
  `juce::Result prepareTake(const ScratchTakeContext&)`,
  `juce::Result startPreparedTake()`, `void cancelPreparedTake(const String&)`.
  Keep `start(context)` as a convenience for non-snapshot callers/tests. Preparing
  creates one pending bundle/manifest, not two. Tag preparation with a monotonic
  request ID and patch/device generation. Completed old writes cannot start
  recording after Stop, a root/device/patch change, or a newer REC request.
- [ ] Add `void setPreparedSnapshotResult(ScratchSnapshotState, const String&)`
  to update pending metadata on the control thread. Saved or Failed snapshot
  completion permits capture unless cancelled; audio and snapshot success stay
  distinct. Disable destination/actions during Preparing and Saving as well as
  Recording. Capture snapshot exactly once at start, never in getStatus/paint.
- [ ] Test pending cancellation, late completion, snapshot failure warning, and
  REC double-click using controllable worker completion (no arbitrary sleeps).
  Test the actual preparation controller methods, not a mirrored state machine.
  Gatherer uses live serializers in MainPanel; record app integration evidence
  with a changed unsaved plugin parameter and a master insert, not just a fixture.
- [ ] Run green build and `Pedalboard3_Tests.exe "[scratch-rig],[scratch-storage],[scratch-routing]"`.
  Verify original patch/device settings and all prior take bytes are unchanged.
- [ ] Commit `feat: save capture-time rig context with scratch takes`.

## Task 8: Integrated UI polish and acceptance checkpoint

**Files:** `src/ScratchPanel.{h,cpp}`, `src/ScratchPanelLayout.h`,
`src/ScratchPanelPresentation.h`, `tests/scratch_panel_test.cpp`, CHANGELOG.md,
ROADMAP.md, STATE_OF_PROJECT.md, HOWTO.md, ARCHITECTURE.md, readme.md, and a
dated implementation handoff under `docs/` containing verification evidence.

**Interfaces:** no new audio/storage APIs. Consume prior task snapshots and
forward explicit commands only. Existing semantic fonts/colors remain authoritative.

- [ ] Add `[scratch][scratch-ui]` tests for reachable transport/action bounds at
  narrow/default sizes; disabled-reason presentation in Preparing/Recording/
  Saving; selection stability; keyboard focus; Space ignored by text editors and
  other windows; Escape stops audition; Ctrl+Shift+R stays unchanged. Delete
  tests of removed fake-waveform/paint-only row behavior with a documented reason.
  Keep key arbitration testable in the production presentation helper:
  `bool shouldHandleScratchSpace(bool panelFocused, bool textInputFocused) noexcept`.
  In `Scratch shortcuts leave text input and other windows alone`, assert:

  ```cpp
  REQUIRE(shouldHandleScratchSpace(true, false));
  REQUIRE_FALSE(shouldHandleScratchSpace(true, true));
  REQUIRE_FALSE(shouldHandleScratchSpace(false, false));
  ```

  Test helper contracts automatically; actual focus traversal and shortcut
  dispatch still require the running-panel check below. Do not label a helper
  test as proof that all keyboard wiring works.
- [ ] Tighten duplicate status badges and empty space, make selected RAW/WET
  lanes/transport primary, use `Takes`, `Play original`, `Reamp current rig` labels,
  and show channel, capture, snapshot and buffering warnings. All buttons have
  normal/hover/pressed/disabled/focus states. Destination actions remain secondary
  but reachable. Stack controls instead of clipping at narrow widths; retain
  theme switching, FontManager and the existing REC visual identity.
- [ ] Run the final non-clean build, `Pedalboard3_Tests.exe "[scratch],[rt],[masterbus]"`,
  then the integration checkpoint `ctest --test-dir build -C Release --output-on-failure`.
  Separate real production tests from historical/mock test coverage in the handoff.
  Do not invent success for unrun tests or macOS builds.
- [ ] Inspect new screenshots at default/narrow sizes in a dark and light theme,
  plus 150% scale. Use the existing QA script only after reviewing its settings/
  default-patch backup-and-restore behavior; all process TEMP/TMP point at the
  manifested D: scratch root. Do not claim historical screenshots as new evidence.
- [ ] Run the spec's real guitar/interface workflow, including restart discovery,
  original wet audition, current-rig loop while adjusting tone, REC handoff, device
  removal, patch changes, snapshot availability, and raw/wet frame comparison.
  If hardware access is unavailable, record this exact remaining acceptance item
  and do not call the feature user-ready. Do not overwrite the user's recordings.
- [ ] Synchronize docs to verified behavior and list deferred rig restoration,
  portable assets, reamp export, and remaining platform/hardware QA explicitly.
  Audit scratch manifest; remove disposable resources only after validating exact
  paths inside its root and rejecting reparse points. Verify absence, and report
  any deliberately retained artifact's path, size, purpose and retention condition.
- [ ] Commit `feat: polish the integrated scratch take workflow`; request the
  selected method's final independent review. No PR or push unless asked.

## Plan self-review and execution handoff

Coverage: spec library/storage -> Tasks 1–2; waveform/selection -> Task 3;
transport/channel mapping/lifetime -> Task 4; safety -> Task 5;
wet/reamp/REC/device boundary -> Task 6; rig context/preparation -> Task 7;
UI/keyboard/theme/hardware/docs -> Task 8. Review Focus items have explicit
owning tests. New API names above are the cross-task contracts; changes must be
reflected in all dependent tasks before handoff to another implementer.

Recommended execution: **Native**, implementing sequentially in this chat with
documentation updated in parallel and one fresh whole-change review at the end.
Most tasks share the same take/transport interfaces and audio callback; splitting
them among many simultaneous implementers would add integration churn. The
alternative is **Subagent-driven**, with a fresh implementer and reviewer per
task for more intermediate independent checking and higher context cost.

Written-plan review and execution choice are the writing-plans skill's remaining
gate. Do not interpret this document's unchecked steps as completed work.
