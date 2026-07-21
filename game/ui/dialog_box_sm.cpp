// dialog_box_sm.cpp — PORT_GEN draft, byte-faithful transcription of gen_func_8007D594.
// ORACLE: gen_func_8007D594 (../../generated/shard_4.c:12027-12295)
// PORT_GEN: 0x8007D594 ../../generated/shard_4.c:12027-12295
//
// This body is the gen function's guest-visible operations VERBATIM — every c->r[] op,
// mem_r/mem_w call, func_X/rec_dispatch call with its r31 constant, and label/goto is
// preserved unchanged. Faithful by construction; the only allowed next step is RENAMING
// (locals/labels -> named fields/control-flow), verified equivalent by tools/port_check.py.
// UNWIRED — dead code. Do not wire into any dispatch table before running port_check.py
// and the mandatory line-by-line verify pass (docs/fleet-workflow.md §9).
#include "ui/dialog_box_sm.h"
#include "core.h"
#include "override_registry.h"

void rec_dispatch(Core*, uint32_t);  // overlay_router.cpp — shared dispatch choke point
void func_8007C0D0(Core*);  // generated/shard_disp.c
void func_8007C940(Core*);  // generated/shard_disp.c
void func_8001CF78(Core*);  // generated/shard_disp.c
void func_8007CDD4(Core*);  // generated/shard_disp.c
void func_8007D14C(Core*);  // generated/shard_disp.c
void func_8007CC00(Core*);  // generated/shard_disp.c
void func_8005019C(Core*);  // generated/shard_disp.c

