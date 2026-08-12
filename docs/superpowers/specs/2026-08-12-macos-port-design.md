# Pedalboard3 macOS Usable Beta Design

**Date:** 2026-08-12

**Status:** Approved design

**Initial target:** Apple Silicon on the development Mac running macOS 26.6.1

**Later compatibility target:** macOS 13 Ventura on Apple Silicon

## Summary

Pedalboard3 will become a single cross-platform JUCE application rather than a
separate macOS fork. The first macOS beta will validate the live-host essentials:
CoreAudio/CoreMIDI, safe VST3 scanning and hosting, built-in processors, NAM,
patches, mappings, Stage Mode, and live patch switching. Existing Windows VST3
patches must open on macOS when the same VST3 plugins are installed.

The initial beta targets the current Apple Silicon development Mac. Audio Unit
hosting follows VST3 parity as the next beta milestone. macOS 13 support is a
later compatibility gate. Intel builds, signing, notarization, DMG packaging,
and complete validation of secondary features are outside this design.

## Goals

- Build and launch Pedalboard3 as a native Apple Silicon `.app`.
- Use CoreAudio and CoreMIDI through JUCE.
- Safely scan, load, display, process, bypass, remove, and restore VST3 plugins.
- Preserve Windows-to-macOS VST3 patch compatibility.
- Preserve Pedalboard3's scanner timeout, blacklist, restart, and progress
  behavior while replacing its Windows-only transport.
- Keep patch activation transactional so a missing or failed plugin cannot
  replace the active graph with a partial graph.
- Validate the live-performance path with automated, smoke, and soak tests.
- Enable and validate Audio Unit hosting after VST3 parity.
- Leave a clear path to a tested macOS 13 deployment target.

## Non-goals

- A separate macOS codebase or long-lived platform fork.
- Intel or universal binaries in the first beta.
- Signing, notarization, hardened-runtime configuration, or a DMG installer.
- CI distribution or public release automation.
- Automatic substitution between VST3 and Audio Unit variants.
- Complete validation of every secondary Windows feature before the first beta.
- Claiming macOS 13 compatibility solely from a deployment-target setting.

## Current Project Findings

The repository already contains useful macOS foundations:

- JUCE 9 and a CMake GUI application target.
- An `.icns` application icon.
- macOS main-menu and file-open handling in `App.cpp`.
- Portable settings paths based on JUCE special locations.
- Conditional Audio Unit registration code in `AudioSingletons.cpp`.
- CoreAudio/CoreMIDI support available through JUCE modules.

The main porting gaps are:

- Windows audio and UI definitions are enabled globally in CMake.
- Audio Unit hosting is disabled globally.
- Plugin format registration has more than one owner and can register VST3
  twice.
- The isolated scanner client, worker, and pipe name are Windows-specific.
- The scanner helper and documentation are not installed using macOS bundle
  conventions.
- Plugin drag-and-drop checks obsolete macOS extensions and omits `.vst3`.
- Patch restore attempts to instantiate the serialized plugin description
  directly, including a platform-specific plugin path.
- Patch restore can commit fewer nodes than requested when a plugin fails.
- macOS microphone privacy metadata is not part of the application target.
- Tests and documentation contain Windows-only assumptions.

## Architecture

### One shared application

Platform-independent graph, UI, patch, MIDI, Stage Mode, NAM, and built-in
processor code remains shared. Platform conditions are limited to build
configuration, bundle layout, plugin formats, and operating-system integration.

The top-level CMake configuration will define explicit platform behavior:

- `WIN32` owns ASIO, WASAPI, DirectSound, per-monitor DPI, Win32 compatibility
  definitions, and Windows libraries.
- `APPLE` builds the native application bundle, uses the existing `.icns`,
  supplies microphone privacy text, and relies on JUCE CoreAudio/CoreMIDI.
- VST3 hosting is enabled on Windows and macOS.
- Audio Unit hosting is enabled only for the Audio Unit milestone.
- The scanner helper is embedded at
  `Pedalboard3.app/Contents/Helpers/Pedalboard3Scanner`.
