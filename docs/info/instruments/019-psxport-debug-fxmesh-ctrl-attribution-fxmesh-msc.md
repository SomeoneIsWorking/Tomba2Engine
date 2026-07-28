---
id: I019
kind: instrument
status: trusted
created: 2026-07-28
---

## Instrument

PSXPORT_DEBUG=fxmesh ctrl= attribution (FxMesh::mScopeFn) — says WHICH effect-mesh controller raised the scope that produced each emitted list.

## Validated by

Introduced 2026-07-28 precisely because a bare pixel A/B lied: an earlier with/without capture reported 0 changed pixels on all 6 sampled frames while the controller was in fact emitting 136 non-degenerate quads. Re-running with ctrl= counts as the leg control (68 lists vs 0) gave 700-1367 px/frame. RULE: attribute the emission before believing the pixels.

## Known failure modes

(none recorded yet)
