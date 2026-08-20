// game/ai/contact_stamp.cpp — see contact_stamp.h. Body is port_gen output, verbatim.
#include "contact_stamp.h"
#include "cfg.h" // cfg_dbg/cfg_logf — the one configurable logger
#include "core.h"
#include "game.h"
#include "ov_a00_decls.h"
#include "override_registry.h"

void rec_dispatch(Core *, uint32_t);

// ORACLE: ov_a00_gen_80111304
void ContactStamp::stampAndSnap(Core *c) {
  c->r[29] = c->r[29] + (uint32_t)-32;
  c->mem_w32((c->r[29] + (uint32_t)24), c->r[18]);
  c->r[18] = c->r[4] + c->r[0];
  c->mem_w32((c->r[29] + (uint32_t)20), c->r[17]);
  c->r[17] = c->r[5] + c->r[0];
  c->mem_w32((c->r[29] + (uint32_t)28), c->r[31]);
  c->mem_w32((c->r[29] + (uint32_t)16), c->r[16]);
  c->r[6] = (uint32_t)(int16_t)c->mem_r16((c->r[17] + (uint32_t)128));
  c->r[31] = 0x8011132Cu;
  c->r[6] = c->r[6] << 2;
  rec_dispatch(c, 0x8002300Cu);
  // DIAGNOSTIC (host-only, writes no guest memory). kanban #8's open question is whether this
  // overlap test ever passes: the producer runs ~1151 times per 1500 frames but no nonzero contact
  // index is ever observed, so either this returns zero every time or a clearer wipes the stamp in
  // the same frame. Counting the RETURN separates those two, which counting calls cannot.
  if (cfg_dbg("contact")) {
    static long calls = 0, passes = 0;
    // Distinct item pointers seen, so "the producer only ever sees one candidate" is a COUNT and
    // not an inference from every-500th-call sampling.
    static uint32_t seen[16] = {0};
    static int nseen = 0;
    {
      bool known = false;
      for (int k = 0; k < nseen; k++) {
        if (seen[k] == c->r[17]) {
          known = true;
          break;
        }
      }
      if (!known && nseen < 16) {
        seen[nseen++] = c->r[17];
      }
    }
    calls++;
    if (c->r[2] != 0) {
      passes++;
    }
    if ((calls % 500) == 0 || (c->r[2] != 0 && passes <= 5)) {
      // The test is 0x8002300C(G, item, radius<<2). If it always rejects, the interesting inputs
      // are the radius and how far apart the two records are — log both rather than guessing.
      const int32_t rad = (int32_t)(int16_t)c->mem_r16(c->r[17] + 128);
      const int32_t ix = (int16_t)c->mem_r16(c->r[17] + 0x2E), iz = (int16_t)c->mem_r16(c->r[17] + 0x36);
      const int32_t gx = (int16_t)c->mem_r16(c->r[18] + 0x2E), gz = (int16_t)c->mem_r16(c->r[18] + 0x36);
      cfg_logf("contact",
               "calls=%ld passes=%ld distinctItems=%d item=%08X rad=%d itemXZ=(%d,%d) G=%08X gXZ=(%d,%d) d=(%d,%d)",
               calls,
               passes,
               nseen,
               c->r[17],
               rad,
               ix,
               iz,
               c->r[18],
               gx,
               gz,
               ix - gx,
               iz - gz);
    }
  }
  {
    int _t = (c->r[2] == c->r[0]);
    c->r[2] = c->r[0] + (uint32_t)1;
    if (_t) {
      goto L_801114DC;
    }
  }
  c->mem_w8((c->r[17] + (uint32_t)43), (uint8_t)c->r[2]);
  c->r[2] = (uint32_t)8064u << 16;
  c->r[2] = (uint32_t)c->mem_r8((c->r[2] + (uint32_t)311));
  {
    int _t = (c->r[2] != c->r[0]);
    if (_t) {
      goto L_80111428;
    }
  }
  c->r[4] = (uint32_t)c->mem_r16((c->r[17] + (uint32_t)54));
  c->r[2] = (uint32_t)c->mem_r16((c->r[18] + (uint32_t)54));
  c->r[5] = (uint32_t)c->mem_r16((c->r[17] + (uint32_t)46));
  c->r[16] = (uint32_t)c->mem_r16((c->r[18] + (uint32_t)320));
  c->r[4] = c->r[4] - c->r[2];
  c->r[4] = c->r[4] << 16;
  c->r[4] = (uint32_t)((int32_t)c->r[4] >> 16);
  c->r[4] = c->r[0] - c->r[4];
  c->r[2] = (uint32_t)c->mem_r16((c->r[18] + (uint32_t)46));
  c->r[16] = c->r[16] + (uint32_t)-1024;
  c->r[5] = c->r[5] - c->r[2];
  c->r[5] = c->r[5] << 16;
  c->r[31] = 0x80111384u;
  c->r[5] = (uint32_t)((int32_t)c->r[5] >> 16);
  rec_dispatch(c, 0x80085690u);
  c->r[4] = c->r[2] << 4;
  c->r[16] = c->r[16] << 4;
  c->r[16] = c->r[4] - c->r[16];
  c->r[16] = c->r[16] << 16;
  c->r[16] = (uint32_t)((int32_t)c->r[16] >> 16);
  {
    int _t = ((int32_t)c->r[16] >= 0);
    if (_t) {
      goto L_801113A4;
    }
  }
  c->r[16] = c->r[0] - c->r[16];
L_801113A4:;
  c->r[16] = (uint32_t)((int32_t)c->r[16] < 4096);
  {
    int _t = (c->r[16] == c->r[0]);
    c->r[3] = (uint32_t)8064u << 16;
    if (_t) {
      goto L_801113D8;
    }
  }
  c->r[2] = c->r[0] + (uint32_t)16640;
  c->r[4] = (uint32_t)32780u << 16;
  c->mem_w16((c->r[3] + (uint32_t)398), (uint16_t)c->r[2]);
  c->r[3] = (uint32_t)c->mem_r16((c->r[18] + (uint32_t)320));
  c->r[2] = c->r[0] + (uint32_t)66;
  c->mem_w8((c->r[4] + (uint32_t)-1984), (uint8_t)c->r[2]);
  c->r[2] = (uint32_t)8064u << 16;
  c->r[3] = c->r[3] + (uint32_t)-1024;
  c->mem_w16((c->r[2] + (uint32_t)396), (uint16_t)c->r[3]);
  goto L_80111428;
L_801113D8:;
  c->r[2] = (uint32_t)c->mem_r16((c->r[18] + (uint32_t)320));
  c->r[3] = c->r[2] + (uint32_t)1024;
  c->r[2] = c->r[3] << 4;
  c->r[2] = c->r[4] - c->r[2];
  c->r[2] = c->r[2] << 16;
  c->r[2] = (uint32_t)((int32_t)c->r[2] >> 16);
  {
    int _t = ((int32_t)c->r[2] >= 0);
    if (_t) {
      goto L_80111400;
    }
  }
  c->r[2] = c->r[0] - c->r[2];
L_80111400:;
  c->r[2] = (uint32_t)((int32_t)c->r[2] < 4096);
  {
    int _t = (c->r[2] == c->r[0]);
    c->r[2] = (uint32_t)8064u << 16;
    if (_t) {
      goto L_80111428;
    }
  }
  c->mem_w16((c->r[2] + (uint32_t)396), (uint16_t)c->r[3]);
  c->r[3] = (uint32_t)8064u << 16;
  c->r[2] = c->r[0] + (uint32_t)16896;
  c->mem_w16((c->r[3] + (uint32_t)398), (uint16_t)c->r[2]);
  c->r[3] = (uint32_t)32780u << 16;
  c->r[2] = c->r[0] + (uint32_t)67;
  c->mem_w8((c->r[3] + (uint32_t)-1984), (uint8_t)c->r[2]);
L_80111428:;
  c->r[2] = (uint32_t)(int16_t)c->mem_r16((c->r[18] + (uint32_t)128));
  c->r[3] = (uint32_t)(int16_t)c->mem_r16((c->r[17] + (uint32_t)128));
  c->r[4] = (uint32_t)8064u << 16;
  c->r[2] = c->r[2] + c->r[3];
  c->r[3] = (uint32_t)c->mem_r16((c->r[4] + (uint32_t)140));
  c->r[2] = c->r[2] + (uint32_t)60;
  c->r[2] = (uint32_t)((int32_t)c->r[2] < (int32_t)c->r[3]);
  {
    int _t = (c->r[2] != c->r[0]);
    c->r[2] = c->r[0] + (uint32_t)2;
    if (_t) {
      goto L_801114DC;
    }
  }
  c->mem_w8((c->r[17] + (uint32_t)43), (uint8_t)c->r[2]);
  c->r[2] = (uint32_t)(int16_t)c->mem_r16((c->r[18] + (uint32_t)128));
  c->r[3] = (uint32_t)(int16_t)c->mem_r16((c->r[17] + (uint32_t)128));
  c->r[4] = (uint32_t)c->mem_r16((c->r[4] + (uint32_t)140));
  c->r[2] = c->r[2] + c->r[3];
  c->r[2] = (uint32_t)((int32_t)c->r[2] < (int32_t)c->r[4]);
  {
    int _t = (c->r[2] != c->r[0]);
    c->r[16] = (uint32_t)8064u << 16;
    if (_t) {
      goto L_801114DC;
    }
  }
  c->r[4] = c->mem_r32((c->r[16] + (uint32_t)156));
  c->r[31] = 0x80111478u;
  c->r[4] = c->r[4] + (uint32_t)2048;
  rec_dispatch(c, 0x80083F50u);
  c->r[3] = (uint32_t)(int16_t)c->mem_r16((c->r[18] + (uint32_t)128));
  c->r[4] = (uint32_t)(int16_t)c->mem_r16((c->r[17] + (uint32_t)128));
  c->r[3] = c->r[3] + c->r[4];
  {
    int64_t _p = (int64_t)(int32_t)c->r[2] * (int64_t)(int32_t)c->r[3];
    c->lo = (uint32_t)_p;
    c->hi = (uint32_t)((uint64_t)_p >> 32);
  }
  c->r[4] = c->mem_r32((c->r[16] + (uint32_t)156));
  c->r[4] = c->r[4] + (uint32_t)2048;
  c->r[7] = c->lo;
  c->r[31] = 0x801114A4u;
  c->r[16] = (uint32_t)((int32_t)c->r[7] >> 12);
  rec_dispatch(c, 0x80083E80u);
  c->r[3] = (uint32_t)(int16_t)c->mem_r16((c->r[18] + (uint32_t)128));
  c->r[4] = (uint32_t)(int16_t)c->mem_r16((c->r[17] + (uint32_t)128));
  c->r[3] = c->r[3] + c->r[4];
  {
    int64_t _p = (int64_t)(int32_t)c->r[2] * (int64_t)(int32_t)c->r[3];
    c->lo = (uint32_t)_p;
    c->hi = (uint32_t)((uint64_t)_p >> 32);
  }
  c->r[2] = (uint32_t)c->mem_r16((c->r[17] + (uint32_t)46));
  c->r[2] = c->r[2] + c->r[16];
  c->mem_w16((c->r[18] + (uint32_t)46), (uint16_t)c->r[2]);
  c->r[2] = (uint32_t)c->mem_r16((c->r[17] + (uint32_t)54));
  c->r[7] = c->lo;
  c->r[3] = (uint32_t)((int32_t)c->r[7] >> 12);
  c->r[2] = c->r[2] - c->r[3];
  c->mem_w16((c->r[18] + (uint32_t)54), (uint16_t)c->r[2]);
L_801114DC:;
  c->r[31] = c->mem_r32((c->r[29] + (uint32_t)28));
  c->r[18] = c->mem_r32((c->r[29] + (uint32_t)24));
  c->r[17] = c->mem_r32((c->r[29] + (uint32_t)20));
  c->r[16] = c->mem_r32((c->r[29] + (uint32_t)16));
  c->r[29] = c->r[29] + (uint32_t)32;
  return;
  return;
}

void ContactStamp::registerOverrides(Game *) {
  engine_set_override_a00(0x80111304u, &ContactStamp::stampAndSnap, ov_a00_gen_80111304);
}
