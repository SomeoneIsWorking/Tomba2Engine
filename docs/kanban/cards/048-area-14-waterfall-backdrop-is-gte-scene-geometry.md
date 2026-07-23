---
id: 48
title: area 14 waterfall backdrop is GTE SCENE GEOMETRY with no native producer (split from #42)
status: todo
labels: [render]
created: 2026-07-23
updated: 2026-07-23
---

**2026-07-23:** Split from #42 on 2026-07-23 after the backdrop RE proved area 14 is NOT the tilemap-backdrop family.

EVIDENCE (measured, area reached with newgame; run 3000; warp 14; run 600):
  - bg-state selector mem_r8(0x800BF870) = 14. The field backdrop dispatcher (gen_func_8003DF04 @0x8003DF04) indexes its 16-entry jump table @0x80014FC0 at [14], which holds 0x8003E020 — the guest's OWN 'draw nothing' handler. So the backdrop dispatcher deliberately draws no tilemap in this area.
  - PARALLAX_BG_SM @0x800ED018 reads ALL ZEROS in area 14 (vs fully populated in areas 10/11/21), confirming no tilemap backdrop exists here.
  - The waterfall psx_render draws is GTE-projected SCENE GEOMETRY. 'debug otattr' RTPS/RTPT histogram for the area-14 frame: fn=0x80114320 node=0x800FE198 count=913 (plus fn=0x8003F9A8 node=0x800FB218 count=641 and eight fn=0x8003BDAC nodes at 36-68 each).

So this is an OBJECT/SCENE-LAYER producer gap (class of #39/#44), NOT a backdrop-plane gap. Fixing it means RE'ing and porting the 0x80114320 emitter (or whichever of those nodes owns the waterfall) as a native producer that draws from the object's own state — it will NOT be fixed by any backdrop-tilemap work.

Current gap: 68703 px >8/255 vs psx (unchanged by the #42 fix), whole-frame, vs the ~23k generic baseline. Evidence: scratch/screenshots/warpsweep/fix14_{pc,psx}.png + fix14_triptych.png. See docs/findings/render.md.
