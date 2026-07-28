---
id: 64
title: Quest-update banner lerps its LETTERS but not the PLANKS they sit on
status: todo
labels: [bug, render, fps60]
created: 2026-07-28
updated: 2026-07-28
---

USER 2026-07-28 with a capture of the 'A Red Treasure Chest' banner. Reached STATICALLY — read from the port's own source, no live run.

ROOT CAUSE. Render::textLabelEmit (game/render/text_label.cpp, guest FUN_80039F4C) draws a text-label node in two halves and only ONE of them produces a display-pass record:
  - step (1) MESH pass: func_8003F174(node,1) = Render::subPartWalk — per sub-part, load that sub-part's own transform into the GTE and submit its geomblk through func_8003F698. These are the WOODEN PLANKS. Captures NOTHING; it only emits guest packets.
  - step (4) GLYPH pass: per character, one quad from the fixed template V(-3,-7,-1)..(5,9,-1), and for each surviving glyph it pushes a Render::WqRec (template corners + the cmd+0x18 pre-composed matrix factored against the scene camera) so Render::billboardsRender emits the LETTERS through the float camera path.
So the two halves are on different presentation tiers. At 60fps the letters interpolate under the lerped camera and the boards step at 30Hz — which is precisely the capture: glyphs sitting at inconsistent, per-letter offsets on their planks rather than fixed to them.

THE FIX is the one #16 and #23 already name for this class: give the MESH half a display-pass producer so board and letters come from the same state and interpolate together. NOT a prim matcher, NOT an anchor/stamp — both banned by the NATIVE PRESENTATION directive. Render::subPartWalk (game/render/subpart_walk.cpp) is already a native port and is the natural place to capture from: it already has each sub-part's transform at sub+0x18 and its geomblk at sub+0x40. What is missing is a geomblk -> quad decode on the host side; MeshQuads (game/render/mesh_quads.h) supplies the matrix half but not the vertex half, so that decoder is the real unit of work.

SAME FAMILY, do not fix separately: #16 (sign text jitters at fps60) and #23. #16's capture is the same emitter with a different string.

STRING TABLES, recorded so nobody re-derives them: 12-byte/3-word entries {one-line ptr, two-line ptr, packed id}. Objectives are based at 0x800A33C8 and textLabelEmit reads word +4 ('Find Tabby!', 'Go to the Burning House!', 'Pour the Water In!', 'Save the Crab!'). The QUEST-ITEM names actually shown in this banner are a SECOND table at 0x800A3660 ('A Red Treasure Chest' @0x80013BD0 one-line / 0x80013BB8 two-line, 'Adventurer's Chest', 'Capture the Last Evil Pig!', 'Golden Tower', ...). That base is materialised NOWHERE in a field RAM dump — scanned every opcode for the immediate — so its reader is overlay-resident and the banner MAY be a different emitter that shares the same node class. Confirm which before porting: if it is a second emitter, it needs the same two-tier fix.

**2026-07-28:** 2026-07-28 CORRECTION + CONFIRMATION — the banner IS Render::textLabelEmit, and there is no second table.

USER: 'that banner shows up also on game start for another quest.' That is the confirmation, and it exposed an error in this card's own closing paragraph.

I claimed the quest-item names lived in a SECOND table at 0x800A3660 with an overlay-resident reader, and therefore that the banner MIGHT be a different emitter needing identification before any port. Wrong — I started the entry grid 4 bytes off. There is ONE contiguous table based at 0x800A33C8, stride 12, {one-line ptr @+4, two-line ptr @+8, packed id}, which is exactly what textLabelEmit reads. Walked end to end:
  entry  1  'Find Tabby!'
  entry  2  'Go to the Burning House!'        <- kanban #16's sign, same table
  entry  3  'Pour the Water In!'
  ...
  entry 55  'Capture the Last Evil Pig!'
  entry 56  'A Red Treasure Chest'            <- THIS BANNER (0x80013BD0 / 0x80013BB8)
  entry 57  'Adventurer's Chest'
A banner at game start showing a different quest is entry 1 of the same table — precisely what the user describes.

CONSEQUENCES, all of them good:
  - No overlay hunt. The emitter is Render::textLabelEmit / FUN_80039F4C, already owned and already half-producing (game/render/text_label.cpp).
  - #16 (sign text jitters at fps60), #23 and this card are ONE bug seen through three strings. One display-pass producer for the MESH half closes all three.
  - It is reachable at GAME START, so a repro needs no navigation — PSXPORT_AUTO_SKIP passes through it. That makes verification cheap once the producer exists.
The root cause in the body above stands unchanged: the glyph pass pushes a Render::WqRec per character and lerps, the mesh pass (func_8003F174 = Render::subPartWalk, the planks) captures nothing. Still no matcher and no anchor/stamp.
