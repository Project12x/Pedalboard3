# Pedalboard3 Native Apple Silicon Bring-Up Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Produce a Debug and Release Apple Silicon Pedalboard3 application bundle that launches on the development Mac, uses the normal JUCE CoreAudio/CoreMIDI path, stores data and resources in native macOS locations, and has a repeatable startup smoke test.

**Architecture:** Keep the existing JUCE application and CMake targets, but isolate Windows-only definitions, remove two obsolete Apple compatibility branches, and centralize platform paths. The first milestone intentionally uses JUCE's in-process plugin scan fallback on macOS; VST3 identity portability and the shared coordinator/worker scanner are separate follow-on plans.

**Tech Stack:** C++17, C++20 for the existing NAM A2 targets, JUCE 9, CMake 3.22+, Ninja, Catch2, Apple Clang, CoreAudio, CoreMIDI, Homebrew.

## Global Constraints

- Initial target is Apple Silicon on the development Mac running macOS 26.6.1.
- Do not set a macOS deployment target in this milestone; macOS 13 Ventura compatibility is a separate tested milestone.
- Build only `arm64`; do not add Intel or universal binary settings.
- Keep VST3 hosting enabled, but do not add Audio Unit registration in this milestone.
- Do not add signing, notarization, hardened-runtime settings, a DMG, or CI distribution.
- Preserve Windows behavior by keeping ASIO, WASAPI, DirectSound, Win32 DPI, `NOMINMAX`, and `ws2_32` inside `WIN32` conditions.
- Do not copy or enable the Windows-only scanner helper on macOS; absence of the helper deliberately selects the existing in-process fallback until the shared scanner plan.
- Do not change patch identity or patch commit behavior in this milestone.
- Keep changes on `codex/macos-port` and make the focused commits listed below.

## Plan Decomposition

The approved design covers independently reviewable subsystems. This document implements only the native Apple Silicon baseline. After this plan passes, create separate implementation plans in this order:

1. VST3 format registration, cross-platform identity resolution, and transactional patch restore.
2. Cross-platform coordinator/worker scanner transport through `KnownPluginList::CustomScanner`.
3. Live-host beta validation and one-hour soak coverage.
4. Audio Unit hosting.
5. macOS 13 deployment and runtime compatibility.

## File Structure

### Files created by this plan

- `src/PlatformPaths.h` — declares deterministic and runtime platform path helpers.
- `src/PlatformPaths.cpp` — implements user-data and documentation path policies.
- `src/StartupOptions.h` — declares parsing for deterministic startup behavior.
- `src/StartupOptions.cpp` — parses `--startup-smoke-test`.
- `tests/macos_build_config_test.cpp` — protects CMake and legacy Apple cleanup contracts.
- `tests/platform_paths_test.cpp` — tests platform path policy without using the real home directory.
- `tests/startup_options_test.cpp` — tests startup option parsing.
- `documentation/research/2026-08-12-macos-native-bring-up.md` — records verified commands, artifacts, and known milestone deferrals.

### Files modified by this plan

- `CMakePresets.json` — adds Apple Silicon Debug, Release, build, and test presets.
- `CMakeLists.txt` — isolates platform definitions, adds privacy metadata and new sources, gates the old scanner target to Windows, and copies documentation into the app bundle.
- `tests/CMakeLists.txt` — compiles the new tests and path/startup implementation units.
- `src/PluginComponent.h` — uses `std::shared_ptr` directly.
- `src/PluginComponent.cpp` — qualifies cached preset smart-pointer uses with `std::`.
- `src/c0xHeaders.h` — deleted because the project already requires C++17.
- `src/PluginFieldPersistence.cpp` — uses `AudioProcessorGraph::Node::Ptr` on every platform.
- `src/SettingsManager.cpp` — delegates its root directory to `PlatformPaths`.
- `src/JuceHelperStuff.cpp` — delegates legacy app-data lookup to `PlatformPaths`.
- `src/LogFile.cpp` — writes event logs below the shared user-data root.
- `src/PresetManager.cpp` — writes plugin presets below the shared user-data root.
- `src/UserPresetWindow.h` — enumerates plugin preset directories below the shared root.
- `src/UserPresetWindow.cpp` — imports plugin presets below the shared root.
- `src/ScratchRecorder.cpp` — derives its default scratch root from the shared root.
- `src/ScratchTake.cpp` — derives take-storage defaults from the shared root.
- `src/MainPanel.cpp` — delegates help-documentation lookup to `PlatformPaths`.
- `src/App.cpp` — writes the primary log below the shared root and schedules a clean exit for startup smoke runs.
- `readme.md` — documents the verified macOS development build and smoke commands.

