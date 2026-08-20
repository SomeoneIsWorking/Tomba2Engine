---
id: 109
title: Costume change: native render explodes into full-screen garbage polygons (PSX path is correct)
status: todo
labels: [render, bug]
created: 2026-08-20
updated: 2026-08-20
---

USER 2026-08-20, LIVE windowed run (pc_faithful + pc_render). Changing costumes fills the whole screen with huge stretched multicoloured triangles; only the dialogue box ('Bird Clothes removed!') stays legible. Evidence: scratch/screenshots/live/costume_now.png, captured from the user's live session over the debug server at present-frame f8471.

THE SAME MOMENT ON THE PSX PATH IS CORRECT — user-supplied screenshot: a clean white/pink transformation FLASH around Tomba, with 'equipped!' in the dialogue box. So the guest's effect is fine and intact; what is broken is OUR NATIVE PRODUCER for it. This is a pc_render bug, not a faithfulness bug.

MEASURED ON THE LIVE SESSION, at the corrupt frame:
  vkstats      textured = 172,878 verts (57,626 tris);  semi = 64,164 verts (21,388 tris);  flat = 0
  scene        guest OT holds ONE black FILL rect. poly=0 rect=0 line=0 fill=1.
The second line is the important one: the guest ordering table contains NO geometry at all this frame, so every one of those ~79,000 triangles was submitted by a native producer. Nothing is being mis-copied from the guest OT — a native producer is generating them.

~21k SEMI-TRANSPARENT triangles is the signature to chase: the PSX reference shows this effect IS a big additive flash, i.e. a large number of semi quads is EXPECTED here. So the producer is probably the right one and is being reached correctly — its CORNERS are wrong, which is what turns a compact flash into screen-spanning stretched triangles. That is the shape of a producer projecting with the wrong transform contract rather than one drawing the wrong object.

WHY THAT IS A LIVE SUSPICION AND NOT A GUESS (#103 context, same day): Tomba!2's effect emitters funnel through FUN_8003B320 (game/render/quad_rtpt_submit.cpp), and ELEVEN distinct guest functions call it, EACH composing its own GTE CR0-7 contract immediately before the call. A native producer that assumed the wrong caller's contract gets corners in the wrong space — which renders exactly like this. quad_rtpt_submit.cpp's own banner says the contracts do not transfer between callers ('B704's answer does NOT transfer — case-188 loads CR0-7 from CASE188_SCR, not 0x1F8000F8').

NEXT STEP, cheap and decisive: re-enter the costume change and toggle renderpath native<->psx live (F5, or 'renderpath' on the debug server) to confirm the split, then bisect WHICH producer by layer — the frame is ~57.6k textured + 21.4k semi tris against a normal field frame of a few thousand, so whichever producer's count collapses when the effect ends is the one. 'debug rqhist' (layer x opaque/semi census) is the existing instrument for that split; it is REPL-only today.

TOOL GAP FOUND WHILE INVESTIGATING, worth fixing regardless of this bug: 'provat' cannot attribute a NATIVELY-drawn pixel. It reads guest VRAM, and under pc_render the native producers never write VRAM, so every probe into the corruption returned '<never written> rgb(0,0,0)' for pixels that are plainly not black on screen. The one instrument for 'which prim drew this pixel' is blind on the only render path we ship. It needs a native-leg answer sourced from the RenderQueue (each RqItem already carries dbg_node and a producer scope), or it will keep returning a confident, wrong-looking negative.

ALSO OBSERVED on the live session, relevant to #108: 127 entities on the list, 27 with the render flag set.
