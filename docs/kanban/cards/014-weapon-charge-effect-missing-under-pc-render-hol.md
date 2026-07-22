---
id: 14
title: Weapon CHARGE effect missing under pc_render (hold attack)
status: todo
labels: [render]
created: 2026-07-22
updated: 2026-07-22
evidence: docs/reference/issues/issue14_charge_swing.png
---

USER 2026-07-22: holding the attack button charges the weapon and that charge effect does not render under pc_render. Same family as the torch flame (#12) and the HUD carousel (#13) - a whole effect layer simply absent. Repro: reach free-roam then HOLD square (REPL: press sq / run N / shot). Likely the #39 precedent: an effect emitter not wrapped in a depth-tag scope has its prims dropped by the field 2D-only OT walk (withDepthTag, game/render/render_internal.h; s_ot_2d_only in runtime/recomp/gpu_native.cpp). Verify with the pc-vs-oracle method: PSXPORT_GATE=1 vs PSXPORT_ORACLE=1 at identical exec state, diff the frames.

**2026-07-22:** NOT reproduced (2026-07-22): hold-square on land (teleport w 800E7EAE) shows NO charge effect on the psx_render reference either with the newgame blackjack (chg*_pc/psx.png) — needs a later-game weapon/scenario or the exact input recipe. Coverage improved anyway: 27A4C-family effect quads now render via the #12 tap; billboardEmit/submitQuad already display-pass. Prime remaining suspect if still missing: effect pool 0x800EC188 walked by FUN_8003F024 -> FUN_8003D23C (7-word textured-tri emitter, fully RE'd in scratch/decomp/hud_fx_leaves.c + render.md 2026-07-22 entry) — unowned, wants the display-pass treatment. USER: please re-test charge after this build and name the weapon used.

**2026-07-22:** REPRO FROM USER 2026-07-22 (this is what was missing - the earlier attempt never hit the trigger, which is why the effect was absent on the psx_render reference too and looked unreproducible): HOLD the attack button for A FEW SECONDS. Tomba then starts SWINGING the weapon (mace on a chain) continuously - see docs/reference/issues/issue14_charge_swing.png, captured mid-swing. That sustained swing is SUPPOSED to produce an effect, and does not under pc_render. So the trigger is not a tap and not a brief hold; it is a multi-second hold that transitions Tomba into a swing state. Drive it with REPL: press square, run ~180+ frames (3s at 60Hz) WITHOUT releasing, shot every ~10 frames through the swing. Compare pc_render against psx_render IN ONE PROCESS via the renderpsx toggle on the DEFAULT leg (GATE-vs-ORACLE is blind to taps). Note the swing state is also where the #28 chain/smear quads live, so check whether the existing FUN_80027A4C sprite tap already covers part of this before porting anything new.
