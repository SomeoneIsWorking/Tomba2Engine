// game/render/narration_swirl.cpp — native producer for the SOP intro-narration SWIRL/VORTEX.
//
// The narration's full-screen swirl is NOT a cmd-list object: its render node (0x800ED9E8, type 0x20)
// carries a CUSTOM render fn at node+0x18 = 0x8010BF54 (SOP overlay), dispatched by the substrate
// master walk's default case. The native object walk (Render::fieldObjectsRender) used to skip it
// (cmd counts 0/0) — that was the "void beat renders ~black" gap (findings/render.md '#5').
//
// RE (2026-07-16, Ghidra on the void-beat dump — scratch/bin/ram_void.bin):
//   FUN_8010BF54(node):                                  // the swirl render fn
//     IR0(spad 0x1f800090) = 0                           // depth-cue OFF -> colors pass through
//     for blade in {0,1}:
//       FUN_80031D24(node+0x2c, DAT_80108FF0, node+0x48) // transform setup (below)
//       FUN_80027768(&MESH 0x8010CC08, 0, 0, node[0x50]) // mesh submit, U-scroll = node[0x50]
//       node[0x4e] += 0x800                              // second blade rotated +0x800
//   FUN_80031D24 = object matrix: M = colScale( rotmat(node+0x48 angles) · rotY(node[0x4e]),
//                  DAT_80108FF0 bytes<<2 = (128,256,128) ) composed with the CAMERA (spad f8..114) —
//                  exactly what projComposeObjectHost does natively. Translation = SVECTOR node+0x2c.
//   FUN_80027768 = 36-byte quad records at 0x8010CC08 (loop while word1 > 0; the record with word1
//     bit31 set is drawn LAST — its uv1 masks the bit off):
//       word0: u0,v0 bytes + CLUT<<16            word1: u1,v1 bytes + tpage bits16-22; bit30=semi
//       word2: u2,v2 + u3,v3                     words3-6: RGB0..RGB3 (byte3 of each = vertex Z, s8)
//       bytes28-35: x0,y0? no — x0,x1? layout:   x0@28 y0@30 x1@29 y1@31 x2@32 y2@34 x3@33 y3@35
//       vertex k = (s8 x, s8 y, s8 z) * 256 model units. Prim = 0x3C/0x3E gouraud-TEXTURED QUAD.
//     NO backface cull (only the GTE FLAG overflow check); IR0=0 -> DPCT leaves colors unchanged;
//     every U byte gets +node[0x50] (the animated texture scroll; FUN_8010BEAC ticks it -9 wrap +75).
//
// pc_render rules: READ-ONLY (the substrate render fn still runs underneath and owns the node[0x4e]
// advance + spad writes — this producer only reads); picture from game state with real depth.
#include "core.h"
#include "game.h"
#include "render.h"
#include "render_queue.h"
#include "proj_params.h"
#include "mesh_quads.h"   // MeshQuads: the shared host-side 1.3.12 rotation builders + record walk


// narrationSwirlRender — draw the SOP swirl node natively (see banner). `node` is the type-0x20 render
// node whose custom render fn is the SOP swirl renderer; `yawBias` selects the blade (the substrate
// render fn advanced node[0x4e] by +0x800 per blade during exec, so blade k of THIS frame used
// yaw = node[0x4e] - 0x1000 + k*0x800).
void Render::narrationSwirlRender(uint32_t node) {
  Core* c = mCore;
  const uint32_t MESH = 0x8010CC08u;                      // SOP overlay swirl mesh (RE'd constant of 0x8010BF54)
  // transform inputs (all read-only)
  const int16_t ax = (int16_t)c->mem_r16(node + 0x48);
  const int16_t ay = (int16_t)c->mem_r16(node + 0x4a);
  const int16_t az = (int16_t)c->mem_r16(node + 0x4c);
  const int16_t yawNow = (int16_t)c->mem_r16(node + 0x4e);  // post-exec value (both blades already added)
  const uint8_t uScroll = c->mem_r8(node + 0x50);           // animated texture U scroll
  const uint32_t posw0 = c->mem_r32(node + 0x2c), posw1 = c->mem_r32(node + 0x30);
  const float Tobj[3] = { (float)(int16_t)posw0, (float)(int16_t)(posw0 >> 16), (float)(int16_t)posw1 };
  const uint32_t scw = c->mem_r32(0x80108ff0u);             // base col-scale bytes (<<2): (32,64,32)<<2
  const int32_t colScale[3] = { (int32_t)((scw & 0xFF) << 2), (int32_t)(((scw >> 8) & 0xFF) << 2),
                                (int32_t)(((scw >> 16) & 0xFF) << 2) };

  for (int blade = 0; blade < 2; blade++) {
    // Robj = colScale( rotmat(ax,ay,az) · rotY(yaw) )  — integer 1.3.12 math mirroring the guest chain.
    // Blade phase: the substrate advanced node[0x4e] by +0x800 per blade during exec, so this frame's
    // blades were yaw-0x1000 and yaw-0x800. (The pair {yaw-0x800, yaw} renders pixel-identically — the
    // two blades are 0x800-symmetric — verified by pixel-diff; either anchoring is the same picture.)
    const int16_t yaw = (int16_t)(yawNow - 0x1000 + blade * 0x800);
    int32_t M1[3][3], M2[3][3];
    MeshQuads::rotmat(c, ax, ay, az, M1);
    MeshQuads::rotY(c, yaw, M2);
    float Robj[3][3];
    MeshQuads::composeScaled(M1, M2, colScale, Robj);
    EObjXform w; projComposeObjectHost(Robj, Tobj, &w);
    projSetActive(&w);

    // mesh records (36 B): the shared walk. IR0 = 0 here, so the depth cue is a pass-through and the
    // record colours reach the screen unchanged; uBias = the guest's animated U scroll.
    const int32_t noFarColour[3] = { 0, 0, 0 };
    meshQuadRecordsEmit(MESH, (int)uScroll, noFarColour, /*ir0=*/0);
    projClearActive();
  }
}
