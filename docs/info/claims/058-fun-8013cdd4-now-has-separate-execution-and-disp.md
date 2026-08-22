---
id: C058
kind: claim
status: holds
created: 2026-08-22
tags:
depends: game/render/prop_quad.cpp#Render::propQuadRender, game/render/mesh_quads.cpp#Render::meshQuadRecordsEmit, game/render/widescreen_margin_quad.cpp#WidescreenMarginQuad::emit
---

## Claim

FUN_8013CDD4 now has separate execution and display owners: WidescreenMarginQuad mirrors guest state, while Render::propQuadRender rebuilds the pc_render picture only from persistent object/node state.

## Evidence

docs/findings/render.md section 0x8013CDD4 missing picture; 2026-08-22 bucket-softlock headless gate: 3520 propquad rows, 4326 native prims over 97 frames, native/psx captures; after bumping to committed psxport `7f5d3f13`, the clean-pin Clang build, 5/5 CTest, and a fresh 461-frame replay again produced exactly 3520 propquad rows with no failure signature

## What would falsify it

A live object shows a software-oracle prop that the native producer omits or materially misrenders while logged persistent inputs match, or the producer begins reading GTE/packet/OT/scratchpad/generated state
