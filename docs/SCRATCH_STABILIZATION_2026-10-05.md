# Scratch stabilization — 2026-10-05

Base: `31e1311`, existing `codex/rt-hosting-sprint` checkout/build.
Authorized continuation: diagnose/fix intermittent test-process exits, reconcile
stale ReverbSC assertions, then assess remaining native/interface acceptance.
No push, merge, PR, alternate build tree, or macOS worktree changes.

Product phase: active development. Process/thread lifetime and reliable regression
results are durable contracts; a defect can crash shutdown or hide regressions.
ReverbSC layout is volatile: verify current semantic policy, not obsolete strings.

## Work sequence

1. Inspect existing crash dumps and pinned local lifetime implementations before
   proposing a fix. Distinguish test-runner lifetime from app/audio changes.
2. Establish a discriminating regression/negative control, apply the smallest
   root-cause fix, and verify in the existing incremental build.
3. Reconcile the three obsolete ReverbSC assertions against actual shell policy,
   preserving meaningful coverage rather than accepting changed source spelling.
4. One focused run and a full integration checkpoint; update changelog/status with
   exact evidence. Runtime UI/plugin/interface acceptance stays open unless run.
5. Audit task scratch; retain only deliberate failure evidence with cleanup terms.

## Resources and reference inspection

Task root: `D:\tmp\codex\scratch-stabilization-20261005`, maximum 256 MiB for logs
and debugger symbol data. Existing small crash dumps are read in place; no dumps
from unrelated programs are touched. Prior task manifest reviewed; its retained
failure logs are not reused as TEMP. Coordinator owns all builds.

Context7 tools are unavailable in this session. Inspect pinned local JUCE/Catch2
source and use official vendor documentation where API/CLI behavior needs checking.

## Diagnosis and fix

The three existing Windows crash dumps/events share `ntdll.dll` 10.0.19041.6456,
exception `0xc0000005`, offset `0x64777`, after successful assertions. A dump
places the main thread in `RtlpWakeByAddress` / `RtlLeaveCriticalSection` under
UCRT `common_exit`; its referenced waiter address is unmapped. A live debugger
breakpoint at CRT exit shows the global thumbnail worker still waiting on a
condition variable after Connection Dragging State Machine passes all nine
assertions. The production app explicitly deletes host services during normal
shutdown; the previous Catch-provided test main did not.

Pinned JUCE source explains the unsafe boundary: AudioThumbnailCache starts a
TimeSliceThread during global initialization. Its late destruction can stop a
worker while CRT exit holds its callback-table lock; JUCE has a forced-kill
fallback after a stop timeout. The observed dump and worker lifetime support
that diagnosis, but the debugger did **not** directly observe a forced-kill call
in the failing process. Do not turn that inferred interleaving into a proven trace.

`tests/test_main.cpp` now owns explicit host-service cleanup around Catch's
session. On Windows it retains a SYNCHRONIZE thread handle before tests start,
requests cooperative exit, and waits up to five seconds for native termination
before cache deletion. A failed handle acquisition/join reports failure and
exits 87 without entering static teardown. This covers discovery/listing and
invalid command lines as well as tests. Non-Windows runs retain explicit service
cleanup; no macOS verification is claimed. App code, audio routing and DSP did
not change.

The independent `[test-lifecycle]` case retains its own native thread handle and
registers an exit callback before global destructors. A non-signaled worker at
that boundary fails the process with exit 86 even if Catch assertions passed.
This is an actual process-lifetime check, not a mock or a retry-until-green gate.

Three ReverbSC tests described obsolete private helper/source spelling. Their
replacement calls the processor's virtual shell policy through the host base
type and checks embedded participation, redundant-control suppression, host-title
ownership and compact relative padding. It deliberately does not freeze a pixel
value or claim to prove rendered host layout. Existing other UI tests remain.

## Reference-code-first record

- JUCE: `https://github.com/juce-framework/JUCE`, pinned
  `f8f8864172464b9adf9eba6101e1f784838d1597`, AGPLv3 dependency. Inspected
  `modules/juce_audio_utils/gui/juce_AudioThumbnailCache.cpp`,
  `modules/juce_core/threads/juce_TimeSliceThread.cpp`,
  `modules/juce_core/threads/juce_Thread.cpp` and
  `modules/juce_core/native/juce_Threads_windows.cpp`.
  Reuse: existing dependency/API and pattern-only inspection; no copied source.
