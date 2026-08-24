// game/render/projection.cpp — PC-NATIVE object render transform & projection (see projection.h).
//
// The per-object render is decoupled from the PSX GTE here. Render::projComposeObject builds the camera ×
// object transform in FLOAT from the object's real world coordinates; EObjXform::project runs the RTPT
// (rotate/translate + perspective divide) in float. The math mirrors the engine's projection (gte_beetle
// proj_native_xform's float path) so per-object geometry lines up with terrain and the rest of the scene —
// but the rotation and translation come from world-space data, not the GTE-composed control registers.
//
// Compose, in world space (all int16 matrix elements are 1.3.12 fixed = value*4096):
//   Rcam = scene camera view rotation   (scratchpad 0x1F8000F8, CR0-4 packing)
//   Tcam = scene camera view translation (scratchpad 0x1F80010C/110/114)
//   Robj = object world rotation matrix  (cmd+0x18, columns at +0/+2/+4, rows at +0/+6/+0xC)
//   Tobj = object world position         (cmd+0x2C/0x30/0x34)
//   R = (Rcam · Robj) / 4096     (composed rotation, kept in 1.3.12 scale)
//   T = (Rcam · Tobj) / 4096 + Tcam   (composed view translation)
// This is exactly view = Rcam·(Robj·v + Tobj) + Tcam — a standard model→world→view transform.

#include "projection.h"
#include "cfg.h"
#include "core.h"
#include "fps60.h"
#include "game.h"
#include "guest_face_gate.h" // GteFlag — the CR31 bits each clamp below raises
#include "render.h"
#include <lucent/log.h>
#include <stdio.h>

int gpu_frame_no(Core *); // the presented-frame counter the other render censuses tag their lines with

#define SCR 0x1F800000u

static inline int16_t r16(Core *c, uint32_t a) {
  return c->mem_r16s(a);
}

// Shared camera-compose core: R = (Rcam · Robj) / 4096, T = (Rcam · Tobj) / 4096 + Tcam, plus the
// camera's projection constants. Robj/Tobj are already-float object rotation/translation — either read
// from cmd+0x18/0x2C (projComposeObject) or host-computed by a caller whose object is stale in guest RAM
// (projComposeObjectHost). Camera state (Rcam/Tcam/ofx/ofy/H) is always read live — the camera itself
// is never stale.
static void projComposeCore(Core *c, const float Robj[3][3], const float Tobj[3], EObjXform *out) {
  // Scene camera (Rcam int16 rows, Tcam raw view units, OFX/OFY/H): the scratchpad view matrix plus the
  // projection constants the GAME SET, both through the shared Fps60::sceneCam choke (see fps60.cpp).
  float Rcam[3][3], Tcam[3], ofx, ofy, H;
  fps60(*c->game).sceneCam(c, Rcam, Tcam, ofx, ofy, H);

  // composed rotation R = (Rcam · Robj) / 4096, kept in 1.3.12 scale.
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 3; j++) {
      double s = (double)Rcam[i][0] * Robj[0][j] + (double)Rcam[i][1] * Robj[1][j] + (double)Rcam[i][2] * Robj[2][j];
      out->R[i][j] = (float)(s / 4096.0);
    }
  }
  // composed view translation T = (Rcam · Tobj) / 4096 + Tcam.
  for (int i = 0; i < 3; i++) {
    double s = (double)Rcam[i][0] * Tobj[0] + (double)Rcam[i][1] * Tobj[1] + (double)Rcam[i][2] * Tobj[2];
    out->T[i] = (float)(s / 4096.0) + Tcam[i];
  }
  out->ofx = ofx;
  out->ofy = ofy;
  out->H = H;
}

