// dialog_driver.cpp — PORT_GEN draft, byte-faithful transcription of gen_func_8007DC38.
// ORACLE: gen_func_8007DC38 (../../generated/shard_6.c:13634-13726)
// PORT_GEN: 0x8007DC38 ../../generated/shard_6.c:13634-13726
//
// This body is the gen function's guest-visible operations VERBATIM — every c->r[] op,
// mem_r/mem_w call, func_X/rec_dispatch call with its r31 constant, and label/goto is
// preserved unchanged. Faithful by construction; the only allowed next step is RENAMING
// (locals/labels -> named fields/control-flow), verified equivalent by tools/port_check.py.
// UNWIRED — dead code. Do not wire into any dispatch table before running port_check.py
// and the mandatory line-by-line verify pass (docs/fleet-workflow.md §9).
#include "ui/dialog_driver.h"
#include "core.h"

void rec_dispatch(Core*, uint32_t);  // overlay_router.cpp — shared dispatch choke point
void func_8007C0D0(Core*);  // generated/shard_disp.c
void func_8007C940(Core*);  // generated/shard_disp.c
void func_8007CC00(Core*);  // generated/shard_disp.c
void func_8005019C(Core*);  // generated/shard_disp.c
void func_8007A624(Core*);  // generated/shard_disp.c

