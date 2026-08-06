---
id: 77
title: Camera blocked by geometry vanilla does not show — 2 spots, stage GAME (USER)
status: doing
labels: [bug, render]
created: 2026-08-06
updated: 2026-08-06
---

USER 2026-08-06, two spots, stage GAME 0x8010637C, coordinates are the RmlUi HUD camera triple (0x1F8000D2/D6/DA):
 (1) cam X 13029 Y -2872 Z 7161 — 'geometry blocking the screen ... that geometry IS there, but vanilla culls this'
 (2) cam X 20161 Y -1923 Z 8268 — 'the water way blocks the camera'
MECHANISM UNIDENTIFIED. Do NOT invent a distance/ID/alpha cull — that is a bandaid with no ground truth.
BLOCKED ON TWO INSTRUMENT DEFECTS, both measured 2026-08-06 (see the two sibling cards):
 - the report carries no AREA/SUB, and the HUD does not print one, so the coordinate cannot be resolved to a place;
 - psx_render draws no world geometry, so the pc-vs-psx reference comparison that would split 'vanilla culls it' from 'the object should be unloaded' cannot be run at all.

**2026-08-06:** MEASURED 2026-08-06 (agent). PHASE 1 LOCATED, both spots, 44-run sweep (2 targets x 22 areas, 42 ok, area 3 rc=139 both):
  T1 (13029,-2872,7161) -> AREA 13: green mass between camera and player. scratch/shots/blockcull-sweep/t1_a13.png
  T2 (20161,-1923,8268) -> AREA 14: wall of water filling the frame. scratch/shots/blockcull-sweep/t2_a14.png
  Coordinate-space finding: BOTH triples hold exactly (readback 800E7EAC unchanged after 300 frames) in EIGHT areas each {7,10,11,12,13,14,15,20}, and hold in X/Z with Y drifting in {2,4}. Elsewhere the teleport relocates. a9/a18/a19 are teleport-REJECTION artifacts (t1 and t2 land on the identical pixel), not repros.
PHASE 2 — MECHANISM NAMED, NOT YET IMPLEMENTED. Three hypotheses tested, TWO REFUTED BY MEASUREMENT:
  (R1) 'pc_render includes on node+1 while the guest uses the cull queues' — REFUTED AT THE SOURCE. The guest's own master render walk gen_func_8003C048 gates on mem_r8(node+1)!=0, exactly as render_walk.cpp does. node+1 is the guest's gate too.
  (R2) 'Cull::objectCull's margin re-include lowers the guest's near cull (0x200 -> 0x80) and puts blockers back' — REFUTED: Cull::objectCull HAS NO CALLER and NO overrides::install entry, and MarginRenderer::collect is called from nowhere else. The whole widescreen-margin re-include is DEAD CODE. (Filed separately.)
  (R3) THE LIVE ONE: the guest has NO RENDER WALK over HEADS[0] (0x800FB168). Its three render walks are gen_func_8003C048 over HEADS[1] (table 0x80014DB8), objListWalk4 over HEADS[2] (table 0x80015000), and gen_func_8003BB50 over the cull's QUEUE B (table 0x80014A70). HEADS[0] nodes reach vanilla's picture only via the cull's class-keyed queue push. render_walk.cpp flushes the whole HEADS[0] list instead — its own comment says 'table not yet RE'd — keep the flush-all behavior'.
EVIDENCE THE T1 BLOB IS A HEADS[0] NODE (break-first A/B, negative control = the pre-change binary): suppressing the HEADS[0] arm removes the green mass and NOTHING else in area 13 (2787/76800 px, scratch/shots/blockcull-ab/{before,after}_a13.png).
WHY THAT IS NOT THE FIX, also measured: (a) area 14's water wall is NOT a HEADS[0] node — the same suppression leaves it pixel-identical; the only a14 delta was 459 px of the PLAYER vanishing, so the arm carries legitimate content; (b) gating on queue membership cannot work — the queues are already drained/reset by display-pass time (live=45, queueCounters(A,B,C)=(0,0,0), 42 of the 45 are class 4, the very class that goes to queue B); (c) gating on the class byte removes nothing — 45/45 in a13 and 27/27 in a14 are already class 2 or 4.
REAL FIX = RE gen_func_8003BB50 (queue-B consumer, per-type table 0x80014A70, 144 entries) and reproduce its submission set for the HEADS[0] arm instead of flushing all. T2's water wall needs a separate producer trace (scene-table/terrain, dbgnode=FFFF0002 in PRIMAT) — different mechanism, same card.
LANDED THIS PASS: the PSXPORT_DEBUG=heads0 census only (no behaviour change; pre-change vs post-change is 0/76800 px in both scenes). NO invented cull was added.
