# Tuner Closeout Handoff — 2026-09-16

## Restart point

- Branch: `codex/rt-hosting-sprint`.
- Product phase: **active development**.
- Contract status: the visual layout is **volatile**; the serial guitar-string checklist is a **durable functional contract** because it gives the player persistent progress feedback.

The completed tuner work is real: background pitch analysis, response smoothing,
reference and drift state, the string checklist, and the simplified quick-read node
layout are all present. The current node is `430 x 350`, with the note, coarse
deviation strip, and primary meter prioritized.

## Closeout status — 2026-09-16

### 1. Persistent-string checklist — done

Root cause: after high E is marked, the first overlapping analysis window of low E
is still classified as E4 at a deterministic **9.2 cents** sharp. Fast-mode
checklist logic treated that delayed mixed window as "high E is now out of tune"
and cleared bit 5. The local `updateGuitarStringChecklist()` code only ever
cleared the currently matched string; the matched string was still high E.

Fix: keep the 4-cent in-tune set band, and only clear a string at **12 cents**
or more. The 4–12 cent gap is hysteresis for overlapping-window revisits, not a
weakened high-E assertion. The existing high-E-then-low-E check is unchanged.

Verification:

```powershell
cmake --build build --config Release --target Pedalboard3_Tests
.\build\tests\Release\Pedalboard3_Tests.exe "[tuner]"
```

Result: 10/10 cases, 512/512 assertions. The strings case was repeated 5 times
and passed each time.

### 2. Visual sign-off — captured; live note-glyph check still useful

Node snapshot: `documentation/qa/2026-09-16-tuner-closeout/app-node-tuner.png`
(430 x 350). Header, mode buttons, coarse strip, needle, and pins are unclipped.
The capture is a waiting/no-signal node, so the note glyph is empty by design.
Playing a note in the running app is the remaining eyeball check.

## Explicit non-blockers

- `drawStatusBadge`, `drawPitchTrace`, and `drawReferenceResponseRail` remain as
  dormant drawing helpers. The quick-read layout intentionally no longer calls
  them. Keep them unless a later product decision makes their removal worthwhile.
- Do not add a new tuner mode, redo pitch analysis, or fold Scratch/Stage work
  into this closeout.

## Done definition

1. The focused `[tuner]` test run is green, including the high-E/low-E sequence. Done.
2. The manual visual result is recorded. Done; live detected-note eyeball remains optional.
3. The focused Release test build is green and `CHANGELOG.md` records the checklist fix.
