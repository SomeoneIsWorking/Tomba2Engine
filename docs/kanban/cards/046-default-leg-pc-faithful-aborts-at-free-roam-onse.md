---
id: 46
title: Default leg (pc_faithful) aborts at free-roam onset: recomp-MISS 0x80028E64
status: todo
labels: [bug, recomp]
created: 2026-07-23
updated: 2026-07-23
---

Flagged by the fx_sprite port agent 2026-07-23 (NOT caused by it — pre-existing, and it verified via GATE=1 instead). The default pc_faithful leg aborts entering free-roam with recomp-MISS 0x80028E64. Same class as kanban #24/#27: a recompiler function-boundary discovery gap — 0x80028E64 is a mid-body label of gen_func_80028E10 that gen_func_8003116C (intro-narration effect family) jumps to, and the recompiler never emitted it as an entry. Fix is the same shape as #24: seed the mid-body entry / area-indexed-table discovery in external/psxport/tools/recomp/emit.py and regenerate, OR port the function. Note this blocks default-leg verification of anything past free-roam onset in the current checkout — the fx_sprite flame port was gated on GATE=1 + psx_render for exactly this reason, so once this is fixed, re-verify the flame on the default leg too.
