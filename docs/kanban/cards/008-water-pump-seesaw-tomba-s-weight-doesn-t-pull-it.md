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

**2026-07-21:** ROOT CAUSE of the replay not reproducing (a TOOLING bug, not the game bug): the dbg-server pause loop called Pad::serviceFrame() every 15ms while frozen, and the record/replay frame index lives inside serviceFrame - so the pad-frame clock advanced while the game did not. The user's session was paused (P key) during capture, so seesaw-weight.pad contains masks recorded against a frozen game; replaying consumes them while the game runs -> everything after the pause is shifted. FIXED via Pad::pumpHostInput() (verified: padrec 5720->5720 over 6s paused, advances on resume). CONSEQUENCE: the current seesaw-weight.pad is unreliable past the first pause and must be RE-RECORDED. Also SUPERSEDES the earlier 'replays desync across exec paths' note - that observation is explained by this bug and must be re-tested with a clean capture before being believed.

**2026-07-21:** RETRACTION (USER caught it): the previous note's claim that seesaw-weight.pad is corrupt and must be re-recorded is WRONG. The replay DOES reproduce the grab - zooming the probe screenshot shows Tomba hanging on the pump; I had mistaken an NPC for him at 320x240 and concluded 'on the ground, not grabbing'. Everything I derived from that (capture corrupt, cross-path desync explained) was speculation asserted as fact. The pause-clock defect is separately real and measured, but implies nothing about this capture. USABLE STATE now established: replay to pad frame ~6424 and pause -> Tomba grabbed, both beam nodes node[4]=1 node[5]=0, nothing moving. That is the state to investigate from.

**2026-07-21:** MECHANISM ESTABLISHED (docs/findings/ai.md): consumer FUN_801308e0 turns node[0x2b] (contact index) into node[0x48]=0xe000, the weight. WWATCH over the whole replay: node[+0x2b] written 6585x per node, ALWAYS 0, only by clearers (80130D5C/801316CC/80146348/80084A80). Nothing ever writes a nonzero contact value - the producer never fires. Consumer healthy. NEXT: find the guest producer that stamps a contact index into a struck object's +0x2b; unverified lead = player surface-probe cluster FUN_80024448 / caller 0x8005D530 (not yet ported).
