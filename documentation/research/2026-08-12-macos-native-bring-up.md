# Native Apple Silicon bring-up

## Build environment

- Architecture: `arm64`
- Host: macOS 26.6.1
- Compiler: Apple Clang 21.0.0 (`arm64-apple-darwin25.6.0`)
- SDK: `/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk`
- CMake: 4.4.2
- Ninja: 1.13.2

## Reproducible debug build

```bash
/opt/homebrew/bin/cmake --preset macos-arm64-debug
/opt/homebrew/bin/cmake --build --preset macos-arm64-debug --target Pedalboard3 Pedalboard3_Tests -j 4
/opt/homebrew/bin/ctest --preset macos-arm64-debug --output-on-failure
```

## Startup smoke mode

The application accepts `--startup-smoke-test`. The option is deliberately
explicit and schedules application quit 1.5 seconds after normal startup,
allowing CI and manual bring-up to validate construction without leaving an
interactive application open.

```bash
build/macos-arm64-debug/Pedalboard3_artefacts/Debug/Pedalboard3.app/Contents/MacOS/Pedalboard3 --startup-smoke-test
```

CoreAudio and CoreMIDI device validation remain manual checks on a host with
the relevant hardware. They are not represented by this deterministic smoke
mode.
