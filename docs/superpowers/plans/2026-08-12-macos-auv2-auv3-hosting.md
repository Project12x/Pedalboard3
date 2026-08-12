# macOS AUv2 and AUv3 Hosting Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Deliver a JUCE 9 macOS beta that safely scans, loads, edits, processes, saves, and restores AUv2 and AUv3 effects and instruments without regressing VST3 or Windows behavior.

**Architecture:** Replay the verified macOS portability series onto the JUCE 9 development tip, centralize plugin-format registration, then introduce a sequential format-aware scan coordinator backed by a crash-isolated macOS scanner helper. Route external plugin creation through one asynchronous factory, represent pending or unavailable plugins with state-preserving graph nodes, and make main-graph and rack restoration completion-aware.

**Tech Stack:** C++17, JUCE 9.0.0 at submodule commit `f8f8864172464b9adf9eba6101e1f784838d1597`, CMake 3.22+, Ninja, Catch2 3.7, AudioComponent APIs through JUCE, Windows named pipes, macOS Unix-domain sockets.

## Global Constraints

- Create the integration branch from the planning branch's current tip so this plan remains available; Pedalboard commit `f7bbbc4` and the JUCE 9 migration at `1e0b4c365574f17134423fd149ca8fa8129de35a` must be ancestors.
- Replay all 13 commits from `84d3264` through `ba792ff` and preserve their cross-platform guards.
- Keep Internal and VST3 hosting enabled; enable AudioUnit only when `APPLE` is true.
- AUv2 and AUv3 effects and instruments are release blockers; AU MIDI effects are supported when JUCE exposes them but are not a beta blocker.
- Keep AU prewarming and pooling disabled in this beta.
- Preserve exact `PluginDescription::pluginFormatName` and `fileOrIdentifier`; never substitute AU and VST3 automatically.
- Keep the JUCE message thread unblocked during AUv3 discovery and creation.
- The release scanner must remain out of process on macOS; an unsafe fallback must be explicitly reported.
- Preserve unavailable plugin description XML, opaque state, node ID, position, and connections across repeated patch saves.
- Keep macOS 13 deployment work and CLAP hosting outside this implementation.
- Use test-first changes and focused local commits; do not create a pull request unless explicitly requested.

---

## File Structure

### New production files

- `src/SupportedPluginFormats.h/.cpp`: the only application/scanner format-registration policy.
- `src/PluginFormatIdentity.h/.cpp`: exact identity keys and short display labels such as `AU`.
- `src/PluginScanCoordinator.h/.cpp`: format job planning, sequential execution, progress, cancellation, and summaries.
- `src/PluginQuarantine.h/.cpp`: format-scoped scanner failure records.
- `src/AtomicFileWriter.h/.cpp`: same-directory temporary-file replacement used by settings and quarantine persistence.
- `src/PluginScannerTransport.h/.cpp`: framed scanner channel plus Windows named-pipe and macOS Unix-socket endpoints.
- `src/PluginInstanceFactory.h/.cpp`: cancellable asynchronous wrapper around JUCE plugin creation.
- `src/PluginPlaceholderProcessor.h/.cpp`: loading/unresolved graph processor retaining exact identity and state.

### New tests

- `tests/plugin_format_registration_test.cpp`
- `tests/plugin_scan_coordinator_test.cpp`
- `tests/plugin_quarantine_test.cpp`
- `tests/plugin_scanner_transport_test.cpp`
- `tests/plugin_instance_factory_test.cpp`
- `tests/plugin_placeholder_test.cpp`
- `tests/async_graph_loading_test.cpp`
- `tests/au_host_integration_test.cpp`

### Existing files changed

- Build: `CMakeLists.txt`, `tests/CMakeLists.txt`.
- Registration/scanning: `src/App.cpp`, `src/AudioSingletons.cpp`, `src/PluginScannerIPC.h`, `src/PluginScannerClient.h/.cpp`, `src/SafePluginScanner.h/.cpp`, `src/scanner/PluginScannerMain.cpp`.
- Persistence/UI: `src/SettingsManager.cpp`, `src/PluginBlacklist.h/.cpp`, `src/BlacklistWindow.h/.cpp`, `src/MainPanel.cpp`, `src/PluginSearchOverlay.cpp`, `src/PluginSearchLogic.h`.
- Graph/racks: `src/IFilterGraph.h`, `src/FilterGraph.h/.cpp`, `src/UndoActions.h/.cpp`, `src/PluginField.h`, `src/PluginFieldPersistence.cpp`, `src/PluginComponent.h/.cpp`, `src/SubGraphFilterGraph.h/.cpp`, `src/SubGraphProcessor.h/.cpp`.
- Pool/docs: `src/PluginPoolManager.cpp`, `tests/plugin_pool_manager_test.cpp`, `documentation/plugins.html`, `documentation/troubleshooting.htm`.

---

### Task 1: Replay the macOS Port on the JUCE 9 Tip

**Files:**
- Create: `docs/qa/2026-08-12-juce9-macos-replay.md`
- Verify: `JUCE`, `CMakeLists.txt`, `CMakePresets.json`, `tests/CMakeLists.txt`

**Interfaces:**
- Consumes: Pedalboard `f7bbbc4` and the 13 macOS commits listed below.
- Produces: `codex/macos-au-hosting` containing this plan, the verified macOS beta, and JUCE 9.

- [ ] **Step 1: Create an isolated execution worktree**

Invoke `superpowers:using-git-worktrees`, create `codex/macos-au-hosting` from the current planning-branch tip, and confirm:

```bash
git rev-parse HEAD
git merge-base --is-ancestor f7bbbc4 HEAD
git submodule status JUCE
```