void Render::projComposeObject(uint32_t cmd, EObjXform *out) {
  Core *c = mCore;
  // Object world rotation Robj (cmd+0x18) / position Tobj (cmd+0x2C) go through the Fps60::projObj choke
  // (docs/fps60-rework.md unified-path redesign): real frame reads them live from guest RAM and captures
  // them keyed by cmd (byte-identical to the old inline read); the interp present re-run returns the
  // lerp(prev,cur,t) so the object interpolates through THIS same render path.
  float Robj[3][3], Tobj[3];
  fps60(*c->game).projObj(c, cmd, Robj, Tobj);

  projComposeCore(c, Robj, Tobj, out);
  if (cfg_dbg("eproj")) {
    static long n = 0;
    if (n++ % 240 == 0) {
      cfg_logf("eproj",
               "native compose #%ld cmd=%08x Tview=(%.0f,%.0f,%.0f) H=%.0f",
               n,
               cmd,
               (double)out->T[0],
               (double)out->T[1],
               (double)out->T[2],
               (double)out->H);
    }
  }
}

void Render::projComposeObjectHost(const float Robj[3][3], const float Tobj[3], EObjXform *out) {
  projComposeCore(mCore, Robj, Tobj, out);
}

void Render::projComposeCamera(EObjXform *out) {
  Core *c = mCore;
  // The field's entity (scene-table) verts are already world-space, so view = Rcam·world + Tcam and the
  // composed rotation IS the camera rotation. Read through the shared Fps60::sceneCam choke.
  fps60(*c->game).sceneCam(c, out->R, out->T, out->ofx, out->ofy, out->H);
}

