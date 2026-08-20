---
id: I050
kind: instrument
status: DISTRUSTED
created: 2026-08-20
distrusted_on: 2026-08-20
---

## Instrument

provat Vulkan presented-pixel provenance

## Validated by

Negative control failed: during a HUT run whose visible pixel (140,80) changed to cyan, provat reported that pixel was never written

## Known failure modes

The instrument reads guest-VRAM write history. Native RenderQueue/Vulkan presentation does not write
those final pixels through that history, so a visible Native/PC pixel can be reported as never written.

## DISTRUSTED 2026-08-20

On deterministic HUT f1200 the visible Vulkan output at (140,80) changed from dark to cyan, while provat reported the pixel was never written; it cannot establish produced-vs-presented provenance on this route.

> Every result this instrument produced is suspect until it is re-validated.
