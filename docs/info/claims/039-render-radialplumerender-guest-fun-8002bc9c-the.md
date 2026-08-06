---
id: C039
kind: claim
status: holds
created: 2026-08-06
tags: render
depends: game/render/fx_plume.cpp#radialPlumeRender, game/render/mesh_quads.cpp#meshQuadRecordsEmit, game/render/render_walk.cpp#fieldObjectsRender
---

## Claim

Render::radialPlumeRender (guest FUN_8002BC9C, the four-copy radial plume) DRAWS under pc_render: it contributes real pixels in its own active frame window, and nothing outside it.

## Evidence

Two Release binaries built in the ISOLATED tree psx/scratch-plumeab/T2, identical except the single dispatch branch in Render::fieldObjectsRender, distinct md5s (ON 9f356400540caf2efafb6d671a777dc6, OFF e3920b921a23d5d70dac8bd1902b40bf). Same replay replays/bugs/bucket-softlock.pad, headless, PSXPORT_GATE=1 pc_render, PSXPORT_PRESENT_SHOT_AT, 960x720 sink (instrument I038). The producer's own channel says it is called on f252-f263. IN WINDOW: present 254 = 675 changed px of 691200 bbox x[465,515] y[273,299]; present 258 = 3267 px bbox x[432,545] y[261,368]; present 262 = 828 px bbox x[432,551] y[291,341]; the f258 diff mask is one connected radial cluster (evidence PNGs at scratch/plume/evidence/ in that tree). OUT OF WINDOW, the negative control: presents 300 and 320 = 0 changed px. LEG PROOF is the channel, not the pixels: 24 plumefx lines on the ON leg, 0 on the OFF leg. Library census, all 17 replays at 900 frames each: SEVEN reach the producer with 24 calls each (bucket-softlock, house-on-the-point, save-prompt-black-screen, seesaw-weight, sequence-softlock-2, title-options-page, walk-dust-puff); the other TEN reach it 0 times (general-session, short-session, start-mash-smoke, dark-screen-repro, ingame-item-menu, ingame-options-page, save-sign-softlock, weapon-impact-bucket, hut-entry-alt, hut-entry-door-freeze); every run exit 0 with zero abort/FATAL, and quads=0 never occurred. CONTAINMENT (added after the channel was extended to report the producer's own emitted screen box, and the pixel numbers above were re-measured against a freshly rebuilt OFF leg from the SAME source so the two binaries still differ only by the one dispatch branch — ON 4b58b00cb9d3cf30d582f87693327c39 / OFF b99520b74d2268a65bce5ebdb5e86ab3, same 675 / 3267 / 828 / 0 / 0 result): scaled by ires 3, 675/675 (100%) of the f254 diff, 828/828 (100%) of the f262 diff and 3258/3267 (99.72%) of the f258 diff lie inside the box the producer itself reported. THE 9 THAT DO NOT are one native 320x240 pixel (146,122) = the 3x3 block (438..440, 366..368), which goes pale yellow (206,206,107) with the producer OFF to dark brown (107,74,49) with it ON. That is outside the plume's own footprint, so it is an ordering/depth-coincidence flip caused by the extra draws, NOT a plume texel — un-root-caused and recorded as such.

## What would falsify it

A run in the f252-f263 window where the ON and OFF legs agree pixel-for-pixel would refute it. So would a psx_render cross-check (NOT DONE — the two legs' present-frame timelines are offset because the psx leg skips the OP FMV) showing the plume in a different place or a different shape: this claim says the producer DRAWS, it does NOT claim the picture is correct, and no USER has looked at it.
