# Tuner And Meter Upgrade Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use `superpowers:subagent-driven-development` or `superpowers:executing-plans` to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Upgrade Pedalboard3's tuner and meter foundation from polished-looking utility nodes into truthful, stage-useful tools with tested DSP behavior, RT-safe analysis boundaries, and reusable meter ballistics.

**Status:** In progress as of 2026-06-22 on branch `codex/rt-hosting-sprint`.

**Architecture:** Keep `TunerControl` as a direct-painted JUCE UI surface, but move pitch/tuning behavior behind a small tested analysis core. The audio callback must only perform bounded work: copy into fixed storage, perform lightweight gating, or publish to a fixed analysis queue. Meter work should follow the existing `DeviceMeterTap`/atomic UI polling pattern and mature it with explicit peak/RMS/VU/clip ballistics.

**Tech Stack:** C++17, JUCE 8, Catch2, Pedalboard3 internal processor host, fixed buffers/atomics for audio-to-UI communication.

---

## Scope Check

Included:

- Make tuner capability labels honest: the former `STROBE` view is now a `DRIFT` display over a monophonic YIN estimate, and the six-string view is not a true polyphonic detector.
- Add tested pitch/tuning behavior around note acquire, note hold, silence timeout, confidence, reference pitch, and fast/stable response.
- Decide and implement an RT-safe analyzer boundary before adding heavier detector variants.
- Add focused algorithm tests using generated sine/noise input at multiple sample rates and buffer boundaries.
- Use actual tuner products as references, not only pitch detector libraries.
- Improve the meter architecture using permissive meter source and ballistics references.

Excluded:

- Copying GPL tuner source unless a later task explicitly chooses that route and records license compatibility.
- Claiming true polyphonic tuning in this sprint.
- Adding plugin format support or external tuner plugins.
- Reworking Stage Mode action routing beyond what is required to surface a better global tuner.
- Replacing the current tuner visual design wholesale.

## Current Evidence

- `src/TunerProcessor.cpp` runs YIN directly from `processBlock()` every `ANALYSIS_HOP = 512` samples over a 2048-sample window.
- `src/TunerProcessor.cpp` copies the circular buffer into a stack `std::array<float, BUFFER_SIZE>` on each analysis hop, then runs an O(N^2) difference loop.
- `src/TunerProcessor.h` and `src/TunerControl.h` describe a "Pro" phase-based strobe and `+/-0.1 cent` behavior that the backend does not currently implement.
- `TunerControl` has `Needle`, `PitchDrift`, and `SixString` views. `PitchDrift` is an animated pitch-error display over the detected monophonic pitch, not a real hardware/comparator strobe. The `SixString` view is for visual feedback against the six guitar string references using the detected pitch; it is not simultaneous polyphonic detection.
- `ToneGenerator` tests cover frequency/cents math, but no tests directly exercise `TunerProcessor::detectPitchYIN()` or end-to-end tuner detection accuracy.
- `DeviceMeterTap` already provides device-level atomic meter values for Audio I/O nodes and Soundcheck, but the current meter model is peak-with-decay rather than a reusable source with explicit peak/RMS/VU/clip semantics.
- `VuMeterDsp.h` references a GPL VU source as inspiration; future close ports should prefer permissive references unless a GPL copy path is intentional.

## Reference-Code-First Record