Expected: the ancestor check exits 0, this plan exists in the new worktree, and JUCE begins with `f8f8864172464b9adf9eba6101e1f784838d1597`.

- [ ] **Step 2: Replay the macOS commits in original order**

```bash
git cherry-pick 84d3264 16e2a3c 1309c2c 54fd52f 58f361c 1c41003 45e2f1b cd01bed 55547ab 83c97bc 301e5bc bc36d35 ba792ff
```

For every conflict, preserve the JUCE 9 changes and the macOS platform guards. Resolve individual hunks; do not replace a whole conflicted file with either side.

- [ ] **Step 3: Verify ancestry, commit count, and submodule**

```bash
git merge-base --is-ancestor f7bbbc4 HEAD
git log --reverse --format='%h %s' f7bbbc4..HEAD
git submodule status JUCE
```

Expected: ancestor check exits 0, the log contains this plan commit plus 13 replayed commits, and JUCE remains at `f8f8864`.

- [ ] **Step 4: Build and test the VST3 baseline**

```bash
/opt/homebrew/bin/cmake --preset macos-arm64-debug
/opt/homebrew/bin/cmake --build --preset macos-arm64-debug --target Pedalboard3 Pedalboard3_Tests
/opt/homebrew/bin/ctest --preset macos-arm64-debug -R 'macos|platform.paths|startup.options|vst3|rt.hosting' --output-on-failure
test -x build/macos-arm64-debug/Pedalboard3_artefacts/Debug/Pedalboard3.app/Contents/MacOS/Pedalboard3
test -f build/macos-arm64-debug/Pedalboard3_artefacts/Debug/Pedalboard3.app/Contents/Resources/documentation/index.htm
```

Expected: targets link, selected tests pass, and bundle checks exit 0.

- [ ] **Step 5: Record evidence and commit**

Record source tip, replayed commits, submodule hash, commands, and results in the QA file.

```bash
git add docs/qa/2026-08-12-juce9-macos-replay.md
git commit -m "docs: record JUCE 9 macOS replay validation"
```

---

### Task 2: Centralize Platform-Aware Format Registration

**Files:**
- Create: `src/SupportedPluginFormats.h`, `src/SupportedPluginFormats.cpp`, `tests/plugin_format_registration_test.cpp`
- Modify: `src/App.cpp`, `src/AudioSingletons.cpp`, `src/scanner/PluginScannerMain.cpp`, `CMakeLists.txt`, `tests/CMakeLists.txt`

**Interfaces:**
- Produces: `enum class PluginFormatRole { application, scanner };`
- Produces: `using InternalFormatRegistrar = std::function<void(juce::AudioPluginFormatManager&)>;`
- Produces: `void registerSupportedPluginFormats(juce::AudioPluginFormatManager&, PluginFormatRole, InternalFormatRegistrar = {});`
- Produces: `juce::StringArray getRegisteredFormatNames(const juce::AudioPluginFormatManager&);`

- [ ] **Step 1: Write failing registration tests**

```cpp
TEST_CASE("application formats are registered exactly once", "[plugins][formats]")
{
    juce::AudioPluginFormatManager manager;
    int internalRegistrationCalls = 0;
    auto registerInternal = [&] (auto&) { ++internalRegistrationCalls; };
    registerSupportedPluginFormats(manager, PluginFormatRole::application, registerInternal);
    auto names = getRegisteredFormatNames(manager);
    REQUIRE(internalRegistrationCalls == 1);
    REQUIRE(names.contains("VST3"));
#if JUCE_MAC
    REQUIRE(names.contains("AudioUnit"));
#else
    REQUIRE_FALSE(names.contains("AudioUnit"));
#endif
    for (const auto& name : names)
        REQUIRE(names.indexOf(name) == names.lastIndexOf(name));
}

TEST_CASE("scanner excludes internal processors", "[plugins][formats]")
{
    juce::AudioPluginFormatManager manager;
    registerSupportedPluginFormats(manager, PluginFormatRole::scanner);
    registerSupportedPluginFormats(manager, PluginFormatRole::scanner);
    REQUIRE_FALSE(getRegisteredFormatNames(manager).contains("Internal"));
    auto names = getRegisteredFormatNames(manager);
    for (const auto& name : names)
        REQUIRE(names.indexOf(name) == names.lastIndexOf(name));
}
```

- [ ] **Step 2: Run and confirm the missing API failure**

```bash
/opt/homebrew/bin/cmake --build --preset macos-arm64-debug --target Pedalboard3_Tests
```

Expected: compilation fails because `SupportedPluginFormats.h` does not exist.

- [ ] **Step 3: Implement the single registration policy**

Define the header exactly:

```cpp
#pragma once
#include <JuceHeader.h>
#include <functional>

enum class PluginFormatRole { application, scanner };
using InternalFormatRegistrar = std::function<void(juce::AudioPluginFormatManager&)>;
void registerSupportedPluginFormats(juce::AudioPluginFormatManager&, PluginFormatRole,
                                    InternalFormatRegistrar = {});
juce::StringArray getRegisteredFormatNames(const juce::AudioPluginFormatManager&);
```

For `application`, invoke the supplied registrar once when no existing format is named `Internal`. The shared implementation must not include or reference `InternalPluginFormat`, keeping the scanner's link graph small. Add VST3 under `JUCE_PLUGINHOST_VST3` and AudioUnit under `JUCE_PLUGINHOST_AU && JUCE_MAC`. Check existing format names before each action so repeated calls are idempotent.

- [ ] **Step 4: Remove duplicate owners and set target definitions**

Make `AudioPluginFormatManagerSingleton` allocate an empty manager. Replace the hand-written Internal/VST3 block in `StupidWindow` with one centralized call whose registrar executes `manager.addFormat(new InternalPluginFormat)`. Make the scanner call it with `scanner` and no registrar.

