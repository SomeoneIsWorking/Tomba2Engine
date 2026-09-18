---
id: 84
title: Replay library cannot exercise the two PC-ONLY producers (pc/widescreen-margin, pc/wide-page-fill)
status: todo
labels: [producers]
created: 2026-08-12
updated: 2026-09-19
---

The graphics-producer DB's native leg is at 0 undeclared native producers on both aspects, and the PC-only key mechanism (psxport pc_producer(), ProducerKey::native) is covered by framework hermetic tests. But NEITHER of the two PC-only consumers emits a prim in any replay currently in replays/, so their rows have never appeared in a real run.

MEASURED, so this is a coverage gap and not a suspected bug:
- pc/widescreen-margin (MarginRenderer::flush, game/render/margin_render.cpp): the producer RUNS — nativeEnabled() defaults to 1 and object_list.cpp:55 calls flush(c) unconditionally from walkAll — but it collects no nodes in the scenes covered, so it emits nothing and correctly mints no row (a scope with no pushes creates no row, which is the desired behaviour). Distinguish 'runs and draws nothing' from 'never runs': it is the former. Its own debug line is gated to gpu_frame_no==2900, which no short replay reaches — that gate is worth loosening to a channel-rate limit if this is picked up.
- pc/options-pillarbox (Render::optionsBackdrop, game/render/render_options.cpp): the options page is never reached under PSXPORT_GATE by replays/bugs/title-options-page.pad or ingame-options-page.pad (verified: no 0x8007FC24 row in either 600-frame run at 16:9), so the pillarbox quad never draws.

WHAT WOULD CLOSE IT: a replay that reaches the in-game options page on the GATE leg, and a scene/aspect where margin geometry is actually re-included (cull.cpp:404 collect path). Then assert both native-only rows appear. Until then, do not claim the PC-only rows are verified end-to-end — the mechanism is, the consumers are not.

**2026-09-19:** 2026-09-19: the options half of this gap is a REPLAY LENGTH artefact, not an unreachable page. The 600-frame runs cited above stop short: replays/bugs/ingame-options-page.pad lands on the in-game Select Options page at frame 1160 and replays/bugs/title-options-page.pad on the front-end one at 1027. Both draw the page at 16:9 in a looks-right run of 1180 / 1050 frames (scratch/wide/optpage2, scratch/wide/titleopt). The producer itself changed in #122: pc/options-pillarbox is gone, replaced by the shared pc/wide-page-fill (WidePageFill::pushBehindPage), whose stale docs/producers/pc-10E43669.md row was removed. What still has to be done here is the census assertion — a PSXPORT_PRODUCERS run of one of those replays at 16:9 wrote no scratch/producers/*.jsonl even with the census armed, so how a run feeds the DB needs establishing before either PC-only row can be asserted end to end. The margin half (pc/widescreen-margin) is untouched.