| Project | Commit | License posture | Files inspected | Reuse mode |
| --- | --- | --- | --- | --- |
| `Fannon/trace-tuner` | `e4651fb89220783d4ad984a995590daa5bdbec8b` | MIT | `src/core.rs`, `src/ui.rs` | close-port / pattern-only for state machine, thresholds, response modes, and history trace |
| `googlearchive/guitar-tuner` | `35bdc95a2388742e8e56c3c20c390b64797c7c97` | Apache-2.0 | `src/elements/audio-visualizer/audio-visualizer.html`, `src/elements/tuning-instructions/tuning-instructions.html` | pattern-only for central visualizer and direct tune-up/tune-down feedback |
| `jbergknoff/guitar-tuner` | `ab22383925407d93d138c26d1eb776d43c23bd5c` | MIT | `index.html` | pattern-only for compact note/frequency/confidence presentation |
| `jpsim/ZenTuner` | `74b6862009189f02747c69538e7422c62639b961` | MIT repo; pitch algorithm lineage needs provenance check | `ZenTuner/Models/TunerData.swift`, `ZenTuner/Models/ScaleNote.swift`, `Packages/MicrophonePitchDetector/Sources/ZenPTrack/ZenPTrack.swift`, `Packages/MicrophonePitchDetector/Sources/MicrophonePitchDetector/PitchTracker.swift` | note model, test fixture, UX reference; avoid algorithm copy until provenance is clear |
| `adrielcafe/chroma` | `3f01f3608de295d2490d382dce3fe710196f932c` | MIT | `TunerManager.kt`, `Tuning.kt`, `TuningDeviationResult.kt`, `TunerState.kt` | UX/state/settings reference |
| `duff2013/AudioTuner` | `962f7fb7740c08adc4f6abaa2dc6e8152e0c89bb` | MIT | `AudioTuner.h`, `AudioTuner.cpp` | RT fixed-buffer/decimation pattern reference |
| `x42/tuna.lv2` | `febfd45e0941fae3c47cb8192c13dc5de929271f` | GPL-2.0-or-later source headers; verify before copying | `src/tuna.c`, `gui/tuna.c` | behavior reference for real strobe/PLL semantics |
| `gillesdegottex/fmit` | `bee349de3549cd08b8a82af37dab28372f21c60b` | GPL-2.0 | `libs/Music/CumulativeDiffAlgo.cpp` | behavior reference |
| `ibancg/lingot` | `35e2189e626d4069aca6011fee361e945f68c1d7` | GPL-2.0 | `src/lingot-core.c`, `src/lingot-gui-strobe-disc.c` | behavior reference for analyzer thread and strobe/gauge |
| `dsego/strobe-tuner` | `34aedf67e947ca8099a1629500db0917bbf1a00d` | GPL-3.0 | `core/pitch.odin`, `app/strobe_display.odin` | behavior reference for NSDF confidence and strobe display |
| `ffAudio/ff_meters` | `968bb8e00c5e47aaf18b388341493edc771a088f` | BSD-3-Clause | `LevelMeter/LevelMeterSource.h`, `LevelMeter/LevelMeter.h`, `LevelMeter/LevelMeter.cpp` | close-port / pattern-only for meter source separation |
| `SoundDevelopment/sound_meter` | `a614425d9ae3f6bcb9c94dc335e2f73ccfad211b` | MIT | `meter/sd_MeterLevel.h`, `meter/sd_MeterHelpers.cpp` | close-port / pattern-only for ballistics, scale ticks, peak hold |
| `jiixyj/libebur128` | `67b33abe1558160ed76ada1322329b0e9e058b02` | MIT | `ebur128/ebur128.h` | dependency candidate only if LUFS/true peak becomes in-scope |

Before copying, closely porting, or vendoring any source, add the exact source path, license text, attribution, and change notes to `THIRD_PARTY_LICENSES.md`.

## Acceptance Criteria

- Current tuner UI no longer claims backend capabilities it does not implement.
- Generated audio tests prove correct note/cents detection for stable sine input across at least 44.1 kHz, 48 kHz, and 96 kHz.
- Quiet/noisy input is rejected through a clear confidence/no-signal state instead of flickering stale notes.
- Reference pitch is represented in state and UI logic, even if the first UI control is minimal.
- The chosen analyzer boundary is RT-safe: no heap allocation, no locks, no logging, no file I/O, no unbounded work in the audio callback.
- Stage/global tuner and node tuner continue to share the same processor truth.
- Meter work exposes explicit peak/RMS/VU/clip semantics through atomics or a fixed snapshot, and UI code only polls/decays on the message thread.
- All source reuse is documented with repo, commit, license, files, and reuse mode.

