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

**2026-07-28:** 2026-07-28 IMPLEMENTATION ATTEMPT — the producer is written and measured, and it is NOT wired, because wiring it as-is DOUBLE-DRAWS. Recording this so the next attempt starts from the measurement instead of repeating it.

WHAT WAS BUILT: game/render/subpart_capture.cpp — Render::subPartCapture(c, node, sub). Decodes the sub-part's geomblk host-side (GT3 36B / GT4 44B records, the layout mesh_draw.cpp and submit.cpp's GT3/GT4 submitters both use) and pushes one Render::WqRec per prim, with model-space corners plus the sub-part's transform at sub+0x18 read by wq_read_matrix and factored to world by wq_factor_world — the SAME contract textLabelEmit's glyph pass uses, so billboardsRender projects and lerps board and letters through one path with one identity key. Colours follow the submitters exactly (rgb0 keeps the op byte through COL_MASK 0xFFF0F0F0, rgb1 = rgb0<<4, rgb2 @+4, rgb3 = rgb2<<4); uv words repack to the WqRec convention (clut in wUv0>>16, tpage in wUv1>>16). Strictly read-only, touches no c->r[], skipped on the oracle leg. Diagnostic channel PSXPORT_DEBUG=subpartcap reports node/sub/geomblk/counts.

IT FIRES AND IT IS THE RIGHT OBJECT: on an AUTO_SKIP boot, subpartcap logs node=800FB218 walking ~26 sub-parts, each geomblk=8015CA04 with gt3=0 gt4=6 — six quads per sub-part, i.e. one box per PLANK, ~156 quads for the banner. That is the plank strip.

WHY IT IS NOT WIRED — the A/B, on a deterministic frame. replays/bugs/bucket-softlock.pad reaches the 'Go to the Burning House!' banner at replay frame 240 (PSXPORT_PAD_SHOT_AT=240). Enabling the call changes exactly 26 of 76800 pixels, ALL inside the banner band x61..252 y66..86. ~156 extra quads that move only 26 edge pixels are a SECOND COPY landing exactly on the first: the sub-parts are ALREADY drawn at guest time (func_8003F698 -> the native GT3/GT4 submitters), so on a REAL frame the two copies coincide and only rounding differs — and on an INTERPOLATED frame only the WqRec copy would move, GHOSTING the planks. That is worse than the bug being fixed. The premise in this card's body — that the mesh half 'produces only guest packets, nothing the display pass can draw' — is therefore WRONG for the drawing half: the planks do get drawn, natively, at guest time. What they do not get is a display-pass record, which is why they do not lerp.

THE PREREQUISITE, and it is the real unit of work: capture and guest-time draw must be MUTUALLY EXCLUSIVE per prim, not additive. Render::gt3gt4 already has the pattern — it skips its own projection+submit when the transform was captured upstream (submit.cpp, the fps60 mWorldCaptureOnly / rqRedirect tier-1 path). subPartWalk's sub-parts need the same treatment before subPartCapture can be turned on. Until then the call site carries the full writeup and the producer stays compiled-but-unwired (the demo_leaf_a.cpp precedent).

VERIFIED NO REGRESSION: the shipped build is pixel-IDENTICAL to the capture-disabled baseline at frame 240 (diff bbox None).

**2026-07-28:** 2026-07-28 SECOND ITERATION — the handover works, and the LERP BENEFIT IS UNPROVEN. Null result, recorded so it is not re-derived.

BUILT THE MISSING HALF: Render::mSubPartDrawSuppress (render.h), a scope the native GT3/GT4 submitters (submit.cpp) check to skip ONLY their drawWorldQuad — host-side handover, every guest-visible effect still happens. subPartWalk raises it around the per-sub-part submit while subPartCapture has taken those prims. That removes the double-draw the previous iteration measured.

THE HANDOVER IS PICTURE-NEUTRAL: on frame 240 of bucket-softlock.pad ('Go to the Burning House!' banner), capture+suppression vs the plain guest-time draw differs by 26 of 76800 px — edge rounding only, banner fully intact, planks and letters all present. So the display-pass producer reproduces what the guest-time submitter draws.

BUT IT DEMONSTRATES NO BENEFIT, and that is the finding. fps60 triple classification over the banner band (x40..288 y56..96, 249 triples, PSXPORT_DEBUG=fps60dump) is BYTE-IDENTICAL with and without the handover:
    STATIC 4002 / BETWEEN 5518 / STALE 5 / AHEAD 2427   (both builds, to the tile)
STALE ~0 BEFORE the change means the planks were never frozen in this repro — there was nothing for the fix to fix here. Which is consistent with #16's own wording: the artefact needs a MOVING CAMERA ('the text jitters while the camera moves'), and the camera is static while this banner is up, so this repro cannot show the artefact OR its fix.

DECISION — NOT WIRED. The code stays compiled-but-unwired with the full measurement at the call site. Turning on a change that reroutes the producer for EVERY sub-part user, game-wide, on an unmeasured hypothesis is not worth the blast radius (sort order and lighting differ between emitRecQuad and the GT3/GT4 submitters; the 26px agreement at one static frame is not evidence that holds everywhere). Shipped tree verified pixel-IDENTICAL to baseline at frame 240 (diff bbox None).

NEXT, precisely: capture a replay with the banner UP WHILE THE CAMERA PANS, re-measure the same band both ways, and wire this only if STALE/AHEAD actually improves. The banner is reachable at game start and the quest-update banner triggers on a quest event, so a capture that walks during the banner is the missing instrument — not more code.
