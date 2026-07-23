---
id: 39
title: Movement DUST CLOUDS never spawn — missing on the ORACLE too, so it is not a render gap
status: done
labels: [bug]
created: 2026-07-23
updated: 2026-07-23
---

USER 2026-07-23: Tomba is supposed to kick up dust clouds on certain movement actions — starting to walk, turning while running, probably others the USER cannot enumerate ('RE should make it more clear'). They are missing under pc_render AND under the oracle.

WHY THAT SENTENCE IS THE WHOLE DIAGNOSIS: PSXPORT_ORACLE=1 is pure recomp exec + pure PSX render. If the effect is absent there, no renderer is at fault and this is NOT the missing-native-producer class that #12/#13/#21 belonged to — the prims are never SUBMITTED, so the spawn/trigger path in guest execution never runs (or runs and immediately despawns). Do not go looking for a producer to write; find why the emitter never fires. Same reasoning that settled #11 and #21: read what the guest submits, not what the screen shows.

CONTRADICTION TO RESOLVE FIRST, it is the sharpest lead available: docs/reference/issues/issue8_9_re.md (2026-07-10) is titled 'Walking dust clouds render as VERTICAL BARS' and describes the dust puffs Tomba kicks up while walking as PRESENT but drawn wrong (prims sampling beyond their texpage cell because the GP0 0xE2 texture-window confinement is not in force at draw time; an earlier commit 2a11b4f 'fixed' it by dropping u&=255, which that doc correctly calls a bandaid). Absent and drawn-wrong cannot both be true of the same effect at the same time. So EITHER (a) something regressed between 2026-07-10 and now and dust stopped spawning entirely — bisect-free: find the spawn site and check whether it still runs; OR (b) they are DIFFERENT effects — sustained-walking dust (the old report) versus start-walk / turn-while-running dust (this report) — in which case the old one may still draw as bars and only the transient ones are missing. Establish which before anything else; the answer changes the whole task.

WHAT THE PRIOR INVESTIGATION ESTABLISHED, do not redo: that doc is about HOW THE DUST IS DRAWN, not how it is MADE, which is exactly the gap the USER is pointing at. Carry over only its repro intelligence — 'dust = sustained walking', 'animation-gated (not reliably headless-reachable)', and that a previous session explicitly FAILED to catch the dust prim headless: blind press-right / AUTO_WALK produced no dust sprite (scratch/screenshots/dust_w2.png, dust_w3.png, walk1.png). Assume catching it is the hard part and budget for it. Starting to walk and turning while running are TRANSIENT one-shot triggers, which may actually be easier to provoke deterministically than sustained walking was: tap a direction, or press one direction then the opposite while at run speed.

RE TARGET: the movement state machine that requests the effect — ActorTomba and the walk/run state transitions (game/player/, and Engine::walkStart 0x80054D14 is already owned). Find the spawn call, then find its gate. Tools: 'debug otattr' attributes OT packets to emitting function AND caller; tools/find_refs.py <dump> <addr> --rw r finds who reads a state word; tools/decomp.sh for Ghidra. Also worth checking whether the effect family is the 8-byte-sprite-record family already tapped in game/render/fx_sprite.cpp (FUN_80027A4C and callers 27CB4/27E5C/281EC) — issue8_9_re.md line 3149 of findings/render.md associates the 27A4C caller family with grab/dust semi-quads, so the DRAW side may already be owned while the SPAWN side is not.

DELIVERABLE THE USER ASKED FOR EXPLICITLY: the RE should produce the LIST of actions that spawn dust, since they do not know it themselves. Record it in the finding.

---

RESOLVED 2026-07-23 (see docs/findings/effects.md — full RE + evidence). The premise "missing on the
ORACLE too, so not a render gap" is FALSIFIED by direct measurement: the dust effect DOES spawn and
DOES render on the oracle. It is missing ONLY under pc_render, and is the SAME missing-native-producer
class as #12/#13/#21.

Evidence: temporary logging override on the dust-spawn decision FUN_80053670 / pool-spawn FUN_800534B0
showed the emitter FIRES (seesaw-weight.pad: state 6 walk-start → SPAWN subtype 1). Screenshots at the
spawn frame: psx_render (dust_psx_310.png) and true PSXPORT_ORACLE=1 (orsweep_320.png) BOTH show the
white dust puff; pc_render (dust_pc_310.png) shows nothing.

ACTION LIST (which movement actions spawn dust): starting to walk (state 6, FUN_8005D16C, kind 1),
mid-walk transition (kind 2), run→walk/skid (states 5/50, FUN_8005C8A0, SFX 0x1D), turn-while-running
(state 7, FUN_8005CDF8, SFX 0x1D, elevated spawn). Puff style = surface material (FUN_800535D4 =
material+1); no dust on materials >9; stopping does a material-refresh only (p3=8, no spawn).

ROOT CAUSE: the whole dust family (spawn FUN_80053670/800534B0 → controllers 8006AE28/8006BB6C/
8006C478 → tick FUN_80029B40 → GTE render FUN_80029F6C) is UNOWNED substrate; pc_render no longer
walks the guest OT (break-first rebuild 2026-07-15) so there is no native producer → invisible.

FIX (scoped, not done — deferred render work in operator-owned game/render/): native producer tapping
FUN_80029F6C, like fx_sprite.cpp taps FUN_80027A4C. Reclassify as a pc_render missing-producer bug,
NOT an emitter bug. issue8_9_re.md is the SAME effect (pc_render bars pre-2026-07-15 → absent after).

**2026-07-23:** PORTED 2026-07-23. Native producer Render::dustEffectRender (game/render/fx_dust.cpp) on render fn FUN_80029F6C, dispatched from fieldObjectsRender's type-0x20 walk (the dust visual node is stamped type 0x20 + beh FUN_80029B40 + rf FUN_80029F6C by FUN_80031558). Real port: no gen_func body and no gte_op in the picture path; transform rebuilt host-side (MeshQuads) and composed with the fps60-lerped camera. Verified vs the guest on seesaw-weight f226: all six trail quads within 2px of the guest's own submissions. Lerps at fps60 through the new EffectLerp tier (interp frame measured between its two real neighbours). Same pass added Render::fxAnimSpriteRender for the sibling FUN_800286CC/FUN_8002847C family (impact starburst). Full RE + action list + evidence in docs/findings/effects.md. Not exercised: the puff MESH layer (ring state never reaches 2/3 in any available repro) - ported-unverified.