## Task 1: Add Truthfulness And Source-Guard Tests

Files:

- `tests/ui_regression_harness_test.cpp`
- `src/TunerProcessor.h`
- `src/TunerControl.h`
- `src/TunerControl.cpp`

Steps:

- [x] Add a source-level regression that fails if comments/tooltips claim `+/-0.1 cent`, "Pro phase-based strobe", or true polyphonic behavior before the backend supports it.
- [x] Rename user-facing `POLY` copy to an honest six-string/string view label, or record a product reason to keep the label while the backend remains monophonic.
- [x] Keep existing direct-painted node chrome and mode controls intact.

Verification:

```powershell
cmake --build build --config Debug --target Pedalboard3_Tests -- /m:1
.\build\tests\Debug\Pedalboard3_Tests.exe "[ui][regression][visual][source][nodes][tuner]"
```

Commit target: `fix: make tuner capability labels truthful`

## Task 2: Create A Tested Tuner Analysis Core

Files:

- Create: `src/dsp/TunerAnalysis.h`
- Create: `src/dsp/TunerAnalysis.cpp`
- Create: `tests/tuner_analysis_test.cpp`
- Modify: `tests/CMakeLists.txt`

Steps:

- [x] Add a small JUCE-light or JUCE-free core with `prepare(sampleRate, maxBlockSize)`, `reset()`, `pushSamples()`, and `analyze()`/`pollResult()` style API.
- [x] Model result data as POD: frequency, midi note, cents, confidence, no-signal/unstable/stable state, and reference pitch.
- [x] Add generated sine tests for E2, A2, A4, C5, and boundary cents.
- [x] Add silence, low RMS, noise, and decaying note tests.
- [x] Add sample-rate coverage at 44.1 kHz, 48 kHz, and 96 kHz.
- [x] Add block-boundary coverage for block sizes 1, 17, 64, 511, 512, and 1024.

Implementation note: the portable core uses a 4096-sample analysis window. This is intentional because a 2048-sample window cannot cover low E at 96 kHz.

Implementation notes:

- Use `trace-tuner` as the primary permissive reference for confidence/acquire/hold/silence behavior.
- Use `AudioTuner` as a fixed-buffer/decimation reference if low-frequency guitar/bass detection costs are too high.
- Do not copy GPL tuner source in this task.

Verification:

```powershell
cmake --build build --config Debug --target Pedalboard3_Tests -- /m:1
.\build\tests\Debug\Pedalboard3_Tests.exe "[tuner][analysis]"
```

Commit target: `test: cover tuner analysis core`

## Task 3: Choose And Implement The RT Analyzer Boundary

Files:

- `src/TunerProcessor.h`
- `src/TunerProcessor.cpp`
- `src/dsp/TunerAnalysis.*`
- tests

Decision:

- Use a background analyzer boundary for this sprint. `TunerAnalysis` currently performs a 4096-sample YIN pass; keeping that O(N^2) work in `processBlock()` is the wrong reliability shape for a stage utility.
- The audio callback will keep fixed-size ring/snapshot storage only, publish complete windows with atomics, and preserve pass-through/mute behavior.
- The analyzer thread will own `TunerAnalysis`, consume the latest complete snapshot without making the audio thread wait, and publish frequency/note/cents/strobe state through the existing atomics.

Steps:

- [x] Decide between two acceptable boundaries:
  - Audio callback fills fixed analysis storage and performs only bounded lightweight analysis after RMS gating.
  - Audio callback writes to a fixed ring buffer and a background analyzer computes heavier pitch estimates.
- [x] Record the decision in this plan before implementation.
- [x] If using a background analyzer, make thread lifecycle explicit in `prepareToPlay()`, `releaseResources()`, and destruction, and ensure the audio callback never waits on it.
- [x] If keeping analysis on the audio callback, cap work with RMS gating, min/max frequency constraints, and measurable CPU tests. N/A for this sprint because the background analyzer path was chosen.
- [x] Publish results through atomics or double-buffered POD snapshots.
- [x] Preserve mute-output behavior and direct graph pass-through semantics.

