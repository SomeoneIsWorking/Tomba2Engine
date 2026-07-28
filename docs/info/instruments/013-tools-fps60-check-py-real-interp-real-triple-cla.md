---
id: I013
kind: instrument
status: trusted
created: 2026-07-28
---

## Instrument

tools/fps60_check.py — real/interp/real triple classifier over an fps60dump capture

## Validated by

Classifies every 16x16 tile of each interpolated frame as STATIC / BETWEEN / STALE (pixel-identical to the previous real frame while the next differs = did not lerp) / AHEAD. Validated both directions: walking with a moving camera gives 98.7% BETWEEN with 3 STALE tiles out of 42028 (it does not cry wolf), and a fully idle scene gives 100% STATIC (it does not invent motion). Requires PSXPORT_DEBUG=fps60dump, cap 600 files.

## Known failure modes

(none recorded yet)
