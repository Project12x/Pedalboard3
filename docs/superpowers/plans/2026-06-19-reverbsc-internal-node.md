# ReverbSC Internal Node Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a GPLv3-compatible built-in stereo `ReverbSC` effect node based on the Costello/Varga ReverbSC algorithm.

**Status:** Complete as of 2026-06-19 on branch `codex/rt-hosting-sprint`.

**Architecture:** Implement a small JUCE-free DSP core in `src/dsp`, wrap it with a `PedalboardProcessor` internal node, and register it through the existing `InternalPluginFormat` path. Keep allocation in `prepare`, keep `processBlock` bounded and non-blocking, and preserve legacy internal-processor parameter compatibility.

**Tech Stack:** C++17, JUCE 8, CMake/MSVC, Catch2, Pedalboard3 internal processor host, Csound/Soundpipe ReverbSC close-port reference.

---

## File Structure

- Create `src/dsp/ReverbSC.h`: portable DSP core public API and delay-line state.
- Create `src/dsp/ReverbSC.cpp`: close-port of the ReverbSC algorithm with C++ ownership and no JUCE dependency.
- Create `src/ReverbSCProcessor.h`: `PedalboardProcessor` wrapper and parameter enum.
- Create `src/ReverbSCProcessor.cpp`: JUCE processor wrapper, state serialization, dry/wet/width/output handling.
- Create `tests/reverbsc_processor_test.cpp`: core DSP, state, and internal-format tests.
- Modify `src/InternalFilters.h`: add `reverbScProcFilter` and descriptor member.
- Modify `src/InternalFilters.cpp`: include/register/create/list `ReverbSCProcessor`.
- Modify `CMakeLists.txt`: add new source files to the app target.
- Modify `tests/CMakeLists.txt`: add tests and new source files to `Pedalboard3_Tests`.
- Modify `THIRD_PARTY_LICENSES.md`: add ReverbSC attribution for Csound and Soundpipe references.

## Task 1: Add Failing Core DSP Tests

**Files:**
- Create: `tests/reverbsc_processor_test.cpp`
- Modify: `tests/CMakeLists.txt`

- [x] **Step 1: Write failing core tests**

Add `tests/reverbsc_processor_test.cpp` with tests that include `dsp/ReverbSC.h`, construct `pedalboard3::dsp::ReverbSC`, and assert:

```cpp
TEST_CASE("ReverbSC core keeps silence silent", "[reverbsc][dsp]")
{
    pedalboard3::dsp::ReverbSC reverb;
    reverb.prepare(48000.0, 64);

    std::vector<float> inL(64, 0.0f), inR(64, 0.0f), outL(64, 1.0f), outR(64, 1.0f);
    reverb.process(inL.data(), inR.data(), outL.data(), outR.data(), 64);

    for (int i = 0; i < 64; ++i)
    {
        REQUIRE(outL[static_cast<size_t>(i)] == 0.0f);
        REQUIRE(outR[static_cast<size_t>(i)] == 0.0f);
    }
}
```

Also add tests for impulse tail, finite output at several sample rates, and block sizes `1`, `17`, and `256`.

- [x] **Step 2: Wire the test source**

Add `reverbsc_processor_test.cpp` to `tests/CMakeLists.txt`.

- [x] **Step 3: Run test build and verify RED**

Run: `cmake --build build --config Debug --target Pedalboard3_Tests -- /m:1`

Expected: compile failure because `dsp/ReverbSC.h` does not exist yet.

## Task 2: Implement Portable ReverbSC Core

**Files:**
- Create: `src/dsp/ReverbSC.h`
- Create: `src/dsp/ReverbSC.cpp`
- Modify: `tests/CMakeLists.txt`

- [x] **Step 1: Add DSP core declaration**

Create `src/dsp/ReverbSC.h` with namespace `pedalboard3::dsp`, class `ReverbSC`, methods:

```cpp
void prepare(double sampleRate, int maxBlockSize);
void reset() noexcept;
void setFeedback(float value) noexcept;
void setDampingHz(float value) noexcept;
float getFeedback() const noexcept;
float getDampingHz() const noexcept;
void process(const float* inL, const float* inR, float* outL, float* outR, int numSamples) noexcept;
```

