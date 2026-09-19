---
id: 11
title: The cutscene keeps submitting after the queue stops being flushed, so prims accumulate to the cap
status: fixed
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

## ROOT CAUSE (measured 2026-09-19): the queue is never consumed, so frames accumulate

The 65,536 prims are not one frame's geometry. They are roughly a thousand frames' geometry, pushed
into a queue that stopped being flushed.

- The last `rqattr` flush attribution is **gpu-frame f1936**. The overflow is at tomba-frame 2940,
  and the `gt3gt4` census shows submission still running at **gpu-frame f2958** — about 1,022 gpu
  frames after the last flush.
- Every flush that DID happen on this route carried at most **2,043 prims**. The failing queue holds
  32x that, with no intermediate values: the growth is not gradual, it is an absence of resets.
- The failing frame itself makes only **13 `gt3gt4` calls declaring 221 faces total**. It cannot
  account for 64,792 world prims. It is merely the frame that happened to cross the cap.
- The largest geomblk anywhere on the route declares **441** faces (166 gt3 + 275 gt4), so no single
  call has an absurd count.
- The garbage-geomblk check fired **0 times over a 17,333-line denominator**, so the counts are real.

`RenderQueue::push()` resets the queue lazily on the first push after `consumed` is set, and
`flush()` is what sets it. Accumulation therefore requires pushes to continue while `flush()` never
runs — which is exactly what the two counters show.

That also explains every earlier observation at once: the repeats (29,343) are a largely static scene
re-submitted on successive frames, producing bit-identical geometry; the single dominant node is one
submission phase repeating; and the 4:3/16:9 attributions are identical because accumulation has
nothing to do with aspect.

## Why submission outlives the flush

`game/game_tomba2.cpp:194` calls `rq.flush(c)` at the end of the drawOTag path, after
`Render::renderScene()`. The overflow backtrace does NOT come through that path — it is
`Engine::fieldFrame` -> `Render::frame` -> (guest) -> `Render::cmdListDispatch` -> `Render::gt3gt4`,
the substrate render orchestrator running during guest execution. So this route has a submitter that
keeps feeding the queue while the presentation boundary that drains it is not reached.

psxport `gpu_native.cpp:3854` already carries a flush for "the GUEST-DRIVEN path only", added
because `rq_flush` lived solely in `Engine::drawOTag`. Whether that boundary is reached on this
route, and why drawOTag stops during this cutscene, is the next thing to measure.

## What is not yet known

- What `cmdListDispatch` is iterating at this frame. 36,169 distinct faces from one submission phase
  is itself the anomaly; the 29,343 repeats are secondary.
- Whether the repeats are duplicate records in the guest's own geometry block, or the same block
  walked more than once.
- `dbg_node 2148433536` (`0x800E7E00`) is weak attribution: geometry submission is a deferred flush
  phase decoupled from the per-object entity walk, so this names a submission phase, not an owner
  (`game/render/submit.cpp`).

## Next step

Find why the presentation/flush boundary stops after gpu-frame f1936 on this route while the
substrate submitter keeps running. Two concrete questions, in order:

1. Does `Engine::drawOTag` stop being called, or is it called and `flush()` returning early on
   `consumed`? The `rqflush` channel answers this directly — it logs inside `flush()` past the
   `consumed` early-out, so silence there separates "not called" from "called and skipped".
2. If drawOTag genuinely stops, is the guest-driven flush at psxport `gpu_native.cpp:3854` the
   boundary that should cover this route, and why is it not reached?

Do NOT raise `RQ_MAX`. It is not a capacity problem: a correctly drained queue on this route peaks
at 2,043 prims, 3% of the cap.

## Consequence

S005's remaining cutscene coverage measurement is blocked: neither aspect completes this route, so
the 16:9/4:3 drawn-aspect comparison cannot be taken at frames 30160-30360.

## Fixed (2026-09-19, `7350ebd`)

`submitFrame` now calls `rq.mark_consumed()` on the suppressed path before returning. That is the
signal the queue already had for "this frame is over"; it simply had one caller, `Engine::drawOTag`,
which a suppressed frame never reaches. The invariant restored is that a queue frame lasts one
FRAME, not one PRESENTATION.

No picture changes. State 3 is "stay": nothing new is presented and no capture is taken, so the
prims that now end with their frame were never going to be drawn.

`RQ_MAX` is unchanged, and must stay unchanged. Raising it would have hidden a queue frame that
never ends behind a larger number, and the next long suppressed stretch would have reached the new
cap the same way.

### Evidence

The full 30,400-field route, which before this could not be run past field 2,940:

```
[looks-right] reaches      PASS — 4/4 shot(s) captured, failure marks: none
[looks-right] widescreen   PASS — f30160 PNG differs from 4:3
[looks-right] coverage     WIDER — drawn aspect 1.333 -> 1.784
[looks-right] fps60        PASS — 20877044 interpolated prim(s) over 30384 extra present(s)
```

`grep -c "render queue full"` is 0 in all three leg logs.

### What is NOT established by that

The oracle route (`tools/oracle_compare.py`) reaches free roam in the opening seaside field and
compares 405 checkpoints per-frame; it never reaches the machinery cutscene. So this fix has a gate
behind it and not an independent-reference comparison. The three oracle legs run on 2026-09-19 —
enhancements off, `aspect=1`, and `PSXPORT_FPS60=1` — are 405/405 with 0 divergences each, which
establishes that neither enhancement perturbs guest state on the route the oracle does cover, and
says nothing about this one. Extending the oracle to a recorded-pad route is the open follow-up.
