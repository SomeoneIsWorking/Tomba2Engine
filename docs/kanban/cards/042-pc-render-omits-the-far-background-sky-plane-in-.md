---
id: 42
title: pc_render omits the far BACKGROUND/SKY plane in non-seaside areas (10, 11, 14, 21)
status: done
labels: [render]
created: 2026-07-23
updated: 2026-07-23
---

Found 2026-07-23 by the CORRECTED area sweep (tools/warpsweep.sh, real PSXPORT_RENDER_PSX=1 reference at the same frame index; recipe newgame; run 3000; warp N; run 600). In areas 10 (lava cave), 11 (snow night), 14 (waterfall) and 21 (wolf-ride, blue sky) the FOREGROUND is byte-for-byte the same state on both legs (character/platforms/props coincident) but psx_render draws the far backdrop plane (cave walls + distant lava / night sky + stars / waterfall / blue sky) while pc_render draws PURE BLACK there. Evidence: scratch/screenshots/warpsweep/s{10,11,14,21}_{pc,psx}.png. >8/255 whole-frame deltas 69731 / 39303 / 68703 / 61375 vs the ~23k generic baseline.

MECHANISM (from docs/findings/render.md, the 2026-06-26 backdrop finding): pc_render's native backdrop producer is SEASIDE-SPECIFIC — ov_bg_tilemap_native publishes the seaside field's backdrop texpage and the classifier routes it to RQ_BACKGROUND. Non-seaside areas have their own backdrop tilemap with no native producer, and since the break-first render rebuild pc_render no longer walks the guest OT, those backdrops are drawn by nobody -> black. The fix is a general native backdrop producer keyed on the active area's backdrop tilemap, not a per-area special case, and NOT a re-introduction of the OT walk or a span tag (banned by NATIVE PRESENTATION). RE the area backdrop drawer and own it.

THIS IS THE CLASS kanban #25 CLAIMED DID NOT EXIST ('missing-layer class NOT distributed across places, 24 of 32 clean'). That claim rested on a pc-vs-pc compare (bare renderpsx no-op) and is falsified — at least 4 areas have a missing backdrop. See docs/findings/render.md 2026-07-23.

**2026-07-23:** 2026-07-23 PARTIALLY FIXED — areas 10 and 11 done; 14 and 21 are DIFFERENT mechanisms and remain. The card's premise ('one missing producer, fix = a general area-keyed backdrop producer') held for 10/11 only.

CAUSE (RE'd from gen_func_8003DF04 @0x8003DF04, not guessed): the field backdrop dispatcher reads selector mem_r8(0x800BF870) (== area id), gated by mem_r8(0x800BF873)==0, special-cases state 21 to 0x8010BE30, drops state>=16, else indexes the 16-entry jump table @0x80014FC0 -> a MAIN stub whose 2nd insn is 'jal <overlay drawer>'. EVERY area's drawer is the SAME tilemap routine compiled per overlay, all called with r4=0x800ED018 (PARALLAX_BG_SM) and reading the identical struct. Render::backdropRender was hard-gated to bgstate==0, so only seaside ever fired.

FIX: Render::backdropTilemapDrawer(int& vAdd) (game/render/render_walk.cpp) mirrors that dispatch, decodes the stub's jal at runtime to find the RESIDENT drawer, then scans the drawer's own code for the tilemap V-decode 'andi r2,r7,0xF0' (0x30E200F0) and reads the per-drawer V bias from the next word (addiu r2,r2,8 -> 8, else 0). Ground truth per area, replacing the old scene heuristic (0x80109450 ? 0 : 8) that wrongly gave bias 8 to every field area. Absent signature = not the tilemap routine = stays black (honest gap). Both the real-frame dispatch and the fps60 present re-render (fps60_worldpass.cpp, which had a duplicate bgstate==0 gate) use the one resolver.

NUMBERS (tools/warpsweep.sh, real PSXPORT_RENDER_PSX=1 reference at same frame; base.report vs fix2.report):
  area 10  69761 -> 18022  FIXED (below the ~23k generic baseline; cave-wall backdrop pixel-clean vs psx)
  area 11  45719 -> 26580  FIXED (night-sky plane matches; the white STAR sparkles are a SEPARATE effect producer still missing)
  area 14  68703 -> 68703  REMAINS — not the tilemap family at all: bg-state 14 indexes jump-table entry 0x8003E020 (the guest's own 'draw nothing' handler) and PARALLAX_BG_SM is ALL ZEROS. The waterfall is GTE-projected SCENE GEOMETRY (otattr fn=0x80114320 node=0x800FE198, 913 RTPS/frame) — an object/scene-layer gap, not a backdrop gap.
  area 21  61375 -> 61375  REMAINS — a COMPOSITE: 0x8010BE30 calls helper 0x8010BB64 first (4 gouraud POLY_G quads = the sky GRADIENT base, 0x00AC0606->0x00EA9898 across x[0,320]) and only then runs its tilemap loop. Real backdrop = gradient base + tilemap cloud overlay. Drawing the tilemap ALONE (opaque, no gradient) paints a bright sky where the reference is darker and MEASURED WORSE (61375 -> 64650), so state 21 is explicitly excluded and left black. DO NOT route area 21 into the plain tilemap producer — tried and measured worse.

GATES: seaside NO-REGRESSION proved by building the binary with the change REVERTED vs applied and diffing the same frame (seesaw-weight.pad f6000) = 0/76800 differing pixels. SBS full 0-diff (560 A/B-identical checkpoints, 0 diverge, through f16770). No gte_op in the producer path. codemap --dup-installs = 0. Dialog opaque+legible at bucket-softlock f1200; triangle item menu renders.

Full write-up: docs/findings/render.md (top section).

**2026-07-23:** 2026-07-23 SPLIT + CLOSED for the tilemap family. Areas 10 and 11 are FIXED here. The two areas that are NOT this cause moved to their own cards: #47 (area 14 — GTE scene geometry, no backdrop drawer at all) and #48 (area 21 — gouraud-gradient + tilemap composite; routing it through the plain tilemap producer measured WORSE, 61375 -> 64650, so it is explicitly excluded). This card covers the shared tilemap-family backdrop producer only, which is done and gated.
