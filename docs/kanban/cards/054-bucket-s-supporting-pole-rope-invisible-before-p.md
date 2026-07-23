---
id: 54
title: Bucket's supporting POLE/ROPE invisible before pickup, visible after — state-gated missing producer
status: todo
labels: [render, bug]
created: 2026-07-23
updated: 2026-07-23
evidence: docs/reference/issues/issue54_bucket_pole_missing.png
---

USER 2026-07-23 with a capture: the bucket hangs in mid-air outside the seaside hut with nothing holding it. There is supposed to be a POLE (and a rope) supporting it. THE KEY OBSERVATION, and it is an unusually strong discriminator: the pole IS visible AFTER picking up the bucket, and invisible BEFORE.

WHY THAT DETAIL IS THE WHOLE DIAGNOSTIC: the geometry exists and CAN be drawn under pc_render, so this is NOT simply "no producer exists for a pole". Something about the PRE-pickup state routes the pole through an emitter with no native producer, while the POST-pickup state routes it through one that has one (or through the same emitter under a different node/type). So do not hunt a pole emitter from scratch — DIFF THE TWO STATES:
  1. Run `debug otattr` at a frame BEFORE pickup and one AFTER, and compare which fn / caller / node emits the pole packets in each. The post-pickup emitter is presumably already owned; the pre-pickup one is the gap. That one comparison should name the target.
  2. Cross-check with PSXPORT_PRIMAT="x,y,frame" on a pole pixel in the POST-pickup frame to get its material identity, then look for that same identity PRE-pickup (present in the guest OT, absent from the pc queue = confirmed missing producer).
  3. `python3 tools/codemap.py --addr <hex>` on both emitters — the asymmetry may literally be "one is in the owned set, the other is not".

READY-MADE REPRO, no new capture needed: replays/bugs/bucket-softlock.pad reaches the bucket AND picks it up, so ONE replay contains both states. The dialog "This'll work for carrying the Water!" lands at f1200 (the pickup moment), so sample a frame comfortably before f1200 and one after. Take the psx reference from a SECOND run with PSXPORT_RENDER_PSX=1 at the same frame index — never a bare mid-scene `renderpsx` toggle (kanban #41 proves it silently returns a pc frame).

SAME CLASS as the torch flame (#12), dust (#39), non-seaside backdrops (#42), minimap (#43) and vortex (#44) — missing native producers under pc_render, several now fixed. Follow the proven recipe: RE the emitter and PORT it. Per the USER directive a TAP IS NOT ACCEPTABLE ("I don't want stamps, taps or whatever, just do it like how it is supposed to be done, via porting") — game/render/fx_sprite.cpp and game/render/fx_dust.cpp are the templates: read the node's own state, project with projComposeCamera(sceneCam) so it lerps, emit drawWorldQuad with has_xyf=1, dispatch from the display-pass walk.

NOTE the bucket object cluster is well-trodden — it is also the subject of #2 (pickup softlock) and #15 (weapon impact) — so run `python3 tools/findings.py bucket` first rather than re-deriving.

**2026-07-23:** 2026-07-23 SEE #56: the bucket's ROPE (as distinct from the pole) is almost certainly the systemic line-primitive gap — pc_render has no line producer at all, so every GP0 op-0x40..0x5F line is invisible. Fix #56 and the rope may return for free; the POLE (solid geometry) is likely still its own missing producer. Split the two when investigating.

**2026-07-23:** 2026-07-23 THE ROPE IS FIXED (see #56); THE POLE WAS NOT REPRODUCED. Comparing pc_render against a separate psx_render boot at f443 (pre-pickup) and f1200 (post-pickup) on bucket-softlock.pad, the lashed support poles are present on BOTH legs — so the missing thing at the bucket was the rope, now drawn by the new line producer. If the user still sees a pole-less bucket it is a vantage this replay does not reach; needs a capture from that viewpoint before assuming a second missing producer.
