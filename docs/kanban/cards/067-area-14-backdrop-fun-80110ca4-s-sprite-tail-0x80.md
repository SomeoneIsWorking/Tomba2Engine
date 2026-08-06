---
id: 67
title: Area-14 backdrop: FUN_80110CA4's sprite tail 0x801104D0 (440 lines) is unported
status: todo
labels: [render]
created: 2026-07-29
updated: 2026-08-06
---

Render::fxBackdropPlaneRender (fx_backdrop_plane.cpp) owns the two GRIDS of FUN_80110CA4 — the waterfall wall, its mirrored reflection and the additive seam glow — and is pixel-verified. But the guest render fn TAIL-CALLS 0x801104D0 with the same node, a 440-line sprite-family body that is not ported, so whatever that half draws is still absent from area 14. Spec context: docs/re/render-targets-static-re.md, section 0x80110CA4. Portmap step fx-backdrop-plane-110ca4 is marked verified for the grids only and says so.

**2026-07-29:** 2026-07-29: BLOCKED on the shared PRNG constraint, not on its own size. ov_a0e_gen_801104D0 calls FUN_8009A450 34 times, and that fn reads AND WRITES the guest seed 0x80105EE8 — forbidden to a read-only pc_render producer. See the new card on the host-side RNG mirror; this cannot be ported faithfully until that is designed.

**2026-07-29:** 2026-07-29: the PRNG blocker is GONE (card #69 resolved — use Render::mRngMirror). What remains is the RE itself: ov_a0e_gen_801104D0 is a 441-line multi-state machine (labels L_80110524/530/61C/614/658/6C8/734/944/AB0/C50 ...), not a straight emitter, so it needs a proper RE pass rather than a transcription. Structure so far: FUN_800329E0 camera+DQA, FUN_800317CC gate with bias -50, FUN_80027A4C 8-byte record writer (-> Render::spriteRecordsEmit), 34 PRNG draws, and a per-state branch on a counter at r23. Annotated body: tools/gen_annotate.py 801104D0.

**2026-07-29:** 2026-07-29 PORTED (draw half), status ported-unverified. Render::fxBackdropSparkRender in fx_backdrop_plane.cpp, called from fxBackdropPlaneRender as the guest tail-calls it. The 441-line body turned out to be mostly SIMULATION + SPAWN state machine (where its 34 PRNG draws live); the DRAW half is a 200-slot pool read. So this needed no GuestRngMirror after all — the randomness is upstream of the state we read, and the guest's own body keeps the pool simulated underneath. NOT pixel-verified: the pool reads live=0/200 in the reachable area-14 capture, so it correctly draws nothing there and the delta vs the grids-only build is 0. The 'live=N drawn=M/200' diagnostic is deliberate so that zero is distinguishable from a broken producer (instrument I022).

**2026-08-06:** 2026-08-06 (G10 survey) — THIS CARD IS STALE; it should be re-read as a VERIFICATION task, not a porting one. 0x801104D0 IS ported: tools/codemap.py --addr 0x801104D0 returns Render::fxBackdropSparkRender LIVE at game/render/fx_backdrop_plane.cpp:210, called from fxBackdropPlaneRender exactly as the guest tail-calls it (portmap step fx-backdrop-sparks-1104d0, ported-unverified). The card's stated blocker — 'ov_a0e_gen_801104D0 calls FUN_8009A450 34 times and that writes the guest seed' — does NOT apply, and the port's own note says why: the guest body SIMULATES AND DRAWS (integrates pos/vel, runs the spawn state machine where all 34 PRNG draws live), while a read-only producer reproduces none of that and only reads slot state. The randomness is upstream of what is read, so no GuestRngMirror is needed. What IS still open is verification: the pool reads live=0/200 in the area-14 capture, so the pixel delta against the grids-only build is 0 — an EMPTY POOL, not a dead producer. Find a scene that populates it. See docs/unported-render-inventory.md R5.
