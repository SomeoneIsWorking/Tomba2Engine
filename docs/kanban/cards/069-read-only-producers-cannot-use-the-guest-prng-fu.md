---
id: 69
title: Read-only producers cannot use the guest PRNG (FUN_8009A450 writes seed 0x80105EE8) — needs a host-side mirror design
status: todo
labels: [render]
created: 2026-07-29
updated: 2026-07-29
---

Two unported render halves are blocked on this ONE constraint, not on their own difficulty:
  * area 14: FUN_80110CA4's sprite tail 0x801104D0 calls the prng 34 times (card #67);
  * area 4: FUN_8013B118's mesh IR0 cue uses it for a dither (card #68).
FUN_8009A450 reads AND writes the guest seed at 0x80105EE8, so a read-only pc_render producer may not call it — that is a guest-memory write and would break the SBS byte-exact invariant.
THE HARD PART is not mirroring the LCG, it is SEQUENCE ALIGNMENT: the guest's own body still executes underneath and advances the real seed every frame, so a host mirror must be re-seeded from the guest value at a well-defined point. Which point is correct depends on whether the substrate render walk has already run this frame — the same ordering hazard already handled for area 8's history array (class MoteStreaks, game/render/fx_motes.h), and worth solving the same way: snapshot at a fixed point per logic frame keyed on gpu.s_frame, never read mid-walk.
Until this is designed, treat any producer whose cue/dither comes from FUN_8009A450 as BLOCKED rather than merely hard.
