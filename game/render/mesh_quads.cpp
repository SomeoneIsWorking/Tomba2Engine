// game/render/mesh_quads.cpp — MeshQuads (see mesh_quads.h) + Render::meshQuadRecordsEmit, the ONE
// host-side walk of the engine's packed-mesh quad-record format.
//
// The format is FUN_80027768's: a run of 36-byte records, drawn until (and including) the one whose
// word1 has bit31 set.
//   +0  uv0 word : u0 | v0<<8 | clut<<16          +12 RGB0   (byte3 = vertex 0 model Z, s8)
//   +4  uv1 word : u1 | v1<<8 | tpage<<16,        +16 RGB1   (byte3 = vertex 1 Z)
//                  bit30 = semi-transparent,      +20 RGB2   (byte3 = vertex 2 Z)
//                  bit31 = last record            +24 RGB3   (byte3 = vertex 3 Z)
//   +8  uv2 word (uv3 = uv2 >> 16)                +28 s8 x0,x1  +30 s8 y0,y1
//                                                 +32 s8 x2,x3  +34 s8 y2,y3
// Model coordinates are the signed bytes * 256. There is NO backface test in this emitter (the guest
// only checks the GTE overflow flag), so none is added. Every U byte takes the caller's uBias (the
// animated texture scroll these effects use as their frame counter).
//
// COLOUR: the guest runs the four RGBs through DPCT/DPCS, a lerp toward the GTE far colour by the
// depth-cue factor IR0. Callers that publish IR0 = 0 (the narration swirl) get their colours through
// unchanged; the dust puff publishes IR0 = 0xFFF, which replaces the mesh's own colour with the
// per-surface-material far colour — that is HOW the puff is tinted by the ground it comes off.
//
// Submission is RenderQueue::drawWorldQuad with float corners (has_xyf = 1), so these prims are
// tier1-owned and the interp present re-renders them under the lerped camera.
#include "mesh_quads.h"
#include "core.h"
#include "game.h"
#include "render.h"
#include "render_queue.h"
#include "proj_params.h"

namespace {
constexpr uint32_t kTrigLut   = 0x800A6490u;   // word = cos<<16 | sin, 4096 entries
constexpr uint32_t kRecStride = 36u;
constexpr uint32_t kRecMax    = 512u;          // runaway guard; the guest's terminator is word1 bit31
constexpr uint32_t kSemiBit   = 0x40000000u;

int16_t gpf(int a, int b) {                    // the GPF product-and-clamp the rotmat leaf uses
  int32_t v = ((int32_t)a * b) >> 12;
  return (int16_t)(v < -32768 ? -32768 : v > 32767 ? 32767 : v);
}

// One channel of the guest's DPCS/DPCT cue toward the far colour (sf=1, lm=0):
//   MAC = c<<16 ; IR = clamp16(((FC<<12) - MAC) >> 12) ; MAC = (IR*IR0 + MAC) >> 12 ; out = MAC/16.
unsigned char depthCue(int colour, int32_t farColour, int32_t ir0) {
  const int32_t mac = colour << 16;
  int32_t ir = ((farColour << 12) - mac) >> 12;
  if (ir < -32768) ir = -32768; else if (ir > 32767) ir = 32767;
  int32_t out = ((ir * ir0 + mac) >> 12) / 16;
  if (out < 0) out = 0; else if (out > 255) out = 255;
  return (unsigned char)out;
}
}  // namespace

void MeshQuads::trig(Core* c, int32_t angle, int* sinOut, int* cosOut) {
  const int32_t sign = angle >> 31;
  const uint32_t absa = (uint32_t)((angle + sign) ^ sign);
  const uint32_t word = c->mem_r32(kTrigLut + ((absa << 2) & 0x3FFCu));
  *cosOut = (int)((int32_t)word >> 16);
  uint32_t at = (word << 16); at = (at + (uint32_t)sign) ^ (uint32_t)sign;   // sin is odd in the angle
  *sinOut = (int)(int16_t)(at >> 16);
}

void MeshQuads::rotmat(Core* c, int16_t ax, int16_t ay, int16_t az, int32_t M[3][3]) {
  int sx, cx, sy, cy, sz, cz;
  trig(c, ax, &sx, &cx); trig(c, ay, &sy, &cy); trig(c, az, &sz, &cz);
  const int16_t cxsy = gpf(cx, sy), cxsz = gpf(cx, sz), cxcz = gpf(cx, cz);
  const int16_t sxsy = gpf(sx, sy), sxsz = gpf(sx, sz), sxcz = gpf(sx, cz);
  const int16_t czcy = gpf(cz, cy), czSxsy = gpf(cz, sxsy), czCxsy = gpf(cz, cxsy);
  const int16_t szcy = gpf(sz, cy), szSxsy = gpf(sz, sxsy), szCxsy = gpf(sz, cxsy);
  const int16_t cycx = (int16_t)(((int32_t)cy * cx) >> 12), cysx = (int16_t)(((int32_t)cy * sx) >> 12);
  M[0][0] = czcy;                       M[0][1] = -szcy;                 M[0][2] = sy;
  M[1][0] = (int16_t)(czSxsy + cxsz);   M[1][1] = (int16_t)(cxcz - szSxsy); M[1][2] = (int16_t)-cysx;
  M[2][0] = (int16_t)(sxsz - czCxsy);   M[2][1] = (int16_t)(sxcz + szCxsy); M[2][2] = cycx;
}

