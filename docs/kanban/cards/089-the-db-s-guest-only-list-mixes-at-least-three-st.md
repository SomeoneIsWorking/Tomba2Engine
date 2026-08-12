---
id: 89
title: The DB's guest-only list mixes at least three states — it is NOT a work-remaining ranking
status: done
labels: [bug, render]
created: 2026-08-12
updated: 2026-08-12
---

2026-08-12, found while trying to USE the newly-joined DB to name the top unported effects — so the join works but its headline list does not mean what I first reported. The 27 guest-only rows (guest prims > 0, native prims == 0) split into at least three distinct states, and treating them as one ranking is wrong:

(1) NOT override-installed, 16 rows, topped by 0x8003DF04 at 394,944 prims. But even this is not simply 'unported': codemap names 0x8003DF04 as Render::backdropTilemapDrawer LIVE at game/render/render_walk.cpp:160, with NO install site — so a native reimplementation exists by name while the GUEST body is what runs at that address. And the effect arguably DOES have a native producer at a different key: 0x8010C26C backdropRender carries 472,384 native prims. The otchain chain for 0x8003DF04 (0x8003DF04 <- 0x8003F9A8 <- 0x80108B0C <- 0x80106B98 <- ...) contains 0x8010C26C nowhere in 20 frames, so the guest leg reaches the backdrop by a DIFFERENT path than the native producer covers. Whether that is one effect with two paths or two effects is unresolved.

(2) override-installed but native NEVER REACHED while the guest drew, 11 rows: 0x8013FE58 (251,138), 0x801465EC (161,378), 0x8013FB88 (139,750), 0x801467BC (136,991), 0x8003C048, 0x8007E6DC, 0x80078CA8, 0x8003C8F4 and 3 more. These are byte-faithful guest-writing emitters (OverlayGroundGt3Gt4::gt4, OverlayGt3Gt4::gt3) — natively OWNED, but they produce the picture by writing guest packets, so they never open a ProducerScope and can never become claims. producers.py already flags has_native+not-reached as 'a lie in the DB' and ranks it above new work.

(3) SDK libgs builders, 0x80080000 / 0x8008007C / 0x8007FDB0 — kanban #88.

WHAT TO DO: the DB needs to REPORT these three states separately rather than leaving a reader to derive them, because the undifferentiated list invites exactly the wrong conclusion (it invited mine). The data is already present per row (has_native, native_reached, prims_native), so this is a reporting change in producers.py plus a schema note, not new instrumentation. Until then, do NOT quote the guest-only count as work remaining.

**2026-08-12:** 2026-08-12 FIXED: tools/producers.py report now breaks the guest-only rows into the three states (16 not override-installed / 11 installed-but-never-reached / 0 reached-yet-pushed-nothing) and says in the output itself that this is NOT a work-remaining ranking, naming the reason (a byte-faithful guest-writing emitter is natively owned and still draws through the guest packet path, so it can never open a ProducerScope) and pointing at codemap.py --addr to identify a row before calling it unported. No new instrumentation — every number comes from fields already in the schema.