void DialogBoxSm::step(Core* c) {
    c->r[29] = c->r[29] + (uint32_t)-32;
    c->mem_w32((c->r[29] + (uint32_t)24), c->r[16]);
    c->r[16] = c->r[4] + c->r[0];
    c->r[2] = (uint32_t)32769u << 16;
    c->mem_w32((c->r[29] + (uint32_t)28), c->r[31]);
    c->r[11] = c->r[2] + (uint32_t)28916;
    c->r[8] = c->mem_lwl(c->r[8], (c->r[11] + (uint32_t)3));
    c->r[8] = c->mem_lwr(c->r[8], (c->r[11] + (uint32_t)0));
    c->r[9] = (uint32_t)(int8_t)c->mem_r8((c->r[11] + (uint32_t)4));
    c->r[10] = (uint32_t)(int8_t)c->mem_r8((c->r[11] + (uint32_t)5));
    c->mem_swl((c->r[29] + (uint32_t)19), c->r[8]);
    c->mem_swr((c->r[29] + (uint32_t)16), c->r[8]);
    c->mem_w8((c->r[29] + (uint32_t)20), (uint8_t)c->r[9]);
    c->mem_w8((c->r[29] + (uint32_t)21), (uint8_t)c->r[10]);
    c->r[3] = (uint32_t)c->mem_r8((c->r[16] + (uint32_t)5));
    c->r[2] = (uint32_t)(c->r[3] < (uint32_t)93);
    { int _t = (c->r[2] == c->r[0]); c->r[2] = (uint32_t)32769u << 16; if (_t) goto L_8007DA1C; }
    c->r[2] = c->r[2] + (uint32_t)28924;
    c->r[3] = c->r[3] << 2;
    c->r[3] = c->r[3] + c->r[2];
    c->r[2] = c->mem_r32((c->r[3] + (uint32_t)0));
    {  switch (c->r[2]) { case 0x8007D5FCu: goto L_8007D5FC; case 0x8007D644u: goto L_8007D644; case 0x8007D6D8u: goto L_8007D6D8; case 0x8007D734u: goto L_8007D734; case 0x8007D774u: goto L_8007D774; case 0x8007D7ACu: goto L_8007D7AC; case 0x8007D810u: goto L_8007D810; case 0x8007D858u: goto L_8007D858; case 0x8007DA1Cu: goto L_8007DA1C; case 0x8007D874u: goto L_8007D874; case 0x8007D884u: goto L_8007D884; case 0x8007D964u: goto L_8007D964; case 0x8007D9F0u: goto L_8007D9F0; default: rec_dispatch(c, c->r[2]); return; } }
  L_8007D5FC:;
    c->r[3] = (uint32_t)c->mem_r8((c->r[16] + (uint32_t)3));
    c->r[2] = (uint32_t)(c->r[3] < (uint32_t)6);
    { int _t = (c->r[2] == c->r[0]); c->r[2] = (uint32_t)32769u << 16; if (_t) goto L_8007DA40; }
    c->r[2] = c->r[2] + (uint32_t)29300;
    c->r[3] = c->r[3] << 2;
    c->r[3] = c->r[3] + c->r[2];
    c->r[2] = c->mem_r32((c->r[3] + (uint32_t)0));
    {  switch (c->r[2]) { case 0x8007D62Cu: goto L_8007D62C; case 0x8007D638u: goto L_8007D638; default: rec_dispatch(c, c->r[2]); return; } }
  L_8007D62C:;
    c->r[2] = c->r[0] + (uint32_t)2;
    c->mem_w8((c->r[16] + (uint32_t)4), (uint8_t)c->r[2]); goto L_8007DA40;
  L_8007D638:;
    c->r[2] = c->r[0] + (uint32_t)3;
    c->mem_w8((c->r[16] + (uint32_t)4), (uint8_t)c->r[2]); goto L_8007DA40;
  L_8007D644:;
    c->r[3] = (uint32_t)c->mem_r8((c->r[16] + (uint32_t)3));
    { int _t = ((int32_t)c->r[3] < 0); c->r[2] = (uint32_t)((int32_t)c->r[3] < 2); if (_t) goto L_8007D79C; }
    { int _t = (c->r[2] != c->r[0]); c->r[2] = (uint32_t)32782u << 16; if (_t) goto L_8007D670; }
    c->r[2] = (uint32_t)((int32_t)c->r[3] < 6);
    { int _t = (c->r[2] == c->r[0]);  if (_t) goto L_8007D79C; }
     goto L_8007D6A8;
  L_8007D670:;
    c->r[3] = (uint32_t)8064u << 16;
    c->r[2] = (uint32_t)c->mem_r16((c->r[2] + (uint32_t)32360));
    c->r[3] = (uint32_t)c->mem_r16((c->r[3] + (uint32_t)372));
    c->r[2] = c->r[2] & c->r[3];
    { int _t = (c->r[2] == c->r[0]);  if (_t) goto L_8007D6A8; }
    c->r[2] = (uint32_t)c->mem_r8((c->r[16] + (uint32_t)70));
    c->r[2] = c->r[2] & 1u;
    { int _t = (c->r[2] != c->r[0]); c->r[4] = c->r[16] + c->r[0]; if (_t) goto L_8007D6A8; }
    c->r[5] = c->r[0] + c->r[0]; goto L_8007D6C8;
  L_8007D6A8:;
    c->r[2] = (uint32_t)c->mem_r16((c->r[16] + (uint32_t)64));
    c->r[2] = c->r[2] + (uint32_t)-1;
    c->mem_w16((c->r[16] + (uint32_t)64), (uint16_t)c->r[2]);
    c->r[2] = c->r[2] << 16;
    { int _t = ((int32_t)c->r[2] > 0); c->r[4] = c->r[16] + c->r[0]; if (_t) goto L_8007D79C; }
    c->r[5] = c->r[0] + (uint32_t)1;
  L_8007D6C8:;
    c->r[31] = 0x8007D6D0u;
     func_8007C0D0(c);
     goto L_8007D79C;
  L_8007D6D8:;
    c->r[31] = 0x8007D6E0u;
    c->r[4] = c->r[16] + c->r[0]; func_8007C940(c);
    c->r[2] = (uint32_t)32782u << 16;
    c->r[3] = (uint32_t)8064u << 16;
    c->r[2] = (uint32_t)c->mem_r16((c->r[2] + (uint32_t)32360));
    c->r[3] = (uint32_t)c->mem_r16((c->r[3] + (uint32_t)372));
    c->r[2] = c->r[2] & c->r[3];
    { int _t = (c->r[2] == c->r[0]);  if (_t) goto L_8007D73C; }
    c->r[2] = (uint32_t)c->mem_r8((c->r[16] + (uint32_t)70));
    c->r[2] = c->r[2] & 1u;
    { int _t = (c->r[2] != c->r[0]); c->r[2] = c->r[0] + (uint32_t)255; if (_t) goto L_8007D73C; }
    c->r[3] = (uint32_t)c->mem_r8((c->r[16] + (uint32_t)13));
    { int _t = (c->r[3] == c->r[2]);  if (_t) goto L_8007D874; }
    c->r[31] = 0x8007D72Cu;
     func_8001CF78(c);
     goto L_8007D874;
  L_8007D734:;
    c->r[31] = 0x8007D73Cu;
    c->r[4] = c->r[16] + c->r[0]; func_8007C940(c);
  L_8007D73C:;
    c->r[3] = (uint32_t)c->mem_r16((c->r[16] + (uint32_t)66));
    c->r[2] = c->r[0] | 65535u;
    { int _t = (c->r[3] == c->r[2]); c->r[3] = c->r[3] + (uint32_t)1; if (_t) goto L_8007DA1C; }
    c->r[2] = c->r[3] & 65535u;
    c->r[2] = (uint32_t)(c->r[2] < (uint32_t)9);
    { int _t = (c->r[2] != c->r[0]); c->mem_w16((c->r[16] + (uint32_t)66), (uint16_t)c->r[3]); if (_t) goto L_8007DA1C; }
    c->r[2] = c->r[3] & 15u;
    c->mem_w16((c->r[16] + (uint32_t)66), (uint16_t)c->r[2]);
    c->r[31] = 0x8007D76Cu;
    c->r[4] = c->r[16] + (uint32_t)84; func_8007CDD4(c);
     goto L_8007DA1C;
  L_8007D774:;
    c->r[2] = (uint32_t)c->mem_r16((c->r[16] + (uint32_t)66));
    c->r[2] = c->r[2] + (uint32_t)-1;
    c->mem_w16((c->r[16] + (uint32_t)66), (uint16_t)c->r[2]);
    c->r[2] = c->r[2] & 65535u;
    { int _t = (c->r[2] != c->r[0]); c->r[2] = c->r[0] | 65535u; if (_t) goto L_8007D79C; }
    c->mem_w16((c->r[16] + (uint32_t)66), (uint16_t)c->r[2]);
    c->r[2] = c->r[0] + (uint32_t)3;
    c->mem_w8((c->r[16] + (uint32_t)5), (uint8_t)c->r[2]);
  L_8007D79C:;
    c->r[31] = 0x8007D7A4u;
    c->r[4] = c->r[16] + c->r[0]; func_8007C940(c);
     goto L_8007DA1C;
  L_8007D7AC:;
    c->r[31] = 0x8007D7B4u;
    c->r[4] = c->r[16] + c->r[0]; func_8007C940(c);
    c->r[2] = (uint32_t)c->mem_r16((c->r[16] + (uint32_t)66));
    { int _t = (c->r[2] == c->r[0]); c->r[2] = c->r[2] + (uint32_t)-1; if (_t) goto L_8007D874; }
    c->r[3] = (uint32_t)c->mem_r8((c->r[16] + (uint32_t)13));
    c->mem_w16((c->r[16] + (uint32_t)66), (uint16_t)c->r[2]);
    c->r[2] = c->r[0] + (uint32_t)255;
    { int _t = (c->r[3] == c->r[2]); c->r[3] = (uint32_t)32780u << 16; if (_t) goto L_8007DA1C; }
    c->r[2] = (uint32_t)c->mem_r8((c->r[3] + (uint32_t)-7964));
    c->r[2] = c->r[2] & 128u;
    { int _t = (c->r[2] != c->r[0]);  if (_t) goto L_8007DA1C; }
    c->r[2] = (uint32_t)c->mem_r8((c->r[3] + (uint32_t)-7964));
    c->r[2] = c->r[2] & 2u;
    { int _t = (c->r[2] == c->r[0]); c->r[3] = c->r[0] + (uint32_t)300; if (_t) goto L_8007DA1C; }
    c->r[2] = (uint32_t)c->mem_r8((c->r[16] + (uint32_t)5));
    c->mem_w16((c->r[16] + (uint32_t)66), (uint16_t)c->r[3]);
    c->r[2] = c->r[2] + (uint32_t)1; goto L_8007DA18;
  L_8007D810:;
    c->r[2] = (uint32_t)c->mem_r16((c->r[16] + (uint32_t)66));
    c->r[2] = c->r[2] + (uint32_t)-1;
    c->mem_w16((c->r[16] + (uint32_t)66), (uint16_t)c->r[2]);
    c->r[2] = c->r[2] & 65535u;
    { int _t = (c->r[2] == c->r[0]);  if (_t) goto L_8007D844; }
    c->r[2] = (uint32_t)32780u << 16;
    c->r[2] = (uint32_t)c->mem_r8((c->r[2] + (uint32_t)-7964));
    c->r[2] = c->r[2] & 2u;
    { int _t = (c->r[2] != c->r[0]);  if (_t) goto L_8007DA1C; }
  L_8007D844:;
    c->r[2] = (uint32_t)c->mem_r8((c->r[16] + (uint32_t)5));
    c->r[3] = c->r[0] + (uint32_t)15;
    c->mem_w16((c->r[16] + (uint32_t)66), (uint16_t)c->r[3]);
    c->r[2] = c->r[2] + (uint32_t)1; goto L_8007DA18;
  L_8007D858:;
    c->r[2] = (uint32_t)c->mem_r16((c->r[16] + (uint32_t)66));
    c->r[2] = c->r[2] + (uint32_t)-1;
    c->mem_w16((c->r[16] + (uint32_t)66), (uint16_t)c->r[2]);
    c->r[2] = c->r[2] & 65535u;
    { int _t = (c->r[2] != c->r[0]);  if (_t) goto L_8007DA1C; }
  L_8007D874:;
    c->r[31] = 0x8007D87Cu;
    c->r[4] = c->r[16] + c->r[0]; func_8007D14C(c);
     goto L_8007DA1C;
  L_8007D884:;
    c->r[3] = (uint32_t)(int16_t)c->mem_r16((c->r[16] + (uint32_t)130));
    c->r[2] = (uint32_t)8064u << 16;
    c->mem_w16((c->r[2] + (uint32_t)382), (uint16_t)c->r[0]);
    c->r[2] = (uint32_t)((int32_t)c->r[3] < 128);
    { int _t = (c->r[2] != c->r[0]);  if (_t) goto L_8007D900; }
    c->r[2] = (uint32_t)(int16_t)c->mem_r16((c->r[16] + (uint32_t)86));
    c->r[4] = (uint32_t)c->mem_r16((c->r[16] + (uint32_t)86));
    c->r[2] = c->r[2] - c->r[3];
    { int _t = ((int32_t)c->r[2] >= 0);  if (_t) goto L_8007D8B4; }
    c->r[2] = c->r[2] + (uint32_t)3;
  L_8007D8B4:;
    c->r[2] = (uint32_t)((int32_t)c->r[2] >> 2);
    c->r[3] = c->r[2] + c->r[0];
    c->r[2] = (uint32_t)((int32_t)c->r[2] < 4);
    { int _t = (c->r[2] == c->r[0]); c->r[2] = c->r[4] - c->r[3]; if (_t) goto L_8007D8D0; }
    c->r[3] = c->r[0] + (uint32_t)2;
    c->r[2] = c->r[4] - c->r[3];
  L_8007D8D0:;
    c->mem_w16((c->r[16] + (uint32_t)86), (uint16_t)c->r[2]);
    c->r[2] = c->r[2] << 16;
    c->r[2] = (uint32_t)((int32_t)c->r[2] >> 16);
    c->r[3] = (uint32_t)(int16_t)c->mem_r16((c->r[16] + (uint32_t)130));
    c->r[4] = (uint32_t)c->mem_r16((c->r[16] + (uint32_t)130));
    c->r[3] = (uint32_t)((int32_t)c->r[3] < (int32_t)c->r[2]);
    { int _t = (c->r[3] != c->r[0]);  if (_t) goto L_8007DA1C; }
    c->r[2] = (uint32_t)c->mem_r8((c->r[16] + (uint32_t)5));
    c->mem_w16((c->r[16] + (uint32_t)86), (uint16_t)c->r[4]);
    c->r[2] = c->r[2] + (uint32_t)1; goto L_8007DA18;
  L_8007D900:;
    c->r[2] = (uint32_t)(int16_t)c->mem_r16((c->r[16] + (uint32_t)86));
    c->r[4] = (uint32_t)c->mem_r16((c->r[16] + (uint32_t)86));
    c->r[2] = c->r[3] - c->r[2];
    { int _t = ((int32_t)c->r[2] >= 0);  if (_t) goto L_8007D918; }
    c->r[2] = c->r[2] + (uint32_t)3;
  L_8007D918:;
    c->r[2] = (uint32_t)((int32_t)c->r[2] >> 2);
    c->r[3] = c->r[2] + c->r[0];
    c->r[2] = (uint32_t)((int32_t)c->r[2] < 4);
    { int _t = (c->r[2] == c->r[0]); c->r[2] = c->r[4] + c->r[3]; if (_t) goto L_8007D934; }
    c->r[3] = c->r[0] + (uint32_t)2;
    c->r[2] = c->r[4] + c->r[3];
  L_8007D934:;
    c->mem_w16((c->r[16] + (uint32_t)86), (uint16_t)c->r[2]);
    c->r[2] = c->r[2] << 16;
    c->r[3] = (uint32_t)(int16_t)c->mem_r16((c->r[16] + (uint32_t)130));
    c->r[2] = (uint32_t)((int32_t)c->r[2] >> 16);
    c->r[2] = (uint32_t)((int32_t)c->r[2] < (int32_t)c->r[3]);
    c->r[3] = (uint32_t)c->mem_r16((c->r[16] + (uint32_t)130));
    { int _t = (c->r[2] != c->r[0]);  if (_t) goto L_8007DA1C; }
    c->r[2] = (uint32_t)c->mem_r8((c->r[16] + (uint32_t)5));
    c->mem_w16((c->r[16] + (uint32_t)86), (uint16_t)c->r[3]);
    c->r[2] = c->r[2] + (uint32_t)1; goto L_8007DA18;
  L_8007D964:;
    c->r[3] = (uint32_t)c->mem_r16((c->r[16] + (uint32_t)88));
    c->r[4] = (uint32_t)c->mem_r16((c->r[16] + (uint32_t)96));
    c->r[2] = (uint32_t)8064u << 16;
    c->mem_w16((c->r[2] + (uint32_t)382), (uint16_t)c->r[0]);
    c->r[3] = c->r[3] + c->r[4];
    c->r[2] = c->r[4] + c->r[0];
    c->r[4] = (uint32_t)(int16_t)c->mem_r16((c->r[16] + (uint32_t)132));
    c->mem_w16((c->r[16] + (uint32_t)88), (uint16_t)c->r[3]);
    c->r[3] = (uint32_t)(int16_t)c->mem_r16((c->r[16] + (uint32_t)88));
    c->r[2] = c->r[2] + (uint32_t)4;
    c->mem_w16((c->r[16] + (uint32_t)96), (uint16_t)c->r[2]);
    c->r[2] = (uint32_t)c->mem_r16((c->r[16] + (uint32_t)132));
    c->r[3] = (uint32_t)((int32_t)c->r[3] < (int32_t)c->r[4]);
    { int _t = (c->r[3] != c->r[0]);  if (_t) goto L_8007D9C4; }
    c->mem_w16((c->r[16] + (uint32_t)88), (uint16_t)c->r[2]);
    c->r[2] = c->r[0] + (uint32_t)5;
    c->mem_w16((c->r[16] + (uint32_t)64), (uint16_t)c->r[2]);
    c->r[2] = c->mem_r32((c->r[16] + (uint32_t)20));
    c->r[3] = (uint32_t)c->mem_r8((c->r[16] + (uint32_t)5));
    c->r[2] = c->r[2] + (uint32_t)1;
    c->r[3] = c->r[3] + (uint32_t)1;
    c->mem_w32((c->r[16] + (uint32_t)20), c->r[2]);
    c->mem_w8((c->r[16] + (uint32_t)5), (uint8_t)c->r[3]);
  L_8007D9C4:;
    c->r[2] = (uint32_t)c->mem_r16((c->r[16] + (uint32_t)88));
    c->r[2] = c->r[2] << 16;
    c->r[3] = (uint32_t)((int32_t)c->r[2] >> 16);
    c->r[2] = c->r[2] >> 31;
    c->r[3] = c->r[3] + c->r[2];
    c->r[3] = (uint32_t)((int32_t)c->r[3] >> 1);
    c->r[2] = c->r[0] + (uint32_t)160;
    c->r[2] = c->r[2] - c->r[3];
    c->mem_w16((c->r[16] + (uint32_t)84), (uint16_t)c->r[2]); goto L_8007DA1C;
  L_8007D9F0:;
    c->r[2] = (uint32_t)c->mem_r16((c->r[16] + (uint32_t)64));
    c->r[3] = (uint32_t)8064u << 16;
    c->mem_w16((c->r[3] + (uint32_t)382), (uint16_t)c->r[0]);
    c->r[2] = c->r[2] + (uint32_t)-1;
    c->mem_w16((c->r[16] + (uint32_t)64), (uint16_t)c->r[2]);
    c->r[2] = c->r[2] << 16;
    { int _t = ((int32_t)c->r[2] > 0); c->r[2] = c->r[0] + (uint32_t)3; if (_t) goto L_8007DA1C; }
    c->mem_w16((c->r[16] + (uint32_t)64), (uint16_t)c->r[2]);
    c->r[2] = c->r[0] + (uint32_t)1;
  L_8007DA18:;
    c->mem_w8((c->r[16] + (uint32_t)5), (uint8_t)c->r[2]);
  L_8007DA1C:;
    c->r[31] = 0x8007DA24u;
    c->r[4] = c->r[16] + c->r[0]; func_8007CC00(c);
    c->r[4] = c->r[16] + (uint32_t)84;
    c->r[2] = (uint32_t)c->mem_r8((c->r[16] + (uint32_t)24));
    c->r[6] = c->r[0] + (uint32_t)1;
    c->r[2] = c->r[29] + c->r[2];
    c->r[5] = (uint32_t)c->mem_r8((c->r[2] + (uint32_t)16));
    c->r[31] = 0x8007DA40u;
    c->r[7] = c->r[0] + (uint32_t)2; func_8005019C(c);
  L_8007DA40:;
    c->r[31] = c->mem_r32((c->r[29] + (uint32_t)28));
    c->r[16] = c->mem_r32((c->r[29] + (uint32_t)24));
    c->r[29] = c->r[29] + (uint32_t)32; return;
    return;
}

// ---- wiring -----------------------------------------------------------------------------------
// The only caller reaches this through the direct `func_8007D594(c)` thunk (generated/shard_5.c),
// NOT through rec_dispatch, so the SETTER is mandatory: installing without it leaves the native
// registered and never run while the substrate quietly keeps doing the work (the same trap
// interact_scan documents).
static void ov_dialog_box_sm_step(Core* c) { DialogBoxSm::step(c); }

void dialog_box_sm_install() {
  extern void gen_func_8007D594(Core*);
  extern void shard_set_override(uint32_t, void (*)(Core*));
  overrides::install(0x8007D594u, "DialogBoxSm::step", ov_dialog_box_sm_step, gen_func_8007D594,
                     shard_set_override);
}
