// game/render/ui_ft4_layout.cpp — see ui_ft4_layout.h. Body is port_gen output, verbatim.
#include "ui_ft4_layout.h"
#include "core.h"
#include "game.h"
#include "override_registry.h"
#include "rec_decls.h"

extern void func_8007E620(Core *);

// ORACLE: gen_func_8007E2F8
void UiFt4Layout::plainQuadVerts(Core *c) {
  c->r[2] = c->mem_r32((c->r[12] + (uint32_t)0));
  c->mem_w32((c->r[8] + (uint32_t)8), c->r[2]);
  c->r[2] = (uint32_t)c->mem_r8((c->r[9] + (uint32_t)-1));
  c->r[3] = (uint32_t)c->mem_r16((c->r[8] + (uint32_t)8));
  c->r[2] = c->r[2] << 24;
  c->r[2] = (uint32_t)((int32_t)c->r[2] >> 24);
  c->r[3] = c->r[3] + c->r[2];
  c->mem_w16((c->r[8] + (uint32_t)8), (uint16_t)c->r[3]);
  c->r[2] = (uint32_t)c->mem_r8((c->r[9] + (uint32_t)0));
  c->r[3] = (uint32_t)c->mem_r16((c->r[8] + (uint32_t)10));
  c->r[2] = c->r[2] << 24;
  c->r[2] = (uint32_t)((int32_t)c->r[2] >> 24);
  c->r[3] = c->r[3] + c->r[2];
  c->r[2] = (uint32_t)c->mem_r16((c->r[8] + (uint32_t)8));
  c->mem_w16((c->r[8] + (uint32_t)10), (uint16_t)c->r[3]);
  c->r[3] = c->r[3] + c->r[5];
  c->mem_w16((c->r[8] + (uint32_t)26), (uint16_t)c->r[3]);
  c->r[3] = (uint32_t)c->mem_r16((c->r[8] + (uint32_t)10));
  c->r[5] = (uint32_t)c->mem_r16((c->r[8] + (uint32_t)26));
  c->r[2] = c->r[2] + c->r[6];
  c->mem_w16((c->r[8] + (uint32_t)16), (uint16_t)c->r[2]);
  c->r[2] = (uint32_t)c->mem_r16((c->r[8] + (uint32_t)8));
  c->r[4] = (uint32_t)c->mem_r16((c->r[8] + (uint32_t)16));
  c->mem_w16((c->r[8] + (uint32_t)18), (uint16_t)c->r[3]);
  c->mem_w16((c->r[8] + (uint32_t)34), (uint16_t)c->r[5]);
  c->mem_w16((c->r[8] + (uint32_t)24), (uint16_t)c->r[2]);
  c->mem_w16((c->r[8] + (uint32_t)32), (uint16_t)c->r[4]);
  func_8007E620(c);
  return;
  return;
}

