---
id: 16
title: Sign text jitters at fps60 when the camera moves
status: todo
labels: [render, fps60]
created: 2026-07-22
updated: 2026-08-04
evidence: docs/reference/issues/issue16_sign_text_jitter.png
---

USER 2026-07-22 with a capture of the 'Go to the burning House!' sign. At fps60 the text jitters while the camera moves; the capture also shows GLYPHS DROPPING OUT (reads 'urnIn' and 'Ho se' - the b, u and letters are missing), so this may be two faults or one. STRONG PRIOR, do not re-derive: the fps60 audit (docs/findings/render.md, tagging-purge entry) states the honest remainder as 'RQ_WORLD items with has_xyf==0 and screen-space HUD/2D present VERBATIM on both frame kinds - they step at 30 Hz'. A verbatim 2D layer held still for one frame while the camera lerps underneath is EXACTLY a jitter against the 60Hz world. So the fix is the stated one: give this producer a display-pass native producer so it interpolates like the rest, NOT a matcher and NOT an anchor/stamp special case (banned - see CLAUDE.md NATIVE PRESENTATION lerp clause). Find the text-label emitter (game/render/text_label.cpp is the likely owner) and route it through the display pass. The dropped glyphs need their own root cause - check whether they are dropped on the interp frame only (then it is the same verbatim path) or on both (then it is an emitter/atlas fault).

**2026-07-23:** 2026-07-23 (investigated during #33). Same class as #23: screen-space HUD/2D text presents VERBATIM on both fps60 frame kinds (has_xyf==0 / screen-space), so it steps at 30Hz while the camera lerps under it -> jitter. Fix = the same REDIRECT graduation as #23: the text-label emitter must re-derive under the lerped camera at present (its anchor re-projected), not a matcher/anchor-stamp. The dropped-glyph half is a SEPARATE root cause (check whether glyphs drop on the interp frame only = same verbatim path, or on both = an emitter/atlas fault) and must be diagnosed independently. Deferred with #23 as the parent #31 redirect work.

**2026-07-28:** 2026-07-28: SAME EMITTER AND SAME BUG AS #64 — do not work these separately. The 'Go to the Burning House!' sign is entry 2 of the 12-byte string table at 0x800A33C8 that Render::textLabelEmit (FUN_80039F4C) reads; the quest-update banner the user reported on 2026-07-28 ('A Red Treasure Chest') is entry 56 of that SAME table, and the banner shown at game start is entry 1. One emitter, three strings.

Root cause, established statically in #64: textLabelEmit draws the node in two halves and only the GLYPH half produces a display-pass record (a Render::WqRec per character, corners + the cmd+0x18 matrix factored against the scene camera, emitted through the float camera path). The MESH half — guest 0x8003F174 = Render::subPartWalk, the sub-part transforms + geomblk submits that draw the signboard/planks — captures nothing and only emits guest packets. So letters lerp at 60fps and the board steps at 30Hz. That is this card's jitter and #64's per-letter offsets, from one cause.

This card's SECOND half (glyphs dropping out, 'urnIn' / 'Ho se') is still a separate root cause and still needs its own diagnosis — see the note below. It is NOT explained by the two-tier split.

**2026-07-28:** 2026-07-28: THE JITTER HALF SHOULD NOW BE FIXED — re-test. This card's sign ('Go to the Burning House!') is entry 2 of the string table at 0x800A33C8 and is drawn by Render::textLabelEmit, the same emitter as #64's quest banner. #64's root cause was measured this session and fixed: a character's glyph cmd and its plank sub are THE SAME POINTER (node+0xC0[i]), that shared transform moves 10-15 units/axis every logic frame, and only the GLYPH half had a display-pass record — so on interpolated frames the letters moved to the half-way position while the boards held the real-frame position. subPartCapture + mSubPartDrawSuppress (subpart_walk.cpp) now give the mesh half a record too; verified 1740 agree / 0 differ on same-frame glyph-vs-plank objT.

Since this card's symptom is 'text jitters WHILE THE CAMERA MOVES' and the mechanism was a per-frame transform delta being applied to one half only, the same fix should cover it. RE-TEST at 60fps with the camera panning past the sign before doing any further work here.

The DROPPED-GLYPH half of this card ('urnIn' / 'Ho se') is untouched by that fix and still needs its own root cause — it is an emitter/atlas question, not a presentation-tier one.

**2026-08-04:** 2026-08-04: the emitter behind this (FUN_80039F4C text-label / cube-text banner) now has a real native producer — game/render/cube_text_banner.cpp, see #71 and #64. It projects in view space with no camera term, so the camera-driven jitter class is structurally gone for every string this emitter draws. This card is NOT closed on that inference: nobody has reproduced #16's sign text or measured it. Reproduce it and re-measure with tools/preseqobj_check.py --node before closing.
