// game/ai/cull_substate_native.cpp — see cull_substate_native.h. Body is port_gen output, verbatim.
#include "core.h"
#include "game.h"
#include "cull_substate_native.h"
#include "override_registry.h"
#include "ov_a00_decls.h"
#include "assembly_node.h"

namespace {
// Node-field offsets shared by every leaf of this orchestrator. Meanings are the ones established
// while converting the oscillator and arm paths (see game/ai/assembly_node.h, which documents each);
// they are named here as constants because these bodies address the node through a guest register
// that must stay live across their own branches, so they cannot hold the C++ lens object.
constexpr uint32_t kNodeRole        = 3;      // < 2 = a master assembly
constexpr uint32_t kNodeState       = 4;      // node[4], outer state
constexpr uint32_t kNodeSubState    = 5;      // node[5], sub-state
constexpr uint32_t kNodePartCount   = 8;      // `cmds` in the ents dump
constexpr uint32_t kNodeModeByte    = 94;     // 0x5E, bit1 selects the angle source
constexpr uint32_t kNodeConfig      = 96;     // 0x60, config word (bit1 pair mode, bit2 has-oscillators)
constexpr uint32_t kNodeAngleSel    = 108;    // 0x6C
constexpr uint32_t kNodeAngleParam  = 110;    // 0x6E, masked to 12 bits
constexpr uint32_t kNodeArmDuration = 114;    // 0x72
constexpr uint32_t kNodePendingCmd  = 122;    // 0x7A, low 2 bits = command
constexpr uint32_t kChildTableOff   = 192;    // 0xC0, the sub-part pointer table
}  // namespace

void ov_a00_func_80133610(Core*);
void ov_a00_func_80133700(Core*);
void ov_a00_func_801332C4(Core*);

// ORACLE: ov_a00_gen_80133550
void CullSubstateLeaves::tickChildEulerZSwing(Core* c) {
    c->r[5] = c->r[4] + c->r[0];
    c->r[3] = (uint32_t)(int16_t)c->mem_r16((c->r[5] + (uint32_t)120));
    c->r[2] = c->r[0] + (uint32_t)1;
    { int _t = (c->r[3] == c->r[2]);  if (_t) goto L_80133578; }
    c->r[2] = c->r[0] + (uint32_t)2;
    { int _t = (c->r[3] == c->r[2]);  if (_t) goto L_80133590; }
     goto L_801335EC;
  L_80133578:;
    c->r[3] = c->mem_r32((c->r[5] + kChildTableOff));
    c->r[2] = (uint32_t)c->mem_r16((c->r[3] + (uint32_t)12));
    c->r[4] = (uint32_t)c->mem_r16((c->r[3] + (uint32_t)20));
    c->r[2] = c->r[2] + c->r[4]; goto L_801335A8;
  L_80133590:;
    c->r[3] = c->mem_r32((c->r[5] + kChildTableOff));
    c->r[2] = (uint32_t)c->mem_r16((c->r[3] + (uint32_t)12));
    c->r[4] = (uint32_t)c->mem_r16((c->r[3] + (uint32_t)20));
    c->r[2] = c->r[2] - c->r[4];
  L_801335A8:;
    c->mem_w16((c->r[3] + (uint32_t)12), (uint16_t)c->r[2]);
    c->r[3] = c->mem_r32((c->r[5] + kChildTableOff));
    c->r[2] = (uint32_t)c->mem_r16((c->r[3] + (uint32_t)20));
    c->r[2] = c->r[2] + (uint32_t)-8;
    c->mem_w16((c->r[3] + (uint32_t)20), (uint16_t)c->r[2]);
    c->r[2] = c->mem_r32((c->r[5] + kChildTableOff));
    c->r[2] = (uint32_t)(int16_t)c->mem_r16((c->r[2] + (uint32_t)20));
    c->r[2] = (uint32_t)((int32_t)c->r[2] < -32);
    { int _t = (c->r[2] == c->r[0]);  if (_t) goto L_801335EC; }
    c->r[2] = c->mem_r32((c->r[5] + kChildTableOff));
    c->mem_w16((c->r[5] + (uint32_t)120), (uint16_t)c->r[0]);
    c->mem_w16((c->r[2] + (uint32_t)12), (uint16_t)c->r[0]);
  L_801335EC:;
    c->r[2] = (uint32_t)c->mem_r8((c->r[5] + (uint32_t)43));
    { int _t = (c->r[2] == c->r[0]);  if (_t) goto L_80133608; }
    c->r[2] = c->mem_r32((c->r[5] + kChildTableOff));
    c->mem_w16((c->r[5] + (uint32_t)120), (uint16_t)c->r[0]);
    c->mem_w16((c->r[2] + (uint32_t)12), (uint16_t)c->r[0]);
  L_80133608:;
     return;
    return;
}