// FUN_0x8007E36C — layout mode 1 — X-MIRRORED. Base XY goes to VERTEX 1 rather than 0 and the width is added to the
// other x pair, so the corners come out (x+w,y) (x,y) (x+w,y+h) (x,y+h); all four u bytes are
// decremented by 1, the texel-edge fixup that pairs with a horizontal flip. The y layout is
// byte-for-byte case 0.
//
// Like case 0 this is a CASE BLOCK, not a function: no frame, no sp, and it TAIL-JUMPS to the shared
// OT-link tail. abi_extract prints `r[31] = MISSING` and that is correct, not a defect — this is a
// `j`, so r31 must keep the value it arrived with. (My batch brief said these blocks write the ra
// constant unconditionally; the verifier corrected that — they do not write it at all.)
// ORACLE: gen_func_8007E36C
void UiFt4Layout::xMirroredQuadVerts(Core *c) {
  c->r[2] = c->mem_r32((c->r[12] + (uint32_t)0));
  c->mem_w32((c->r[8] + (uint32_t)16), c->r[2]);
  c->r[2] = (uint32_t)c->mem_r8((c->r[9] + (uint32_t)-1));
  c->r[3] = (uint32_t)c->mem_r16((c->r[8] + (uint32_t)16));
  c->r[2] = c->r[2] << 24;
  c->r[2] = (uint32_t)((int32_t)c->r[2] >> 24);
  c->r[3] = c->r[3] + c->r[2];
  c->mem_w16((c->r[8] + (uint32_t)16), (uint16_t)c->r[3]);
  c->r[4] = (uint32_t)c->mem_r8((c->r[9] + (uint32_t)0));
  c->r[2] = (uint32_t)c->mem_r8((c->r[8] + (uint32_t)12));
  c->r[3] = (uint32_t)c->mem_r8((c->r[8] + (uint32_t)20));
  c->r[2] = c->r[2] + (uint32_t)-1;
  c->r[3] = c->r[3] + (uint32_t)-1;
  c->r[4] = c->r[4] << 24;
  c->mem_w8((c->r[8] + (uint32_t)12), (uint8_t)c->r[2]);
  c->r[2] = (uint32_t)c->mem_r16((c->r[8] + (uint32_t)18));
  c->r[4] = (uint32_t)((int32_t)c->r[4] >> 24);
  c->mem_w8((c->r[8] + (uint32_t)20), (uint8_t)c->r[3]);
  c->r[2] = c->r[2] + c->r[4];
  c->mem_w16((c->r[8] + (uint32_t)18), (uint16_t)c->r[2]);
  c->r[2] = (uint32_t)c->mem_r8((c->r[8] + (uint32_t)28));
  c->r[3] = (uint32_t)c->mem_r16((c->r[8] + (uint32_t)18));
  c->r[2] = c->r[2] + (uint32_t)-1;
  c->mem_w8((c->r[8] + (uint32_t)28), (uint8_t)c->r[2]);
  c->r[2] = (uint32_t)c->mem_r16((c->r[8] + (uint32_t)16));
  c->mem_w16((c->r[8] + (uint32_t)10), (uint16_t)c->r[3]);
  c->r[3] = c->r[3] + c->r[5];
  c->mem_w16((c->r[8] + (uint32_t)26), (uint16_t)c->r[3]);
  c->r[3] = (uint32_t)c->mem_r16((c->r[8] + (uint32_t)16));
  c->r[4] = (uint32_t)c->mem_r16((c->r[8] + (uint32_t)26));
  c->r[2] = c->r[2] + c->r[6];
  c->mem_w16((c->r[8] + (uint32_t)8), (uint16_t)c->r[2]);
  c->r[5] = c->r[2] + c->r[0];
  c->r[2] = (uint32_t)c->mem_r8((c->r[8] + (uint32_t)36));
  c->mem_w16((c->r[8] + (uint32_t)32), (uint16_t)c->r[3]);
  c->mem_w16((c->r[8] + (uint32_t)24), (uint16_t)c->r[5]);
  c->mem_w16((c->r[8] + (uint32_t)34), (uint16_t)c->r[4]);
  c->r[2] = c->r[2] + (uint32_t)-1;
  c->mem_w8((c->r[8] + (uint32_t)36), (uint8_t)c->r[2]);
  func_8007E620(c);
  return;
  return;
}

