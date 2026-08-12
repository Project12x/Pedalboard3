# macOS AUv2 and AUv3 Hosting Design

Date: 2026-08-12

## Goal

Expand the Pedalboard3 macOS beta from VST3-only hosting to production-usable AUv2 and AUv3 hosting while preserving exact plugin-format identity, patch integrity, scanner crash isolation, and Windows VST3 behavior.

The supported macOS formats after this work are:

- Internal Pedalboard processors.
- VST3 effects and instruments.
- AUv2 effects and instruments.
- AUv3 effects and instruments.

Audio Unit MIDI effects should be discoverable and usable when JUCE exposes and routes them correctly, but they are not a blocker for this first AU beta. CLAP is outside this design and should remain isolated for a later JUCE update.

## Repository and JUCE Baseline

The AU work must start from the real JUCE 9 development tip rather than from the previously built macOS branch tip:

- Pedalboard3 JUCE 9 migration: `1e0b4c365574f17134423fd149ca8fa8129de35a` (`feat: migrate to JUCE 9`).
- JUCE submodule revision pinned by that commit: `f8f8864172464b9adf9eba6101e1f784838d1597`.
- Existing macOS port branch: `codex/macos-port`.
- Common ancestor between the macOS work and JUCE 9 migration: `4b7aa40`.

The macOS branch contains 13 commits beyond the common ancestor, while the development branch contains the single later JUCE 9 migration commit. Before AU implementation, replay the macOS commits onto `1e0b4c3`, resolve conflicts without discarding cross-platform changes, and verify the VST3 beta baseline again.

The rebase/replay is a prerequisite, not part of the AU behavior itself. It should be kept reviewable so JUCE 9 integration problems are distinguishable from new Audio Unit problems.

## Current Constraints

The existing source has several constraints that make a compile-definition-only change unsafe:

- Application and scanner targets currently define `JUCE_PLUGINHOST_AU=0`.
- `AudioSingletons.cpp` conditionally knows how to register Audio Units, but `App.cpp` also manually registers formats, creating multiple registration owners.
- `SafePluginListComponent` selects only the format named `VST3`.
- The external scanner IPC and process control are implemented only on Windows.
- The scanner helper invokes `findAllTypesForFile` from its JUCE message thread. AUv3 instantiation requires that thread to remain unblocked.
- The successful out-of-process path advances `PluginDirectoryScanner` by calling `scanNextFile`, which may scan the same plugin again inside the main process and undermine isolation.
- Graph and patch-loading code uses synchronous plugin creation. AUv3 requires asynchronous creation when its message thread must remain available.
- The plugin pool intentionally excludes Audio Units. This remains the safer default for the first AU beta.

## Format Registration

Introduce one format-registration entry point used by every process that needs plugin formats. It should accept whether internal formats are appropriate for that process and register formats in a deterministic order:

1. Internal format for the Pedalboard application only.
2. VST3 when `JUCE_PLUGINHOST_VST3` is enabled.
3. AudioUnit on Apple platforms when `JUCE_PLUGINHOST_AU` is enabled.

The application, tests, and scanner helper should no longer contain independent hand-written VST3/AU registration lists. The scanner helper registers external formats only.

CMake behavior must be platform-specific:

- macOS application, scanner, and relevant tests: VST3 enabled and AudioUnit enabled.
- Windows application, scanner, and tests: VST3 enabled and AudioUnit disabled.
- Non-Apple source must not include or instantiate `juce::AudioUnitPluginFormat`.

This keeps the change additive on macOS and prevents AU support from leaking into Windows builds.

## Scan Coordinator

Replace the VST3-only selection in `SafePluginListComponent` with a format-aware scan coordinator. The coordinator owns a sequence of scan jobs rather than pretending every format is a filesystem directory scan.

Each job contains:

- Format name and stable format key.
- Format object.
- Discovery roots or registry enumeration input.
- Whether asynchronous instantiation is allowed.
- Dead-man/quarantine storage scoped to that format.
- Progress and cancellation state.
- Selected scan transport: helper process or an explicitly reported fallback.

The default macOS plan runs VST3 and AudioUnit sequentially. The interface reports the active format, for example `Scanning VST3` and `Scanning Audio Units`, along with per-format and overall progress. Sequential execution avoids concurrent mutation of `KnownPluginList`, makes failures attributable, and reduces CPU/memory spikes during first launch.

VST3 remains path-based. Audio Unit discovery uses Apple AudioComponent registry identifiers returned by JUCE rather than treating Audio Units as ordinary files. The Audio Unit job must construct `PluginDirectoryScanner` with `allowPluginsWhichRequireAsynchronousInstantiation=true`; otherwise JUCE omits AUv3 components from discovery.

The shared `KnownPluginList` is updated after each successful candidate and persisted atomically after each completed format. Cancelling preserves all completed results.

## Crash-Isolated Scanner Helper

Extend the existing scanner protocol to macOS using a local Unix-domain socket with per-launch randomized endpoint and restrictive permissions. Do not reuse a predictable global socket name. The host launches the bundled scanner executable and passes the endpoint and an unguessable session token through command-line arguments or inherited environment scoped to that child.