// FUN_80132A88 — the phase-advancing sibling of tickChildEulerZSwing above. 7,650 substrate
// dispatches per 6000 replay frames. Replaces a hand draft that carried FIVE defects, three of them
// guest-visible divergences.
// ORACLE: ov_a00_gen_80132A88
void CullSubstateLeaves::tickChildEulerZSwingPhase(Core* c) {
    c->r[29] = c->r[29] + (uint32_t)-32;
    c->mem_w32((c->r[29] + (uint32_t)16), c->r[16]);
    c->r[16] = c->r[4] + c->r[0];
    c->mem_w32((c->r[29] + (uint32_t)20), c->r[17]);
    c->r[2] = c->r[0] + (uint32_t)1;
    c->mem_w32((c->r[29] + (uint32_t)24), c->r[31]);
    c->r[3] = (uint32_t)(int16_t)c->mem_r16((c->r[16] + (uint32_t)118));
    c->r[4] = (uint32_t)c->mem_r16((c->r[16] + (uint32_t)118));
    { int _t = (c->r[3] == c->r[2]); c->r[17] = c->r[0] + c->r[0]; if (_t) goto L_80132AF4; }
    c->r[2] = (uint32_t)((int32_t)c->r[3] < 2);
    { int _t = (c->r[2] == c->r[0]); c->r[2] = c->r[0] + (uint32_t)2; if (_t) goto L_80132ACC; }
    { int _t = (c->r[3] == c->r[0]);  if (_t) goto L_80132ADC; }
     goto L_80132D30;
  L_80132ACC:;
    { int _t = (c->r[3] == c->r[2]);  if (_t) goto L_80132C34; }
     goto L_80132D30;
  L_80132ADC:;
    c->r[2] = (uint32_t)c->mem_r8((c->r[16] + (uint32_t)43));
    { int _t = (c->r[2] == c->r[0]); c->r[2] = c->r[4] + (uint32_t)1; if (_t) goto L_80132D44; }
    c->mem_w16((c->r[16] + (uint32_t)118), (uint16_t)c->r[2]); goto L_80132D30;
  L_80132AF4:;
    c->r[2] = (uint32_t)c->mem_r8((c->r[16] + (uint32_t)43));
    c->r[2] = c->r[2] & 128u;
    { int _t = (c->r[2] == c->r[0]); c->r[2] = c->r[0] + (uint32_t)512; if (_t) goto L_80132B78; }
    c->r[3] = (uint32_t)(int16_t)c->mem_r16((c->r[16] + (uint32_t)98));
    c->mem_w16((c->r[16] + (uint32_t)76), (uint16_t)c->r[2]);
    { int _t = (c->r[3] == c->r[0]); c->mem_w16((c->r[16] + (uint32_t)72), (uint16_t)c->r[2]); if (_t) goto L_80132B94; }
    c->r[4] = (uint32_t)c->mem_r8((c->r[16] + kNodeRole));
    c->r[2] = c->r[4] & 192u;
    { int _t = (c->r[2] == c->r[0]); c->r[3] = (uint32_t)32789u << 16; if (_t) goto L_80132B3C; }
    c->r[3] = c->r[3] + (uint32_t)-22824;
    c->r[2] = c->r[4] & 63u;
    c->r[2] = c->r[2] + (uint32_t)6; goto L_80132B44;
  L_80132B3C:;
    c->r[2] = (uint32_t)c->mem_r8((c->r[16] + kNodeRole));
    c->r[3] = c->r[3] + (uint32_t)-22824;
  L_80132B44:;
    c->r[2] = c->r[2] << 1;
    c->r[2] = c->r[2] + c->r[3];
    c->r[2] = (uint32_t)c->mem_r16((c->r[2] + (uint32_t)0));
    c->r[2] = c->r[2] << 16;
    c->r[5] = (uint32_t)((int32_t)c->r[2] >> 16);
    c->r[2] = c->r[0] + (uint32_t)-1;
    { int _t = (c->r[5] == c->r[2]);  if (_t) goto L_80132B84; }
    c->r[31] = 0x80132B70u;
    c->r[4] = c->r[16] + c->r[0]; rec_dispatch(c, 0x8004CBD8u);
    c->r[17] = c->r[2] + c->r[0]; goto L_80132B84;
  L_80132B78:;
    c->r[2] = c->r[0] + (uint32_t)384;
    c->mem_w16((c->r[16] + (uint32_t)76), (uint16_t)c->r[2]);
    c->mem_w16((c->r[16] + (uint32_t)72), (uint16_t)c->r[2]);
  L_80132B84:;
    c->r[2] = (uint32_t)(int16_t)c->mem_r16((c->r[16] + (uint32_t)98));
    { int _t = (c->r[2] != c->r[0]); c->r[6] = c->r[0] + (uint32_t)1; if (_t) goto L_80132BB0; }
  L_80132B94:;
    c->r[2] = (uint32_t)c->mem_r16((c->r[16] + (uint32_t)76));
    c->r[3] = (uint32_t)c->mem_r16((c->r[16] + (uint32_t)72));
    c->r[2] = c->r[2] + (uint32_t)-128;
    c->r[3] = c->r[3] + (uint32_t)-128;
    c->mem_w16((c->r[16] + (uint32_t)76), (uint16_t)c->r[2]);
    c->mem_w16((c->r[16] + (uint32_t)72), (uint16_t)c->r[3]);
    c->r[6] = c->r[0] + (uint32_t)1;
  L_80132BB0:;
    c->r[5] = (uint32_t)(int16_t)c->mem_r16((c->r[16] + (uint32_t)86));
    c->r[4] = (uint32_t)c->mem_r8((c->r[16] + (uint32_t)70));
    c->r[2] = c->r[0] + (uint32_t)-8;
    c->mem_w16((c->r[16] + (uint32_t)82), (uint16_t)c->r[2]);
    c->r[31] = 0x80132BC8u;
    c->r[4] = c->r[4] << 4; rec_dispatch(c, 0x80077768u);
    { int _t = (c->r[2] == c->r[0]);  if (_t) goto L_80132BE8; }
    c->r[2] = (uint32_t)c->mem_r16((c->r[16] + (uint32_t)76));
    c->r[3] = (uint32_t)c->mem_r16((c->r[16] + (uint32_t)82));
    c->r[2] = c->r[0] - c->r[2];
    c->r[3] = c->r[0] - c->r[3];
    c->mem_w16((c->r[16] + (uint32_t)76), (uint16_t)c->r[2]);
    c->mem_w16((c->r[16] + (uint32_t)82), (uint16_t)c->r[3]);
  L_80132BE8:;
    { int _t = (c->r[17] == c->r[0]);  if (_t) goto L_80132BFC; }
    c->r[2] = (uint32_t)c->mem_r16((c->r[16] + (uint32_t)76));
    c->mem_w16((c->r[17] + (uint32_t)120), (uint16_t)c->r[2]);
  L_80132BFC:;
    c->r[3] = (uint32_t)c->mem_r8((c->r[16] + (uint32_t)1));
    c->r[2] = c->r[0] + (uint32_t)1;
    c->mem_w8((c->r[16] + (uint32_t)43), (uint8_t)c->r[0]);
    { int _t = (c->r[3] == c->r[0]); c->mem_w8((c->r[16] + (uint32_t)95), (uint8_t)c->r[2]); if (_t) goto L_80132C20; }
    c->r[4] = c->r[0] + (uint32_t)15;
    c->r[5] = c->r[0] + (uint32_t)-5;
    c->r[31] = 0x80132C20u;
    c->r[6] = c->r[0] + c->r[0]; rec_dispatch(c, 0x80074590u);
  L_80132C20:;
    c->r[2] = (uint32_t)c->mem_r16((c->r[16] + (uint32_t)118));
    c->r[2] = c->r[2] + (uint32_t)1;
    c->mem_w16((c->r[16] + (uint32_t)118), (uint16_t)c->r[2]); goto L_80132D30;
  L_80132C34:;
    c->r[4] = c->mem_r32((c->r[16] + kChildTableOff));
    c->r[3] = (uint32_t)c->mem_r16((c->r[16] + (uint32_t)76));
    c->r[2] = (uint32_t)c->mem_r16((c->r[4] + (uint32_t)12));
    c->r[2] = c->r[2] + c->r[3];
    c->mem_w16((c->r[4] + (uint32_t)12), (uint16_t)c->r[2]);
    c->r[4] = c->mem_r32((c->r[16] + kChildTableOff));
    c->r[6] = (uint32_t)(int16_t)c->mem_r16((c->r[16] + (uint32_t)72));
    c->r[3] = (uint32_t)c->mem_r16((c->r[16] + (uint32_t)72));
    c->r[5] = (uint32_t)(int16_t)c->mem_r16((c->r[4] + (uint32_t)12));
    c->r[2] = c->r[0] - c->r[6];
    c->r[2] = (uint32_t)((int32_t)c->r[5] < (int32_t)c->r[2]);
    { int _t = (c->r[2] != c->r[0]); c->r[2] = (uint32_t)((int32_t)c->r[6] < (int32_t)c->r[5]); if (_t) goto L_80132C74; }
    { int _t = (c->r[2] == c->r[0]);  if (_t) goto L_80132CE8; }
  L_80132C74:;
    c->r[2] = (uint32_t)(int16_t)c->mem_r16((c->r[16] + (uint32_t)76));
    { int _t = ((int32_t)c->r[2] <= 0); c->r[2] = c->r[0] - c->r[3]; if (_t) goto L_80132C8C; }
    c->mem_w16((c->r[4] + (uint32_t)12), (uint16_t)c->r[3]); goto L_80132C90;
  L_80132C8C:;
    c->mem_w16((c->r[4] + (uint32_t)12), (uint16_t)c->r[2]);
  L_80132C90:;
    c->r[2] = (uint32_t)c->mem_r16((c->r[16] + (uint32_t)72));
    c->r[2] = c->r[2] + (uint32_t)-24;
    c->mem_w16((c->r[16] + (uint32_t)72), (uint16_t)c->r[2]);
    c->r[2] = c->r[2] << 16;
    c->r[2] = (uint32_t)((int32_t)c->r[2] >> 16);
    { int _t = ((int32_t)c->r[2] > 0); c->r[2] = (uint32_t)((int32_t)c->r[2] < 161); if (_t) goto L_80132CC4; }
    c->r[2] = c->mem_r32((c->r[16] + kChildTableOff));
    c->mem_w16((c->r[2] + (uint32_t)12), (uint16_t)c->r[0]);
    c->mem_w16((c->r[16] + (uint32_t)118), (uint16_t)c->r[0]); goto L_80132D30;
  L_80132CC4:;
    { int _t = (c->r[2] == c->r[0]);  if (_t) goto L_80132CD0; }
    c->mem_w8((c->r[16] + (uint32_t)95), (uint8_t)c->r[0]);
  L_80132CD0:;
    c->r[2] = (uint32_t)c->mem_r16((c->r[16] + (uint32_t)76));
    c->r[3] = (uint32_t)c->mem_r16((c->r[16] + (uint32_t)82));
    c->r[2] = c->r[0] - c->r[2];
    c->r[3] = c->r[0] - c->r[3];
    c->mem_w16((c->r[16] + (uint32_t)76), (uint16_t)c->r[2]);
    c->mem_w16((c->r[16] + (uint32_t)82), (uint16_t)c->r[3]);
  L_80132CE8:;
    c->r[3] = (uint32_t)c->mem_r16((c->r[16] + (uint32_t)76));
    c->r[2] = (uint32_t)c->mem_r16((c->r[16] + (uint32_t)82));
    c->r[3] = c->r[3] + c->r[2];
    c->r[2] = c->r[3] + (uint32_t)-1;
    c->r[2] = c->r[2] & 65535u;
    c->r[2] = (uint32_t)(c->r[2] < (uint32_t)95);
    { int _t = (c->r[2] == c->r[0]); c->mem_w16((c->r[16] + (uint32_t)76), (uint16_t)c->r[3]); if (_t) goto L_80132D14; }
    c->r[2] = c->r[0] + (uint32_t)96; goto L_80132D2C;
  L_80132D14:;
    c->r[2] = c->r[3] << 16;
    c->r[2] = (uint32_t)((int32_t)c->r[2] >> 16);
    { int _t = ((int32_t)c->r[2] >= 0); c->r[2] = (uint32_t)((int32_t)c->r[2] < -95); if (_t) goto L_80132D30; }
    { int _t = (c->r[2] != c->r[0]); c->r[2] = c->r[0] + (uint32_t)-96; if (_t) goto L_80132D30; }
  L_80132D2C:;
    c->mem_w16((c->r[16] + (uint32_t)76), (uint16_t)c->r[2]);
  L_80132D30:;
    c->r[2] = (uint32_t)c->mem_r8((c->r[16] + (uint32_t)43));
    { int _t = (c->r[2] == c->r[0]); c->r[2] = c->r[0] + (uint32_t)1; if (_t) goto L_80132D44; }
    c->mem_w16((c->r[16] + (uint32_t)118), (uint16_t)c->r[2]);
  L_80132D44:;
    c->r[31] = c->mem_r32((c->r[29] + (uint32_t)24));
    c->r[17] = c->mem_r32((c->r[29] + (uint32_t)20));
    c->r[16] = c->mem_r32((c->r[29] + (uint32_t)16));
    c->r[29] = c->r[29] + (uint32_t)32; return;
    return;
}

