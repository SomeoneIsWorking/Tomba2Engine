// game/ai/substate_edge_native.cpp — see substate_edge_native.h.
//
// All three bodies are port_gen output: the gen function's guest-visible operations verbatim, frame
// and spills included. That choice is the point rather than convenience — of five hand-written
// drafts checked on 2026-07-29, three were defective (claim C021), and the sibling draft file for
// this very orchestrator omits 22 guest stack spills across its four bodies.
//
// The RE that named these came from a multi-agent pass; each spec was then adversarially verified
// against the gen body. Nothing below depends on those specs being right — the bodies are
// transcriptions and port_check gates them — but the NAMES do, so treat a name as a description of
// observed mechanism, not a claim about intent. FUN_801316CC's analyst explicitly returned UNKNOWN
// for the game-level purpose; tickChildOscillators describes what it does, not why.
#include "core.h"
#include "game.h"
#include "substate_edge_native.h"
#include "override_registry.h"   // engine_set_override_a00
#include "ov_a00_decls.h"        // the gen bodies the oracle leg runs

// ORACLE: ov_a00_gen_80130AC4
void SubstateEdgeLeaves::visibilityGate(Core* c) {
    c->r[29] = c->r[29] + (uint32_t)-48;
    c->mem_w32((c->r[29] + (uint32_t)40), c->r[16]);
    c->r[16] = c->r[4] + c->r[0];
    c->mem_w32((c->r[29] + (uint32_t)44), c->r[31]);
    c->r[3] = (uint32_t)c->mem_r16((c->r[16] + (uint32_t)96));
    c->r[2] = c->r[3] & 1u;
    { int _t = (c->r[2] == c->r[0]); c->r[2] = (uint32_t)32782u << 16; if (_t) goto L_80130CA8; }
    c->r[4] = (uint32_t)c->mem_r8((c->r[2] + (uint32_t)32426));
    c->r[2] = (uint32_t)(c->r[4] < (uint32_t)12);
    { int _t = (c->r[2] == c->r[0]); c->r[2] = c->r[3] & 2u; if (_t) goto L_80130D48; }
    { int _t = (c->r[2] == c->r[0]); c->r[6] = c->r[0] + c->r[0]; if (_t) goto L_80130BD4; }
    c->r[7] = c->r[29] + (uint32_t)16;
    c->r[5] = c->r[6] << 3;
  L_80130B0C:;
    c->r[6] = c->r[6] + (uint32_t)1;
    c->r[4] = c->r[6] << 2;
    c->r[4] = c->r[16] + c->r[4];
    c->r[2] = c->mem_r32((c->r[4] + (uint32_t)192));
    c->r[3] = c->mem_r32((c->r[16] + (uint32_t)192));
    c->r[2] = (uint32_t)c->mem_r16((c->r[2] + (uint32_t)44));
    c->r[3] = (uint32_t)c->mem_r16((c->r[3] + (uint32_t)44));
    c->r[5] = c->r[7] + c->r[5];
    c->r[2] = c->r[2] - c->r[3];
    c->mem_w16((c->r[5] + (uint32_t)0), (uint16_t)c->r[2]);
    c->r[2] = c->mem_r32((c->r[4] + (uint32_t)192));
    c->r[3] = c->mem_r32((c->r[16] + (uint32_t)192));
    c->r[2] = (uint32_t)c->mem_r16((c->r[2] + (uint32_t)48));
    c->r[3] = (uint32_t)c->mem_r16((c->r[3] + (uint32_t)48));
    c->r[2] = c->r[2] - c->r[3];
    c->mem_w16((c->r[5] + (uint32_t)2), (uint16_t)c->r[2]);
    c->r[2] = c->mem_r32((c->r[4] + (uint32_t)192));
    c->r[3] = c->mem_r32((c->r[16] + (uint32_t)192));
    c->r[2] = (uint32_t)c->mem_r16((c->r[2] + (uint32_t)52));
    c->r[3] = (uint32_t)c->mem_r16((c->r[3] + (uint32_t)52));
    c->r[2] = c->r[2] - c->r[3];
    c->mem_w16((c->r[5] + (uint32_t)4), (uint16_t)c->r[2]);
    c->r[2] = (uint32_t)((int32_t)c->r[6] < 3);
    { int _t = (c->r[2] != c->r[0]); c->r[5] = c->r[6] << 3; if (_t) goto L_80130B0C; }
    c->r[5] = (uint32_t)(int16_t)c->mem_r16((c->r[29] + (uint32_t)16));
    c->r[6] = (uint32_t)(int16_t)c->mem_r16((c->r[29] + (uint32_t)18));
    c->r[7] = (uint32_t)(int16_t)c->mem_r16((c->r[29] + (uint32_t)20));
    c->r[31] = 0x80130B8Cu;
    c->r[4] = c->r[16] + c->r[0]; rec_dispatch(c, 0x80077A4Cu);
    { int _t = (c->r[2] != c->r[0]); c->r[2] = c->r[0] + (uint32_t)1; if (_t) goto L_80130D4C; }
    c->r[5] = (uint32_t)(int16_t)c->mem_r16((c->r[29] + (uint32_t)24));
    c->r[6] = (uint32_t)(int16_t)c->mem_r16((c->r[29] + (uint32_t)26));
    c->r[7] = (uint32_t)(int16_t)c->mem_r16((c->r[29] + (uint32_t)28));
    c->r[31] = 0x80130BA8u;
    c->r[4] = c->r[16] + c->r[0]; rec_dispatch(c, 0x80077A4Cu);
    { int _t = (c->r[2] != c->r[0]); c->r[2] = c->r[0] + (uint32_t)1; if (_t) goto L_80130D4C; }
    c->r[5] = (uint32_t)(int16_t)c->mem_r16((c->r[29] + (uint32_t)32));
    c->r[6] = (uint32_t)(int16_t)c->mem_r16((c->r[29] + (uint32_t)34));
    c->r[7] = (uint32_t)(int16_t)c->mem_r16((c->r[29] + (uint32_t)36));
    c->r[31] = 0x80130BC4u;
    c->r[4] = c->r[16] + c->r[0]; rec_dispatch(c, 0x80077A4Cu);
    { int _t = (c->r[2] == c->r[0]); c->r[2] = c->r[0] + (uint32_t)1; if (_t) goto L_80130D48; }
     goto L_80130D4C;
  L_80130BD4:;
    c->r[2] = (uint32_t)32780u << 16;
    c->r[3] = (uint32_t)c->mem_r8((c->r[2] + (uint32_t)-1892));
    c->r[2] = c->r[0] + (uint32_t)2;
    { int _t = (c->r[3] == c->r[2]); c->r[2] = c->r[0] + (uint32_t)1; if (_t) goto L_80130BF0; }
    { int _t = (c->r[4] == c->r[2]); c->r[2] = c->r[0] + c->r[0]; if (_t) goto L_80130D4C; }
  L_80130BF0:;
    c->r[6] = c->r[0] + c->r[0];
    c->r[7] = c->r[29] + (uint32_t)16;
    c->r[5] = c->r[6] << 3;
  L_80130BFC:;
    c->r[6] = c->r[6] + (uint32_t)1;
    c->r[4] = c->r[6] << 2;
    c->r[4] = c->r[16] + c->r[4];
    c->r[2] = c->mem_r32((c->r[4] + (uint32_t)192));
    c->r[3] = c->mem_r32((c->r[16] + (uint32_t)192));
    c->r[2] = (uint32_t)c->mem_r16((c->r[2] + (uint32_t)44));
    c->r[3] = (uint32_t)c->mem_r16((c->r[3] + (uint32_t)44));
    c->r[5] = c->r[7] + c->r[5];
    c->r[2] = c->r[2] - c->r[3];
    c->mem_w16((c->r[5] + (uint32_t)0), (uint16_t)c->r[2]);
    c->r[2] = c->mem_r32((c->r[4] + (uint32_t)192));
    c->r[3] = c->mem_r32((c->r[16] + (uint32_t)192));
    c->r[2] = (uint32_t)c->mem_r16((c->r[2] + (uint32_t)48));
    c->r[3] = (uint32_t)c->mem_r16((c->r[3] + (uint32_t)48));
    c->r[2] = c->r[2] - c->r[3];
    c->mem_w16((c->r[5] + (uint32_t)2), (uint16_t)c->r[2]);
    c->r[2] = c->mem_r32((c->r[4] + (uint32_t)192));
    c->r[3] = c->mem_r32((c->r[16] + (uint32_t)192));
    c->r[2] = (uint32_t)c->mem_r16((c->r[2] + (uint32_t)52));
    c->r[3] = (uint32_t)c->mem_r16((c->r[3] + (uint32_t)52));
    c->r[2] = c->r[2] - c->r[3];
    c->mem_w16((c->r[5] + (uint32_t)4), (uint16_t)c->r[2]);
    c->r[2] = (uint32_t)((int32_t)c->r[6] < 2);
    { int _t = (c->r[2] != c->r[0]); c->r[5] = c->r[6] << 3; if (_t) goto L_80130BFC; }
    c->r[5] = (uint32_t)(int16_t)c->mem_r16((c->r[29] + (uint32_t)16));
    c->r[6] = (uint32_t)(int16_t)c->mem_r16((c->r[29] + (uint32_t)18));
    c->r[7] = (uint32_t)(int16_t)c->mem_r16((c->r[29] + (uint32_t)20));
    c->r[31] = 0x80130C7Cu;
    c->r[4] = c->r[16] + c->r[0]; rec_dispatch(c, 0x80077A4Cu);
    { int _t = (c->r[2] != c->r[0]); c->r[2] = c->r[0] + (uint32_t)1; if (_t) goto L_80130D4C; }
    c->r[5] = (uint32_t)(int16_t)c->mem_r16((c->r[29] + (uint32_t)24));
    c->r[6] = (uint32_t)(int16_t)c->mem_r16((c->r[29] + (uint32_t)26));
    c->r[7] = (uint32_t)(int16_t)c->mem_r16((c->r[29] + (uint32_t)28));
    c->r[31] = 0x80130C98u;
    c->r[4] = c->r[16] + c->r[0]; rec_dispatch(c, 0x80077A4Cu);
    { int _t = (c->r[2] == c->r[0]); c->r[2] = c->r[0] + (uint32_t)1; if (_t) goto L_80130D48; }
     goto L_80130D4C;
  L_80130CA8:;
    c->r[3] = (uint32_t)c->mem_r8((c->r[2] + (uint32_t)32426));
    c->r[2] = c->r[3] + (uint32_t)-14;
    c->r[2] = (uint32_t)(c->r[2] < (uint32_t)14);
    { int _t = (c->r[2] == c->r[0]); c->r[2] = c->r[3] + (uint32_t)-19; if (_t) goto L_80130D48; }
    c->r[2] = (uint32_t)(c->r[2] < (uint32_t)5);
    { int _t = (c->r[2] == c->r[0]); c->r[4] = c->r[16] + c->r[0]; if (_t) goto L_80130CD4; }
    c->r[2] = c->r[0] + (uint32_t)1; goto L_80130D4C;
  L_80130CD4:;
    c->r[2] = c->mem_r32((c->r[16] + (uint32_t)200));
    c->r[3] = c->mem_r32((c->r[16] + (uint32_t)192));
    c->r[5] = (uint32_t)c->mem_r16((c->r[2] + (uint32_t)44));
    c->r[2] = (uint32_t)c->mem_r16((c->r[3] + (uint32_t)44));
    c->r[5] = c->r[5] - c->r[2];
    c->mem_w16((c->r[29] + (uint32_t)16), (uint16_t)c->r[5]);
    c->r[5] = c->r[5] << 16;
    c->r[2] = c->mem_r32((c->r[4] + (uint32_t)200));
    c->r[3] = c->mem_r32((c->r[4] + (uint32_t)192));
    c->r[6] = (uint32_t)c->mem_r16((c->r[2] + (uint32_t)48));
    c->r[2] = (uint32_t)c->mem_r16((c->r[3] + (uint32_t)48));
    c->r[5] = (uint32_t)((int32_t)c->r[5] >> 16);
    c->r[6] = c->r[6] - c->r[2];
    c->mem_w16((c->r[29] + (uint32_t)18), (uint16_t)c->r[6]);
    c->r[6] = c->r[6] << 16;
    c->r[2] = c->mem_r32((c->r[4] + (uint32_t)200));
    c->r[3] = c->mem_r32((c->r[4] + (uint32_t)192));
    c->r[2] = (uint32_t)c->mem_r16((c->r[2] + (uint32_t)52));
    c->r[3] = (uint32_t)c->mem_r16((c->r[3] + (uint32_t)52));
    c->r[6] = (uint32_t)((int32_t)c->r[6] >> 16);
    c->r[2] = c->r[2] - c->r[3];
    c->r[7] = c->r[2] << 16;
    c->r[7] = (uint32_t)((int32_t)c->r[7] >> 16);
    c->r[31] = 0x80130D3Cu;
    c->mem_w16((c->r[29] + (uint32_t)20), (uint16_t)c->r[2]); rec_dispatch(c, 0x80077A4Cu);
    c->r[3] = c->r[2] + c->r[0];
    { int _t = (c->r[3] != c->r[0]); c->r[2] = c->r[0] + (uint32_t)1; if (_t) goto L_80130D4C; }
  L_80130D48:;
    c->r[2] = c->r[0] + c->r[0];
  L_80130D4C:;
    c->r[31] = c->mem_r32((c->r[29] + (uint32_t)44));
    c->r[16] = c->mem_r32((c->r[29] + (uint32_t)40));
    c->r[29] = c->r[29] + (uint32_t)48; return;
    return;
}