- Documentation is embedded below `Pedalboard3.app/Contents/Resources`.

The first implementation pass may use Ninja or an Xcode generator, but the
generated product and runtime layout must be equivalent. Toolchain setup may
install CMake, Ninja, and any required Apple development tools and libraries.

### Single plugin-format registry

Plugin format registration will have one owner used by the application,
scanner worker, and tests. It registers:

- Pedalboard3 internal processors in the host where required.
- VST3 on both supported desktop platforms.
- Audio Unit on macOS only after the VST3 milestone is accepted.

Window construction must not register external formats. This eliminates the
current duplicate VST3 registration and keeps host and scanner capabilities in
sync.

## Safe Scanner Design

### Boundary

Pedalboard3 retains its scan policy and adopts JUCE's cross-platform process
transport. This is a hybrid, not a replacement of the custom scanner.

Pedalboard3 continues to own:

- Per-plugin scan requests and XML plugin-description results.
- A 30-second per-plugin deadline.
- Crash and hang blacklisting.
- Worker restart after failure.
- Scan progress and user-visible errors.
- Format selection and protocol compatibility.

JUCE `ChildProcessCoordinator` and `ChildProcessWorker` provide:

- Cross-platform worker launch.
- Random pipe names rather than a single global pipe.
- Framed `MemoryBlock` messages.
- Heartbeats and disconnect detection.
- Worker termination and connection lifecycle.

### JUCE scanning integration

The isolated bridge will implement `KnownPluginList::CustomScanner`.
`PluginDirectoryScanner` remains responsible for plugin enumeration, progress,
unchanged-plugin detection, failed-file tracking, and the dead-man's-pedal
file. When it needs to inspect one plugin candidate, `KnownPluginList` delegates
that operation to the Pedalboard custom scanner, which sends the work to the
isolated process.

This replaces the existing indirect sequence that scans externally, inserts
results, and invokes the base scanner again to advance its cursor.
`SafePluginScanner` remains the scan UI and session orchestrator, but it attaches
the custom scanner to `KnownPluginList` and calls `scanNextFile()` only once per
candidate. It no longer invokes `PluginScannerClient` as a separate scan path.

### Request flow

1. The plugin-list UI starts or resumes `PluginDirectoryScanner`.
2. JUCE selects the next file or format identifier.
3. Pedalboard's `KnownPluginList::CustomScanner` sends a scan request.
4. The coordinator lazily launches or reuses the bundled worker.
5. The worker calls the selected format's `findAllTypesForFile()`.
6. The worker serializes all successful `PluginDescription` values to XML.
7. The host validates the response and returns the descriptions to
   `KnownPluginList`.
8. JUCE records the results and advances scanning normally.

### Protocol

Messages use a versioned envelope serialized into `MemoryBlock`, not a raw C
struct. Every request or response contains the applicable fields from:

- Protocol version.
- Request ID.
- Message type.
- Plugin format.
- Plugin path or format identifier.
- Result code.
- Error text.
- Plugin-description XML.

The host enforces a bounded payload size. Unknown message types, malformed
payloads, mismatched request IDs, and protocol-version mismatches fail safely.
A version mismatch reports that the application and helper do not belong to
the same build.

### Scanner lifecycle and failures

- The worker starts lazily and remains alive during a normal scan session.
- A request timeout kills the worker, blacklists only the active candidate,
  reports the timeout, and permits scanning to continue with a fresh worker.
- An unexpected disconnect during an active request is treated as a scanner
  crash and blacklists only that candidate.
- A disconnect while idle does not blacklist the last successful plugin.
- User cancellation terminates or releases the active request without
  blacklisting it.
- Successful descriptions collected before a later failure remain valid.
- Scanner failure never terminates the host process.

## Plugin and Patch Compatibility

### VST3 identity resolution

Serialized VST3 descriptions include a platform-specific file location. Patch
restore therefore resolves every external VST3 in this order:

1. Attempt the serialized description exactly as stored.
2. If it is unavailable or cannot be instantiated, search the local scanned
   VST3 list using format, JUCE `uniqueId`, manufacturer, and plugin name.