Verification:

```powershell
cmake --build build --config Debug --target Pedalboard3_Tests -- /m:1
.\build\tests\Debug\Pedalboard3_Tests.exe "[tuner]"
.\build\tests\Debug\Pedalboard3_Tests.exe "[rt]"
```

Commit target: `feat: isolate tuner analysis from processor hot path`

## Task 4: Add Tuner State, Reference Pitch, And Response Modes

Files:

- `src/TunerProcessor.h`
- `src/TunerProcessor.cpp`
- `src/TunerControl.h`
- `src/TunerControl.cpp`
- tests

Steps:

- [x] Add reference pitch state with safe default `A=440`.
- [x] Add stable/fast response mode state based on the `trace-tuner` pattern.
- [x] Add note acquire/hold behavior so the display does not flicker during short confidence drops.
- [x] Serialize versioned tuner state.
- [x] Surface current reference pitch in the node without crowding the existing direct-painted layout.

Verification:

```powershell
cmake --build build --config Debug --target Pedalboard3_Tests -- /m:1
.\build\tests\Debug\Pedalboard3_Tests.exe "[tuner]"
```

Commit target: `feat: add tuner reference and response state`

## Task 5: Decide Real Strobe Scope

Files:

- `src/dsp/TunerAnalysis.*`
- `src/TunerProcessor.*`
- `src/TunerControl.*`
- tests

Decision:

- Defer a true phase/comparator strobe backend in this sprint. The current YIN-based result is enough for a visual pitch-error drift display, but not enough to claim hardware-style strobe or PLL behavior.
- Rename the user-facing mode from `STROBE` to `DRIFT` and keep GPL strobe projects as behavior references only until a clean-room spec is explicitly written.
- Add processor tests for the pitch-drift phase direction so the visual mode has a concrete, tested meaning even while true strobe is deferred.

Steps:

- [x] Decide whether this sprint implements a real phase/comparator strobe backend or keeps strobe as a visual mode with honest labeling.
- [x] If implementing real strobe, use `x42/tuna.lv2`, `lingot`, and `dsego/strobe-tuner` as behavior references and write a clean-room spec first. N/A for this sprint because true strobe is deferred.
- [x] Add tests proving pitch-drift phase advances in the correct direction for sharp/flat tones.
- [x] If deferring, rename/copy-adjust the view so it does not imply `+/-0.1 cent` hardware-strobe accuracy.

Verification:

```powershell
cmake --build build --config Debug --target Pedalboard3_Tests -- /m:1
.\build\tests\Debug\Pedalboard3_Tests.exe "[tuner]"
```

Commit target: `feat: define tuner strobe backend behavior`

## Task 6: Meter Source And Ballistics Upgrade

Files:

- `src/DeviceMeterTap.h`
- `src/DeviceMeterTap.cpp`
- `src/VuMeterDsp.h`
- `src/PluginComponent.cpp`
- `tests/*meter*`

Steps:

- [x] Introduce a reusable meter source model with explicit peak, RMS, VU, clip, and decay/hold behavior.
- [x] Use `ff_meters` as the primary audio/UI separation reference.
- [x] Use `sound_meter` as the primary ballistics/scale reference.
- [x] Keep `DeviceMeterTap` fixed-size and callback-safe.
- [x] Add tests for peak decay, RMS window behavior, VU response, clip latch/clear, channel bounds, and stopped-device reset.
- [x] Keep existing Audio I/O node meter behavior visually stable unless product polish is explicitly in-scope.

Implementation note:

- Added `PedalboardMeterSource` as a fixed-capacity, no-lazy-allocation source with peak, rolling block RMS, VU one-pole ballistics, and clip latch semantics.
- `SafetyLimiter` and `DeviceMeterTap` now use the shared source and keep legacy peak getters compatible with existing Audio I/O node drawing.
- Removed the unused GPL-inspired `VuMeterDsp.h` path after confirming no production code referenced it.
- `RoutingProcessors` still paints its strip meters from peak-with-decay values for visual stability; converting mixer/splitter strip displays to true VU is a separate UI-visible follow-up.