The existing JUCE application message loop remains the scanner helper's control loop. A scan request must be dispatched to a dedicated worker thread. The worker performs format discovery while the JUCE message thread remains free to service asynchronous AUv3 creation and completion callbacks. The response is serialized only after discovery completes or times out.

The helper registers VST3 and AudioUnit on macOS. It returns complete JUCE `PluginDescription` XML for all component types found under a candidate identifier.

The host applies a per-candidate timeout. If the helper exits, crashes, hangs, or returns malformed data, the host records the exact candidate in the format-specific quarantine, restarts the helper, and continues with the next candidate.

After accepting helper results, the host must advance `PluginDirectoryScanner` with its skip/advance facility rather than invoking an in-process scan of the same candidate. Crash isolation is not considered working if a successful helper scan is followed by a redundant host-process load.

An in-process fallback may exist for development diagnostics, but the release UI must state when isolation is unavailable. A usable macOS beta requires the helper path; silently downgrading to unsafe scanning is not acceptable.

## Asynchronous Plugin Creation

Introduce a single asynchronous plugin-instance factory used by all external-plugin creation paths. Its logical input is:

- Exact `PluginDescription`.
- Sample rate.
- Block size.
- Creation purpose such as user insertion, patch restoration, or pool preparation.
- Completion callback or equivalent lifetime-safe result channel.

The factory delegates to JUCE asynchronous creation and returns either an owned instance or a structured error. Callbacks must be marshalled to the graph's mutation thread, and cancellation/lifetime tokens must prevent callbacks from touching a destroyed graph, patch loader, or editor.

User insertion behavior:

1. Create a temporary loading node at the requested location.
2. Begin asynchronous instance creation.
3. Replace the loading node with the real processor and apply connections when creation succeeds.
4. Replace it with an unresolved node and show a useful error when creation fails.

Patch restoration is staged. Internal nodes may restore immediately, but external nodes enter a pending state while instances are created. Connections are reconstructed against stable saved node IDs, not completion order. Plugin state is applied after the instance is created and before audio processing is enabled. Patch loading reports completion only when every pending node has resolved to either a live processor or an unresolved placeholder.

VST3 and AUv2 may complete quickly, but they still use this common API. Having one lifecycle avoids special cases and makes AUv3 safe without duplicating graph-loading logic.

## Audio Unit Pooling

Keep the existing Audio Unit exclusion from plugin prewarming/pooling for the first beta. AUv2 and AUv3 must load normally through the asynchronous factory, but the pool should not create spare Audio Unit instances until teardown, process-boundary, state-reset, and editor-lifecycle behavior have dedicated evidence.

This restriction affects startup optimization only; it must not prevent scanning, insertion, audio/MIDI processing, editor display, patch save, or restoration. A later milestone may enable pooling independently for AUv2 and AUv3 after lifecycle tests pass.

## Identity, State, and Patch Compatibility

JUCE `PluginDescription` remains the canonical identity record stored with a node. Restoration is strict:

- `AudioUnit` descriptions restore through the Audio Unit format.
- `VST3` descriptions restore through VST3.
- Matching name, manufacturer, or version is insufficient to substitute formats.
- AU registry identifiers are preserved verbatim and are not rewritten as filesystem paths.

Existing plugin state continues to use `getStateInformation` and `setStateInformation`. Tests must prove state round trips for both AUv2 and AUv3.

If exact creation fails during patch restoration, create an unresolved placeholder that retains:

- Original `PluginDescription` XML.
- Opaque saved plugin-state bytes.
- Stable node ID and graph position.
- Audio and MIDI connection records.
- Last failure reason.

The placeholder must round-trip through subsequent saves without discarding the unavailable plugin's information. A retry action attempts only the exact recorded format and identifier. Cross-format replacement, if ever added, must be a separate explicit user operation and is outside this design.

Existing Windows and VST3-only patches remain readable. The patch schema should add optional data rather than invalidating older files.

## User Interface

Plugin lists and search results display a concise format badge whenever ambiguity is possible: `VST3`, `AU`, or `Internal`. Sorting and search continue to use plugin name and manufacturer, while identity-sensitive actions use the complete description.

Scanning UI includes:

- Active format.
- Current candidate display name or identifier.
- Per-format and overall progress.
- Cancel action.
- Summary counts for discovered, skipped, quarantined, and failed candidates.
- A quarantine view with retry and clear actions.

Runtime creation errors include the plugin name, explicit format, identifier, and actionable reason. The graph remains responsive while AUv3 loads. Loading and unresolved nodes must be visually distinct from bypassed or disabled live nodes.

macOS follows the native application menu bar; this design does not add a duplicate Windows-style menu bar inside the application window.

## Failure and Recovery Semantics