---

### Task 1: Install the Build Tools and Add Apple Silicon Presets

**Files:**
- Modify: `CMakePresets.json`

**Interfaces:**
- Consumes: Apple Command Line Tools at `/Library/Developer/CommandLineTools` and the current `base` CMake preset.
- Produces: configure/build/test presets named `macos-arm64-debug` and `macos-arm64-release`.

- [ ] **Step 1: Verify the existing Apple compiler and SDK**

Run:

```bash
uname -m
xcrun --show-sdk-path
clang++ --version
```

Expected: `uname` prints `arm64`, `xcrun` prints an SDK below `/Library/Developer/CommandLineTools/SDKs`, and Apple Clang reports an `arm64-apple-darwin` target.

- [ ] **Step 2: Install Homebrew from its official installer**

Run:

```bash
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
eval "$(/opt/homebrew/bin/brew shellenv)"
```

Expected: `/opt/homebrew/bin/brew --version` succeeds. The installer may request normal macOS administrator approval.

- [ ] **Step 3: Install and verify CMake and Ninja**

Run:

```bash
/opt/homebrew/bin/brew install cmake ninja
/opt/homebrew/bin/cmake --version
/opt/homebrew/bin/ninja --version
```

Expected: CMake is at least 3.22 and Ninja prints a version number.

- [ ] **Step 4: Add Apple Silicon configure presets**

Add these objects to `configurePresets` after `relwithdebinfo`:

```json
{
  "name": "macos-arm64-base",
  "hidden": true,
  "inherits": "base",
  "condition": {
    "type": "equals",
    "lhs": "${hostSystemName}",
    "rhs": "Darwin"
  },
  "cacheVariables": {
    "CMAKE_OSX_ARCHITECTURES": "arm64",
    "Pedalboard3_BUILD_TESTS": "ON"
  }
},
{
  "name": "macos-arm64-debug",
  "displayName": "macOS Apple Silicon Debug",
  "inherits": "macos-arm64-base",
  "cacheVariables": {
    "CMAKE_BUILD_TYPE": "Debug"
  }
},
{
  "name": "macos-arm64-release",
  "displayName": "macOS Apple Silicon Release",
  "inherits": "macos-arm64-base",
  "cacheVariables": {
    "CMAKE_BUILD_TYPE": "Release"
  }
}
```

- [ ] **Step 5: Add matching build and test presets**

Add to `buildPresets`:

```json
{
  "name": "macos-arm64-debug",
  "configurePreset": "macos-arm64-debug"
},
{
  "name": "macos-arm64-release",
  "configurePreset": "macos-arm64-release"
}
```

Add to `testPresets`:

```json
{
  "name": "macos-arm64-debug",
  "configurePreset": "macos-arm64-debug",
  "output": {
    "outputOnFailure": true
  }
},
{
  "name": "macos-arm64-release",
  "configurePreset": "macos-arm64-release",
  "output": {
    "outputOnFailure": true
  }
}
```

- [ ] **Step 6: Configure the Debug tree**

Run:

```bash
eval "$(/opt/homebrew/bin/brew shellenv)"
cmake --preset macos-arm64-debug
```

Expected: configuration completes and writes `build/macos-arm64-debug/compile_commands.json`. CPM may download missing dependencies during this first configure.

- [ ] **Step 7: Commit the presets**

```bash
git add CMakePresets.json
git commit -m "build: add Apple Silicon CMake presets"
```

---

### Task 2: Make the Existing Application Target Build Natively

**Files:**
- Create: `tests/macos_build_config_test.cpp`
- Modify: `tests/CMakeLists.txt`
- Modify: `CMakeLists.txt`
- Modify: `src/PluginComponent.h`
- Modify: `src/PluginComponent.cpp`
- Modify: `src/PluginFieldPersistence.cpp`
- Delete: `src/c0xHeaders.h`