void DialogDriver::stepFaithful(Core* c) {
    c->r[29] = c->r[29] + (uint32_t)-32;
    c->mem_w32((c->r[29] + (uint32_t)16), c->r[16]);
    c->r[16] = c->r[4] + c->r[0];
    c->mem_w32((c->r[29] + (uint32_t)24), c->r[31]);
    c->mem_w32((c->r[29] + (uint32_t)20), c->r[17]);
    c->r[3] = (uint32_t)c->mem_r8((c->r[16] + (uint32_t)4));
    c->r[17] = c->r[0] + (uint32_t)1;
    { int _t = (c->r[3] == c->r[17]); c->r[2] = (uint32_t)((int32_t)c->r[3] < 2); if (_t) goto L_8007DD38; }
    { int _t = (c->r[2] == c->r[0]);  if (_t) goto L_8007DC74; }
    { int _t = (c->r[3] == c->r[0]); c->r[4] = c->r[16] + c->r[0]; if (_t) goto L_8007DC90; }
     goto L_8007DDCC;
  L_8007DC74:;
    c->r[2] = c->r[0] + (uint32_t)2;
    { int _t = (c->r[3] == c->r[2]); c->r[2] = c->r[0] + (uint32_t)3; if (_t) goto L_8007DD98; }
    { int _t = (c->r[3] == c->r[2]);  if (_t) goto L_8007DDA0; }
     goto L_8007DDCC;
  L_8007DC90:;
    c->r[2] = (uint32_t)(int16_t)c->mem_r16((c->r[16] + (uint32_t)94));
    c->r[3] = c->mem_r32((c->r[16] + (uint32_t)76));
    c->r[2] = c->r[2] << 2;
    c->r[2] = c->r[2] + c->r[3];
    c->r[3] = (uint32_t)c->mem_r16((c->r[2] + (uint32_t)0));
    c->r[2] = c->mem_r32((c->r[16] + (uint32_t)80));
    c->r[5] = c->r[0] + c->r[0];
    c->r[2] = c->r[2] + c->r[3];
    c->mem_w32((c->r[16] + (uint32_t)16), c->r[2]);
    c->r[31] = 0x8007DCBCu;
    c->mem_w32((c->r[16] + (uint32_t)20), c->r[2]); func_8007C0D0(c);
    c->r[2] = (uint32_t)c->mem_r8((c->r[16] + (uint32_t)4));
    c->r[3] = (uint32_t)c->mem_r8((c->r[16] + (uint32_t)3));
    c->mem_w8((c->r[16] + (uint32_t)70), (uint8_t)c->r[17]);
    c->r[2] = c->r[2] + (uint32_t)1;
    c->mem_w8((c->r[16] + (uint32_t)4), (uint8_t)c->r[2]);
    c->r[2] = c->r[0] + (uint32_t)2;
    { int _t = (c->r[3] == c->r[2]); c->r[2] = (uint32_t)((int32_t)c->r[3] < 3); if (_t) goto L_8007DD24; }
    { int _t = (c->r[2] == c->r[0]); c->r[2] = c->r[0] + (uint32_t)3; if (_t) goto L_8007DCF4; }
    { int _t = (c->r[3] == c->r[0]); c->r[2] = c->r[0] + (uint32_t)4; if (_t) goto L_8007DD04; }
     goto L_8007DDCC;
  L_8007DCF4:;
    { int _t = (c->r[3] == c->r[2]);  if (_t) goto L_8007DD30; }
     goto L_8007DDCC;
  L_8007DD04:;
    c->r[3] = (uint32_t)32780u << 16;
    c->r[3] = c->r[3] + (uint32_t)-2040;
    c->mem_w8((c->r[16] + (uint32_t)24), (uint8_t)c->r[2]);
    c->r[2] = (uint32_t)c->mem_r8((c->r[3] + (uint32_t)26));
    c->r[2] = c->r[2] | 4u;
    c->mem_w8((c->r[3] + (uint32_t)26), (uint8_t)c->r[2]); goto L_8007DDCC;
  L_8007DD24:;
    c->r[2] = c->r[0] + (uint32_t)5;
    c->mem_w8((c->r[16] + (uint32_t)24), (uint8_t)c->r[2]); goto L_8007DDCC;
  L_8007DD30:;
    c->mem_w8((c->r[16] + (uint32_t)24), (uint8_t)c->r[17]); goto L_8007DDCC;
  L_8007DD38:;
    c->r[2] = (uint32_t)c->mem_r8((c->r[16] + (uint32_t)3));
    { int _t = (c->r[2] != c->r[0]); c->r[2] = (uint32_t)32780u << 16; if (_t) goto L_8007DD60; }
    c->r[2] = (uint32_t)c->mem_r8((c->r[2] + (uint32_t)-2014));
    c->r[2] = c->r[2] & 251u;
    { int _t = (c->r[2] == c->r[0]); c->r[2] = c->r[0] + (uint32_t)2; if (_t) goto L_8007DD60; }
    c->mem_w8((c->r[16] + (uint32_t)4), (uint8_t)c->r[2]);
  L_8007DD60:;
    c->r[31] = 0x8007DD68u;
    c->r[4] = c->r[16] + c->r[0]; func_8007C940(c);
    c->r[31] = 0x8007DD70u;
    c->r[4] = c->r[16] + c->r[0]; func_8007CC00(c);
    c->r[3] = (uint32_t)c->mem_r8((c->r[16] + (uint32_t)3));
    c->r[2] = c->r[0] + (uint32_t)1;
    { int _t = (c->r[3] == c->r[2]); c->r[6] = c->r[2] + c->r[0]; if (_t) goto L_8007DDCC; }
    c->r[4] = c->r[16] + (uint32_t)84;
    c->r[5] = (uint32_t)c->mem_r8((c->r[16] + (uint32_t)24));
    c->r[31] = 0x8007DD90u;
    c->r[7] = c->r[0] + (uint32_t)2; func_8005019C(c);
     goto L_8007DDCC;
  L_8007DD98:;
    c->mem_w8((c->r[16] + (uint32_t)4), (uint8_t)c->r[2]); goto L_8007DDCC;
  L_8007DDA0:;
    c->r[2] = (uint32_t)c->mem_r8((c->r[16] + (uint32_t)3));
    { int _t = (c->r[2] != c->r[0]); c->r[3] = (uint32_t)32780u << 16; if (_t) goto L_8007DDC4; }
    c->r[3] = c->r[3] + (uint32_t)-2040;
    c->r[2] = (uint32_t)c->mem_r8((c->r[3] + (uint32_t)26));
    c->r[2] = c->r[2] & 251u;
    c->mem_w8((c->r[3] + (uint32_t)26), (uint8_t)c->r[2]);
  L_8007DDC4:;
    c->r[31] = 0x8007DDCCu;
    c->r[4] = c->r[16] + c->r[0]; func_8007A624(c);
  L_8007DDCC:;
    c->r[31] = c->mem_r32((c->r[29] + (uint32_t)24));
    c->r[17] = c->mem_r32((c->r[29] + (uint32_t)20));
    c->r[16] = c->mem_r32((c->r[29] + (uint32_t)16));
    c->r[29] = c->r[29] + (uint32_t)32; return;
    return;
}

