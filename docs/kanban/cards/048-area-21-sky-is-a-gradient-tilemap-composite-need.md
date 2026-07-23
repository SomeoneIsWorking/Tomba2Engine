---
id: 48
title: area 21 sky is a GRADIENT+tilemap COMPOSITE — needs the gouraud base ported (split from #42)
status: todo
labels: [render]
created: 2026-07-23
updated: 2026-07-23
---

**2026-07-23:** Split from #42 on 2026-07-23. Area 21's backdrop IS tilemap-adjacent but is a COMPOSITE, and routing it through the plain tilemap producer was tried and MEASURED WORSE.

MECHANISM (RE'd): the field backdrop dispatcher (gen_func_8003DF04 @0x8003DF04) special-cases bg-state 21 AHEAD of its jump table, calling 0x8010BE30 with r4=0x800ED018. That drawer is NOT the plain tilemap routine: keyed on mem_r8(0x800BF871) (==1 in area 21) it first calls helper ov_a0l_gen_8010BB64, which builds FOUR gouraud POLY_G quads spanning x[0,320] with colour words 0x00AC0606 and 0x00EA9898 and a scroll-derived Y origin (from the signed hword at 0x800C00F0, scaled >>12, +120) — that is the sky GRADIENT BASE. Only after that does 0x8010BE30 run its own tilemap loop (same struct/centering/window as the shared routine, V bias 0). A sibling helper 0x8010BCA8 is called on the other sub-state branch.

So the real picture = gouraud gradient base + the tilemap as a CLOUD OVERLAY on top. PARALLAX_BG_SM is populated (W=36 H=72 tpage=0x000F clut=0x3EBF tilemap=0x8018D8DC scroll=(0x1CA,0x1BE)).

MEASURED DEAD END — do not repeat: allowing state 21 into Render::backdropTilemapDrawer draws the tilemap layer ALONE, opaque, with no gradient behind it. That paints a BRIGHT full sky where psx shows the darker gradient, and the whole-frame delta got WORSE: 61375 -> 64650 px >8/255. State 21 is therefore explicitly excluded in backdropTilemapDrawer ('if (st == 21) return false') and the plane is left black as an honest missing-producer gap. Evidence of the worse result: scratch/screenshots/warpsweep/fix21_triptych.png (left=pc tilemap-only, middle=psx, right=diff).

TO FIX PROPERLY: port ov_a0l_gen_8010BB64 as a native gouraud-gradient RQ_BACKGROUND producer (drawn behind), THEN re-admit state 21 to the tilemap producer so the cloud layer composites over it with the correct blend. Both halves must land together — neither alone is right. Current gap 61375 px >8/255. See docs/findings/render.md.