**Interfaces:**
- Consumes: `macos-arm64-debug` from Task 1.
- Produces: a buildable `Pedalboard3` Apple bundle target and source-contract test case `macOS build configuration isolates platform code`.

- [ ] **Step 1: Run the application build to capture the red baseline**

Run:

```bash
cmake --build --preset macos-arm64-debug --target Pedalboard3 -j 4
```

Expected: FAIL on the obsolete Apple Boost compatibility include (`boost/pointer_cast.hpp`) or an earlier Windows-only configuration problem. Save the first compiler error in the task notes.

- [ ] **Step 2: Write the failing build-contract test**

Create `tests/macos_build_config_test.cpp`:

```cpp
#include <catch2/catch_test_macros.hpp>

#include <fstream>
#include <iterator>
#include <string>

namespace
{
std::string readSource(const char* relativePath)
{
    std::ifstream input(std::string(PEDALBOARD3_SOURCE_DIR) + "/" + relativePath);
    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}
}

TEST_CASE("macOS build configuration isolates platform code", "[macos][build]")
{
    const auto cmake = readSource("CMakeLists.txt");
    const auto pluginHeader = readSource("src/PluginComponent.h");
    const auto persistence = readSource("src/PluginFieldPersistence.cpp");
    const auto compileCommands = readSource("build/macos-arm64-debug/compile_commands.json");

    REQUIRE_FALSE(cmake.empty());
    REQUIRE_FALSE(compileCommands.empty());
    CHECK(cmake.find("MICROPHONE_PERMISSION_ENABLED TRUE") != std::string::npos);
    CHECK(cmake.find("if(WIN32)\n    juce_add_console_app(Pedalboard3Scanner") != std::string::npos);
    CHECK(pluginHeader.find("c0xHeaders.h") == std::string::npos);
    CHECK(pluginHeader.find("std::shared_ptr<MemoryBlock>") != std::string::npos);
    CHECK(persistence.find("#ifdef __APPLE__\nstruct NodeAndId") == std::string::npos);
    CHECK(persistence.find("AudioProcessorGraph::Node::Ptr node;") != std::string::npos);

#if JUCE_MAC
    CHECK(compileCommands.find("JUCE_ASIO=1") == std::string::npos);
    CHECK(compileCommands.find("JUCE_WASAPI=1") == std::string::npos);
    CHECK(compileCommands.find("JUCE_DIRECTSOUND=1") == std::string::npos);
    CHECK(compileCommands.find("JUCE_WIN_PER_MONITOR_DPI_AWARE=1") == std::string::npos);
#endif
}
```

Add `macos_build_config_test.cpp` to `Pedalboard3_Tests` in `tests/CMakeLists.txt`.

- [ ] **Step 3: Build and run the contract test to verify it fails**

Run:

```bash
cmake --build --preset macos-arm64-debug --target Pedalboard3_Tests -j 4
ctest --preset macos-arm64-debug -R "macOS build configuration isolates platform code"
```

Expected: FAIL because the microphone metadata, scanner condition, standard smart pointer, and unified node type are absent.

- [ ] **Step 4: Isolate Windows definitions and add macOS privacy metadata**

In the `juce_add_gui_app(Pedalboard3 ...)` call, add:

```cmake
MICROPHONE_PERMISSION_ENABLED TRUE
MICROPHONE_PERMISSION_TEXT "Pedalboard3 uses audio input for live processing, recording, metering, and tuning."
```

Keep common definitions in the existing `target_compile_definitions(Pedalboard3 PRIVATE ...)` block, but remove these entries from it:

```cmake
JUCE_ASIO=1
JUCE_WASAPI=1
JUCE_DIRECTSOUND=1
JUCE_WIN_PER_MONITOR_DPI_AWARE=1
NOMINMAX
```

Add them to the existing `if(WIN32)` block:

```cmake
target_compile_definitions(Pedalboard3 PRIVATE
    JUCE_ASIO=1
    JUCE_WASAPI=1
    JUCE_DIRECTSOUND=1
    JUCE_WIN_PER_MONITOR_DPI_AWARE=1
    NOMINMAX
    _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING
)
```

- [ ] **Step 5: Gate the legacy scanner executable to Windows**

