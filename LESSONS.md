# Pedalboard3 Lessons

Last updated: 2026-06-16

This document captures durable lessons from Pedalboard3's own roadmap and from source-led review of Element, Carla, Ardour, Spotify Pedalboard, Ildaeil, and `juce_clap_hosting`. It is intended to survive individual sprints and keep future implementation choices tied to observed host behavior rather than novelty.

## Product Shape

Pedalboard3 should stay focused on immediate musician utility: plug in, hear sound, know signal is flowing, switch patches safely, and capture raw plus wet ideas without setup. Element and Carla prove that modular hosts can grow into deep engineering tools, but Pedalboard3's useful lane is faster and more performance-oriented than a general DAW or plugin laboratory.

P0 work should favor time-to-sound and confidence over breadth: real scratch-capture hardware smoke testing, One-Click Soundcheck, Starter Rig Browser, missing-plugin resilience, and release hardening. Format breadth matters later, but it should not interrupt the current live-use foundation.

## Session And Patch Survival

A host must preserve the user's graph even when the plugin ecosystem fails. Ardour saves plugin I/O shape, pin maps, state, routing, and automation so missing processors do not destroy the session. Element exposes placeholder-node behavior as a normal graph feature.

Pedalboard3 currently drops a node when `FilterGraph::createNodeFromXml()` cannot recreate the plugin, then attempts to restore connections by saved node IDs. That means a missing plugin can collapse routing and make a patch harder to repair. A durable `MissingPlugin` or placeholder processor should preserve:

- saved node ID, x/y position, width/height, window state, bypass, MIDI channel, and program index
- original `PluginDescription`, `fileOrIdentifier`, format name, manufacturer, version, and unique ID
- saved state blob as opaque data
- declared audio/MIDI pin shape, or best available saved fallback
- load error string and timestamp
- reconnect, rescan, substitute, and remove actions

Starter rigs should depend on this resilience. A rig with missing optional plugins should open in a degraded but explainable state, not fail silently.

## Plugin Discovery And Failure UX

Pedalboard3 already has important stability assets: `SafePluginScanner`, `PluginScannerClient`, plugin blacklist management, crash protection, protected editor creation, VST3 wrapper hardening, and VST3 concurrent-access tests. That is above a naive JUCE AudioPluginHost clone.

The next maturity step is user-visible failure management:

- scan status, last scanned plugin, timeout, crash, blacklist reason, retry, and clear actions
- scanner logs that are reachable from the UI
- separate "known bad during scan" from "failed during runtime"
- clear messages when a plugin is skipped because it is blacklisted
- crash context that names the operation and plugin

Element's crashed-plugin file and scanner worker pattern are the closest direct source lesson. Carla's lesson is that every plugin format eventually needs rich, specific error reporting.

## Real-Time Audio Discipline

The audio callback contract is non-negotiable: no heap allocation, no blocking locks, no logging, no file I/O, no settings reads, no UI calls, and no message-thread dispatch. Audio-to-UI communication should use atomics, bounded queues, or fixed ring buffers with explicit drop behavior.

Pedalboard3 has good RT intent in the main callback: preallocated input and master-bus buffers, atomic gain state, smoothed gain ramps, device-level metering, and scratch recorder taps. It also has known audit targets:

- fixed stack gain ramps guarded only by `jassert`
- channel pointer arrays sized for `MaxChannels` while device channel counts can be larger
- logging from `VirtualMidiInputProcessor::processBlock()`
- JUCE IIR coefficient factory calls from `IRLoaderProcessor::processBlock()` via `updateFilters()`
- `MidiMappingManager` reading settings and command targets from audio-reachable code
- `MidiAppFifo` using a writer-side `SpinLock`
- scratch capture relying on `ThreadedWriter` without slow-writer stress coverage
- safety limiter lacking explicit NaN/Inf tests and a fully verified protection chain

These are not reasons to stop feature work forever, but they should be handled before claiming stage-grade reliability.

## Plugin Runtime Stability

Scanning protects startup. Runtime stability protects the gig. Carla, Ardour, and Spotify Pedalboard all spend serious effort on behavior after a plugin has loaded: latency, state, variable block sizes, reset/reload behavior, editor behavior, bus layouts, and hostile plugin edge cases.

Pedalboard3's `BypassableInstance` cache of bus/channel data is a strong local lesson: once the audio thread is active, querying plugin bus state from UI code can race and crash. Keep extending that pattern. The UI should prefer cached host-owned shape data and avoid direct plugin queries during processing.

