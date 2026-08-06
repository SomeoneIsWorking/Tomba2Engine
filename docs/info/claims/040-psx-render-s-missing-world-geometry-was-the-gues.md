---
id: C040
kind: claim
status: holds
created: 2026-08-06
tags: render,reference
depends: game/render/render_walk.cpp#backdropTexpagePublishTick, game/game_tomba2.cpp
---

## Claim

psx_render's missing world geometry was the guest's own 16x16 backdrop tiles being banded RQ_HUD (topmost) and painted over it — NOT an empty guest OT, and NOT kanban #45's producer retirement

## Evidence

REFUTED the empty-OT hypothesis: REPL 'otattr' re-walk of the last-walked OT, area 0 f3103 psx leg = 983 packets (352 op=0x7C sprites, 308 GT3, 298 GT4) with plausible screen coords; both legs report identical spans=15668/gteBuckets=15. PROVED the banding: PSXPORT_PRIMDUMP=3100:3102 area 0 f3100, pre-fix binary = 972 prims (617 is3d=1 polys + 355 sprites, 355/355 bg=0, 352 of them 16x16 tiles of texpage (896,0)). ROW-EXACT CONTROL across the fix: the pre-fix and post-fix CSVs have 972 rows each, every column identical row-for-row EXCEPT bg, which flips 0->1 on exactly 352 rows and nothing else — so the picture change (scratch/shots/psxref/a0_psx.png sea-only -> a0_psx_fix.png full village) is attributable to the banding alone, even though the post-fix binary also carried another session's in-flight game/render/queue_dispatch.cpp + cull.cpp.

## What would falsify it

a capture where PSXPORT_RENDER_PSX=1 still shows no world while PSXPORT_DEBUG=bgtp reports tilemap=1, or a primdump where the backdrop tiles read bg=1 and the world is still occluded
