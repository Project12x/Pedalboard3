# Native Apple Silicon bring-up

## Build environment

- Architecture: `arm64`
- Host: macOS 26.6.1
- Compiler: Apple Clang 21.0.0 (`arm64-apple-darwin25.6.0`)
- SDK: `/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk`
- CMake: 4.4.2
- Ninja: 1.13.2

## Reproducible builds

```bash
/opt/homebrew/bin/cmake --preset macos-arm64-debug
/opt/homebrew/bin/cmake --build --preset macos-arm64-debug --target Pedalboard3 Pedalboard3_Tests Pedalboard3_NAMCoreRuntimeTests -j 4
/opt/homebrew/bin/ctest --preset macos-arm64-debug --output-on-failure

/opt/homebrew/bin/cmake --preset macos-arm64-release
/opt/homebrew/bin/cmake --build --preset macos-arm64-release --target Pedalboard3 -j 4
```

## Startup smoke mode

The application accepts `--startup-smoke-test`. The option is deliberately
explicit and schedules application quit 1.5 seconds after normal startup,
allowing CI and manual bring-up to validate construction without leaving an
interactive application open.

```bash
build/macos-arm64-debug/Pedalboard3_artefacts/Debug/Pedalboard3.app/Contents/MacOS/Pedalboard3 --startup-smoke-test
build/macos-arm64-release/Pedalboard3_artefacts/Release/Pedalboard3.app/Contents/MacOS/Pedalboard3 --startup-smoke-test
```

Both Debug and Release smoke invocations exited successfully on the development
Mac. The lifecycle shutdown order explicitly destroys `PluginPoolManager`
before the audio format manager and static logging teardown; this corrected a
Release shutdown crash seen during the first smoke run.

The Debug build still prints existing JUCE assertion and leaked-object
diagnostics for `MasterBusProcessor` and UI/accessibility objects during
teardown, although the smoke process exits zero. The Release build starts and
shuts down without output and exits zero.

## Beta artifact

- Path: `releases/Pedalboard3-macos-arm64-beta.zip` (ignored build artifact)
- Size: 375 MiB
- SHA-256: `867386d88b590d7ff33051c05a34cca118b954bb98834bf15134a3371a2b2210`
- Architecture: thin `arm64` Mach-O
- Signature: ad-hoc/linker-signed; not Developer ID signed or notarized

The copied ZIP passed `unzip -t`. It was then extracted into a fresh temporary
directory and the extracted application passed `--startup-smoke-test` with
exit code zero.

## Verification status

- Debug application, test target, and NAM runtime test target build successfully.
- Release application builds successfully.
- All macOS build, native-path, startup-option, and NAM runtime CTests pass.
- The full suite under normal user filesystem permissions reports 313 of 317
  tests passing. The four remaining failures are outside this native bring-up:
  one timing-sensitive guitar-string checklist test and three stale ReverbSC
  source-text contracts that name helpers replaced by the generalized embedded
  node shell policy. The sandbox-only run additionally blocks scratch fixtures
  because JUCE places them below `~/Library/Caches/Pedalboard3_Tests`; those
  scratch tests pass with normal user permissions.

CoreAudio and CoreMIDI device validation remain manual checks on a host with
the relevant hardware. They are not represented by this deterministic smoke
mode.