Verification:

```powershell
cmake --build build --config Debug --target Pedalboard3_Tests -- /m:1
.\build\tests\Debug\Pedalboard3_Tests.exe "[mixer][metering]"
.\build\tests\Debug\Pedalboard3_Tests.exe "[meter]"
```

Latest verification:

```powershell
cmake --build build --config Debug --target Pedalboard3_Tests -- /m:1
cmake --build build --config Debug --target Pedalboard3 -- /m:1
.\build\tests\Debug\Pedalboard3_Tests.exe "[meter]"
.\build\tests\Debug\Pedalboard3_Tests.exe "[mixer][metering]"
.\build\tests\Debug\Pedalboard3_Tests.exe "[rt]"
```

Results:

- `[meter]`: 121 assertions in 8 test cases.
- `[mixer][metering]`: 4 assertions in 1 test case.
- `[rt]`: 3,288 assertions in 17 test cases.

Commit target: `feat: add reusable meter ballistics source`

## Task 6A: Six-String Checklist Feedback

Files:

- `src/TunerProcessor.h`
- `src/TunerProcessor.cpp`
- `src/TunerControl.cpp`
- `tests/tuner_processor_test.cpp`
- `tests/ui_regression_harness_test.cpp`

Steps:

- [x] Treat the six-string view as serial visual feedback for standard guitar tuning, not polyphonic tuning.
- [x] Track low E, A, D, G, B, and high E as octave-specific string slots so E2 and E4 cannot both light from root-letter matching.
- [x] Keep the string checklist outside the audio callback by updating atomics from the analyzer result path.
- [x] Preserve checked-string state across short signal drops until the checklist is explicitly reset.
- [x] Add source guards preventing a regression back to `strings[i].startsWith(root)` string matching.
- [x] Add a compact all-strings-ready status in the direct-painted string view.

Implementation note:

- The processor now publishes `guitarStringInTuneMask`, `currentGuitarStringIndex`, and `currentGuitarStringCents` from the background analyzer path. The UI reads those atomics and renders current/checked states directly.
- The checklist tolerance is intentionally tighter than capture range: a note can be associated with a string while still clearing that string's ready bit if it drifts out of tune.

Latest verification:

```powershell
cmake --build build --config Debug --target Pedalboard3_Tests -- /m:1
.\build\tests\Debug\Pedalboard3_Tests.exe "[tuner]"
.\build\tests\Debug\Pedalboard3_Tests.exe "[ui][regression][visual][source][nodes][tuner]"
.\build\tests\Debug\Pedalboard3_Tests.exe "[rt]"
git -c safe.directory='C:/Users/estee/Desktop/My Stuff/Code/Antigravity/Pedalboard2' diff --check
```

Results:

- `[tuner]`: 483 assertions in 10 test cases.
- `[ui][regression][visual][source][nodes][tuner]`: 103 assertions in 1 test case.
- `[rt]`: 3,288 assertions in 17 test cases.
- `diff --check`: clean, with existing CRLF normalization warnings only.

Commit target: `feat: add tuner string checklist`

## Task 6B: Real Tuner UI Pass

Files:

- `src/TunerProcessor.h`
- `src/TunerProcessor.cpp`
- `src/TunerControl.h`
- `src/TunerControl.cpp`
- `src/StageView.h`
- `src/StageView.cpp`
- `tests/ui_regression_harness_test.cpp`

Steps:

- [x] Expose analyzer confidence to the UI through a processor atomic instead of deriving fake certainty in paint code.
- [x] Add a pitch-history trace to the direct-painted Tuner node, inspired by `trace-tuner` but implemented against Pedalboard3 theme and processor APIs.
- [x] Add a visible signal confidence strip and reference/response rail to the Tuner node.
- [x] Bring Stage/global tuner closer to node parity with note, cents, confidence, recent pitch history, reference/response, and six-string checklist rendering.
- [x] Add source guards so future changes cannot silently collapse the tuner back to a note plus one simple bar.