// FUN_0x8007E410 — layout mode 2.
//
// Like case 0 this is a CASE BLOCK, not a function: no frame, no sp, and it TAIL-JUMPS to the shared
// OT-link tail. abi_extract prints `r[31] = MISSING` and that is correct, not a defect — this is a
// `j`, so r31 must keep the value it arrived with. (My batch brief said these blocks write the ra
// constant unconditionally; the verifier corrected that — they do not write it at all.)
// ORACLE: gen_func_8007E410
void UiFt4Layout::vMirroredQuadVerts(Core *c) {
  c->r[2] = c->mem_r32((c->r[12] + (uint32_t)0));
  c->mem_w32((c->r[8] + (uint32_t)24), c->r[2]);
  c->r[2] = (uint32_t)c->mem_r8((c->r[9] + (uint32_t)-1));
  c->r[3] = (uint32_t)c->mem_r16((c->r[8] + (uint32_t)24));
  c->r[2] = c->r[2] << 24;
  c->r[2] = (uint32_t)((int32_t)c->r[2] >> 24);
  c->r[3] = c->r[3] + c->r[2];
  c->r[2] = (uint32_t)c->mem_r8((c->r[8] + (uint32_t)13));
  c->mem_w16((c->r[8] + (uint32_t)24), (uint16_t)c->r[3]);
  c->r[4] = (uint32_t)c->mem_r8((c->r[9] + (uint32_t)0));
  c->r[3] = (uint32_t)c->mem_r8((c->r[8] + (uint32_t)29));
  c->r[2] = c->r[2] + (uint32_t)-1;
  c->r[3] = c->r[3] + (uint32_t)-1;
  c->mem_w8((c->r[8] + (uint32_t)13), (uint8_t)c->r[2]);
  c->r[2] = (uint32_t)c->mem_r8((c->r[8] + (uint32_t)21));
  c->r[4] = c->r[4] << 24;
  c->mem_w8((c->r[8] + (uint32_t)29), (uint8_t)c->r[3]);
  c->r[3] = (uint32_t)c->mem_r16((c->r[8] + (uint32_t)24));
  c->r[2] = c->r[2] + (uint32_t)-1;
  c->mem_w8((c->r[8] + (uint32_t)21), (uint8_t)c->r[2]);
  c->r[2] = (uint32_t)c->mem_r16((c->r[8] + (uint32_t)26));
  c->r[4] = (uint32_t)((int32_t)c->r[4] >> 24);
  c->mem_w16((c->r[8] + (uint32_t)8), (uint16_t)c->r[3]);
  c->r[3] = c->r[3] + c->r[6];
  c->mem_w16((c->r[8] + (uint32_t)16), (uint16_t)c->r[3]);
  c->r[2] = c->r[2] + c->r[4];
  c->mem_w16((c->r[8] + (uint32_t)26), (uint16_t)c->r[2]);
  c->r[2] = c->r[2] + c->r[5];
  c->mem_w16((c->r[8] + (uint32_t)10), (uint16_t)c->r[2]);
  c->r[4] = c->r[2] + c->r[0];
  c->r[5] = c->r[3] + c->r[0];
  c->r[2] = (uint32_t)c->mem_r8((c->r[8] + (uint32_t)37));
  c->r[3] = (uint32_t)c->mem_r16((c->r[8] + (uint32_t)26));
  c->mem_w16((c->r[8] + (uint32_t)18), (uint16_t)c->r[4]);
  c->mem_w16((c->r[8] + (uint32_t)32), (uint16_t)c->r[5]);
  c->r[2] = c->r[2] + (uint32_t)-1;
  goto L_8007E618;
  return;
L_8007E204:;
  c->r[2] = c->mem_r32((c->r[11] + (uint32_t)0));
  c->mem_w32((c->r[8] + (uint32_t)12), c->r[2]);
  c->r[2] = c->mem_r32((c->r[9] + (uint32_t)-11));
  c->mem_w32((c->r[8] + (uint32_t)20), c->r[2]);
  c->r[2] = (uint32_t)c->mem_r16((c->r[9] + (uint32_t)-7));
  c->mem_w16((c->r[8] + (uint32_t)28), (uint16_t)c->r[2]);
  c->r[3] = (uint32_t)c->mem_r16((c->r[9] + (uint32_t)-3));
  c->r[2] = c->r[0] + (uint32_t)44;
  c->mem_w8((c->r[8] + (uint32_t)7), (uint8_t)c->r[2]);
  {
    int _t = (c->r[15] == c->r[0]);
    c->mem_w16((c->r[8] + (uint32_t)36), (uint16_t)c->r[3]);
    if (_t) {
      goto L_8007E244;
    }
  }
  c->r[2] = c->r[0] + (uint32_t)46;
  c->mem_w8((c->r[8] + (uint32_t)7), (uint8_t)c->r[2]);
L_8007E244:;
  {
    int _t = (c->r[10] == c->r[0]);
    if (_t) {
      goto L_8007E25C;
    }
  }
  c->mem_w8((c->r[8] + (uint32_t)6), (uint8_t)c->r[10]);
  c->mem_w8((c->r[8] + (uint32_t)5), (uint8_t)c->r[10]);
  c->mem_w8((c->r[8] + (uint32_t)4), (uint8_t)c->r[10]);
  goto L_8007E26C;
L_8007E25C:;
  c->r[2] = (uint32_t)c->mem_r8((c->r[8] + (uint32_t)7));
  c->r[2] = c->r[2] | 1u;
  c->mem_w8((c->r[8] + (uint32_t)7), (uint8_t)c->r[2]);
L_8007E26C:;
  {
    int _t = (c->r[24] == c->r[0]);
    if (_t) {
      goto L_8007E280;
    }
  }
  c->r[2] = (uint32_t)c->mem_r16((c->r[13] + (uint32_t)2));
  c->mem_w16((c->r[8] + (uint32_t)14), (uint16_t)c->r[2]);
L_8007E280:;
  c->r[2] = (uint32_t)(int16_t)c->mem_r16((c->r[12] + (uint32_t)4));
  c->r[3] = (uint32_t)c->mem_r16((c->r[12] + (uint32_t)4));
  {
    int _t = ((int32_t)c->r[2] > 0);
    c->r[6] = c->r[3] + c->r[0];
    if (_t) {
      goto L_8007E2A8;
    }
  }
  {
    int _t = ((int32_t)c->r[2] >= 0);
    if (_t) {
      goto L_8007E2A4;
    }
  }
  c->r[2] = (uint32_t)c->mem_r8((c->r[9] + (uint32_t)-5));
  c->r[6] = c->r[2] + c->r[3];
  goto L_8007E2A8;
L_8007E2A4:;
  c->r[6] = (uint32_t)c->mem_r8((c->r[9] + (uint32_t)-5));
L_8007E2A8:;
  c->r[2] = (uint32_t)(int16_t)c->mem_r16((c->r[12] + (uint32_t)6));
  c->r[3] = (uint32_t)c->mem_r16((c->r[12] + (uint32_t)6));
  {
    int _t = ((int32_t)c->r[2] > 0);
    c->r[5] = c->r[3] + c->r[0];
    if (_t) {
      goto L_8007E2D0;
    }
  }
  {
    int _t = ((int32_t)c->r[2] >= 0);
    if (_t) {
      goto L_8007E2CC;
    }
  }
  c->r[2] = (uint32_t)c->mem_r8((c->r[9] + (uint32_t)-4));
  c->r[5] = c->r[2] + c->r[3];
  goto L_8007E2D0;
L_8007E2CC:;
  c->r[5] = (uint32_t)c->mem_r8((c->r[9] + (uint32_t)-4));
L_8007E2D0:;
  c->r[3] = (uint32_t)c->mem_r8((c->r[13] + (uint32_t)0));
  c->r[2] = (uint32_t)(c->r[3] < (uint32_t)6);
  {
    int _t = (c->r[2] == c->r[0]);
    c->r[2] = c->r[3] << 2;
    if (_t) {
      goto L_8007E620;
    }
  }
  c->r[2] = c->r[2] + c->r[25];
  c->r[2] = c->mem_r32((c->r[2] + (uint32_t)0));
  rec_dispatch(c, c->r[2]);
  return;
  return;
L_8007E618:;
  c->mem_w8((c->r[8] + (uint32_t)37), (uint8_t)c->r[2]);
  c->mem_w16((c->r[8] + (uint32_t)34), (uint16_t)c->r[3]);
L_8007E620:;
  c->r[6] = (uint32_t)32780u << 16;
  c->r[4] = c->mem_r32((c->r[6] + (uint32_t)-2748));
  c->r[3] = (uint32_t)32783u << 16;
  c->r[2] = (uint32_t)c->mem_r8((c->r[13] + (uint32_t)1));
  c->r[5] = c->mem_r32((c->r[3] + (uint32_t)-10040));
  c->r[2] = c->r[2] << 2;
  c->r[5] = c->r[5] + c->r[2];
  c->r[2] = c->mem_r32((c->r[5] + (uint32_t)0));
  c->r[3] = (uint32_t)2304u << 16;
  c->r[2] = c->r[2] | c->r[3];
  c->mem_w32((c->r[4] + (uint32_t)0), c->r[2]);
  c->mem_w32((c->r[5] + (uint32_t)0), c->r[4]);
  c->r[4] = c->r[4] + (uint32_t)4;
  c->r[9] = c->r[9] + (uint32_t)16;
  c->r[11] = c->r[11] + (uint32_t)16;
  c->r[2] = c->mem_r32((c->r[8] + (uint32_t)4));
  c->r[14] = c->r[14] + (uint32_t)-1;
  c->mem_w32((c->r[4] + (uint32_t)0), c->r[2]);
  c->r[2] = c->mem_r32((c->r[8] + (uint32_t)8));
  c->r[4] = c->r[4] + (uint32_t)4;
  c->mem_w32((c->r[4] + (uint32_t)0), c->r[2]);
  c->r[2] = c->mem_r32((c->r[8] + (uint32_t)12));
  c->r[4] = c->r[4] + (uint32_t)4;
  c->mem_w32((c->r[4] + (uint32_t)0), c->r[2]);
  c->r[2] = c->mem_r32((c->r[8] + (uint32_t)16));
  c->r[4] = c->r[4] + (uint32_t)4;
  c->mem_w32((c->r[4] + (uint32_t)0), c->r[2]);
  c->r[2] = c->mem_r32((c->r[8] + (uint32_t)20));
  c->r[4] = c->r[4] + (uint32_t)4;
  c->mem_w32((c->r[4] + (uint32_t)0), c->r[2]);
  c->r[2] = c->mem_r32((c->r[8] + (uint32_t)24));
  c->r[4] = c->r[4] + (uint32_t)4;
  c->mem_w32((c->r[4] + (uint32_t)0), c->r[2]);
  c->r[2] = c->mem_r32((c->r[8] + (uint32_t)28));
  c->r[4] = c->r[4] + (uint32_t)4;
  c->mem_w32((c->r[4] + (uint32_t)0), c->r[2]);
  c->r[2] = c->mem_r32((c->r[8] + (uint32_t)32));
  c->r[4] = c->r[4] + (uint32_t)4;
  c->mem_w32((c->r[4] + (uint32_t)0), c->r[2]);
  c->r[2] = c->mem_r32((c->r[8] + (uint32_t)36));
  c->r[4] = c->r[4] + (uint32_t)4;
  c->mem_w32((c->r[4] + (uint32_t)0), c->r[2]);
  c->r[4] = c->r[4] + (uint32_t)4;
  {
    int _t = (c->r[14] != c->r[0]);
    c->mem_w32((c->r[6] + (uint32_t)-2748), c->r[4]);
    if (_t) {
      goto L_8007E204;
    }
  }
  return;
  return;
}

