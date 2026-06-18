# RT Hosting Sprint Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to execute this plan.

**Goal:** Remove the currently verified real-time audio and plugin-host stability hazards from Pedalboard3 before broadening plugin-format support. The sprint covers live graph restore, callback bounds, audio-thread logging/allocation/blocking, NAM/IR model handoff, MIDI/control plumbing, bypass correctness, scanner responsiveness, scratch capture safety, and focused regression tests.

**Architecture:** Keep the existing JUCE `AudioProcessorGraph` host architecture, but make every host mutation either prepared off the audio callback before insertion or performed behind an explicit graph mutation boundary. Replace audio-thread side effects with bounded counters, cached atomics, preallocated scratch buffers, and message-thread work queues. Use tests and hostile fixtures to lock down the contracts.

**Tech Stack:** JUCE 8, C++17, CMake/MSVC, Catch2, `AudioProcessorGraph`, `AudioProcessorPlayer`, `FilterGraph`, `PluginField`, `MainPanel`, `BypassableInstance`, `SafetyLimiter`, `IRLoaderProcessor`, `NAMCore`, `NAMProcessor`, `MidiAppFifo`, `MidiMappingManager`, `ScratchRecorder`, `SafePluginScanner`.

---

## Scope Check

This sprint is about realtime and hosting correctness. It intentionally does not implement new plugin formats. Expanded format support is deferred to JUCE 9, as discussed.

Included:

- Fix graph restore and patch load behavior that can race the live audio callback.
- Fix callback buffer/channel bounds and stack scratch assumptions.
- Remove or isolate logging, allocation, settings reads, app command dispatch, named-pipe blocking, and plugin state/model destruction from realtime paths.
- Add tests that exercise the corrected contracts, including hostile plugin behaviors where practical.
- Record ownership of ambiguous MIDI routing semantics before changing behavior.

Excluded:

- Native CLAP/LV2/AUv3 host support before JUCE 9.
- UI redesign, preset browser redesign, or large feature work.
- Copying GPL/AGPL/proprietary host source. Those projects remain behavior references only.

## Evidence And Finding Coverage

The 16 audit findings plus earlier internal RT findings map to these work items.

| ID | Finding | Disposition | Plan Task |
| --- | --- | --- | --- |
| F01 | Live graph restore can mutate `FilterGraph` while audio processing is active. | Accepted, highest priority. | Task 2 |
| F02 | `MainPanel` callback ramps use fixed stack arrays and channel pointer arrays without full runtime bounds. | Accepted. | Task 3 |
| F03 | `VirtualMidiInputProcessor::processBlock` logs from the audio path. | Accepted. | Task 4 |
| F04 | `NAMCore` staged model handoff uses unsynchronized `unique_ptr` state and can destroy old model work on the callback. | Accepted. | Task 7 |
| F05 | `NAMProcessor::setStateInformation` can perform heavy model/IR loads during state restore. | Accepted. | Task 7 |
| F06 | `IRLoaderProcessor::processBlock` can call `updateFilters()` and allocate filter coefficients. | Accepted. | Task 6 |
| F07 | `ScratchRecorder` sink lifetime is mostly guarded but lacks stress tests for stop/restart and writer failure. | Partial, test and harden. | Task 10 |
| F08 | `MidiAppFifo` uses spin locks and has no visible overflow counters. | Accepted. | Task 5 |
| F09 | `MidiMappingManager` performs app command/setting work on the MIDI callback path. | Accepted. | Task 5 |
| F10 | `SafetyLimiter` DC blocker math and shared gain behavior are suspect. | Accepted. | Task 6 |
| F11 | Safe plugin scanner can block UI while waiting on named-pipe reads. | Accepted as host stability, not audio RT. | Task 9 |
| F12 | `BypassableInstance` bypass crossfade does not cover synth/no-input temp-buffer path. | Accepted. | Task 8 |
| F13 | `BypassableInstance` MIDI `swapWith` may discard nonmatching messages. | Needs semantic decision before mutation. | Task 8 |
| F14 | Parameter access lacks bounds checks and still uses deprecated `AudioProcessor::setParameter`. | Accepted. | Task 5 |
| F15 | `VirtualMidiInputProcessor` static raw `instance` pointer is a race-prone lifetime signal. | Partial, harden. | Task 4 |
| F16 | Missing regression tests for graph restore, scanner IPC, hostile plugins, bounds, NAM handoff. | Accepted. | Task 1 and Task 11 |