// EVERY clamp below is a GTE saturation, and the guest's geometry submitters DROP a face whose GTE
// FLAG (CR31) error bit came back set — so each clamp is also a cull the real game performs. The flag
// word records which fired; see game/render/guest_face_gate.h for the four gates and the guest code
// they were read from. `project` is `projectFlags` with the report thrown away, so there is exactly
// one projection body and the flag can never drift from the clamp that raises it.
uint32_t EObjXform::projectFlags(int vx, int vy, int vz, ProjVtx *out) const {
  uint32_t flag = 0;
  const float V0 = (float)(int16_t)vx, V1 = (float)(int16_t)vy, V2 = (float)(int16_t)vz;
  // view = R·V + T  (R in 1.3.12 scale, so divide the rotate product by 4096; T is raw view units).
  double view[3], vz_raw = 0;
  for (int i = 0; i < 3; i++) {
    double t = (double)T[i] * 4096.0 + (double)R[i][0] * V0 + (double)R[i][1] * V1 + (double)R[i][2] * V2;
    if (i == 2) {
      vz_raw = t;
    }
    // MAC1..3 hold this product in 1/4096 units and overflow past 43 bits.
    if (t >= 4398046511104.0 || t < -4398046511104.0) {
      flag |= (i == 0 ? GteFlag::MAC1_OVF : i == 1 ? GteFlag::MAC2_OVF : GteFlag::MAC3_OVF);
    }
    view[i] = t / 4096.0;
  }
  // IR saturation to ±32767 (kept so per-object geometry lines up with the rest of the projection pipeline).
  if (view[0] < -32768 || view[0] > 32767) {
    flag |= GteFlag::IR1_SAT;
  }
  if (view[1] < -32768 || view[1] > 32767) {
    flag |= GteFlag::IR2_SAT;
  }
  if (view[2] < -32768 || view[2] > 32767) {
    flag |= GteFlag::IR3_SAT;
  }
  float ir1 = (float)(view[0] < -32768 ? -32768 : view[0] > 32767 ? 32767 : view[0]);
  float ir2 = (float)(view[1] < -32768 ? -32768 : view[1] > 32767 ? 32767 : view[1]);
  float ir3 = (float)(view[2] < -32768 ? -32768 : view[2] > 32767 ? 32767 : view[2]);
  out->ir1 = (int)ir1;
  out->ir2 = (int)ir2;
  out->ir3 = (int)ir3;
  out->vx = ir1;
  out->vy = ir2;
  out->vz = ir3;
  float szf = (float)(vz_raw / 4096.0);
  if (szf < 0.0f || szf > 65535.0f) {
    flag |= GteFlag::SZ3_SAT;
  }
  int32_t szi = (int32_t)szf;
  out->sz = szi < 0 ? 0 : szi > 65535 ? 65535 : szi;
  // perspective: pz = max(H/2, view-Z); screen = OFX/OFY + IR * (H / pz).
  // The GTE's divide saturates (and raises bit 17) exactly when SZ3 == 0 or H >= 2*SZ3 — THE NEAR
  // PLANE. `pz = max(H/2, szf)` is that same saturation; flag it from the integer SZ3 the hardware
  // divides by, not from the float, so the boundary case matches the hardware's.
  if (out->sz == 0 || (int32_t)H >= out->sz * 2) {
    flag |= GteFlag::DIV_OVF;
  }
  float pz = H * 0.5f;
  if (szf > pz) {
    pz = szf;
  }
  float ph = (pz > 0.0f) ? H / pz : 0.0f;
  out->px = ofx + ir1 * ph;
  out->py = ofy + ir2 * ph;
  // MAC0 holds the screen coordinate in 1/65536 units and overflows past 31 bits before SX2/SY2 are
  // taken; at these magnitudes SX2/SY2 saturation has already fired, but the guest's test is the OR.
  if (out->px * 65536.0f >= 2147483648.0f || out->px * 65536.0f < -2147483648.0f ||
      out->py * 65536.0f >= 2147483648.0f || out->py * 65536.0f < -2147483648.0f) {
    flag |= GteFlag::MAC0_OVF;
  }
  if (out->px < -1024.f || out->px > 1023.f) {
    flag |= GteFlag::SX2_SAT;
  }
  if (out->py < -1024.f || out->py > 1023.f) {
    flag |= GteFlag::SY2_SAT;
  }
  if (out->px < -1024.f) {
    out->px = -1024.f;
  }
  if (out->px > 1023.f) {
    out->px = 1023.f;
  }
  if (out->py < -1024.f) {
    out->py = -1024.f;
  }
  if (out->py > 1023.f) {
    out->py = 1023.f;
  }
  int32_t sxi = (int32_t)(out->px < 0 ? out->px - 0.5f : out->px + 0.5f);
  int32_t syi = (int32_t)(out->py < 0 ? out->py - 0.5f : out->py + 0.5f);
  out->sx = sxi < -1024 ? -1024 : sxi > 1023 ? 1023 : sxi;
  out->sy = syi < -1024 ? -1024 : syi > 1023 ? 1023 : syi;
  out->pz = pz;
  return flag;
}

void EObjXform::project(int vx, int vy, int vz, ProjVtx *out) const {
  (void)projectFlags(vx, vy, vz, out);
}

// The active object xform: set once per render command by the per-object flush; the GT3/GT4 submitters
// project every vertex through it. There is NO GTE fallback — a submitter that runs in the per-object path
// always has an active xform.
void Render::projSetActive(const EObjXform *w) {
  mActiveXform = *w;
  mActiveXformSet = true;
}
void Render::projClearActive() {
  mActiveXformSet = false;
}
void Render::projVertexActive(int vx, int vy, int vz, ProjVtx *out) {
  mActiveXform.project(vx, vy, vz, out);
}
uint32_t Render::projVertexActiveFlags(int vx, int vy, int vz, ProjVtx *out) {
  return mActiveXform.projectFlags(vx, vy, vz, out);
}