Insert `if(WIN32)` immediately before the current `juce_add_console_app(Pedalboard3Scanner` call. Indent the complete scanner target definition, source list, compile definitions, link libraries, and post-build copy by four spaces so the block begins exactly as follows:

```cmake
if(WIN32)
    juce_add_console_app(Pedalboard3Scanner
        PRODUCT_NAME "Pedalboard3Scanner"
        COMPANY_NAME "Pedalboard3 Project"
    )
```

Insert `endif()` immediately after the scanner's existing `add_custom_command` closing parenthesis. Do not change the scanner target's contents in this milestone. This prevents creation or copying of `Pedalboard3Scanner` on Apple, so `SafePluginScanner` will not find a helper and will use its existing in-process fallback.

- [ ] **Step 6: Replace the obsolete Boost compatibility shim**

In `src/PluginComponent.h`, replace:

```cpp
#include "c0xHeaders.h"
```

with:

```cpp
#include <memory>
```

Change the cached preset member to:

```cpp
std::map<int, std::shared_ptr<MemoryBlock>> cachedPresets;
```

In `src/PluginComponent.cpp`, change both unqualified `shared_ptr<MemoryBlock>` uses to `std::shared_ptr<MemoryBlock>`. Delete `src/c0xHeaders.h`; it has no remaining consumers.

- [ ] **Step 7: Use one graph node holder on every platform**

Remove the file-scope Apple-only `NodeAndId` struct from `src/PluginFieldPersistence.cpp`. Inside the connection-restoration block, replace the conditional struct with:

```cpp
struct NodeAndId
{
    AudioProcessorGraph::Node::Ptr node;
    uint32 id;
};
```

Keep the later `.node.get()` calls unchanged because `Node::Ptr` supports them on every platform.

- [ ] **Step 8: Reconfigure, build, and run the contract test**

Run:

```bash
cmake --preset macos-arm64-debug
cmake --build --preset macos-arm64-debug --target Pedalboard3 Pedalboard3_Tests -j 4
ctest --preset macos-arm64-debug -R "macOS build configuration isolates platform code"
```

Expected: the app and test targets build, and the contract test passes.

- [ ] **Step 9: Verify the generated bundle metadata**

Run:

```bash
plutil -extract CFBundleIdentifier raw build/macos-arm64-debug/Pedalboard3_artefacts/Debug/Pedalboard3.app/Contents/Info.plist
plutil -extract NSMicrophoneUsageDescription raw build/macos-arm64-debug/Pedalboard3_artefacts/Debug/Pedalboard3.app/Contents/Info.plist
file build/macos-arm64-debug/Pedalboard3_artefacts/Debug/Pedalboard3.app/Contents/MacOS/Pedalboard3
```

Expected: bundle ID is `com.pedalboard3.app`, microphone text matches Step 4, and `file` reports a Mach-O 64-bit arm64 executable.

- [ ] **Step 10: Commit the native build fixes**

```bash
git add CMakeLists.txt tests/CMakeLists.txt tests/macos_build_config_test.cpp src/PluginComponent.h src/PluginComponent.cpp src/PluginFieldPersistence.cpp src/c0xHeaders.h
git commit -m "build: make application target compile on Apple Silicon"
```

---

### Task 3: Centralize macOS Data and Bundle Resource Paths

**Files:**
- Create: `src/PlatformPaths.h`
- Create: `src/PlatformPaths.cpp`
- Create: `tests/platform_paths_test.cpp`
- Modify: `CMakeLists.txt`
- Modify: `tests/CMakeLists.txt`
- Modify: `src/SettingsManager.cpp`
- Modify: `src/JuceHelperStuff.cpp`
- Modify: `src/LogFile.cpp`
- Modify: `src/PresetManager.cpp`
- Modify: `src/UserPresetWindow.h`
- Modify: `src/UserPresetWindow.cpp`
- Modify: `src/ScratchRecorder.cpp`
- Modify: `src/ScratchTake.cpp`
- Modify: `src/MainPanel.cpp`
- Modify: `src/App.cpp`

**Interfaces:**
- Consumes: JUCE `File::userApplicationDataDirectory` and `File::currentApplicationFile`.
- Produces: `pedalboard3::platform::userDataDirectoryForBase(const juce::File&, bool)`, `getUserDataDirectory()`, `documentationDirectoryForApplication(const juce::File&, bool)`, and `getDocumentationDirectory()`.