// ORACLE: ov_a00_gen_801316CC
void SubstateEdgeLeaves::tickChildOscillators(Core* c) {
    c->r[29] = c->r[29] + (uint32_t)-32;
    c->mem_w32((c->r[29] + (uint32_t)20), c->r[17]);
    c->r[17] = c->r[4] + c->r[0];
    c->mem_w32((c->r[29] + (uint32_t)24), c->r[31]);
    c->mem_w32((c->r[29] + (uint32_t)16), c->r[16]);
    c->r[2] = (uint32_t)c->mem_r16((c->r[17] + (uint32_t)96));
    c->r[2] = c->r[2] & 4u;
    { int _t = (c->r[2] == c->r[0]); c->r[16] = c->r[0] + (uint32_t)2; if (_t) goto L_80131754; }
    c->r[2] = c->r[16] << 16;
  L_801316F8:;
    c->r[2] = (uint32_t)((int32_t)c->r[2] >> 14);
    c->r[3] = c->r[2] + (uint32_t)-4;
    c->r[2] = (uint32_t)c->mem_r16((c->r[17] + (uint32_t)96));
    c->r[2] = c->r[2] & 2u;
    { int _t = (c->r[2] != c->r[0]); c->r[5] = c->r[3] + c->r[0]; if (_t) goto L_80131718; }
    c->r[5] = c->r[3] + (uint32_t)-1;
  L_80131718:;
    c->r[5] = c->r[5] << 16;
    c->r[4] = c->r[17] + c->r[0];
    c->r[31] = 0x80131728u;
    c->r[5] = (uint32_t)((int32_t)c->r[5] >> 16); ov_a00_func_80130D5C(c);
    c->r[2] = (uint32_t)c->mem_r16((c->r[17] + (uint32_t)96));
    c->r[2] = c->r[2] & 2u;
    { int _t = (c->r[2] == c->r[0]); c->r[2] = c->r[16] + (uint32_t)1; if (_t) goto L_80131754; }
    c->r[16] = c->r[2] + c->r[0];
    c->r[2] = c->r[2] << 16;
    c->r[2] = (uint32_t)((int32_t)c->r[2] >> 16);
    c->r[2] = (uint32_t)((int32_t)c->r[2] < 4);
    { int _t = (c->r[2] != c->r[0]); c->r[2] = c->r[16] << 16; if (_t) goto L_801316F8; }
  L_80131754:;
    c->r[31] = c->mem_r32((c->r[29] + (uint32_t)24));
    c->r[17] = c->mem_r32((c->r[29] + (uint32_t)20));
    c->r[16] = c->mem_r32((c->r[29] + (uint32_t)16));
    c->r[29] = c->r[29] + (uint32_t)32; return;
    return;
}