// FUN_0x8007E4A8 — layout mode 3.
//
// Like case 0 this is a CASE BLOCK, not a function: no frame, no sp, and it TAIL-JUMPS to the shared
// OT-link tail. abi_extract prints `r[31] = MISSING` and that is correct, not a defect — this is a
// `j`, so r31 must keep the value it arrived with. (My batch brief said these blocks write the ra
// constant unconditionally; the verifier corrected that — they do not write it at all.)
// ORACLE: gen_func_8007E4A8
void UiFt4Layout::flipXYQuadVerts(Core *c) {
  c->r[2] = c->mem_r32((c->r[12] + (uint32_t)0));
  c->mem_w32((c->r[8] + (uint32_t)32), c->r[2]);
  c->r[2] = (uint32_t)c->mem_r8((c->r[9] + (uint32_t)-1));
  c->r[3] = (uint32_t)c->mem_r16((c->r[8] + (uint32_t)32));
  c->r[2] = c->r[2] << 24;
  c->r[2] = (uint32_t)((int32_t)c->r[2] >> 24);
  c->r[3] = c->r[3] + c->r[2];
  c->r[2] = (uint32_t)c->mem_r8((c->r[8] + (uint32_t)12));
  c->mem_w16((c->r[8] + (uint32_t)32), (uint16_t)c->r[3]);
  c->r[4] = (uint32_t)c->mem_r8((c->r[9] + (uint32_t)0));
  c->r[3] = (uint32_t)c->mem_r8((c->r[8] + (uint32_t)28));
  c->r[2] = c->r[2] + (uint32_t)-1;
  c->r[3] = c->r[3] + (uint32_t)-1;
  c->r[4] = c->r[4] << 24;
  c->mem_w8((c->r[8] + (uint32_t)12), (uint8_t)c->r[2]);
  c->r[2] = (uint32_t)c->mem_r8((c->r[8] + (uint32_t)20));
  c->r[4] = (uint32_t)((int32_t)c->r[4] >> 24);
  c->mem_w8((c->r[8] + (uint32_t)28), (uint8_t)c->r[3]);
  c->r[2] = c->r[2] + (uint32_t)-1;
  c->mem_w8((c->r[8] + (uint32_t)20), (uint8_t)c->r[2]);
  c->r[2] = (uint32_t)c->mem_r8((c->r[8] + (uint32_t)36));
  c->r[3] = (uint32_t)c->mem_r8((c->r[8] + (uint32_t)13));
  c->r[2] = c->r[2] + (uint32_t)-1;
  c->mem_w8((c->r[8] + (uint32_t)36), (uint8_t)c->r[2]);
  c->r[2] = (uint32_t)c->mem_r16((c->r[8] + (uint32_t)34));
  c->r[3] = c->r[3] + (uint32_t)-1;
  c->mem_w8((c->r[8] + (uint32_t)13), (uint8_t)c->r[3]);
  c->r[2] = c->r[2] + c->r[4];
  c->mem_w16((c->r[8] + (uint32_t)34), (uint16_t)c->r[2]);
  c->r[2] = (uint32_t)c->mem_r16((c->r[8] + (uint32_t)32));
  c->r[3] = (uint32_t)c->mem_r16((c->r[8] + (uint32_t)34));
  c->r[2] = c->r[2] + c->r[6];
  c->r[3] = c->r[3] + c->r[5];
  c->mem_w16((c->r[8] + (uint32_t)10), (uint16_t)c->r[3]);
  c->r[7] = c->r[3] + c->r[0];
  c->mem_w16((c->r[8] + (uint32_t)8), (uint16_t)c->r[2]);
  c->r[4] = c->r[2] + c->r[0];
  c->r[6] = (uint32_t)c->mem_r16((c->r[8] + (uint32_t)32));
  c->r[5] = (uint32_t)c->mem_r16((c->r[8] + (uint32_t)34));
  c->r[2] = (uint32_t)c->mem_r8((c->r[8] + (uint32_t)21));
  c->r[3] = (uint32_t)c->mem_r8((c->r[8] + (uint32_t)29));
  c->mem_w16((c->r[8] + (uint32_t)24), (uint16_t)c->r[4]);
  c->mem_w16((c->r[8] + (uint32_t)18), (uint16_t)c->r[7]);
  c->r[2] = c->r[2] + (uint32_t)-1;
  c->r[3] = c->r[3] + (uint32_t)-1;
  c->mem_w8((c->r[8] + (uint32_t)21), (uint8_t)c->r[2]);
  c->mem_w16((c->r[8] + (uint32_t)16), (uint16_t)c->r[6]);
  c->mem_w16((c->r[8] + (uint32_t)26), (uint16_t)c->r[5]);
  c->mem_w8((c->r[8] + (uint32_t)29), (uint8_t)c->r[3]);
  c->r[2] = (uint32_t)c->mem_r8((c->r[8] + (uint32_t)37));
  c->r[2] = c->r[2] + (uint32_t)-1;
  c->mem_w8((c->r[8] + (uint32_t)37), (uint8_t)c->r[2]);
  func_8007E620(c);
  return;
  return;
}

