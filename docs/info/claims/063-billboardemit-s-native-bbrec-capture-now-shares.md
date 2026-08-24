---
id: C063
kind: claim
status: falsified
created: 2026-08-25
tags: render,fps60,billboard
depends: game/render/billboard_capture_policy.h#BillboardCapturePolicy::allowed, game/render/perobj_billboard.cpp#Render::billboardEmit
falsified_on: 2026-08-25
---

## Claim

BillboardEmit's native BbRec capture now shares fieldAreaInit's field-world existence gate with both real and interpolated billboard presentation.

## Evidence

tests/test_billboard_capture_policy.cpp was red on the old oracle+rotation-only predicate at the field-area-init case and is green on all four cases after adding the shared gate; combined Clang tomba2_port relink and targeted clang-tidy pass. Runtime ledger confirmation remains issue #2's closing gate.

## What would falsify it

billboardEmit records any BbRec while Render::fieldAreaInit() is true, or a presentation path draws field billboards during that same state.

## FALSIFIED 2026-08-25

Bounded integration run scratch/logs/gate-run-20260825-002511.log was identical after compiling and relinking the fieldAreaInit capture predicate: f3016 still captured two tier1 world quads for node 0x800EDE28 and presented zero. Therefore the residual RqItems do not come from billboardEmit's BbRec capture path; node provenance was not producer provenance. The ineffective policy/header/test/CMake change was removed.

> Anything that cited this claim as proof must be re-checked. Grep the repo for it.