- [ ] **Step 1: Write the failing platform path tests**

Create `tests/platform_paths_test.cpp`:

```cpp
#include "PlatformPaths.h"

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <fstream>
#include <iterator>
#include <string>

namespace
{
std::string readProjectSource(const char* relativePath)
{
    std::ifstream input(std::string(PEDALBOARD3_SOURCE_DIR) + "/" + relativePath);
    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}
}

TEST_CASE("macOS user data lives in Application Support", "[macos][paths]")
{
    const juce::File library("/Users/test/Library");
    const auto result = pedalboard3::platform::userDataDirectoryForBase(library, true);
    CHECK(result.getFullPathName() == "/Users/test/Library/Application Support/Pedalboard3");
}

TEST_CASE("non-macOS user data remains below the JUCE application-data root", "[paths]")
{
    const juce::File root("/var/test-app-data");
    const auto result = pedalboard3::platform::userDataDirectoryForBase(root, false);
    CHECK(result.getFullPathName() == "/var/test-app-data/Pedalboard3");
}

TEST_CASE("documentation resolves inside a macOS application bundle", "[macos][paths]")
{
    const juce::File app("/Applications/Pedalboard3.app");
    const auto result = pedalboard3::platform::documentationDirectoryForApplication(app, true);
    CHECK(result.getFullPathName() ==
          "/Applications/Pedalboard3.app/Contents/Resources/documentation");
}

TEST_CASE("documentation remains adjacent to non-bundle executables", "[paths]")
{
    const juce::File executable("/opt/Pedalboard3/Pedalboard3.exe");
    const auto result = pedalboard3::platform::documentationDirectoryForApplication(executable, false);
    CHECK(result.getFullPathName() == "/opt/Pedalboard3/documentation");
}

TEST_CASE("application data consumers use the shared path policy", "[macos][paths]")
{
    constexpr std::array<const char*, 9> consumers{
        "src/App.cpp",
        "src/JuceHelperStuff.cpp",
        "src/LogFile.cpp",
        "src/PresetManager.cpp",
        "src/UserPresetWindow.h",
        "src/UserPresetWindow.cpp",
        "src/ScratchRecorder.cpp",
        "src/ScratchTake.cpp",
        "src/SettingsManager.cpp"
    };

    for (const auto* path : consumers)
    {
        INFO(path);
        const auto source = readProjectSource(path);
        REQUIRE_FALSE(source.empty());
        CHECK(source.find("userApplicationDataDirectory") == std::string::npos);
        CHECK(source.find("PlatformPaths.h") != std::string::npos);
    }
}
```

Add `platform_paths_test.cpp` and `../src/PlatformPaths.cpp` to `Pedalboard3_Tests`, and add `src/PlatformPaths.cpp` plus `src/PlatformPaths.h` to the application target.

- [ ] **Step 2: Build the test target to verify the new tests fail**

Run:

```bash
cmake --preset macos-arm64-debug
cmake --build --preset macos-arm64-debug --target Pedalboard3_Tests -j 4
```

Expected: FAIL because `PlatformPaths.h` and its functions do not exist.

- [ ] **Step 3: Declare the platform path interface**

Create `src/PlatformPaths.h`:

```cpp
#pragma once

#include <JuceHeader.h>

namespace pedalboard3::platform
{
juce::File userDataDirectoryForBase(const juce::File& applicationDataBase, bool macOS);
juce::File getUserDataDirectory();

juce::File documentationDirectoryForApplication(const juce::File& currentApplication, bool macOSBundle);
juce::File getDocumentationDirectory();
}
```

- [ ] **Step 4: Implement deterministic and runtime paths**

Create `src/PlatformPaths.cpp`:

```cpp
#include "PlatformPaths.h"

namespace pedalboard3::platform
{
juce::File userDataDirectoryForBase(const juce::File& applicationDataBase, bool macOS)
{
    auto root = applicationDataBase;
    if (macOS)
        root = root.getChildFile("Application Support");
    return root.getChildFile("Pedalboard3");
}

juce::File getUserDataDirectory()
{
    const auto base = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory);
#if JUCE_MAC
    return userDataDirectoryForBase(base, true);
#else
    return userDataDirectoryForBase(base, false);
#endif
}

juce::File documentationDirectoryForApplication(const juce::File& currentApplication, bool macOSBundle)
{
    if (macOSBundle)
        return currentApplication.getChildFile("Contents").getChildFile("Resources").getChildFile("documentation");
    return currentApplication.getParentDirectory().getChildFile("documentation");
}

juce::File getDocumentationDirectory()
{
    const auto application = juce::File::getSpecialLocation(juce::File::currentApplicationFile);
#if JUCE_MAC
    return documentationDirectoryForApplication(application, true);
#else
    return documentationDirectoryForApplication(application, false);
#endif
}
}
```

- [ ] **Step 5: Route every current caller through the shared policy**

In `src/SettingsManager.cpp`, include `PlatformPaths.h` and implement:

```cpp
juce::File SettingsManager::getUserDataDirectory() const
{
    return pedalboard3::platform::getUserDataDirectory();
}
```

In `src/JuceHelperStuff.cpp`, include `PlatformPaths.h` and replace `getAppDataFolder()` with:

```cpp
File JuceHelperStuff::getAppDataFolder()
{
    auto directory = pedalboard3::platform::getUserDataDirectory();
    directory.createDirectory();
    return directory;
}
```

Include `PlatformPaths.h` in `src/App.cpp`, `src/LogFile.cpp`, `src/PresetManager.cpp`, `src/UserPresetWindow.h`, `src/UserPresetWindow.cpp`, `src/ScratchRecorder.cpp`, and `src/ScratchTake.cpp`. Replace every direct construction rooted at `File::userApplicationDataDirectory` with `pedalboard3::platform::getUserDataDirectory()`.

Use these resulting path expressions:

```cpp
// App.cpp
auto dataDirectory = pedalboard3::platform::getUserDataDirectory();
dataDirectory.createDirectory();
auto logPath = dataDirectory.getChildFile("debug.log").getFullPathName().toStdString();

// LogFile.cpp
auto logDirectory = pedalboard3::platform::getUserDataDirectory();
logDirectory.createDirectory();
logFile.reset(new FileOutputStream(logDirectory.getChildFile(tempstr)));

// PresetManager.cpp and UserPresetWindow files
auto presetDir = pedalboard3::platform::getUserDataDirectory().getChildFile("presets");

// ScratchRecorder.cpp and ScratchTake.cpp
return pedalboard3::platform::getUserDataDirectory().getChildFile("Scratch Ideas");
```

In `PresetManager::getPluginPresetDir`, return:

```cpp
return pedalboard3::platform::getUserDataDirectory()
    .getChildFile("presets")
    .getChildFile(pluginName);
```

In `src/MainPanel.cpp`, include `PlatformPaths.h` and replace the `WIN32`/`LINUX`/`__APPLE__` documentation path block with:

```cpp
const auto docDir = pedalboard3::platform::getDocumentationDirectory();
```

- [ ] **Step 6: Copy documentation to the native bundle resource directory**

Replace the unconditional documentation post-build command in `CMakeLists.txt` with:

```cmake
if(APPLE)
    add_custom_command(TARGET Pedalboard3 POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/documentation"
            "$<TARGET_BUNDLE_CONTENT_DIR:Pedalboard3>/Resources/documentation"
        VERBATIM
    )
else()
    add_custom_command(TARGET Pedalboard3 POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/documentation"
            "$<TARGET_FILE_DIR:Pedalboard3>/documentation"
        VERBATIM
    )
endif()
```

- [ ] **Step 7: Run the path tests and verify the copied help file**

Run:

```bash
cmake --preset macos-arm64-debug
cmake --build --preset macos-arm64-debug --target Pedalboard3 Pedalboard3_Tests -j 4
ctest --preset macos-arm64-debug -R "user data|documentation|application data consumers"
test -f build/macos-arm64-debug/Pedalboard3_artefacts/Debug/Pedalboard3.app/Contents/Resources/documentation/index.htm
```

Expected: all deterministic path and consumer-policy tests pass and `index.htm` exists inside the app bundle.

- [ ] **Step 8: Commit platform path handling**