- [x] **Step 2: Add DSP core implementation**

Create `src/dsp/ReverbSC.cpp` as a close-port from:

- Csound `Opcodes/reverbsc.c` at `2932c7fd14681493b5db83df3efdda175c1eb116`
- Soundpipe `modules/revsc.c` at `3efb43bdabd0ed23b17c694292b5a79f1692a3ea`

Use `std::array<DelayLine, 8>`, `std::vector<float>` buffers allocated in `prepare`, fixed delay table values, cubic interpolation, and deterministic random line segments. Clamp feedback to `0.0f..0.99f`, damping to `20.0f..20000.0f`, and invalid samples to `0.0f`.

- [x] **Step 3: Wire core source into tests**

Add `../src/dsp/ReverbSC.cpp` to `tests/CMakeLists.txt`.

- [x] **Step 4: Run core tests and verify GREEN**

Run: `cmake --build build --config Debug --target Pedalboard3_Tests -- /m:1`

Run: `.\build\tests\Debug\Pedalboard3_Tests.exe "[reverbsc][dsp]"`

Expected: all `[reverbsc][dsp]` tests pass.

- [x] **Step 5: Commit**

Commit message: `feat: add reverbsc dsp core`

## Task 3: Add Processor Wrapper Tests

**Files:**
- Modify: `tests/reverbsc_processor_test.cpp`

- [x] **Step 1: Write failing processor tests**

Extend the test file to include `ReverbSCProcessor.h` and assert:

```cpp
TEST_CASE("ReverbSCProcessor state round-trips parameters", "[reverbsc][processor]")
{
    ReverbSCProcessor source;
    source.setParameter(ReverbSCProcessor::MixParam, 0.25f);
    source.setParameter(ReverbSCProcessor::FeedbackParam, 0.75f);
    source.setParameter(ReverbSCProcessor::DampingParam, 0.5f);
    source.setParameter(ReverbSCProcessor::WidthParam, 0.4f);
    source.setParameter(ReverbSCProcessor::OutputParam, 0.8f);

    juce::MemoryBlock state;
    source.getStateInformation(state);

    ReverbSCProcessor restored;
    restored.setStateInformation(state.getData(), static_cast<int>(state.getSize()));

    REQUIRE(restored.getParameter(ReverbSCProcessor::MixParam) == Catch::Approx(0.25f));
    REQUIRE(restored.getParameter(ReverbSCProcessor::FeedbackParam) == Catch::Approx(0.75f));
    REQUIRE(restored.getParameter(ReverbSCProcessor::DampingParam) == Catch::Approx(0.5f));
    REQUIRE(restored.getParameter(ReverbSCProcessor::WidthParam) == Catch::Approx(0.4f));
    REQUIRE(restored.getParameter(ReverbSCProcessor::OutputParam) == Catch::Approx(0.8f));
}
```

Also assert `getName() == "ReverbSC"`, `acceptsMidi() == false`, `producesMidi() == false`, and processing a stereo impulse produces finite output.

- [x] **Step 2: Run test build and verify RED**

Run: `cmake --build build --config Debug --target Pedalboard3_Tests -- /m:1`

Expected: compile failure because `ReverbSCProcessor.h` does not exist yet.

## Task 4: Implement ReverbSCProcessor

**Files:**
- Create: `src/ReverbSCProcessor.h`
- Create: `src/ReverbSCProcessor.cpp`
- Modify: `CMakeLists.txt`
- Modify: `tests/CMakeLists.txt`

- [x] **Step 1: Add processor declaration**

Create `ReverbSCProcessor` deriving from `PedalboardProcessor`, with enum:

```cpp
enum Parameters { MixParam = 0, FeedbackParam, DampingParam, WidthParam, OutputParam, NumParameters };
```

Store normalized atomics for mix, feedback, damping, width, and output. Add helpers for mapping normalized damping to Hz and output to gain.

- [x] **Step 2: Add processor implementation**

Construct with `setPlayConfigDetails(2, 2, 0, 0)`. In `prepareToPlay`, call `reverb.prepare(sampleRate, estimatedSamplesPerBlock)` and pre-size dry/wet buffers. In `processBlock`, copy dry stereo, call the DSP core into preallocated wet buffers, apply width, linear dry/wet mix, and output gain. No allocation in `processBlock`.