// ORACLE: ov_a00_gen_80131134
void SubstateEdgeLeaves::armPendingChildPair(Core* c) {
    c->r[6] = c->r[4] + c->r[0];
    c->r[2] = (uint32_t)c->mem_r8((c->r[6] + (uint32_t)3));
    c->r[2] = (uint32_t)(c->r[2] < (uint32_t)2);
    { int _t = (c->r[2] == c->r[0]);  if (_t) goto L_801312C4; }
    c->r[2] = (uint32_t)c->mem_r16((c->r[6] + (uint32_t)122));
    c->r[3] = c->r[2] & 3u;
    c->r[5] = c->r[3] + c->r[0];
    { int _t = (c->r[5] == c->r[0]); c->r[7] = c->r[3] + c->r[0]; if (_t) goto L_801312C4; }
    c->r[2] = c->r[5] << 2;
    c->r[2] = c->r[6] + c->r[2];
    c->r[2] = c->mem_r32((c->r[2] + (uint32_t)192));
    c->r[2] = (uint32_t)c->mem_r8((c->r[2] + (uint32_t)62));
    c->r[2] = c->r[2] & 3u;
    { int _t = (c->r[2] != c->r[0]);  if (_t) goto L_801312C4; }
    c->r[2] = (uint32_t)c->mem_r8((c->r[6] + (uint32_t)94));
    c->r[2] = c->r[2] & 2u;
    { int _t = (c->r[2] == c->r[0]); c->r[4] = c->r[3] + c->r[0]; if (_t) goto L_801311B8; }
    c->r[2] = (uint32_t)(int16_t)c->mem_r16((c->r[6] + (uint32_t)108));
    { int _t = (c->r[2] != c->r[5]);  if (_t) goto L_801311B8; }
    c->r[2] = (uint32_t)c->mem_r16((c->r[6] + (uint32_t)110));
    c->r[2] = c->r[2] & 4095u; goto L_801311D0;
  L_801311B8:;
    c->r[2] = c->r[4] << 16;
    c->r[2] = (uint32_t)((int32_t)c->r[2] >> 14);
    c->r[2] = c->r[6] + c->r[2];
    c->r[2] = c->mem_r32((c->r[2] + (uint32_t)192));
    c->r[2] = (uint32_t)c->mem_r16((c->r[2] + (uint32_t)12));
  L_801311D0:;
    c->r[3] = c->r[2] << 16;
    c->r[3] = (uint32_t)((int32_t)c->r[3] >> 16);
    c->r[2] = c->r[0] + (uint32_t)2048;
    c->r[2] = c->r[2] << 16;
    c->r[2] = (uint32_t)((int32_t)c->r[2] >> 16);
    { int _t = (c->r[3] != c->r[2]); c->r[2] = c->r[4] << 16; if (_t) goto L_801311FC; }
    c->r[2] = (uint32_t)((int32_t)c->r[2] >> 14);
    c->r[4] = c->r[2] + (uint32_t)-2; goto L_80131204;
  L_801311FC:;
    c->r[2] = (uint32_t)((int32_t)c->r[2] >> 14);
    c->r[4] = c->r[2] + (uint32_t)-4;
  L_80131204:;
    c->r[2] = (uint32_t)c->mem_r16((c->r[6] + (uint32_t)96));
    c->r[2] = c->r[2] & 2u;
    { int _t = (c->r[2] != c->r[0]); c->r[3] = c->r[4] << 16; if (_t) goto L_80131220; }
    c->r[4] = c->r[4] + (uint32_t)-1;
    c->r[3] = c->r[4] << 16;
  L_80131220:;
    c->r[3] = (uint32_t)((int32_t)c->r[3] >> 16);
    c->r[2] = c->r[3] << 2;
    c->r[2] = c->r[6] + c->r[2];
    c->r[4] = c->r[0] + (uint32_t)1;
    c->r[3] = c->r[3] + (uint32_t)1;
    c->r[3] = c->r[3] << 2;
    c->r[2] = c->mem_r32((c->r[2] + (uint32_t)192));
    c->r[3] = c->r[6] + c->r[3];
    c->mem_w8((c->r[2] + (uint32_t)62), (uint8_t)c->r[4]);
    c->r[2] = c->mem_r32((c->r[3] + (uint32_t)192));
    c->mem_w8((c->r[2] + (uint32_t)62), (uint8_t)c->r[4]);
    c->r[2] = c->r[7] << 2;
    c->r[2] = c->r[6] + c->r[2];
    c->r[3] = c->mem_r32((c->r[2] + (uint32_t)192));
    c->r[2] = (uint32_t)c->mem_r8((c->r[3] + (uint32_t)62));
    c->r[2] = c->r[2] | 2u;
    c->mem_w8((c->r[3] + (uint32_t)62), (uint8_t)c->r[2]);
    c->r[2] = (uint32_t)c->mem_r16((c->r[6] + (uint32_t)122));
    c->r[3] = c->r[2] & 3u;
    c->r[2] = c->r[0] + (uint32_t)2;
    { int _t = (c->r[3] == c->r[2]); c->r[2] = c->r[0] + (uint32_t)3; if (_t) goto L_80131298; }
    { int _t = (c->r[3] == c->r[2]); c->r[2] = c->r[0] + (uint32_t)8; if (_t) goto L_8013129C; }
     goto L_801312A0;
  L_80131298:;
    c->r[2] = c->r[0] + (uint32_t)4;
  L_8013129C:;
    c->mem_w16((c->r[6] + (uint32_t)114), (uint16_t)c->r[2]);
  L_801312A0:;
    c->r[2] = (uint32_t)c->mem_r16((c->r[6] + (uint32_t)122));
    c->r[2] = c->r[2] & 4u;
    { int _t = (c->r[2] == c->r[0]);  if (_t) goto L_801312C4; }
    c->r[2] = (uint32_t)c->mem_r16((c->r[6] + (uint32_t)114));
    c->r[2] = c->r[2] + (uint32_t)2;
    c->mem_w16((c->r[6] + (uint32_t)114), (uint16_t)c->r[2]);
  L_801312C4:;
     return;
    return;
}

void SubstateEdgeLeaves::registerOverrides(Game*) {
  engine_set_override_a00(0x80130AC4u, &SubstateEdgeLeaves::visibilityGate,        ov_a00_gen_80130AC4);
  engine_set_override_a00(0x801316CCu, &SubstateEdgeLeaves::tickChildOscillators,  ov_a00_gen_801316CC);
  engine_set_override_a00(0x80131134u, &SubstateEdgeLeaves::armPendingChildPair,   ov_a00_gen_80131134);
}
