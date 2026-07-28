---
id: 15
title: Weapon IMPACT effect missing under pc_render (hitting something)
status: todo
labels: [render]
created: 2026-07-22
updated: 2026-07-28
---

USER 2026-07-22: striking something with the weapon produces an impact effect that does not render under pc_render. Note docs/findings/render.md already records issue #39 as 'weapon chain + impact effect' fixed via withDepthTag depth-tagging - so either that fix regressed, or it covered only the chain and the impact half was never actually verified. CHECK THE EXISTING FINDING FIRST before re-deriving. Repro: free-roam, tap square next to a breakable/enemy, shot on the contact frames; compare PSXPORT_GATE=1 against PSXPORT_ORACLE=1 at identical exec state.

**2026-07-22:** NOT reproduced (2026-07-22): swing/contact frames vs the apple contraption show no impact-flash difference between renderers (atk*_p*.png). IMPORTANT correction: the old #39 'impact effect fixed via withDepthTag' claim is OBSOLETE — pc_render no longer walks the guest OT, so obj_depth tags no longer produce a picture; the successor is the #12 FUN_80027A4C tap (the #28 analysis showed the swing smear quads flow 27E5C->27A4C, so impact quads through that family now render). If the user still sees a missing impact effect, prime suspect = FUN_8003D23C effect pool (see card 14 / render.md 2026-07-22 entry). USER: please re-test impact after this build.

**2026-07-22:** 2026-07-22 (sweep agent) — unchanged, not re-attempted this session. Same note as #14: the impact trigger was never reached, and the area sweep (24 of 32 areas, card #25) confirms this class is TRIGGER-gated rather than place-gated, so warping around will not surface it.

**2026-07-23:** FIXED 2026-07-23 (fx_mesh.cpp). Root cause: the impact effect has TWO emitters and only one was owned. FUN_80033080 = { FUN_80027E5C(); FUN_800288AC(node); } - the sprite half goes through FUN_80027A4C (tapped in fx_sprite.cpp, so it rendered), the MESH half FUN_800288AC->FUN_80027768 (the two 0x3E gouraud quads that ARE the radial plume) had no native producer, and pc_render does not walk the guest OT. Fix = scoped leaf tap (pause_menu pattern; 27768 is game-wide with 17 callers) + host-side float re-derivation of the quads from the record template and the controller's composed GTE transform. Repro replays/bugs/weapon-impact-bucket.pad f654-660. Two traps recorded in docs/findings/render.md: drawWorldQuad sets has_xyf which makes fps60 skip a guest-execution-time prim at present (submitted every frame, zero pixels changed), and the burst needs the effect's own node+0x32 sort bias applied to depth or the bucket occludes it. Evidence f656 x[115,190) y[80,175): px differing from psx_render 2494 -> 2266, 622 px changed; whole-frame delta confined to bbox x136-166 y92-156. ALSO FALSIFIED: the attack button is CIRCLE, not SQUARE - every prior 'not reproduced' verdict on this card held square.