- A scanner crash or timeout quarantines only the exact format/candidate pair.
- Quarantine records include reason and timestamp and can be retried or cleared by the user.
- Cancellation stops after the current candidate succeeds, fails, or times out; completed discoveries remain valid.
- An empty Audio Unit registry is a successful scan with zero results.
- Scanner data is written through temporary-file-plus-replace semantics so a crash cannot destroy the last valid database.
- A plugin creation failure never leaves a half-connected live processor in the graph.
- A patch restore failure preserves an unresolved placeholder and does not discard state or connections.
- AU and VST3 variants never substitute for one another automatically.
- Logs and UI errors use the same structured failure information, with technical details in logs and concise language in the UI.

## Test Strategy

Implementation follows test-driven development with focused contracts before production changes.

### Automated Unit and Contract Tests

- Format registration returns Internal/VST3/AU in the intended processes and platforms.
- Non-Apple builds do not reference or register AudioUnit.
- Scan planning includes VST3 and AudioUnit on macOS and only VST3 on Windows.
- Audio Unit jobs enable asynchronous-instantiation discovery.
- Helper results advance without an in-process rescan.
- Format-scoped quarantine does not suppress a different format with a similar identifier.
- Scan cancellation preserves completed results.
- Empty Audio Unit discovery succeeds.
- Plugin-list persistence survives simulated interrupted replacement.
- Exact-format restoration rejects silent AU/VST3 substitution.
- Asynchronous creation handles success, failure, cancellation, and owner destruction.
- Patch restoration is independent of asynchronous completion order.
- Unresolved placeholders preserve descriptions, state, node IDs, positions, and connections across another save/load cycle.
- Search/list presentation distinguishes same-named AU and VST3 plugins.

### Scanner Integration Tests

- macOS host launches and authenticates the bundled scanner through its private local socket.
- Scanner enumerates a known AUv2 and a known AUv3.
- A deliberately failing or terminated scanner is restarted and the host continues.
- A hung candidate reaches the timeout and is quarantined.
- Malformed or wrong-session IPC data is rejected.
- The scanner helper message loop remains responsive during AUv3 discovery.

### Real-Plugin Beta Matrix

The test set must include at least one AUv2 and one AUv3, with an effect and an instrument represented across the set. For each applicable plugin:

1. Discover and list it with the correct format badge.
2. Insert it without blocking the interface.
3. Open, interact with, close, and reopen its editor.
4. Process audio; instruments also receive MIDI and produce audio.
5. Bypass, reconnect, remove, and re-add it.
6. Save non-default state in a patch.
7. Restart Pedalboard and restore the patch and state.
8. Confirm the restored node remains the exact original format.

Repeat the existing VST3 effect and instrument smoke tests after AU support is enabled.

### Cross-Platform Verification

- Configure and build Debug and release-like macOS application, scanner, and tests from the rebased JUCE 9 branch.
- Run focused AU/scanner/patch tests and the relevant existing plugin-host test suite.
- Smoke-test the signed/bundled application rather than only an unbundled executable.
- Configure and build the Windows application, scanner, and tests with AudioUnit disabled.
- Re-run Windows VST3 scanner, load, and patch-restoration tests.
- Run `git diff --check` before every focused commit.

## Beta Acceptance Criteria

The AU beta is complete when:

- The macOS build uses JUCE 9 and retains the existing macOS portability patches.
- VST3 scanning and hosting still pass their existing functional smoke tests.
- AUv2 effect and instrument discovery, creation, editing, processing, state, and patch restoration work.
- AUv3 effect and instrument complete the same lifecycle without blocking the UI.
- Duplicate AU/VST3 names remain visibly distinguishable and restore as their original formats.
- Scanner crash and timeout tests demonstrate host survival, quarantine, helper restart, and continued scanning.
- Missing-plugin restoration produces a recoverable placeholder without losing graph connections or state.
- Scan cancellation and zero installed Audio Units are handled cleanly.
- Release-build smoke tests cover adding, bypassing, reconnecting, removing, saving, and reopening plugins.
- Windows remains AU-free and preserves existing VST3 behavior.
- Automated tests cover registration, scan planning, strict identity, asynchronous loading, persistence, and failure recovery.

## Explicit Non-Goals

- CLAP hosting.
- AU plugin creation or export; Pedalboard remains a host.
- Automatic AU/VST3 substitution.
- Audio Unit pooling or prewarming in the first beta.
- Replacing JUCE's Audio Unit implementation.
- A broad plugin-browser redesign beyond format identity and scan status.
- Making macOS 13 the initial deployment target; that remains a later compatibility goal.

## Implementation Boundaries

Keep work divided into focused commits so regressions are attributable:

1. Replay macOS portability commits on JUCE 9 and restore the VST3 baseline.
2. Centralize format registration and add platform contracts.
3. Add the format-aware scan coordinator.
4. Add macOS scanner IPC and worker-thread AU discovery.
5. Introduce asynchronous instance creation and staged graph insertion.
6. Add strict patch restoration and unresolved placeholders.
7. Add format-aware UI and quarantine controls.
8. Complete integration tests, real-plugin validation, and beta documentation.

Cross-platform source should remain guarded by platform capability rather than copied into macOS-only forks. Existing user changes and unrelated branch work must be preserved throughout replay and conflict resolution.
