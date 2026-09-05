---
id: 14
title: Weapon CHARGE effect missing under pc_render (hold attack)
status: done
labels: [render]
created: 2026-07-22
updated: 2026-08-21
evidence: docs/reference/issues/issue14_charge_swing.png
---

USER 2026-07-22: holding the attack button charges the weapon and that charge effect does not render under pc_render. Same family as the torch flame (#12) and the HUD carousel (#13) - a whole effect layer simply absent. Repro: reach free-roam then HOLD square (REPL: press sq / run N / shot). Likely the #39 precedent: an effect emitter not wrapped in a depth-tag scope has its prims dropped by the field 2D-only OT walk (withDepthTag, game/render/render_internal.h; s_ot_2d_only in runtime/psx/gpu_native.cpp). Verify with the pc-vs-oracle method: PSXPORT_GATE=1 vs PSXPORT_ORACLE=1 at identical exec state, diff the frames.

**2026-07-22:** NOT reproduced (2026-07-22): hold-square on land (teleport w 800E7EAE) shows NO charge effect on the psx_render reference either with the newgame blackjack (chg*_pc/psx.png) — needs a later-game weapon/scenario or the exact input recipe. Coverage improved anyway: 27A4C-family effect quads now render via the #12 tap; billboardEmit/submitQuad already display-pass. Prime remaining suspect if still missing: effect pool 0x800EC188 walked by FUN_8003F024 -> FUN_8003D23C (7-word textured-tri emitter, fully RE'd in scratch/decomp/hud_fx_leaves.c + render.md 2026-07-22 entry) — unowned, wants the display-pass treatment. USER: please re-test charge after this build and name the weapon used.

**2026-07-22:** REPRO FROM USER 2026-07-22 (this is what was missing - the earlier attempt never hit the trigger, which is why the effect was absent on the psx_render reference too and looked unreproducible): HOLD the attack button for A FEW SECONDS. Tomba then starts SWINGING the weapon (mace on a chain) continuously - see docs/reference/issues/issue14_charge_swing.png, captured mid-swing. That sustained swing is SUPPOSED to produce an effect, and does not under pc_render. So the trigger is not a tap and not a brief hold; it is a multi-second hold that transitions Tomba into a swing state. Drive it with REPL: press square, run ~180+ frames (3s at 60Hz) WITHOUT releasing, shot every ~10 frames through the swing. Compare pc_render against psx_render IN ONE PROCESS via the renderpsx toggle on the DEFAULT leg (GATE-vs-ORACLE is blind to taps). Note the swing state is also where the #28 chain/smear quads live, so check whether the existing FUN_80027A4C sprite tap already covers part of this before porting anything new.

**2026-07-22:** 2026-07-22 (sweep agent) — NOT attempted with the corrected multi-second-hold repro; the long-hold instruction arrived after this session's budget was spent on the area sweep and the recorded binary evidence blocker. What IS ruled out: a SHORT hold does nothing — 12 A/B/C samples across 12 frames of held square at the seaside (sweep tag atk1..atk12) show 100-230 px renderer-attributable, all motion/edge residual, no effect layer on either renderer. So the previous 'not reproduced' result was a repro failure, exactly as the USER says, and the multi-second hold is still the thing to drive.

**2026-07-23:** 2026-07-23 FIXED. THE CARD'S REPRO LINE WAS WRONG: attack is CIRCLE (0x2000), not square — that single error is why two sessions logged 'not reproduced', and the claim that psx_render shows no effect either is FALSE (it draws the starburst plainly). Root cause: the effect object (beh FUN_8002A584, node 0x800EEA60) renders via FUN_8002A834, which composes a per-particle transform into the GTE CRs and calls the SHARED packed-mesh emitter FUN_80027768 (model 0x8009FB0C) ten times = the 60 op-0x3E POLY_GT4 packets per swing frame; pc_render walks no OT and that emitter had no native producer. Fix: game/render/swing_fx.cpp — class SwingFx, a SCOPED tap (scope on 0x8002A834, re-derivation on 0x80027768; the scope is mandatory because 0x80027768 also serves the natively-produced terrain). Faces re-derived host-side from the model + the composed CRs + the guest's DPCT/DPCS far-colour lerp (IR0=0xFFF); the stale leaf_80027768 transcription was deleted so the address has one owner (codemap --conflicts stays 18). Verified: spike-pixel count in the mace crop 175 -> 416 px (psx reference 381); dialog/menu/plain-field regression frames 0/76800 px differ. TOOLING TRAP recorded in findings: REPL 'shot' lags one frame — always 'run 2' after 'renderpsx on|off'. DEAD END recorded: a guest-execution-time producer must not use drawWorldQuad (has_xyf=1 makes Fps60::isTier1Owned skip it on BOTH presents — 60 quads/frame and a byte-identical picture). NOT covered here: the swing TRAIL (beh 0x80029B40 / renderer 0x80029F6C, POLY_GT3) and the mace ball itself — card #15's lead.
**2026-07-23:** 2026-07-23: the effect-MESH producer that fixed #15 (game/render/fx_mesh.cpp, taps FUN_800288AC/FUN_80027768) is very likely the same missing producer here - the swing effect object 0x800EE9D8 / beh 0x800293F4 alternates its renderer between FUN_80027E5C (sprite, covered by fx_sprite.cpp) and FUN_800288AC (mesh, now covered). Re-test the CHARGE effect against this build before closing. NOTE the attack button is CIRCLE, not SQUARE as this card says - that is why it kept coming back 'not reproduced'.

**2026-07-23:** 2026-07-23: the 2026-07-22 sweep-agent note ('12 A/B/C samples show no effect layer on EITHER renderer') was pc-vs-pc — the bare-renderpsx reference was a no-op, so only pc_render was ever sampled; it says nothing about psx. Moot now (this card was FIXED 2026-07-23 by the CIRCLE-hold repro). Flagged only so the retraction is complete. See docs/findings/render.md 2026-07-23.

**2026-08-06:** REOPENED 2026-08-06 by the G10 unported-render survey. This card was closed citing game/render/fx_mesh.cpp (the effect-MESH producer). THAT FILE NO LONGER EXISTS: commit abf3cf9 'Delete the GTE-register render taps; four layers are now honestly absent' removed fx_mesh.cpp/.h, mesh_emit_tap.cpp and swing_fx.cpp/.h on 2026-08-04. mesh_emit_tap.cpp was the SINGLE owner of the shared writer FUN_80027768 and dispatched to whichever controller SCOPE was up, so with it gone nothing draws the family at all — pc_render does not walk the guest OT, so the guest packets are not a fallback. tools/codemap.py --addr now answers NO NATIVE OWNER for 0x80027768, 0x800288AC, 0x8002BC9C and 0x8002A834 (SwingFx::effectDrawTick, this card's own fix). The deletion was CORRECT (those producers read the transform out of GTE CR0-7 = a tap, banned by PROTOCOL.md); what is wrong is that this card still reads 'done'. Tracked as portmap step render-producer-effect-mesh-family; full inventory in docs/unported-render-inventory.md item R1. NOT VERIFIED BY ME: I did not reproduce the charge effect on screen — this reopen rests on the ownership query plus the deleted-file check, not on a capture.

**2026-08-21:** FIXED AGAIN, this time without restoring the deleted tap. The durable repro is
`replays/bugs/weapon-charge-starburst.pad`: it holds CIRCLE from pad frame 620 through 1000, and the true
`PSXPORT_SBS_MODE=oracle` software pane begins drawing the lavender starburst at lockstep f667. Root
cause remains the missing picture owner for controller `FUN_8002A834`, but the replacement is now a
display-pass producer (`Render::swingStarburstRender`, `game/render/fx_swing.cpp`) built entirely from
the controller's persistent state: ten `{angleX,angleY,angleZ,scale}` byte records at node+0x50, the
node's world anchor at +0x2C, owner-selected far colour, fixed mesh 0x8009FB0C, IR0=0xFFF and the
writer's measured sort bias. No GTE register, guest packet, OT or shared-writer tap is read.

Fresh bounded true-oracle run on the rebuilt Clang binary: core B identifies itself as
`PURE-ORACLE(interp+softGPU)`; the native channel emits exactly 10 copies / 60 quads on every f667+
frame sampled. Before the producer, native A differs from its deterministic post-fix capture by 843,
642, 939 and 1001 pixels at f670/f680/f690/f700, with every delta confined to the starburst boxes
`(54,94)-(133,158)`, `(44,92)-(127,157)`, `(39,92)-(140,162)` and
`(38,75)-(140,157)`. At f650/f660, before the controller fires, the delta is exactly 0 pixels. The B
pane is byte-identical before/after at all six sampled frames, proving the implementation did not
contaminate the oracle. Contact sheet: `scratch/screenshots/c14_charge_route_post_sheet.png`; fresh
log: `scratch/logs/c14_charge_fresh.log`. The final combined-tree rerun retained all 38 producer
telemetry lines exactly and kept all six B captures byte-identical; its A pane also contains the
separate #55/#72 fix, so that whole-frame delta is not attributed to this card.
