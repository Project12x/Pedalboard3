# NAM A2 Migration Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add Neural Amp Modeler Architecture 2 support while preserving existing local A1/custom NAM model behavior and the RT-hosting safety improvements already landed in the sprint.

**Status:** In progress as of 2026-06-19 on branch `codex/rt-hosting-sprint`.

**Architecture:** Treat A2 as a two-part migration. First, update TONE3000 discovery/download code so the browser can request A2 models explicitly. Second, replace the vendored NeuralAmpModelerCore integration behind the existing `NAMCore`/`NAMProcessor` boundary, adapting the new multi-channel API, C++20 implementation files, and slimmable model controls without putting model loads, prewarming, or `SetSlimmableSize()` on the audio callback.

**Tech Stack:** JUCE 8, CMake/MSVC, C++17 host code with an expected C++20 NAM-core compile island, Catch2, TONE3000 API v1, NeuralAmpModelerCore v0.5.3, NAM Plugin v0.7.15/v0.7.14 pattern references.

---

## Scope Check

Included:

- Request and display A2 models through TONE3000 API calls.
- Preserve A1/custom fallback and existing local model paths.
- Update vendored NeuralAmpModelerCore from the current `0.1.0` snapshot to a version that can load A2.
- Adapt `NAMCore` and wrappers to the current `nam::DSP` process/reset/channel APIs.
- Add tests that fail if TONE3000 requests omit `architecture=2`, legacy model paths are removed, or A2-only core assumptions leak into the audio callback.
- Keep all model load, unload, prewarm, channel validation, and slimmable-size changes outside `processBlock`.

Excluded:

- ReverbSC work. It is parked until A2 impact is understood.
- New plugin format hosting. Expanded format support remains deferred to JUCE 9.
- Copying GPL/proprietary host/plugin code.
- Replacing the whole NAM UI. UI changes should be only what is required to avoid hiding architecture/version state.

## Current Evidence

- Local vendored NeuralAmpModelerCore reports `0.1.0` in `external/NeuralAmpModelerCore/NAM/version.h`; it cannot load A2.
- Local app and tests default to C++17 in `CMakeLists.txt` and `tests/CMakeLists.txt`.
- Local TONE3000 search requests currently set `platform=nam` but do not set `architecture=2`.
- Local TONE3000 model download lookup currently calls `/models?tone_id=...` without `architecture=2`.
- Local `Tone3000::ToneInfo` has an `architecture` string but does not parse `architecture_version`.
- Local `NAMCore` still uses the older mono pointer API and calls `finalize_(numSamples)`.
- The RT sprint has already moved NAM model/IR swaps out of normal `processBlock`; A2 must preserve that boundary.

## Official A2 Facts

Sources checked on 2026-06-19:

- TONE3000 A2 launch blog, 2026-06-02: `https://www.tone3000.com/blog/introducing-neural-amp-modeler-nam-architecture-2-a2`
- TONE3000 A2 complete guide: `https://www.tone3000.com/guides/nam-a2-the-complete-guide`
- TONE3000 API docs: `https://www.tone3000.com/api`
- NeuralAmpModelerCore releases: `https://github.com/sdatkinson/NeuralAmpModelerCore/releases`
- NeuralAmpModelerPlugin releases: `https://github.com/sdatkinson/NeuralAmpModelerPlugin/releases`
- Context7 docs for `/sdatkinson/neuralampmodelercore`

Facts that drive this plan:

- A2 was announced on 2026-06-02 and is the default for new TONE3000 captures.
- A2 is not automatically surfaced by legacy API queries. TONE3000 documents `architecture=2` for A2; omitting `architecture` returns the legacy A1/custom default and excludes A2-only tones.
- `/tones/search`, `/tones/{id}`, and `/models` all understand the architecture selector for NAM tones.
- Model JSON includes `architecture_version` for NAM models.
- A2-Full and A2-Lite are sizes of one A2 model; TONE3000 does not require separate Full/Lite training.
- NeuralAmpModelerCore `v0.5.2+` is the relevant A2-support floor; `v0.5.3` is current from the release page checked today.
- NeuralAmpModelerCore now uses C++20 in its CMake configuration and a multi-channel `process(NAM_SAMPLE** input, NAM_SAMPLE** output, int frames)` API.
- `nam::SlimmableModel::SetSlimmableSize(double)` is documented as thread-safe but not real-time safe.

## Reference-Code-First Record