Earlier internal RT findings are covered by the same task set:

- Process-block logging and UI work: Tasks 4 and 5.
- Allocation during parameter/filter changes: Task 6.
- Hostile plugin isolation and bypass correctness: Tasks 8 and 11.
- Metering and device callback bounds: Task 3.
- Plugin scanning responsiveness: Task 9.
- Durable documentation: this plan plus `LESSONS.md`.

## Reference-Code-First Record

References already inspected for host architecture lessons:

| Project | License posture | Reuse mode | Lesson used |
| --- | --- | --- | --- |
| Element | GPL family | behavior-only | Separate graph edits from live processing, treat plugin scans as fault domains, avoid UI-thread scanner stalls. |
| Carla | GPL family | behavior-only | Strict isolation around plugin loading/scanning; avoid trusting third-party plugin behavior on critical paths. |
| Ardour | GPL family | behavior-only | Stage session/route mutations away from realtime callbacks; prefer explicit activation boundaries. |
| Spotify Pedalboard | Apache-2.0 | pattern-only | Keep DSP graph operations deterministic and bounded from the caller perspective. |
| DISTRHO/Ildaeil | permissive components mixed with plugin SDK constraints | pattern-only | Do not broaden formats until host abstractions and tests are stable. |
| `juce_clap_hosting` | permissive | pattern-only | CLAP can wait for JUCE 9; the immediate risk is host lifecycle and realtime safety. |

No upstream source is copied by this plan. During implementation, every nontrivial direct adaptation from permissive source must record repo, commit SHA, license, inspected files, and reuse mode in the commit notes or implementation summary.

## Current Status: 2026-06-17

Branch: `codex/rt-hosting-sprint`

The first implementation pass is committed locally as four focused checkpoints:

| Commit | Scope | Notes |
| --- | --- | --- |
| `84ad534` `fix: harden core rt hosting paths` | Core RT host/audio paths | Locks graph clear/restore mutations, hardens callback bounds, removes Virtual MIDI audio-thread logging, removes IR filter allocation, fixes limiter DC/invalid-sample behavior, and applies bypass crossfade to temp-buffer/synth paths. |
| `bba6eb4` `fix: defer nam model and ir swaps` | NAM lifecycle | Removes `NAMCore` staged-model handoff from `process`, defers model/IR load/clear while prepared, and applies deferred heavy state changes before the processor becomes active again. |
| `da4f9da` `fix: move plugin scanning off ui timer` | Scanner responsiveness | Moves `SafePluginListComponent` scan progression to a worker thread, keeps the timer as a UI progress pump, replaces fake named-pipe timeouts with bounded exact reads, caps scanner payloads, and removes blocking pipe flushes. |
| `0dba1a2` `docs: record rt hosting lessons and tests` | Durable docs and regression guards | Adds `LESSONS.md`, peer audit artifacts, this sprint plan, and `tests/rt_hosting_sprint_test.cpp` wired into the existing `Pedalboard3_Tests` target. |

Verification already run on this branch:

```powershell
cmake --build build --config Debug --target Pedalboard3_Tests
cmake --build build --config Debug --target Pedalboard3
.\build\tests\Debug\Pedalboard3_Tests.exe "[rt]"
.\build\tests\Debug\Pedalboard3_Tests.exe "[scanner]"
.\build\tests\Debug\Pedalboard3_Tests.exe "[nam]"
```

Latest focused results:

- `[rt]`: 3,158 assertions in 12 test cases.
- `[midi]`: 3,871 assertions in 23 test cases.
- `[midi][fifo]`: 3,323 assertions in 6 test cases.
- `[rt][bypassable][midi]`: 37 assertions in 3 test cases.
- `[scanner]`: 17 assertions in 2 test cases.
- `[nam]`: 259 assertions in 34 test cases.

