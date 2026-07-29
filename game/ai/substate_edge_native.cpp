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
#include "assembly_node.h"       // AssemblyNode — the typed lens over this class's node

namespace {
// tickChildOscillators' frame + loop constants. The ra value is the RE'd guest return address, not a
// magic number: the sub-part tick is still substrate and spills whatever r31 holds.
constexpr uint32_t kOscFrame      = 32;
constexpr uint32_t kOscSpillSlot  = 16;   // r16
constexpr uint32_t kOscSpillNode  = 20;   // r17
constexpr uint32_t kOscSpillRa    = 24;   // r31
constexpr uint32_t kRaAfterPartTick = 0x80131728u;
constexpr int32_t  kFirstDrivenPart = 2;  // k starts at 2 -> slot 4 (or 3 outside pair mode)
constexpr int32_t  kPartLimit       = 4;  // loop runs k = 2, 3

// Field offsets for armPendingChildPair. Named here rather than via the AssemblyNode lens because
// this body addresses the node through a guest register (r6) that must stay live across its own
// branches — the lens is for bodies that can hold a C++ object. Same meanings, same header.
constexpr uint32_t kNodeRole        = 3;      // < 2 = a master assembly
constexpr uint32_t kNodePartCount   = 8;      // `cmds` in the ents dump
constexpr uint32_t kNodeState       = 4;      // node[4], outer state
constexpr uint32_t kNodeSubState    = 5;      // node[5], sub-state
constexpr uint32_t kNodePendingCmd  = 122;    // 0x7A, low 2 bits = command, bit2 = extra flag
constexpr uint32_t kNodeModeByte    = 94;     // 0x5E, bit1 selects the angle source
constexpr uint32_t kNodeAngleSel    = 108;    // 0x6C, compared against the command
constexpr uint32_t kNodeAngleParam  = 110;    // 0x6E, masked to 12 bits
constexpr uint32_t kNodeConfig      = 96;     // 0x60, the config word (bit1 = pair mode)
constexpr uint32_t kNodeArmDuration = 114;    // 0x72
constexpr uint32_t kChildTableOff   = 192;    // 0xC0, the sub-part pointer table
constexpr uint32_t kChildStateFlags = 62;     // 0x3E on a child record
constexpr uint32_t kChildAccum      = 12;     // 0x0C on a child record
}  // namespace
void ov_a00_func_801308E0(Core*);
void ov_a00_func_80130788(Core*);
void ov_a00_func_801314B4(Core*);