// FUN_80132954 — the orchestrator's sub-state-zero tick. 7,583 substrate dispatches per 6000 replay
// frames. Replaces a hand draft with SIX defects against the live extent.
// ORACLE: ov_a00_gen_80132954
void CullSubstateLeaves::tickSubstateZero(Core* c) {
    c->r[29] = c->r[29] + (uint32_t)-24;
    c->mem_w32((c->r[29] + (uint32_t)16), c->r[16]);
    c->r[16] = c->r[4] + c->r[0];
    c->mem_w32((c->r[29] + (uint32_t)20), c->r[31]);
    c->r[3] = (uint32_t)c->mem_r8((c->r[16] + (uint32_t)6));
    { int _t = (c->r[3] == c->r[0]); c->r[2] = c->r[0] + (uint32_t)1; if (_t) goto L_80132984; }
    { int _t = (c->r[3] == c->r[2]); c->r[2] = (uint32_t)8064u << 16; if (_t) goto L_801329B4; }
     goto L_80132A30;
  L_80132984:;
    c->r[2] = (uint32_t)c->mem_r16((c->r[16] + (uint32_t)64));
    c->r[2] = c->r[2] + (uint32_t)-1;
    c->mem_w16((c->r[16] + (uint32_t)64), (uint16_t)c->r[2]);
    c->r[2] = c->r[2] << 16;
    { int _t = ((int32_t)c->r[2] > 0);  if (_t) goto L_80132A30; }
    c->r[2] = (uint32_t)c->mem_r8((c->r[16] + (uint32_t)6));
    c->r[2] = c->r[2] + (uint32_t)1;
    c->mem_w8((c->r[16] + (uint32_t)6), (uint8_t)c->r[2]); goto L_80132A30;
  L_801329B4:;
    c->r[2] = (uint32_t)c->mem_r16((c->r[2] + (uint32_t)380));
    c->r[2] = c->r[2] & 15u;
    { int _t = (c->r[2] != c->r[0]); c->r[4] = (uint32_t)32789u << 16; if (_t) goto L_80132A30; }
    c->r[3] = (uint32_t)(int16_t)c->mem_r16((c->r[16] + kNodePendingCmd));
    c->r[4] = c->r[4] + (uint32_t)-22908;
    c->r[2] = c->r[3] << 1;
    c->r[2] = c->r[2] + c->r[3];
    c->r[2] = c->r[2] << 1;
    c->r[2] = c->r[2] + c->r[4];
    c->r[2] = (uint32_t)c->mem_r16((c->r[2] + (uint32_t)0));
    c->mem_w16((c->r[16] + (uint32_t)184), (uint16_t)c->r[2]);
    c->r[2] = c->r[3] << 1;
    c->r[2] = c->r[2] + c->r[3];
    c->r[2] = c->r[2] << 1;
    c->r[2] = c->r[2] + c->r[4];
    c->r[2] = (uint32_t)c->mem_r16((c->r[2] + (uint32_t)2));
    c->mem_w16((c->r[16] + (uint32_t)186), (uint16_t)c->r[2]);
    c->r[2] = c->r[3] << 1;
    c->r[2] = c->r[2] + c->r[3];
    c->r[2] = c->r[2] << 1;
    c->r[2] = c->r[2] + c->r[4];
    c->r[3] = (uint32_t)c->mem_r16((c->r[16] + kNodePendingCmd));
    c->r[2] = (uint32_t)c->mem_r16((c->r[2] + (uint32_t)4));
    c->r[3] = c->r[3] + (uint32_t)1;
    c->r[3] = c->r[3] & 7u;
    c->mem_w16((c->r[16] + kNodePendingCmd), (uint16_t)c->r[3]);
    c->mem_w16((c->r[16] + (uint32_t)188), (uint16_t)c->r[2]);
  L_80132A30:;
    c->r[31] = 0x80132A38u;
    c->r[4] = c->r[16] + c->r[0]; ov_a00_func_801332C4(c);
    c->r[2] = c->r[2] << 16;
    { int _t = (c->r[2] != c->r[0]);  if (_t) goto L_80132A78; }
    c->r[31] = 0x80132A4Cu;
    c->r[4] = c->r[16] + c->r[0]; ov_a00_func_80133700(c);
    c->r[2] = c->r[2] << 16;
    c->r[2] = (uint32_t)((int32_t)c->r[2] >> 16);
    c->r[3] = c->r[0] + (uint32_t)1;
    { int _t = (c->r[2] != c->r[3]); c->r[4] = c->r[16] + c->r[0]; if (_t) goto L_80132A78; }
    c->r[31] = 0x80132A68u;
    c->r[5] = c->r[0] + c->r[0]; ov_a00_func_80133610(c);
    c->r[2] = c->r[0] + (uint32_t)3;
    c->mem_w8((c->r[16] + kNodeSubState), (uint8_t)c->r[2]);
    c->mem_w8((c->r[16] + (uint32_t)6), (uint8_t)c->r[0]);
    c->mem_w16((c->r[16] + kNodeAngleSel), (uint16_t)c->r[0]);
  L_80132A78:;
    c->r[31] = c->mem_r32((c->r[29] + (uint32_t)20));
    c->r[16] = c->mem_r32((c->r[29] + (uint32_t)16));
    c->r[29] = c->r[29] + (uint32_t)24; return;
    return;
}

void CullSubstateLeaves::registerOverrides(Game*) {
  engine_set_override_a00(0x80132A88u, &CullSubstateLeaves::tickChildEulerZSwingPhase, ov_a00_gen_80132A88);
  engine_set_override_a00(0x80132954u, &CullSubstateLeaves::tickSubstateZero,          ov_a00_gen_80132954);
  engine_set_override_a00(0x80133550u, &CullSubstateLeaves::tickChildEulerZSwing, ov_a00_gen_80133550);
}
