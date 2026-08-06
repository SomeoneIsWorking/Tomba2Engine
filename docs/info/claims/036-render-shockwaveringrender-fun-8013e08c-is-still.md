---
id: C036
kind: claim
status: falsified
created: 2026-08-06
falsified: 2026-08-06
tags: render, pc_render, line, shockwave, measurement-window
depends: game/render/fx_line.cpp#shockwaveRingRender
---

## Claim (FALSIFIED — kept for the lesson, do not cite the original)

ORIGINAL: "Render::shockwaveRingRender (FUN_8013E08C) is still UNVERIFIED ON PIXELS: it is called 152
times in bucket-softlock and contributes ZERO pixels to the presented picture. It is COLD, not proven
broken."

**FALSE. It is BROKEN, not cold.** The zero was real and the conclusion drawn from it was not.

## Why it was wrong — the measurement window could not produce the failing answer

The A/B was captured at presents **440-485**. The producer fires only at **f270..f358** — the capture
window contains **ZERO of its calls**. A zero measured where the producer never runs is not evidence
about drawing; it is evidence about the window.

The same report used exactly this reasoning ("the producer is not called there at all") to explain
worldLineDraw's own zero at presents 455-485, and then failed to apply it to this producer. That is
the workspace's recurring failure in its purest form: a negative from an instrument that was
structurally incapable of showing the positive.

The "off-screen, therefore innocent" explanation was contradicted by the step's own census log.
`scratch/lineclass/logs/bucket-softlock.err`, same replay: fn=0x8013E08C emits **1064 line packets
across f270..f358 with vertices at (160,163), (158,162), (163,163)** — dead centre of the 320x240
screen. The guest draws this ring, centre-screen, for ~90 frames.

## The measurement that settles it (RUN, not reasoned)

Same isolated tree, same two binaries (legB = both producers deleted, legD = shockwave live),
`PSXPORT_PRESENT_SHOT_AT=280,300,320,340,355` — i.e. INSIDE the active window — headless:

    0 changed pixels of 691,200 on ALL FIVE presents, with the full 152 shockwave calls logged in
    BOTH legs.

The instrument is not dead: the same binaries and the same differ produce 1584 / 1197 / 927 changed
pixels at presents 440/445/450 for the rope. So it can produce the other answer, and here it does not.

**=> Render::shockwaveRingRender draws NOTHING under pc_render at the exact frames the guest draws it
centre-screen.**

## The mechanism — RESOLVED 2026-08-06, and it was TWO bugs, not one

The suspicion recorded here (that `node+0x4E` was the wrong translation source) was right, and there
was a second, independent bug underneath it. Either one alone would have produced the same zero.

1. **Wrong translation source.** `FUN_8013E08C` hands `node+0x2C` to `0x80084220`, which loads word0 as
   `VXY0` and word1 as `VZ0` — a packed SVECTOR, X@0x2C Y@0x2E Z@0x30. The producer read
   `node+0x4E/0x50/0x52`, which is the ROPE/TETHER node family's layout. Worse, on a ring node `0x50`
   is the SCALE animator, so the port's Y literally *was* the ring's own radius — which is why the
   logged position walked `(0,30,10)..(7,210,10)` in step with the scale. The real position on this
   replay is `(6861,-756,3968)`, right next to the camera. Not parent-relative: just the wrong field.
2. **Robj scaled down by 4096.** `projComposeObjectHost` takes the object rotation in the guest's
   1.3.12 convention (4096 = identity); the producer divided `scale<<4` by 4096 first, so every ring
   collapsed onto a single point. This one only became visible once the position was fixed and the
   producer's own screen box was logged with a denominator.

The **152-vs-76 factor of 2** is root-caused and is NOT a bug: 152 calls = 76 distinct (frame, node)
pairs seen twice, one per PRESENT, because fps60 re-renders the field object walk for the interpolated
present. Measured, not inferred — with `fps60=0` the same replay logs exactly 76.

## Superseded by

[[038-render-shockwaveringrender-fun-8013e08c-now-draw]] — the fix landed 2026-08-06 with a pixel gate
inside the producer's own window and the shipped-producer zero as its negative control. Kanban #56's
closure line and `docs/unported-render-inventory.md` row R-CLOSED-1 were corrected in the same change.

## What would falsify THIS (the falsification)

A pc_render present, captured inside f270..f358, where a leg with shockwaveRingRender live differs
from a leg without it. That would mean the ring draws after all and the 0/691,200 above was wrong.