3. Use the matched local macOS description to instantiate the plugin.
4. Apply the original plugin state, program, bypass state, layout, mappings,
   and connections.

The resolver is shared by Windows and macOS. This is required because saving a
patch after successful resolution may replace the serialized file location with
the current platform's location; the other platform must be able to resolve it
again on the next transfer.

The resolver prefers the modern JUCE `uniqueId`, which is intended to be stable
for VST3 across platforms. Manufacturer and name guard against ambiguous or
malformed matches. Legacy `deprecatedUid` may be used only as a constrained
fallback when the modern ID is absent and the descriptive fields agree.

### Format boundaries

- A VST3 patch resolves only to VST3.
- Pedalboard never silently replaces VST3 with Audio Unit or vice versa.
- Audio Unit patches are macOS-specific unless a future migration design says
  otherwise.
- Saving a restored patch may store the local plugin description, but either
  platform can recover the corresponding VST3 through stable resolution.
- The original file is not changed until the user explicitly saves it.

### Transactional patch restore

All processors and states are prepared before the active graph is replaced.
The prepared graph is committed only if every required node is available and
valid. If any external plugin is missing, blacklisted, architecture-incompatible,
or fails to instantiate:

- The active graph continues processing.
- No partial graph is committed.
- The patch file remains unchanged.
- The user receives a structured list of unresolved plugins and reasons.
- The user may scan or install the missing plugin and retry.

Connections, UI properties, MIDI/OSC mappings, program selection, bypass state,
and plugin state are applied only to a complete prepared graph.

## macOS Runtime Behavior

### Audio and MIDI

JUCE supplies CoreAudio and CoreMIDI device integration. The application bundle
declares microphone use before requesting audio input. If device initialization
fails, the UI remains usable and directs the user to audio-device settings.

The initial smoke matrix covers common sample rates and buffer sizes supported
by the attached device. Audio-thread rules already documented in
`ARCHITECTURE.md` remain unchanged.

### Plugin architecture reporting

The first beta is Apple Silicon-only. A plugin that cannot load because it is
Intel-only must be reported as architecture-incompatible when that distinction
can be established, rather than being presented as a generic missing plugin.
Rosetta hosting and cross-architecture bridging are outside this design.

### File and bundle behavior

- `.pdl` files passed at launch continue to open through the existing macOS
  application path.
- Plugin drag-and-drop recognizes `.vst3`; Audio Unit discovery uses the normal
  format search mechanism rather than requiring bundle drag-and-drop.
- Help resolves documentation from the application resources directory.
- Settings, logs, blacklists, scan state, and caches remain below the JUCE user
  application-data location for `Pedalboard3`.

## Milestones

### Milestone 1: Native bring-up

- Install and verify the local Apple build toolchain.
- Isolate platform-specific CMake definitions.
- Build Debug and Release application bundles for `arm64`.
- Fix compile, link, resource, bundle, and startup issues.
- Validate CoreAudio/CoreMIDI and built-in processors.

### Milestone 2: VST3 parity and portable patches

- Consolidate plugin-format registration.
- Validate VST3 discovery, instantiation, editors, state, and processing.
- Implement stable VST3 patch resolution.
- Make patch preparation transactional.
- Correct VST3 drag-and-drop and platform-specific test assumptions.

### Milestone 3: Shared safe scanner

- Replace Win32 scanner transport with the coordinator/worker bridge.
- Integrate through `KnownPluginList::CustomScanner`.
- Bundle and locate the helper correctly.
- Validate timeout, crash, cancellation, restart, payload, and version behavior.
- Re-run Windows scanner tests to preserve existing behavior.

### Milestone 4: Live-host beta validation

- Validate real VST3 plugins and a representative Windows patch.
- Validate NAM, built-ins, MIDI mappings, Stage Mode, and patch switching.
- Complete smoke and soak gates.
- Record any secondary-feature validation gaps without allowing critical audio,
  scanning, patch, or data-integrity failures into the beta.

### Milestone 5: Audio Unit hosting

