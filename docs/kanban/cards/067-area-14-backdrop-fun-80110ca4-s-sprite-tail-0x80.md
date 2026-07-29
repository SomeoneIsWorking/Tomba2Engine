---
id: 67
title: Area-14 backdrop: FUN_80110CA4's sprite tail 0x801104D0 (440 lines) is unported
status: todo
labels: [render]
created: 2026-07-29
updated: 2026-07-29
---

Render::fxBackdropPlaneRender (fx_backdrop_plane.cpp) owns the two GRIDS of FUN_80110CA4 — the waterfall wall, its mirrored reflection and the additive seam glow — and is pixel-verified. But the guest render fn TAIL-CALLS 0x801104D0 with the same node, a 440-line sprite-family body that is not ported, so whatever that half draws is still absent from area 14. Spec context: docs/re/render-targets-static-re.md, section 0x80110CA4. Portmap step fx-backdrop-plane-110ca4 is marked verified for the grids only and says so.
