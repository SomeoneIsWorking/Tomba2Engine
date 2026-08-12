---
id: 82
title: psxport docs/config.md:124 still calls 'PSXPORT_GATE=1 PSXPORT_RENDER_PSX=1' THE REFERENCE — it is not; PSXPORT_ORACLE=1 is
status: done
labels: [docs, psxport]
created: 2026-08-06
updated: 2026-08-12
---

**2026-08-06:** external/psxport/docs/config.md lines 123-124 read:
  | PSXPORT_RENDER_PSX=1 | pc_faithful + psx_render | Substrate renderer only. Faithful still broken. |
  | PSXPORT_GATE=1 PSXPORT_RENDER_PSX=1 | recomp_path + psx_render | THE REFERENCE. Works perfectly. |
'Works perfectly' was false for at least this session: that leg rendered sky and sea only (kanban #78). It is fixed game-side now, but it is still NOT the pure reference — it composites the guest OT through the native RenderQueue's layer split and per-pixel depth, so its fidelity depends on native producers having run. PSXPORT_ORACLE=1 is the leg with no native ordering decision in it (gpu_native.cpp:870 forces is3d=0/bg=0 under game->oracle), and native_boot.cpp:590-600 already documents it as 'THE PURE PSX REFERENCE'.
Tomba2Engine's own CLAUDE.md carries the same wording ('Works perfectly. The reference build.') and should be corrected in the same pass.
NOT DONE HERE: external/psxport was held by another agent this session and was not edited. Operator/psxport owner to apply.

**2026-08-12:** **CLOSED 2026-08-12 — fixed in psxport 7dc380c5.** The table now gives PSXPORT_ORACLE=1 its own row as THE PICTURE REFERENCE, and the old row says NOT the reference with a paragraph naming exactly what it fails to suppress (painter order, depth band, widescreen, fps60 — all of which can still reach the frame under GATE+RENDER_PSX, so a compare against it can report 'no difference' about a difference a native decision produced).

Two things beyond the literal fix. (1) config.md was contradicting docs/oracle.md:186 in the same tree, which already called ORACLE the best in-tree picture reference — so this was a drift between two docs, not a missing fact. (2) The oracle's OWN limit is now stated in config.md rather than only in oracle.md: it is still the native rasterizer at native precision at ires>1, so it answers 'what does the SUBSTRATE draw', never 'what does the HARDWARE draw'.

Checked for the same claim elsewhere: the only other occurrences of the flag pair are docs/workspace/LAYOUT.md:66 and :196, both boot-gate invocations rather than reference claims, and correct as written. Reaches this tree at pin 0a6c90f9 or later.
