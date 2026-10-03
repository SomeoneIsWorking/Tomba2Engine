---
id: 22
title: The water-jet replay aborted because ProjPrim held no depths for guest-written packets
status: closed
symptom: the A00 water-jet path reached missingGuestDepth and aborted the product mid-play
state_items: S004, S011
tags: tomba2,render,water-jet,depth,gte
created: 2026-09-27
updated: 2026-10-03
closed: 2026-10-03
---

## Cause, as measured

The abort was never a depth bug to be fixed in place — it was the fallback announcing, correctly, that
it could not reproduce the guest's depths, and the fallback has since been deleted.

`FUN_80027768`'s depths come from `ProjPrim`, which records a per-vertex depth when the RENDERER
executes a packet (`gp0_exec`). The water-jet writer is a *guest* writer appending to the packet pool,
so those read addresses were never recorded, every lookup missed, and the replay's 4/4 check refused —
correctly. A run printed the counts that place it on that row of the three: `packets=2
expected_hits=8 hits=0 misses=2 stale=0` — every vertex missed, nothing went stale, so the arithmetic
was never in question.

## Fix

`Render::waterJetMeshRender` in `game/render/fx_water_jet.cpp` builds the jet from the node's own
controller state, so there are no guest packets and no depths to reproduce: the anchor at
node+0x2E/0x32/0x36, the signed mode at node+0x60 (which indexes the packed record-list table at
0x8010A058 and selects the branch), the uniform scale byte `(s16)node+0x62 >> 4`, the Euler angles at
node+0x54, and the writer's literal arguments (clut row 0, sort bias -250, U scroll 0). It composes
through `projComposeObjectHost` and emits through the existing `meshQuadRecordsEmit`, so the jet also
interpolates under the lerped camera. `guest_gte_water_jet.cpp` — the scope, the replay and the depth
check — is deleted.

The check was never widened or relaxed; it went away with the code that owned it.

## Evidence

Same headless route as issue 0026. A 2500-frame run of the seaside field with `PSXPORT_DEBUG=waterjetmesh`:

```
rc=0, zero FAULT lines, fallback_blocks=0 fallback_instructions=0
[waterjetmesh] f1116 t=1.00 node=800FF928 mode=1 mesh=8014B1B8 pos=(8080.0,-1559.0,5439.0)
               ang=(0,-193,0) scale=52 quads=2 screen=[92.1,40.0]..[131.3,83.6] interp=host
```

All five non-zero modes of the table (1..5) draw across f874..f1164, each on both the real and the
interpolated present. The captured frame at the logged bbox shows the translucent white jet plume
rising out of the sea with the water visible through it.