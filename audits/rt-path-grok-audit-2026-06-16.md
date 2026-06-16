# Grok Build CLI RT/Plugin Stability Audit - 2026-06-16

Provenance: Grok was run against a fresh temporary clone created from the public GitHub repository `https://github.com/Project12x/Pedalboard3`, not against the local working tree.

Command shape: `grok --cwd "$env:TEMP\pb3audit\Pedalboard3" --single "<audit prompt>" --sandbox read-only --output-format plain --max-turns 20 --no-memory --no-subagents --no-plan --tools Read,Grep,Glob`

## Critical Findings

1. **`VirtualMidiInputProcessor::processBlock`** (`src/VirtualMidiInputProcessor.cpp` ~51-70): `spdlog::info` on the audio thread (periodic + per MIDI batch) - logging/I/O and non-RT work in the realtime path; remove or gate behind a debug flag off in release.

2. **Patch restore vs live graph** (`src/PluginFieldPersistence.cpp` ~92-140, `src/FilterGraph.cpp` ~575-581, ~896-933): Crossfade waits ~150 ms then `restoreFromXml` -> `clear()` -> `graph.clear()` / `addNode` / `setStateInformation` with no `removeAudioCallback` / `suspendProcessing`. Concurrent `AudioProcessorPlayer` callbacks can race graph teardown - classic host crash/glitches under load.

3. **`NAMProcessor::setStateInformation`** (`src/NAMProcessor.cpp` ~686-760): Patch restore calls `loadModel` / `loadIR` / nested FX `setStateInformation` synchronously on the message thread during graph rebuild - heavy disk + allocation; long patch switches and risk if any inner work touches audio-owned state.

4. **`ScratchRecorder` sink lifetime** (`src/ScratchRecorder.cpp` ~236-251, ~336-362): Audio thread uses `rawSink.get()` without holding `stateLock`; `finishStop` resets sinks after `activeAudioWrites` drain - mostly safe, but a stop/restart race can still hit a null sink or `ThreadedWriter` teardown edge case under stress.

## Important Findings

5. **`MeteringProcessorPlayer`** (`src/MainPanel.h` ~95-98, ~157-160): Stack ramps `float[8192]`; only `jassert(numSamples <= 8192)`. In release, ASIO buffers greater than 8192 can corrupt stack if asserts are off.

6. **`MidiAppFifo::writeParamChange`** (`src/MidiAppFifo.cpp` ~150-162): When FIFO is full, writes are silently dropped (no `size1`/`size2`) - MIDI/OSC automation can stop updating under burst CC.

7. **`MidiMappingManager::midiCcReceived`** (`src/MidiMappingManager.cpp` ~260-281): App mappings (`value > 64`) run `dynamic_cast<MainPanel*>` + `invokeCommandFromOtherThread` from the MIDI/audio path via `MidiInterceptor` - more work on RT thread than the deferred param path.

8. **`SafetyLimiterProcessor::processBlock`** (`src/SafetyLimiter.cpp` ~82-140): Single `currentGain` shared across channels per sample - stereo limiter coupling; one hot channel can over-attenuate the other.

9. **`MainPanel::timerCallback` param drain** (`src/MainPanel.cpp` ~2370-2373): Uses deprecated `processor->setParameter()` - weak for VST3/modern parameters vs `AudioProcessorParameter::setValueNotifyingHost`.

10. **`BypassableInstance`** (`src/BypassableInstance.h` ~189-193): `isPluginParameterAutomatable` indexes `getParameters()[parameterIndex]` without bounds check - OOB if UI/state passes bad index.

11. **`PluginScannerClient`** (`src/PluginScannerClient.cpp` ~147-149): `SetCommTimeouts` on a named pipe - serial-port API; read timeouts may not behave as intended on Windows pipes.

## Minor Findings

12. **`BypassableInstance::processBlock`** (`src/BypassableInstance.cpp` ~219): Compares `std::atomic<int> midiChannel` to channel without explicit `.load()` - style/portability nit.

13. **`NAMProcessor::processBlock`** (`src/NAMProcessor.cpp` ~232+): No top-level `isPrepared` guard, unlike `updateIRFilters` - early callbacks before `prepareToPlay` are unsafe.

14. **`MidiMapping` XML** (`src/MidiMappingManager.cpp` ~51, ~121): Attribute `"channe"` typo - channel mapping may not round-trip on old saves.

15. **Tests** (`tests/filtergraph_test.cpp`, `tests/integration_test.cpp`, `tests/audio_component_test.cpp`): Mostly mocks/logic - limited coverage of live graph restore, OOP scanner IPC, and full `MeteringProcessorPlayer` RT path. `tests/vst3_loading_test.cpp` is the main real-graph test.

## What Product Does Well

- Deferred MIDI-to-parameter path: `Mapping::updateParameter` -> `MidiAppFifo` -> `MainPanel` timer drain (`src/Mapping.cpp` ~48-52, `src/MainPanel.cpp` ~2351-2377).
- `MidiMappingManager`: `ScopedTryLock` on mappings; logging removed from CC path (`src/MidiMappingManager.cpp` ~223-232).
- `BypassableInstance`: `prepared` gate, cached bus metadata, bypass ramp, temp-buffer path for channel mismatch (`src/BypassableInstance.cpp` ~55-93, ~185-288).
- Patch UX: Crossfade out/in around restore (`src/PluginFieldPersistence.cpp` ~92-150); infrastructure nodes excluded from XML and rebuilt after `clear`.
- `ScratchRecorder`: Atomics + `activeAudioWrites` barrier before close; threaded WAV sink (`src/ScratchRecorder.cpp` ~426-441).
- `SafePluginScanner`: Blacklist, OOP scanner fallback, in-process timeout via `CrashProtection` (`src/SafePluginScanner.cpp` ~82-153).
- `MeteringProcessorPlayer`: Pre-allocated buffers, atomic scratch pointer, master-bus insert guard (`src/MainPanel.h` ~47-186).

## Top 5 Next Actions

1. Suspend audio (remove callback or `graph.suspendProcessing`) for the full `restoreFromXml` / `graph.clear()` window; keep crossfade for audibility only after graph is consistent.
2. Strip all `spdlog`/I/O from `VirtualMidiInputProcessor::processBlock` and audit other internal processors for the same.
3. Defer NAM/IR heavy loads off the synchronous `setStateInformation` restore path: background job + atomic swap when prepared.
4. Harden device callback: cap `numSamples`, use member ramps instead of stack ramps in `MeteringProcessorPlayer`, and add FIFO overflow policy for `MidiAppFifo` (drop-oldest or coalesce CC).
5. Extend automated tests: headless patch switch with `AudioProcessorPlayer` + `restoreFromXml`, scratch stop during record, and scanner client timeout/blacklist behavior on Windows.
