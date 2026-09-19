---
id: 17
title: Three producers let the widened window decide a write back into guest memory
status: invalid
symptom: FILED IN ERROR — the three producers' widened right-edge gates gate only per-frame presentation (guest stack scratch, packet-pool packets, OT links), not persistent guest state; there is no guest-belief defect here
state_items: S005
tags: widescreen,guest-write,oracle-coverage
created: 2026-09-19
updated: 2026-09-19
---

## INVALID — this was filed in error, and here is the disproof

I filed this by pattern-matching Spyro 1's particle defect (its issue 0124), where both particle
producers wrote a visibility byte back into the guest's own PARTICLE RECORD, so widescreen changed
state the simulation carries between frames. That was a real defect. This is not the same thing, and
I did not check the write targets before claiming it was.

What the three widened gates actually gate:

| producer | claimed target | ACTUAL target |
|---|---|---|
| `perobj_billboard.cpp` | "the record's `mem_w32(FR(56), ...)`" | `FR(off)` is `c->r[29] + off` — the GUEST STACK POINTER. FR(56) is a stack local, scratch inside one call. The unconditional depth write at :782 is also BEFORE the gate, not after it as I wrote. |
| `text_label.cpp` | a guest write | `pk` is the packet at the pool tail; the code byte and tpage/clut go into that packet, and `PKT_POOL_PTR` is bumped past it |
| `quad_rtpt_submit.cpp` | guest packet pool + OT | correct as to WHERE, wrong as to what it means — see below |

The packet pool at `0x800BF544` is a per-frame bump allocator the guest resets every frame, and the
ordering table is the frame's display list. Measured: three samples of the pool pointer after 600
stepped frames all sit inside a 2.5 KB band (`0x000C7E50`..`0x000C8848`). An allocator that were
never reset would be megabytes further on after 600 frames at tens of packets each.

So admitting more geometry in the `[320, 428)` band puts more packets in this frame's display list
and links more OT buckets. That is not widescreen deciding what the guest believes — it is what
drawing more geometry on a PSX consists of. The OT bucket is also where layering correctly lives
(the standing rule that a screen's layering IS the guest OT bucket, not a host layer to guess at),
so moving this geometry to a host-side queue would be the actual violation.

Nothing here needs fixing, and the "sanctioned wide-mode guest deviation" comments at those sites
are accurate. The real remaining exposure is capacity, not belief: more admitted prims mean more
pool and OT pressure at 16:9, which is the failure mode issue 0011 already hit from another
direction (a queue frame that never ended, 65,536 prims). That is worth a bound, and it is a
different issue from this one.



## What is wrong

Widescreen is a presentation change. It must not decide what the guest believes. Three producers
currently let it:

| producer | the test | what it gates |
|---|---|---|
| `game/render/perobj_billboard.cpp` | `mem_r16(BUF+8/16/24/32) < xmax` | `continue` past the record's `mem_w32(FR(56), ...)` |
| `game/render/text_label.cpp` | `sx(8/16/24/32) < xmax` | `mem_w8(pk+7, 45)`, `mem_w16(pk+22/14, ...)` |
| `game/render/quad_rtpt_submit.cpp` | `sx(8/16/24/32) < xmax` | `pop()` and early return |

`xmax` is `tomba2::wide_window::drawRight()` — 320 at 4:3, 428 at 16:9. So at 16:9 a prim whose
verts all sit in the `[320, 428)` right band passes a test it fails at 4:3, and the guest's own
packet is written where it otherwise would not be.

This is the same defect class measured on Spyro 1 on 2026-09-19 (Spyro issue 0124): both of its
particle producers computed a guest visibility byte from the widened window, and the same gameplay
frame emitted 8 particles at 4:3 and 20 at 16:9, with a solid quad appearing 261 px INSIDE the
shared field of view. It was fixed there by separating the two questions — what the guest's own
routine would have decided (its own 320/512 window) from what this port draws (the wide window).

## Why it has not been caught

Each site carries a comment saying the deviation is sanctioned because "SBS legs run 4:3 so
byte-exactness is untouched". That is accurate and is exactly the problem: the oracle never stands
in the configuration where the deviation happens, so "byte-exact" is a statement about a
configuration this defect is absent from. The RAM comparison cannot see it and is not expected to.

## Why the Spyro fix does not transfer unchanged

In Spyro the guest byte and the port's draw admission were separable: the byte was written for the
guest, and the port decided separately whether to draw. Here the write IS the admission — writing
the code byte into the guest packet is what makes the prim drawn. Gating the write on the 320
window would stop the right band being drawn at all, regressing the widescreen coverage that
`looks_right.py` measures as drawn aspect 1.333 -> 1.784. The fix therefore needs the port to
admit a prim without depending on a guest-visible byte, which is a producer-ownership change, not
a threshold change.

## What the shipping source proves without any run

`game/render/quad_rtpt_submit.cpp` is the clearest of the three, because what follows its `xmax`
gate is not drawing -- it is guest allocation:

```cpp
  const uint16_t xmax = (uint16_t)tomba2::wide_window::drawRight(c);   // 320 at 4:3, 428 at 16:9
  bool xok = sx(8) < xmax || sx(16) < xmax || sx(24) < xmax || sx(32) < xmax;
  if (!xok) { pop(); return; }
  ...
  uint32_t pool = c->mem_r32(POOL_PTR);        // 0x800BF544 — the GUEST packet pool pointer
  uint32_t otbase = c->mem_r32(OT_BASE_PTR);   // 0x800ED8C8 — the GUEST ordering table
  c->mem_w32(pool, old_head | (9u << 24));     // writes the guest packet
  c->mem_w32(slot, pool);                      // relinks the guest OT bucket
  for (...) c->mem_w32(dstw, c->mem_r32(out + off));
  c->mem_w32(POOL_PTR, pool + 40);             // advances the guest allocator
```

So a quad whose four corners all sit in `[320, 428)` is admitted at 16:9 and rejected at 4:3, and
admission consumes a 40-byte slot of the guest's own packet pool, relinks a guest OT bucket, and
advances the guest allocator. The extra widescreen geometry is not drawn beside the guest's picture;
it is drawn INTO the guest's own structures. That is why the Spyro fix does not transfer: there the
guest byte and the draw were separable, here the guest write IS the admission.

## A probe that did not work, so nobody repeats it

Read the guest pool pointer at both aspects through the debug server, same binary, same scenario
(`pause; step 600`), only `PSXPORT_SETTINGS` differing:

| run | `0x800BF544` |
|---|---|
| 4:3 | `0x000C8848` |
| 16:9 | `0x000C7E50` |
| 4:3 again | `0x000C803C` |

The two 4:3 runs differ by `0x80C`, the same order as the `0x9F8` between the aspects. The pool
pointer is a per-frame bump allocator and `step N` does not land at a fixed point within the frame,
so this reads a different phase every run. It measures nothing about widescreen and it is recorded
here only so the next attempt starts somewhere else.

A valid probe has to sample at a FIXED point in the frame -- immediately after the draw kick, before
the pool resets -- or count admitted prims per aspect at the producer itself rather than reading the
allocator afterwards.

## Discriminator

Run the RAM oracle with the product leg at 16:9 against a 4:3 console leg over a route that puts
geometry in the right band, and compare the packet regions these three producers write. A null
result must report how many prims landed in `[320, drawRight)` on that route, so "no divergence"
is distinguishable from "the route never put anything in the band".

## Not yet known

Whether the guest ever reads back the bytes these producers write, which decides whether this is a
latent correctness defect or only an architecture violation. Nothing measured either way yet.