In CMake:

```cmake
if(APPLE)
    set(PEDALBOARD3_PLUGINHOST_AU 1)
else()
    set(PEDALBOARD3_PLUGINHOST_AU 0)
endif()
```

Use `JUCE_PLUGINHOST_AU=${PEDALBOARD3_PLUGINHOST_AU}` for app, scanner, and tests, and add the new source files to all required targets.

- [ ] **Step 5: Build, test, and commit**

```bash
/opt/homebrew/bin/cmake --preset macos-arm64-debug
/opt/homebrew/bin/cmake --build --preset macos-arm64-debug --target Pedalboard3 Pedalboard3Scanner Pedalboard3_Tests
/opt/homebrew/bin/ctest --preset macos-arm64-debug -R 'application formats|scanner excludes internal|macos.build' --output-on-failure
git diff --check
git add CMakeLists.txt tests/CMakeLists.txt tests/plugin_format_registration_test.cpp src/SupportedPluginFormats.h src/SupportedPluginFormats.cpp src/App.cpp src/AudioSingletons.cpp src/scanner/PluginScannerMain.cpp
git commit -m "feat: register VST3 and Audio Unit formats centrally"
```

---

### Task 3: Add Exact Identity and a Sequential Scan Plan

**Files:**
- Create: `src/PluginFormatIdentity.h/.cpp`, `src/PluginScanCoordinator.h/.cpp`, `tests/plugin_scan_coordinator_test.cpp`
- Modify: `src/SafePluginScanner.h/.cpp`, `CMakeLists.txt`, `tests/CMakeLists.txt`

**Interfaces:**
- Produces: `juce::String makeExactPluginKey(const juce::PluginDescription&);`
- Produces: `juce::String pluginFormatBadge(const juce::String&);`
- Produces: `PluginScanJob { formatName, format, locations, allowAsyncInstantiation, deadMansPedalFile };`
- Produces: `std::vector<PluginScanJob> makePluginScanPlan(manager, dataDirectory);`

- [ ] **Step 1: Write failing identity and plan tests**

```cpp
TEST_CASE("exact identity includes format", "[plugins][scan-plan]")
{
    juce::PluginDescription au, vst;
    au.name = vst.name = "Twin";
    au.fileOrIdentifier = vst.fileOrIdentifier = "vendor.product";
    au.pluginFormatName = "AudioUnit";
    vst.pluginFormatName = "VST3";
    REQUIRE(makeExactPluginKey(au) != makeExactPluginKey(vst));
    REQUIRE(pluginFormatBadge("AudioUnit") == "AU");
}

TEST_CASE("mac scan plan enables asynchronous Audio Units", "[plugins][scan-plan]")
{
    juce::AudioPluginFormatManager manager;
    registerSupportedPluginFormats(manager, PluginFormatRole::scanner);
    auto jobs = makePluginScanPlan(manager, juce::File::getSpecialLocation(juce::File::tempDirectory));
    REQUIRE(jobs.front().formatName == "VST3");
#if JUCE_MAC
    auto au = std::find_if(jobs.begin(), jobs.end(),
                           [] (const auto& job) { return job.formatName == "AudioUnit"; });
    REQUIRE(au != jobs.end());
    REQUIRE(au->allowAsyncInstantiation);
#endif
}
```

- [ ] **Step 2: Run and verify compilation failure**

```bash
/opt/homebrew/bin/cmake --build --preset macos-arm64-debug --target Pedalboard3_Tests
```

- [ ] **Step 3: Implement identity and scan-plan construction**

Build the exact key from `pluginFormatName + "|" + createIdentifierString()`. Map `AudioUnit` to `AU` and preserve other known labels. Iterate manager registration order, skip Internal, use `getDefaultLocationsToSearch()`, create per-format dead-man names, and set `allowAsyncInstantiation` only for AudioUnit.

- [ ] **Step 4: Pass the async flag into JUCE scanning**

Extend `SafePluginScanner` construction and call:

```cpp
baseScanner = std::make_unique<juce::PluginDirectoryScanner>(
    listToAddTo, formatToScan, directoriesToSearch, searchRecursively,
    deadMansPedalFile, allowPluginsWhichRequireAsyncInstantiation);
```

- [ ] **Step 5: Run tests and commit**

```bash
/opt/homebrew/bin/cmake --build --preset macos-arm64-debug --target Pedalboard3_Tests
/opt/homebrew/bin/ctest --preset macos-arm64-debug -R 'scan plan|exact identity' --output-on-failure
git diff --check
git add CMakeLists.txt tests/CMakeLists.txt tests/plugin_scan_coordinator_test.cpp src/PluginFormatIdentity.h src/PluginFormatIdentity.cpp src/PluginScanCoordinator.h src/PluginScanCoordinator.cpp src/SafePluginScanner.h src/SafePluginScanner.cpp
git commit -m "feat: plan VST3 and Audio Unit scans by format"
```

---

### Task 4: Add Atomic Persistence and Format-Scoped Quarantine

**Files:**
- Create: `src/AtomicFileWriter.h/.cpp`, `src/PluginQuarantine.h/.cpp`, `tests/plugin_quarantine_test.cpp`
- Modify: `src/SettingsManager.cpp`, `src/PluginBlacklist.h/.cpp`, `CMakeLists.txt`, `tests/CMakeLists.txt`

**Interfaces:**
- Produces: `bool replaceTextAtomically(const juce::File&, juce::StringRef);`
- Produces: `PluginQuarantineReason { crash, timeout, malformedResponse, loadFailure };`
- Produces: `PluginQuarantineEntry { formatName, identifier, reason, timestamp, detail };`
- Produces: `PluginQuarantine::contains/record/remove/clear/entries`.