```bash
git add CMakeLists.txt tests/CMakeLists.txt tests/platform_paths_test.cpp src/PlatformPaths.h src/PlatformPaths.cpp src/SettingsManager.cpp src/JuceHelperStuff.cpp src/LogFile.cpp src/PresetManager.cpp src/UserPresetWindow.h src/UserPresetWindow.cpp src/ScratchRecorder.cpp src/ScratchTake.cpp src/MainPanel.cpp src/App.cpp
git commit -m "feat: use native macOS data and resource paths"
```

---

### Task 4: Add a Repeatable Startup Smoke Test and Validate the Native Baseline

**Files:**
- Create: `src/StartupOptions.h`
- Create: `src/StartupOptions.cpp`
- Create: `tests/startup_options_test.cpp`
- Create: `documentation/research/2026-08-12-macos-native-bring-up.md`
- Modify: `CMakeLists.txt`
- Modify: `tests/CMakeLists.txt`
- Modify: `src/App.cpp`
- Modify: `readme.md`

**Interfaces:**
- Consumes: the native application bundle from Tasks 2 and 3.
- Produces: `pedalboard3::StartupOptions::parse(const juce::String&)` and the command-line flag `--startup-smoke-test`, which exits cleanly 1.5 seconds after initialization.

- [ ] **Step 1: Write the failing startup option tests**

Create `tests/startup_options_test.cpp`:

```cpp
#include "StartupOptions.h"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("startup smoke test option is explicit", "[startup][macos]")
{
    CHECK_FALSE(pedalboard3::StartupOptions::parse("").startupSmokeTest);
    CHECK_FALSE(pedalboard3::StartupOptions::parse("--debug").startupSmokeTest);
    CHECK(pedalboard3::StartupOptions::parse("--debug --startup-smoke-test").startupSmokeTest);
}
```

Add `startup_options_test.cpp` and `../src/StartupOptions.cpp` to the test target. Add `src/StartupOptions.cpp` and `src/StartupOptions.h` to the application target.

- [ ] **Step 2: Build the test target to verify it fails**

Run:

```bash
cmake --preset macos-arm64-debug
cmake --build --preset macos-arm64-debug --target Pedalboard3_Tests -j 4
```

Expected: FAIL because `StartupOptions.h` does not exist.

- [ ] **Step 3: Implement startup option parsing**

Create `src/StartupOptions.h`:

```cpp
#pragma once

#include <JuceHeader.h>

namespace pedalboard3
{
struct StartupOptions
{
    bool startupSmokeTest = false;

    static StartupOptions parse(const juce::String& commandLine);
};
}
```

Create `src/StartupOptions.cpp`:

```cpp
#include "StartupOptions.h"

namespace pedalboard3
{
StartupOptions StartupOptions::parse(const juce::String& commandLine)
{
    StartupOptions options;
    options.startupSmokeTest = commandLine.contains("--startup-smoke-test");
    return options;
}
}
```

- [ ] **Step 4: Schedule a clean smoke-test exit after application initialization**

Include `StartupOptions.h` in `src/App.cpp`. At the end of `App::initialise`, after macOS menu setup, add:

```cpp
if (pedalboard3::StartupOptions::parse(commandLine).startupSmokeTest)
{
    Timer::callAfterDelay(1500, [] { JUCEApplicationBase::quit(); });
}
```

The normal startup path is unchanged when the flag is absent.

- [ ] **Step 5: Run the unit and executable smoke tests**

Run:

```bash
cmake --preset macos-arm64-debug
cmake --build --preset macos-arm64-debug --target Pedalboard3 Pedalboard3_Tests -j 4
ctest --preset macos-arm64-debug -R "startup smoke test option is explicit"
build/macos-arm64-debug/Pedalboard3_artefacts/Debug/Pedalboard3.app/Contents/MacOS/Pedalboard3 --startup-smoke-test
```

Expected: the Catch2 test passes; the application creates its window and exits with status 0 after approximately 1.5 seconds.

- [ ] **Step 6: Run the full automated suite**

Run:

```bash
cmake --build --preset macos-arm64-debug --target Pedalboard3_Tests Pedalboard3_NAMCoreRuntimeTests -j 4
ctest --preset macos-arm64-debug
```

