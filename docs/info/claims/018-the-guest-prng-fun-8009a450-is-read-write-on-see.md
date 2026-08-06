---
id: C018
kind: claim
status: holds
created: 2026-07-29
tags: render
reconfirmed: 2026-08-06
verified_at: 2026-08-06
depends: game/render/guest_rng_mirror.h
---

## Claim

The guest PRNG FUN_8009A450 is read-write on seed 0x80105EE8, and that single constraint blocks BOTH remaining unported render halves

## Evidence

gen body of FUN_8009A450 reads mem_r32(0x80105EE8) and writes mem_w32(0x80105EE8) — verified with tools/gen_annotate.py 8009A450 on 2026-07-29. The area-14 sprite tail ov_a0e_gen_801104D0 calls it 34 times (grep count over the annotated body), and the area-4 mesh IR0 cue calls it for its dither (spec, docs/re/render-targets-static-re.md). pc_render is a READ-ONLY overlay, so a native producer may not advance it; doing so would write guest memory and break the SBS byte-exact invariant that is Job #1

## What would falsify it

a demonstration that the seed at 0x80105EE8 is never read back by guest logic that affects canon state, which would make advancing it non-observable; or a host-side mirror shown to reproduce the guest's sequence despite the substrate advancing the real seed in the same frame

## Re-confirmed 2026-07-29

Re-confirmed 2026-07-29 as a statement about the GUEST fn: gen_func_8009A450 reads mem_r32(0x80105EE8) and writes mem_w32(0x80105EE8), so a read-only producer still may not call it. What changed is that it is no longer a BLOCKER — Render::mRngMirror (game/render/guest_rng_mirror.h, commit 7d131b8) is a sanctioned read-only stand-in, so producers needing a jitter have a route that writes nothing.

## Re-confirmed 2026-08-06

RE-VERIFIED 2026-08-06 against the CURRENT gen body, read verbatim rather than grepped. gen_func_8009A450 is generated/shard_2.c:13598-13611 (14 lines) and is a textbook LCG on ONE global: it loads 32784u<<16 + 24296 == 0x80105EE8, mem_r32 it, multiplies by 16838<<16|20077 == 0x41C64E6D (1103515245), adds 12345, mem_w32 the result BACK to 0x80105EE8, and returns (seed>>16)&0x7FFF. So read-write on 0x80105EE8 stands exactly as claimed, and a read-only pc_render producer still may not call it. game/render/guest_rng_mirror.{h,cpp} both still present, so the sanctioned read-only stand-in route is intact. TWO CORRECTIONS TO THE RECORD: (1) the claim's HEADLINE still says the constraint 'blocks BOTH remaining unported render halves' — its own 2026-07-29 reconfirm already retired that, and the headline was never updated; treat the reconfirm as authoritative. (2) The staleness flag was a FALSE POSITIVE: both commits it cites (20ca42c, 7d131b8) are the very commits the 2026-07-29 reconfirm was written about, but a bare `reconfirmed:` DATE does not move the checker's baseline off the creation commit. See instrument note on grepping generated/ by hex address. NOT re-verified: no runtime observation of the seed's effect on canon state.