| Project | Tag / Commit | License posture | Files inspected | Reuse mode |
| --- | --- | --- | --- | --- |
| `sdatkinson/NeuralAmpModelerCore` | `v0.5.3` / `9c7b185de346fe0725dea537bcee4bc38b5bb6d6` | MIT expected; verify license file before vendoring | `CMakeLists.txt`, `NAM/version.h`, `NAM/dsp.h`, `NAM/get_dsp.h`, `NAM/get_dsp.cpp`, `NAM/slimmable.h`, `NAM/container.h`, `NAM/model_config.h`, `NAM/wavenet/*`, `docs/nam_file_version.rst` | direct-copy or close-port for vendored core update, with upstream attribution |
| `sdatkinson/NeuralAmpModelerPlugin` | `v0.7.15` / `96337e9ab6e3beb619459779bbb5c47e1b04d8c4`; `v0.7.14` / `feb4f8c4fcf4a98be021de1d06cc816642899e50` | Pattern-only unless license permits copying; verify before any source reuse | Release notes for core `0.5.3`; `NeuralAmpModeler/NeuralAmpModeler.cpp`; `NeuralAmpModeler/NeuralAmpModeler.h`; `NeuralAmpModeler/Unserialization.cpp`; slimmable model support, default `Slim` parameter, A2 support path | pattern-only |
| TONE3000 API docs | 2026-06-19 page read | Documentation | `/tones/search`, `/models`, `Architecture`, `Model` schema | behavioral contract |

Before any source copy or close-port, add the upstream license file and exact source-path notes to `THIRD_PARTY_LICENSES.md` or a dedicated vendored notice.

## Acceptance Criteria

- Existing A1/custom local `.nam` files still load after the migration.
- A2 TONE3000 search and model download requests include `architecture=2`.
- The browser exposes architecture/version enough that users can tell whether they are downloading A1, A2, or custom.
- `NAMCore` validates channel counts and only accepts a configuration the current processor can process.
- New core reset/prewarm/model load work remains outside `NAMProcessor::processBlock`.
- `SetSlimmableSize()` is never called from the audio callback.
- Tests cover API query construction, architecture parsing, legacy fallback, and RT boundaries.
- The first runtime commit that updates NeuralAmpModelerCore includes at least one A1 fixture and one A2 fixture or a documented reason why fixture coverage is deferred.

## Task 1: Add TONE3000 Architecture Selection Tests

Files:

- `tests/nam_processor_test.cpp` or a new focused TONE3000 test
- `src/Tone3000Types.h`
- `src/Tone3000Client.h`
- `src/Tone3000Client.cpp`

Steps:

- [x] Add tests or source guards asserting TONE3000 search adds `architecture=2` when requesting NAM models.
- [x] Add tests or source guards asserting model download lookup adds `architecture=2`.
- [x] Add tests for architecture string conversion: legacy default, A1, A2, custom.
- [x] Add tests for parsing `architecture_version` from tone/model JSON where reachable without network.

Verification:

```powershell
cmake --build build --config Debug --target Pedalboard3_Tests -- /m:1
.\build\tests\Debug\Pedalboard3_Tests.exe "[nam]"
```

Commit target: `test: cover tone3000 nam architecture selection`

## Task 2: Implement TONE3000 A2 Query Support

Files:

- `src/Tone3000Types.h`
- `src/Tone3000Client.h`
- `src/Tone3000Client.cpp`
- `src/Tone3000DownloadManager.cpp`
- `src/NAMOnlineBrowser.cpp`

Steps:

- [x] Add a small architecture enum/helper in `Tone3000Types.h`.
- [x] Default NAM search to A2 by passing `architecture=2`.
- [x] Default model download lookup to A2 by passing `architecture=2`.
- [x] Preserve a legacy/default escape hatch for A1/custom so future UI or fallback logic can still request old models.
- [x] Parse `architecture_version` into `ToneInfo`.
- [x] Display an architecture label in the details/list UI without blocking downloads for legacy tones.
- [x] Ensure cache naming does not collide if the same tone ID can download different architectures.

Verification:

```powershell
cmake --build build --config Debug --target Pedalboard3_Tests -- /m:1
.\build\tests\Debug\Pedalboard3_Tests.exe "[nam]"
```

Commit target: `feat: request nam a2 tones from tone3000`

## Task 3: Prepare A2 Core Build Island

Files:

- `CMakeLists.txt`
- `tests/CMakeLists.txt`
- `external/NeuralAmpModelerCoreA2/**`
- `THIRD_PARTY_LICENSES.md`

Steps:

- [x] Stage the `v0.5.3` source layout in a parallel `external/NeuralAmpModelerCoreA2` tree instead of replacing the legacy runtime core in-place.
- [x] Add upstream license/notice text and source-path notes.
- [x] Compile NeuralAmpModelerCore sources as a C++20 island, preferably an object/static library linked by the C++17 app target.
- [x] Keep public Pedalboard3 headers C++17-compatible.
- [x] Avoid raising the entire application language standard until a clean build shows it is necessary.

