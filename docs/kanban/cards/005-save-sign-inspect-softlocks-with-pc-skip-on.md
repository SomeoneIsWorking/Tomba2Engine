---
id: 5
title: Save-sign inspect SOFTLOCKS with pc_skip ON
status: doing
labels: [bug, pc-skip]
created: 2026-07-18
updated: 2026-07-21
---

USER 2026-07-18: inspecting the save sign (seaside) soft-locks — Tomba frozen mid-inspect, no save menu appears, game logic fully halted (state byte-identical across 120 stepped frames). Repro: replays/bugs/save-sign-softlock.pad (headless replay reaches the frozen state, sm48=2 field). USER confirms caused by pc_skip=ON (default). Same class as #2 (bucket-pickup cutscene softlock) — a pc_skip shortcut fork in the interaction/cutscene flow. Root-cause the shared fork.