// ORACLE: ov_a00_gen_80130AC4
void SubstateEdgeLeaves::visibilityGate(Core* c) {
    c->r[29] = c->r[29] + (uint32_t)-48;
    c->mem_w32((c->r[29] + (uint32_t)40), c->r[16]);
    c->r[16] = c->r[4] + c->r[0];
    c->mem_w32((c->r[29] + (uint32_t)44), c->r[31]);
    c->r[3] = (uint32_t)c->mem_r16((c->r[16] + kNodeConfig));
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
    c->r[2] = c->mem_r32((c->r[4] + kChildTableOff));
    c->r[3] = c->mem_r32((c->r[16] + kChildTableOff));
    c->r[2] = (uint32_t)c->mem_r16((c->r[2] + (uint32_t)44));
    c->r[3] = (uint32_t)c->mem_r16((c->r[3] + (uint32_t)44));
    c->r[5] = c->r[7] + c->r[5];
    c->r[2] = c->r[2] - c->r[3];
    c->mem_w16((c->r[5] + (uint32_t)0), (uint16_t)c->r[2]);
    c->r[2] = c->mem_r32((c->r[4] + kChildTableOff));
    c->r[3] = c->mem_r32((c->r[16] + kChildTableOff));
    c->r[2] = (uint32_t)c->mem_r16((c->r[2] + (uint32_t)48));
    c->r[3] = (uint32_t)c->mem_r16((c->r[3] + (uint32_t)48));
    c->r[2] = c->r[2] - c->r[3];
    c->mem_w16((c->r[5] + (uint32_t)2), (uint16_t)c->r[2]);
    c->r[2] = c->mem_r32((c->r[4] + kChildTableOff));
    c->r[3] = c->mem_r32((c->r[16] + kChildTableOff));
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
    c->r[2] = c->mem_r32((c->r[4] + kChildTableOff));
    c->r[3] = c->mem_r32((c->r[16] + kChildTableOff));
    c->r[2] = (uint32_t)c->mem_r16((c->r[2] + (uint32_t)44));
    c->r[3] = (uint32_t)c->mem_r16((c->r[3] + (uint32_t)44));
    c->r[5] = c->r[7] + c->r[5];
    c->r[2] = c->r[2] - c->r[3];
    c->mem_w16((c->r[5] + (uint32_t)0), (uint16_t)c->r[2]);
    c->r[2] = c->mem_r32((c->r[4] + kChildTableOff));
    c->r[3] = c->mem_r32((c->r[16] + kChildTableOff));
    c->r[2] = (uint32_t)c->mem_r16((c->r[2] + (uint32_t)48));
    c->r[3] = (uint32_t)c->mem_r16((c->r[3] + (uint32_t)48));
    c->r[2] = c->r[2] - c->r[3];
    c->mem_w16((c->r[5] + (uint32_t)2), (uint16_t)c->r[2]);
    c->r[2] = c->mem_r32((c->r[4] + kChildTableOff));
    c->r[3] = c->mem_r32((c->r[16] + kChildTableOff));
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
    c->r[3] = c->mem_r32((c->r[16] + kChildTableOff));
    c->r[5] = (uint32_t)c->mem_r16((c->r[2] + (uint32_t)44));
    c->r[2] = (uint32_t)c->mem_r16((c->r[3] + (uint32_t)44));
    c->r[5] = c->r[5] - c->r[2];
    c->mem_w16((c->r[29] + (uint32_t)16), (uint16_t)c->r[5]);
    c->r[5] = c->r[5] << 16;
    c->r[2] = c->mem_r32((c->r[4] + (uint32_t)200));
    c->r[3] = c->mem_r32((c->r[4] + kChildTableOff));
    c->r[6] = (uint32_t)c->mem_r16((c->r[2] + (uint32_t)48));
    c->r[2] = (uint32_t)c->mem_r16((c->r[3] + (uint32_t)48));
    c->r[5] = (uint32_t)((int32_t)c->r[5] >> 16);
    c->r[6] = c->r[6] - c->r[2];
    c->mem_w16((c->r[29] + (uint32_t)18), (uint16_t)c->r[6]);
    c->r[6] = c->r[6] << 16;
    c->r[2] = c->mem_r32((c->r[4] + (uint32_t)200));
    c->r[3] = c->mem_r32((c->r[4] + kChildTableOff));
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
  // Guest frame, mirrored exactly: sp descends 32, r17/ra/r16 spill at +20/+24/+16 in that order.
  const uint32_t sp0 = c->r[29];
  c->r[29] = sp0 - kOscFrame;
  c->mem_w32(c->r[29] + kOscSpillNode, c->r[17]);
  c->r[17] = c->r[4];                       // node stays in r17 for the whole body — see below
  c->mem_w32(c->r[29] + kOscSpillRa,   c->r[31]);
  c->mem_w32(c->r[29] + kOscSpillSlot, c->r[16]);

  // LIVE-REGISTER LAW (docs/findings/sbs.md, game/render/subpart_walk.cpp): the sub-part tick is
  // still substrate and its prologue SPILLS its incoming r16/r17 into its own guest frame. So the
  // loop counter and the node pointer are guest-visible state at the call, not bookkeeping — they
  // live in the guest registers and are only NAMED here.
  uint32_t& partIndex = c->r[16];           // the guest's k: 2, then 3
  const AssemblyNode node(c, c->r[17]);

  partIndex = kFirstDrivenPart;
  if (node.hasOscillatingParts()) {
    for (;;) {
      // slot = k*4 - 4, one lower when the assembly is NOT in pair mode. The <<16 >>14 the guest
      // writes is sext16(k) * 4; the shifts are the sign-extension, not a scale trick.
      const int32_t k    = (int16_t)(uint16_t)partIndex;
      const int32_t slot = k * 4 - 4 - (node.oscillatorPairMode() ? 0 : 1);

      c->r[4]  = node.addr();
      c->r[5]  = (uint32_t)(int32_t)(int16_t)(uint16_t)slot;
      c->r[31] = kRaAfterPartTick;
      ov_a00_func_80130D5C(c);              // the per-sub-part oscillator, still substrate

      // Re-read the config word AFTER the call — the tick above can clear pair mode, and when it is
      // clear this loop runs exactly once. Caching it across the call would change behaviour.
      if (!node.oscillatorPairMode()) break;
      partIndex = (uint32_t)(int32_t)(int16_t)(uint16_t)(partIndex + 1);
      if ((int16_t)(uint16_t)partIndex >= kPartLimit) break;
    }
  }

  c->r[31] = c->mem_r32(c->r[29] + kOscSpillRa);
  c->r[17] = c->mem_r32(c->r[29] + kOscSpillNode);
  c->r[16] = c->mem_r32(c->r[29] + kOscSpillSlot);
  c->r[29] = sp0;
}

