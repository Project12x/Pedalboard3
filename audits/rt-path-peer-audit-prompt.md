You are working in: C:\Users\estee\Desktop\My Stuff\Code\Antigravity\Pedalboard2

Task: Perform a read-only peer audit of Pedalboard3's real-time audio and plugin-hosting stability path. Do not modify files. Verify claims against source code, not summaries.

Primary context to read:
- AGENTS.md
- LESSONS.md
- docs/superpowers/plans/2026-06-16-rt-hosting-sprint.md
- src/MainPanel.h
- src/BypassableInstance.cpp
- src/BypassableInstance.h
- src/FilterGraph.cpp
- src/SafetyLimiter.cpp
- src/SafetyLimiter.h
- src/ScratchRecorder.cpp
- src/ScratchRecorder.h
- src/VirtualMidiInputProcessor.cpp
- src/VirtualMidiInputProcessor.h
- src/IRLoaderProcessor.cpp
- src/IRLoaderProcessor.h
- src/NAMProcessor.cpp
- src/NAMProcessor.h
- src/MidiAppFifo.cpp
- src/MidiAppFifo.h
- src/MidiMappingManager.cpp
- src/MidiMappingManager.h
- src/SafePluginScanner.cpp
- src/PluginScannerClient.cpp
- tests/audio_thread_stress_test.cpp
- tests/vst3_loading_test.cpp
- tests/scratch_recorder_test.cpp
- tests/nam_processor_test.cpp

Audit focus:
- Audio callback real-time violations: allocation, locks/spin waits, logging, file I/O, settings reads, message-thread calls, unbounded work.
- Plugin-host stability: VST3 wrapper races, graph mutation safety, scanner/runtime crash containment, missing-plugin behavior, state/latency/block-size hazards.
- Test gaps: places where current tests overstate confidence or miss hostile plugin/audio cases.
- The RT sprint plan: identify incorrect assumptions, invalid JUCE/API usage, missing tasks, wrong priorities, or unsafe suggested fixes.

Constraints:
- Read-only audit only. Do not edit files. Do not create commits. Do not run long builds.
- Prefer `rg` and targeted file reads.
- If you run commands, keep them read-only.
- Treat JUCE 9 expanded format support as a future assumption; do not recommend pre-JUCE-9 native LV2/CLAP work unless it is required for RT safety.

Return a concise markdown audit to stdout with these sections:

# RT Path Peer Audit

## Critical Findings
Show only release-blocking RT or host-stability issues. Include file/line evidence.

## Important Findings
Show real bugs, races, test gaps, or plan problems. Include file/line evidence.

## Minor Findings
Small issues or polish for the plan/docs.

## What The Product Already Does Well
Concrete strengths with file evidence.

## RT Sprint Plan Review
Say whether the plan targets the right work. List any task changes you recommend.

## Top 5 Next Actions
Ordered, concrete actions.
