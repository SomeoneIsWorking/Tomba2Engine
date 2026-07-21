// actor_tomba.cpp — PORT_GEN draft, byte-faithful transcription of gen_func_8002288C.
// ORACLE: gen_func_8002288C (../../generated/shard_5.c:1908-2019)
// PORT_GEN: 0x8002288C ../../generated/shard_5.c:1908-2019
//
// This body is the gen function's guest-visible operations VERBATIM — every c->r[] op,
// mem_r/mem_w call, func_X/rec_dispatch call with its r31 constant, and label/goto is
// preserved unchanged. Faithful by construction; the only allowed next step is RENAMING
// (locals/labels -> named fields/control-flow), verified equivalent by tools/port_check.py.
// UNWIRED — dead code. Do not wire into any dispatch table before running port_check.py
// and the mandatory line-by-line verify pass (docs/fleet-workflow.md §9).
#include "player/actor_tomba.h"
#include "core.h"

void rec_dispatch(Core*, uint32_t);  // overlay_router.cpp — shared dispatch choke point

void ActorTomba::framePreTick() {
  Core* c = core;
    c->r[6] = c->r[4] + c->r[0];
    c->r[2] = (uint32_t)c->mem_r8((c->r[6] + (uint32_t)41));
    c->r[2] = c->r[2] & 128u;
    { int _t = (c->r[2] == c->r[0]); c->r[2] = (uint32_t)8064u << 16; if (_t) goto L_800228B0; }
    c->r[3] = c->r[0] + (uint32_t)1;
    c->mem_w32((c->r[2] + (uint32_t)148), c->r[3]); goto L_800228B8;
  L_800228B0:;
    c->mem_w32((c->r[2] + (uint32_t)148), c->r[0]);
    c->r[3] = (uint32_t)c->mem_r8((c->r[6] + (uint32_t)41));
  L_800228B8:;
    c->r[2] = (uint32_t)8064u << 16;
    c->mem_w32((c->r[2] + (uint32_t)152), c->r[3]);
    c->r[2] = (uint32_t)8064u << 16;
    c->r[3] = (uint32_t)c->mem_r8((c->r[2] + (uint32_t)595));
    c->r[3] = c->r[3] & 3u;
    c->mem_w8((c->r[2] + (uint32_t)595), (uint8_t)c->r[3]);
    c->r[2] = (uint32_t)c->mem_r8((c->r[6] + (uint32_t)95));
    c->mem_w8((c->r[6] + (uint32_t)96), (uint8_t)c->r[0]);
    c->r[2] = c->r[2] & 4u;
    { int _t = (c->r[2] != c->r[0]); c->mem_w8((c->r[6] + (uint32_t)41), (uint8_t)c->r[0]); if (_t) goto L_800228EC; }
    c->mem_w8((c->r[6] + (uint32_t)95), (uint8_t)c->r[0]);
  L_800228EC:;
    c->r[2] = (uint32_t)32780u << 16;
    c->r[2] = c->r[2] + (uint32_t)-2040;
    c->mem_w16((c->r[6] + (uint32_t)322), (uint16_t)c->r[0]);
    c->r[4] = (uint32_t)c->mem_r8((c->r[2] + (uint32_t)64));
    c->mem_w8((c->r[2] + (uint32_t)64), (uint8_t)c->r[0]);
    c->r[3] = c->mem_r32((c->r[6] + (uint32_t)56));
    c->r[2] = (uint32_t)8064u << 16;
    c->mem_w16((c->r[2] + (uint32_t)398), (uint16_t)c->r[0]);
    c->r[2] = (uint32_t)8064u << 16;
    c->mem_w32((c->r[2] + (uint32_t)144), c->r[4]);
    c->r[2] = (uint32_t)32778u << 16;
    c->r[2] = c->r[2] + (uint32_t)-11924;
    c->r[3] = (uint32_t)c->mem_r16((c->r[3] + (uint32_t)4));
    c->r[4] = (uint32_t)(int16_t)c->mem_r16((c->r[6] + (uint32_t)382));
    c->r[3] = c->r[3] >> 8;
    c->r[3] = c->r[3] << 3;
    { int _t = ((int32_t)c->r[4] < 0); c->r[5] = c->r[3] + c->r[2]; if (_t) goto L_80022964; }
    c->r[2] = (uint32_t)c->mem_r16((c->r[5] + (uint32_t)0));
    c->mem_w16((c->r[6] + (uint32_t)128), (uint16_t)c->r[2]);
    c->r[2] = (uint32_t)c->mem_r16((c->r[5] + (uint32_t)2));
    c->mem_w16((c->r[6] + (uint32_t)130), (uint16_t)c->r[2]);
    c->r[2] = (uint32_t)c->mem_r16((c->r[5] + (uint32_t)4));
    c->mem_w16((c->r[6] + (uint32_t)132), (uint16_t)c->r[2]);
    c->r[2] = (uint32_t)c->mem_r16((c->r[5] + (uint32_t)6));
    c->mem_w16((c->r[6] + (uint32_t)134), (uint16_t)c->r[2]); goto L_800229E4;
  L_80022964:;
    c->r[2] = (uint32_t)c->mem_r16((c->r[5] + (uint32_t)0));
    c->r[2] = c->r[2] << 16;
    c->r[3] = (uint32_t)((int32_t)c->r[2] >> 16);
    c->r[2] = c->r[2] >> 31;
    c->r[3] = c->r[3] + c->r[2];
    c->r[3] = (uint32_t)((int32_t)c->r[3] >> 1);
    c->mem_w16((c->r[6] + (uint32_t)128), (uint16_t)c->r[3]);
    c->r[2] = (uint32_t)c->mem_r16((c->r[5] + (uint32_t)2));
    c->r[2] = c->r[2] << 16;
    c->r[3] = (uint32_t)((int32_t)c->r[2] >> 16);
    c->r[2] = c->r[2] >> 31;
    c->r[3] = c->r[3] + c->r[2];
    c->r[3] = (uint32_t)((int32_t)c->r[3] >> 1);
    c->mem_w16((c->r[6] + (uint32_t)130), (uint16_t)c->r[3]);
    c->r[2] = (uint32_t)c->mem_r16((c->r[5] + (uint32_t)4));
    c->r[2] = c->r[2] << 16;
    c->r[3] = (uint32_t)((int32_t)c->r[2] >> 16);
    c->r[2] = c->r[2] >> 31;
    c->r[3] = c->r[3] + c->r[2];
    c->r[3] = (uint32_t)((int32_t)c->r[3] >> 1);
    c->mem_w16((c->r[6] + (uint32_t)132), (uint16_t)c->r[3]);
    c->r[2] = (uint32_t)c->mem_r16((c->r[5] + (uint32_t)6));
    c->r[2] = c->r[2] << 16;
    c->r[3] = (uint32_t)((int32_t)c->r[2] >> 16);
    c->r[2] = c->r[2] >> 31;
    c->r[3] = c->r[3] + c->r[2];
    c->r[3] = (uint32_t)((int32_t)c->r[3] >> 1);
    c->mem_w16((c->r[6] + (uint32_t)134), (uint16_t)c->r[3]);
  L_800229E4:;
    c->r[2] = (uint32_t)32780u << 16;
    c->r[2] = (uint32_t)c->mem_r8((c->r[2] + (uint32_t)-1983));
    { int _t = (c->r[2] != c->r[0]);  if (_t) goto L_80022A78; }
    c->r[3] = (uint32_t)c->mem_r8((c->r[6] + (uint32_t)120));
    { int _t = (c->r[3] == c->r[0]); c->r[2] = c->r[0] + (uint32_t)3; if (_t) goto L_80022A78; }
    { int _t = (c->r[3] == c->r[2]);  if (_t) goto L_80022A78; }
    c->r[2] = c->mem_r32((c->r[6] + (uint32_t)16));
    c->r[5] = (uint32_t)c->mem_r16((c->r[6] + (uint32_t)50));
    c->r[4] = (uint32_t)c->mem_r16((c->r[2] + (uint32_t)50));
    c->r[3] = (uint32_t)c->mem_r16((c->r[2] + (uint32_t)134));
    c->r[2] = (uint32_t)c->mem_r16((c->r[2] + (uint32_t)132));
    c->r[4] = c->r[4] - c->r[5];
    c->r[3] = c->r[3] - c->r[2];
    c->r[4] = c->r[4] + c->r[3];
    c->r[2] = (uint32_t)(int16_t)c->mem_r16((c->r[6] + (uint32_t)382));
    { int _t = ((int32_t)c->r[2] < 0); c->r[5] = c->r[4] + c->r[0]; if (_t) goto L_80022A50; }
    c->r[2] = (uint32_t)c->mem_r16((c->r[6] + (uint32_t)134));
    c->r[3] = (uint32_t)c->mem_r16((c->r[6] + (uint32_t)132));
    c->r[2] = c->r[2] - c->r[3]; goto L_80022A6C;
  L_80022A50:;
    c->r[2] = (uint32_t)(int16_t)c->mem_r16((c->r[6] + (uint32_t)134));
    c->r[3] = (uint32_t)(int16_t)c->mem_r16((c->r[6] + (uint32_t)132));
    c->r[2] = c->r[2] - c->r[3];
    c->r[3] = c->r[2] >> 31;
    c->r[2] = c->r[2] + c->r[3];
    c->r[2] = (uint32_t)((int32_t)c->r[2] >> 1);
  L_80022A6C:;
    c->r[2] = c->r[4] + c->r[2];
    c->mem_w16((c->r[6] + (uint32_t)134), (uint16_t)c->r[2]);
    c->mem_w16((c->r[6] + (uint32_t)132), (uint16_t)c->r[5]);
  L_80022A78:;
     return;
    return;
}

