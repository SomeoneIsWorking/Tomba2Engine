---
id: C031
kind: claim
status: falsified
created: 2026-07-23
renumbered: 2026-08-06 from C004 — that id was shared by several claims, so every by-id command resolved it to an arbitrary one
tags: 
falsified_on: 2026-08-06
---

## Claim

pc_render draws the game's ropes/chains/tethers natively; the only line emitter still without a producer is the ground ring shadow FUN_8013E08C

## Evidence

debug lineprim census over replays/bugs/bucket-softlock.pad f0..445 finds 3 line emitters; 2 share the leaf FUN_8013DD34, both ported (fx_line.cpp); native stroke count == guest stroke count at the matched frame; A/B with the dispatch disabled isolates 130 px (rope) and 69 px (tether) and the added pixels are exactly dst+F/4 (blend 3)

## What would falsify it

a lineprim census in another scene showing a 0x5E line packet from a fourth emitter, or a rope visible under PSXPORT_RENDER_PSX=1 and absent under pc_render at the same frame index

## FALSIFIED 2026-08-06

FALSIFIED 2026-08-06 on its SECOND clause, statically. The claim said 'the only line emitter still without a producer is the ground ring shadow FUN_8013E08C'. That address HAS had a producer since commit f4de6f6 (2026-07-28), which is the very commit that flagged this claim stale: tools/codemap.py --addr 8013E08C returns Render::shockwaveRingRender [LIVE] game/render/fx_line.cpp:211. So the exception the claim named no longer exists, and the sentence is now false as written. The FIRST clause (pc_render draws the ropes/chains/tethers natively) was not contradicted by anything measured here and is superseded rather than refuted — the surviving statement is 'all three line emitters found by the lineprim census have a native producer'. WHAT THIS DOES NOT SETTLE, and nobody should read it as settled: the ORIGINAL falsifier is still untested — no lineprim census has been run in another scene, so a fourth emitter elsewhere remains possible; and the census denominator was one replay (bucket-softlock.pad f0..445). DOWNSTREAM, grepped: no doc, tool or source cites this claim by id (its old id C004 is cited once, in docs/kanban/cards/061, but that reference is to a DIFFERENT claim that shared the id — see the renumbering note in this file's frontmatter and in docs/info/claims/004-render-subpartwalk-*). docs/port-map.md step world-line-ring-shadow was independently corrected the same day and already records shockwaveRingRender as the owner, so nothing downstream is left resting on the false clause.

> Anything that cited this claim as proof must be re-checked. Grep the repo for it.