- Enable Audio Unit registration for the host and worker.
- Validate discovery, scanning, instantiation, editor display, state restore,
  bypass, removal, and rescan behavior.
- Keep VST3/AU identity and patch behavior separate.

### Later milestone: macOS 13 compatibility

- Set `CMAKE_OSX_DEPLOYMENT_TARGET=13.0` or the equivalent preset setting.
- Build against an SDK capable of producing a macOS 13-compatible binary.
- Run the first-beta smoke suite on real macOS 13 Apple Silicon hardware or a
  suitable Apple Silicon test environment.
- Resolve API, dependency, and bundle compatibility issues discovered there.

## Testing Strategy

### Automated tests

- Platform-specific CMake configuration and definitions.
- Exactly one registration of each enabled plugin format.
- Protocol serialization, version checks, request correlation, malformed input,
  and payload limits.
- Scanner timeout, crash, idle disconnect, cancellation, restart, and blacklist
  decisions.
- Cross-platform VST3 resolution using synthetic Windows and macOS
  `PluginDescription` values.
- Rejection of ambiguous, mismatched-format, and VST3-to-AU substitutions.
- Transactional patch behavior when any node fails to prepare.
- Preservation of state, layout, mappings, and connections after successful
  portable resolution.
- Application resource and scanner-helper path resolution.

### Deterministic scanner fixtures

Test-only JUCE VST3 fixtures provide:

- A working pass-through plugin with an editor and restorable state.
- A plugin that exits during scanning.
- A plugin that hangs during scanning.

These fixtures make scanner isolation repeatable without depending solely on
third-party products.

### Manual smoke tests

On the Apple Silicon development Mac:

- Launch the `.app` and complete microphone permission handling.
- Select CoreAudio input/output and CoreMIDI devices.
- Scan, load, edit, process, bypass, remove, and reload real VST3 plugins.
- Open a representative Windows patch whose VST3 plugins are also installed on
  macOS.
- Exercise built-ins, NAM, mappings, Stage Mode, and patch switching.
- Verify missing, blacklisted, architecture-incompatible, crashed, and hung
  plugin messages.

### Soak test

Run one hour of continuous audio while repeatedly changing representative
patches. The host must not crash, replace the graph partially, or lose patch
data. Any audible failure is investigated to distinguish device or plugin
behavior from a host regression before acceptance.

## First-beta Acceptance Criteria

The first VST3 beta is accepted when all of the following are true:

- Debug and Release Apple Silicon builds complete cleanly.
- The native `.app` launches with its resources and scanner helper available.
- CoreAudio input/output and CoreMIDI operate on the development Mac.
- VST3 scan, load, editor, processing, bypass, state restore, and removal work.
- A representative Windows VST3 patch resolves and restores on macOS.
- Scanner crash and hang fixtures do not terminate the host.
- Missing or failed plugins cannot cause a partial patch commit.
- Built-ins, NAM, mappings, Stage Mode, and live patch switching pass smoke
  tests.
- The one-hour soak test completes without a host crash or patch-data loss.
- There are no known critical live-audio, scanner-isolation, patch-integrity,
  or data-loss defects.

Secondary features may have explicitly recorded validation gaps. Audio Unit
support is accepted separately after it passes the corresponding scan, load,
editor, processing, and state tests.

## Risks and Mitigations

- **Third-party plugin variability:** use deterministic fixtures plus a small
  representative real-plugin matrix.
- **VST3 identity ambiguity:** require format and modern ID, then validate
  manufacturer and name; reject ambiguity instead of guessing.
- **Scanner refactor regression on Windows:** retain Pedalboard policy tests and
  run Windows scanner coverage before considering the shared transport done.
- **Bundle helper discovery:** centralize path resolution and test both build-tree
  and bundled execution.
- **Apple Silicon-only plugins or dependencies:** report architecture failures
  clearly and defer bridging.
- **macOS 13 API or dependency gaps:** keep compatibility as a separate tested
  milestone rather than constraining initial bring-up prematurely.
- **Scope growth:** first-beta gating remains limited to live-host essentials;
  packaging and complete secondary-feature validation remain separate work.
