# Tuner Closeout Handoff — 2026-09-16

## Restart point

- Branch/commit: `codex/rt-hosting-sprint` at `3ca57f9` (`feat: refine tuner layout and roadmap`).
- Product phase: **active development**.
- Contract status: the visual layout is **volatile**; the serial guitar-string checklist is a **durable functional contract** because it gives the player persistent progress feedback.

The completed tuner work is real: background pitch analysis, response smoothing,
reference and drift state, the string checklist, and the simplified quick-read node
layout are all present. The current node is `430 x 350`, with the note, coarse
deviation strip, and primary meter prioritized.

## What remains

### 1. Fix the persistent-string checklist regression

`tests/tuner_processor_test.cpp:274` fails. The test tunes high E, then low E,
and expects the high-E bit to remain set. In the observed run it was cleared.

The intended behavior is:

- A string set in tune stays marked while other strings are tuned.
- Only a later out-of-tune detection for that same string clears its bit.
- `resetGuitarStringChecklist()` clears every bit.

Start at `TunerProcessor::updateGuitarStringChecklist()` in
`src/TunerProcessor.cpp:309`. Its apparent local logic only clears the currently
matched string, so determine whether delayed/asynchronous analysis results can
subsequently revisit high E out of tune. Do not weaken the assertion merely to
make the test pass; first establish whether the analysis/result-lifetime path or
the test's synchronization is wrong.

Reproduce with:

```powershell
cmake --build build --config Release --target Pedalboard3_Tests
.\build\tests\Release\Pedalboard3_Tests.exe "[tuner]"
```

The latest audit identified this as independent of the layout-only change, so
keep the defect fix in its own focused commit.

### 2. Perform a small visual sign-off

`tests/ui_regression_harness_test.cpp` has a passing 132-assertion tuner source
contract, but it checks draw-call/source presence rather than pixels. Before
calling the layout complete, open a Tuner node and verify the 430 x 350 surface
at normal scale: note glyph, needle/meter, cents strip, controls, and pins must
remain readable and unclipped. Capture one screenshot or record a concise manual
QA result. Repeat at the highest app scale used in normal development if scaling
is part of the release target.

## Explicit non-blockers

- `drawStatusBadge`, `drawPitchTrace`, and `drawReferenceResponseRail` remain as
  dormant drawing helpers. The quick-read layout intentionally no longer calls
  them. Keep them unless a later product decision makes their removal worthwhile.
- Do not add a new tuner mode, redo pitch analysis, or fold Scratch/Stage work
  into this closeout.

## Done definition

1. The focused `[tuner]` test run is green, including the high-E/low-E sequence.
2. The manual visual result is recorded.
3. The focused Release build is green; update `CHANGELOG.md` in the same commit
   if user-visible behavior changes.
