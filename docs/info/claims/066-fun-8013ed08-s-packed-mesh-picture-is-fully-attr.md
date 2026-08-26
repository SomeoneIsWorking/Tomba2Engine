---
id: C066
kind: claim
status: holds
created: 2026-08-26
tags: render,tomba2,native-producer
depends: generated/ov_a00_shard_0.c#ov_a00_gen_8013ED08, game/render/fx_rigid_mesh.cpp#Render::rigidMeshEffectRender, game/render/render_walk.cpp#Render::fieldObjectsRender
---

## Claim

FUN_8013ED08's packed-mesh picture is fully attributable to persistent node position, Euler angles, unsigned scale bytes, mesh, sort bias and U scroll, with an explicit identity IR0 cue

## Evidence

generated/ov_a00_shard_0.c ov_a00_gen_8013ED08 is a 22-line controller: zero 0x1F800090; FUN_800318A0(node+0x2C,node+0x54,node+0x48); FUN_80027768(*(node+0x50),0,(s16)node+0x32,(u8)node[7]). Render::rigidMeshEffectRender implements those inputs through MeshQuads + EffectLerp + projComposeObjectHost + meshQuadRecordsEmit; Clang build, mesh math, format/tidy and codemap gates pass.

## What would falsify it

A changed retail/generated controller, or a same-frame guest packet/oracle comparison showing transform, material, ordering, or interpolation output outside this contract, falsifies the claim
