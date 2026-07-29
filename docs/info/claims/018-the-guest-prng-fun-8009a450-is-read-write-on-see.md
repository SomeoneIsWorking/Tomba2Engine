---
id: C018
kind: claim
status: holds
created: 2026-07-29
tags: render
reconfirmed: 2026-07-29
---

## Claim

The guest PRNG FUN_8009A450 is read-write on seed 0x80105EE8, and that single constraint blocks BOTH remaining unported render halves

## Evidence

gen body of FUN_8009A450 reads mem_r32(0x80105EE8) and writes mem_w32(0x80105EE8) — verified with tools/gen_annotate.py 8009A450 on 2026-07-29. The area-14 sprite tail ov_a0e_gen_801104D0 calls it 34 times (grep count over the annotated body), and the area-4 mesh IR0 cue calls it for its dither (spec, docs/re/render-targets-static-re.md). pc_render is a READ-ONLY overlay, so a native producer may not advance it; doing so would write guest memory and break the SBS byte-exact invariant that is Job #1

## What would falsify it

a demonstration that the seed at 0x80105EE8 is never read back by guest logic that affects canon state, which would make advancing it non-observable; or a host-side mirror shown to reproduce the guest's sequence despite the substrate advancing the real seed in the same frame

## Re-confirmed 2026-07-29

Re-confirmed 2026-07-29 as a statement about the GUEST fn: gen_func_8009A450 reads mem_r32(0x80105EE8) and writes mem_w32(0x80105EE8), so a read-only producer still may not call it. What changed is that it is no longer a BLOCKER — Render::mRngMirror (game/render/guest_rng_mirror.h, commit 7d131b8) is a sanctioned read-only stand-in, so producers needing a jitter have a route that writes nothing.