Earlier in the same sprint pass, these targeted filters also passed before committing:

- `[bypassable]`: 20 assertions in 3 test cases.
- `[audio]`: 2,049,353 assertions in 8 test cases.
- `[patchswitch]`: 572 assertions in 9 test cases.
- `[protection]`: 186 assertions in 12 test cases.

Known remaining sprint work:

- Move MIDI mapping command/settings work fully out of callback-sensitive paths.
- Surface `MidiAppFifo` diagnostics in UI/dev telemetry if useful; the RT-safe FIFO core now reports drops and max depth.
- Broaden host MIDI routing coverage if future graph nodes need the same broadcast-preservation contract outside `BypassableInstance`.
- Add ScratchRecorder stop/restart/writer-failure stress coverage.
- Add stronger hostile-plugin/end-to-end graph restore and scanner fake-server tests.
- Run Release build verification; only Debug app/test builds have been verified in this pass.

## Acceptance Criteria

- Loading/restoring a patch cannot clear or rebuild the live graph while the audio callback is using it.
- Plugin state/model/IR restoration is prepared outside realtime processing and committed through a bounded mutation boundary.
- No known `processBlock` path performs `spdlog`, settings lookups, UI command dispatch, scanner IPC waits, dynamic coefficient creation, or unbounded locking.
- The device callback clamps or safely rejects oversized `numSamples`, channel counts beyond `MaxChannels`, and null channel pointer cases.
- `SafetyLimiter` handles NaN/Inf, DC offset, channel gain behavior, and mute reset deterministically.
- NAM model installation does not use a plain unsynchronized `unique_ptr` handoff, and old model destruction is not performed by the audio callback during normal operation.
- Bypass behavior is correct for effects, synths, and no-input plugins, including crossfade where audio appears from a temp buffer.
- MIDI FIFO overflow is observable through counters, and MIDI mapping uses cached realtime-safe state.
- Scanner work cannot freeze the message thread during child-process IPC waits.
- Tests cover the fixed contracts and run through CTest in Debug or Release.

## Task 1: Add Consolidated RT Hosting Regression Tests

Create a focused test surface before making behavioral changes.

Files:

- `tests/rt_hosting_sprint_test.cpp`
- `tests/graph_restore_rt_test.cpp`
- `tests/hostile_plugin_fixtures.h`
- `tests/CMakeLists.txt`

Steps:

1. Add a new `Pedalboard3_RT_Hosting_Tests` executable in `tests/CMakeLists.txt`.
2. Link it with the same internal targets used by the existing test binaries.
3. Add deterministic fixtures:
   - `CountingLoggerProcessor`: fails the test when audio-thread logging hooks are called.
   - `AllocatingOnStateProcessor`: exposes whether `setStateInformation` occurs while the graph is live.
   - `SynthBypassFixture`: no input buses, produces output, supports bypass crossfade checks.
   - `SlowDestroyModel`: records the thread that destroys the previous NAM-like model.
4. Add initial failing tests for F01, F02, F03, F06, F10, F12, F14, and F16.

Initial test cases:

```cpp
TEST_CASE("MainPanel callback rejects unsafe buffer dimensions")
{
    MainPanel panel;
    REQUIRE(panel.prepareCallbackScratchForTest(8192, 2));
    REQUIRE_FALSE(panel.prepareCallbackScratchForTest(8193, 2));
    REQUIRE_FALSE(panel.prepareCallbackScratchForTest(512, MainPanel::MaxChannels + 1));
}

TEST_CASE("Virtual MIDI processBlock records drops without logging")
{
    VirtualMidiInputProcessor proc;
    juce::AudioBuffer<float> audio(2, 64);
    juce::MidiBuffer midi;

    proc.setRealtimeDiagnosticsForTest(true);
    proc.processBlock(audio, midi);

    REQUIRE(proc.getProcessLogAttemptCountForTest() == 0);
}

TEST_CASE("SafetyLimiter mutes invalid samples and resets cleanly")
{
    SafetyLimiter limiter;
    limiter.prepare(48000.0, 64, 2);

    juce::AudioBuffer<float> block(2, 64);
    block.clear();
    block.setSample(0, 12, std::numeric_limits<float>::quiet_NaN());

    limiter.processBlock(block);
    REQUIRE(limiter.isMutedForTest());

    limiter.resetMuteForTest();
    REQUIRE_FALSE(limiter.isMutedForTest());
}
```