- [x] **Step 3: Add state serialization**

Use a versioned XML tag `Pedalboard3ReverbSCSettings` with attributes `mix`, `feedback`, `damping`, `width`, `output`, and editor bounds.

- [x] **Step 4: Wire sources**

Add `src/dsp/ReverbSC.cpp`, `src/dsp/ReverbSC.h`, `src/ReverbSCProcessor.cpp`, and `src/ReverbSCProcessor.h` to `CMakeLists.txt`. Add processor files to `tests/CMakeLists.txt`.

- [x] **Step 5: Run processor tests and verify GREEN**

Run: `cmake --build build --config Debug --target Pedalboard3_Tests -- /m:1`

Run: `.\build\tests\Debug\Pedalboard3_Tests.exe "[reverbsc]"`

Expected: all `[reverbsc]` tests pass.

- [x] **Step 6: Commit**

Commit message: `feat: add reverbsc processor`

## Task 5: Register Internal Node

**Files:**
- Modify: `src/InternalFilters.h`
- Modify: `src/InternalFilters.cpp`
- Modify: `tests/reverbsc_processor_test.cpp`

- [x] **Step 1: Write failing registration test**

Add a test that creates `InternalPluginFormat`, calls `getUserFacingTypes`, and finds a description named `ReverbSC` with category `Effects`. Then call `createInstanceFromDescription` and require the returned instance is non-null and named `ReverbSC`.

Implementation note: the test target does not link `InternalFilters.cpp`, so the landed guard follows the existing source-structure test pattern and verifies the `InternalPluginFormat` include, descriptor, factory case, `getDescriptionFor` case, and `getUserFacingTypes` registration directly in `src/InternalFilters.*`.

- [x] **Step 2: Run registration test and verify RED**

Run: `.\build\tests\Debug\Pedalboard3_Tests.exe "[reverbsc][internal-format]"`

Expected: fail because `ReverbSC` is not registered yet.

- [x] **Step 3: Register descriptor and factory**

Add `reverbScProcFilter` before `endOfFilterTypes`, a `PluginDescription reverbScProcDesc`, constructor descriptor fill, factory case, `getDescriptionFor` case, and `getUserFacingTypes` entry.

- [x] **Step 4: Run registration tests and verify GREEN**

Run: `cmake --build build --config Debug --target Pedalboard3_Tests -- /m:1`

Run: `.\build\tests\Debug\Pedalboard3_Tests.exe "[reverbsc]"`

Expected: all `[reverbsc]` tests pass.

- [x] **Step 5: Commit**

Commit message: `feat: register reverbsc internal node`

## Task 6: Add Attribution And Final Verification

**Files:**
- Modify: `THIRD_PARTY_LICENSES.md`

- [x] **Step 1: Add license entry**

Add `ReverbSC / Soundpipe revsc / Csound reverbsc` to the summary and a section with source URLs, pinned commits, MIT Soundpipe notice, and Csound LGPL-2.1-or-later lineage.

- [x] **Step 2: Run focused and app verification**

Run:

```powershell
cmake --build build --config Debug --target Pedalboard3_Tests -- /m:1
.\build\tests\Debug\Pedalboard3_Tests.exe "[reverbsc]"
cmake --build build --config Debug --target Pedalboard3 -- /m:1
```

Expected: build succeeds and `[reverbsc]` tests pass.

- [x] **Step 3: Commit**

Commit message: `docs: attribute reverbsc sources`

## Plan Self-Review

Spec coverage:

- Internal effect node: Tasks 4 and 5.
- Portable DSP core: Tasks 1 and 2.
- Pinned source/license record: design spec plus Task 6.
- Test-first implementation: each production task has a RED step before code.
- Debug tests/app verification: Task 6.
- Non-goals: no plugin formats, no legacy parameter changes, no RT sprint edits.

Placeholder scan: no TBD/TODO placeholders.

Type consistency: the plan consistently uses `pedalboard3::dsp::ReverbSC`, `ReverbSCProcessor`, and `[reverbsc]` test tags.
