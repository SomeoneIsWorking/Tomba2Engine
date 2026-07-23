---
id: 57
title: Hut interior wall/room decorations missing — likely occluded (possible #29 regression)
status: todo
labels: [render, bug]
created: 2026-07-23
updated: 2026-07-23
evidence: docs/reference/issues/issue57_hut_wall_decor_missing.png, docs/reference/issues/issue57_hut_wall_decor_missing_2.png
---

USER 2026-07-23 with a side-by-side: in the hut interior the WALL DECORATIONS (anchor, hanging rope coils, wall-mounted items) are absent under pc_render; the reference shows them on the wall. The user's own read — "probably occluded by the wall" — is the most likely mechanism and should be tested first.

CHECK KANBAN #29 FIRST — THIS MAY BE A REGRESSION FROM IT. #29 was "hut interior wall decorations Z-FIGHT with the wall", fixed 2026-07-23 by extending RenderQueue::resolveKeyOrder: a decal and the wall quad it sits on are EXACTLY COINCIDENT (identical four projected corners and depths, different texpage/CLUT) but listed with ROTATED vertex order, so our fixed (0,1,2)+(1,2,3) triangulation split them on opposite diagonals and each won half the pixels. The fix detects exact coincidence and snaps such a pair to their shared key ord, so DEPTH IS EQUAL EVERYWHERE AND SUBMISSION ORDER DECIDES (GREATER_OR_EQUAL + seq).
That is precisely a mechanism that can flip "z-fighting" into "consistently behind": if the wall is submitted AFTER the decal within the same object, the wall now wins every pixel and the decoration disappears cleanly instead of flickering. Symptom changing from flicker to total absence, on the same objects, on the same day the fix landed, is a strong signal.

FIRST DIAGNOSTIC (cheap, decisive): capture the interior and run `PSXPORT_DEBUG=keyord` — it logs every snapped face with seq, node, key and bbox. If the decal and wall are being snapped as a coincident pair, check their relative `seq`. If the decal's seq is LOWER than the wall's, the decal is drawn first and loses; that is the bug and the fix is in the ordering, not a missing producer. Cross-check with PSXPORT_PRIMAT="x,y,frame" on a pixel where a decoration should be: if BOTH prims are present in the pc queue and the wall simply wins, it is occlusion (this card); if the decoration prim is ABSENT from the queue, it is a missing producer instead (different fix).
Also verify against psx_render from a SECOND run with PSXPORT_RENDER_PSX=1 at the same frame index — never a bare mid-scene `renderpsx` toggle (kanban #41).

REPRO: the hut interior is reachable headlessly with replays/scene-transitions/hut-entry-door-freeze.pad, NO `newgame`, `run 1200` — that lands standing inside the room with the wall decorations on screen (recorded during #29). `press left` + `run 80` gives a second interior viewpoint.

DO NOT "fix" this by reverting #29 — the z-fighting it cured is a real defect and the user has objected to reverting working fixes. If the ordering is wrong, correct the ordering (which of the coincident pair the guest actually intends in front — its own OT/link order is the ground truth), and re-verify BOTH symptoms: no flicker AND decorations visible.