Reference-code-first note:

- `Fannon/trace-tuner` at `e4651fb89220783d4ad984a995590daa5bdbec8b`, MIT, `src/ui.rs`: pattern-only for confidence meter, large note/cents hierarchy, reference/response controls, and pitch trace with note-break behavior.
- `googlearchive/guitar-tuner` at `35bdc95a2388742e8e56c3c20c390b64797c7c97`, Apache-2.0, tuner visualizer/instruction elements: pattern-only for central signal feedback and direct tuning direction.
- `jbergknoff/guitar-tuner` at `ab22383925407d93d138c26d1eb776d43c23bd5c`, MIT, `index.html`: pattern-only for compact note/frequency/confidence display.
- No source was copied. Reuse mode is pattern-only / clean-room JUCE implementation.

Verification:

```powershell
cmake --build build --config Debug --target Pedalboard3_Tests -- /m:1
.\build\tests\Debug\Pedalboard3_Tests.exe "[ui][regression][visual][source][nodes][tuner]"
.\build\tests\Debug\Pedalboard3_Tests.exe "[tuner]"
.\build\tests\Debug\Pedalboard3_Tests.exe "[rt]"
cmake --build build --config Debug --target Pedalboard3 -- /m:1
powershell -ExecutionPolicy Bypass -File scripts\run_d2_visual_qa.ps1 -Configuration Debug -OutputName 2026-06-22-tuner-ui-pass -NodeSnapshotsOnly -UiScalePercent 100
git -c safe.directory='C:/Users/estee/Desktop/My Stuff/Code/Antigravity/Pedalboard2' diff --check
```

Latest verification:

- `cmake --build build --config Debug --target Pedalboard3_Tests -- /m:1`: passed with existing warnings.
- `[ui][regression][visual][source][nodes][tuner]`: 129 assertions in 1 test case.
- `[tuner]`: 509 assertions in 10 test cases.
- `[rt]`: 3,288 assertions in 17 test cases.
- `cmake --build build --config Debug --target Pedalboard3 -- /m:1`: passed with existing warnings.
- Visual QA node snapshots: `documentation\qa\2026-06-22-tuner-ui-pass`, captured with app UI scale 100% on a 175% OS-scale display.
- `diff --check`: clean, with existing CRLF normalization warnings only.

Commit target: `feat: polish tuner UI from real references`

## Task 7: Manual Instrument And Stage Verification

Steps:

- [ ] Launch the app with a real guitar/interface input.
- [ ] Compare node tuner and Stage/global tuner on the same signal.
- [ ] Test low E, A, D, G, B, high E, plus a quiet signal near the no-signal threshold.
- [ ] Test deliberate sharp/flat offsets against an external trusted tuner.
- [ ] Confirm mute-output still silences the tuner path when expected.
- [ ] Confirm Stage View remains responsive while tuner is active.
- [ ] Confirm Audio I/O node meters and Soundcheck meter readouts remain live and do not clip visually at 75%, 100%, and 150% UI scale.

Record evidence in `TESTLATER.md` until completed.

## Known Risks

- A true strobe backend is more than a visual change; it requires phase/error semantics that the current YIN result alone does not provide.
- O(N^2) pitch analysis in the audio callback may be acceptable on many machines but is the wrong reliability shape for a stage-grade global utility if it spikes under load.
- Background analyzer threads solve RT spikes but add lifecycle and stale-result complexity.
- MIT repo license does not automatically make every algorithm lineage safe to copy; `ZenTuner`'s `ptrack` lineage needs a separate provenance check before any close port.
- Meter UI polish can hide semantics. Peak, RMS, VU, and LUFS are different tools and should not be collapsed into one unlabeled animated bar.