**2026-07-23:** 2026-07-23: the 2026-07-22 note leaned on the area sweep ('#25 confirms this class is TRIGGER-gated') — that sweep was pc-vs-pc and its own conclusion is falsified (see #25 note, #42/#43/#44). The 'impact effect is trigger-gated' claim is still fine on its own logic (it needs a hit to fire), but do not cite the sweep as evidence for it.

**2026-07-28:** 2026-07-28 REOPENED BY MEASUREMENT (static, no live run). USER reports the weapon impact effect is still missing. The 2026-07-23 fix is real but covers ONE path only, and the static census says that is the expected default rather than a regression.

The shared mesh writer is FUN_80027768. game/render/mesh_emit_tap.cpp is its single owner and dispatches to whichever producer SCOPE is up (FxMesh::mScope, raised only around controller FUN_800288AC; SwingFx::mInEffectDraw). With no scope up the tap returns and NOTHING is drawn — pc_render does not walk the guest OT, so guest packets are not a fallback.

Every jal 0x80027768 site in the binary, resolved to its enclosing function and cross-checked with codemap --addr: 20 distinct callers, 4 owned.
  OWNED/scoped: 0x800288AC (FxMesh controller — this card's fix), 0x80029F6C Render::dustEffectRender, 0x8002A834 SwingFx::effectDrawTick, 0x8002AB5C NativeScenePass::terrainRender.
  NO NATIVE OWNER (14): 0x80028B70, 0x8002BC9C, 0x8002C138, 0x8002C6AC, 0x8002CD18 (3 call sites), 0x8002D65C, 0x8002DF68, 0x8002F36C, 0x8002FDD0, 0x80030264 (2 sites), 0x80030D68, and the overlay four 0x8013D454, 0x8013D828, 0x8013ED08, 0x8013EF58. 0x8002AE0C exists only as an ORPHAN leaf.
So any impact whose controller is not FUN_800288AC still draws nothing. The bucket repro (replays/bugs/weapon-impact-bucket.pad f654-660) went through the one owned controller, which is why it verified green while the user still sees a gap.

NEXT, and it is derivable statically without triggering anything in-game: identify which of the 14 is the impact controller for the case the user hits (weapon type / target class), then port it as a producer. The full list is the render frontier's work-list — see docs/findings/render.md 'The two named render targets, reached STATICALLY'.

**2026-07-28:** 2026-07-28 THE CENSUS SHARPENS — these are NODE RENDER-FN POINTERS, and that names the frontier exactly.

Follow-up to this card's 20-caller census. None of the 14 unowned mesh-writer callers has a single  caller anywhere in RAM — every one is reached by FUNCTION POINTER. Checking them as pointer CONSTANTS in a field RAM dump confirms what they are: effect-node render fns installed at node+0x18, the same family Render::fieldObjectsRender already whitelists by address for type-0x20 nodes (render_walk.cpp:688+).

So the real statement is not 'the mesh writer has 20 callers'. It is: THE TYPE-0x20 RENDER-FN WHITELIST IS THE PRODUCER LIST, IT HAS 5 ENTRIES, AND AT LEAST 10 MORE EFFECT RENDER FNS ARE RESIDENT AND UNOWNED. A type-0x20 node whose render fn is not on the whitelist is SKIPPED outright — pc_render does not walk the guest OT, so those effects draw nothing at all.

  OWNED (on the whitelist): 0x80027CB4 / 0x80027E5C / 0x800281EC -> fxSpriteRender (torch + hut-roof flames, #12/#23), 0x800286CC -> fxAnimSpriteRender (dust puffs + impact starburst, #39), 0x80029F6C -> dustEffectRender (Tomba's movement puff).
  UNOWNED, resident as node render-fn pointers (occurrence count in one field dump):
    0x8002BC9C (5)   0x8002C6AC (2)   0x80028B70 (1)   0x8002CD18 (1)   0x8002D65C (1)
    0x8002DF68 (1)   0x8002F36C (1)   0x8002FDD0 (1)   0x80030264 (1)   0x80030D68 (1)
  (0x8002C138 and 0x8002AE0C reach the writer but appear as no pointer in THIS dump — they belong to another area/overlay's node set.)

WHY THIS MATTERS FOR THIS CARD: 'the impact effect is missing' needs no repro to explain. The 2026-07-23 fix covered the impact path that flows through FUN_800288AC; whichever of the ten above is the impact renderer for the case the user hits has no producer and therefore draws nothing. Identifying it is now a bounded question — ten addresses, each a self-contained render fn to RE and port — instead of an open hunt.

SUGGESTED ORDER: 0x8002BC9C first (5 resident nodes in a single field dump = the most common unowned effect), then 0x8002C6AC (2). Each one ported is a whitelist entry plus a producer, exactly like fx_sprite.cpp / fx_dust.cpp already are — and per the NATIVE PRESENTATION directive the producer draws from the node's own state, never from a tag.

**2026-07-28:** 2026-07-28 FIRST ENTRY OFF THE CENSUS PORTED — 0x8002BC9C now has a producer.

RE (scratch/decomp/fx_bc9c.c, Ghidra headless): FUN_8002BC9C reads an animation-script byte at node+0x3C (bit 0x80 = terminator, low 7 bits = index) to pick a model from the table at node+0x50, then runs FOUR iterations. Each builds the node's rotation from node+0x48 via Math::rotmat (FUN_80085480) into scratchpad 0x1F800000, applies the DAT_800A1CD4..D6 column scale (FUN_80084520), composes it with the scene camera block at 0x1F8000F8, transforms the node position (node+0x2C/+0x30) and adds the camera translation, loads the result into GTE CR0-7, and calls the shared writer FUN_80027768(model, 0, node+0x32, 0) — then advances node+0x4A by 0x400 (90 degrees). Four copies at 90-degree steps = the radial plume shape FxMesh::draw already reproduces. A second block handles node+3 in {0x14,0x15} from node+0x34.

IT NEEDED NO NEW PRODUCER, and that is the useful part of the finding. FxMesh::draw takes its transform from composedXform(c) — the GTE state the CALLER just set up — and mesh_emit_tap reads (model, clutBias, sortBias, uBias) straight from r4..r7. Both are controller-agnostic. The ONLY thing missing was a SCOPE around this caller; without one mesh_emit_tap fell through to 'no producer's scope is up' and, since pc_render never walks the guest OT, the effect drew nothing at all. Fix = armTapBc9c in fx_mesh.cpp, the same five lines as the existing 0x800288AC armTap.

VERIFIED: on replays/bugs/weapon-impact-bucket.pad the fxmesh channel now reports 30 draws where it previously reported none for this controller — real quads with real transforms (list=800A1D98 clutRow=9 bias=-80, semi=1 tp=0035 clut=7E57, screen xy0=(141,81) xy3=(157,158), depth 0.10). Smoke: short-session / ingame-item-menu / bucket-softlock all exit 0 with 0 fatal / 0 abort / 0 recomp-MISS.

WHAT THIS DOES NOT CLAIM: that this is THE effect the user reported missing. It is one of the ten unowned controllers and the most resident; whether it is the weapon impact for their case needs their eyeball. The remaining nine are unchanged and each is the same five-line shape IF its controller composes CR0-7 before calling the writer — which is worth checking per controller rather than assuming, since a controller that instead relies on a caller's transform would need a real producer.

**2026-07-28:** 2026-07-28 NINE OF TEN CENSUS ENTRIES WIRED — and the tenth is deliberately excluded.

Rather than assume the 0x8002BC9C pattern generalises, each remaining controller was checked STATICALLY for the precondition it depends on: does the controller compose its own transform into GTE control regs 0..7 (ctc2 to CR0-7) before calling the writer? FxMesh::draw reads composedXform(c) — the caller's GTE state — so a controller that does NOT set it up would have its prims drawn under a transform it does not own. Scanned each function body (prologue to next prologue) for ctc2 writes to CR0-7 plus a jal to 0x80027768:

  SCOPE-READY (CR0-7 all written, calls the writer):
    0x8002BC9C (1 call)   0x80028B70 (1)   0x8002C138 (1)   0x8002C6AC (1)
    0x8002CD18 (3 calls)  0x8002D65C (1)   0x8002DF68 (1)   0x8002FDD0 (1)
    0x80030264 (2 calls)  0x80030D68 (1)
  NOT WIRED — 0x8002F36C: reaches the writer but writes NO control register at all, so it inherits
    whatever transform its caller left set. Scoping it would draw its prims under a foreign
    transform. It needs a real producer (or an owned caller), and is left out on purpose.

All ten scope wrappers are generated from one FX_CONTROLLER_SCOPE macro in fx_mesh.cpp rather than copy-pasted, since they are three identical lines each.

SMOKE (all with PSXPORT_DEBUG=fxmesh, 1200 frames): short-session exit 0 / 0 bad / 0 draws; bucket-softlock exit 0 / 0 bad / 74 draws; weapon-impact-bucket exit 0 / 0 bad / 10 draws; ingame-item-menu exit 0 / 0 bad / 0 draws. Zero fatal, zero abort, zero recomp-MISS across all four. The #64 banner frame at f240 is visually unchanged.

WHAT IS PROVEN vs NOT: proven that these controllers now route their prims to a native producer instead of drawing nothing, and that nothing regressed on the replay set. NOT proven that each effect now looks correct — only two of the ten fire on the available replays (74 + 10 draws), and the other eight are simply unreached by any replay in the library. Those need either a scene that triggers them or a USER eyeball. Do not mark this card done on the smoke result alone.
