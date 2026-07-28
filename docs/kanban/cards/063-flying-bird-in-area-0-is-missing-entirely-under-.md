---
id: 63
title: Flying BIRD in area 0 is missing entirely under pc_render
status: todo
labels: [bug, render]
created: 2026-07-28
updated: 2026-07-28
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
