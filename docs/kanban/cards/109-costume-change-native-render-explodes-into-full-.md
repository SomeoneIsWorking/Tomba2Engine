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

**2026-08-20 — THE RETAINED REPLAY DOES NOT REPRODUCE THIS FRAME. Do not use `f8471` as a replay
coordinate.** A direct replay of `replays/bugs/long-session-many-bugs.pad` with the existing Clang-built
binary shows an ordinary outdoor field-dialogue frame at present 8471, not an
outfit transition or corrupt geometry. This is not a timing-near-miss around an input: the replay's raw
16-bit pad words are `0xFFFF` (PAD_NONE) continuously from pad frame 6500 through 13194. The live
present-frame label in the original report belonged to that already-running debug session; it is not a
coordinate in the retained pad file. No pad cut, guest-RAM dump, or producer trace from the 01:24 live
failure survives alongside `costume_now.png`. Therefore the screenshot plus aggregate vkstats can prove
the native/guest split, but cannot identify which native producer or which transform/state contract was
active. The earlier `submitQuad` paragraph is a hypothesis, not a root cause.

STATIC RE NARROWING, NOT A CAUSE CLAIM: MAIN `gen_func_8004DF94` is an equippable-item controller. It
subtracts 102 from its item id and dispatches a nine-way table. One branch reads
`*(u32*)0x800BF880 & 0x3000` and calls `Engine::gStateMutate(G=0x800E7E80, op=4)` when clear or `op=5`
when set. The native `gStateMutate` body shows op 4 setting `G+0x174` bit `0x10` and `G+0x0D` bits
`0x12`, while op 5 clears the two `G+0x174` costume-shaped bits (`0x30`) and conditionally clears the
same `G+0x0D` bits. Separately, `Render::fieldObjectsRender` draws Tomba whenever `G+8` and `G+9` are
nonzero by calling the generic `perObjFlush`, without an outfit-specific branch. That is a concrete
place where a state-dependent guest render contract could diverge from the generic native walk, but the
failed frame's `G` bytes and command records were not captured, so changing a gate here would be a guess.

EXACT EVIDENCE NEEDED ON THE NEXT LIVE REPRO, captured for the corrupt frame and one adjacent clean frame:

1. `G=0x800E7E80`: bytes `G+0x0D`, `G+0x174`, command counts `G+8/G+9`, every command pointer at
   `G+0xC0`, and each command's `cmd+0x40` geomblk pointer/count word;
2. whether the guest's own per-object submission path submitted Tomba on that frame (including its
   per-object early gate and resolved emitter), rather than only the final guest-OT primitive census;
3. native RenderQueue counts grouped by producer/debug node for that single frame, so the 57,626
   textured and 21,388 semi triangles can be assigned to a producer instead of inferred from appearance;
4. a `padrec save` cut or RAM snapshot before leaving the frame, so the clean/corrupt comparison can be
replayed under both render paths.

Until that evidence exists, this card remains `todo`: the live bug is real, but a systemic transform or
producer fix is not yet proven.

VERIFICATION LIMIT: a fresh CMake tree configured successfully with Clang 22.1.8 for both C and C++ and
resolved psxport exactly to `b80e115c5a6c25950763e3a51a40ab32d068bd37`. Its full link was stopped at
88% when the investigation was ended to free the agent slot, so no claim here depends on a new
`b80e115c` runtime capture.
