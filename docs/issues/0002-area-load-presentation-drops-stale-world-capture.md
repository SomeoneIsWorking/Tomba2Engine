---
id: 2
title: Area-load presentation drops stale world capture
status: fix-verified
symptom: PSXPORT_GATE_PRESENTATION aborts across a cold area warp because transition-world quads are captured but field-area initialization intentionally presents no world
tags: render,tomba2,presentation,fps60,area-load
created: 2026-08-24
updated: 2026-08-25
---

## Root cause

Two independent boundary violations met at the cold-warp frame:

1. Standalone psxport serviced the armed REPL warp before `native_step_frame` presented the pending
   old-area capture. Tomba's present-time world rebuild therefore read destination-area state while
   consuming old-area capture metadata and emitted no old world.
2. `CubeTextBanner::render` has no temporal input to capture, but the real-frame object walk still
   invoked it while `Fps60::mWorldCaptureOnly` was true. Those quads entered a dead capture queue;
   the presentation-time walk correctly suppresses the field world during area initialization, so no
   later presentation could own them.
3. `RenderQueue::flush` retained an already-consumed nonempty queue for the next producer's lazy
   reset, but treated that retained storage as a new submission when a later `DrawOTag` produced no
   items. At f3016 the real submission captured one `0x800EDE28` world item plus three HUD items; the
   later empty submission re-captured the same four items. The two apparent world items were therefore
   one item duplicated by queue lifecycle, not two producer calls.

## What was tried / dead ends

The fatal frame was not evidence that Area 21's new sky producer failed. The pending queue contained
old-area terrain and banner nodes, while the destination scene had already replaced the guest state.
Disabling presentation-ledger validation would only hide that cross-scene transaction and was
rejected.

Moving the capture reset after the cold operation was a falsified hypothesis. A hermetic callback
model passed after that reorder, but the bounded integration run
`scratch/logs/gate-run-20260825-001823.log` produced the identical f3016 8/6 ledger and the identical
two `0x800EDE28` world quads. The records are produced during the scheduled guest frame, not by the
synchronous cold operation. The reset reorder and its model-only regression were removed.

Gating `billboardEmit`'s BbRec capture on `fieldAreaInit()` was also falsified. The predicate compiled
and the executable relinked, but `scratch/logs/gate-run-20260825-002511.log` reproduced the exact same
f3016 8/6 ledger and two `0x800EDE28` tier-1 items. Those RqItems therefore do not come from the BbRec
capture path. The ineffective policy/header/test/CMake change was removed and C063 was falsified.

## Resolution

All three causes have verified fixes, but the two framework patches remain in isolated worktrees and
are not landed yet:

- Tomba centralizes the banner invariant in `CubeTextBanner::pictureBuildAllowed`; the producer now
  refuses oracle and world-capture-only picture builds no matter which caller reaches it.
  `tests/test_cube_text_banner_policy.cpp` exercises the full truth table hermetically.
- The isolated psxport `repl-warp-frame-boundary` patch orders the shipping boundary as pending-frame
  presentation, capture reset, cold warp, then destination guest scheduling. Its red-first callback
  regression failed against the extracted old ordering and passes against the correction.
- The isolated psxport `render-queue-consumed-flush` patch makes `RenderQueue::flush` return before
  sorting, diagnostics, ledger accounting, capture, or emit when its retained payload is already
  consumed. Its shipping-seam regression was RED because the second empty flush raised the presenter
  count from 1 to 2; GREEN holds both presenter and HUD-ledger counts at 1, then proves the next real
  push still performs the lazy reset and advances both to 2.
- A combined Clang build linked `tomba2_port` against that isolated framework worktree; both focused
  render policy tests and the normal format/structure/clang-tidy gate pass.

The first granted repeat, `scratch/logs/gate-run-20260825-001241.log`, proved the first two corrections
moved the boundary: f3015 presented the full pending old-area frame before the cold warp, the former
164 CubeText capture quads were absent, and the narrowed f3016 failure was 8 captured / 6 presented.

The one-shot consumed-queue discriminator, `scratch/logs/gate-run-consumed-flush-20260825.log`, used a
combined scratch framework containing both non-overlapping patches. It proves the residual's exact
cause and the corrected answer:

- old f3016: 8 captured, including two `0x800EDE28` world entries; 6 presented; one dropped layer;
- corrected f3016: `captured n=0`, with no queue flush or dropped layer;
- destination rendering resumes at f3336 with 131 world items, and continues through the requested
  f3614 boundary;
- run-end: 3,614 frames reconciled, zero with a dropped layer; PID 187070 exited normally.

The next boundary is landing both verified framework patches on current psxport main, updating the
consumer pin, and repeating the combined gate from the exact recorded framework commit. This issue is
not a remaining producer-identification task: the producer hypothesis was disproved by the consumed
queue lifecycle discriminator.
