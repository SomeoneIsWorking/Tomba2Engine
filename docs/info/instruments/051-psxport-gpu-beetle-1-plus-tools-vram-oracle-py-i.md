---
id: I051
kind: instrument
status: trusted
created: 2026-08-21
---

## Instrument

PSXPORT_GPU_BEETLE=1 plus tools/vram_oracle.py (independent Beetle rasterizer tee; not a whole-machine oracle)

## Validated by

OTHER ANSWER: the shipped 1px Beetle selftest produces a nonzero VRAM difference; SAME ANSWER: area-4 health f914 completed with accepted=5417, dropped=0, starved=0, queued=0, our/beetle prims=969/969, 19,712 GPUREAD words with 0 mismatches, and the tight 48x48 wheel box had only five one-5-bit-step pixels. The adapter also demonstrated its loss answers while being repaired: missing GPUREAD drain wedged at GP0(C0), and synthetic GPU_Update tripped sl_zero_reached in an assert build. LIMIT: both sides consume the PORT'S command stream, so this instrument answers rasterization only and must never be called the true gameplay/packet oracle.

## Known failure modes

(none recorded yet)
