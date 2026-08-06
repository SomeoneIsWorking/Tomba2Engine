---
id: C038
kind: claim
status: holds
created: 2026-08-06
tags: render,pc_render,line,shockwave
depends: game/render/fx_line.cpp#shockwaveRingRender
---

## Claim

Render::shockwaveRingRender (FUN_8013E08C) now DRAWS the expanding shockwave ring under pc_render, in the place the guest draws it. Its two root causes are named and fixed: the object translation came from node+0x4E (the rope/tether node layout) instead of the packed SVECTOR at node+0x2C that the emitter hands to 0x80084220, and node+0x50 is the ring's SCALE animator so the port's Y was its own radius; and Robj was divided by 4096 although projComposeObjectHost takes 1.3.12 (4096 = identity), collapsing every ring to a point.

## Evidence

MEASURED 2026-08-06 in an isolated tree (/home/bhamil/repo/psx/scratch-shockwave/T2), three separately-built binaries with distinct md5s, each build exit-0 checked, never the shared checkout: .off = the producer call site replaced by a shockwave-LEGOFF log line, .old = the shipped producer, .new = the fixed producer. In-band leg proof: off logs 152 LEGOFF marks and 0 producer calls, old and new log 152 producer calls and 0 marks, so all three reached the same scene.
(1) PIXEL GATE, scratch/ring/ring_gate.sh, replays/bugs/bucket-softlock.pad, PSXPORT_GATE=1 pc_render headless, PSXPORT_PRESENT_SHOT_AT=275,280,287,320,340,355 — every one INSIDE the producer's own f270..f358 window (the window C036's predecessor got wrong). new-vs-off = 450 / 657 / 909 / 2151 / 1602 / 2232 changed px of 691,200. NEGATIVE CONTROL, the whole point: old-vs-off on the SAME presents with the SAME differ = 0 / 0 / 0 / 0 / 0 / 0, i.e. the instrument reproduces the reported failure before it reports the fix. Differ is scratch/ring/ppmdiff.py (instrument I038), --selftest PASS beforehand.
(2) SHAPE, not just a count: every diff mask rendered at guest resolution is a single closed ellipse outline (top arc, both sides, bottom arc, doubled 2px apart by the highlight+shadow pair). No stray region anywhere on the 960x720 frame — so nothing draws outside the ring.
(3) POSITION against the guest's OWN submission, not against GTE output read back: the native producer's projected screen box (new ropeline denominator line) tracks the guest's lineprim packet vertices to ~1px on 8 sampled frames — f270 guest (154,162)..(164,166) vs native (154.3,162.4)..(162.8,165.6)+shadow(2,1); f279 guest (108,155)..(148,170) vs native (109.1,155.7)..(148.5,170.8); f320 guest (93,132)..(165,159) vs native (93.3,132.6)..(165.3,160.2); f355 guest (50,155)..(122,177) vs native (51.0,155.3)..(122.6,178.1). Same for f271/f275/f283/f287.
(4) psx_render is unaffected by the change (old_psx and new_psx present shots are byte-identical, md5 694d0eef...), confirming the ring in the psx picture is the guest's own drawing and the pc-side fix touches only the pc picture; old_pc vs new_pc at the same present differ by 2151 px.
(5) BOOT GATE: both binaries reach frame 2021, stage 0x8010637C sm48=2, rc=0, 24.5s vs 24.9s.
BLIND SPOTS, stated: one replay, one area (0/seaside), six presents; the +-1 frame correspondence between the present-shot index and the ropeline f-stamp is not pinned down (at present 355 the changed box sits inside f354's envelope rather than f355's, a ~2 guest-px difference in the ring's own growth); and no USER has eyeballed it.

## What would falsify it

A present captured inside f270..f358 of bucket-softlock.pad where the fixed leg does NOT differ from a producer-deleted leg; or a changed pixel outside the ring's own projected footprint (a depth-write side effect on some other layer); or a scene where the ring lands somewhere the guest's own lineprim vertices say it does not
