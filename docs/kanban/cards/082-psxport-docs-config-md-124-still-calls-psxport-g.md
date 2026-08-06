---
id: 82
title: psxport docs/config.md:124 still calls 'PSXPORT_GATE=1 PSXPORT_RENDER_PSX=1' THE REFERENCE — it is not; PSXPORT_ORACLE=1 is
status: todo
labels: [docs, psxport]
created: 2026-08-06
updated: 2026-08-06
---

**2026-08-06:** external/psxport/docs/config.md lines 123-124 read:
  | PSXPORT_RENDER_PSX=1 | pc_faithful + psx_render | Substrate renderer only. Faithful still broken. |
  | PSXPORT_GATE=1 PSXPORT_RENDER_PSX=1 | recomp_path + psx_render | THE REFERENCE. Works perfectly. |
'Works perfectly' was false for at least this session: that leg rendered sky and sea only (kanban #78). It is fixed game-side now, but it is still NOT the pure reference — it composites the guest OT through the native RenderQueue's layer split and per-pixel depth, so its fidelity depends on native producers having run. PSXPORT_ORACLE=1 is the leg with no native ordering decision in it (gpu_native.cpp:870 forces is3d=0/bg=0 under game->oracle), and native_boot.cpp:590-600 already documents it as 'THE PURE PSX REFERENCE'.
Tomba2Engine's own CLAUDE.md carries the same wording ('Works perfectly. The reference build.') and should be corrected in the same pass.
NOT DONE HERE: external/psxport was held by another agent this session and was not edited. Operator/psxport owner to apply.
