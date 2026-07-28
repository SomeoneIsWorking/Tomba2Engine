---
id: I018
kind: instrument
status: trusted
created: 2026-07-28
---

## Instrument

PSXPORT_DEBUG=nofx — names every type-0x20 node render fn the pc_render display walk SKIPS (no whitelist entry). Run it over the whole replay library to turn a static work-list into the entries that actually fire.

## Validated by

Validated by producing the OTHER answer on demand: with 0x80033080 unwhitelisted it names that fn; after the whitelist entry lands the line disappears while the rest of the census is unchanged (2026-07-28). It reports per-Core into Render::mNofxSeen so each fn is named once — a low hit count is dedup, NOT rarity.

## Known failure modes

(none recorded yet)