- [ ] **Step 1: Write failing persistence tests**

```cpp
TEST_CASE("quarantine is scoped by format and survives reload", "[plugins][quarantine]")
{
    juce::TemporaryFile temp;
    auto file = temp.getFile();
    {
        PluginQuarantine quarantine(file);
        quarantine.record({"AudioUnit", "vendor.twin", PluginQuarantineReason::timeout,
                           juce::Time::getCurrentTime(), "30 second timeout"});
        REQUIRE(quarantine.contains("AudioUnit", "vendor.twin"));
        REQUIRE_FALSE(quarantine.contains("VST3", "vendor.twin"));
    }
    REQUIRE(PluginQuarantine(file).contains("AudioUnit", "vendor.twin"));
}
```

Add an atomic-writer test verifying a pre-existing target remains unchanged when an injected test writer fails before replacement.

- [ ] **Step 2: Run and verify missing type failures**

```bash
/opt/homebrew/bin/cmake --build --preset macos-arm64-debug --target Pedalboard3_Tests
```

- [ ] **Step 3: Implement atomic replacement and quarantine XML**

Use `juce::TemporaryFile temporary(target)`, write the complete new content to `temporary.getFile()`, and call `overwriteTargetFileWithTemporary()`. Persist:

```xml
<PLUGIN_QUARANTINE version="1">
  <ENTRY format="AudioUnit" identifier="vendor.twin" reason="timeout"
         timestamp="..." detail="30 second timeout"/>
</PLUGIN_QUARANTINE>
```

Protect entries with a mutex and deduplicate by format plus identifier.

- [ ] **Step 4: Route settings through atomic writing**

Replace `SettingsManager::save()` direct writing with `replaceTextAtomically`. Keep `PluginBlacklist` as the manual load blocklist and add description-based helpers using `makeExactPluginKey`; do not merge quarantine into its legacy path set.

- [ ] **Step 5: Test and commit**

```bash
/opt/homebrew/bin/cmake --build --preset macos-arm64-debug --target Pedalboard3_Tests
/opt/homebrew/bin/ctest --preset macos-arm64-debug -R 'quarantine|atomic replacement' --output-on-failure
git diff --check
git add CMakeLists.txt tests/CMakeLists.txt tests/plugin_quarantine_test.cpp src/AtomicFileWriter.h src/AtomicFileWriter.cpp src/PluginQuarantine.h src/PluginQuarantine.cpp src/PluginBlacklist.h src/PluginBlacklist.cpp src/SettingsManager.cpp
git commit -m "feat: persist format-scoped plugin quarantine atomically"
```

---

### Task 5: Add Authenticated macOS Scanner Transport

**Files:**
- Create: `src/PluginScannerTransport.h/.cpp`, `tests/plugin_scanner_transport_test.cpp`
- Modify: `src/PluginScannerIPC.h`, `src/PluginScannerClient.h/.cpp`, `src/scanner/PluginScannerMain.cpp`, `CMakeLists.txt`, `tests/CMakeLists.txt`

**Interfaces:**
- Produces: `ScannerLaunchInfo { endpoint, sessionToken };`
- Produces: `ScannerChannel::send/receive`.
- Produces: `ScannerHostEndpoint::create/launchInfo/accept`.
- Produces: `connectScannerChannel(launchInfo, timeoutMs)`.

- [ ] **Step 1: Write failing framing and transport tests**

Reject wrong magic, wrong protocol version, truncated frames, and payloads over 16 MiB. On macOS, create an endpoint, connect from a thread, exchange `Ready` and `Pong`, then confirm endpoint destruction unlinks the socket. Add a handshake test that rejects a wrong session token.

- [ ] **Step 2: Run and verify missing transport failures**

```bash
/opt/homebrew/bin/cmake --build --preset macos-arm64-debug --target Pedalboard3_Tests
```

- [ ] **Step 3: Implement protocol validation**

Increment `PROTOCOL_VERSION` to 2. Add JSON `Handshake { protocolVersion, sessionToken }`. Validate header fields and size before payload allocation.

- [ ] **Step 4: Implement private local endpoints**

On macOS create `/tmp/pb3-<128-bit-random>.sock`, restrict bind with `umask(0077)`, call `chmod(path, 0600)`, and use `poll` for bounded accept/read/write loops. The endpoint owns and unlinks its path. Keep Windows named pipes behind the same interface and randomize their suffix.

- [ ] **Step 5: Launch and authenticate the helper**

Store the child as `juce::ChildProcess` and pass:

```text
--scanner-endpoint <endpoint> --scanner-token <sessionToken>
```

The helper sends `Ready` containing the token. The host rejects a mismatch and terminates that child.

- [ ] **Step 6: Package and verify the helper**

Build the scanner on `WIN32 OR APPLE`. Copy it to `Pedalboard3.app/Contents/Helpers/Pedalboard3Scanner` on macOS and beside the executable on Windows.

```bash
/opt/homebrew/bin/cmake --preset macos-arm64-debug
/opt/homebrew/bin/cmake --build --preset macos-arm64-debug --target Pedalboard3 Pedalboard3Scanner Pedalboard3_Tests
/opt/homebrew/bin/ctest --preset macos-arm64-debug -R 'scanner transport|scanner protocol' --output-on-failure
test -x build/macos-arm64-debug/Pedalboard3_artefacts/Debug/Pedalboard3.app/Contents/Helpers/Pedalboard3Scanner
git diff --check
git add CMakeLists.txt tests/CMakeLists.txt tests/plugin_scanner_transport_test.cpp src/PluginScannerTransport.h src/PluginScannerTransport.cpp src/PluginScannerIPC.h src/PluginScannerClient.h src/PluginScannerClient.cpp src/scanner/PluginScannerMain.cpp
git commit -m "feat: add authenticated macOS plugin scanner transport"
```