// ORACLE: ov_a00_gen_80131134
// FUN_80131134 — ARM A PAIR OF ADJACENT SUB-PARTS from the assembly's pending command.
//
// Reads the 2-bit command at node+0x7A. Does nothing unless this is one of the two master assemblies
// (roleByte < 2), a command is pending, and the commanded sub-part is idle. Then it picks an angle —
// the node's own angleParam when modeByte bit1 is set AND angleSelector matches the command, else the
// commanded part's oscillator accumulator — and uses whether that angle is exactly 2048 (a half turn)
// to choose which adjacent PAIR of slots to start. Both parts of the pair get state 1; the commanded
// part additionally gets bit1. Finally armDuration at +0x72 is set from the command (2 -> 4, 3 -> 8)
// and bumped by 2 when the command word's bit2 is set.
//
// FIELD-NAMED, CONTROL FLOW NOT YET REWRITTEN. The offsets are now named (see assembly_node.h for
// what each is); the branch structure is still the guest's labels. That is the same deliberate split
// used on collision_resolve: the naming half is mechanical and independently gate-able, the
// control-flow half is not, and a half-rewritten branch structure is worse than an unrewritten one.
void SubstateEdgeLeaves::armPendingChildPair(Core* c) {
    c->r[6] = c->r[4] + c->r[0];
    c->r[2] = (uint32_t)c->mem_r8((c->r[6] + kNodeRole));
    c->r[2] = (uint32_t)(c->r[2] < (uint32_t)2);
    { int _t = (c->r[2] == c->r[0]);  if (_t) goto L_801312C4; }
    c->r[2] = (uint32_t)c->mem_r16((c->r[6] + kNodePendingCmd));
    c->r[3] = c->r[2] & 3u;
    c->r[5] = c->r[3] + c->r[0];
    { int _t = (c->r[5] == c->r[0]); c->r[7] = c->r[3] + c->r[0]; if (_t) goto L_801312C4; }
    c->r[2] = c->r[5] << 2;
    c->r[2] = c->r[6] + c->r[2];
    c->r[2] = c->mem_r32((c->r[2] + kChildTableOff));
    c->r[2] = (uint32_t)c->mem_r8((c->r[2] + kChildStateFlags));
    c->r[2] = c->r[2] & 3u;
    { int _t = (c->r[2] != c->r[0]);  if (_t) goto L_801312C4; }
    c->r[2] = (uint32_t)c->mem_r8((c->r[6] + kNodeModeByte));
    c->r[2] = c->r[2] & 2u;
    { int _t = (c->r[2] == c->r[0]); c->r[4] = c->r[3] + c->r[0]; if (_t) goto L_801311B8; }
    c->r[2] = (uint32_t)(int16_t)c->mem_r16((c->r[6] + kNodeAngleSel));
    { int _t = (c->r[2] != c->r[5]);  if (_t) goto L_801311B8; }
    c->r[2] = (uint32_t)c->mem_r16((c->r[6] + kNodeAngleParam));
    c->r[2] = c->r[2] & 4095u; goto L_801311D0;
  L_801311B8:;
    c->r[2] = c->r[4] << 16;
    c->r[2] = (uint32_t)((int32_t)c->r[2] >> 14);
    c->r[2] = c->r[6] + c->r[2];
    c->r[2] = c->mem_r32((c->r[2] + kChildTableOff));
    c->r[2] = (uint32_t)c->mem_r16((c->r[2] + kChildAccum));
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
    c->r[2] = (uint32_t)c->mem_r16((c->r[6] + kNodeConfig));
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
    c->r[2] = c->mem_r32((c->r[2] + kChildTableOff));
    c->r[3] = c->r[6] + c->r[3];
    c->mem_w8((c->r[2] + kChildStateFlags), (uint8_t)c->r[4]);
    c->r[2] = c->mem_r32((c->r[3] + kChildTableOff));
    c->mem_w8((c->r[2] + kChildStateFlags), (uint8_t)c->r[4]);
    c->r[2] = c->r[7] << 2;
    c->r[2] = c->r[6] + c->r[2];
    c->r[3] = c->mem_r32((c->r[2] + kChildTableOff));
    c->r[2] = (uint32_t)c->mem_r8((c->r[3] + kChildStateFlags));
    c->r[2] = c->r[2] | 2u;
    c->mem_w8((c->r[3] + kChildStateFlags), (uint8_t)c->r[2]);
    c->r[2] = (uint32_t)c->mem_r16((c->r[6] + kNodePendingCmd));
    c->r[3] = c->r[2] & 3u;
    c->r[2] = c->r[0] + (uint32_t)2;
    { int _t = (c->r[3] == c->r[2]); c->r[2] = c->r[0] + (uint32_t)3; if (_t) goto L_80131298; }
    { int _t = (c->r[3] == c->r[2]); c->r[2] = c->r[0] + (uint32_t)8; if (_t) goto L_8013129C; }
     goto L_801312A0;
  L_80131298:;
    c->r[2] = c->r[0] + (uint32_t)4;
  L_8013129C:;
    c->mem_w16((c->r[6] + kNodeArmDuration), (uint16_t)c->r[2]);
  L_801312A0:;
    c->r[2] = (uint32_t)c->mem_r16((c->r[6] + kNodePendingCmd));
    c->r[2] = c->r[2] & 4u;
    { int _t = (c->r[2] == c->r[0]);  if (_t) goto L_801312C4; }
    c->r[2] = (uint32_t)c->mem_r16((c->r[6] + kNodeArmDuration));
    c->r[2] = c->r[2] + (uint32_t)2;
    c->mem_w16((c->r[6] + kNodeArmDuration), (uint16_t)c->r[2]);
  L_801312C4:;
     return;
    return;
}

