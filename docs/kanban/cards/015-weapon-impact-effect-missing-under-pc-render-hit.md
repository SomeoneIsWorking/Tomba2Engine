---
id: 15
title: Weapon IMPACT effect missing under pc_render (hitting something)
status: done
labels: [render]
created: 2026-07-22
updated: 2026-07-23
---

USER 2026-07-22: striking something with the weapon produces an impact effect that does not render under pc_render. Note docs/findings/render.md already records issue #39 as 'weapon chain + impact effect' fixed via withDepthTag depth-tagging - so either that fix regressed, or it covered only the chain and the impact half was never actually verified. CHECK THE EXISTING FINDING FIRST before re-deriving. Repro: free-roam, tap square next to a breakable/enemy, shot on the contact frames; compare PSXPORT_GATE=1 against PSXPORT_ORACLE=1 at identical exec state.

**2026-07-22:** NOT reproduced (2026-07-22): swing/contact frames vs the apple contraption show no impact-flash difference between renderers (atk*_p*.png). IMPORTANT correction: the old #39 'impact effect fixed via withDepthTag' claim is OBSOLETE — pc_render no longer walks the guest OT, so obj_depth tags no longer produce a picture; the successor is the #12 FUN_80027A4C tap (the #28 analysis showed the swing smear quads flow 27E5C->27A4C, so impact quads through that family now render). If the user still sees a missing impact effect, prime suspect = FUN_8003D23C effect pool (see card 14 / render.md 2026-07-22 entry). USER: please re-test impact after this build.

**2026-07-22:** 2026-07-22 (sweep agent) — unchanged, not re-attempted this session. Same note as #14: the impact trigger was never reached, and the area sweep (24 of 32 areas, card #25) confirms this class is TRIGGER-gated rather than place-gated, so warping around will not surface it.

**2026-07-23:** FIXED 2026-07-23 (fx_mesh.cpp). Root cause: the impact effect has TWO emitters and only one was owned. FUN_80033080 = { FUN_80027E5C(); FUN_800288AC(node); } - the sprite half goes through FUN_80027A4C (tapped in fx_sprite.cpp, so it rendered), the MESH half FUN_800288AC->FUN_80027768 (the two 0x3E gouraud quads that ARE the radial plume) had no native producer, and pc_render does not walk the guest OT. Fix = scoped leaf tap (pause_menu pattern; 27768 is game-wide with 17 callers) + host-side float re-derivation of the quads from the record template and the controller's composed GTE transform. Repro replays/bugs/weapon-impact-bucket.pad f654-660. Two traps recorded in docs/findings/render.md: drawWorldQuad sets has_xyf which makes fps60 skip a guest-execution-time prim at present (submitted every frame, zero pixels changed), and the burst needs the effect's own node+0x32 sort bias applied to depth or the bucket occludes it. Evidence f656 x[115,190) y[80,175): px differing from psx_render 2494 -> 2266, 622 px changed; whole-frame delta confined to bbox x136-166 y92-156. ALSO FALSIFIED: the attack button is CIRCLE, not SQUARE - every prior 'not reproduced' verdict on this card held square.

**2026-07-23:** 2026-07-23: the 2026-07-22 note leaned on the area sweep ('#25 confirms this class is TRIGGER-gated') — that sweep was pc-vs-pc and its own conclusion is falsified (see #25 note, #42/#43/#44). The 'impact effect is trigger-gated' claim is still fine on its own logic (it needs a hit to fire), but do not cite the sweep as evidence for it.