Scope note: this task intentionally does not wire A2 into `NAMCore` yet. The
existing legacy `external/NeuralAmpModelerCore` include/source path remains the
runtime path until the adapter task has explicit A1/A2 fixture coverage.

Verification:

```powershell
cmake -S . -B build
cmake --build build --config Debug --target Pedalboard3 -- /m:1
cmake --build build --config Debug --target Pedalboard3_Tests -- /m:1
```

Commit target: `build: isolate neural amp modeler core a2 build`

## Task 4: Adapt NAMCore To Current NeuralAmpModelerCore

Files:

- `src/NAMCore.h`
- `src/NAMCore.cpp`
- `src/NAMCoreA2.h`
- `src/NAMCoreA2.cpp`
- `CMakeLists.txt`
- tests added under Task 1 or a new fixture file

Steps:

- [x] Replace old mono `process(input, output, frames)` calls with the new channel pointer API for explicit A2 models.
- [x] Replace `finalize_` usage with the current reset/prewarm path for explicit A2 models.
- [x] Validate `NumInputChannels()` and `NumOutputChannels()` before accepting an A2 model.
- [x] Use `HasLoudness()` and level APIs defensively.
- [x] Keep model creation, reset, prewarm, and failed-load destruction outside `processBlock`.
- [x] Preserve current `NAMCore` outward API where practical so `NAMProcessor` and legacy state stay stable.

Scope note: A2 is now isolated behind `NAMCoreA2` with the upstream namespace
remapped to `pedalboard3_nam_a2` so it can link beside the legacy core. Models
explicitly marked `architecture_version: 2`, A2-only slimmable shapes, and
newer NAM file versions route to A2. Unmarked `0.5.x` files keep the legacy
`ResamplingNAM` path first and fall back to A2 only if legacy parsing fails.
This commit intentionally rejects A2 sample-rate mismatches instead of adding a
new RT resampler wrapper; do not call A2 runtime verification complete until a
real A2 fixture or manual model load confirms the end-to-end path. Loads that
happen before `NAMCore::prepare()` defer sample-rate enforcement until the real
device rate is known.

Verification:

```powershell
cmake --build build --config Debug --target Pedalboard3_Tests -- /m:1
.\build\tests\Debug\Pedalboard3_Tests.exe "[nam]"
.\build\tests\Debug\Pedalboard3_Tests.exe "[rt][nam]"
```

Commit target: `feat: update nam core for a2 runtime`

## Task 5: Add Slimmable Model Control Safely

Files:

- `src/NAMProcessor.h`
- `src/NAMProcessor.cpp`
- `src/NAMControl.cpp`
- `src/NAMCore.h`
- `src/NAMCore.cpp`
- tests

Steps:

- [x] Detect whether the loaded model implements `nam::SlimmableModel`.
- [x] Decide UI semantics after verifying official plugin behavior: quality/CPU, Full/Lite, or raw slim value.
- [x] Apply slimmable-size changes only at a non-audio boundary.
- [x] Serialize the setting in a backward-compatible state version.
- [x] Hide or disable the control for non-slimmable A1/custom models.

Scope note: Official NAM Plugin `v0.7.15` exposes this as a `Slim` parameter
with `0.0..1.0` range and a restored default of `1.0`. Pedalboard now stores
the setting as state v9 and exposes a compact `SIZE` slider only when the
loaded explicit A2 model reports `SlimmableModel`. It is intentionally not
added to the legacy `AudioProcessor` parameter enum in this slice, keeping host
automation from creating a new possible audio-thread path into
`SetSlimmableSize()`.

Verification:

```powershell
.\build\tests\Debug\Pedalboard3_Tests.exe "[nam]"
.\build\tests\Debug\Pedalboard3_Tests.exe "[rt][nam]"
```

Commit target: `feat: add rt-safe nam a2 size control`

## Task 6: Add Fixture And Compatibility Coverage

Files:

- `tests/nam_processor_test.cpp`
- `tests/nam_core_runtime_test.cpp`
- possibly `tests/fixtures/nam/*`
- documentation for fixture provenance

Steps:

- [x] Add a generated minimal legacy Linear `.nam` model at test runtime.
- [x] Add a generated minimal explicit A2 Linear `.nam` model at test runtime.
- [ ] Add exact redistributable real-world A1/A2 fixtures when license and provenance are clear.
- [x] If fixture size or license blocks repo storage, add a documented manual verification command and keep source-structure guards until fixtures are available.
- [x] Verify model load failure is clean for unsupported/malformed files.
- [x] Verify legacy serialized states still restore paths without forcing loads on the audio callback.