---

### Task 6: Discover VST3, AUv2, and AUv3 Safely

**Files:**
- Modify: `src/scanner/PluginScannerMain.cpp`, `src/PluginScannerClient.h/.cpp`, `src/SafePluginScanner.h/.cpp`, `src/PluginScanCoordinator.h/.cpp`, `src/MainPanel.cpp`
- Modify: `tests/plugin_scan_coordinator_test.cpp`, `tests/rt_hosting_sprint_test.cpp`

**Interfaces:**
- Produces: `PluginScanProgress { formatName, candidate, formatProgress, overallProgress };`
- Produces: `PluginScanSummary { discovered, skipped, quarantined, failed, cancelled };`
- Produces: coordinator listener callbacks delivered on the message thread.

- [ ] **Step 1: Add failing coordinator and no-rescan tests**

Use fake formats and transport to assert VST3-before-AudioUnit order, overall progress, cancellation between candidates, quarantine continuation, and preserved completed results. Prove the helper-success path calls `skipNextFile()` and never calls an in-process `scanNextFile()` for that candidate. Also assert an AudioUnit job with zero registry identifiers succeeds with zero failures, and a release-mode job reports `isolationUnavailable` instead of silently scanning in process when the helper cannot launch.

- [ ] **Step 2: Run and verify failures**

```bash
/opt/homebrew/bin/cmake --build --preset macos-arm64-debug --target Pedalboard3_Tests
/opt/homebrew/bin/ctest --preset macos-arm64-debug -R 'scan coordinator|out-of-process scan advances' --output-on-failure
```

- [ ] **Step 3: Move helper discovery off the message thread**

Give the scanner one-worker `juce::ThreadPool`. Enqueue `findAllTypesForFile` and place the response in a locked queue. A message-thread timer drains responses to IPC. Reject a second request while one is active. This leaves the helper message thread free for AUv3 callbacks.

- [ ] **Step 4: Correct safe scanning and quarantine**

On helper success, add descriptions then:

```cpp
return baseScanner->skipNextFile();
```

On crash, timeout, malformed response, or failed load, record the exact format/candidate, restart the helper for the next candidate, and skip the failed candidate without host-process loading.

- [ ] **Step 5: Implement sequential coordination and UI delegation**

Make `PluginScanCoordinator` own its worker thread and one `SafePluginScanner` at a time. Calculate overall progress as `(jobIndex + formatProgress) / jobCount`. Stop after the active candidate resolves on cancellation. Replace `SafePluginListComponent` thread ownership with coordinator start/cancel/listener calls and show format plus summary counts. On `isolationUnavailable`, stop a release scan and show `Safe scanner unavailable`; permit in-process fallback only in an explicit diagnostic test/development mode.

- [ ] **Step 6: Test and commit**

```bash
/opt/homebrew/bin/cmake --build --preset macos-arm64-debug --target Pedalboard3 Pedalboard3Scanner Pedalboard3_Tests
/opt/homebrew/bin/ctest --preset macos-arm64-debug -R 'scan coordinator|rt.hosting|scanner' --output-on-failure
git diff --check
git add tests/plugin_scan_coordinator_test.cpp tests/rt_hosting_sprint_test.cpp src/scanner/PluginScannerMain.cpp src/PluginScannerClient.h src/PluginScannerClient.cpp src/SafePluginScanner.h src/SafePluginScanner.cpp src/PluginScanCoordinator.h src/PluginScanCoordinator.cpp src/MainPanel.cpp
git commit -m "feat: scan VST3 and Audio Units through the helper"
```

---

### Task 7: Add the Async Factory and Placeholder Processor

**Files:**
- Create: `src/PluginInstanceFactory.h/.cpp`, `src/PluginPlaceholderProcessor.h/.cpp`
- Create: `tests/plugin_instance_factory_test.cpp`, `tests/plugin_placeholder_test.cpp`
- Modify: `CMakeLists.txt`, `tests/CMakeLists.txt`

**Interfaces:**
- Produces: `PluginLoadPurpose { userInsert, patchRestore, rackRestore, retry };`
- Produces: `PluginLoadRequestId`, `PluginLoadResult { instance, error };`
- Produces: `IPluginInstanceFactory::createAsync/cancel` and `JucePluginInstanceFactory`.
- Produces: `PluginPlaceholderProcessor::State { loading, unresolved };`

- [ ] **Step 1: Write factory lifecycle tests**

Use a fake backend whose completion is manually released. Verify successful ownership transfer, cancellation suppression, and safe destruction:

```cpp
auto id = factory.createAsync(desc, 48000.0, 256, PluginLoadPurpose::userInsert,
                              [&called] (PluginLoadResult) { called = true; });
factory.cancel(id);
backend.completeSuccessfully(id);
REQUIRE_FALSE(called);
```

- [ ] **Step 2: Write placeholder tests**

Construct an unresolved two-in/two-out AU placeholder with state bytes. Assert exact description return, state round-trip, pass-through on common audio channels, clearing of extra outputs, MIDI preservation, and error retention.

- [ ] **Step 3: Run and verify missing type failures**

```bash
/opt/homebrew/bin/cmake --build --preset macos-arm64-debug --target Pedalboard3_Tests
```

- [ ] **Step 4: Implement the cancellable JUCE factory**

Keep request IDs in shared mutex-protected state and call:

```cpp
formatManager.createPluginInstanceAsync(description, sampleRate, blockSize,
    [state, requestId, completion = std::move(completion)]
    (std::unique_ptr<juce::AudioPluginInstance> instance, const juce::String& error)
    {
        if (!state->takeIfActive(requestId))
            return;
        completion({std::move(instance), error});
    });
```

Deliver the public completion on the JUCE message thread without waiting.

- [ ] **Step 5: Implement the placeholder**

Construct buses from the maximum description/connection-required counts. Return original description and opaque state unchanged. Loading state exposes no error; unresolved state exposes the failure. Keep MIDI enabled.

- [ ] **Step 6: Test and commit**

```bash
/opt/homebrew/bin/cmake --build --preset macos-arm64-debug --target Pedalboard3_Tests
/opt/homebrew/bin/ctest --preset macos-arm64-debug -R 'plugin instance factory|plugin placeholder' --output-on-failure
git diff --check
git add CMakeLists.txt tests/CMakeLists.txt tests/plugin_instance_factory_test.cpp tests/plugin_placeholder_test.cpp src/PluginInstanceFactory.h src/PluginInstanceFactory.cpp src/PluginPlaceholderProcessor.h src/PluginPlaceholderProcessor.cpp
git commit -m "feat: create external plugins asynchronously"
```

---

### Task 8: Make User Graph Insertion Async and Undoable

**Files:**
- Create: `tests/async_graph_loading_test.cpp`
- Modify: `src/IFilterGraph.h`, `src/FilterGraph.h/.cpp`, `src/UndoActions.h/.cpp`, `src/PluginField.h`, `src/PluginComponent.h/.cpp`, `CMakeLists.txt`, `tests/CMakeLists.txt`

**Interfaces:**
- Produces: `PluginNodeLoadEvent { nodeId, description, resolved, error };`
- Produces: `IFilterGraph::retryPluginNode(NodeID)`.
- Changes: existing `IFilterGraph::addFilterRaw(...)` returns a stable loading-node ID immediately for external formats.

- [ ] **Step 1: Write failing pending-node tests**

Inject a fake factory into `FilterGraph`. Assert external add immediately creates a loading placeholder, success replaces it under the same ID, failure makes it unresolved, removal before completion cancels, and undo/redo creates a new request generation.

- [ ] **Step 2: Run and verify missing APIs**

```bash
/opt/homebrew/bin/cmake --build --preset macos-arm64-debug --target Pedalboard3_Tests
```

- [ ] **Step 3: Insert loading nodes and track generations**

Keep synchronous creation for Internal/infrastructure only. For external descriptions, add a loading placeholder, assign x/y, start the factory, and store `{requestId, generation}` by node ID. Ignore obsolete completions.

- [ ] **Step 4: Replace nodes without losing identity/connections**

On completion, snapshot properties and connections, remove the placeholder under the callback lock, add the wrapped real instance or unresolved placeholder with the same node ID, restore properties, then restore connections.

- [ ] **Step 5: Update undo and node UI**

`AddPluginAction::perform()` succeeds after loading-node insertion; undo cancels then removes; redo starts a new generation. Fix `RemovePluginAction` connection remapping by retaining the old ID before assigning the replacement ID. Paint `Loading AU…` and an unresolved error strip with `Retry` in `PluginComponent`; error detail includes the short format badge, exact identifier, and factory error text.

- [ ] **Step 6: Test and commit**

```bash
/opt/homebrew/bin/cmake --build --preset macos-arm64-debug --target Pedalboard3 Pedalboard3_Tests
/opt/homebrew/bin/ctest --preset macos-arm64-debug -R 'async graph loading|filtergraph|undo' --output-on-failure
git diff --check
git add CMakeLists.txt tests/CMakeLists.txt tests/async_graph_loading_test.cpp src/IFilterGraph.h src/FilterGraph.h src/FilterGraph.cpp src/UndoActions.h src/UndoActions.cpp src/PluginField.h src/PluginComponent.h src/PluginComponent.cpp
git commit -m "feat: add plugins to the graph without blocking"
```

---

### Task 9: Restore Patches with Recoverable Missing Nodes

**Files:**
- Modify: `tests/async_graph_loading_test.cpp`, `tests/patch_switch_test.cpp`
- Modify: `src/PluginInstanceFactory.h`, `src/FilterGraph.h/.cpp`, `src/PluginField.h`, `src/PluginFieldPersistence.cpp`

**Interfaces:**
- Produces in `PluginInstanceFactory.h`: `GraphRestoreSummary { requested, live, unresolved, failedConnections };`
- Produces: `FilterGraph::restoreFromXmlAsync(xml, oscManager, completion)`.
- Produces: exact `retryPluginNode(NodeID)`.

- [ ] **Step 1: Write failing strict-identity and round-trip tests**

Create same-named AU/VST3 nodes, complete fake callbacks in reverse order, and assert stable IDs, exact requested formats, restored connections, and one completion. Fail one AU, save again, and assert unchanged format, identifier, state, UID, position, and connections.

- [ ] **Step 2: Run and confirm synchronous restore fails the contract**

```bash
/opt/homebrew/bin/cmake --build --preset macos-arm64-debug --target Pedalboard3_Tests
/opt/homebrew/bin/ctest --preset macos-arm64-debug -R 'strict format restore|unresolved placeholder' --output-on-failure
```

- [ ] **Step 3: Prepare stable nodes before async loads**

Parse all descriptions/states and derive required channel counts from connections. Create internal processors and external placeholders under saved IDs, add saved connections, release the lock, then start external requests.

- [ ] **Step 4: Complete once per restore generation**

Track generation and pending count. Replace each placeholder on callback and invoke `RestoreCompletion(GraphRestoreSummary)` once at zero. A newer patch cancels the older generation and suppresses its completion.

