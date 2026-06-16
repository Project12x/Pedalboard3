# Antigravity CLI RT/Plugin Stability Audit - 2026-06-16

Provenance: Antigravity CLI was run against a fresh temporary clone created from the public GitHub repository `https://github.com/Project12x/Pedalboard3`, not against the local working tree.

CLI note: `agy -p ... --sandbox --print-timeout 10m` returned exit code 0 but emitted no stdout. The audit response was recovered from the Antigravity CLI conversation store at conversation `ca190824-1423-4650-864f-8a9eff039ece`.

## Critical Findings

1. **Unsynchronized FilterGraph Reconstruction** (`FilterGraph.cpp:896`, `PluginFieldPersistence.cpp:138`): Modifies graph connections/nodes on the message thread during XML restores without locking `graph.getCallbackLock()` or suspending audio. This causes data races, heap corruption, and crashes when the audio thread executes `processBlock`.

2. **Unsynchronized & Non-RT-Safe NAM Model Swaps** (`NAMCore.cpp:59-82`, `NAMCore.cpp:124-143`): UI thread stages new NAM models into `impl->stagedModel` without atomic synchronization. The audio thread moves this to `impl->model` via `std::move` and deallocates the old model inline, causing data races and blocking heap deletions.

3. **Stack Buffer Overflow Risk on Large Buffers** (`MainPanel.h:95-129`, `MainPanel.h:158-175`): Pre-allocated stack arrays `smoothedInputRamp` and `smoothedOutputRamp` are hardcoded to `8192` samples. If a user sets the device block size higher, such as 16384, this triggers stack overflows and host crashes in release builds.

## Important Findings

4. **Inoperable DC Blocker Math** (`SafetyLimiter.cpp:94-98`): DC blocker stores input sample `dcBlockerState[ch] = inputSample` instead of output `dcBlockedSample`, converting an IIR high-pass to a flat FIR filter. No DC offset protection is actually applied.

5. **Non-RT-Safe Coefficient Allocations** (`IRLoaderProcessor.cpp:180-196`): Reallocates `juce::dsp::IIR::Coefficients` on the heap inside `processBlock` when `lowCut`/`highCut` changes, causing audio glitches/stalls during UI parameter changes.

6. **Bypass Ignored on Synths** (`BypassableInstance.cpp:224-239`): If a plugin has fewer inputs than outputs, like synths, the temp buffer path runs without executing the bypass crossfade logic, preventing bypassing.

7. **Blocking Named Pipe Reads on Message Thread** (`PluginScannerClient.cpp:291-305`): `SafePluginScanner::scanNextFile` runs on the Message Thread; if a plugin hangs during out-of-process scanning, the pipe read freezes the UI for up to 30 seconds.

## Minor Findings

8. **Ultrasonic Detector Block Boundary Skip** (`SafetyLimiter.cpp:109-114`): Ultrasonic detector only checks sample-to-sample difference within blocks (`sample > 0`), ignoring the boundary transition between consecutive blocks.

9. **MIDI Message Discarding** (`BypassableInstance.cpp:211-222`): Swaps `tempMidi` back into the graph's `midiMessages` via `swapWith`, losing MIDI data on channels filtered out by the wrapper.

10. **VirtualMidiInputProcessor Static Pointer Race** (`VirtualMidiInputProcessor.cpp:40`, `MainPanel.cpp:1111`): The raw static `instance` pointer is set/cleared in `prepareToPlay`/destructors and read by UI thread callbacks without atomic/lock protection.

## What Product Does Well

- **Out-of-Process Plugin Isolation**: Successfully sandboxes Windows VST3 scans to isolate the host app from plugin crashes.
- **Glitch-Free Patch Switching**: Employs a dedicated crossfade processor to mute and unmute audio cleanly during patch transitions.
- **Cached Channel Configuration**: Snapshotting channel layouts at instantiation prevents thread-unsafe plugin queries on the audio path.
- **Equal-Power Cabinets Blending**: Correctly uses trigonometric equal-power crossfading for dual impulse responses.

## Top 5 Next Actions

1. **Synchronize Graph Modification**: Wrap `restoreFromXml` and graph clears inside `const juce::ScopedLock sl (graph.getCallbackLock());` to prevent audio thread races.
2. **Lock-Free Model Hot-Swapping**: Use an atomic pointer or double buffering to stage and release NAM models on the audio thread safely.
3. **Correct DC Blocker State Update**: Update `dcBlockerState[ch]` with the output `dcBlockedSample` to restore high-pass filtering.
4. **Replace Dynamic Filter Allocations**: Calculate IIR coefficients inline, as done in `NAMProcessor::updateIRFilters`, or use a lock-free queue.
5. **Asynchronous Plugin Scanning**: Move plugin scans to a background thread to prevent UI freezing on named-pipe timeouts.
