---
id: 14
title: Weapon CHARGE effect missing under pc_render (hold attack)
status: todo
labels: [render]
created: 2026-07-22
updated: 2026-07-22
---

USER 2026-07-22: holding the attack button charges the weapon and that charge effect does not render under pc_render. Same family as the torch flame (#12) and the HUD carousel (#13) - a whole effect layer simply absent. Repro: reach free-roam then HOLD square (REPL: press sq / run N / shot). Likely the #39 precedent: an effect emitter not wrapped in a depth-tag scope has its prims dropped by the field 2D-only OT walk (withDepthTag, game/render/render_internal.h; s_ot_2d_only in runtime/recomp/gpu_native.cpp). Verify with the pc-vs-oracle method: PSXPORT_GATE=1 vs PSXPORT_ORACLE=1 at identical exec state, diff the frames.