static inline int32_t round_i16(float f) {
  int32_t v = (int32_t)(f < 0 ? f - 0.5f : f + 0.5f);
  return v < -32768 ? -32768 : v > 32767 ? 32767 : v;
}
static inline int32_t round_i32(float f) {
  return (int32_t)(f < 0 ? f - 0.5f : f + 0.5f);
}
// Unpack the scratchpad scene view matrix. Read the halfword picks against projActiveCr directly below:
// cr[0]=R00|R01<<16, cr[1]=R02|R10<<16, cr[2]=R11|R12<<16, cr[3]=R20|R21<<16, cr[4]=R22 (LOW half —
// the upper half of that word is not part of the matrix), cr[5..7]=T. Anything that needs this layout
// calls here rather than re-deriving it; a second hand-written copy is how R22 once became w4>>16 and
// fed the whole native projection path a corrupt view-Z row.
void Render::readSceneViewMatrix(Core *c, float R[3][3], float T[3]) {
  constexpr uint32_t kSceneViewMatrix = 0x1F8000F8u; // scratchpad; written where the guest's libgte
                                                     // SetRotMatrix/SetTransMatrix record the camera
  const uint32_t w0 = c->mem_r32(kSceneViewMatrix + 0), w1 = c->mem_r32(kSceneViewMatrix + 4);
  const uint32_t w2 = c->mem_r32(kSceneViewMatrix + 8), w3 = c->mem_r32(kSceneViewMatrix + 12);
  const uint32_t w4 = c->mem_r32(kSceneViewMatrix + 16);
  R[0][0] = (int16_t)w0;
  R[0][1] = (int16_t)(w0 >> 16);
  R[0][2] = (int16_t)w1;
  R[1][0] = (int16_t)(w1 >> 16);
  R[1][1] = (int16_t)w2;
  R[1][2] = (int16_t)(w2 >> 16);
  R[2][0] = (int16_t)w3;
  R[2][1] = (int16_t)(w3 >> 16);
  R[2][2] = (int16_t)w4;
  for (int i = 0; i < 3; i++) {
    T[i] = (float)(int32_t)c->mem_r32(kSceneViewMatrix + 0x14u + (uint32_t)i * 4u);
  }
}

void Render::projActiveCr(uint32_t cr[11]) {
  // pack R (1.3.12 scale) into CR0-4 halfword layout, T into CR5-7, projection consts into cr[8..10].
  const EObjXform &a = mActiveXform;
  uint16_t R[3][3];
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 3; j++) {
      R[i][j] = (uint16_t)round_i16(a.R[i][j]);
    }
  }
  cr[0] = (uint32_t)R[0][0] | ((uint32_t)R[0][1] << 16);
  cr[1] = (uint32_t)R[0][2] | ((uint32_t)R[1][0] << 16);
  cr[2] = (uint32_t)R[1][1] | ((uint32_t)R[1][2] << 16);
  cr[3] = (uint32_t)R[2][0] | ((uint32_t)R[2][1] << 16);
  cr[4] = (uint32_t)R[2][2];
  cr[5] = (uint32_t)round_i32(a.T[0]);
  cr[6] = (uint32_t)round_i32(a.T[1]);
  cr[7] = (uint32_t)round_i32(a.T[2]);
  cr[8] = (uint32_t)(int32_t)(a.ofx * 65536.0f);
  cr[9] = (uint32_t)(int32_t)(a.ofy * 65536.0f);
  cr[10] = (uint32_t)(uint16_t)a.H;
}

// ── The guest-face-gate census verdict (game/render/guest_face_gate.h) ─────────────────────────────
// One line per rendered frame on `PSXPORT_DEBUG=guestgate`. It prints the DENOMINATOR first and names
// what it cannot see, because the failure this instrument exists to prevent is a silent zero being read
// as "the game keeps every face we draw".
void Render::guestGateFlush() {
  GuestFaceGateCensus &g = mGuestGate;
  lucent::debug("guestgate",
                "f{} faces={} droppedGTE={} droppedOTKEY={} keyUnknown={} flagsSeen={:08X}"
                " [scope: the GT3/GT4 per-object submitters only — the tile/sprite/2D producers and any face"
                " this port already culled for backface or screen bounds are OUTSIDE this count]",
                gpu_frame_no(mCore),
                g.faces,
                g.dropGte,
                g.dropOtKey,
                g.unknownKey,
                g.flagsSeen);
  g.reset();
}
