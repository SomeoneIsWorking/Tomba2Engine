---
id: 22
title: The water-jet replay aborts because ProjPrim holds no depths for guest-written packets, and the file's own comment blamed a class that is present
status: open
symptom: the A00 water-jet path reaches missingGuestDepth and aborts the product mid-play
state_items: S011
tags: tomba2,render,water-jet,depth,gte,fallback
created: 2026-09-27
updated: 2026-09-27
---

## What is measured, and what is only read

**Read from the image and the product source, not from a run — no product run was possible this session**
(the single product slot was held by the Spyro 1 level-dispatch investigation), so nothing here is a
runtime measurement and the cause below is a hypothesis with its discriminating test named.

## The mechanism

`FUN_80027768` is the guest's packed-mesh writer and is owned by `waterJetWriterTap`, which calls the
original and then — only when `WaterJetScope::activeFor(c)` and not `psxRender()` — replays the guest's
GT4 packet span through `gpu_dma2_block` to reconstruct a native submission. The replay's depths come from
`ProjPrim`, and the replay refuses unless every vertex resolved:

```cpp
if (result.depthHits != static_cast<long>(result.packets) * 4L || result.depthMisses != 0 ||
    result.depthStale != 0) {
  missingGuestDepth(result);
}
```

So the abort is not a crash in the ordinary sense: it is the fallback announcing that it **cannot
reproduce the guest's depths**, and declining to submit a picture built from depths that are not the
guest's. That refusal is correct and stays. A submission here would be a wrong picture rather than a
missing effect, and a wrong picture that looks right is the one failure this project will not ship.

## The likely cause, and why

`ProjPrim` records a per-vertex depth when the **renderer executes a packet** — `gp0_exec` is the recording
path (`runtime/psx/proj_prim.h`: "the renderer's gp0_exec looks up the depth at each read address", with
`setPz` doing the recording). The water-jet writer is a *guest* writer appending to the packet pool. If it
appends without the renderer's `gp0_exec` running over those packets, the depths for those read addresses
were never recorded, every lookup misses, and the abort fires on the first water jet — which is what a
fallback that "replays packets to recover their depths" structurally cannot do.

**This is a hypothesis, and it is falsifiable in one run:** if `misses` equals `packets * 4` (every vertex
missed), the depths were never recorded and this is it. If `stale` is non-zero the addresses resolved into
a recycled pool slot instead. If `hits == expected` but the comparison still fired, the arithmetic or the
walk disagrees about the vertex count and it is a packet-walk bug. The abort message now prints
`expected_hits` alongside the counts precisely so those three can be told apart from the log.

## The comment that was wrong, and what it cost

`guest_gte_water_jet.cpp` said of this path:

> …only the authenticated executable/overlay evidence because WaterJetScope is absent.

**That was false.** `WaterJetScope` is declared in that same file, and `waterJetControllerTap` establishes
it around its call of the original controller — the only route by which the writer is reached with the
scope up. The class is not absent; what is absent is the depth accounting. A reader who believed the
comment would have gone looking for a missing class rather than for a depth-recording gap, and the
comment was load-bearing enough that this issue was nearly filed as "the scope was never wired up".

Corrected in place, with the correction and its reason kept next to the code.

## The proper fix, and what is explicitly not it

**Fix the recording, not the check.** Either guest-written packets go through the renderer's packet
execution path so their depths are recorded, or the water-jet owner runs the guest writer inside a scope
that already holds the depths and stops replaying for depths at all. The second is the better shape and is
the one the file's own design was reaching for.

**Not a fix:** relaxing the comparison, defaulting the depth, or skipping the submission. Each of those
converts "we cannot reproduce this" into a picture built from depths that are not the guest's — the
outcome the abort exists to prevent.

## Also decided here, and why the A/B was not landed

An uncommitted A/B in the tree swapped the controller's overlay-scoped declaration for the resident form at
the same address, with a comment predicting the abort would disappear. **That prediction was never
measured**, and landing it would have been committing an unmeasured experimental revert as a fix. The tree
was restored to the committed declaration. If the conversion of `waterJetControllerTap` turns out to be the
cause, the honest action is to revert that conversion as a deliberate whole-feature decision — not to leave
a speculative A/B in the tree, and not to weaken the depth check.

## Next step

One product run, with the abort's own counters as the measurement: reach the water jet, and read
`packets` / `expected_hits` / `hits` / `misses` / `stale` from the log. That single line places the defect
on one of the three rows above and names the fix.
