---
id: I018
kind: instrument
status: DISTRUSTED
created: 2026-07-28
distrusted_on: 2026-08-05
---

## Instrument

PSXPORT_DEBUG=nofx — names every type-0x20 node render fn the pc_render display walk SKIPS (no whitelist entry). Run it over the whole replay library to turn a static work-list into the entries that actually fire.

## Validated by

Validated by producing the OTHER answer on demand: with 0x80033080 unwhitelisted it names that fn; after the whitelist entry lands the line disappears while the rest of the census is unchanged (2026-07-28). It reports per-Core into Render::mNofxSeen so each fn is named once — a low hit count is dedup, NOT rarity.

## Known failure modes

(none recorded yet)

## DISTRUSTED 2026-08-05

OVERCLAIMS ITS COVERAGE. I018 is recorded as naming 'every type-0x20 node render fn the pc_render display walk SKIPS', but the nofx branch (game/render/render_walk.cpp:769) sits INSIDE the type==0x20 block, which is reached only after fieldObjectsRender's 'if (c->mem_r8(n + 1) == 0) continue;' visibility skip. So nofx can only ever report nodes that are ALREADY VISIBLE, and is structurally blind to a node that is skipped for invisibility — which is the single most important case for a MISSING-LAYER bug and exactly the class kanban #72/#55 is about. Measured this session: with nofx's blind spot in place, the live ring node 800EE730 was invisible to every existing type-0x20 diagnostic, while the new ringcensus instrument (I033), placed BEFORE the skip, reported it on 676 consecutive frames. Not wrong about what it does print — wrong about its denominator, which is the failure mode that makes a negative read as proof. Use I033 for absent/invisible nodes; nofx remains usable only for nodes that are drawn.

> Every result this instrument produced is suspect until it is re-validated.