Verification command:

```powershell
cmake --build build --config Debug --target Pedalboard3_RT_Hosting_Tests
ctest --test-dir build -C Debug --output-on-failure -R "RT_Hosting"
```

## Task 2: Make Patch Restore A Graph Mutation Transaction

F01 is the highest risk finding. Fix it before other broad host work.

Files:

- `src/FilterGraph.h`
- `src/FilterGraph.cpp`
- `src/PluginFieldPersistence.cpp`
- `tests/graph_restore_rt_test.cpp`

Current evidence:

- `PluginFieldPersistence.cpp` calls `signalPath->restoreFromXml(*graphXml, oscManager)` after a fade, without removing the audio callback or using a full restore transaction.
- `FilterGraph::restoreFromXml` clears the graph, rebuilds nodes and connections, and calls `removeIllegalConnections`.
- Normal single-node insertion uses `graph.getCallbackLock()` around `graph.addNode`, while restore does bulk mutation without the same safety boundary.

Implementation:

1. Split restore into two phases:
   - Preparation phase: parse XML, create plugin instances, apply plugin state, collect node metadata, and collect connection specs while the nodes are not in the live graph.
   - Commit phase: acquire the graph callback lock, clear the existing graph, add prepared nodes, add prepared connections, call `removeIllegalConnections`, then release the lock.
2. Emit `changed()` and OSC/UI updates after the commit lock is released.
3. Add `FilterGraph::ScopedMutation` or `FilterGraph::restoreFromXmlTransaction(...)` so patch load sites cannot call the old unsafe sequence by accident.
4. Keep plugin construction and `setStateInformation` out of the locked commit phase.
5. Add a test that runs a fake audio processing loop while restore is requested repeatedly and asserts no graph clear/add happens outside the mutation boundary.

Commit-shape contract:

```cpp
struct PreparedGraphNode
{
    juce::AudioProcessorGraph::NodeID id;
    std::unique_ptr<juce::AudioProcessor> processor;
    juce::ValueTree properties;
};

struct PreparedGraphRestore
{
    std::vector<PreparedGraphNode> nodes;
    std::vector<juce::AudioProcessorGraph::Connection> connections;
};

PreparedGraphRestore FilterGraph::prepareRestoreFromXml(const juce::XmlElement& xml,
                                                         OscMappingManager& oscManager);

void FilterGraph::commitPreparedRestore(PreparedGraphRestore&& prepared)
{
    {
        const juce::ScopedLock lock(graph.getCallbackLock());
        clearForRestoreWithoutSendingChange();
        for (auto& node : prepared.nodes)
            addPreparedNodeToGraph(std::move(node));
        for (const auto& connection : prepared.connections)
            graph.addConnection(connection);
        removeIllegalConnections();
    }

    changed();
}
```

Test contract:

```cpp
TEST_CASE("Graph restore commits only inside callback lock")
{
    FilterGraph graph;
    auto restore = graph.prepareRestoreFromXml(makeTwoNodePatchXml(), makeOscManagerForTest());

    graph.enableMutationInstrumentationForTest(true);
    graph.commitPreparedRestore(std::move(restore));

    REQUIRE(graph.getUnsafeMutationCountForTest() == 0);
    REQUIRE(graph.getRestoreCommitCountForTest() == 1);
}
```

## Task 3: Harden Device Callback Bounds And Scratch Storage

Files:

- `src/MainPanel.h`
- `src/MainPanel.cpp`
- `tests/rt_hosting_sprint_test.cpp`

Current evidence:

- `smoothedInputRamp[8192]` and `smoothedOutputRamp[8192]` rely on stack arrays.
- One ramp path asserts `numSamples <= 8192`; release builds can still continue.
- `gainedInputPtrs[MaxChannels]` can be filled using runtime channel counts.