- [ ] **Step 5: Preserve XML and defer patch UI completion**

When serializing a placeholder, write original description/state plus optional `unresolved` and `unresolvedError` attributes. Split `PluginField::loadPatch` so component rebuild, user names, connection components, and fade-in occur from the async completion guarded by a safe pointer and generation.

- [ ] **Step 6: Implement exact retry and commit**

Retry only the placeholder's stored description/state; never search by name.

```bash
/opt/homebrew/bin/cmake --build --preset macos-arm64-debug --target Pedalboard3 Pedalboard3_Tests
/opt/homebrew/bin/ctest --preset macos-arm64-debug -R 'async graph loading|patch.switch|strict format restore|unresolved placeholder' --output-on-failure
git diff --check
git add tests/async_graph_loading_test.cpp tests/patch_switch_test.cpp src/PluginInstanceFactory.h src/FilterGraph.h src/FilterGraph.cpp src/PluginField.h src/PluginFieldPersistence.cpp
git commit -m "feat: preserve unavailable plugins during patch restore"
```

---

### Task 10: Apply Async Loading Inside Effect Racks

**Files:**
- Modify: `tests/subgraph_test.cpp`
- Modify: `src/SubGraphFilterGraph.h/.cpp`, `src/SubGraphProcessor.h/.cpp`, `src/FilterGraph.cpp`

**Interfaces:**
- Produces: `SubGraphProcessor::restoreStateAsync(data, size, completion)`.
- Produces: `SubGraphProcessor::restoreFromRackXmlAsync(xml, completion)`.
- Consumes: factory, placeholder, and restore-summary interfaces.

- [ ] **Step 1: Write failing rack lifecycle tests**

Use rack XML containing an AU, state, and fixed-I/O connections. Assert no synchronous creation, stable async replacement, unresolved round-trip, and top-level completion waiting for nested completion.

- [ ] **Step 2: Run and verify failure**

```bash
/opt/homebrew/bin/cmake --build --preset macos-arm64-debug --target Pedalboard3_Tests
/opt/homebrew/bin/ctest --preset macos-arm64-debug -R 'subgraph.*async|rack.*AudioUnit' --output-on-failure
```

- [ ] **Step 3: Convert rack insertion/restoration**

Give `SubGraphFilterGraph` the same factory/generation lifecycle. Keep fixed I/O synchronous, insert external placeholders under stable rack IDs, restore connections, and replace on completion.

- [ ] **Step 4: Make nested state completion-aware**

Implement both async restore methods. Generic `setStateInformation` calls async restore with a no-op completion. During top-level restoration, `FilterGraph` detects `SubGraphProcessor`, calls `restoreStateAsync` explicitly, and includes its pending work before final completion.

- [ ] **Step 5: Test and commit**

```bash
/opt/homebrew/bin/cmake --build --preset macos-arm64-debug --target Pedalboard3 Pedalboard3_Tests
/opt/homebrew/bin/ctest --preset macos-arm64-debug -R 'subgraph|rack|patch.switch|async graph loading' --output-on-failure
git diff --check
git add tests/subgraph_test.cpp src/SubGraphFilterGraph.h src/SubGraphFilterGraph.cpp src/SubGraphProcessor.h src/SubGraphProcessor.cpp src/FilterGraph.cpp
git commit -m "feat: restore Audio Units asynchronously inside racks"
```

---

### Task 11: Finish Badges, Quarantine Controls, and AU Pool Guard

**Files:**
- Modify: `tests/plugin_search_logic_test.cpp`, `tests/plugin_pool_manager_test.cpp`, `tests/ui_regression_harness_test.cpp`
- Modify: `src/PluginSearchLogic.h`, `src/PluginSearchOverlay.cpp`, `src/SafePluginScanner.h/.cpp`, `src/BlacklistWindow.h/.cpp`, `src/PluginPoolManager.cpp`

**Interfaces:**
- Consumes: `pluginFormatBadge`, quarantine CRUD, and scan summary.
- Produces: explicit AU badges and retry/clear quarantine management.

- [ ] **Step 1: Write failing UI and pool contracts**

Assert same-named AU/VST3 search entries remain distinct and AudioUnit badge is `AU`. Feed AUv2-like and AUv3-like descriptions through `extractPluginsFromPatchForTest` and assert neither is returned for prewarming; do not expose the anonymous `shouldPoolPlugin` helper solely for testing. Add source/UI contracts for scan totals and quarantine format, identifier, reason, timestamp, and detail.

- [ ] **Step 2: Run and verify failures**

```bash
/opt/homebrew/bin/cmake --build --preset macos-arm64-debug --target Pedalboard3_Tests
/opt/homebrew/bin/ctest --preset macos-arm64-debug -R 'plugin search|plugin pool|quarantine UI' --output-on-failure
```

- [ ] **Step 3: Render format identity**

Use `pluginFormatBadge` in search and scanner tables, add a distinct AU colour, and keep callbacks keyed to original plugin-list index.

- [ ] **Step 4: Add quarantine management**

Change the blacklist window to tabs `Manual Blacklist` and `Scan Quarantine`. The second tab shows all structured fields. `Retry Selected` removes only that exact record; `Clear Quarantine` leaves manual blacklist entries untouched. Add `View Quarantine` and final counts to the scanner UI.

- [ ] **Step 5: Lock the pool policy and commit**

Keep:

```cpp
return desc.pluginFormatName != "Internal"
    && desc.pluginFormatName != "AudioUnit";
```

