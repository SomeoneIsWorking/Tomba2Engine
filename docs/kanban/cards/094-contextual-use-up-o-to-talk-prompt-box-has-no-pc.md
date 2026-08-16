---
id: 94
title: Contextual "Use UP + O to talk" prompt box has no pc_render producer
status: done
labels: [render, bug]
created: 2026-08-16
updated: 2026-08-16
---

Present on the oracle, absent under pc_render. Seen in the fisherman's-hut interior at f600/f800/f930 of replays/scene-transitions/hut-entry-door-freeze.pad — compare scratch/screenshots/sweep/oracle_930.png vs pc_930.png.

NOT part of the 2026-08-16 camera regression (it is absent both before and after that fix) and findings.py has no prior hit for it, so it is an uninvestigated missing producer rather than a regression. Belongs on the docs/unported-render-inventory.md list.

**2026-08-16:** CORRECTION to this card's own framing: I only measured this AFTER the #93 camera fix (before it, the entire world was missing, so 'absent before' is not something this session observed). What is measured: post-fix, pc_render omits the prompt while the oracle draws it, at f600/f800/f930. Whether it predates the 2026-08-14 window is UNVERIFIED.

**2026-08-16:** ROOT CAUSE was not a missing producer — this card's own title was wrong. The producer runs and pushes (PSXPORT_DEBUG=panelq prints the exact rects and glyph count; ovhit shows 0x8004FFB4 native=2665 oracle=0). Fps60::rq_capture OVERWROTE its single snapshot on every RenderQueue::flush, and a logic frame flushes once per guest DrawOTag — commonly twice. Tomba2 emits 2D chrome in the FIRST flush and the world in the second, so at the default fps60=1 the whole panel family was discarded. Fixed framework-side (psxport 284c012e): accumulate across the frame's flushes, reset at the fence.