Implementation:

1. Replace callback stack ramps with persistent preallocated `HeapBlock<float>` or fixed `std::array<float, kMaxCallbackBlockSize>` members prepared outside the callback.
2. Add one helper that validates `numSamples`, input channel count, output channel count, and null channel pointers before any scratch arrays are written.
3. Clamp processing to `MaxChannels` and silence unsupported extra outputs.
4. Return silence for callback dimensions beyond the supported contract, and increment a realtime-safe diagnostic counter.
5. Add test accessors guarded by `#if PEDALBOARD3_ENABLE_TEST_ACCESSORS`.

Helper shape:

```cpp
struct CallbackBufferPlan
{
    int samples = 0;
    int inputChannels = 0;
    int outputChannels = 0;
    bool valid = false;
};

CallbackBufferPlan MainPanel::makeCallbackBufferPlanForTest(int numSamples,
                                                            int numInputChannels,
                                                            int numOutputChannels) const noexcept;
```

Tests:

- `8192` samples and `MaxChannels` channels passes.
- `8193` samples fails without writing scratch memory.
- `MaxChannels + 1` input channels fails and increments `callbackBoundsRejectCount`.
- Null output channel pointers produce silence, not undefined behavior.

## Task 4: Remove Virtual MIDI Audio-Thread Side Effects

Files:

- `src/VirtualMidiInputProcessor.h`
- `src/VirtualMidiInputProcessor.cpp`
- `tests/rt_hosting_sprint_test.cpp`

Current evidence:

- `processBlock` logs with `spdlog` on both empty and connected paths.
- A static raw `instance` pointer is exposed through `getInstance`/`setInstance`.

Implementation:

1. Remove all `spdlog` calls from `processBlock`.
2. Replace logging with atomics:
   - `droppedMessageCount`
   - `emptyProcessCount`
   - `connectedProcessCount`
   - `lastErrorCode`
3. Drain and report diagnostics from the message thread or explicit debug UI timer.
4. Replace the static raw pointer with `std::atomic<VirtualMidiInputProcessor*>` at minimum, and clear it in the destructor only when it still points at `this`.
5. Prefer eliminating global access entirely where call sites can hold an owned reference.

Static pointer guard:

```cpp
VirtualMidiInputProcessor::~VirtualMidiInputProcessor()
{
    auto* expected = this;
    instance.compare_exchange_strong(expected, nullptr, std::memory_order_acq_rel);
}
```

Tests:

- `processBlock` increments counters and never calls a logging hook.
- Destroying a processor while a newer processor is registered does not clear the newer pointer.

## Task 5: Make MIDI And Parameter Control Paths Realtime-Safe

Files:

- `src/MidiAppFifo.h`
- `src/MidiAppFifo.cpp`
- `src/MidiMappingManager.h`
- `src/MidiMappingManager.cpp`
- `src/MainPanel.cpp`
- `src/BypassableInstance.h`
- `tests/rt_hosting_sprint_test.cpp`

Current evidence:

- `MidiAppFifo` previously used `SpinLock::ScopedLockType` in push/pop paths and dropped overflow silently. This slice replaces it with a bounded atomic ring and exposes drop/max-depth diagnostics.
- `MidiMappingManager::midiCcReceived` calls `appManager->getFirstCommandTarget`, performs dynamic casts, and reads settings through `SettingsManager`.
- Parameter access uses `plugin->getParameters()[parameterIndex]` without bounds.
- `MainPanel.cpp` uses deprecated `processor->setParameter`.

Implementation:

1. Replace `MidiAppFifo` locking with `juce::AbstractFifo` plus fixed arrays, or a local bounded atomic ring buffer.
2. Add atomics for:
   - `droppedEvents`
   - `maxDepth`
   - `lastOverflowTick`
3. Move app command target resolution to the message thread.
4. Cache mapping settings in atomics updated from the settings UI:
   - mapping enabled
   - soft takeover enabled
   - last channel mode
   - pickup threshold
