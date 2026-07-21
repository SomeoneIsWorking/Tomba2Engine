---
id: 8
title: Water-pump seesaw: Tomba's weight doesn't pull it down when grabbed while climbing (pc_skip ON)
status: doing
labels: [bug, pc-skip]
created: 2026-07-21
updated: 2026-07-21
---

**2026-07-21:** Repro captured LIVE via the new dbg-server 'padrec save': replays/bugs/seesaw-weight.pad (6668 frames, from boot, NO memory-card load - user confirmed - so it is self-contained). Live probe while the user held the seesaw grabbed: an 'ents' diff over 4s shows NOTHING on either pump assembly moving (only Tomba's idle jitter at 800EE0D0 and one unrelated mover at 800FD118). Beam is genuinely static, matching the report.

**2026-07-21:** The two pump assemblies (left x~5.4-5.9k, right x~6.7-7.1k) are each 4 nodes: h=8012EB54 beh_substate_edge_orchestrator (12 cmds - the beam assembly), h=80136D9C beh_pure_inner_dispatch, h=80125E0C beh_pure_substate_dispatch, t=05 h=8004C238 beh_visibility_gate_dispatch. All are natively-owned DISPATCHERS, but 8012EB54 still has 12 still-PSX leaves (0x8012E8A8 8012ED84 8012F494 8012F5B4 8012FD88 80130524 80130AC4 80131134 801313C4 801316CC 80146348 8018C820). Per CLAUDE.md do NOT debug the divergence until that chain is owned end-to-end - grow ownership there first.

**2026-07-21:** DEAD END (method): comparing the pad replay across exec paths is INVALID. Same replay frame under pc_faithful vs PSXPORT_GATE=1 puts Tomba in completely different map locations - the two paths consume different numbers of pad frames during boot, so a capture desyncs on the other path. Evidence scratch/screenshots/seesaw_pc_vs_gate.png. Use SBS (lockstep, same input) for path comparison - but note SBS forces mPcSkip=false on both cores, so it does NOT exercise the pc_skip fork this bug is reported under.

**2026-07-21:** DEAD END (tool): 'provat' is blind under pc_render - it attributes GP0/VRAM writes, and pc_render never writes guest VRAM, so every probed pixel reads '<never written>'. Cannot use it to identify which node drew the beam.

**2026-07-21:** CORRECTION to the provat note above: that was NOT a new dead end - docs/gfx-debug.md:58 already said provat is blind to VK-teed polys and to use PSXPORT_PRIMAT instead. I re-derived a documented fact by not reading first. The listing at gfx-debug.md:22 has been fixed to carry the caveat where provat is introduced. Next time: PSXPORT_PRIMAT="x,y" is the attribution tool under pc_render.
