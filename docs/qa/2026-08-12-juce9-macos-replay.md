# JUCE 9 macOS replay validation

Date: 2026-08-12
Branch: `codex/macos-au-hosting`
Base: `35c88f559474f4b59f3f9f249022810bc2e98b23`
Required ancestor: `f7bbbc4` (verified with `git merge-base --is-ancestor`, exit 0)
JUCE submodule: `f8f8864172464b9adf9eba6101e1f784838d1597` (`6.0.8-4429-gf8f8864172`)

## Replay

Replayed, in order:

1. `84d3264` `docs: design macOS usable beta port`
2. `16e2a3c` `docs: plan native Apple Silicon bring-up`
3. `1309c2c` `chore: ignore local worktrees`
4. `54fd52f` `build: add Apple Silicon CMake presets`
5. `58f361c` `build: restore reproducible dependency bootstrap`
6. `1c41003` `build: enable C for native dependencies`
7. `45e2f1b` `build: make application target compile on Apple Silicon`
8. `cd01bed` `fix: compile UDP socket on POSIX`
9. `55547ab` `fix: resolve remaining macOS portability blockers`
10. `83c97bc` `feat: use native macOS data and resource paths`
11. `301e5bc` `test: verify native macOS application startup`
12. `bc36d35` `fix: destroy plugin pool before logging teardown`
13. `ba792ff` `docs: record verified Apple Silicon beta`

Conflicts were resolved hunk-by-hunk. The `.gitignore` conflicts retained the JUCE 9/base ignore content and the macOS dependency bootstrap exceptions, including `!cmake/CPM.cmake` and the vendored dependency guards. `1309c2c` was intentionally retained as an empty provenance commit because the base already contained the `.worktrees/` ignore. `1c41003` was also retained as an empty provenance commit because the JUCE 9 base already declared `LANGUAGES C CXX`; the JUCE 9 explanatory comment was retained.

After replay, the history from `f7bbbc4` includes the planning commit, the 13 replay commits above, and the later JUCE 9 test-ordering fix described below. The JUCE submodule remained at `f8f8864`.

## JUCE 9 test dependency fix

The first macOS build reproduced a JUCE 9 ordering failure: `plugin_pool_manager_test.cpp` included production headers that include generated `JuceHeader.h`, but `Pedalboard3_Tests` had only the generated include directory and no dependency on the `Pedalboard3` custom command that creates the header. Ninja therefore compiled the test before `JuceHeader.h` existed.

Added the minimal cross-platform target ordering rule:

```cmake
add_dependencies(Pedalboard3_Tests Pedalboard3)
```

The red build failed with `fatal error: 'JuceHeader.h' file not found`. The green build log showed `Generating Pedalboard3_artefacts/JuceLibraryCode/JuceHeader.h` before test compilation, and produced both requested binaries.

## Build and test evidence

The authoritative checkout is on an SMB-mounted volume. FetchContent left asynchronous temporary Git packs there, so final compilation ran from a local mirror at `/private/tmp/pedalboard2-macos-au-hosting-juce9-verify`. The mirror was created from `git archive HEAD`, then populated with the initialized JUCE working tree while excluding only JUCE Git metadata. No source changes were made in the mirror; the authoritative branch holds the replay and fix.

Commands and results:

```text
/opt/homebrew/bin/cmake --preset macos-arm64-debug
exit 0 (in local verification mirror)

/opt/homebrew/bin/cmake --build --preset macos-arm64-debug --target Pedalboard3 Pedalboard3_Tests
generated JuceHeader.h before test compilation; both target binaries produced

test -x build/macos-arm64-debug/Pedalboard3_artefacts/Debug/Pedalboard3.app/Contents/MacOS/Pedalboard3
exit 0

test -f build/macos-arm64-debug/Pedalboard3_artefacts/Debug/Pedalboard3.app/Contents/Resources/documentation/index.htm
exit 0
```

The exact required CTest command exited 0 but selected no tests because CTest's regex is case-sensitive and the supplied lowercase alternatives (`macos`, `vst3`) do not match registered names (`macOS`, `VST3`):

```text
/opt/homebrew/bin/ctest --preset macos-arm64-debug -R 'macos|platform.paths|startup.options|vst3|rt.hosting' --output-on-failure
No tests were found!!!
exit 0
```

The intended case-correct selection was run against the same generated build tree:

```text
/opt/homebrew/bin/ctest --test-dir build/macos-arm64-debug -R 'macOS|VST3|startup' --output-on-failure
100% tests passed, 8/8
```

It included the three VST3 loading tests, four macOS build/path/documentation tests, and the explicit startup smoke-option test.

## Self-review and concerns

- The replay preserves all requested macOS platform guards and leaves JUCE at the required `f8f8864` revision.
- The JUCE 9 test ordering rule is narrowly scoped to the existing test target and is necessary because that target consumes `Pedalboard3`'s generated header.
- Compiler output contains pre-existing source and upstream-dependency warnings (deprecated JUCE APIs and legacy style warnings); no compiler errors remained after the target-ordering fix.
- System Git is unusable due an unaccepted Xcode license. All Git/submodule operations used the required fallback Git.
- The SMB worktree volume made generated FetchContent clones and Git index operations unreliable. The source replay is authoritative in the worktree; build verification used the documented local mirror.
- During commit recovery, two agent-owned Git commit processes became stuck and were stopped. Their linked-worktree index lock was confirmed stale before removal. The first interrupted commit had left a truncated index; it was preserved at `/private/tmp/macos-au-hosting-index-corrupt-20260812` and the worktree index was rebuilt from `HEAD` before re-staging the focused change. No source file was discarded.
