---
id: 16
title: Sign text jitters at fps60 when the camera moves
status: todo
labels: [render, fps60]
created: 2026-07-22
updated: 2026-07-23
evidence: docs/reference/issues/issue16_sign_text_jitter.png
---

USER 2026-07-22 with a capture of the 'Go to the burning House!' sign. At fps60 the text jitters while the camera moves; the capture also shows GLYPHS DROPPING OUT (reads 'urnIn' and 'Ho se' - the b, u and letters are missing), so this may be two faults or one. STRONG PRIOR, do not re-derive: the fps60 audit (docs/findings/render.md, tagging-purge entry) states the honest remainder as 'RQ_WORLD items with has_xyf==0 and screen-space HUD/2D present VERBATIM on both frame kinds - they step at 30 Hz'. A verbatim 2D layer held still for one frame while the camera lerps underneath is EXACTLY a jitter against the 60Hz world. So the fix is the stated one: give this producer a display-pass native producer so it interpolates like the rest, NOT a matcher and NOT an anchor/stamp special case (banned - see CLAUDE.md NATIVE PRESENTATION lerp clause). Find the text-label emitter (game/render/text_label.cpp is the likely owner) and route it through the display pass. The dropped glyphs need their own root cause - check whether they are dropped on the interp frame only (then it is the same verbatim path) or on both (then it is an emitter/atlas fault).

**2026-07-23:** 2026-07-23 (investigated during #33). Same class as #23: screen-space HUD/2D text presents VERBATIM on both fps60 frame kinds (has_xyf==0 / screen-space), so it steps at 30Hz while the camera lerps under it -> jitter. Fix = the same REDIRECT graduation as #23: the text-label emitter must re-derive under the lerped camera at present (its anchor re-projected), not a matcher/anchor-stamp. The dropped-glyph half is a SEPARATE root cause (check whether glyphs drop on the interp frame only = same verbatim path, or on both = an emitter/atlas fault) and must be diagnosed independently. Deferred with #23 as the parent #31 redirect work.