5. Convert audio/MIDI path output to simple command tokens pushed into the FIFO.
6. Replace raw parameter indexing with a checked helper.
7. Replace deprecated `setParameter` calls with `AudioProcessorParameter::setValueNotifyingHost` when host notification is correct, and `setValue` for internal silent updates.

Checked parameter helper:

```cpp
juce::AudioProcessorParameter* BypassableInstance::getParameterChecked(int parameterIndex) const noexcept
{
    if (plugin == nullptr)
        return nullptr;

    const auto& parameters = plugin->getParameters();
    if (parameterIndex < 0 || parameterIndex >= parameters.size())
        return nullptr;

    return parameters[parameterIndex];
}
```

Tests:

- Out-of-range parameter automation is ignored and increments `badParameterIndexCount`.
- MIDI FIFO full condition drops the newest event or oldest event according to the documented policy and increments `droppedEvents`.
- `midiCcReceived` can run with a fake realtime guard that fails on settings reads or command target lookup.

## Task 6: Fix IRLoader And SafetyLimiter Realtime Behavior

Files:

- `src/IRLoaderProcessor.h`
- `src/IRLoaderProcessor.cpp`
- `src/SafetyLimiter.h`
- `src/SafetyLimiter.cpp`
- `tests/rt_hosting_sprint_test.cpp`

Current evidence:

- `IRLoaderProcessor::processBlock` calls `updateFilters()`.
- `updateFilters()` constructs new `juce::dsp::IIR::Coefficients<float>` objects.
- `SafetyLimiter` DC blocker state update is not a standard high-pass recurrence.
- `SafetyLimiter` uses a shared `currentGain` across channels.

Implementation for `IRLoaderProcessor`:

1. Move filter coefficient creation to parameter-change, prepare, or message-thread staging.
2. Store coefficients in preallocated value objects or atomically swapped reference-counted objects prepared off the audio callback.
3. Make `processBlock` consume the current coefficient snapshot only.
4. Add a realtime guard test that fails when coefficient factories are called from `processBlock`.

Implementation for `SafetyLimiter`:

1. Replace the DC blocker with the standard recurrence:

```cpp
y[n] = x[n] - x[n - 1] + R * y[n - 1]
```

2. Store `previousInput[ch]` and `previousOutput[ch]`.
3. Decide gain topology explicitly:
   - linked stereo/global gain for final output protection, or
   - per-channel gain for independent channels.
4. For a pedalboard output safety limiter, use linked gain derived from the maximum absolute sample across active channels, then document that choice in the header.
5. Clear DC and gain state on sample-rate changes, channel-count changes, mute reset, and transport reset.
6. Mute on NaN/Inf before computing limiter gain.

Tests:

- DC input decays below threshold.
- NaN/Inf mutes immediately.
- Linked gain applies the same gain to left and right when one channel clips.
- Reset clears mute and DC history.

## Task 7: Make NAM Model And State Restore Handoff Safe

Files:

- `src/NAMCore.h`
- `src/NAMCore.cpp`
- `src/NAMProcessor.h`
- `src/NAMProcessor.cpp`
- `tests/rt_hosting_sprint_test.cpp`

Current evidence:

- `NAMCore::loadModel` assigns `impl->stagedModel = std::move(resamplingModel)` from a non-audio thread.
- `NAMCore::processBlock` moves `stagedModel` into `model` from the audio thread.
- Old model destruction can happen inline during audio processing.
- `NAMProcessor::setStateInformation` loads model and IR data during state restore.

Implementation:

1. Remove the unsynchronized `stagedModel` shared `unique_ptr`.
2. Add an explicit model install API that is called from the message thread or graph restore preparation phase:

```cpp
struct PreparedNAMModel
{
    std::unique_ptr<ResamplingNAM> model;
    int targetSampleRate = 0;
    int maxBlockSize = 0;
};

PreparedNAMModel NAMCore::prepareModelForInstall(const juce::File& modelFile,
                                                  double sampleRate,
                                                  int maxBlockSize);

void NAMCore::installPreparedModel(PreparedNAMModel&& prepared);
```

3. Ensure `installPreparedModel` runs while the owning processor is not concurrently processing:
   - during graph restore preparation before graph insertion, or
   - under an explicit processor suspend boundary for user-triggered model loads.
