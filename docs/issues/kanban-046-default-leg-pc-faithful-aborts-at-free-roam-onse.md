---
id: kanban-046
title: historical guest-entry miss 0x80028E64 latent (label not emitted as entry) — NOT reproduced on current main
status: open
labels: [bug, guest instruction path]
---


Flagged by the fx_sprite port agent 2026-07-23 (NOT caused by it — pre-existing, and it verified via GATE=1 instead). The default pc_faithful leg aborts entering free-roam with historical guest-entry miss 0x80028E64. Same class as kanban #24/#27: a recorded function-boundary gap — 0x80028E64 is a mid-body label of guest 0x80028E10 that guest 0x8003116C (intro-narration effect family) jumps to, and the recorded binary evidence never emitted it as an entry. Fix is the same shape as #24: seed the mid-body entry / area-indexed-table discovery in the removed offline emitter and regenerate, OR port the function. Note this blocks default-leg verification of anything past free-roam onset in the current checkout — the fx_sprite flame port was gated on GATE=1 + psx_render for exactly this reason, so once this is fixed, re-verify the flame on the default leg too.
