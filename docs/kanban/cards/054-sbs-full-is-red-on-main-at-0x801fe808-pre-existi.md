---
id: 54
title: SBS-full is RED on main at 0x801FE808 — pre-existing, reproduces on an unmodified HEAD build
status: todo
labels: [bug, verification]
created: 2026-07-23
updated: 2026-07-23
---

**2026-07-23:** Found 2026-07-23 while gating kanban #50. NOT caused by #50 — proven by building the binary TWICE from the same tree (unmodified HEAD 1cd8647 vs HEAD+the #50 scheduler fix) and running both:
  * replays/bugs/seesaw-weight.pad, PSXPORT_SBS_MODE=full AUTONAV=1: BOTH first diverge at f169, 0x801FE808..0x801FE80F (7 B), A=18 B2 0F 80 A8 E6 0E  B=17 00 00 00 78 B2 0F — identical frame, address and bytes.
  * the repo's standard push-gate leg (PSXPORT_SBS_MODE=full AUTONAV=combat, docs/fleet-workflow.md): BOTH first diverge at f190, 0x801FE808..0x801FE810 (8 B), A=18 B2 0F 80 00 00 00 00  B=17 00 00 00 78 B2 0F 80.
Checkpoints are 'A/B identical' every 30 frames up to f180 (combat) / f150 (seesaw), then the divergence persists and wanders for the rest of the run (1400-1900 [sbs-div] lines within a 100s window).
0x801FE808 is guest-STACK scratch just above the 3-slot task table (0x801FE000 + 3*0x70 = 0x801FE150). A looks like a live pointer pair (0x800FB218 / 0x800EE6A8-ish), B like a zero-filled slot plus 0x800FB278 — i.e. a spill-slot residency difference, the same FAMILY as the resolved 0x801FE918 spawn-leaf residual in docs/findings/sbs.md, at a different address. Root-cause it with a watch on 0x801FE808 + the SBS debug server (sbs watch / sbs bt) at f169.
This means the tree's SBS gate currently cannot be cited as 0-diff by any agent; treat a '0-diff' claim from before this date as stale.