4. Move destruction of the old model to the same non-audio thread that performs install.
5. In `NAMProcessor::setStateInformation`, parse state and record desired model/IR paths first. Perform actual model and IR loading through the same prepared install path used by user-initiated loads.
6. Add a monotonic `modelGeneration` atomic for diagnostics only; do not use it as a synchronization substitute.

Tests:

- A model swap does not destroy the old model on the audio thread.
- `setStateInformation` records pending model/IR state without calling the heavy load path while the processor is marked active.
- Repeated swaps leave the processor using either the previous model or the new model, never a partially moved object.

## Task 8: Fix BypassableInstance Host Contracts

Files:

- `src/BypassableInstance.h`
- `src/BypassableInstance.cpp`
- `tests/rt_hosting_sprint_test.cpp`
- `docs/host-midi-routing-contract.md`

Current evidence:

- Synth/no-input plugins use a temp buffer path.
- Bypass crossfade only runs when `needTempForPlugin` is false.
- MIDI messages filtered into `tempMidi` are swapped back, which can discard nonmatching messages.
- Parameter indexing lacks bounds checks.

Implementation:

1. Add `docs/host-midi-routing-contract.md` with the exact intended semantics:
   - channel-filtered messages consumed by a node
   - nonmatching messages forwarded unchanged
   - all-note-off and panic messages delivered to every plugin that can receive MIDI
2. Adjust MIDI filtering to preserve nonmatching messages in the outgoing buffer unless the documented contract says the node consumes all input.
3. Apply bypass ramp/crossfade after plugin rendering for both direct and temp-buffer paths.
4. For synth/no-input plugins:
   - when bypassed, fade rendered output to silence
   - when unbypassed, fade silence to rendered output
5. Reuse the checked parameter helper from Task 5.

Test matrix:

| Case | Expected |
| --- | --- |
| Audio effect, bypass off to on | Output crossfades to dry signal. |
| Audio effect, bypass on to off | Output crossfades from dry to wet signal. |
| Synth plugin, bypass off to on | Rendered synth output fades to silence. |
| Synth plugin, bypass on to off | Rendered synth output fades in from silence. |
| MIDI channel mismatch | Message is forwarded unchanged. |
| Bad parameter index | Ignored, counted, no crash. |

## Task 9: Move Scanner IPC Waits Off The Message Thread

Files:

- `src/SafePluginScanner.h`
- `src/SafePluginScanner.cpp`
- `src/PluginScannerClient.h`
- `src/PluginScannerClient.cpp`
- `tests/scanner_ipc_test.cpp`

Current evidence:

- `SafePluginScanner` calls scanner work from a timer callback.
- `PluginScannerClient` uses blocking `ReadFile` with named-pipe timeouts.

Implementation:

1. Keep the timer callback as a UI progress pump only.
2. Move scanner child-process IPC to a dedicated worker thread.
3. Use bounded per-plugin timeout state owned by the worker.
4. Deliver progress to the UI through a lock-free queue, `AsyncUpdater`, or `MessageManager::callAsync`.
5. Ensure cancellation terminates the child scanner process and joins the worker outside the timer callback.

Tests:

- A fake scanner server that never writes does not block a synthetic message-thread heartbeat.
- Timeout produces one failed plugin result and scanner continues with the next candidate.
- Cancel during blocked read exits cleanly.

## Task 10: Stress ScratchRecorder Stop, Restart, And Writer Failure

Files:

- `src/ScratchRecorder.h`
- `src/ScratchRecorder.cpp`
- `tests/scratch_recorder_test.cpp`

Current evidence:

- The writer callback uses a raw sink pointer guarded by `activeAudioWrites`.
- `finishStop` sets `stopRequested`, waits for active writes to drain, then resets sinks.
- The audit did not prove a direct use-after-free, but the path needs stress coverage.

Implementation:

1. Add counters:
   - `droppedBlocksAfterStop`
   - `writerFailureCount`
   - `maxActiveAudioWrites`