void MeshQuads::rotY(Core* c, int16_t angle, int32_t M[3][3]) {
  int s, co; trig(c, angle, &s, &co); s = -s;             // the rows-0/2 variant flips the sin sign
  M[0][0] = co; M[0][1] = 0;    M[0][2] = -s;
  M[1][0] = 0;  M[1][1] = 4096; M[1][2] = 0;
  M[2][0] = s;  M[2][1] = 0;    M[2][2] = co;
}

void MeshQuads::rotZ(Core* c, int16_t angle, int32_t M[3][3]) {
  int s, co; trig(c, angle, &s, &co);
  M[0][0] = co; M[0][1] = -s; M[0][2] = 0;
  M[1][0] = s;  M[1][1] = co; M[1][2] = 0;
  M[2][0] = 0;  M[2][1] = 0;  M[2][2] = 4096;
}

void MeshQuads::fromGuest(Core* c, uint32_t matPtr, int32_t M[3][3]) {
  for (int i = 0; i < 3; i++)
    for (int j = 0; j < 3; j++) M[i][j] = c->mem_r16s(matPtr + (uint32_t)(i * 3 + j) * 2u);
}

void MeshQuads::composeScaled(const int32_t A[3][3], const int32_t B[3][3], const int32_t colScale[3],
                              float out[3][3]) {
  for (int i = 0; i < 3; i++)
    for (int j = 0; j < 3; j++) {
      int32_t s = (int32_t)(((int64_t)A[i][0] * B[0][j] + (int64_t)A[i][1] * B[1][j] +
                             (int64_t)A[i][2] * B[2][j]) >> 12);
      if (s < -32768) s = -32768; else if (s > 32767) s = 32767;    // the multiply leaf's IR clamp
      out[i][j] = (float)((s * colScale[j]) >> 12);
    }
}

int Render::meshQuadRecordsEmit(uint32_t mesh, int uBias, const int32_t farColour[3], int32_t ir0) {
  Core* c = mCore;
  int drawn = 0;
  for (uint32_t n = 0, rec = mesh; n < kRecMax; n++, rec += kRecStride) {
    const uint32_t w0 = c->mem_r32(rec + 0u), w1 = c->mem_r32(rec + 4u), w2 = c->mem_r32(rec + 8u);
    auto sb = [&](uint32_t off) { return (int)(int8_t)c->mem_r8(rec + off) * 256; };
    const int vx[4] = { sb(28), sb(29), sb(32), sb(33) };
    const int vy[4] = { sb(30), sb(31), sb(34), sb(35) };
    const int vz[4] = { sb(15), sb(19), sb(23), sb(27) };

    float px[4], py[4], depth[4];
    for (int k = 0; k < 4; k++) {
      ProjVtx p; projVertexActive(vx[k], vy[k], vz[k], &p);
      px[k] = p.px; py[k] = p.py; depth[k] = proj_pz_to_ord(p.pz);
    }
    unsigned char r[4], g[4], b[4];
    for (int k = 0; k < 4; k++) {
      const uint32_t cw = c->mem_r32(rec + 12u + (uint32_t)k * 4u);
      r[k] = depthCue((int)(cw & 0xFFu),         farColour[0], ir0);
      g[k] = depthCue((int)((cw >> 8) & 0xFFu),  farColour[1], ir0);
      b[k] = depthCue((int)((cw >> 16) & 0xFFu), farColour[2], ir0);
    }
    const int u[4] = { (int)(uint8_t)((w0 & 0xFFu)         + (unsigned)uBias),
                       (int)(uint8_t)((w1 & 0xFFu)         + (unsigned)uBias),
                       (int)(uint8_t)((w2 & 0xFFu)         + (unsigned)uBias),
                       (int)(uint8_t)(((w2 >> 16) & 0xFFu) + (unsigned)uBias) };
    const int v[4] = { (int)((w0 >> 8) & 0xFFu), (int)((w1 >> 8) & 0xFFu),
                       (int)((w2 >> 8) & 0xFFu), (int)((w2 >> 24) & 0xFFu) };
    const uint16_t clut = (uint16_t)(w0 >> 16);
    const uint16_t tp   = (uint16_t)((w1 >> 16) & 0x7Fu);
    const int semi = (w1 & kSemiBit) ? 1 : 0;
    c->game->activeRq().drawWorldQuad(c, px, py, depth, u, v, r, g, b, tp, clut, semi, nullptr);
    drawn++;
    if ((int32_t)w1 <= 0) break;                         // the terminal record IS drawn, then stop
  }
  return drawn;
}
