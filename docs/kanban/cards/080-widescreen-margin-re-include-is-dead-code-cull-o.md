---
id: 80
title: Widescreen margin re-include is DEAD CODE — Cull::objectCull has no caller
status: todo
labels: [bug, render, tooling]
created: 2026-08-06
updated: 2026-08-06
---

**2026-08-06:** MEASURED 2026-08-06 while working kanban #77. Cull::objectCull() (game/render/cull.cpp) is the ONLY caller of MarginRenderer::collect, and objectCull ITSELF has no caller: it is not in Cull::registerOverrides() (which installs only the six cullWrapper* variants for 0x80077870/8007778C/800777FC/80077ACC/800779D0/80077A4C/800778E4), there is no overrides::install for 0x8007712C, and a grep of game/ + runtime/ for 'objectCull' finds only its own declaration, definition and comments. Actor::boundsCull reaches Cull::performBaseCull directly, which has no margin arm.
CONSEQUENCE: MarginRenderer::flush(c) (called from ObjectList::walkAll) always flushes an EMPTY list, so the widescreen margin does not exist. Everything downstream of that is dead too: the CULL_MARGIN_FAR / CULL_MARGIN_FOV constants, the PSXPORT_CULL_FAR / PSXPORT_CULL_FOV / PSXPORT_CULL_ONLY_TYPE / PSXPORT_CULL_SKIP_TYPE / PSXPORT_MARGIN_POKE levers, and the cullobj/cullinc channels — all unreachable. cull.h's own banner already says 'Currently ORPHANED', but the margin is documented elsewhere as live.
WHY IT MATTERS BEYOND DEAD CODE: it is a live TRAP for exactly the class of bug #77 is. The margin arm reads as the obvious cause of 'the port draws geometry vanilla culls' (it deliberately lowers the guest's own near cull from 512 to 0x80 and re-includes what the cull dropped), and a session can spend its budget fixing code that never runs. Also note CULL_FAR_MULT=4 in Cull::cullFarMultSkip is likewise inert under PSXPORT_GATE=1, where override_registry runs the gen body for every registered address.
NOT VERIFIED: whether the margin should be re-wired or DELETED. Per BREAK-FIRST the default is delete; re-wiring it would need the near-cull question settled first (gen_func_8007712C rejects dist<512 in every state branch, 768 in state 2 / 1024 in state 4 — that near test is the game's own view-blocker removal and a margin must not undo it).
