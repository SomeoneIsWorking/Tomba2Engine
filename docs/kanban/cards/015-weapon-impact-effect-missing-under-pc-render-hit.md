---
id: 15
title: Weapon IMPACT effect missing under pc_render (hitting something)
status: todo
labels: [render]
created: 2026-07-22
updated: 2026-07-22
---

USER 2026-07-22: striking something with the weapon produces an impact effect that does not render under pc_render. Note docs/findings/render.md already records issue #39 as 'weapon chain + impact effect' fixed via withDepthTag depth-tagging - so either that fix regressed, or it covered only the chain and the impact half was never actually verified. CHECK THE EXISTING FINDING FIRST before re-deriving. Repro: free-roam, tap square next to a breakable/enemy, shot on the contact frames; compare PSXPORT_GATE=1 against PSXPORT_ORACLE=1 at identical exec state.

**2026-07-22:** NOT reproduced (2026-07-22): swing/contact frames vs the apple contraption show no impact-flash difference between renderers (atk*_p*.png). IMPORTANT correction: the old #39 'impact effect fixed via withDepthTag' claim is OBSOLETE — pc_render no longer walks the guest OT, so obj_depth tags no longer produce a picture; the successor is the #12 FUN_80027A4C tap (the #28 analysis showed the swing smear quads flow 27E5C->27A4C, so impact quads through that family now render). If the user still sees a missing impact effect, prime suspect = FUN_8003D23C effect pool (see card 14 / render.md 2026-07-22 entry). USER: please re-test impact after this build.

**2026-07-22:** 2026-07-22 (sweep agent) — unchanged, not re-attempted this session. Same note as #14: the impact trigger was never reached, and the area sweep (24 of 32 areas, card #25) confirms this class is TRIGGER-gated rather than place-gated, so warping around will not surface it.
