---
id: C011
kind: claim
status: holds
created: 2026-07-28
tags: render
---

## Claim

Across the whole 15-replay library, every type-0x20 render fn the nofx census reaches now has a producer.

## Evidence

Full sweep 2026-07-28, each replay sized to its own pad length, headless: all 15 exit 0, zero abort/fatal/recomp-MISS. Skipped-fn union is 0x8002AB5C (terrain), 0x8013CDD4 (widescreen margin), 0x800288AC + 0x8002BC9C (scoped FxMesh controllers), 0x8002A834 (SwingFx) — all owned by another route. 0x8013D454 has left the list.

## What would falsify it

any new replay or area that makes nofx name a fn with no owner; render_fns.py still lists static candidates no replay visits, so this is a statement about coverage, not completeness
