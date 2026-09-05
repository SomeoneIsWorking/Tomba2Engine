---
id: 64
title: Quest-update banner lerps its LETTERS but not the PLANKS they sit on
status: done
labels: [bug, render, fps60]
created: 2026-07-28
updated: 2026-08-04
---

USER 2026-07-28 with a capture of the 'A Red Treasure Chest' banner. Reached STATICALLY — read from the port's own source, no live run.

ROOT CAUSE. Render::textLabelEmit (game/render/text_label.cpp, guest FUN_80039F4C) draws a text-label node in two halves and only ONE of them produces a display-pass record:
  - step (1) MESH pass: guest 0x8003F174(node,1) = Render::subPartWalk — per sub-part, load that sub-part's own transform into the GTE and submit its geomblk through guest 0x8003F698. These are the WOODEN PLANKS. Captures NOTHING; it only emits guest packets.
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
The root cause in the body above stands unchanged: the glyph pass pushes a Render::WqRec per character and lerps, the mesh pass (guest 0x8003F174 = Render::subPartWalk, the planks) captures nothing. Still no matcher and no anchor/stamp.

**2026-07-28:** 2026-07-28 IMPLEMENTATION ATTEMPT — the producer is written and measured, and it is NOT wired, because wiring it as-is DOUBLE-DRAWS. Recording this so the next attempt starts from the measurement instead of repeating it.

WHAT WAS BUILT: game/render/subpart_capture.cpp — Render::subPartCapture(c, node, sub). Decodes the sub-part's geomblk host-side (GT3 36B / GT4 44B records, the layout mesh_draw.cpp and submit.cpp's GT3/GT4 submitters both use) and pushes one Render::WqRec per prim, with model-space corners plus the sub-part's transform at sub+0x18 read by wq_read_matrix and factored to world by wq_factor_world — the SAME contract textLabelEmit's glyph pass uses, so billboardsRender projects and lerps board and letters through one path with one identity key. Colours follow the submitters exactly (rgb0 keeps the op byte through COL_MASK 0xFFF0F0F0, rgb1 = rgb0<<4, rgb2 @+4, rgb3 = rgb2<<4); uv words repack to the WqRec convention (clut in wUv0>>16, tpage in wUv1>>16). Strictly read-only, touches no c->r[], skipped on the oracle leg. Diagnostic channel PSXPORT_DEBUG=subpartcap reports node/sub/geomblk/counts.

IT FIRES AND IT IS THE RIGHT OBJECT: on an AUTO_SKIP boot, subpartcap logs node=800FB218 walking ~26 sub-parts, each geomblk=8015CA04 with gt3=0 gt4=6 — six quads per sub-part, i.e. one box per PLANK, ~156 quads for the banner. That is the plank strip.

WHY IT IS NOT WIRED — the A/B, on a deterministic frame. replays/bugs/bucket-softlock.pad reaches the 'Go to the Burning House!' banner at replay frame 240 (PSXPORT_PAD_SHOT_AT=240). Enabling the call changes exactly 26 of 76800 pixels, ALL inside the banner band x61..252 y66..86. ~156 extra quads that move only 26 edge pixels are a SECOND COPY landing exactly on the first: the sub-parts are ALREADY drawn at guest time (guest 0x8003F698 -> the native GT3/GT4 submitters), so on a REAL frame the two copies coincide and only rounding differs — and on an INTERPOLATED frame only the WqRec copy would move, GHOSTING the planks. That is worse than the bug being fixed. The premise in this card's body — that the mesh half 'produces only guest packets, nothing the display pass can draw' — is therefore WRONG for the drawing half: the planks do get drawn, natively, at guest time. What they do not get is a display-pass record, which is why they do not lerp.

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

**2026-07-28:** 2026-07-28 third iteration — narrowing the measurement, and one dead end.

CONFIRMED there IS motion to interpolate: over the shipped-config fps60dump (250 real frames of bucket-softlock.pad), 185 of 249 consecutive REAL-frame pairs change inside the banner band, median 6489 of 9920 px. So the earlier 'nothing moves here' reading of the null result was wrong — the band moves plenty. What the null result actually shows is that the band-level classification cannot SEPARATE the banner from the scene moving behind it, so it is the wrong instrument for this question, not evidence that the planks lerp.

DEAD END (do not repeat): isolating the planks by COLOUR MASK (bright cream/tan, R>185 G>165 105<B<205, R-B>25) does not work. It matches 150 frames, but at seq 18-23 it returns exactly 1651 px on every frame including interps (a static loading screen, not the banner) and at seq 494-499 it swings 2186..4387 px on the field (foliage and terrain highlights match the same range). A colour heuristic cannot tell plank from sunlit grass here.

THE MEASUREMENT THAT WOULD SETTLE IT, for the next iteration: stop trying to find the planks in PIXELS and use the game's own state instead. The planks are sub-parts of node 800FB218 (subpartcap already logs node/sub/geomblk). Extend that channel to log each sub-part's transform translation (sub+0x18 word +0x14..0x1C) per logic frame. Two questions fall out immediately: (a) does a plank's transform change frame to frame at all — if not there is nothing to lerp and this card's premise is dead; (b) does it change by the SAME delta the glyph cmd+0x18 transform changes by — if the deltas match, the two halves cannot separate and the artefact must be elsewhere; if they differ, that difference IS the drift in the user's capture, measured in guest units instead of guessed from pixels.
That is a small, targeted change to an existing diagnostic, and it answers the card without touching the renderer.