// FUN_8012F494 — the orchestrator's node[5]==0 sub-state tick. 14,833 substrate dispatches per 6000
// replay frames.
//
// REPLACES A DEFECTIVE DRAFT. game/ai/beh_substate_edge_leaves.cpp carried a hand-transliterated
// func_8012F494 with EIGHT defects against the live extent, the whole-file one being that it descends
// sp and writes none of its guest stack spills. This body is port_gen output, so the prologue is
// verbatim and the spills cannot go missing.
// ORACLE: ov_a00_gen_8012F494
void SubstateEdgeLeaves::substate0Tick(Core* c) {
    c->r[29] = c->r[29] + (uint32_t)-24;
    c->mem_w32((c->r[29] + (uint32_t)16), c->r[16]);
    c->r[16] = c->r[4] + c->r[0];
    c->mem_w32((c->r[29] + (uint32_t)20), c->r[31]);
    c->r[2] = (uint32_t)c->mem_r8((c->r[16] + (uint32_t)6));
    { int _t = (c->r[2] != c->r[0]);  if (_t) goto L_8012F4CC; }
    c->r[31] = 0x8012F4BCu;
     ov_a00_func_801314B4(c);
    c->r[2] = (uint32_t)c->mem_r8((c->r[16] + (uint32_t)6));
    c->r[2] = c->r[2] + (uint32_t)1;
    c->mem_w8((c->r[16] + (uint32_t)6), (uint8_t)c->r[2]);
  L_8012F4CC:;
    c->r[2] = (uint32_t)c->mem_r16((c->r[16] + kNodePendingCmd));
    c->r[2] = c->r[2] & 2u;
    { int _t = (c->r[2] != c->r[0]); c->r[3] = c->r[0] + (uint32_t)64; if (_t) goto L_8012F5A4; }
    c->r[2] = (uint32_t)c->mem_r16((c->r[16] + kNodeConfig));
    c->r[2] = c->r[2] & 240u;
    { int _t = (c->r[2] == c->r[3]); c->r[4] = c->r[16] + c->r[0]; if (_t) goto L_8012F584; }
    c->r[31] = 0x8012F4FCu;
    c->r[5] = c->r[0] + (uint32_t)1; ov_a00_func_80130788(c);
    { int _t = (c->r[2] == c->r[0]); c->r[2] = c->r[0] + (uint32_t)1; if (_t) goto L_8012F510; }
    c->mem_w8((c->r[16] + kNodeSubState), (uint8_t)c->r[2]);
    c->mem_w8((c->r[16] + (uint32_t)6), (uint8_t)c->r[0]); goto L_8012F59C;
  L_8012F510:;
    c->r[3] = (uint32_t)c->mem_r8((c->r[16] + kNodeRole));
    c->r[2] = c->r[0] + (uint32_t)2;
    { int _t = (c->r[3] != c->r[2]);  if (_t) goto L_8012F59C; }
    c->r[5] = (uint32_t)c->mem_r16((c->r[16] + (uint32_t)100));
    c->r[3] = (uint32_t)(int16_t)c->mem_r16((c->r[16] + (uint32_t)84));
    c->r[2] = c->r[5] & 4095u;
    { int _t = (c->r[3] == c->r[2]);  if (_t) goto L_8012F59C; }
    c->r[6] = c->mem_r32((c->r[16] + (uint32_t)196));
    c->r[2] = (uint32_t)c->mem_r16((c->r[6] + (uint32_t)8));
    c->r[3] = c->r[2] + (uint32_t)-4;
    c->r[2] = c->r[3] << 16;
    c->r[2] = (uint32_t)((int32_t)c->r[2] >> 16);
    c->r[2] = (uint32_t)((int32_t)c->r[2] < 2049);
    { int _t = (c->r[2] != c->r[0]); c->r[4] = c->r[3] + c->r[0]; if (_t) goto L_8012F560; }
    c->r[4] = c->r[3] | 61440u;
  L_8012F560:;
    c->r[3] = c->r[4] << 16;
    c->r[2] = c->r[5] << 16;
    c->r[2] = (uint32_t)((int32_t)c->r[2] < (int32_t)c->r[3]);
    { int _t = (c->r[2] != c->r[0]); c->r[2] = c->r[4] & 4095u; if (_t) goto L_8012F57C; }
    c->r[4] = c->r[5] + c->r[0];
    c->r[2] = c->r[4] & 4095u;
  L_8012F57C:;
    c->mem_w16((c->r[6] + (uint32_t)8), (uint16_t)c->r[2]); goto L_8012F59C;
  L_8012F584:;
    c->r[2] = (uint32_t)c->mem_r16((c->r[16] + (uint32_t)120));
    c->r[2] = c->r[2] & 2u;
    { int _t = (c->r[2] == c->r[0]);  if (_t) goto L_8012F59C; }
    c->mem_w16((c->r[16] + (uint32_t)120), (uint16_t)c->r[0]);
  L_8012F59C:;
    c->r[31] = 0x8012F5A4u;
    c->r[4] = c->r[16] + c->r[0]; ov_a00_func_801308E0(c);
  L_8012F5A4:;
    c->r[31] = c->mem_r32((c->r[29] + (uint32_t)20));
    c->r[16] = c->mem_r32((c->r[29] + (uint32_t)16));
    c->r[29] = c->r[29] + (uint32_t)24; return;
    return;
}

void SubstateEdgeLeaves::registerOverrides(Game*) {
  engine_set_override_a00(0x8012F494u, &SubstateEdgeLeaves::substate0Tick, ov_a00_gen_8012F494);
  engine_set_override_a00(0x80130AC4u, &SubstateEdgeLeaves::visibilityGate,        ov_a00_gen_80130AC4);
  engine_set_override_a00(0x801316CCu, &SubstateEdgeLeaves::tickChildOscillators,  ov_a00_gen_801316CC);
  engine_set_override_a00(0x80131134u, &SubstateEdgeLeaves::armPendingChildPair,   ov_a00_gen_80131134);
}