Expected: all platform-independent tests pass. A real-VST3 test may report a Catch2 warning and skip when no matching plugin is installed; a crash, failed assertion, or failed CTest entry is not acceptable.

- [ ] **Step 7: Perform the native CoreAudio/CoreMIDI smoke gate**

Run:

```bash
open build/macos-arm64-debug/Pedalboard3_artefacts/Debug/Pedalboard3.app
```

Verify in the running application:

1. The microphone permission explanation names live processing, recording, metering, and tuning.
2. The audio device dialog lists the attached CoreAudio device.
3. Input and output channel selections persist after closing and reopening the dialog.
4. The MIDI device list opens without an error; select an attached CoreMIDI device when one is available.
5. Add a Tone Generator, connect it to Audio Output, and hear output.
6. Add Audio Input and a meter or tuner node, and observe input activity after permission is granted.
7. Open Help Documentation and confirm the bundled page launches.
8. Quit normally and relaunch once.

Expected: all checks pass. Pause execution and report the exact failed item if any check cannot be completed.

- [ ] **Step 8: Document the verified build path and milestone boundary**

Create `documentation/research/2026-08-12-macos-native-bring-up.md` with:

```markdown
# macOS Native Bring-Up Record

## Verified Environment

- Architecture: Apple Silicon arm64
- Operating system: macOS 26.6.1
- Apple SDK: output recorded from `xcrun --show-sdk-path`
- Apple Clang: output recorded from `clang++ --version`
- CMake: output recorded from `cmake --version`
- Ninja: output recorded from `ninja --version`

## Verified Commands

- Configure: `cmake --preset macos-arm64-debug`
- Build: `cmake --build --preset macos-arm64-debug --target Pedalboard3 -j 4`
- Automated tests: `ctest --preset macos-arm64-debug`
- Startup smoke: `Pedalboard3.app/Contents/MacOS/Pedalboard3 --startup-smoke-test`

## Verified Artifact

`build/macos-arm64-debug/Pedalboard3_artefacts/Debug/Pedalboard3.app`

## Manual Smoke Results

The CoreAudio, CoreMIDI, tone-generator output, audio-input activity, bundled documentation, normal quit, and relaunch checks in the implementation plan passed on the verified environment.

## Deliberate Deferrals

- VST3 cross-platform patch resolution and transactional restore
- Cross-platform isolated scanner transport
- Audio Unit hosting
- Intel and universal binaries
- macOS 13 compatibility
- Signing, notarization, DMG packaging, and distribution CI
```

Update `readme.md` Build From Source with a macOS subsection containing the Homebrew prerequisites, `cmake --preset macos-arm64-debug`, `cmake --build --preset macos-arm64-debug`, the `.app` artifact path, and the startup smoke command.

- [ ] **Step 9: Build the Release bundle**

Run:

```bash
cmake --preset macos-arm64-release
cmake --build --preset macos-arm64-release --target Pedalboard3 -j 4
file build/macos-arm64-release/Pedalboard3_artefacts/Release/Pedalboard3.app/Contents/MacOS/Pedalboard3
```

Expected: Release build succeeds and `file` reports a Mach-O 64-bit arm64 executable.

- [ ] **Step 10: Commit the smoke harness and verified documentation**

```bash
git add CMakeLists.txt tests/CMakeLists.txt tests/startup_options_test.cpp src/StartupOptions.h src/StartupOptions.cpp src/App.cpp readme.md documentation/research/2026-08-12-macos-native-bring-up.md
git commit -m "test: verify native macOS application startup"
```

## Plan Completion Gate

Before declaring this plan complete, run:

```bash
git status --short
cmake --build --preset macos-arm64-debug --target Pedalboard3 Pedalboard3_Tests Pedalboard3_NAMCoreRuntimeTests -j 4
ctest --preset macos-arm64-debug
build/macos-arm64-debug/Pedalboard3_artefacts/Debug/Pedalboard3.app/Contents/MacOS/Pedalboard3 --startup-smoke-test
cmake --build --preset macos-arm64-release --target Pedalboard3 -j 4
```

Expected: the worktree is clean, both app configurations build, all applicable automated tests pass, and the startup smoke process exits with status 0. Include the manual CoreAudio/CoreMIDI gate results and commit hashes in the completion report.
