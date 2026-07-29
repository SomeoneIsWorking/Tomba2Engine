// collision_resolve.cpp — PORT_GEN draft, byte-faithful transcription of gen_func_80023D48.
// ORACLE: gen_func_80023D48 (../../generated/shard_1.c:2502-2728)
// PORT_GEN: 0x80023D48 ../../generated/shard_1.c:2502-2728
//
// This body is the gen function's guest-visible operations VERBATIM — every c->r[] op,
// mem_r/mem_w call, func_X/rec_dispatch call with its r31 constant, and label/goto is
// preserved unchanged. Faithful by construction; the only allowed next step is RENAMING
// (locals/labels -> named fields/control-flow), verified equivalent by tools/port_check.py.
// UNWIRED — dead code. Do not wire into any dispatch table before running port_check.py
// and the mandatory line-by-line verify pass (docs/fleet-workflow.md §9).
#include "world/collision_resolve.h"
#include "core.h"
#include "game.h"
#include "override_registry.h"   // engine_set_override_main
#include "rec_decls.h"           // gen_func_80023D48 — the body the oracle leg runs


void CollisionResolve::cylinderResolve(Core* c) {
    c->r[29] = c->r[29] + (uint32_t)-80;
    c->mem_w32((c->r[29] + (uint32_t)44), c->r[17]);
    c->r[17] = c->r[4] + c->r[0];
    c->mem_w32((c->r[29] + (uint32_t)64), c->r[22]);
    c->r[22] = c->r[5] + c->r[0];
    c->mem_w32((c->r[29] + (uint32_t)72), c->r[30]);
    c->r[30] = c->r[6] + c->r[0];
    c->r[7] = c->r[7] & 1u;
    c->mem_w32((c->r[29] + (uint32_t)76), c->r[31]);
    c->mem_w32((c->r[29] + (uint32_t)68), c->r[23]);
    c->mem_w32((c->r[29] + (uint32_t)60), c->r[21]);
    c->mem_w32((c->r[29] + (uint32_t)56), c->r[20]);
    c->mem_w32((c->r[29] + (uint32_t)52), c->r[19]);
    c->mem_w32((c->r[29] + (uint32_t)48), c->r[18]);
    { int _t = (c->r[7] == c->r[0]); c->mem_w32((c->r[29] + (uint32_t)40), c->r[16]); if (_t) goto L_80023E60; }
    c->r[4] = (uint32_t)(int16_t)c->mem_r16((c->r[17] + (uint32_t)86));
    c->r[31] = 0x80023D94u;
     func_80083F50(c);
    c->r[3] = (uint32_t)(int16_t)c->mem_r16((c->r[17] + (uint32_t)124));
    { int64_t _p = (int64_t)(int32_t)c->r[2] * (int64_t)(int32_t)c->r[3]; c->lo = (uint32_t)_p; c->hi = (uint32_t)((uint64_t)_p >> 32); }
    c->r[4] = (uint32_t)(int16_t)c->mem_r16((c->r[17] + (uint32_t)86));
    c->r[3] = c->lo;
    c->r[31] = 0x80023DB0u;
    c->r[16] = (uint32_t)((int32_t)c->r[3] >> 12); func_80083E80(c);
    c->r[3] = (uint32_t)(int16_t)c->mem_r16((c->r[17] + (uint32_t)124));
    { int64_t _p = (int64_t)(int32_t)c->r[2] * (int64_t)(int32_t)c->r[3]; c->lo = (uint32_t)_p; c->hi = (uint32_t)((uint64_t)_p >> 32); }
    c->r[4] = (uint32_t)c->mem_r16((c->r[17] + (uint32_t)46));
    c->r[2] = (uint32_t)c->mem_r16((c->r[30] + (uint32_t)44));
    c->r[4] = c->r[4] + c->r[16];
    c->r[4] = c->r[4] - c->r[2];
    c->r[3] = c->lo;
    c->r[2] = c->r[4] << 16;
    c->r[2] = (uint32_t)((int32_t)c->r[2] >> 16);
    { int64_t _p = (int64_t)(int32_t)c->r[2] * (int64_t)(int32_t)c->r[2]; c->lo = (uint32_t)_p; c->hi = (uint32_t)((uint64_t)_p >> 32); }
    c->r[5] = (uint32_t)((int32_t)c->r[3] >> 12);
    c->r[3] = (uint32_t)c->mem_r16((c->r[17] + (uint32_t)54));
    c->r[2] = (uint32_t)c->mem_r16((c->r[30] + (uint32_t)52));
    c->r[3] = c->r[3] - c->r[5];
    c->r[3] = c->r[3] - c->r[2];
    c->r[6] = c->lo;
    c->r[2] = c->r[3] << 16;
    c->r[2] = (uint32_t)((int32_t)c->r[2] >> 16);
    { int64_t _p = (int64_t)(int32_t)c->r[2] * (int64_t)(int32_t)c->r[2]; c->lo = (uint32_t)_p; c->hi = (uint32_t)((uint64_t)_p >> 32); }
    c->mem_w16((c->r[29] + (uint32_t)16), (uint16_t)c->r[16]);
    c->mem_w16((c->r[29] + (uint32_t)32), (uint16_t)c->r[4]);
    c->mem_w16((c->r[29] + (uint32_t)24), (uint16_t)c->r[5]);
    c->r[19] = c->r[3] + c->r[0];
    c->r[9] = c->lo;
    c->r[31] = 0x80023E1Cu;
    c->r[4] = c->r[6] + c->r[9]; func_80084080(c);
    c->r[21] = c->r[2] + c->r[0];
    c->r[5] = c->r[21] & 65535u;
    c->r[2] = (uint32_t)(int16_t)c->mem_r16((c->r[17] + (uint32_t)130));
    c->r[3] = (uint32_t)(int16_t)c->mem_r16((c->r[17] + (uint32_t)128));
    c->r[4] = (uint32_t)(int16_t)c->mem_r16((c->r[22] + (uint32_t)128));
    c->r[2] = c->r[2] - c->r[3];
    c->r[2] = c->r[2] + c->r[4];
    c->r[2] = (uint32_t)((int32_t)c->r[2] < (int32_t)c->r[5]);
    { int _t = (c->r[2] != c->r[0]); c->r[2] = c->r[0] + c->r[0]; if (_t) goto L_800240CC; }
    c->r[23] = (uint32_t)c->mem_r16((c->r[17] + (uint32_t)126));
    c->r[5] = (uint32_t)c->mem_r16((c->r[17] + (uint32_t)50));
    c->r[2] = (uint32_t)c->mem_r16((c->r[30] + (uint32_t)48));
    c->r[3] = (uint32_t)c->mem_r16((c->r[17] + (uint32_t)134));
    c->r[4] = (uint32_t)c->mem_r16((c->r[22] + (uint32_t)132));
    c->r[5] = c->r[5] + c->r[23]; goto L_80023EF4;
  L_80023E60:;
    c->r[4] = (uint32_t)c->mem_r16((c->r[17] + (uint32_t)46));
    c->r[2] = (uint32_t)c->mem_r16((c->r[30] + (uint32_t)44));
    c->r[4] = c->r[4] - c->r[2];
    c->r[2] = c->r[4] << 16;
    c->r[2] = (uint32_t)((int32_t)c->r[2] >> 16);
    { int64_t _p = (int64_t)(int32_t)c->r[2] * (int64_t)(int32_t)c->r[2]; c->lo = (uint32_t)_p; c->hi = (uint32_t)((uint64_t)_p >> 32); }
    c->r[3] = (uint32_t)c->mem_r16((c->r[17] + (uint32_t)54));
    c->r[2] = (uint32_t)c->mem_r16((c->r[30] + (uint32_t)52));
    c->r[3] = c->r[3] - c->r[2];
    c->r[5] = c->lo;
    c->r[2] = c->r[3] << 16;
    c->r[2] = (uint32_t)((int32_t)c->r[2] >> 16);
    { int64_t _p = (int64_t)(int32_t)c->r[2] * (int64_t)(int32_t)c->r[2]; c->lo = (uint32_t)_p; c->hi = (uint32_t)((uint64_t)_p >> 32); }
    c->r[23] = c->r[0] + c->r[0];
    c->mem_w16((c->r[29] + (uint32_t)16), (uint16_t)c->r[0]);
    c->mem_w16((c->r[29] + (uint32_t)24), (uint16_t)c->r[0]);
    c->mem_w16((c->r[29] + (uint32_t)32), (uint16_t)c->r[4]);
    c->r[19] = c->r[3] + c->r[0];
    c->r[9] = c->lo;
    c->r[31] = 0x80023EBCu;
    c->r[4] = c->r[5] + c->r[9]; func_80084080(c);
    c->r[21] = c->r[2] + c->r[0];
    c->r[5] = c->r[21] & 65535u;
    c->r[2] = (uint32_t)(int16_t)c->mem_r16((c->r[17] + (uint32_t)130));
    c->r[3] = (uint32_t)(int16_t)c->mem_r16((c->r[17] + (uint32_t)128));
    c->r[4] = (uint32_t)(int16_t)c->mem_r16((c->r[22] + (uint32_t)128));
    c->r[2] = c->r[2] - c->r[3];
    c->r[2] = c->r[2] + c->r[4];
    c->r[2] = (uint32_t)((int32_t)c->r[2] < (int32_t)c->r[5]);
    { int _t = (c->r[2] != c->r[0]); c->r[2] = c->r[0] + c->r[0]; if (_t) goto L_800240CC; }
    c->r[5] = (uint32_t)c->mem_r16((c->r[17] + (uint32_t)50));
    c->r[2] = (uint32_t)c->mem_r16((c->r[30] + (uint32_t)48));
    c->r[3] = (uint32_t)c->mem_r16((c->r[17] + (uint32_t)134));
    c->r[4] = (uint32_t)c->mem_r16((c->r[22] + (uint32_t)132));
  L_80023EF4:;
    c->r[5] = c->r[5] - c->r[2];
    c->r[2] = (uint32_t)c->mem_r16((c->r[17] + (uint32_t)132));
    c->r[2] = c->r[3] - c->r[2];
    c->r[4] = c->r[4] + c->r[2];
    c->r[16] = c->r[4] + c->r[0];
    c->r[4] = c->r[5] + c->r[4];
    c->r[4] = c->r[4] & 65535u;
    c->r[3] = c->r[3] << 16;
    c->r[2] = (uint32_t)(int16_t)c->mem_r16((c->r[22] + (uint32_t)134));
    c->r[3] = (uint32_t)((int32_t)c->r[3] >> 16);
    c->r[3] = c->r[3] + c->r[2];
    c->r[3] = (uint32_t)((int32_t)c->r[3] < (int32_t)c->r[4]);
    { int _t = (c->r[3] == c->r[0]); c->r[18] = c->r[5] + c->r[0]; if (_t) goto L_80023F38; }
    c->r[2] = c->r[0] + c->r[0]; goto L_800240CC;
  L_80023F38:;
    c->r[2] = c->r[18] << 16;
    { int _t = ((int32_t)c->r[2] >= 0); c->r[20] = c->r[16] + c->r[0]; if (_t) goto L_80023F50; }
    c->r[18] = c->r[0] - c->r[18];
    c->r[16] = c->r[0] - c->r[16]; goto L_80023F6C;
  L_80023F50:;
    c->r[2] = (uint32_t)c->mem_r16((c->r[22] + (uint32_t)134));
    c->r[4] = (uint32_t)c->mem_r16((c->r[22] + (uint32_t)132));
    c->r[3] = (uint32_t)c->mem_r16((c->r[17] + (uint32_t)132));
    c->r[2] = c->r[2] - c->r[4];
    c->r[3] = c->r[3] + c->r[2];
    c->r[20] = c->r[3] + c->r[0];
    c->r[16] = c->r[3] + c->r[0];
  L_80023F6C:;
    c->r[4] = c->r[19] << 16;
    c->r[4] = (uint32_t)((int32_t)c->r[4] >> 16);
    c->r[8] = (uint32_t)c->mem_r16((c->r[29] + (uint32_t)32));
    c->r[4] = c->r[0] - c->r[4];
    c->r[5] = c->r[8] << 16;
    c->r[31] = 0x80023F88u;
    c->r[5] = (uint32_t)((int32_t)c->r[5] >> 16); func_80085690(c);
    c->r[4] = (uint32_t)(int16_t)c->mem_r16((c->r[17] + (uint32_t)130));
    c->r[19] = (uint32_t)8064u << 16;
    c->mem_w32((c->r[19] + (uint32_t)156), c->r[2]);
    c->r[2] = (uint32_t)(int16_t)c->mem_r16((c->r[17] + (uint32_t)128));
    c->r[3] = (uint32_t)(int16_t)c->mem_r16((c->r[22] + (uint32_t)128));
    c->r[4] = c->r[4] - c->r[2];
    c->r[4] = c->r[4] + c->r[3];
    c->r[2] = c->r[21] << 16;
    c->r[2] = (uint32_t)((int32_t)c->r[2] >> 16);
    c->r[4] = c->r[4] - c->r[2];
    c->r[3] = c->r[20] << 16;
    c->r[3] = (uint32_t)((int32_t)c->r[3] >> 16);
    c->r[2] = c->r[18] << 16;
    c->r[2] = (uint32_t)((int32_t)c->r[2] >> 16);
    c->r[3] = c->r[3] - c->r[2];
    c->r[4] = (uint32_t)((int32_t)c->r[4] < (int32_t)c->r[3]);
    { int _t = (c->r[4] == c->r[0]); c->r[2] = c->r[0] + (uint32_t)2; if (_t) goto L_80024074; }
    c->r[3] = (uint32_t)c->mem_r8((c->r[17] + (uint32_t)12));
    { int _t = (c->r[3] != c->r[2]);  if (_t) goto L_80023FF8; }
    c->r[4] = (uint32_t)(int16_t)c->mem_r16((c->r[19] + (uint32_t)156));
    c->r[5] = (uint32_t)(int16_t)c->mem_r16((c->r[17] + (uint32_t)96));
    c->r[31] = 0x80023FF0u;
    c->r[6] = c->r[0] + (uint32_t)1; func_80077768(c);
    c->r[2] = c->r[2] + (uint32_t)2;
    c->mem_w8((c->r[17] + (uint32_t)95), (uint8_t)c->r[2]);
  L_80023FF8:;
    c->r[4] = c->mem_r32((c->r[19] + (uint32_t)156));
    c->r[31] = 0x80024004u;
     func_80083F50(c);
    c->r[3] = (uint32_t)(int16_t)c->mem_r16((c->r[17] + (uint32_t)128));
    c->r[4] = (uint32_t)(int16_t)c->mem_r16((c->r[22] + (uint32_t)128));
    c->r[3] = c->r[3] + c->r[4];
    { int64_t _p = (int64_t)(int32_t)c->r[2] * (int64_t)(int32_t)c->r[3]; c->lo = (uint32_t)_p; c->hi = (uint32_t)((uint64_t)_p >> 32); }
    c->r[4] = c->mem_r32((c->r[19] + (uint32_t)156));
    c->r[8] = c->lo;
    c->r[31] = 0x80024028u;
    c->r[16] = (uint32_t)((int32_t)c->r[8] >> 12); func_80083E80(c);
    c->r[3] = (uint32_t)(int16_t)c->mem_r16((c->r[17] + (uint32_t)128));
    c->r[4] = (uint32_t)(int16_t)c->mem_r16((c->r[22] + (uint32_t)128));
    c->r[3] = c->r[3] + c->r[4];
    { int64_t _p = (int64_t)(int32_t)c->r[2] * (int64_t)(int32_t)c->r[3]; c->lo = (uint32_t)_p; c->hi = (uint32_t)((uint64_t)_p >> 32); }
    c->r[8] = (uint32_t)c->mem_r16((c->r[29] + (uint32_t)16));
    c->r[3] = (uint32_t)c->mem_r16((c->r[30] + (uint32_t)44));
    c->r[2] = c->r[0] + (uint32_t)1;
    c->r[3] = c->r[3] + c->r[16];
    c->r[3] = c->r[3] - c->r[8];
    c->mem_w16((c->r[17] + (uint32_t)46), (uint16_t)c->r[3]);
    c->r[3] = (uint32_t)c->mem_r16((c->r[30] + (uint32_t)52));
    c->r[8] = c->lo;
    c->r[4] = (uint32_t)((int32_t)c->r[8] >> 12);
    c->r[8] = (uint32_t)c->mem_r16((c->r[29] + (uint32_t)24));
    c->r[3] = c->r[3] - c->r[4];
    c->r[3] = c->r[3] - c->r[8];
    c->mem_w16((c->r[17] + (uint32_t)54), (uint16_t)c->r[3]); goto L_800240CC;
  L_80024074:;
    c->r[2] = c->r[16] << 16;
    c->r[5] = (uint32_t)((int32_t)c->r[2] >> 16);
    { int _t = ((int32_t)c->r[5] <= 0); c->r[2] = c->r[0] + (uint32_t)3; if (_t) goto L_800240A4; }
    c->r[3] = c->r[23] << 16;
    c->r[4] = c->mem_r32((c->r[30] + (uint32_t)48));
    c->r[3] = (uint32_t)((int32_t)c->r[3] >> 16);
    c->r[4] = c->r[4] + c->r[5];
    c->r[4] = c->r[4] - c->r[3];
    c->r[4] = c->r[4] << 16;
    c->mem_w32((c->r[17] + (uint32_t)48), c->r[4]); goto L_800240CC;
  L_800240A4:;
    c->r[2] = c->r[0] + (uint32_t)2;
    c->r[3] = c->mem_r32((c->r[30] + (uint32_t)48));
    c->r[4] = c->r[0] + (uint32_t)1;
    c->mem_w8((c->r[17] + (uint32_t)41), (uint8_t)c->r[4]);
    c->r[4] = c->r[23] << 16;
    c->r[4] = (uint32_t)((int32_t)c->r[4] >> 16);
    c->r[3] = c->r[3] + c->r[5];
    c->r[3] = c->r[3] - c->r[4];
    c->r[3] = c->r[3] << 16;
    c->mem_w32((c->r[17] + (uint32_t)48), c->r[3]);
  L_800240CC:;
    c->r[31] = c->mem_r32((c->r[29] + (uint32_t)76));
    c->r[30] = c->mem_r32((c->r[29] + (uint32_t)72));
    c->r[23] = c->mem_r32((c->r[29] + (uint32_t)68));
    c->r[22] = c->mem_r32((c->r[29] + (uint32_t)64));
    c->r[21] = c->mem_r32((c->r[29] + (uint32_t)60));
    c->r[20] = c->mem_r32((c->r[29] + (uint32_t)56));
    c->r[19] = c->mem_r32((c->r[29] + (uint32_t)52));
    c->r[18] = c->mem_r32((c->r[29] + (uint32_t)48));
    c->r[17] = c->mem_r32((c->r[29] + (uint32_t)44));
    c->r[16] = c->mem_r32((c->r[29] + (uint32_t)40));
    c->r[29] = c->r[29] + (uint32_t)80; return;
    return;
}


void CollisionResolve::registerOverrides(Game*) {
  engine_set_override_main(0x80023D48u, &CollisionResolve::cylinderResolve, gen_func_80023D48);
}