- Catch2: `https://github.com/catchorg/Catch2`, pinned
  `31588bb4f56b638dd5afc28d3ebff9b9dcefb88d` (v3.7.0), BSL-1.0.
  Inspected `docs/own-main.md`; API/pattern use for `Catch::Session().run()` and
  `Catch2::Catch2` linkage. No upstream implementation copied.
- Installed Windows SDK 10.0.26100.0 UCRT `ucrt/startup/exit.cpp` inspected for
  exit-lock/callback ordering only; Microsoft SDK source was not copied or ported.
  Native join semantics checked against Microsoft's
  [WaitForSingleObject documentation](https://learn.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-waitforsingleobject).
  A small test-runner lifetime owner fits this existing host singleton boundary;
  no external worker abstraction or dependency is warranted.

## Verification and review

- RED: with the old Catch main, the exit regression prints two passed assertions
  then fails with exit **86**. The same build also reproduced an AccessViolation
  during post-link test discovery (before any case body).
- GREEN: explicit runner cleanup passes the exit probe. A fresh read-only review
  found JUCE clears its internal handle before native CRT thread teardown ends.
  The retained native handle and cooperative pre-deletion join address that race;
  reviewer rechecked and found no blocking issue. No sleeps or weakened probe.
- Final existing non-clean Release `Pedalboard3` / `Pedalboard3_Tests` build:
  **exit 0**, including post-link test discovery.
- Focused `[test-lifecycle],[reverbsc],[scratch],[rt],[masterbus]`:
  **9,232 assertions; 116 cases passed, two skipped / 118; exit 0**.
- Full `ctest --test-dir build -C Release --output-on-failure`:
  **356 passed, two skipped, zero failed / 358; exit 0**, 12.16 seconds.
  The two skips require Windows symlink privilege. Count changed from 359 because
  three stale cases became one semantic case and one lifetime case was added.
- Fixed-budget lifecycle stress: **100/100 separate processes passed**; stopped
  on any failure, not repeat-until-green. Listing exits 0; an intentionally
  invalid Catch option exits its expected 255, not shutdown failure 86/87.

## Remaining acceptance / next session

Native computer-use initialization succeeds now; the attempt to launch/control
the existing built app returned **"Computer Use app approval timed out"**.
No native UI interaction, new screenshot, device-setting change or recording
was performed. Do not bypass the approval boundary.

With the user supervising and app-control approval available:

1. Check Scratch at default/narrow sizes, dark/light themes and 150% scale;
   exercise actual Tab/Space/Escape focus and transport dispatch.
2. Record a short guitar/interface take with unsaved graph/plugin and master
   settings; verify RAW/WET headers, frame counts and `rig.xml` fidelity/status.
3. Restart, rediscover, play original WET, then loop RAW through the current rig
   while adjusting tone. Confirm playback-to-REC handoff and patch/device removal.

Only then decide whether foundations are user-ready or need a focused repair.
Asset-reference display remains a minor deferred UI item. Rig restore, portable
asset copying and reamp-file export are not part of this pass. No push, merge, PR
or macOS-worktree modification was made.

## Resource handoff

Validated both exact task roots and every descendant, rejected reparse points,
removed 25 disposable top-level items (**13,459,131 bytes**) from this task, and
verified every removed target absent. These were generated test fixtures,
MSBuild temporary files, debugger symbols and redundant logs, not user recordings.
The prior `D:\tmp\codex\scratch-foundations-20261002` root (**161,809 bytes**) was
also removed and verified absent after its diagnosis-retention condition was met.
Deletion is permanent; reproducible fixtures/symbols/logs can be regenerated.
Original Windows crash dumps were only read and were not deleted.

Deliberately retained: `D:\tmp\codex\scratch-stabilization-20261005`,
**138,627 bytes**: manifest plus seven small logs (original dump/live-exit evidence,
RED discovery build, final build, focused/full suite and bounded lifecycle stress).
Owner: coordinator. Purpose: auditable diagnosis and verification until supervised
Scratch acceptance is recorded, then remove this exact root. Not a cache; no
further TEMP use without reopening the manifest. The existing ignored
`.superpowers/sdd/2026-10-02-scratch-foundations` ledger (**24,872 bytes**) is also
retained to resume open acceptance; remove it after acceptance closes.
No alternate build tree or dependency clone was created.
