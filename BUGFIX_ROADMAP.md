# Bugfix & Gap-Closing Roadmap

Separate from ROADMAP.md. Tracks stability, correctness, and thread-safety fixes
identified during static review pass (2026-02-11).

---

## Phase 1 - Critical: Audio Thread Safety [DONE]

### 1.1 Remove UI calls from audio thread (Looper) [DONE]
- **Files changed:** `LooperProcessor.cpp`, `PedalboardProcessors.h`, `LooperControl.cpp`
- **Fix:** Replaced `AlertWindow::showMessageBoxAsync` in `processBlock` with
  `std::atomic<bool> memoryError` flag. Error is now shown from LooperControl's
  `changeListenerCallback` on the message thread.

### 1.2 Make recorder/looper state flags atomic [DONE]
- **Files changed:** `PedalboardProcessors.h`, `RecorderProcessor.cpp`
- **Fix:** Changed `playing`, `stopPlaying`, `recording`, `stopRecording`,
  `syncToMainTransport`, `stopAfterBar`, `autoPlay` to `std::atomic<bool>` in
  LooperProcessor. Changed `recording`, `stopRecording` to `std::atomic<bool>` in
  RecorderProcessor. Updated spdlog calls to use `.load()`.

### 1.3 Fix DSP coefficient race in NAM and IR Loader filters [DONE]
- **Files changed:** `NAMProcessor.h`, `NAMProcessor.cpp`, `IRLoaderProcessor.h`,
  `IRLoaderProcessor.cpp`
- **Fix:** Added `juce::SpinLock` to protect filter coefficient updates. Coefficients
  are computed outside the lock, then applied under lock. `processBlock` acquires the
  same lock when processing through the filters.

---

## Phase 2 - High: Graph & Data Integrity [DONE]

### 2.1 Lock subgraph mutations against audio callback [DONE]
- **Files changed:** `SubGraphProcessor.cpp`, `SubGraphFilterGraph.cpp`
- **Fix:** Added `const ScopedLock sl(graph.getCallbackLock())` to:
  - `SubGraphProcessor::restoreFromRackXml` (entire function)
  - `SubGraphFilterGraph::removeFilterRaw`
  - `SubGraphFilterGraph::addConnectionRaw`
  - `SubGraphFilterGraph::removeConnectionRaw`
  - `SubGraphFilterGraph::disconnectFilter`
- Note: `addFilterRaw` already had the lock.

### 2.2 Fix MIDI channel restore typo [DONE]
- **Files changed:** `FilterGraph.cpp`
- **Fix:** Changed `"MIDIChanne"` to `"MIDIChannel"` in `createNodeFromXml`.

---

## Phase 3 - Medium: Memory Safety & UI Polish [DONE]

### 3.1 Fix dangling pointer in subgraph connection lookup [DONE]
- **Files changed:** `SubGraphFilterGraph.cpp`
- **Fix:** Used `static` local variable pattern (matching `FilterGraph::getConnectionBetween`)
  to avoid returning pointer into temporary vector.

### 3.2 Fix About dialog height clipping [DONE]
- **Files changed:** `MainPanel.cpp`
- **Fix:** Changed `dlg.setSize(400, 250)` to `dlg.setSize(400, 340)`.

### 3.3 Fix duplicate source entry in CMakeLists.txt [DONE]
- **Files changed:** `CMakeLists.txt`
- **Fix:** Removed duplicate `StageView.cpp` entry.

---

## Phase 4 - Minor: Polish & Typos [DONE]

### 4.1 Fix parameter name typos in Looper [DONE]
- **Files changed:** `LooperProcessor.cpp`
- **Fix:** Changed `"Input Leve"` to `"Input Level"`, `"Loop Leve"` to `"Loop Level"`.

---

## Phase 5 - Hardening: Spot-Check Fixes [DONE]

### 5.1 Fix getConnectionBetween dangling pointer API [DONE]
- **Files changed:** `IFilterGraph.h`, `FilterGraph.h`, `FilterGraph.cpp`,
  `SubGraphFilterGraph.h`, `SubGraphFilterGraph.cpp`, `PluginField.cpp`
- **Fix:** Changed return type from `const Connection*` (static local hack) to `bool`.
  All callers only performed null-checks. Eliminates fragile static local pattern
  and thread-safety concern.

### 5.2 Narrow callback lock scope in SubGraphProcessor::restoreFromRackXml [DONE]
- **Files changed:** `SubGraphProcessor.cpp`
- **Fix:** Restructured into two phases: Phase 1 creates all plugin instances outside
  the callback lock (slow DLL loading no longer blocks audio thread). Phase 2 acquires
  the lock and performs graph mutations (addNode, addConnection) under protection.

### 5.3 Move IR filter coefficient updates to audio thread [DONE]
- **Files changed:** `NAMProcessor.h`, `NAMProcessor.cpp`, `IRLoaderProcessor.h`,
  `IRLoaderProcessor.cpp`
- **Fix:** Removed `juce::SpinLock` from both processors. Setters now only store to
  atomics. `processBlock` detects parameter changes via last-known-value tracking and
  updates filter coefficients on the audio thread. Same pattern already used for
  bass/mid/treble. Eliminates priority inversion risk.

### 5.4 Guard enableMidiForNode against redundant operations [DONE]

- **Files changed:** `PluginField.cpp`
- **Fix:** `enableMidiForNode` computed `connection` via `getConnectionBetween` but never
  used it as a guard, always calling add/remove regardless of current state. Added
  `&& connection` / `&& !connection` guards so operations only fire when state actually
  needs to change.

---

## Phase 6 - Gap: Unimplemented P0 Features

### 6.1 StarterRig and SoundcheckDialog
- **Reference:** `P0_GIG_SPEED_FEATURES.md:29-32,69-70`
- **Status:** No implementation exists in `src/` or `CMakeLists.txt`.
- **Action:** Track in main ROADMAP.md. These are feature work, not bugfixes.

---

## Summary

All items from Phases 1-5 completed on 2026-02-11. Build verified clean (Release).

| Item | Status   | Files touched                                          |
|------|----------|--------------------------------------------------------|
| 1.1  | DONE     | LooperProcessor.cpp, PedalboardProcessors.h, LooperControl.cpp |
| 1.2  | DONE     | PedalboardProcessors.h, RecorderProcessor.cpp          |
| 1.3  | DONE     | NAMProcessor.h/cpp, IRLoaderProcessor.h/cpp            |
| 2.1  | DONE     | SubGraphProcessor.cpp, SubGraphFilterGraph.cpp         |
| 2.2  | DONE     | FilterGraph.cpp                                        |
| 3.1  | DONE     | SubGraphFilterGraph.cpp                                |
| 3.2  | DONE     | MainPanel.cpp                                          |
| 3.3  | DONE     | CMakeLists.txt                                         |
| 4.1  | DONE     | LooperProcessor.cpp                                    |
| 5.1  | DONE     | IFilterGraph.h, FilterGraph.h/cpp, SubGraphFilterGraph.h/cpp, PluginField.cpp |
| 5.2  | DONE     | SubGraphProcessor.cpp                                  |
| 5.3  | DONE     | NAMProcessor.h/cpp, IRLoaderProcessor.h/cpp            |
| 5.4  | DONE     | PluginField.cpp                                        |
| 6.1  | DEFERRED | Feature work - track in ROADMAP.md                     |