Fixture provenance findings:

- Local repo has no committed `.nam` fixture files as of 2026-06-20.
- `sdatkinson/NeuralAmpModelerPlugin` `v0.7.15`
  (`96337e9ab6e3beb619459779bbb5c47e1b04d8c4`, MIT) contains
  `REAPER/model.nam`, size 296,243 bytes. The file metadata reports version
  `0.5.2`, top-level `sample_rate` 48000, name `Obsidian`, and no
  `architecture_version` field.
- That exact upstream `.nam` file now exercises the version-supported unmarked
  compatibility path: legacy parsing is attempted first, and A2 fallback is
  allowed if the legacy core cannot parse the file. Creating an A2 fixture by
  adding `architecture_version: 2` would be a derived opaque model, so it is not
  committed in this slice.
- `Models/*/config.json` plus `weights.npy` samples in the upstream plugin repo
  are not packed `.nam` files and would require a conversion step before they
  represent user-loadable fixtures.
- The broad `Pedalboard3_Tests` target still does not compile `src/NAMCore.cpp`
  or `src/NAMCoreA2.cpp`. Runtime loader coverage now lives in the dedicated
  `Pedalboard3_NAMCoreRuntimeTests` target, which links the real legacy
  `NAMCore.cpp` path and the isolated A2 adapter path.
- Generated runtime tests cover a minimal legacy Linear model, a minimal
  explicit A2 Linear model, A2 sample-rate rejection after deferred prepare, and
  failed unsupported A2 loads preserving the existing model.
  Real-world WaveNet/slimmable fixture coverage still requires exact
  redistributable model files or documented upstream-generated fixtures.

Verification:

```powershell
cmake --build build --config Debug --target Pedalboard3_NAMCoreRuntimeTests -- /m:1
.\build\tests\Debug\Pedalboard3_NAMCoreRuntimeTests.exe
.\build\tests\Debug\Pedalboard3_Tests.exe "[nam]"
ctest --test-dir build -C Debug --output-on-failure
```

Commit target: `test: cover generated nam runtime compatibility`

## Task 7: Manual Product Verification

Steps:

- [ ] Search TONE3000 for A2 NAM tones from the in-app browser.
- [ ] Download an A2 tone and confirm it loads.
- [ ] Load a pre-existing local A1 `.nam` file.
- [ ] Switch patches with a loaded A2 model and confirm no glitch/crash from deferred NAM swap logic.
- [ ] Run a short CPU comparison between A1, A2 full-size, and any slimmable mode exposed.
- [ ] Record any UX differences in the NAM browser/model details.

Verification artifacts:

- Add notes to this plan under "Manual Verification".
- If useful, add a short entry to `LESSONS.md`.

## Manual Verification

Pending product-run verification. For the Task 6 fixture gap, use:

```powershell
cmake --build build --config Debug --target Pedalboard3 -- /m:1
cmake --build build --config Debug --target Pedalboard3_NAMCoreRuntimeTests -- /m:1
.\build\tests\Debug\Pedalboard3_NAMCoreRuntimeTests.exe
.\build\tests\Debug\Pedalboard3_Tests.exe "[nam]"
.\build\tests\Debug\Pedalboard3_Tests.exe "[rt][nam]"
```

Then run the app against real model files:

- Load an exact A2 `.nam` file that either contains `architecture_version: 2` or
  is an unmarked version-supported file that forces A2 fallback, ideally at the
  current device sample rate. Record source URL or immutable download ID,
  license, hash, metadata, and whether the compact `SIZE` slider appears.
- Load a known pre-A2/custom `.nam` file and confirm it still uses the legacy
  path and does not expose the `SIZE` control.
- Attempt a malformed or unsupported `.nam` file and confirm load failure leaves
  the prior model state intact.
- Switch patches while audio is active with loaded A1 and A2 models and confirm
  model swaps remain deferred outside `processBlock`.

## Known Risks

- NeuralAmpModelerCore `v0.5.3` has a larger source layout and C++20 expectations, while Pedalboard3 currently defaults to C++17.
- The local wrapper code was written against the old mono API; blindly replacing files will break builds.
- A2 support can accidentally hide A1/custom models if the browser has no fallback selector.
- TONE3000 cache keys can collide if architecture is ignored for downloads.
- `SetSlimmableSize()` is thread-safe but not RT-safe, so treating it like a normal audio parameter would regress the RT sprint.
- Generated Linear runtime tests can still miss real-world A2 WaveNet/slimmable load failures; do not call the runtime migration complete without an exact real A2 model load path.