```bash
/opt/homebrew/bin/cmake --build --preset macos-arm64-debug --target Pedalboard3 Pedalboard3_Tests
/opt/homebrew/bin/ctest --preset macos-arm64-debug -R 'plugin search|plugin pool|quarantine UI|ui.regression' --output-on-failure
git diff --check
git add tests/plugin_search_logic_test.cpp tests/plugin_pool_manager_test.cpp tests/ui_regression_harness_test.cpp src/PluginSearchLogic.h src/PluginSearchOverlay.cpp src/SafePluginScanner.h src/SafePluginScanner.cpp src/BlacklistWindow.h src/BlacklistWindow.cpp src/PluginPoolManager.cpp
git commit -m "feat: expose Audio Unit identity and scan recovery"
```

---

### Task 12: Validate Real AUv2/AUv3 Lifecycles and Document the Beta

**Files:**
- Create: `tests/au_host_integration_test.cpp`, `docs/qa/2026-08-12-au-beta-validation.md`
- Modify: `tests/CMakeLists.txt`, `documentation/plugins.html`, `documentation/troubleshooting.htm`

**Interfaces:**
- Consumes: all prior interfaces.
- Produces: repeatable automated/manual beta evidence with exact AU identifiers.

- [ ] **Step 1: Add environment-selected Audio Unit integration tests**

Read `PEDALBOARD3_TEST_AUV2_IDENTIFIER` and `PEDALBOARD3_TEST_AUV3_IDENTIFIER`. If absent, explicitly skip that section. If present, scan each exact identifier through `PluginScannerClient`, require the helper result to match, then test asynchronous creation, editor availability, state round-trip, and processing:

```cpp
REQUIRE(description.pluginFormatName == "AudioUnit");
REQUIRE(description.fileOrIdentifier == requestedIdentifier);
```

For AUv2 assert `requiresUnblockedMessageThreadDuringCreation(description)` is false. For AUv3 assert it is true and prove a repeating message-thread timer advances during pending creation.

- [ ] **Step 2: Run once without identifiers**

```bash
/opt/homebrew/bin/cmake --build --preset macos-arm64-debug --target Pedalboard3_Tests
/opt/homebrew/bin/ctest --preset macos-arm64-debug -R 'AU host integration' --output-on-failure
```

Expected: pass with explicit skip messages, not a guessed local plugin.

- [ ] **Step 3: Select and record real plugins**

Copy one AUv2 and one AUv3 exact identifier from the completed scanner. Across them require at least one effect and one instrument. Record name, maker, version, identifier, AU generation, and type.

- [ ] **Step 4: Run the real-plugin automated matrix**

```bash
PEDALBOARD3_TEST_AUV2_IDENTIFIER='<exact AUv2 identifier>' \
PEDALBOARD3_TEST_AUV3_IDENTIFIER='<exact AUv3 identifier>' \
/opt/homebrew/bin/ctest --preset macos-arm64-debug -R 'AU host integration' --output-on-failure
```

Expected: exact discovery, async creation, state round-trip, responsive AUv3 message timer, effect audio, and instrument MIDI-to-audio.

- [ ] **Step 5: Run bundled release smoke testing**

```bash
/opt/homebrew/bin/cmake --preset macos-arm64-release
/opt/homebrew/bin/cmake --build --preset macos-arm64-release --target Pedalboard3 Pedalboard3Scanner Pedalboard3_Tests
codesign --force --deep --sign - build/macos-arm64-release/Pedalboard3_artefacts/Release/Pedalboard3.app
codesign --verify --deep --strict build/macos-arm64-release/Pedalboard3_artefacts/Release/Pedalboard3.app
open build/macos-arm64-release/Pedalboard3_artefacts/Release/Pedalboard3.app
```

For each AU record scan, badge, insertion, editor reopen, audio/MIDI, bypass, reconnect, remove, non-default state save, restart, exact-format restore, and state restore. Repeat with one VST3 effect and instrument.

- [ ] **Step 6: Exercise recovery**

Terminate the helper during a disposable scan and confirm host survival, quarantine, helper restart, and continued scanning. Confirm cancellation stops after the active candidate, empty AU discovery succeeds, and retry/clear affects only one exact format identifier.

- [ ] **Step 7: Run full macOS and Windows gates**

```bash
/opt/homebrew/bin/ctest --preset macos-arm64-debug --output-on-failure
/opt/homebrew/bin/ctest --preset macos-arm64-release --output-on-failure
git diff --check
```

On Windows:

```powershell
cmake --preset msvc-release
cmake --build --preset msvc-release --target Pedalboard3 Pedalboard3Scanner Pedalboard3_Tests
ctest --preset msvc-release -R "formats|scanner|vst3|patch|plugin pool" --output-on-failure
```

Expected: AU absent on Windows and VST3 tests pass.

- [ ] **Step 8: Update docs, record evidence, and commit**

Document supported formats, badges, scan/cancel, quarantine, unresolved nodes, and strict restoration. Put exact commands, results, and plugin matrix in the QA file.

```bash
git add tests/CMakeLists.txt tests/au_host_integration_test.cpp documentation/plugins.html documentation/troubleshooting.htm docs/qa/2026-08-12-au-beta-validation.md
git commit -m "test: validate AUv2 and AUv3 hosting beta"
```

---

## Final Verification Gate

Before calling the beta complete, invoke `superpowers:verification-before-completion` and rerun the current branch's build/test commands. Confirm:

```bash
git log --oneline f7bbbc4..HEAD
git status --short
git diff --check f7bbbc4..HEAD
git submodule status JUCE
```

The branch must contain focused commits for replay evidence, registration, scan planning, quarantine, scanner transport, safe discovery, async creation, graph insertion, patch restoration, rack restoration, UI recovery, and validation. JUCE must remain pinned to `f8f8864` and unrelated user changes must remain untouched.
