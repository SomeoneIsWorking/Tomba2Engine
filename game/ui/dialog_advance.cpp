// dialog_text_stream.cpp — PORT_GEN draft, byte-faithful transcription of gen_func_8007C0D0.
// ORACLE: gen_func_8007C0D0 (../../generated/shard_6.c:13386-13476)
// PORT_GEN: 0x8007C0D0 ../../generated/shard_6.c:13386-13476
//
// This body is the gen function's guest-visible operations VERBATIM — every c->r[] op,
// mem_r/mem_w call, func_X/rec_dispatch call with its r31 constant, and label/goto is
// preserved unchanged. Faithful by construction; the only allowed next step is RENAMING
// (locals/labels -> named fields/control-flow), verified equivalent by tools/port_check.py.
// UNWIRED — dead code. Do not wire into any dispatch table before running port_check.py
// and the mandatory line-by-line verify pass (docs/fleet-workflow.md §9).
#include "ui/dialog_text_stream.h"
#include "core.h"

void rec_dispatch(Core*, uint32_t);  // overlay_router.cpp — shared dispatch choke point
void func_8007D0D0(Core*);  // generated/shard_disp.c

void DialogTextStream::advanceByteGen(Core* c) {
    c->r[29] = c->r[29] + (uint32_t)-32;
    c->mem_w32((c->r[29] + (uint32_t)16), c->r[16]);
    c->r[16] = c->r[4] + c->r[0];
    c->mem_w32((c->r[29] + (uint32_t)24), c->r[31]);
    c->mem_w32((c->r[29] + (uint32_t)20), c->r[17]);
    c->r[4] = c->mem_r32((c->r[16] + (uint32_t)20));
    c->r[3] = (uint32_t)c->mem_r8((c->r[4] + (uint32_t)0));
    c->r[2] = c->r[0] + (uint32_t)255;
    { int _t = (c->r[3] == c->r[2]); c->r[17] = c->r[5] + c->r[0]; if (_t) goto L_8007C27C; }
    c->r[5] = c->r[0] + (uint32_t)1;
    c->r[2] = (uint32_t)32769u << 16;
    c->r[6] = c->r[2] + (uint32_t)28348;
  L_8007C108:;
    c->r[2] = (uint32_t)c->mem_r8((c->r[4] + (uint32_t)0));
    c->r[2] = (uint32_t)(c->r[2] < (uint32_t)192);
    { int _t = (c->r[2] == c->r[0]);  if (_t) goto L_8007C140; }
    { int _t = (c->r[17] != c->r[5]);  if (_t) goto L_8007C24C; }
    c->r[31] = 0x8007C12Cu;
    c->r[4] = c->r[16] + c->r[0]; func_8007D0D0(c);
    c->r[3] = c->mem_r32((c->r[16] + (uint32_t)20));
    c->r[4] = (uint32_t)c->mem_r8((c->r[16] + (uint32_t)42));
    c->r[2] = c->r[0] + (uint32_t)1;
    c->mem_w8((c->r[16] + (uint32_t)5), (uint8_t)c->r[17]); goto L_8007C214;
  L_8007C140:;
    c->r[3] = (uint32_t)c->mem_r8((c->r[4] + (uint32_t)0));
    c->r[2] = (uint32_t)((int32_t)c->r[3] < 250);
    { int _t = (c->r[2] != c->r[0]); c->r[2] = (uint32_t)((int32_t)c->r[3] < 248); if (_t) goto L_8007C168; }
    c->r[2] = c->r[0] + (uint32_t)252;
    { int _t = (c->r[3] == c->r[2]);  if (_t) goto L_8007C228; }
     goto L_8007C24C;
  L_8007C168:;
    { int _t = (c->r[2] == c->r[0]); c->r[2] = (uint32_t)((int32_t)c->r[3] < 209); if (_t) goto L_8007C1A4; }
    { int _t = (c->r[2] == c->r[0]); c->r[2] = (uint32_t)((int32_t)c->r[3] < 192); if (_t) goto L_8007C24C; }
    { int _t = (c->r[2] != c->r[0]);  if (_t) goto L_8007C24C; }
    { int _t = (c->r[17] != c->r[5]);  if (_t) goto L_8007C24C; }
    c->r[31] = 0x8007C190u;
    c->r[4] = c->r[16] + c->r[0]; func_8007D0D0(c);
    c->r[3] = c->mem_r32((c->r[16] + (uint32_t)20));
    c->r[4] = (uint32_t)c->mem_r8((c->r[16] + (uint32_t)42));
    c->r[2] = c->r[0] + (uint32_t)1;
    c->mem_w8((c->r[16] + (uint32_t)5), (uint8_t)c->r[17]); goto L_8007C214;
  L_8007C1A4:;
    c->r[3] = (uint32_t)c->mem_r8((c->r[16] + (uint32_t)3));
    c->r[2] = (uint32_t)(c->r[3] < (uint32_t)6);
    { int _t = (c->r[2] == c->r[0]); c->r[2] = c->r[3] << 2; if (_t) goto L_8007C208; }
    c->r[2] = c->r[2] + c->r[6];
    c->r[2] = c->mem_r32((c->r[2] + (uint32_t)0));
     rec_dispatch(c, c->r[2]); return;
    return;
  L_8007C208:;
    c->r[2] = c->r[0] + (uint32_t)1;
    c->r[3] = c->mem_r32((c->r[16] + (uint32_t)20));
    c->r[4] = (uint32_t)c->mem_r8((c->r[16] + (uint32_t)42));
  L_8007C214:;
    c->r[3] = c->r[3] + c->r[2];
    c->r[4] = c->r[4] + c->r[2];
    c->mem_w32((c->r[16] + (uint32_t)20), c->r[3]);
    c->mem_w8((c->r[16] + (uint32_t)42), (uint8_t)c->r[4]); goto L_8007C290;
  L_8007C228:;
    { int _t = (c->r[17] != c->r[5]); c->r[2] = c->r[0] + (uint32_t)1; if (_t) goto L_8007C24C; }
    c->r[4] = c->mem_r32((c->r[16] + (uint32_t)20));
    c->r[3] = c->r[0] + (uint32_t)15;
    c->mem_w16((c->r[16] + (uint32_t)64), (uint16_t)c->r[3]);
    c->mem_w8((c->r[16] + (uint32_t)5), (uint8_t)c->r[17]);
    c->r[4] = c->r[4] + c->r[2];
    c->mem_w32((c->r[16] + (uint32_t)20), c->r[4]); goto L_8007C290;
  L_8007C24C:;
    c->r[2] = c->mem_r32((c->r[16] + (uint32_t)20));
    c->r[2] = c->r[2] + (uint32_t)1;
    c->mem_w32((c->r[16] + (uint32_t)20), c->r[2]);
    c->r[2] = (uint32_t)c->mem_r8((c->r[16] + (uint32_t)42));
    c->r[4] = c->mem_r32((c->r[16] + (uint32_t)20));
    c->r[2] = c->r[2] + (uint32_t)1;
    c->mem_w8((c->r[16] + (uint32_t)42), (uint8_t)c->r[2]);
    c->r[3] = (uint32_t)c->mem_r8((c->r[4] + (uint32_t)0));
    c->r[2] = c->r[0] + (uint32_t)255;
    { int _t = (c->r[3] != c->r[2]);  if (_t) goto L_8007C108; }
  L_8007C27C:;
    c->r[2] = c->r[0] + c->r[0];
    c->r[3] = c->r[0] + (uint32_t)2;
    c->mem_w8((c->r[16] + (uint32_t)5), (uint8_t)c->r[3]);
    c->r[3] = c->r[0] | 65535u;
    c->mem_w16((c->r[16] + (uint32_t)66), (uint16_t)c->r[3]);
  L_8007C290:;
    c->r[31] = c->mem_r32((c->r[29] + (uint32_t)24));
    c->r[17] = c->mem_r32((c->r[29] + (uint32_t)20));
    c->r[16] = c->mem_r32((c->r[29] + (uint32_t)16));
    c->r[29] = c->r[29] + (uint32_t)32; return;
    return;
}