// FUN_0x8007E584 — layout mode 4.
//
// Like case 0 this is a CASE BLOCK, not a function: no frame, no sp, and it TAIL-JUMPS to the shared
// OT-link tail. abi_extract prints `r[31] = MISSING` and that is correct, not a defect — this is a
// `j`, so r31 must keep the value it arrived with. (My batch brief said these blocks write the ra
// constant unconditionally; the verifier corrected that — they do not write it at all.)
// ORACLE: gen_func_8007E584
void UiFt4Layout::vMirroredPlusQuadVerts(Core *c) {
  c->r[2] = c->mem_r32((c->r[12] + (uint32_t)0));
  c->mem_w32((c->r[8] + (uint32_t)24), c->r[2]);
  c->r[2] = (uint32_t)c->mem_r8((c->r[9] + (uint32_t)-1));
  c->r[3] = (uint32_t)c->mem_r16((c->r[8] + (uint32_t)24));
  c->r[2] = c->r[2] << 24;
  c->r[2] = (uint32_t)((int32_t)c->r[2] >> 24);
  c->r[3] = c->r[3] + c->r[2];
  c->r[2] = (uint32_t)c->mem_r8((c->r[8] + (uint32_t)13));
  c->mem_w16((c->r[8] + (uint32_t)24), (uint16_t)c->r[3]);
  c->r[4] = (uint32_t)c->mem_r8((c->r[9] + (uint32_t)0));
  c->r[3] = (uint32_t)c->mem_r8((c->r[8] + (uint32_t)29));
  c->r[2] = c->r[2] + (uint32_t)1;
  c->r[3] = c->r[3] + (uint32_t)1;
  c->mem_w8((c->r[8] + (uint32_t)13), (uint8_t)c->r[2]);
  c->r[2] = (uint32_t)c->mem_r8((c->r[8] + (uint32_t)21));
  c->r[4] = c->r[4] << 24;
  c->mem_w8((c->r[8] + (uint32_t)29), (uint8_t)c->r[3]);
  c->r[3] = (uint32_t)c->mem_r16((c->r[8] + (uint32_t)24));
  c->r[2] = c->r[2] + (uint32_t)1;
  c->mem_w8((c->r[8] + (uint32_t)21), (uint8_t)c->r[2]);
  c->r[2] = (uint32_t)c->mem_r16((c->r[8] + (uint32_t)26));
  c->r[4] = (uint32_t)((int32_t)c->r[4] >> 24);
  c->mem_w16((c->r[8] + (uint32_t)8), (uint16_t)c->r[3]);
  c->r[3] = c->r[3] + c->r[6];
  c->mem_w16((c->r[8] + (uint32_t)16), (uint16_t)c->r[3]);
  c->r[2] = c->r[2] + c->r[4];
  c->mem_w16((c->r[8] + (uint32_t)26), (uint16_t)c->r[2]);
  c->r[2] = c->r[2] + c->r[5];
  c->mem_w16((c->r[8] + (uint32_t)10), (uint16_t)c->r[2]);
  c->r[4] = c->r[2] + c->r[0];
  c->r[5] = c->r[3] + c->r[0];
  c->r[2] = (uint32_t)c->mem_r8((c->r[8] + (uint32_t)37));
  c->r[3] = (uint32_t)c->mem_r16((c->r[8] + (uint32_t)26));
  c->mem_w16((c->r[8] + (uint32_t)18), (uint16_t)c->r[4]);
  c->mem_w16((c->r[8] + (uint32_t)32), (uint16_t)c->r[5]);
  c->r[2] = c->r[2] + (uint32_t)1;
  c->mem_w8((c->r[8] + (uint32_t)37), (uint8_t)c->r[2]);
  c->mem_w16((c->r[8] + (uint32_t)34), (uint16_t)c->r[3]);
  func_8007E620(c);
  return;
}

void UiFt4Layout::registerOverrides(Game *) {
  engine_set_override_main(0x8007E36Cu, &UiFt4Layout::xMirroredQuadVerts, gen_func_8007E36C);
  engine_set_override_main(0x8007E410u, &UiFt4Layout::vMirroredQuadVerts, gen_func_8007E410);
  engine_set_override_main(0x8007E4A8u, &UiFt4Layout::flipXYQuadVerts, gen_func_8007E4A8);
  engine_set_override_main(0x8007E584u, &UiFt4Layout::vMirroredPlusQuadVerts, gen_func_8007E584);
  engine_set_override_main(0x8007E2F8u, &UiFt4Layout::plainQuadVerts, gen_func_8007E2F8);
}
