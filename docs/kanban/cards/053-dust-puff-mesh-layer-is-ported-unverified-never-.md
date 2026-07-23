---
id: 53
title: Dust PUFF MESH layer is ported-unverified — never observed on screen (ring state 2/3 unreached)
status: todo
labels: [verification, render]
created: 2026-07-23
updated: 2026-07-23
---

Split from #39 2026-07-23. game/render/fx_dust.cpp implements BOTH dust layers from the RE, but only the TRAIL layer has been verified on screen. Across 4000 frames of replays/bugs/seesaw-weight.pad the dust node's ring state never leaves {0,1}, so the guest itself emits only the trail quads there — the PUFF MESH path (FUN_80027768, mesh 0x8009F3E0 x4 at 0x400 apart, transform rotmat(0, owner+0x68, age-0x400)*rotY(i*0x400) column-scaled by 0x800A1FEC[node+6], far-colour tint 0x800A1FC8[sub], and the subtypes>=6 owner-matrix/rotZ/0x8009F398 variant) has never been exercised. It is implemented, plausible and unverified — exactly the state the port-map calls 'ported-unverified', and the #10 sweep is a standing reminder that unverified rebuilds carry real defects. ACTION: find a scenario that drives the dust ring to state 2/3 (likely a longer sustained run, or a specific surface material) and A/B the puff layer's vertices against the guest submission the way the trail layer was gated (guest 0x3A quad first-vertices vs the native producer's, at a fixed frame). Until then do not claim the dust port is fully verified.