2. Reject writes immediately after `stopRequested` is set.
3. Keep the existing active-write drain, but add a bounded wait result that records failure instead of silently continuing.
4. Add tests for:
   - rapid start/stop/start
   - stop while callback is inside writer
   - writer open failure
   - writer write failure

Test contract:

```cpp
TEST_CASE("ScratchRecorder does not use old sink after stop and restart")
{
    ScratchRecorder recorder;
    recorder.startForTest(makeMemorySinkFactory("first"));
    recorder.writeTestBlock();
    recorder.stopForTest();

    recorder.startForTest(makeMemorySinkFactory("second"));
    recorder.writeTestBlock();

    REQUIRE(recorder.getWriterFailureCountForTest() == 0);
    REQUIRE(recorder.getWritesToRetiredSinkCountForTest() == 0);
}
```

## Task 11: Hostile Plugin And End-To-End RT Verification

Files:

- `tests/hostile_plugin_fixtures.h`
- `tests/rt_hosting_sprint_test.cpp`
- `tests/graph_restore_rt_test.cpp`
- `tests/scanner_ipc_test.cpp`

Implementation:

1. Add hostile fixtures for:
   - plugin that changes latency during process
   - plugin that throws during state restore
   - plugin that allocates during process, used only to verify host containment and bypass behavior
   - plugin that reports zero input buses but outputs audio
   - plugin that reports parameter count changes between calls
2. Add end-to-end test for:
   - load patch
   - process audio
   - restore a different patch
   - process audio again
   - bypass/unbypass a synth node
   - automate a bad parameter index
   - scan a hanging fake plugin
3. Capture diagnostics at the end of each test and assert expected counters only.

Verification commands:

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Debug --target Pedalboard3_RT_Hosting_Tests
ctest --test-dir build -C Debug --output-on-failure -R "RT_Hosting|GraphRestore|Scanner|Scratch"
cmake --build build --config Release --target Pedalboard3
```

## Execution Order

1. Task 1: Add the test target and failing tests.
2. Task 2: Fix graph restore transaction behavior.
3. Task 3: Fix callback bounds and scratch storage.
4. Task 4: Remove Virtual MIDI audio-thread side effects.
5. Task 5: Fix MIDI/control/parameter realtime contracts.
6. Task 6: Fix IRLoader and SafetyLimiter.
7. Task 7: Fix NAM model and state restore handoff.
8. Task 8: Fix bypass and MIDI routing semantics.
9. Task 9: Move scanner IPC waits off the UI thread.
10. Task 10: Stress and harden ScratchRecorder.
11. Task 11: Add hostile plugin and end-to-end verification.

## Worker Split

Recommended parallel split after Task 1 lands:

- Worker A: Task 2 graph restore transaction.
- Worker B: Tasks 3, 4, and 6 audio callback cleanup.
- Worker C: Tasks 5 and 8 MIDI/control/bypass contracts.
- Worker D: Tasks 7, 9, and 10 model/scanner/recorder lifecycle.

Do not run Worker A and Worker C against the same `BypassableInstance` or graph mutation files at the same time. Merge Task 2 before broad end-to-end tests, because it changes the graph restore contract that later tests depend on.

## Completion Checklist

- [x] `docs/host-midi-routing-contract.md` exists and matches implemented MIDI routing behavior.
- [ ] `tests/rt_hosting_sprint_test.cpp` covers all accepted audit findings.
- [ ] Graph restore uses preparation plus bounded commit.
- [ ] Callback bounds reject counters are visible in tests.
- [x] Virtual MIDI has no audio-thread logging.
- [x] MIDI FIFO overflow is observable through counters and no producer-side `SpinLock`.
- [ ] MIDI mapping uses cached realtime-safe state and deferred app commands.
- [x] IR filter updates are not allocated in `processBlock`.
- [x] Safety limiter DC blocker and gain behavior are documented and tested.
- [x] NAM model handoff avoids unsynchronized `unique_ptr` sharing and audio-thread destruction.
- [x] Bypass crossfade covers synth/no-input paths.
- [x] Scanner IPC cannot block the message-thread progress timer.
- [ ] Scratch recorder stop/restart/failure tests pass.
- [ ] Debug and Release build commands pass.
