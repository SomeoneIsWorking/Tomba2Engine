---
id: I055
kind: instrument
status: trusted
created: 2026-08-21
---

## Instrument

PSXPORT_GPU_SELFTEST texture-phase production gate: draw_tritri/draw_semi through generated SPIR-V, render_geom, and VRAM readback at ires 1/3

## Validated by

OTHER ANSWER: before the shared integer-pixel phase correction, the 1x matrix passed only positive X/Y and failed negative X/Y plus mixed non-unit slopes (2/5). A first half-derivative implementation passed 10/10 at 1x but failed the live default-ires wheel mask at 81/340, proving the gate needed internal-resolution coverage. SAME ANSWER: the final gate passes 20/20 at ires 1/3 across opaque/semi, +/-X, +/-Y, and mixed non-unit slopes, using constant-UV controls through the same encode path.

## Known failure modes

This isolates textured affine sampling phase, not packet production, primitive coverage, texture
windowing, perspective correction, or whole-frame equivalence. It requires a working Vulkan device
and a regenerated shader header. The constant-UV controls deliberately cancel downstream
internal-resolution box/1555 quantization; compare them directly instead of hardcoding one packed
color per scale.
