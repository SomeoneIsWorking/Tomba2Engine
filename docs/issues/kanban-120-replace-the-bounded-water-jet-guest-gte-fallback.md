---
id: kanban-120
title: Replace the bounded water-jet guest-GTE fallback with a native producer
status: closed
labels: [render, debt]
closed: 2026-10-03
---

Done. `Render::waterJetMeshRender` (`game/render/fx_water_jet.cpp`) is a controller-state display
producer for every non-zero mode, and the fallback it replaces is deleted.

**WHAT IT IS BUILT FROM**, all read out of the guest body at 0x8013D454 (78 instructions, A00.BIN at
the MODE slot) rather than out of anything the writer produced:

* the world anchor as three separate s16s at node+0x2E/0x32/0x36;
* the signed mode at node+0x60, which is both the branch selector and the index into the six-entry
  packed record-list table at 0x8010A058;
* one scale byte, `(s16)node+0x62 >> 4`, written by the guest to all three of FUN_800318A0's scale
  slots — so the jet's column scales are uniform;
* the Euler angles at node+0x54;
* the writer's literal arguments: clut row 0, sort bias -250, U scroll 0.

The mesh records themselves are the shared format `Render::meshQuadRecordsEmit` already owns; both
non-zero modes are two-record translucent strips differing only in height, and all five modes the
table offers (1..5) reach a mesh.

**THE FALLBACK IS GONE.** `game/render/guest_gte_water_jet.cpp` — the `WaterJetScope`, the bounded
0x8013D454 section, the `gpu_dma2_block` replay and the 4/4 guest-depth check — is deleted, along with
its two override registrations (the writer at 0x80027768 and the controller at 0x8013D454), so both
run substrate again like every other `FUN_80027768` caller's siblings. The scope was never widened to
another caller on the way out, and the depth check was never widened or relaxed: the code that owned
both is what went away. The zero branch moved out of `fx_sprite.cpp` into the same file, because it is
the same controller.

**WHAT CHANGED FOR THE PLAYER**: the jet is now drawn from the game's own state every frame and
therefore interpolates with the rest of the picture, instead of being replayed at the logic rate with
interpolation explicitly off.

**EVIDENCE**: `scratch/jet/` — a 2500-frame headless run, `rc=0`, zero faults, `fallback_blocks=0`;
`waterjetmesh` logs for modes 1..5 across f874..f1164 on both the real and interpolated presents; and
`k1116.png`, captured at a logged screen bbox, shows the jet.

**OPEN**: this controller does not publish the depth-cue IR0 (0x1F800090), so the guest inherits
whatever the previous display producer left there. The producer takes the identity (`depthCue = 0`),
which cannot be wrong about an unmeasured value but is not the same claim as having measured it. A
non-zero inherited IR0 would tint the jet by whatever surface it passes over.