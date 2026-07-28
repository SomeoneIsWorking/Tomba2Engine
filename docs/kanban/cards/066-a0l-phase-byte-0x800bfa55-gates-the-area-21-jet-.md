---
id: 66
title: A0L phase byte 0x800BFA55 gates the area-21 jet effect (FUN_8010C1D8) — need a scene where it reaches >= 4
status: todo
labels: [render]
created: 2026-07-29
updated: 2026-07-29
---

FUN_8010C1D8 (A0L overlay, area 21) returns immediately unless *(u8*)0x800BFA55 >= 4. In the standard area-21 capture (warp 21; skip 600) it reads 1, so the effect draws NOTHING and the producer cannot be pixel-verified there — a 0-px A/B would be indistinguishable from a broken port (instrument I022). The port is otherwise ready: its record table at 0x801154E0 is a SINGLE 36-byte record (rec0 [+4] = 0xC02E0000, terminator bit set), and every helper it needs already exists (MeshQuads::rotmat, Math::matColScale, projComposeObjectHost, meshQuadRecordsEmit, altSpriteEmit). NEXT: run tools/find_refs.py scratch/raw/a21_dump.bin 0x800BFA55 --rw w to list the writers and find what advances the phase, then reach that scene and port + verify there. Blocks portmap step fx-jet-mesh-sprite-10c1d8.