**2026-07-28:** 2026-07-28 FOURTH ITERATION — PREMISE CONFIRMED FROM GAME STATE, AND THE FIX IS NOW WIRED.

THE MEASUREMENT THAT SETTLED IT (state, not pixels — the pixel route was a dead end, see the previous note). Extended PSXPORT_DEBUG=subpartcap to log each half's world position, and it produced two facts:

1. THE GLYPH'S cmd AND THE PLANK'S sub ARE THE SAME POINTER. textLabelEmit captures glyphs from cmd = node+0xC0[i]; subPartWalk walks sub = node+0xC0[i]. Measured on node 800FB218: the glyph log reports cmd=800F9C64 / 800F9CA8 / 800F9D30 and the plank log reports sub=800F9C64 / 800F9CA8 / 800F9CEC / 800F9D30 — identical values. So ONE transform block at +0x18 drives BOTH halves of a text-label character: its letter and the plank under it.

2. THAT SHARED TRANSFORM MOVES EVERY LOGIC FRAME. Tracking one cmd across the replay: 87 appearances, 87 DISTINCT world positions, stepping ~10-15 units per axis per frame (3609.3,-1310.4,2965.5) -> (3597.5,-1323.9,2950.5) -> (3590.4,-1338.8,2942.8) -> ...

Those two together ARE the bug, and they close the earlier null result. With only the glyph half captured, the interpolated frame drew the LETTER at the half-way position while its PLANK stayed at the real-frame position — a per-character offset of about half a frame of motion. That is precisely 'the glyphs sit at inconsistent offsets on their planks'. The band-level tile classification could not see it because it cannot separate the banner from the scene moving behind it; it was the wrong instrument, not a disproof.

WIRED. subPartCapture + mSubPartDrawSuppress are now live in subPartWalk. Verification:
  - frame 240 of bucket-softlock.pad: 26 of 76800 px vs the guest-time-draw baseline, all inside the banner band, edge rounding only — banner fully intact, planks and letters all present (scratch/screenshots/wired_f240.png).
  - no double-draw: the suppression scope means the sub-part is drawn once, by the display pass.
  - smoke: replays/boot-smoke/short-session.pad and replays/bugs/ingame-item-menu.pad both exit 0 with 0 fatal / 0 abort / 0 historical guest-entry miss.
STILL WORTH A USER EYEBALL at 60fps on a real banner — the guest-units argument and the still-frame agreement are strong, but only the moving picture proves the letters now stay on their planks. #16 (sign text) and #23 are the same emitter and should be re-checked in the same pass.

**2026-07-28:** 2026-07-28 FIX VERIFIED IN GUEST UNITS — 1740 agree / 0 differ.

With the handover wired, every glyph and its own plank now carry the SAME world transform in the same frame. Measured over replays/bugs/bucket-softlock.pad with PSXPORT_DEBUG=subpartcap (2088 plank records, 1740 glyph records): comparing each glyph's objT against the plank objT emitted for the SAME cmd/sub in that frame gives AGREE 1740, DIFFER 0.

That is the fix stated structurally rather than visually: both halves are now Render::WqRecs carrying one objT, so billboardsRender projects and lerps them with the same factor. They cannot drift apart on an interpolated frame any more — not 'they look aligned in a still', but 'there is no longer a mechanism by which they could separate'. Before the change only the glyph half had a record, so the letter moved to the half-way position while the plank held the real-frame position, ~half a frame of motion (the transform steps 10-15 units/axis/frame).

Remaining verification is the moving picture at 60fps, which needs a USER eyeball — no headless measurement can substitute for 'do the letters sit still on their planks now'.

**2026-08-04:** 2026-08-04 RESOLVED, as a side effect of #71's fix — and by construction rather than by correction.

#64's premise was 'the MESH half has no display-pass producer, so give it one'. A producer WAS added
for it on 2026-07-28 (Render::subPartCapture) and it worked, but it recovered the plank transform by
factoring the scene camera out of the guest's composed matrix — the tap the USER banned outright on
2026-08-04, and the measured cause of #71's vibration. So subpart_capture.cpp is now DELETED along
with every other caller of wq_factor_world.

The replacement makes #64 impossible instead of fixing it. game/render/cube_text_banner.cpp draws the
glyph AND the record's plank geomblk from ONE computed transform, in one pass, in the same frame.
The RE behind that (2026-08-04): textLabelEmit's `cmd` and subPartWalk's `sub` are literally the SAME
pointer, node+0xC0[i], so the two halves were never entitled to separate transforms in the first
place. There is now no second tier for them to drift between.

Verified: the banner renders correctly with letters seated on their planks (screenshot
scratch/screenshots/n71_fix_p0040.png), 164 prims per present with a stable count across all 17
consecutive present pairs (the pre-fix path dropped 4 of 17 comparisons to prim-count mismatches).

#16 and #23 are the SAME emitter with different strings, so this producer should cover them too — but
I have NOT reproduced or measured either, so they stay open rather than being closed on inference.