The test suite should grow from "whatever installed VST3 exists locally" to a curated plugin-host fixture matrix:

- fixed-block and variable-block processors
- processors that report latency
- processors that misreport latency
- processors with zero inputs, zero outputs, sidechains, and MIDI-only behavior
- processors that throw or fail in editor creation
- processors that change bus layout or state shape
- processors that produce NaN/Inf, DC, excessive gain, or silence unexpectedly

Spotify Pedalboard's tests are the useful model here: small hostile fixtures reveal host behavior better than broad manual plugin scans.

## Format Support

Expanded format support is expected to come with JUCE 9. Until then, do not spend major effort building native LV2 or CLAP support unless the goal explicitly changes.

Carla's LV2 implementation shows native LV2 hosting is a subsystem, not a checkbox: URID map/unmap, workers, state, presets, UI bridges, latency ports, bridge processes, and cache behavior. `juce_clap_hosting` is permissively licensed but self-described as early and still missing host behaviors that matter for production. Ildaeil is a pragmatic Carla-wrapper path, but with parameter/state exposure limitations.

Near-term work should keep the host abstraction clean so JUCE 9 support can land without disturbing the product:

- plugin descriptions and saved state remain format-agnostic
- missing-plugin placeholders preserve format names and opaque state
- scanner UX is not VST3-specific
- tests distinguish host contract behavior from format-specific behavior

## Safety And Stage UX

Stage users need obvious controls and predictable failure modes. Pedalboard3 already has a stage panic affordance, input/output meters, limiter state, and scratch status. These should become a tested emergency system:

- Panic sends all-notes-off, silences dangerous output, and un-mutes only when the user explicitly requests recovery
- limiter detects NaN/Inf, sustained gain, DC, and unreasonable output
- limiter state is surfaced in Stage View without blocking the audio thread
- Soundcheck can diagnose no input, no output, clipping, and muted/blacklisted plugin conditions

Safety systems should fail loud in the UI and silent in the speakers.

## Testing And Verification

The current test suite has useful coverage: audio-thread stress patterns, VST3 concurrent access, protection tests, patch-switch infrastructure tests, scratch recorder tests, NAM tests, and UI regression harness tests.

The remaining test gap is realism. Add tests that intentionally violate assumptions:

- block size 1, odd sizes, larger-than-prepared sizes, and changing block sizes
- slow scratch writer and failed writer sinks
- external plugin latency and reset/reload semantics
- missing plugin restore from XML
- blacklisted plugin load attempts
- NaN/Inf and DC injection through the limiter
- MIDI/OSC bursts while patch switching

Manual smoke tests still matter for hardware. Scratch capture cannot be declared complete until a real guitar/interface test confirms raw DI, wet heard sound, correct metadata, and clean stop on patch/device change.

## Licensing And Reuse

Reuse mode for the current upstream research is pattern-only. No upstream source was copied.

Reference ledger:

- Element, `c48bf05bdf03d89bb944258aeddb9646da73783e`, GPL-3.0-or-later source, pattern-only
- Carla, `97a9e0740baf6df2df942495c02532a624c44682`, GPL-2.0-or-later source, pattern-only
- Ardour, `77eebd335624904b7470d28897ea4b6574a0743c`, GPL-2.0-or-later source, pattern-only
- Spotify Pedalboard, `cd18ef0d9ccd972a7b7df33fbc36751d5fb29bfd`, GPLv3 source, pattern-only
- DISTRHO Ildaeil, `af9fc9f73b1a1832da8d6dfa12f7d03c431293d6`, GPL-2.0-or-later source, pattern-only
- `juce_clap_hosting`, `aa8a81232116ad017f9eee07a1b0a84433f61f5a`, MIT source, pattern-only

If future work copies, ports, forks, or closely adapts any permissively reusable code, record the upstream repo, commit, license, source files, reuse mode, attribution, and change notes in the implementation plan and commit summary.

## Release Priorities

Before broad public release, the host should have:

- Windows build and installer path verified
- repeatable CI for build and tests
- crash dump or crash context capture
- scanner log access
- real audio hardware smoke tests
- RT sprint complete or explicitly waived
- missing-plugin placeholders implemented
- Starter Rig Browser resilient to missing dependencies

The release bar is not "all formats." The release bar is "a musician can trust it for normal local use and recover when third-party plugins misbehave."
