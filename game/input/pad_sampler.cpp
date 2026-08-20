// game/input/pad_sampler.cpp — see pad_sampler.h. Body is port_gen output, verbatim.
#include "pad_sampler.h"
#include "core.h"
#include "game.h"
#include "override_registry.h"
#include "rec_decls.h"

extern void func_80087BB8(Core *);
extern void func_80052144(Core *);
extern void func_80052198(Core *);

// ORACLE: gen_func_800524B4
void PadSampler::sampleButtonMask(Core *c) {
  c->r[2] = (uint32_t)32780u << 16;
  c->r[3] = (uint32_t)c->mem_r8((c->r[2] + (uint32_t)-2824));
  c->r[29] = c->r[29] + (uint32_t)-40;
  c->mem_w32((c->r[29] + (uint32_t)28), c->r[19]);
  c->r[19] = c->r[4] + c->r[0];
  c->mem_w32((c->r[29] + (uint32_t)32), c->r[20]);
  c->r[20] = (uint32_t)32783u << 16;
  c->mem_w32((c->r[29] + (uint32_t)20), c->r[17]);
  c->r[17] = c->r[20] + (uint32_t)-12472;
  c->mem_w32((c->r[29] + (uint32_t)24), c->r[18]);
  c->r[18] = c->r[2] + (uint32_t)-2824;
  c->mem_w32((c->r[29] + (uint32_t)36), c->r[31]);
  {
    int _t = (c->r[3] == c->r[0]);
    c->mem_w32((c->r[29] + (uint32_t)16), c->r[16]);
    if (_t) {
      goto L_800524F4;
    }
  }
  c->r[2] = c->r[0] + c->r[0];
  goto L_800525B0;
L_800524F4:;
  c->r[4] = c->r[0] + c->r[0];
  c->r[5] = c->r[0] + (uint32_t)2;
  c->r[31] = 0x80052504u;
  c->r[6] = c->r[4] + c->r[0];
  func_80087BB8(c);
  c->r[3] = (uint32_t)c->mem_r16((c->r[18] + (uint32_t)2));
  c->mem_w16((c->r[17] + (uint32_t)2), (uint16_t)c->r[2]);
  c->r[2] = (uint32_t)c->mem_r8((c->r[18] + (uint32_t)1));
  c->r[3] = ~(c->r[0] | c->r[3]);
  c->r[2] = c->r[2] >> 4;
  c->r[2] = c->r[2] & 7u;
  c->r[4] = c->r[2] & 65535u;
  c->mem_w16((c->r[20] + (uint32_t)-12472), (uint16_t)c->r[2]);
  c->r[2] = c->r[0] + (uint32_t)4;
  {
    int _t = (c->r[4] == c->r[2]);
    c->r[16] = c->r[3] + c->r[0];
    if (_t) {
      goto L_80052544;
    }
  }
  c->r[2] = c->r[0] + (uint32_t)7;
  {
    int _t = (c->r[4] == c->r[2]);
    c->r[2] = c->r[0] + c->r[0];
    if (_t) {
      goto L_80052550;
    }
  }
  goto L_800525B0;
L_80052544:;
  c->mem_w8((c->r[17] + (uint32_t)8), (uint8_t)c->r[0]);
  c->mem_w8((c->r[17] + (uint32_t)9), (uint8_t)c->r[0]);
  goto L_800525A8;
L_80052550:;
  c->r[2] = c->r[19] << 16;
  {
    int _t = (c->r[2] != c->r[0]);
    c->r[2] = c->r[16] << 16;
    if (_t) {
      goto L_800525AC;
    }
  }
  c->r[2] = c->r[3] & 240u;
  {
    int _t = (c->r[2] != c->r[0]);
    c->r[5] = c->r[0] + c->r[0];
    if (_t) {
      goto L_80052588;
    }
  }
  c->r[4] = (uint32_t)c->mem_r8((c->r[18] + (uint32_t)6));
  c->r[31] = 0x80052574u;
  c->r[16] = c->r[3] & 65295u;
  func_80052144(c);
  c->r[5] = c->r[0] + (uint32_t)1;
  c->r[4] = (uint32_t)c->mem_r8((c->r[18] + (uint32_t)7));
  c->r[31] = 0x80052584u;
  c->r[16] = c->r[16] | c->r[2];
  func_80052144(c);
  c->r[16] = c->r[16] | c->r[2];
L_80052588:;
  c->r[4] = (uint32_t)c->mem_r8((c->r[18] + (uint32_t)6));
  c->r[31] = 0x80052594u;
  c->r[5] = c->r[17] + (uint32_t)10;
  func_80052198(c);
  c->r[4] = (uint32_t)c->mem_r8((c->r[18] + (uint32_t)7));
  c->r[5] = c->r[17] + (uint32_t)11;
  c->r[31] = 0x800525A4u;
  c->mem_w8((c->r[17] + (uint32_t)8), (uint8_t)c->r[2]);
  func_80052198(c);
  c->mem_w8((c->r[17] + (uint32_t)9), (uint8_t)c->r[2]);
L_800525A8:;
  c->r[2] = c->r[16] << 16;
L_800525AC:;
  c->r[2] = (uint32_t)((int32_t)c->r[2] >> 16);
L_800525B0:;
  c->r[31] = c->mem_r32((c->r[29] + (uint32_t)36));
  c->r[20] = c->mem_r32((c->r[29] + (uint32_t)32));
  c->r[19] = c->mem_r32((c->r[29] + (uint32_t)28));
  c->r[18] = c->mem_r32((c->r[29] + (uint32_t)24));
  c->r[17] = c->mem_r32((c->r[29] + (uint32_t)20));
  c->r[16] = c->mem_r32((c->r[29] + (uint32_t)16));
  c->r[29] = c->r[29] + (uint32_t)40;
  return;
  return;
}

void PadSampler::registerOverrides(Game *) {
  engine_set_override_main(0x800524B4u, &PadSampler::sampleButtonMask, gen_func_800524B4);
}
