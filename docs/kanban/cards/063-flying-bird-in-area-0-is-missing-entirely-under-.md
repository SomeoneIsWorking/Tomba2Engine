---
id: 63
title: Flying BIRD in area 0 is missing entirely under pc_render
status: done
labels: [bug, render]
created: 2026-07-28
updated: 2026-08-21
evidence: docs/reference/issues/issue63_missing_flying_bird.png
---

FOUND 2026-07-28 by an A/B MOTION comparison (new tool tools/ab_motion.py), then confirmed by eye.

WHAT: the seagull that flies across the sky in area 0 (seaside, above the crane/pole rig right of the starting hut) is drawn by the reference build and NOT drawn at all by pc_render. Evidence image is a 3x side-by-side of the same region: LEFT = reference (PSXPORT_GATE=1 PSXPORT_RENDER_PSX=1), RIGHT = pc_render. The bird — white body, red feet, wings spread — is plainly there on the left and absent on the right.

HOW IT WAS FOUND (the method matters, it generalises):
Both legs reach free-roam at the SAME frame (216) under PSXPORT_AUTO_SKIP, so they can be captured over the same span with no driving at all. A raw pc-vs-psx pixel diff is useless here (different rasterisers — that is distrusted instrument I005), so ab_motion.py compares each leg AGAINST ITSELF over time and diffs the two MOTION maps. The result was one contiguous block of 22 tiles, x=224..300 y=0..96, that moves 14-21 times out of 23 steps on the reference and 0 times under pc_render. Nothing else on screen differed in motion, and PC-only motion was 0 tiles.

NOT AN EXEC/SPAWN GAP — IT IS PURELY RENDER: the object exists on both legs. The handler sets are identical (46 handlers each, comm shows no difference either way), and the bird's node is 0x800FD118, handler 0x8011D988 = beh_actor_move_sm (LIVE native), moving fast on both legs — reference (7858,-2161,6699), pc (9000,-1450,3968), i.e. mid-flight on each. So the AI runs and the object moves under pc_render; only the picture is missing.

WHERE THE GAP PROBABLY IS: Render::fieldObjectsRender (game/render/render_walk.cpp:674) walks the three render heads 0x800FB168/0x800F2624/0x800F2738 and skips any node whose per-frame visibility marker mem_r8(n+1) is 0. The bird node reads [0]=02 [1]=00 [0xB]=00 — marker 0, type 0 (so not the type-0x20 custom-render-fn path with its whitelist). Either the marker means something else for this node class or the bird is drawn from a head/path the native walk does not cover. NEXT: walk the three render heads live and find which one holds 0x800FD118, then compare against what the substrate walk does with it. Do NOT stamp or tag it — the fix is a native producer for that object class (see the NATIVE PRESENTATION directive).

REPRO (no replay needed, that is the nice part):
  PSXPORT_GATE=1 PSXPORT_RENDER_PSX=1 PSXPORT_VK_HEADLESS=1 PSXPORT_NOAUDIO=1 PSXPORT_DEBUG_SERVER=5971 PSXPORT_AUTO_SKIP=1 ./scratch/bin/tomba2_port scratch/bin/tomba2/MAIN.EXE
  PSXPORT_VK_HEADLESS=1 PSXPORT_NOAUDIO=1 PSXPORT_DEBUG_SERVER=5972 PSXPORT_AUTO_SKIP=1 ./scratch/bin/tomba2_port scratch/bin/tomba2/MAIN.EXE
  # ~24 'shot' frames from each into two dirs, then:
  tools/ab_motion.py scratch/screenshots/ab_ref scratch/screenshots/ab_pc

RELATED: the same sweep found NO other missing motion anywhere on screen at this spot, and no pc-only motion — so at the start position the bird is the only missing animated thing. The sweep should be repeated at other spots/areas; that is how the next ones will be found.

**2026-07-28:** 2026-07-28 follow-up, SAME SESSION — two measurements that narrow it, and one that rules out the first guess.

(1) THE VISIBILITY MARKER DIVERGES BETWEEN LEGS. Node 0x800FD118 (handler 0x8011D988 beh_actor_move_sm) reads node+1 = 1 on the reference leg and node+1 = 0 on the pc leg, sampled repeatedly and consistently, with node+0 matching (02/02, then 01/01, so the legs are in comparable states). Render::fieldObjectsRender (game/render/render_walk.cpp:681) skips any node whose node+1 is 0, so on the pc leg that object is culled out of the native object pass.

(2) THE NATIVE HANDLER IS WHAT CLEARS IT. Re-running the pc leg with PSXPORT_BEH_SUBSTRATE=8011D988 flips node+1 to 1 (4/4 samples). So the native beh_actor_move_sm rebuild, not the renderer, is what leaves that node unmarked. This is a NEW and much cheaper repro for kanban #51's open 0x8011D988 divergence: it reproduces at the plain PSXPORT_AUTO_SKIP start position with no replay at all, where #51 only had a 2071-byte diff on the house-on-the-point capture. #51's note says that divergence first shows at the graphics-record freelist cursor 0x800E7E74 / count 0x800ED098 — 'a different NUMBER of records bound' — which is exactly the shape of an object that never gets marked renderable.

(3) BUT THAT ALONE DOES NOT PUT THE BIRD BACK. With the handler on the substrate and node+1 = 1, ab_motion still reports the same missing top-right block, and a zoomed crop still shows no bird. So either 0x800FD118 is not the bird (it is the right-of-Tomba, above-ground mover, but that was inferred from a position sweep, not confirmed on screen), or there is a SECOND gap after the marker. Do not assume the two are the same bug.

NEXT (in this order): confirm the bird's identity on the REFERENCE leg — it is drawn there, so correlate node world positions against the screen region x=224..300 y=0..96 rather than guessing from a position sweep. Only then decide whether the remaining gap is a missing producer or a second exec divergence. Keep #51 updated either way: measurement (2) stands on its own.

**2026-08-21:** CLOSED AS NO LONGER REPRODUCIBLE 2026-08-21. A fresh 32-frame true interpreter/software-GPU A/B sweep (f260..880) shows the seagull crossing the top-right sky on BOTH native A and true-reference B at f500..560. The old contiguous 22-tile reference-only motion block is absent; only three isolated low-count tile differences remain. Earlier node/handler evidence remains relevant to #51, but the current renderer does not have the user-visible missing-bird symptom. Evidence scratch/screenshots/bird_true/{A,B}_topright.png and scratch/logs/bird_true_sweep.log.
