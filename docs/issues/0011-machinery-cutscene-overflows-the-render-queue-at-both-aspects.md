---
id: 11
title: The machinery cutscene overflows the render queue at frame 2940, at BOTH aspects
status: open
symptom: replays/bugs/machinery-cutscene.pad aborts with "render queue full (65536 items)" at tomba-frame 2940; reproduced at aspect=0 and aspect=1 with identical attribution
state_items: S005
tags: render,render-queue,cutscene,widescreen,fps60
created: 2026-09-19
updated: 2026-09-19
---

## What was measured

At tomba-frame 2940 the render queue fills and `push()` fail-fasts. The overflow attribution
(psxport `534ee67e`, `render_queue_attribution`) is **byte-for-byte identical at 4:3 and 16:9**:

```
65536 prim(s) examined: 36193 geometrically distinct, 29343 repeating geometry already
submitted this frame, across 2 owning node(s).
by layer: background=0 world=64792 overlay=279 hud=465
largest owners (dbg_node: prims/distinct):
  2148433536: 64792/36169
  0: 744/24
VERDICT: UNDECIDED
```

Backtrace: `cmdListDispatch` -> `Render::gt3gt4` -> `Render::submitPolyGt3Native` ->
`RenderQueue::drawWorldQuad` -> `emitOrQueue`.

For scale, an ordinary field frame on the same route measures **144 prims with 0 repeats**
(`PSXPORT_DEBUG=rqattr`). This frame is roughly 450x that.

## WIDESCREEN IS NOT THE CAUSE — a prior claim, falsified

This was first characterised as a widescreen defect, because the earlier `looks_right` run over this
route showed the 16:9 and fps60 legs dying at exit 139 while the 4:3 leg reported `reaches PASS`.
The proposed mechanism was that `submit_xmax` widens from 320 to 428 at 16:9, admitting faces the
4:3 right-edge cull drops, so a frame that fits at 4:3 goes over at 16:9.

Four runs of the same replay say otherwise:

| run | aspect | field budget | shot frames | debug channel | result |
|---|---|---|---|---|---|
| 1 | 16:9 | 3000 | none | none | overflows at f2940 |
| 2 | 4:3 | 3000 | none | `rqattr` | overflows at f2940, identical attribution |
| 3 | 4:3 | 3000 | none | none | overflows at f2940, identical attribution |
| 4 | 4:3 | 5000 | none | none | overflows at f2940 |
| 5 | 4:3 | 30400 | 30160,30200,30280,30360 | none | overflows at f2940 |

Run 5 is `looks_right`'s exact 4:3 invocation, the one that previously passed. It overflows now.
Aspect, field budget, shot frames and the diagnostic channel are each ruled out as the variable. The
identical attribution across aspects also means the wider `submit_xmax` admitted **zero** extra prims
before the cap was reached, so the widening is not what fills the queue.

The remaining explanation for the earlier 4:3 pass is a build difference. It is not yet identified;
psxport `10071776..534ee67e` is two commits, both diagnostics (`looks_right`, the attribution owner),
neither of which touches submission. **Do not treat "it used to pass at 4:3" as established** until
that is measured.

## What is not yet known

- What `cmdListDispatch` is iterating at this frame. 36,169 distinct faces from one submission phase
  is itself the anomaly; the 29,343 repeats are secondary.
- Whether the repeats are duplicate records in the guest's own geometry block, or the same block
  walked more than once.
- `dbg_node 2148433536` (`0x800E7E00`) is weak attribution: geometry submission is a deferred flush
  phase decoupled from the per-object entity walk, so this names a submission phase, not an owner
  (`game/render/submit.cpp`).

## Next step

Report, at the overflow, the `count` and `rec` passed to each `Render::gt3gt4` call this frame and
how many times it was called. That distinguishes "one call with an absurd count" from "many calls
replaying the same block" without guessing.

Do NOT raise `RQ_MAX`. The attribution says UNDECIDED, and the distinct count alone is 250x a normal
frame — a capacity number chosen to fit this frame would be sized from a defect.

## Consequence

S005's remaining cutscene coverage measurement is blocked: neither aspect completes this route, so
the 16:9/4:3 drawn-aspect comparison cannot be taken at frames 30160-30360.
