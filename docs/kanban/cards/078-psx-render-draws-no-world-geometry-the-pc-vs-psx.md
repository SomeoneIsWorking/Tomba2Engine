---
id: 78
title: psx_render draws NO world geometry — the pc-vs-psx reference comparison is a DEAD instrument
status: todo
labels: [render, tooling, verification]
created: 2026-08-06
updated: 2026-08-06
---

MEASURED 2026-08-06, area 0, reached by newgame+skip 3000 with NO warp, both legs on the SAME exec leg (PSXPORT_GATE=1, recomp_path), one boot per leg, only PSXPORT_RENDER_PSX differing:
  pc  -> scratch/screenshots/blockcull/ctl_a0_gpc.png  : the village field, hut, Tomba, terrain — correct.
  psx -> scratch/screenshots/blockcull/ctl_a0_gpsx.png : SKY AND SEA ONLY. No terrain, no hut, no Tomba.
Same in area 13 after a dev warp (abg_a13_gpc.png vs abg_a13_gpsx.png): pc draws trunks/fence/terrain, psx draws only the fronds and the particle sparkle.
CONSEQUENCE, and this is the point: any conclusion of the form 'psx_render does not draw X, therefore vanilla culls X' is VOID in this build — psx_render does not draw ANY world geometry, so it cannot produce the failing answer. docs/gfx-debug.md already says 'there is no render oracle'; this is the measurement behind that sentence and it needs to be in the doc as a number.
LIKELY CAUSE (not verified): kanban #45's campaign retired the substrate-GTE projection producers, so the guest OT is no longer filled with the world. If so psx_render is not repairable-by-accident and every doc/tool that offers it as a reference (tools/warpsweep.sh, docs/areas.md, the gfx-debug playbook) is offering a dead instrument.
