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
void ov_a00_func_80125F50(Core*);
void ov_a00_func_80127384(Core*);
void ov_a00_func_8012E8A8(Core*);
void ov_a00_func_801312CC(Core*);
void ov_a00_func_80131578(Core*);
void ov_a00_func_80131600(Core*);
void ov_a00_func_80131768(Core*);
void ov_a00_func_80133444(Core*);
void ov_a00_func_8013892C(Core*);
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

// FUN_0x8012E8A8 — PER-SUB-PART TRANSFORM PROPAGATE. Verified against the body rather than taken
// from the RE spec: it reads the sub-part count at node+8, then walks the pointer table composing a
// rotation for each part — rec_dispatch(0x80085480) is Math::rotmat (libgte RotMatrix), fed the
// part's Euler angles at child+8, with the two scratchpad matrices at 0x1F800000 and 0x1F800020 as
// working space. Each part's sentinel at child+6 selects between a root composition and a
// parent-relative one.
//
// NOTE FOR ANYONE NAMING OFFSETS HERE: the table base ADVANCES each iteration (r16 walks node,
// node+4, ...), so the `+ 192` inside the loop is childTable[i], NOT childTable[0]. It is
// deliberately left as a literal for that reason — kChildTableOff would read as the table base and
// be subtly wrong.
// ORACLE: ov_a00_gen_8012E8A8
void SubstateEdgeLeaves::perChildTransformPropagate(Core* c) {
    c->r[29] = c->r[29] + (uint32_t)-48;
    c->mem_w32((c->r[29] + (uint32_t)20), c->r[17]);
    c->r[17] = c->r[4] + c->r[0];
    c->r[4] = c->r[17] + (uint32_t)84;
    c->r[5] = c->r[17] + (uint32_t)152;
    c->mem_w32((c->r[29] + (uint32_t)44), c->r[31]);
    c->mem_w32((c->r[29] + (uint32_t)40), c->r[22]);
    c->mem_w32((c->r[29] + (uint32_t)36), c->r[21]);
    c->mem_w32((c->r[29] + (uint32_t)32), c->r[20]);
    c->mem_w32((c->r[29] + (uint32_t)28), c->r[19]);
    c->mem_w32((c->r[29] + (uint32_t)24), c->r[18]);
    c->r[31] = 0x8012E8DCu;
    c->mem_w32((c->r[29] + (uint32_t)16), c->r[16]); rec_dispatch(c, 0x80085480u);
    c->r[2] = (uint32_t)c->mem_r8((c->r[17] + kNodePartCount));
    { int _t = (c->r[2] == c->r[0]); c->r[19] = c->r[0] + c->r[0]; if (_t) goto L_8012EB2C; }
    c->r[21] = (uint32_t)8064u << 16;
    c->r[22] = c->r[21] + (uint32_t)0;
    c->r[2] = (uint32_t)8064u << 16;
    c->r[20] = c->r[2] + (uint32_t)32;
    c->r[16] = c->r[17] + c->r[0];
  L_8012E900:;
    c->r[4] = c->mem_r32((c->r[16] + (uint32_t)192));
    c->r[3] = c->r[0] + (uint32_t)-1;
    c->r[2] = (uint32_t)(int16_t)c->mem_r16((c->r[4] + (uint32_t)6));
    c->r[18] = (uint32_t)c->mem_r16((c->r[4] + (uint32_t)6));
    { int _t = (c->r[2] != c->r[3]); c->r[2] = c->r[0] + (uint32_t)2; if (_t) goto L_8012E990; }
    c->r[4] = c->r[4] + (uint32_t)8;
    c->r[31] = 0x8012E924u;
    c->r[5] = c->r[22] + c->r[0]; rec_dispatch(c, 0x80085480u);
    c->r[4] = c->r[17] + (uint32_t)152;
    c->r[6] = c->mem_r32((c->r[16] + (uint32_t)192));
    c->r[5] = c->r[22] + c->r[0];
    c->r[31] = 0x8012E938u;
    c->r[6] = c->r[6] + (uint32_t)24; rec_dispatch(c, 0x80084110u);
    c->r[4] = c->mem_r32((c->r[16] + (uint32_t)192));
    c->r[31] = 0x8012E944u;
    c->r[5] = c->r[4] + (uint32_t)44; rec_dispatch(c, 0x80084220u);
    c->r[4] = c->mem_r32((c->r[16] + (uint32_t)192));
    c->r[3] = (uint32_t)(int16_t)c->mem_r16((c->r[17] + (uint32_t)46));
    c->r[2] = c->mem_r32((c->r[4] + (uint32_t)44));
    c->r[2] = c->r[2] + c->r[3];
    c->mem_w32((c->r[4] + (uint32_t)44), c->r[2]);
    c->r[4] = c->mem_r32((c->r[16] + (uint32_t)192));
    c->r[3] = (uint32_t)(int16_t)c->mem_r16((c->r[17] + (uint32_t)50));
    c->r[2] = c->mem_r32((c->r[4] + (uint32_t)48));
    c->r[2] = c->r[2] + c->r[3];
    c->mem_w32((c->r[4] + (uint32_t)48), c->r[2]);
    c->r[4] = c->mem_r32((c->r[16] + (uint32_t)192));
    c->r[3] = (uint32_t)(int16_t)c->mem_r16((c->r[17] + (uint32_t)54));
    c->r[2] = c->mem_r32((c->r[4] + (uint32_t)52));
    c->r[2] = c->r[2] + c->r[3];
    c->mem_w32((c->r[4] + (uint32_t)52), c->r[2]); goto L_8012EB18;
  L_8012E990:;
    { int _t = (c->r[19] == c->r[2]); c->r[2] = c->r[0] + (uint32_t)3; if (_t) goto L_8012E9B4; }
    { int _t = (c->r[19] != c->r[2]);  if (_t) goto L_8012EA44; }
    c->r[2] = (uint32_t)c->mem_r16((c->r[17] + kNodeConfig));
    c->r[2] = c->r[2] & 2u;
    { int _t = (c->r[2] == c->r[0]);  if (_t) goto L_8012EA44; }
  L_8012E9B4:;
    c->r[4] = c->r[4] + (uint32_t)8;
    c->r[31] = 0x8012E9C0u;
    c->r[5] = c->r[21] + (uint32_t)0; rec_dispatch(c, 0x80085480u);
    c->r[31] = 0x8012E9C8u;
    c->r[4] = c->r[20] + c->r[0]; rec_dispatch(c, 0x80051794u);
    c->r[2] = (uint32_t)c->mem_r16((c->r[17] + kNodeConfig));
    c->r[2] = c->r[2] & 1u;
    { int _t = (c->r[2] == c->r[0]);  if (_t) goto L_8012E9E8; }
    c->r[2] = c->mem_r32((c->r[17] + (uint32_t)196));
     goto L_8012E9EC;
  L_8012E9E8:;
    c->r[2] = c->mem_r32((c->r[17] + kChildTableOff));
  L_8012E9EC:;
    c->r[4] = (uint32_t)(int16_t)c->mem_r16((c->r[2] + (uint32_t)12));
    c->r[31] = 0x8012E9FCu;
    c->r[5] = c->r[20] + c->r[0]; rec_dispatch(c, 0x80085050u);
    c->r[4] = (uint32_t)(int16_t)c->mem_r16((c->r[17] + (uint32_t)86));
    c->r[31] = 0x8012EA08u;
    c->r[5] = c->r[20] + c->r[0]; rec_dispatch(c, 0x80084EB0u);
    c->r[4] = c->r[20] + c->r[0];
    c->r[6] = c->mem_r32((c->r[16] + (uint32_t)192));
    c->r[5] = c->r[21] + (uint32_t)0;
    c->r[31] = 0x8012EA1Cu;
    c->r[6] = c->r[6] + (uint32_t)24; rec_dispatch(c, 0x80084110u);
    c->r[2] = c->r[18] << 16;
    c->r[2] = (uint32_t)((int32_t)c->r[2] >> 14);
    c->r[2] = c->r[17] + c->r[2];
    c->r[4] = c->mem_r32((c->r[2] + (uint32_t)192));
    c->r[5] = c->mem_r32((c->r[16] + (uint32_t)192));
    c->r[4] = c->r[4] + (uint32_t)24;
    c->r[31] = 0x8012EA3Cu;
    c->r[6] = c->r[5] + (uint32_t)44; rec_dispatch(c, 0x80084470u);
    c->r[4] = c->r[18] << 16; goto L_8012EABC;
  L_8012EA44:;
    c->r[2] = (uint32_t)c->mem_r16((c->r[17] + kNodeConfig));
    c->r[2] = c->r[2] & 1u;
    { int _t = (c->r[2] == c->r[0]);  if (_t) goto L_8012EA78; }
    c->r[31] = 0x8012EA60u;
    c->r[4] = c->r[22] + c->r[0]; rec_dispatch(c, 0x80051794u);
    c->r[4] = c->mem_r32((c->r[16] + (uint32_t)192));
    c->r[5] = c->r[22] + c->r[0];
    c->r[31] = 0x8012EA70u;
    c->r[4] = c->r[4] + (uint32_t)8; rec_dispatch(c, 0x80084A80u);
    c->r[5] = c->r[21] + (uint32_t)0; goto L_8012EA8C;
  L_8012EA78:;
    c->r[4] = c->mem_r32((c->r[16] + (uint32_t)192));
    c->r[5] = c->r[21] + (uint32_t)0;
    c->r[31] = 0x8012EA88u;
    c->r[4] = c->r[4] + (uint32_t)8; rec_dispatch(c, 0x80085480u);
    c->r[5] = c->r[21] + (uint32_t)0;
  L_8012EA8C:;
    c->r[2] = c->r[18] << 16;
    c->r[2] = (uint32_t)((int32_t)c->r[2] >> 14);
    c->r[2] = c->r[17] + c->r[2];
    c->r[4] = c->mem_r32((c->r[2] + (uint32_t)192));
    c->r[6] = c->mem_r32((c->r[16] + (uint32_t)192));
    c->r[4] = c->r[4] + (uint32_t)24;
    c->r[31] = 0x8012EAACu;
    c->r[6] = c->r[6] + (uint32_t)24; rec_dispatch(c, 0x80084110u);
    c->r[4] = c->mem_r32((c->r[16] + (uint32_t)192));
    c->r[31] = 0x8012EAB8u;
    c->r[5] = c->r[4] + (uint32_t)44; rec_dispatch(c, 0x80084220u);
    c->r[4] = c->r[18] << 16;
  L_8012EABC:;
    c->r[4] = (uint32_t)((int32_t)c->r[4] >> 14);
    c->r[4] = c->r[17] + c->r[4];
    c->r[5] = c->mem_r32((c->r[16] + (uint32_t)192));
    c->r[3] = c->mem_r32((c->r[4] + (uint32_t)192));
    c->r[2] = c->mem_r32((c->r[5] + (uint32_t)44));
    c->r[3] = c->mem_r32((c->r[3] + (uint32_t)44));
    c->r[2] = c->r[2] + c->r[3];
    c->mem_w32((c->r[5] + (uint32_t)44), c->r[2]);
    c->r[5] = c->mem_r32((c->r[16] + (uint32_t)192));
    c->r[3] = c->mem_r32((c->r[4] + (uint32_t)192));
    c->r[2] = c->mem_r32((c->r[5] + (uint32_t)48));
    c->r[3] = c->mem_r32((c->r[3] + (uint32_t)48));
    c->r[2] = c->r[2] + c->r[3];
    c->mem_w32((c->r[5] + (uint32_t)48), c->r[2]);
    c->r[5] = c->mem_r32((c->r[16] + (uint32_t)192));
    c->r[3] = c->mem_r32((c->r[4] + (uint32_t)192));
    c->r[2] = c->mem_r32((c->r[5] + (uint32_t)52));
    c->r[3] = c->mem_r32((c->r[3] + (uint32_t)52));
    c->r[2] = c->r[2] + c->r[3];
    c->mem_w32((c->r[5] + (uint32_t)52), c->r[2]);
  L_8012EB18:;
    c->r[2] = (uint32_t)c->mem_r8((c->r[17] + kNodePartCount));
    c->r[19] = c->r[19] + (uint32_t)1;
    c->r[2] = (uint32_t)((int32_t)c->r[19] < (int32_t)c->r[2]);
    { int _t = (c->r[2] != c->r[0]); c->r[16] = c->r[16] + (uint32_t)4; if (_t) goto L_8012E900; }
  L_8012EB2C:;
    c->r[31] = c->mem_r32((c->r[29] + (uint32_t)44));
    c->r[22] = c->mem_r32((c->r[29] + (uint32_t)40));
    c->r[21] = c->mem_r32((c->r[29] + (uint32_t)36));
    c->r[20] = c->mem_r32((c->r[29] + (uint32_t)32));
    c->r[19] = c->mem_r32((c->r[29] + (uint32_t)28));
    c->r[18] = c->mem_r32((c->r[29] + (uint32_t)24));
    c->r[17] = c->mem_r32((c->r[29] + (uint32_t)20));
    c->r[16] = c->mem_r32((c->r[29] + (uint32_t)16));
    c->r[29] = c->r[29] + (uint32_t)48; return;
    return;
}

// FUN_0x8012ED84 — STATE 0 INIT. Verified against the body: it seeds the node's own control fields —
// state, sub-state, part count, pending command (three writes), arm duration and angle parameter —
// and calls GraphicsBind::recordAllocBody (0x8007AAE8) TWICE, which is what "builds its sub-parts"
// means concretely: allocating the render records the parts draw through. It also calls
// 0x8004CBD8, which has no owner yet (codemap: ORPHAN leaf_8004CBD8) — so one callee of this
// initialiser is still unexamined, and that is worth knowing before trusting any claim about what
// the finished assembly looks like after init.
// ORACLE: ov_a00_gen_8012ED84
void SubstateEdgeLeaves::stateZeroInit(Core* c) {
    c->r[29] = c->r[29] + (uint32_t)-56;
    c->mem_w32((c->r[29] + (uint32_t)28), c->r[19]);
    c->r[19] = c->r[4] + c->r[0];
    c->mem_w32((c->r[29] + (uint32_t)24), c->r[18]);
    c->r[18] = c->r[0] + c->r[0];
    c->r[2] = (uint32_t)32789u << 16;
    c->r[6] = c->r[2] + (uint32_t)-23744;
    c->r[2] = (uint32_t)32789u << 16;
    c->r[5] = c->r[2] + (uint32_t)-23756;
    c->mem_w32((c->r[29] + (uint32_t)52), c->r[31]);
    c->mem_w32((c->r[29] + (uint32_t)48), c->r[30]);
    c->mem_w32((c->r[29] + (uint32_t)44), c->r[23]);
    c->mem_w32((c->r[29] + (uint32_t)40), c->r[22]);
    c->mem_w32((c->r[29] + (uint32_t)36), c->r[21]);
    c->mem_w32((c->r[29] + (uint32_t)32), c->r[20]);
    c->mem_w32((c->r[29] + (uint32_t)20), c->r[17]);
    c->mem_w32((c->r[29] + (uint32_t)16), c->r[16]);
  L_8012EDC8:;
    c->r[2] = (uint32_t)c->mem_r8((c->r[19] + kNodeRole));
    c->r[2] = c->r[2] + c->r[5];
    c->r[3] = (uint32_t)c->mem_r8((c->r[2] + (uint32_t)0));
    c->r[2] = c->r[3] << 2;
    c->r[2] = c->r[2] + c->r[3];
    c->r[2] = c->r[2] + c->r[18];
    c->r[2] = c->r[2] << 1;
    c->r[2] = c->r[2] + c->r[6];
    c->r[2] = (uint32_t)c->mem_r16((c->r[2] + (uint32_t)0));
    c->r[18] = c->r[18] + (uint32_t)1;
    c->mem_w16((c->r[4] + (uint32_t)96), (uint16_t)c->r[2]);
    c->r[2] = (uint32_t)((int32_t)c->r[18] < 5);
    { int _t = (c->r[2] != c->r[0]); c->r[4] = c->r[4] + (uint32_t)2; if (_t) goto L_8012EDC8; }
    c->r[2] = (uint32_t)c->mem_r8((c->r[19] + kNodeRole));
    { int _t = (c->r[2] == c->r[0]); c->r[2] = c->r[0] + (uint32_t)-170; if (_t) goto L_8012EE20; }
    c->mem_w16((c->r[19] + (uint32_t)112), (uint16_t)c->r[0]); goto L_8012EE24;
  L_8012EE20:;
    c->mem_w16((c->r[19] + (uint32_t)112), (uint16_t)c->r[2]);
  L_8012EE24:;
    c->r[2] = (uint32_t)c->mem_r8((c->r[19] + kNodeState));
    c->r[3] = c->r[0] + (uint32_t)1;
    c->mem_w8((c->r[19] + (uint32_t)0), (uint8_t)c->r[3]);
    c->r[3] = (uint32_t)c->mem_r16((c->r[19] + kNodeConfig));
    c->r[2] = c->r[2] + (uint32_t)1;
    c->mem_w8((c->r[19] + kNodeState), (uint8_t)c->r[2]);
    c->r[2] = (uint32_t)32783u << 16;
    c->r[21] = c->mem_r32((c->r[2] + (uint32_t)-12372));
    c->r[2] = c->r[0] + (uint32_t)4;
    c->r[4] = c->r[3] & 1u;
    c->mem_w8((c->r[19] + (uint32_t)13), (uint8_t)c->r[2]);
    c->mem_w8((c->r[19] + (uint32_t)11), (uint8_t)c->r[0]);
    c->mem_w8((c->r[19] + (uint32_t)9), (uint8_t)c->r[0]);
    { int _t = (c->r[4] == c->r[0]); c->r[22] = c->r[21] + c->r[2]; if (_t) goto L_8012F06C; }
    c->r[2] = c->r[0] + (uint32_t)1;
    { int _t = (c->r[4] != c->r[2]); c->r[2] = c->r[3] & 2u; if (_t) goto L_8012F238; }
    { int _t = (c->r[2] != c->r[0]); c->r[2] = c->r[0] + (uint32_t)12; if (_t) goto L_8012EE78; }
    c->r[2] = c->r[0] + (uint32_t)7;
  L_8012EE78:;
    c->mem_w8((c->r[19] + kNodePartCount), (uint8_t)c->r[2]);
    c->r[20] = c->r[0] + c->r[0];
    c->r[2] = (uint32_t)c->mem_r8((c->r[19] + kNodePartCount));
    { int _t = (c->r[2] == c->r[0]); c->r[18] = c->r[20] + c->r[0]; if (_t) goto L_8012F238; }
    c->r[23] = c->r[0] + (uint32_t)3;
    c->r[2] = (uint32_t)32789u << 16;
    c->r[30] = c->r[2] + (uint32_t)-23948;
    c->r[16] = c->r[19] + c->r[0];
    c->r[17] = c->r[30] + c->r[0];
  L_8012EEA4:;
    c->r[31] = 0x8012EEACu;
     rec_dispatch(c, 0x8007AAE8u);
    c->r[3] = c->r[2] + c->r[0];
    { int _t = (c->r[3] == c->r[0]); c->r[2] = c->r[0] + (uint32_t)3; if (_t) goto L_8012F414; }
    c->r[2] = (uint32_t)c->mem_r16((c->r[19] + kNodeConfig));
    c->r[2] = c->r[2] & 2u;
    { int _t = (c->r[2] != c->r[0]);  if (_t) goto L_8012EEDC; }
    { int _t = (c->r[18] != c->r[23]);  if (_t) goto L_8012EEDC; }
    c->r[17] = c->r[17] + (uint32_t)10;
    c->r[20] = c->r[20] + (uint32_t)1;
  L_8012EEDC:;
    c->mem_w32((c->r[16] + (uint32_t)192), c->r[3]);
    c->r[2] = (uint32_t)c->mem_r16((c->r[17] + (uint32_t)0));
    c->mem_w16((c->r[3] + (uint32_t)6), (uint16_t)c->r[2]);
    c->r[3] = c->mem_r32((c->r[16] + (uint32_t)192));
    c->r[2] = (uint32_t)c->mem_r16((c->r[17] + (uint32_t)2));
    c->mem_w16((c->r[3] + (uint32_t)0), (uint16_t)c->r[2]);
    c->r[3] = c->mem_r32((c->r[16] + (uint32_t)192));
    c->r[2] = (uint32_t)c->mem_r16((c->r[17] + (uint32_t)4));
    c->mem_w16((c->r[3] + (uint32_t)2), (uint16_t)c->r[2]);
    c->r[3] = c->mem_r32((c->r[16] + (uint32_t)192));
    c->r[2] = (uint32_t)c->mem_r16((c->r[17] + (uint32_t)6));
    c->mem_w16((c->r[3] + (uint32_t)4), (uint16_t)c->r[2]);
    c->r[2] = c->mem_r32((c->r[16] + (uint32_t)192));
    c->mem_w16((c->r[2] + (uint32_t)8), (uint16_t)c->r[0]);
    c->r[2] = c->mem_r32((c->r[16] + (uint32_t)192));
    c->mem_w16((c->r[2] + (uint32_t)10), (uint16_t)c->r[0]);
    c->r[2] = c->mem_r32((c->r[16] + (uint32_t)192));
    c->mem_w16((c->r[2] + (uint32_t)12), (uint16_t)c->r[0]);
    c->r[2] = c->mem_r32((c->r[16] + (uint32_t)192));
    c->mem_w8((c->r[2] + (uint32_t)62), (uint8_t)c->r[0]);
    c->r[2] = c->mem_r32((c->r[16] + (uint32_t)192));
    c->mem_w8((c->r[2] + (uint32_t)63), (uint8_t)c->r[0]);
    c->r[2] = c->r[0] + (uint32_t)1;
    { int _t = (c->r[20] == c->r[2]); c->r[7] = (uint32_t)32789u << 16; if (_t) goto L_8012EFBC; }
    c->r[2] = (uint32_t)((int32_t)c->r[20] < 2);
    { int _t = (c->r[2] == c->r[0]);  if (_t) goto L_8012EF80; }
    { int _t = (c->r[20] == c->r[0]);  if (_t) goto L_8012EF9C; }
     goto L_8012F01C;
  L_8012EF80:;
    c->r[2] = c->r[0] + (uint32_t)2;
    { int _t = (c->r[20] == c->r[2]);  if (_t) goto L_8012EFEC; }
    { int _t = (c->r[20] == c->r[23]); c->r[2] = c->r[0] + (uint32_t)2048; if (_t) goto L_8012EFE0; }
     goto L_8012F01C;
  L_8012EF9C:;
    c->r[2] = (uint32_t)c->mem_r8((c->r[19] + kNodeConfig));
    c->r[3] = (uint32_t)(int16_t)c->mem_r16((c->r[30] + (uint32_t)8));
    c->r[2] = c->r[2] >> 4;
    c->r[2] = c->r[2] << 2;
    c->r[3] = c->r[3] << 2;
    c->r[3] = c->r[3] + c->r[22];
    c->r[2] = c->r[2] + c->r[3]; goto L_8012F02C;
  L_8012EFBC:;
    c->r[7] = c->r[7] + (uint32_t)-23938;
    c->r[2] = (uint32_t)c->mem_r16((c->r[19] + kNodeConfig));
    c->r[3] = (uint32_t)(int16_t)c->mem_r16((c->r[7] + (uint32_t)8));
    c->r[2] = c->r[2] & 3840u;
    c->r[2] = c->r[2] >> 6;
    c->r[3] = c->r[3] << 2;
    c->r[3] = c->r[3] + c->r[22];
    c->r[2] = c->r[2] + c->r[3]; goto L_8012F02C;
  L_8012EFE0:;
    c->r[3] = c->mem_r32((c->r[16] + (uint32_t)192));
    c->mem_w16((c->r[3] + (uint32_t)10), (uint16_t)c->r[2]);
  L_8012EFEC:;
    c->r[2] = c->mem_r32((c->r[16] + (uint32_t)192));
    c->mem_w8((c->r[2] + (uint32_t)63), (uint8_t)c->r[23]);
    c->r[3] = (uint32_t)c->mem_r16((c->r[19] + kNodeConfig));
    c->r[2] = (uint32_t)(int16_t)c->mem_r16((c->r[17] + (uint32_t)8));
    c->r[3] = c->r[3] & 4u;
    c->r[2] = c->r[2] << 2;
    c->r[2] = c->r[2] + c->r[22];
    c->r[3] = c->r[3] + c->r[2];
    c->r[2] = c->mem_r32((c->r[3] + (uint32_t)0));
     goto L_8012F030;
  L_8012F01C:;
    c->r[2] = (uint32_t)(int16_t)c->mem_r16((c->r[17] + (uint32_t)8));
    c->r[2] = c->r[2] << 2;
    c->r[2] = c->r[2] + c->r[22];
  L_8012F02C:;
    c->r[2] = c->mem_r32((c->r[2] + (uint32_t)0));
  L_8012F030:;
    c->r[3] = c->mem_r32((c->r[16] + (uint32_t)192));
    c->r[2] = c->r[21] + c->r[2];
    c->mem_w32((c->r[3] + (uint32_t)64), c->r[2]);
    c->r[16] = c->r[16] + (uint32_t)4;
    c->r[18] = c->r[18] + (uint32_t)1;
    c->r[17] = c->r[17] + (uint32_t)10;
    c->r[20] = c->r[20] + (uint32_t)1;
    c->r[2] = (uint32_t)c->mem_r8((c->r[19] + (uint32_t)9));
    c->r[3] = (uint32_t)c->mem_r8((c->r[19] + kNodePartCount));
    c->r[2] = c->r[2] + (uint32_t)1;
    c->r[3] = (uint32_t)((int32_t)c->r[18] < (int32_t)c->r[3]);
    { int _t = (c->r[3] != c->r[0]); c->mem_w8((c->r[19] + (uint32_t)9), (uint8_t)c->r[2]); if (_t) goto L_8012EEA4; }
     goto L_8012F238;
  L_8012F06C:;
    c->r[2] = c->r[3] & 4u;
    { int _t = (c->r[2] != c->r[0]); c->r[2] = c->r[0] + (uint32_t)7; if (_t) goto L_8012F07C; }
    c->r[2] = c->r[0] + (uint32_t)3;
  L_8012F07C:;
    c->mem_w8((c->r[19] + kNodePartCount), (uint8_t)c->r[2]);
    c->r[2] = (uint32_t)c->mem_r8((c->r[19] + kNodePartCount));
    { int _t = (c->r[2] == c->r[0]); c->r[18] = c->r[0] + c->r[0]; if (_t) goto L_8012F238; }
    c->r[2] = (uint32_t)32789u << 16;
    c->r[20] = c->r[2] + (uint32_t)-23828;
    c->r[23] = c->r[20] + (uint32_t)20;
    c->r[17] = c->r[20] + c->r[0];
    c->r[16] = c->r[19] + c->r[0];
  L_8012F0A4:;
    c->r[31] = 0x8012F0ACu;
     rec_dispatch(c, 0x8007AAE8u);
    c->r[3] = c->r[2] + c->r[0];
    { int _t = (c->r[3] == c->r[0]); c->r[2] = c->r[0] + (uint32_t)3; if (_t) goto L_8012F414; }
    c->mem_w32((c->r[16] + (uint32_t)192), c->r[3]);
    c->r[2] = (uint32_t)c->mem_r16((c->r[17] + (uint32_t)0));
    c->mem_w16((c->r[3] + (uint32_t)6), (uint16_t)c->r[2]);
    c->r[3] = c->mem_r32((c->r[16] + (uint32_t)192));
    c->r[2] = (uint32_t)c->mem_r16((c->r[17] + (uint32_t)2));
    c->mem_w16((c->r[3] + (uint32_t)0), (uint16_t)c->r[2]);
    c->r[3] = c->mem_r32((c->r[16] + (uint32_t)192));
    c->r[2] = (uint32_t)c->mem_r16((c->r[17] + (uint32_t)4));
    c->mem_w16((c->r[3] + (uint32_t)2), (uint16_t)c->r[2]);
    c->r[3] = c->mem_r32((c->r[16] + (uint32_t)192));
    c->r[2] = (uint32_t)c->mem_r16((c->r[17] + (uint32_t)6));
    c->mem_w16((c->r[3] + (uint32_t)4), (uint16_t)c->r[2]);
    c->r[2] = c->mem_r32((c->r[16] + (uint32_t)192));
    c->mem_w16((c->r[2] + (uint32_t)8), (uint16_t)c->r[0]);
    c->r[2] = c->mem_r32((c->r[16] + (uint32_t)192));
    c->mem_w16((c->r[2] + (uint32_t)10), (uint16_t)c->r[0]);
    c->r[2] = c->mem_r32((c->r[16] + (uint32_t)192));
    c->mem_w16((c->r[2] + (uint32_t)12), (uint16_t)c->r[0]);
    c->r[2] = c->mem_r32((c->r[16] + (uint32_t)192));
    c->mem_w8((c->r[2] + (uint32_t)62), (uint8_t)c->r[0]);
    c->r[2] = c->mem_r32((c->r[16] + (uint32_t)192));
    c->mem_w8((c->r[2] + (uint32_t)63), (uint8_t)c->r[0]);
    c->r[2] = c->r[0] + (uint32_t)1;
    { int _t = (c->r[18] == c->r[2]); c->r[2] = (uint32_t)((int32_t)c->r[18] < 2); if (_t) goto L_8012F194; }
    { int _t = (c->r[2] == c->r[0]); c->r[2] = c->r[0] + (uint32_t)2; if (_t) goto L_8012F158; }
    { int _t = (c->r[18] == c->r[0]);  if (_t) goto L_8012F168; }
     goto L_8012F1F4;
  L_8012F158:;
    { int _t = (c->r[18] == c->r[2]); c->r[4] = c->r[19] + (uint32_t)8; if (_t) goto L_8012F1C0; }
     goto L_8012F1F4;
  L_8012F168:;
    c->r[2] = (uint32_t)c->mem_r8((c->r[19] + kNodeConfig));
    c->r[3] = (uint32_t)(int16_t)c->mem_r16((c->r[20] + (uint32_t)8));
    c->r[2] = c->r[2] >> 4;
    c->r[2] = c->r[2] << 2;
    c->r[3] = c->r[3] << 2;
    c->r[3] = c->r[3] + c->r[22];
    c->r[2] = c->r[2] + c->r[3];
    c->r[2] = c->mem_r32((c->r[2] + (uint32_t)0));
    c->r[3] = c->mem_r32((c->r[19] + kChildTableOff));
    c->r[2] = c->r[21] + c->r[2]; goto L_8012F210;
  L_8012F194:;
    c->r[2] = (uint32_t)c->mem_r16((c->r[19] + kNodeConfig));
    c->r[3] = (uint32_t)(int16_t)c->mem_r16((c->r[20] + (uint32_t)18));
    c->r[2] = c->r[2] & 3840u;
    c->r[2] = c->r[2] >> 6;
    c->r[3] = c->r[3] << 2;
    c->r[3] = c->r[3] + c->r[22];
    c->r[2] = c->r[2] + c->r[3];
    c->r[2] = c->mem_r32((c->r[2] + (uint32_t)0));
    c->r[3] = c->mem_r32((c->r[19] + (uint32_t)196));
    c->r[2] = c->r[21] + c->r[2]; goto L_8012F210;
  L_8012F1C0:;
    c->r[3] = c->mem_r32((c->r[4] + (uint32_t)192));
    c->r[2] = c->r[0] + (uint32_t)3;
    c->mem_w8((c->r[3] + (uint32_t)63), (uint8_t)c->r[2]);
    c->r[3] = (uint32_t)c->mem_r16((c->r[19] + kNodeConfig));
    c->r[2] = (uint32_t)(int16_t)c->mem_r16((c->r[23] + (uint32_t)8));
    c->r[3] = c->r[3] & 4u;
    c->r[2] = c->r[2] << 2;
    c->r[2] = c->r[2] + c->r[22];
    c->r[3] = c->r[3] + c->r[2];
    c->r[2] = c->mem_r32((c->r[3] + (uint32_t)0));
    c->r[3] = c->mem_r32((c->r[4] + (uint32_t)192));
    c->r[2] = c->r[21] + c->r[2]; goto L_8012F210;
  L_8012F1F4:;
    c->r[2] = (uint32_t)(int16_t)c->mem_r16((c->r[17] + (uint32_t)8));
    c->r[2] = c->r[2] << 2;
    c->r[2] = c->r[2] + c->r[22];
    c->r[2] = c->mem_r32((c->r[2] + (uint32_t)0));
    c->r[3] = c->mem_r32((c->r[16] + (uint32_t)192));
    c->r[2] = c->r[21] + c->r[2];
  L_8012F210:;
    c->mem_w32((c->r[3] + (uint32_t)64), c->r[2]);
    c->r[17] = c->r[17] + (uint32_t)10;
    c->r[16] = c->r[16] + (uint32_t)4;
    c->r[18] = c->r[18] + (uint32_t)1;
    c->r[2] = (uint32_t)c->mem_r8((c->r[19] + (uint32_t)9));
    c->r[3] = (uint32_t)c->mem_r8((c->r[19] + kNodePartCount));
    c->r[2] = c->r[2] + (uint32_t)1;
    c->r[3] = (uint32_t)((int32_t)c->r[18] < (int32_t)c->r[3]);
    { int _t = (c->r[3] != c->r[0]); c->mem_w8((c->r[19] + (uint32_t)9), (uint8_t)c->r[2]); if (_t) goto L_8012F0A4; }
  L_8012F238:;
    c->r[3] = (uint32_t)c->mem_r16((c->r[19] + kNodeConfig));
    c->r[2] = c->r[0] + (uint32_t)100;
    c->mem_w16((c->r[19] + (uint32_t)128), (uint16_t)c->r[2]);
    c->r[2] = c->r[0] + (uint32_t)200;
    c->mem_w16((c->r[19] + (uint32_t)130), (uint16_t)c->r[2]);
    c->r[2] = c->r[0] + (uint32_t)125;
    c->mem_w16((c->r[19] + (uint32_t)132), (uint16_t)c->r[2]);
    c->r[2] = c->r[0] + (uint32_t)250;
    c->mem_w16((c->r[19] + (uint32_t)134), (uint16_t)c->r[2]);
    c->r[2] = c->r[3] & 8u;
    { int _t = (c->r[2] == c->r[0]); c->r[2] = c->r[3] & 1u; if (_t) goto L_8012F288; }
    { int _t = (c->r[2] == c->r[0]); c->r[2] = c->r[0] + (uint32_t)170; if (_t) goto L_8012F27C; }
    c->r[3] = c->mem_r32((c->r[19] + (uint32_t)196));
    c->mem_w16((c->r[3] + (uint32_t)12), (uint16_t)c->r[2]); goto L_8012F288;
  L_8012F27C:;
    c->r[3] = c->mem_r32((c->r[19] + kChildTableOff));
    c->mem_w16((c->r[3] + (uint32_t)12), (uint16_t)c->r[2]);
  L_8012F288:;
    c->r[3] = (uint32_t)c->mem_r8((c->r[19] + kNodeRole));
    c->r[2] = c->r[0] + (uint32_t)735;
    c->mem_w16((c->r[19] + (uint32_t)106), (uint16_t)c->r[2]);
    c->r[2] = c->r[0] + (uint32_t)3;
    { int _t = (c->r[3] == c->r[2]); c->r[2] = (uint32_t)((int32_t)c->r[3] < 4); if (_t) goto L_8012F308; }
    { int _t = (c->r[2] == c->r[0]);  if (_t) goto L_8012F2B8; }
    { int _t = (c->r[3] == c->r[0]); c->r[2] = c->r[0] + (uint32_t)800; if (_t) goto L_8012F2CC; }
     goto L_8012F330;
  L_8012F2B8:;
    c->r[2] = c->r[0] + (uint32_t)11;
    { int _t = (c->r[3] == c->r[2]); c->r[2] = c->r[0] + (uint32_t)734; if (_t) goto L_8012F314; }
     goto L_8012F330;
  L_8012F2CC:;
    c->r[3] = c->mem_r32((c->r[19] + (uint32_t)200));
    c->mem_w16((c->r[19] + (uint32_t)106), (uint16_t)c->r[2]);
    c->r[2] = c->r[0] + (uint32_t)-800;
    c->mem_w16((c->r[3] + (uint32_t)4), (uint16_t)c->r[2]);
    c->r[3] = c->mem_r32((c->r[19] + (uint32_t)204));
    c->r[2] = (uint32_t)c->mem_r16((c->r[19] + (uint32_t)106));
    c->mem_w16((c->r[3] + (uint32_t)4), (uint16_t)c->r[2]);
    c->r[3] = c->mem_r32((c->r[19] + (uint32_t)196));
    c->r[2] = (uint32_t)c->mem_r16((c->r[19] + (uint32_t)112));
    c->mem_w16((c->r[3] + (uint32_t)10), (uint16_t)c->r[2]);
    c->r[3] = c->mem_r32((c->r[19] + (uint32_t)196));
    c->r[2] = c->r[0] + (uint32_t)240; goto L_8012F338;
  L_8012F308:;
    c->r[3] = c->mem_r32((c->r[19] + (uint32_t)196));
    c->r[2] = c->r[0] + (uint32_t)-192; goto L_8012F338;
  L_8012F314:;
    c->r[3] = c->mem_r32((c->r[19] + (uint32_t)200));
    c->mem_w16((c->r[19] + (uint32_t)106), (uint16_t)c->r[2]);
    c->r[2] = c->r[0] + (uint32_t)-734;
    c->mem_w16((c->r[3] + (uint32_t)4), (uint16_t)c->r[2]);
    c->r[2] = c->mem_r32((c->r[19] + (uint32_t)196));
    c->mem_w16((c->r[2] + (uint32_t)8), (uint16_t)c->r[0]); goto L_8012F340;
  L_8012F330:;
    c->r[3] = c->mem_r32((c->r[19] + (uint32_t)196));
    c->r[2] = (uint32_t)c->mem_r16((c->r[19] + (uint32_t)100));
  L_8012F338:;
    c->mem_w16((c->r[3] + (uint32_t)8), (uint16_t)c->r[2]);
  L_8012F340:;
    c->r[3] = c->mem_r32((c->r[19] + (uint32_t)196));
    c->r[2] = (uint32_t)c->mem_r16((c->r[3] + (uint32_t)8));
    c->r[4] = c->r[19] + c->r[0];
    c->r[2] = c->r[2] & 4095u;
    c->r[31] = 0x8012F35Cu;
    c->mem_w16((c->r[3] + (uint32_t)8), (uint16_t)c->r[2]); ov_a00_func_80131600(c);
    c->r[31] = 0x8012F364u;
    c->r[4] = c->r[19] + c->r[0]; ov_a00_func_801314B4(c);
    c->r[31] = 0x8012F36Cu;
    c->r[4] = c->r[19] + c->r[0]; ov_a00_func_8012E8A8(c);
    c->r[4] = c->r[19] + c->r[0];
    c->mem_w16((c->r[19] + kNodeAngleParam), (uint16_t)c->r[0]);
    c->mem_w16((c->r[19] + kNodeArmDuration), (uint16_t)c->r[0]);
    c->mem_w16((c->r[19] + (uint32_t)118), (uint16_t)c->r[0]);
    c->r[31] = 0x8012F384u;
    c->mem_w16((c->r[19] + kNodePendingCmd), (uint16_t)c->r[0]); ov_a00_func_80133444(c);
    c->r[2] = (uint32_t)c->mem_r8((c->r[19] + kNodeRole));
    c->r[2] = (uint32_t)(c->r[2] < (uint32_t)3);
    { int _t = (c->r[2] == c->r[0]);  if (_t) goto L_8012F3F8; }
    c->r[31] = 0x8012F3A0u;
    c->r[4] = c->r[19] + c->r[0]; ov_a00_func_8013892C(c);
    c->r[2] = (uint32_t)c->mem_r8((c->r[19] + kNodeRole));
    c->r[2] = (uint32_t)(c->r[2] < (uint32_t)2);
    { int _t = (c->r[2] == c->r[0]);  if (_t) goto L_8012F3F0; }
    c->r[31] = 0x8012F3BCu;
    c->r[4] = c->r[19] + c->r[0]; ov_a00_func_80125F50(c);
    c->r[18] = c->r[2] & 3u;
    { int _t = (c->r[18] == c->r[0]); c->mem_w16((c->r[19] + kNodePendingCmd), (uint16_t)c->r[2]); if (_t) goto L_8012F3F0; }
    c->r[2] = c->r[2] << 4;
    c->mem_w16((c->r[19] + kNodePendingCmd), (uint16_t)c->r[2]);
    c->r[2] = c->r[18] << 2;
    c->r[2] = c->r[19] + c->r[2];
    c->r[3] = c->mem_r32((c->r[2] + (uint32_t)192));
    c->r[2] = (uint32_t)c->mem_r8((c->r[3] + (uint32_t)62));
    c->r[2] = c->r[2] | 3u;
    c->mem_w8((c->r[3] + (uint32_t)62), (uint8_t)c->r[2]);
  L_8012F3F0:;
    c->r[31] = 0x8012F3F8u;
    c->r[4] = c->r[19] + c->r[0]; ov_a00_func_801312CC(c);
  L_8012F3F8:;
    c->r[3] = (uint32_t)c->mem_r8((c->r[19] + kNodeRole));
    { int _t = (c->r[3] != c->r[0]); c->r[2] = c->r[0] + (uint32_t)1; if (_t) goto L_8012F41C; }
    c->r[4] = c->r[19] + c->r[0];
    c->r[5] = c->r[0] + c->r[0]; goto L_8012F42C;
  L_8012F414:;
    c->mem_w8((c->r[19] + kNodeState), (uint8_t)c->r[2]); goto L_8012F464;
  L_8012F41C:;
    { int _t = (c->r[3] != c->r[2]); c->r[2] = (uint32_t)32780u << 16; if (_t) goto L_8012F438; }
    c->r[4] = c->r[19] + c->r[0];
    c->r[5] = c->r[0] + (uint32_t)6;
  L_8012F42C:;
    c->r[31] = 0x8012F434u;
     rec_dispatch(c, 0x8004CBD8u);
    c->r[2] = (uint32_t)32780u << 16;
  L_8012F438:;
    c->r[3] = (uint32_t)c->mem_r8((c->r[2] + (uint32_t)-1892));
    c->r[2] = c->r[0] + (uint32_t)2;
    { int _t = (c->r[3] != c->r[2]);  if (_t) goto L_8012F45C; }
    c->r[2] = (uint32_t)c->mem_r8((c->r[19] + kNodeRole));
    { int _t = (c->r[2] != c->r[0]); c->r[2] = c->r[0] + (uint32_t)5; if (_t) goto L_8012F45C; }
    c->mem_w8((c->r[19] + kNodeSubState), (uint8_t)c->r[2]);
  L_8012F45C:;
    c->mem_w8((c->r[19] + (uint32_t)41), (uint8_t)c->r[0]);
    c->mem_w8((c->r[19] + (uint32_t)43), (uint8_t)c->r[0]);
  L_8012F464:;
    c->r[31] = c->mem_r32((c->r[29] + (uint32_t)52));
    c->r[30] = c->mem_r32((c->r[29] + (uint32_t)48));
    c->r[23] = c->mem_r32((c->r[29] + (uint32_t)44));
    c->r[22] = c->mem_r32((c->r[29] + (uint32_t)40));
    c->r[21] = c->mem_r32((c->r[29] + (uint32_t)36));
    c->r[20] = c->mem_r32((c->r[29] + (uint32_t)32));
    c->r[19] = c->mem_r32((c->r[29] + (uint32_t)28));
    c->r[18] = c->mem_r32((c->r[29] + (uint32_t)24));
    c->r[17] = c->mem_r32((c->r[29] + (uint32_t)20));
    c->r[16] = c->mem_r32((c->r[29] + (uint32_t)16));
    c->r[29] = c->r[29] + (uint32_t)56; return;
    return;
}

// FUN_0x8012F5B4 — the node[5]==1 sub-state tick.
// ORACLE: ov_a00_gen_8012F5B4
void SubstateEdgeLeaves::substate1Tick(Core* c) {
    c->r[29] = c->r[29] + (uint32_t)-48;
    c->mem_w32((c->r[29] + (uint32_t)20), c->r[17]);
    c->r[17] = c->r[4] + c->r[0];
    c->mem_w32((c->r[29] + (uint32_t)36), c->r[21]);
    c->r[21] = c->r[0] + c->r[0];
    c->r[2] = (uint32_t)32782u << 16;
    c->r[3] = (uint32_t)c->mem_r8((c->r[2] + (uint32_t)32481));
    c->r[2] = c->r[0] + (uint32_t)49;
    c->mem_w32((c->r[29] + (uint32_t)40), c->r[31]);
    c->mem_w32((c->r[29] + (uint32_t)32), c->r[20]);
    c->mem_w32((c->r[29] + (uint32_t)28), c->r[19]);
    c->mem_w32((c->r[29] + (uint32_t)24), c->r[18]);
    { int _t = (c->r[3] == c->r[2]); c->mem_w32((c->r[29] + (uint32_t)16), c->r[16]); if (_t) goto L_8012FD64; }
    c->r[2] = c->r[0] + (uint32_t)65;
    { int _t = (c->r[3] == c->r[2]); c->r[2] = (uint32_t)32780u << 16; if (_t) goto L_8012FD64; }
    c->r[3] = c->r[2] + (uint32_t)-2040;
    c->r[2] = (uint32_t)c->mem_r8((c->r[3] + (uint32_t)57));
    { int _t = (c->r[2] != c->r[0]);  if (_t) goto L_8012FD64; }
    c->r[2] = (uint32_t)c->mem_r8((c->r[3] + (uint32_t)1));
    { int _t = (c->r[2] != c->r[0]);  if (_t) goto L_8012FD64; }
    c->r[2] = (uint32_t)c->mem_r16((c->r[17] + kNodePendingCmd));
    c->r[2] = c->r[2] & 2u;
    { int _t = (c->r[2] != c->r[0]);  if (_t) goto L_8012FD64; }
    c->r[2] = c->mem_r32((c->r[17] + (uint32_t)196));
    c->r[2] = (uint32_t)(int16_t)c->mem_r16((c->r[2] + (uint32_t)8));
    c->r[19] = c->r[2] + c->r[0];
    c->r[2] = (uint32_t)((int32_t)c->r[2] < 2049);
    { int _t = (c->r[2] != c->r[0]);  if (_t) goto L_8012F654; }
    c->r[19] = c->r[19] | 61440u;
  L_8012F654:;
    c->r[3] = (uint32_t)c->mem_r8((c->r[17] + (uint32_t)6));
    c->r[2] = (uint32_t)(c->r[3] < (uint32_t)6);
    { int _t = (c->r[2] == c->r[0]); c->r[16] = c->r[19] + c->r[0]; if (_t) goto L_8012FCC0; }
    c->r[2] = (uint32_t)32785u << 16;
    c->r[2] = c->r[2] + (uint32_t)-25084;
    c->r[3] = c->r[3] << 2;
    c->r[3] = c->r[3] + c->r[2];
    c->r[2] = c->mem_r32((c->r[3] + (uint32_t)0));
    {  switch (c->r[2]) { case 0x8012F688u: goto L_8012F688; case 0x8012F7A4u: goto L_8012F7A4; case 0x8012FA80u: goto L_8012FA80; case 0x8012FBECu: goto L_8012FBEC; case 0x8012F774u: goto L_8012F774; case 0x8012FC54u: goto L_8012FC54; default: rec_dispatch(c, c->r[2]); return; } }
  L_8012F688:;
    c->r[2] = (uint32_t)c->mem_r8((c->r[17] + (uint32_t)1));
    { int _t = (c->r[2] == c->r[0]); c->r[2] = (uint32_t)32789u << 16; if (_t) goto L_8012F768; }
    c->r[2] = c->mem_r32((c->r[2] + (uint32_t)-23952));
    { int _t = (c->r[2] != c->r[0]); c->r[2] = c->r[19] << 16; if (_t) goto L_8012F768; }
    c->r[3] = (uint32_t)(int16_t)c->mem_r16((c->r[17] + (uint32_t)100));
    c->r[4] = (uint32_t)((int32_t)c->r[2] >> 16);
    { int _t = (c->r[4] != c->r[3]); c->r[18] = c->r[0] + (uint32_t)1; if (_t) goto L_8012F6EC; }
    c->r[3] = (uint32_t)c->mem_r8((c->r[17] + (uint32_t)41));
    c->r[2] = c->r[3] & 4u;
    { int _t = (c->r[2] == c->r[0]); c->r[2] = c->r[3] & 128u; if (_t) goto L_8012F6DC; }
    c->r[2] = (uint32_t)(int16_t)c->mem_r16((c->r[17] + (uint32_t)68));
    { int _t = ((int32_t)c->r[2] < 0); c->r[2] = c->r[3] & 128u; if (_t) goto L_8012F730; }
  L_8012F6DC:;
    { int _t = (c->r[2] == c->r[0]);  if (_t) goto L_8012F734; }
    c->r[18] = c->r[0] + c->r[0]; goto L_8012F734;
  L_8012F6EC:;
    c->r[2] = (uint32_t)(int16_t)c->mem_r16((c->r[17] + (uint32_t)102));
    { int _t = (c->r[4] != c->r[2]);  if (_t) goto L_8012F734; }
    c->r[3] = (uint32_t)c->mem_r8((c->r[17] + (uint32_t)41));
    c->r[2] = c->r[3] & 4u;
    { int _t = (c->r[2] == c->r[0]); c->r[2] = c->r[3] & 128u; if (_t) goto L_8012F728; }
    c->r[2] = (uint32_t)(int16_t)c->mem_r16((c->r[17] + (uint32_t)68));
    { int _t = ((int32_t)c->r[2] <= 0);  if (_t) goto L_8012F734; }
    c->r[18] = c->r[0] + c->r[0]; goto L_8012F734;
  L_8012F728:;
    { int _t = (c->r[2] != c->r[0]);  if (_t) goto L_8012F734; }
  L_8012F730:;
    c->r[18] = c->r[0] + c->r[0];
  L_8012F734:;
    { int _t = (c->r[18] == c->r[0]); c->r[2] = c->r[19] << 16; if (_t) goto L_8012F768; }
    { int _t = ((int32_t)c->r[2] >= 0); c->r[4] = c->r[0] + (uint32_t)129; if (_t) goto L_8012F750; }
    c->r[5] = c->r[0] + c->r[0];
    c->r[6] = c->r[0] + (uint32_t)42; goto L_8012F758;
  L_8012F750:;
    c->r[5] = c->r[0] + (uint32_t)-10;
    c->r[6] = c->r[0] + (uint32_t)-14;
  L_8012F758:;
    c->r[31] = 0x8012F760u;
     rec_dispatch(c, 0x80074590u);
    c->r[3] = (uint32_t)32789u << 16;
    c->mem_w32((c->r[3] + (uint32_t)-23952), c->r[2]);
  L_8012F768:;
    c->r[2] = (uint32_t)c->mem_r8((c->r[17] + (uint32_t)6));
    c->mem_w16((c->r[17] + (uint32_t)118), (uint16_t)c->r[0]); goto L_8012F9B0;
  L_8012F774:;
    c->r[2] = (uint32_t)c->mem_r16((c->r[17] + (uint32_t)64));
    c->r[2] = c->r[2] + (uint32_t)-1;
    c->mem_w16((c->r[17] + (uint32_t)64), (uint16_t)c->r[2]);
    c->r[2] = c->r[2] << 16;
    { int _t = ((int32_t)c->r[2] >= 0); c->r[4] = c->r[17] + c->r[0]; if (_t) goto L_8012F7A8; }
    c->r[2] = (uint32_t)c->mem_r8((c->r[17] + (uint32_t)6));
    c->r[3] = c->r[0] + (uint32_t)3;
    c->mem_w16((c->r[17] + (uint32_t)64), (uint16_t)c->r[3]);
    c->r[2] = c->r[2] + (uint32_t)1;
    c->mem_w8((c->r[17] + (uint32_t)6), (uint8_t)c->r[2]);
  L_8012F7A4:;
    c->r[4] = c->r[17] + c->r[0];
  L_8012F7A8:;
    c->r[31] = 0x8012F7B0u;
    c->r[5] = c->r[0] + (uint32_t)1; ov_a00_func_80130788(c);
    c->r[18] = c->r[2] + c->r[0];
    { int _t = (c->r[18] == c->r[0]);  if (_t) goto L_8012F7D0; }
    c->r[2] = (uint32_t)c->mem_r16((c->r[17] + (uint32_t)120));
    c->r[2] = c->r[2] & 2u;
    { int _t = (c->r[2] == c->r[0]); c->r[2] = c->r[0] + (uint32_t)1; if (_t) goto L_8012F848; }
  L_8012F7D0:;
    c->r[2] = (uint32_t)c->mem_r16((c->r[17] + (uint32_t)120));
    c->r[2] = c->r[2] & 1u;
    { int _t = (c->r[2] != c->r[0]);  if (_t) goto L_8012F894; }
    c->r[4] = (uint32_t)c->mem_r16((c->r[17] + (uint32_t)72));
    c->r[3] = (uint32_t)c->mem_r16((c->r[17] + (uint32_t)78));
    c->r[4] = c->r[4] + c->r[3];
    c->r[3] = c->r[3] << 16;
    c->r[3] = (uint32_t)((int32_t)c->r[3] >> 16);
    c->r[2] = c->r[4] << 16;
    c->r[2] = (uint32_t)((int32_t)c->r[2] >> 16);
    { int64_t _p = (int64_t)(int32_t)c->r[3] * (int64_t)(int32_t)c->r[2]; c->lo = (uint32_t)_p; c->hi = (uint32_t)((uint64_t)_p >> 32); }
    c->r[7] = c->lo;
    { int _t = ((int32_t)c->r[7] < 0); c->r[20] = c->r[4] + c->r[0]; if (_t) goto L_8012F894; }
    c->r[16] = (uint32_t)32789u << 16;
    c->mem_w8((c->r[17] + kNodeSubState), (uint8_t)c->r[0]);
    c->mem_w8((c->r[17] + (uint32_t)6), (uint8_t)c->r[0]);
    c->r[4] = c->mem_r32((c->r[16] + (uint32_t)-23952));
    c->mem_w16((c->r[17] + (uint32_t)72), (uint16_t)c->r[0]);
    c->mem_w16((c->r[17] + (uint32_t)78), (uint16_t)c->r[0]);
    c->mem_w16((c->r[17] + (uint32_t)120), (uint16_t)c->r[0]);
    { int _t = (c->r[4] == c->r[0]); c->mem_w16((c->r[17] + (uint32_t)118), (uint16_t)c->r[0]); if (_t) goto L_8012FCC0; }
    c->r[31] = 0x8012F840u;
     rec_dispatch(c, 0x80074AF0u);
    c->mem_w32((c->r[16] + (uint32_t)-23952), c->r[0]); goto L_8012FCC0;
  L_8012F848:;
    { int _t = (c->r[18] != c->r[2]); c->r[2] = (uint32_t)32782u << 16; if (_t) goto L_8012F894; }
    c->r[2] = (uint32_t)c->mem_r8((c->r[2] + (uint32_t)32709));
    c->r[2] = c->r[2] & 1u;
    { int _t = (c->r[2] != c->r[0]);  if (_t) goto L_8012F894; }
    c->r[4] = c->r[19] << 16;
    c->r[31] = 0x8012F870u;
    c->r[4] = (uint32_t)((int32_t)c->r[4] >> 16); rec_dispatch(c, 0x80083E80u);
    c->r[3] = c->mem_r32((c->r[17] + (uint32_t)200));
    c->r[3] = (uint32_t)(int16_t)c->mem_r16((c->r[3] + (uint32_t)4));
    c->r[3] = c->r[0] - c->r[3];
    { int64_t _p = (int64_t)(int32_t)c->r[2] * (int64_t)(int32_t)c->r[3]; c->lo = (uint32_t)_p; c->hi = (uint32_t)((uint64_t)_p >> 32); }
    c->r[21] = c->r[0] + (uint32_t)1;
    c->r[7] = c->lo;
    c->r[20] = c->r[7] >> 12;
  L_8012F894:;
    c->r[2] = (uint32_t)c->mem_r16((c->r[17] + (uint32_t)72));
    c->r[3] = (uint32_t)c->mem_r16((c->r[17] + (uint32_t)78));
    c->r[2] = c->r[2] + c->r[3];
    c->mem_w16((c->r[17] + (uint32_t)72), (uint16_t)c->r[2]);
    c->r[2] = c->r[2] << 16;
    c->r[3] = (uint32_t)((int32_t)c->r[2] >> 16);
    c->r[2] = (uint32_t)((int32_t)c->r[3] < 5633);
    { int _t = (c->r[2] != c->r[0]); c->r[2] = (uint32_t)((int32_t)c->r[3] < -5632); if (_t) goto L_8012F8C4; }
    c->r[2] = c->r[0] + (uint32_t)5632; goto L_8012F8CC;
  L_8012F8C4:;
    { int _t = (c->r[2] == c->r[0]); c->r[2] = c->r[0] + (uint32_t)-5632; if (_t) goto L_8012F8D0; }
  L_8012F8CC:;
    c->mem_w16((c->r[17] + (uint32_t)72), (uint16_t)c->r[2]);
  L_8012F8D0:;
    c->r[2] = (uint32_t)c->mem_r16((c->r[17] + (uint32_t)72));
    c->r[5] = (uint32_t)(int16_t)c->mem_r16((c->r[17] + (uint32_t)100));
    c->r[2] = c->r[2] << 16;
    c->r[4] = (uint32_t)((int32_t)c->r[2] >> 24);
    c->r[2] = c->r[19] + c->r[4];
    c->r[19] = c->r[2] + c->r[0];
    c->r[2] = c->r[2] << 16;
    c->r[3] = (uint32_t)((int32_t)c->r[2] >> 16);
    c->r[2] = (uint32_t)((int32_t)c->r[3] < (int32_t)c->r[5]);
    { int _t = (c->r[2] != c->r[0]);  if (_t) goto L_8012F910; }
    c->r[2] = (uint32_t)(int16_t)c->mem_r16((c->r[17] + (uint32_t)102));
    c->r[2] = (uint32_t)((int32_t)c->r[2] < (int32_t)c->r[3]);
    { int _t = (c->r[2] == c->r[0]);  if (_t) goto L_8012FA5C; }
  L_8012F910:;
    c->r[2] = (uint32_t)c->mem_r8((c->r[17] + (uint32_t)1));
    { int _t = (c->r[2] == c->r[0]); c->r[2] = c->r[16] << 16; if (_t) goto L_8012F960; }
    c->r[3] = (uint32_t)((int32_t)c->r[2] >> 16);
    { int _t = (c->r[3] == c->r[5]); c->r[2] = c->r[19] << 16; if (_t) goto L_8012F964; }
    c->r[2] = (uint32_t)(int16_t)c->mem_r16((c->r[17] + (uint32_t)102));
    { int _t = (c->r[3] == c->r[2]); c->r[2] = c->r[19] << 16; if (_t) goto L_8012F964; }
    c->r[16] = (uint32_t)32789u << 16;
    c->r[4] = c->mem_r32((c->r[16] + (uint32_t)-23952));
    c->r[31] = 0x8012F94Cu;
     rec_dispatch(c, 0x80074AF0u);
    c->r[4] = c->r[0] + (uint32_t)130;
    c->r[5] = c->r[0] + c->r[0];
    c->r[6] = c->r[5] + c->r[0];
    c->r[31] = 0x8012F960u;
    c->mem_w32((c->r[16] + (uint32_t)-23952), c->r[0]); rec_dispatch(c, 0x80074590u);
  L_8012F960:;
    c->r[2] = c->r[19] << 16;
  L_8012F964:;
    c->r[2] = (uint32_t)((int32_t)c->r[2] >> 16);
    { int _t = ((int32_t)c->r[2] >= 0);  if (_t) goto L_8012F9D8; }
    c->r[2] = (uint32_t)c->mem_r16((c->r[17] + (uint32_t)120));
    c->r[19] = (uint32_t)c->mem_r16((c->r[17] + (uint32_t)100));
    c->r[2] = c->r[2] & 4u;
    { int _t = (c->r[2] != c->r[0]); c->r[4] = c->r[17] + c->r[0]; if (_t) goto L_8012FCC4; }
    c->r[2] = (uint32_t)(int16_t)c->mem_r16((c->r[17] + (uint32_t)72));
    c->r[2] = (uint32_t)((int32_t)c->r[2] < -2560);
    { int _t = (c->r[2] == c->r[0]); c->r[2] = c->r[0] + (uint32_t)2560; if (_t) goto L_8012F9BC; }
    c->mem_w16((c->r[17] + (uint32_t)72), (uint16_t)c->r[2]);
    c->r[2] = c->r[0] + (uint32_t)-512;
    c->mem_w16((c->r[17] + (uint32_t)78), (uint16_t)c->r[2]);
    c->r[2] = (uint32_t)c->mem_r8((c->r[17] + (uint32_t)6));
    c->r[3] = c->r[0] + (uint32_t)1;
    c->mem_w8((c->r[17] + kNodeModeByte), (uint8_t)c->r[3]);
  L_8012F9B0:;
    c->r[2] = c->r[2] + (uint32_t)1;
    c->mem_w8((c->r[17] + (uint32_t)6), (uint8_t)c->r[2]); goto L_8012FCC0;
  L_8012F9BC:;
    c->mem_w16((c->r[17] + (uint32_t)72), (uint16_t)c->r[0]);
    c->mem_w16((c->r[17] + (uint32_t)78), (uint16_t)c->r[0]);
    { int _t = (c->r[18] != c->r[0]); c->mem_w16((c->r[17] + (uint32_t)118), (uint16_t)c->r[0]); if (_t) goto L_8012FCC0; }
    c->mem_w16((c->r[17] + (uint32_t)120), (uint16_t)c->r[0]);
    c->mem_w8((c->r[17] + kNodeSubState), (uint8_t)c->r[0]); goto L_8012FCBC;
  L_8012F9D8:;
    { int _t = ((int32_t)c->r[2] <= 0); c->r[4] = c->r[17] + c->r[0]; if (_t) goto L_8012FCC4; }
    c->r[2] = (uint32_t)c->mem_r16((c->r[17] + (uint32_t)120));
    c->r[19] = (uint32_t)c->mem_r16((c->r[17] + (uint32_t)102));
    c->r[2] = c->r[2] & 4u;
    { int _t = (c->r[2] != c->r[0]);  if (_t) goto L_8012FCC4; }
    c->r[2] = (uint32_t)(int16_t)c->mem_r16((c->r[17] + (uint32_t)72));
    c->r[2] = (uint32_t)((int32_t)c->r[2] < 2561);
    { int _t = (c->r[2] != c->r[0]); c->r[2] = c->r[0] + (uint32_t)-2560; if (_t) goto L_8012FA40; }
    c->mem_w16((c->r[17] + (uint32_t)72), (uint16_t)c->r[2]);
    c->r[2] = c->r[0] + (uint32_t)512;
    c->mem_w16((c->r[17] + (uint32_t)78), (uint16_t)c->r[2]);
    c->r[2] = (uint32_t)c->mem_r8((c->r[17] + (uint32_t)6));
    c->r[3] = c->r[0] + (uint32_t)1;
    c->mem_w8((c->r[17] + kNodeModeByte), (uint8_t)c->r[3]);
    c->r[3] = (uint32_t)(int16_t)c->mem_r16((c->r[17] + (uint32_t)118));
    c->r[2] = c->r[2] + (uint32_t)1;
    c->mem_w8((c->r[17] + (uint32_t)6), (uint8_t)c->r[2]);
    c->r[2] = c->r[0] + (uint32_t)1;
    { int _t = (c->r[3] != c->r[2]); c->r[2] = c->r[0] + (uint32_t)2; if (_t) goto L_8012FCC4; }
    c->mem_w16((c->r[17] + (uint32_t)118), (uint16_t)c->r[2]); goto L_8012FCC4;
  L_8012FA40:;
    c->mem_w16((c->r[17] + (uint32_t)72), (uint16_t)c->r[0]);
    c->mem_w16((c->r[17] + (uint32_t)78), (uint16_t)c->r[0]);
    { int _t = (c->r[18] != c->r[0]); c->mem_w16((c->r[17] + (uint32_t)118), (uint16_t)c->r[0]); if (_t) goto L_8012FCC0; }
    c->mem_w16((c->r[17] + (uint32_t)120), (uint16_t)c->r[0]);
    c->mem_w8((c->r[17] + kNodeSubState), (uint8_t)c->r[0]); goto L_8012FCBC;
  L_8012FA5C:;
    { int _t = ((int32_t)c->r[4] > 0);  if (_t) goto L_8012FA6C; }
    c->mem_w16((c->r[17] + (uint32_t)118), (uint16_t)c->r[0]); goto L_8012FCC0;
  L_8012FA6C:;
    { int _t = ((int32_t)c->r[3] >= 0); c->r[4] = c->r[17] + c->r[0]; if (_t) goto L_8012FCC4; }
    c->r[2] = c->r[0] + (uint32_t)1;
    c->mem_w16((c->r[17] + (uint32_t)118), (uint16_t)c->r[2]); goto L_8012FCC4;
  L_8012FA80:;
    c->r[4] = c->r[17] + c->r[0];
    c->r[31] = 0x8012FA8Cu;
    c->r[5] = c->r[0] + c->r[0]; ov_a00_func_80130788(c);
    c->r[3] = c->r[0] + (uint32_t)1;
    { int _t = (c->r[2] != c->r[3]); c->r[2] = (uint32_t)32782u << 16; if (_t) goto L_8012FADC; }
    c->r[2] = (uint32_t)c->mem_r8((c->r[2] + (uint32_t)32709));
    c->r[2] = c->r[2] & c->r[3];
    { int _t = (c->r[2] != c->r[0]);  if (_t) goto L_8012FADC; }
    c->r[4] = c->r[19] << 16;
    c->r[31] = 0x8012FAB8u;
    c->r[4] = (uint32_t)((int32_t)c->r[4] >> 16); rec_dispatch(c, 0x80083E80u);
    c->r[3] = c->mem_r32((c->r[17] + (uint32_t)200));
    c->r[3] = (uint32_t)(int16_t)c->mem_r16((c->r[3] + (uint32_t)4));
    c->r[3] = c->r[0] - c->r[3];
    { int64_t _p = (int64_t)(int32_t)c->r[2] * (int64_t)(int32_t)c->r[3]; c->lo = (uint32_t)_p; c->hi = (uint32_t)((uint64_t)_p >> 32); }
    c->r[21] = c->r[0] + (uint32_t)1;
    c->r[7] = c->lo;
    c->r[20] = c->r[7] >> 12;
  L_8012FADC:;
    c->r[2] = (uint32_t)c->mem_r16((c->r[17] + (uint32_t)72));
    c->r[3] = (uint32_t)c->mem_r16((c->r[17] + (uint32_t)78));
    c->r[2] = c->r[2] + c->r[3];
    c->mem_w16((c->r[17] + (uint32_t)72), (uint16_t)c->r[2]);
    c->r[2] = c->r[2] << 16;
    c->r[3] = (uint32_t)((int32_t)c->r[2] >> 16);
    c->r[2] = (uint32_t)((int32_t)c->r[3] < 5633);
    { int _t = (c->r[2] != c->r[0]); c->r[2] = (uint32_t)((int32_t)c->r[3] < -5632); if (_t) goto L_8012FB0C; }
    c->r[2] = c->r[0] + (uint32_t)5632; goto L_8012FB14;
  L_8012FB0C:;
    { int _t = (c->r[2] == c->r[0]); c->r[2] = c->r[0] + (uint32_t)-5632; if (_t) goto L_8012FB18; }
  L_8012FB14:;
    c->mem_w16((c->r[17] + (uint32_t)72), (uint16_t)c->r[2]);
  L_8012FB18:;
    c->r[3] = c->r[19] << 16;
    c->r[2] = (uint32_t)c->mem_r16((c->r[17] + (uint32_t)72));
    c->r[18] = (uint32_t)((int32_t)c->r[3] >> 16);
    c->r[2] = c->r[2] << 16;
    c->r[2] = (uint32_t)((int32_t)c->r[2] >> 24);
    c->r[2] = c->r[19] + c->r[2];
    c->r[3] = c->r[2] << 16;
    c->r[3] = (uint32_t)((int32_t)c->r[3] >> 16);
    { int64_t _p = (int64_t)(int32_t)c->r[18] * (int64_t)(int32_t)c->r[3]; c->lo = (uint32_t)_p; c->hi = (uint32_t)((uint64_t)_p >> 32); }
    c->r[7] = c->lo;
    { int _t = ((int32_t)c->r[7] > 0); c->r[19] = c->r[2] + c->r[0]; if (_t) goto L_8012FB50; }
    c->r[2] = c->r[0] + (uint32_t)1;
    c->mem_w8((c->r[17] + (uint32_t)6), (uint8_t)c->r[2]);
  L_8012FB50:;
    c->r[2] = (uint32_t)(int16_t)c->mem_r16((c->r[17] + (uint32_t)100));
    c->r[4] = (uint32_t)c->mem_r16((c->r[17] + (uint32_t)100));
    c->r[2] = (uint32_t)((int32_t)c->r[3] < (int32_t)c->r[2]);
    { int _t = (c->r[2] != c->r[0]);  if (_t) goto L_8012FB78; }
    c->r[2] = (uint32_t)(int16_t)c->mem_r16((c->r[17] + (uint32_t)102));
    c->r[2] = (uint32_t)((int32_t)c->r[2] < (int32_t)c->r[3]);
    { int _t = (c->r[2] == c->r[0]);  if (_t) goto L_8012FCC0; }
  L_8012FB78:;
    { int _t = ((int32_t)c->r[3] >= 0);  if (_t) goto L_8012FB88; }
    c->r[19] = c->r[4] + c->r[0]; goto L_8012FB94;
  L_8012FB88:;
    { int _t = ((int32_t)c->r[3] <= 0);  if (_t) goto L_8012FB94; }
    c->r[19] = (uint32_t)c->mem_r16((c->r[17] + (uint32_t)102));
  L_8012FB94:;
    c->r[3] = (uint32_t)c->mem_r16((c->r[17] + (uint32_t)120));
    c->r[2] = c->r[3] & 1u;
    { int _t = (c->r[2] == c->r[0]); c->r[2] = c->r[3] & 32768u; if (_t) goto L_8012FBCC; }
    { int _t = (c->r[2] != c->r[0]); c->r[2] = c->r[0] + (uint32_t)15; if (_t) goto L_8012FBB4; }
    c->r[2] = c->r[0] + (uint32_t)30;
  L_8012FBB4:;
    c->mem_w16((c->r[17] + (uint32_t)64), (uint16_t)c->r[2]);
    c->r[2] = (uint32_t)c->mem_r8((c->r[17] + (uint32_t)6));
    c->r[2] = c->r[2] + (uint32_t)1;
    c->mem_w8((c->r[17] + (uint32_t)6), (uint8_t)c->r[2]); goto L_8012FBE0;
  L_8012FBCC:;
    c->mem_w16((c->r[17] + (uint32_t)72), (uint16_t)c->r[0]);
    c->mem_w16((c->r[17] + (uint32_t)78), (uint16_t)c->r[0]);
    c->mem_w16((c->r[17] + (uint32_t)120), (uint16_t)c->r[0]);
    c->mem_w8((c->r[17] + kNodeSubState), (uint8_t)c->r[0]);
    c->mem_w8((c->r[17] + (uint32_t)6), (uint8_t)c->r[0]);
  L_8012FBE0:;
    c->mem_w8((c->r[17] + kNodeModeByte), (uint8_t)c->r[0]);
    c->mem_w16((c->r[17] + (uint32_t)118), (uint16_t)c->r[0]); goto L_8012FCC0;
  L_8012FBEC:;
    c->r[2] = (uint32_t)c->mem_r16((c->r[17] + (uint32_t)64));
    c->r[2] = c->r[2] + (uint32_t)-1;
    c->mem_w16((c->r[17] + (uint32_t)64), (uint16_t)c->r[2]);
    c->r[2] = c->r[2] << 16;
    { int _t = ((int32_t)c->r[2] > 0); c->r[4] = c->r[17] + c->r[0]; if (_t) goto L_8012FCC4; }
    c->r[3] = (uint32_t)c->mem_r16((c->r[17] + (uint32_t)120));
    c->r[2] = c->r[3] & 2u;
    { int _t = (c->r[2] == c->r[0]); c->r[2] = c->r[3] & 32768u; if (_t) goto L_8012FC30; }
    c->mem_w16((c->r[17] + (uint32_t)72), (uint16_t)c->r[0]);
    c->mem_w16((c->r[17] + (uint32_t)78), (uint16_t)c->r[0]);
    c->mem_w16((c->r[17] + (uint32_t)120), (uint16_t)c->r[0]);
    c->mem_w8((c->r[17] + kNodeSubState), (uint8_t)c->r[0]); goto L_8012FCBC;
  L_8012FC30:;
    { int _t = (c->r[2] != c->r[0]); c->r[2] = c->r[0] + (uint32_t)5632; if (_t) goto L_8012FC3C; }
    c->r[2] = c->r[0] + (uint32_t)3072;
  L_8012FC3C:;
    c->mem_w16((c->r[17] + (uint32_t)72), (uint16_t)c->r[2]);
    c->r[2] = c->r[19] << 16;
    { int _t = ((int32_t)c->r[2] <= 0);  if (_t) goto L_8012FCB8; }
     goto L_8012FCA8;
  L_8012FC54:;
    c->r[2] = (uint32_t)c->mem_r16((c->r[17] + (uint32_t)64));
    c->r[2] = c->r[2] + (uint32_t)-1;
    c->mem_w16((c->r[17] + (uint32_t)64), (uint16_t)c->r[2]);
    c->r[2] = c->r[2] << 16;
    { int _t = (c->r[2] != c->r[0]); c->r[4] = c->r[17] + c->r[0]; if (_t) goto L_8012FCC4; }
    c->r[2] = (uint32_t)c->mem_r16((c->r[17] + (uint32_t)120));
    c->r[2] = c->r[2] & 65531u;
    c->mem_w16((c->r[17] + (uint32_t)120), (uint16_t)c->r[2]);
    c->r[2] = c->r[2] & 32768u;
    { int _t = (c->r[2] != c->r[0]); c->r[2] = c->r[0] + (uint32_t)5632; if (_t) goto L_8012FC90; }
    c->r[2] = c->r[0] + (uint32_t)3072;
  L_8012FC90:;
    c->mem_w16((c->r[17] + (uint32_t)72), (uint16_t)c->r[2]);
    c->r[2] = (uint32_t)(int16_t)c->mem_r16((c->r[17] + (uint32_t)104));
    c->r[2] = c->r[2] & 32768u;
    { int _t = (c->r[2] != c->r[0]);  if (_t) goto L_8012FCB8; }
  L_8012FCA8:;
    c->r[2] = (uint32_t)c->mem_r16((c->r[17] + (uint32_t)72));
    c->r[2] = c->r[0] - c->r[2];
    c->mem_w16((c->r[17] + (uint32_t)72), (uint16_t)c->r[2]);
  L_8012FCB8:;
    c->mem_w16((c->r[17] + (uint32_t)78), (uint16_t)c->r[0]);
  L_8012FCBC:;
    c->mem_w8((c->r[17] + (uint32_t)6), (uint8_t)c->r[0]);
  L_8012FCC0:;
    c->r[4] = c->r[17] + c->r[0];
  L_8012FCC4:;
    c->r[2] = c->mem_r32((c->r[17] + (uint32_t)196));
    c->r[3] = c->r[19] & 4095u;
    c->r[31] = 0x8012FCD4u;
    c->mem_w16((c->r[2] + (uint32_t)8), (uint16_t)c->r[3]); ov_a00_func_801314B4(c);
    c->r[2] = c->r[21] + c->r[0];
    { int _t = (c->r[2] == c->r[0]); c->r[4] = c->r[19] << 16; if (_t) goto L_8012FD3C; }
    c->r[31] = 0x8012FCE8u;
    c->r[4] = (uint32_t)((int32_t)c->r[4] >> 16); rec_dispatch(c, 0x80083E80u);
    c->r[3] = c->mem_r32((c->r[17] + (uint32_t)200));
    c->r[3] = (uint32_t)(int16_t)c->mem_r16((c->r[3] + (uint32_t)4));
    c->r[3] = c->r[0] - c->r[3];
    { int64_t _p = (int64_t)(int32_t)c->r[2] * (int64_t)(int32_t)c->r[3]; c->lo = (uint32_t)_p; c->hi = (uint32_t)((uint64_t)_p >> 32); }
    c->r[7] = c->lo;
    c->r[2] = (uint32_t)((int32_t)c->r[7] >> 12);
    c->r[4] = c->r[2] - c->r[20];
    c->r[2] = c->r[4] << 16;
    { int _t = ((int32_t)c->r[2] >= 0); c->r[3] = (uint32_t)32782u << 16; if (_t) goto L_8012FD28; }
    c->r[3] = c->r[3] + (uint32_t)32384;
    c->r[2] = (uint32_t)c->mem_r16((c->r[3] + (uint32_t)50));
    c->r[2] = c->r[2] - c->r[4]; goto L_8012FD38;
  L_8012FD28:;
    c->r[3] = c->r[3] + (uint32_t)32384;
    c->r[2] = (uint32_t)c->mem_r16((c->r[3] + (uint32_t)50));
    c->r[2] = c->r[2] + c->r[4];
  L_8012FD38:;
    c->mem_w16((c->r[3] + (uint32_t)50), (uint16_t)c->r[2]);
  L_8012FD3C:;
    c->r[2] = (uint32_t)(int16_t)c->mem_r16((c->r[17] + (uint32_t)120));
    { int _t = (c->r[2] == c->r[0]);  if (_t) goto L_8012FD5C; }
    c->r[31] = 0x8012FD54u;
    c->r[4] = c->r[17] + c->r[0]; ov_a00_func_801308E0(c);
     goto L_8012FD64;
  L_8012FD5C:;
    c->r[31] = 0x8012FD64u;
    c->r[4] = c->r[17] + c->r[0]; ov_a00_func_80131578(c);
  L_8012FD64:;
    c->r[31] = c->mem_r32((c->r[29] + (uint32_t)40));
    c->r[21] = c->mem_r32((c->r[29] + (uint32_t)36));
    c->r[20] = c->mem_r32((c->r[29] + (uint32_t)32));
    c->r[19] = c->mem_r32((c->r[29] + (uint32_t)28));
    c->r[18] = c->mem_r32((c->r[29] + (uint32_t)24));
    c->r[17] = c->mem_r32((c->r[29] + (uint32_t)20));
    c->r[16] = c->mem_r32((c->r[29] + (uint32_t)16));
    c->r[29] = c->r[29] + (uint32_t)48; return;
    return;
}

// FUN_0x8012FD88 — the node[5]==2 sub-state tick.
// ORACLE: ov_a00_gen_8012FD88
void SubstateEdgeLeaves::substate2Tick(Core* c) {
    c->r[29] = c->r[29] + (uint32_t)-24;
    c->mem_w32((c->r[29] + (uint32_t)16), c->r[16]);
    c->r[16] = c->r[4] + c->r[0];
    c->mem_w32((c->r[29] + (uint32_t)20), c->r[31]);
    c->r[3] = (uint32_t)c->mem_r8((c->r[16] + (uint32_t)6));
    c->r[2] = c->r[0] + (uint32_t)1;
    { int _t = (c->r[3] == c->r[2]); c->r[2] = (uint32_t)((int32_t)c->r[3] < 2); if (_t) goto L_80130084; }
    { int _t = (c->r[2] == c->r[0]); c->r[2] = c->r[0] + (uint32_t)2; if (_t) goto L_8012FDC0; }
    { int _t = (c->r[3] == c->r[0]); c->r[6] = c->r[0] + c->r[0]; if (_t) goto L_8012FDD0; }
     goto L_801303F8;
  L_8012FDC0:;
    { int _t = (c->r[3] == c->r[2]);  if (_t) goto L_8013019C; }
     goto L_801303F8;
  L_8012FDD0:;
    c->r[2] = (uint32_t)(int16_t)c->mem_r16((c->r[16] + kNodeAngleSel));
    c->r[2] = c->r[2] << 2;
    c->r[2] = c->r[16] + c->r[2];
    c->r[3] = c->mem_r32((c->r[2] + (uint32_t)192));
    c->r[2] = (uint32_t)c->mem_r16((c->r[3] + (uint32_t)12));
    c->r[2] = c->r[2] & 4095u;
    c->mem_w16((c->r[3] + (uint32_t)12), (uint16_t)c->r[2]);
    c->r[2] = (uint32_t)(int16_t)c->mem_r16((c->r[16] + kNodeAngleSel));
    c->r[2] = c->r[2] << 2;
    c->r[2] = c->r[16] + c->r[2];
    c->r[2] = c->mem_r32((c->r[2] + (uint32_t)192));
    c->r[4] = (uint32_t)c->mem_r8((c->r[16] + (uint32_t)70));
    c->r[5] = (uint32_t)(int16_t)c->mem_r16((c->r[2] + (uint32_t)10));
    c->r[31] = 0x8012FE1Cu;
    c->r[4] = c->r[4] << 4; rec_dispatch(c, 0x80077768u);
    c->r[3] = (uint32_t)c->mem_r8((c->r[16] + (uint32_t)71));
    c->r[3] = c->r[3] & 128u;
    { int _t = (c->r[3] == c->r[0]); c->mem_w8((c->r[16] + (uint32_t)70), (uint8_t)c->r[2]); if (_t) goto L_8012FE64; }
    c->r[2] = (uint32_t)(int16_t)c->mem_r16((c->r[16] + kNodeAngleSel));
    c->r[5] = c->r[0] + (uint32_t)6144;
    c->r[2] = c->r[2] << 2;
    c->r[2] = c->r[16] + c->r[2];
    c->r[3] = c->mem_r32((c->r[2] + (uint32_t)192));
    c->r[2] = c->r[0] + (uint32_t)896;
    c->mem_w16((c->r[3] + (uint32_t)20), (uint16_t)c->r[2]);
    c->r[3] = c->mem_r32((c->r[16] + (uint32_t)196));
    c->r[2] = c->r[0] + (uint32_t)14;
    c->mem_w16((c->r[3] + (uint32_t)18), (uint16_t)c->r[2]);
    c->r[3] = c->mem_r32((c->r[16] + (uint32_t)196));
    c->r[2] = c->r[0] + (uint32_t)-2; goto L_8012FE94;
  L_8012FE64:;
    c->r[2] = (uint32_t)(int16_t)c->mem_r16((c->r[16] + kNodeAngleSel));
    c->r[5] = c->r[0] + (uint32_t)2048;
    c->r[2] = c->r[2] << 2;
    c->r[2] = c->r[16] + c->r[2];
    c->r[3] = c->mem_r32((c->r[2] + (uint32_t)192));
    c->r[2] = c->r[0] + (uint32_t)576;
    c->mem_w16((c->r[3] + (uint32_t)20), (uint16_t)c->r[2]);
    c->r[3] = c->mem_r32((c->r[16] + (uint32_t)196));
    c->r[2] = c->r[0] + (uint32_t)16;
    c->mem_w16((c->r[3] + (uint32_t)18), (uint16_t)c->r[2]);
    c->r[3] = c->mem_r32((c->r[16] + (uint32_t)196));
    c->r[2] = c->r[0] + (uint32_t)-8;
  L_8012FE94:;
    c->mem_w16((c->r[3] + (uint32_t)20), (uint16_t)c->r[2]);
    c->r[3] = (uint32_t)c->mem_r8((c->r[16] + (uint32_t)70));
    c->r[2] = c->r[0] + (uint32_t)1;
    { int _t = (c->r[3] == c->r[0]); c->mem_w16((c->r[16] + (uint32_t)116), (uint16_t)c->r[2]); if (_t) goto L_8012FED8; }
    c->r[3] = c->mem_r32((c->r[16] + (uint32_t)196));
    c->r[2] = (uint32_t)c->mem_r16((c->r[3] + (uint32_t)18));
    c->r[2] = c->r[0] - c->r[2];
    c->mem_w16((c->r[3] + (uint32_t)18), (uint16_t)c->r[2]);
    c->r[3] = c->mem_r32((c->r[16] + (uint32_t)196));
    c->r[2] = (uint32_t)c->mem_r16((c->r[3] + (uint32_t)20));
    c->r[2] = c->r[0] - c->r[2];
    c->mem_w16((c->r[3] + (uint32_t)20), (uint16_t)c->r[2]);
  L_8012FED8:;
    c->r[2] = (uint32_t)c->mem_r8((c->r[16] + (uint32_t)71));
    c->r[2] = c->r[2] & 1u;
    { int _t = (c->r[2] != c->r[0]);  if (_t) goto L_8012FEFC; }
    c->r[2] = (uint32_t)c->mem_r8((c->r[16] + (uint32_t)70));
    c->r[2] = c->r[2] ^ 1u;
    c->mem_w8((c->r[16] + (uint32_t)70), (uint8_t)c->r[2]);
  L_8012FEFC:;
    c->r[2] = (uint32_t)c->mem_r8((c->r[16] + (uint32_t)70));
    { int _t = (c->r[2] == c->r[0]); c->r[3] = c->r[0] + (uint32_t)3; if (_t) goto L_8012FF38; }
    c->r[2] = (uint32_t)(int16_t)c->mem_r16((c->r[16] + kNodeAngleSel));
    c->mem_w8((c->r[16] + kNodeModeByte), (uint8_t)c->r[3]);
    c->r[2] = c->r[2] << 2;
    c->r[2] = c->r[16] + c->r[2];
    c->r[2] = c->mem_r32((c->r[2] + (uint32_t)192));
    c->r[2] = (uint32_t)c->mem_r16((c->r[2] + (uint32_t)12));
    c->r[2] = c->r[2] + c->r[5];
    c->mem_w16((c->r[16] + kNodeAngleParam), (uint16_t)c->r[2]); goto L_8012FF80;
  L_8012FF38:;
    c->r[2] = (uint32_t)(int16_t)c->mem_r16((c->r[16] + kNodeAngleSel));
    c->r[3] = c->r[0] + (uint32_t)2;
    c->mem_w8((c->r[16] + kNodeModeByte), (uint8_t)c->r[3]);
    c->r[3] = (uint32_t)(int16_t)c->mem_r16((c->r[16] + kNodeAngleSel));
    c->r[2] = c->r[2] << 2;
    c->r[2] = c->r[16] + c->r[2];
    c->r[2] = c->mem_r32((c->r[2] + (uint32_t)192));
    c->r[3] = c->r[3] << 2;
    c->r[2] = (uint32_t)c->mem_r16((c->r[2] + (uint32_t)12));
    c->r[3] = c->r[16] + c->r[3];
    c->r[2] = c->r[2] - c->r[5];
    c->mem_w16((c->r[16] + kNodeAngleParam), (uint16_t)c->r[2]);
    c->r[3] = c->mem_r32((c->r[3] + (uint32_t)192));
    c->r[2] = (uint32_t)c->mem_r16((c->r[3] + (uint32_t)20));
    c->r[2] = c->r[0] - c->r[2];
    c->mem_w16((c->r[3] + (uint32_t)20), (uint16_t)c->r[2]);
  L_8012FF80:;
    c->r[7] = (uint32_t)(int16_t)c->mem_r16((c->r[16] + kNodeAngleSel));
    c->r[2] = c->r[7] << 2;
    c->r[2] = c->r[16] + c->r[2];
    c->r[4] = c->mem_r32((c->r[2] + (uint32_t)192));
    c->r[3] = (uint32_t)c->mem_r8((c->r[4] + (uint32_t)62));
    c->r[2] = c->r[3] & 1u;
    { int _t = (c->r[2] == c->r[0]); c->r[2] = c->r[3] | 128u; if (_t) goto L_8012FFB4; }
    c->mem_w8((c->r[4] + (uint32_t)62), (uint8_t)c->r[2]); goto L_80130054;
  L_8012FFB4:;
    c->r[2] = c->r[3] & 16u;
    { int _t = (c->r[2] != c->r[0]); c->r[2] = c->r[0] | 65280u; if (_t) goto L_80130054; }
    c->r[6] = (uint32_t)(int16_t)c->mem_r16((c->r[16] + (uint32_t)98));
    c->r[4] = (uint32_t)c->mem_r16((c->r[16] + (uint32_t)98));
    c->r[3] = c->r[6] & 65280u;
    { int _t = (c->r[3] == c->r[2]); c->r[2] = c->r[4] & 32512u; if (_t) goto L_80130054; }
    c->r[3] = c->r[2] >> 8;
    c->r[2] = c->r[0] + (uint32_t)3;
    { int _t = (c->r[7] != c->r[2]); c->r[5] = c->r[3] + c->r[0]; if (_t) goto L_8012FFF4; }
    c->r[2] = c->r[6] & 32768u;
    { int _t = (c->r[2] != c->r[0]); c->r[2] = (uint32_t)((int32_t)c->r[5] < 5); if (_t) goto L_8012FFF8; }
    c->r[5] = c->r[3] + (uint32_t)1;
  L_8012FFF4:;
    c->r[2] = (uint32_t)((int32_t)c->r[5] < 5);
  L_8012FFF8:;
    { int _t = (c->r[2] == c->r[0]); c->r[4] = c->r[16] + c->r[0]; if (_t) goto L_8013002C; }
    c->r[31] = 0x80130008u;
    c->r[5] = c->r[5] + (uint32_t)1; rec_dispatch(c, 0x8004CBD8u);
    c->r[2] = (uint32_t)(int16_t)c->mem_r16((c->r[16] + kNodeAngleSel));
    c->r[2] = c->r[2] << 2;
    c->r[2] = c->r[16] + c->r[2];
    c->r[3] = c->mem_r32((c->r[2] + (uint32_t)192));
    c->r[2] = (uint32_t)c->mem_r8((c->r[3] + (uint32_t)62));
    c->r[2] = c->r[2] | 128u; goto L_80130050;
  L_8013002C:;
    c->r[2] = (uint32_t)(int16_t)c->mem_r16((c->r[16] + kNodeAngleSel));
    c->r[2] = c->r[2] << 2;
    c->r[2] = c->r[16] + c->r[2];
    c->r[3] = c->mem_r32((c->r[2] + (uint32_t)192));
    c->r[2] = (uint32_t)c->mem_r8((c->r[3] + (uint32_t)62));
    c->r[2] = c->r[2] | 16u;
  L_80130050:;
    c->mem_w8((c->r[3] + (uint32_t)62), (uint8_t)c->r[2]);
  L_80130054:;
    c->r[2] = (uint32_t)c->mem_r8((c->r[16] + (uint32_t)1));
    { int _t = (c->r[2] == c->r[0]); c->r[4] = c->r[0] + (uint32_t)131; if (_t) goto L_80130070; }
    c->r[5] = c->r[0] + c->r[0];
    c->r[31] = 0x80130070u;
    c->r[6] = c->r[5] + c->r[0]; rec_dispatch(c, 0x80074590u);
  L_80130070:;
    c->r[2] = (uint32_t)c->mem_r8((c->r[16] + (uint32_t)6));
    c->r[2] = c->r[2] + (uint32_t)1;
    c->mem_w8((c->r[16] + (uint32_t)6), (uint8_t)c->r[2]); goto L_801303F8;
  L_80130084:;
    c->r[2] = (uint32_t)(int16_t)c->mem_r16((c->r[16] + kNodeAngleSel));
    c->r[2] = c->r[2] << 2;
    c->r[2] = c->r[16] + c->r[2];
    c->r[3] = c->mem_r32((c->r[2] + (uint32_t)192));
    c->r[2] = (uint32_t)c->mem_r16((c->r[3] + (uint32_t)12));
    c->r[4] = (uint32_t)c->mem_r16((c->r[3] + (uint32_t)20));
    c->r[2] = c->r[2] + c->r[4];
    c->mem_w16((c->r[3] + (uint32_t)12), (uint16_t)c->r[2]);
    c->r[2] = (uint32_t)(int16_t)c->mem_r16((c->r[16] + kNodeAngleSel));
    c->r[2] = c->r[2] << 2;
    c->r[2] = c->r[16] + c->r[2];
    c->r[4] = c->mem_r32((c->r[2] + (uint32_t)192));
    c->r[5] = (uint32_t)(int16_t)c->mem_r16((c->r[4] + (uint32_t)20));
    { int _t = ((int32_t)c->r[5] <= 0);  if (_t) goto L_801300F0; }
    c->r[2] = (uint32_t)(int16_t)c->mem_r16((c->r[4] + (uint32_t)12));
    c->r[3] = (uint32_t)(int16_t)c->mem_r16((c->r[16] + kNodeAngleParam));
    c->r[2] = (uint32_t)((int32_t)c->r[2] < (int32_t)c->r[3]);
    { int _t = (c->r[2] == c->r[0]);  if (_t) goto L_80130110; }
  L_801300F0:;
    { int _t = ((int32_t)c->r[5] >= 0);  if (_t) goto L_80130168; }
    c->r[3] = (uint32_t)(int16_t)c->mem_r16((c->r[4] + (uint32_t)12));
    c->r[2] = (uint32_t)(int16_t)c->mem_r16((c->r[16] + kNodeAngleParam));
    c->r[2] = (uint32_t)((int32_t)c->r[2] < (int32_t)c->r[3]);
    { int _t = (c->r[2] != c->r[0]);  if (_t) goto L_80130168; }
  L_80130110:;
    c->r[2] = (uint32_t)(int16_t)c->mem_r16((c->r[16] + kNodeAngleSel));
    c->r[2] = c->r[2] << 2;
    c->r[2] = c->r[16] + c->r[2];
    c->r[4] = c->mem_r32((c->r[2] + (uint32_t)192));
    c->r[2] = (uint32_t)c->mem_r16((c->r[4] + (uint32_t)20));
    c->r[2] = c->r[2] << 16;
    c->r[3] = (uint32_t)((int32_t)c->r[2] >> 16);
    c->r[2] = c->r[2] >> 31;
    c->r[3] = c->r[3] + c->r[2];
    c->r[3] = (uint32_t)((int32_t)c->r[3] >> 1);
    c->mem_w16((c->r[4] + (uint32_t)18), (uint16_t)c->r[3]);
    c->r[2] = (uint32_t)c->mem_r8((c->r[16] + (uint32_t)6));
    c->r[3] = (uint32_t)c->mem_r8((c->r[16] + kNodeModeByte));
    c->mem_w16((c->r[16] + (uint32_t)116), (uint16_t)c->r[0]);
    c->r[2] = c->r[2] + (uint32_t)1;
    c->r[3] = c->r[3] | 128u;
    c->mem_w8((c->r[16] + (uint32_t)6), (uint8_t)c->r[2]);
    c->mem_w8((c->r[16] + kNodeModeByte), (uint8_t)c->r[3]); goto L_801303F8;
  L_80130168:;
    c->r[2] = (uint32_t)(int16_t)c->mem_r16((c->r[16] + kNodeAngleSel));
    c->r[2] = c->r[2] << 2;
    c->r[2] = c->r[16] + c->r[2];
    c->r[4] = c->mem_r32((c->r[2] + (uint32_t)192));
    c->r[3] = (uint32_t)c->mem_r16((c->r[4] + (uint32_t)20));
    c->r[2] = c->r[3] << 16;
    c->r[2] = (uint32_t)((int32_t)c->r[2] >> 19);
    c->r[3] = c->r[3] - c->r[2];
    c->mem_w16((c->r[4] + (uint32_t)20), (uint16_t)c->r[3]); goto L_801303F8;
  L_8013019C:;
    c->r[2] = (uint32_t)(int16_t)c->mem_r16((c->r[16] + kNodeAngleSel));
    c->r[2] = c->r[2] << 2;
    c->r[2] = c->r[16] + c->r[2];
    c->r[3] = c->mem_r32((c->r[2] + (uint32_t)192));
    c->r[2] = (uint32_t)c->mem_r16((c->r[3] + (uint32_t)12));
    c->r[4] = (uint32_t)c->mem_r16((c->r[3] + (uint32_t)20));
    c->r[2] = c->r[2] + c->r[4];
    c->mem_w16((c->r[3] + (uint32_t)12), (uint16_t)c->r[2]);
    c->r[2] = (uint32_t)c->mem_r8((c->r[16] + kNodeModeByte));
    c->r[2] = c->r[2] & 1u;
    { int _t = (c->r[2] == c->r[0]);  if (_t) goto L_80130210; }
    c->r[2] = (uint32_t)(int16_t)c->mem_r16((c->r[16] + kNodeAngleSel));
    c->r[2] = c->r[2] << 2;
    c->r[2] = c->r[16] + c->r[2];
    c->r[2] = c->mem_r32((c->r[2] + (uint32_t)192));
    c->r[3] = (uint32_t)(int16_t)c->mem_r16((c->r[16] + kNodeAngleParam));
    c->r[2] = (uint32_t)(int16_t)c->mem_r16((c->r[2] + (uint32_t)12));
    c->r[2] = (uint32_t)((int32_t)c->r[2] < (int32_t)c->r[3]);
    { int _t = (c->r[2] == c->r[0]);  if (_t) goto L_801303CC; }
     goto L_80130240;
  L_80130210:;
    c->r[2] = (uint32_t)(int16_t)c->mem_r16((c->r[16] + kNodeAngleSel));
    c->r[2] = c->r[2] << 2;
    c->r[2] = c->r[16] + c->r[2];
    c->r[2] = c->mem_r32((c->r[2] + (uint32_t)192));
    c->r[3] = (uint32_t)(int16_t)c->mem_r16((c->r[2] + (uint32_t)12));
    c->r[2] = (uint32_t)(int16_t)c->mem_r16((c->r[16] + kNodeAngleParam));
    c->r[2] = (uint32_t)((int32_t)c->r[2] < (int32_t)c->r[3]);
    { int _t = (c->r[2] == c->r[0]);  if (_t) goto L_801303CC; }
  L_80130240:;
    c->r[2] = (uint32_t)(int16_t)c->mem_r16((c->r[16] + kNodeAngleSel));
    c->r[3] = (uint32_t)c->mem_r16((c->r[16] + kNodeAngleParam));
    c->r[2] = c->r[2] << 2;
    c->r[2] = c->r[16] + c->r[2];
    c->r[2] = c->mem_r32((c->r[2] + (uint32_t)192));
    c->r[3] = c->r[3] & 4095u;
    c->mem_w16((c->r[2] + (uint32_t)12), (uint16_t)c->r[3]);
    c->r[2] = c->r[0] | 65280u;
    c->r[6] = (uint32_t)(int16_t)c->mem_r16((c->r[16] + (uint32_t)98));
    c->r[4] = (uint32_t)c->mem_r16((c->r[16] + (uint32_t)98));
    c->r[3] = c->r[6] & 65280u;
    { int _t = (c->r[3] == c->r[2]); c->r[2] = c->r[4] & 32512u; if (_t) goto L_801302F4; }
    c->r[4] = c->r[2] >> 8;
    c->r[3] = (uint32_t)(int16_t)c->mem_r16((c->r[16] + kNodeAngleSel));
    c->r[2] = c->r[0] + (uint32_t)3;
    { int _t = (c->r[3] != c->r[2]); c->r[5] = c->r[4] + c->r[0]; if (_t) goto L_80130298; }
    c->r[2] = c->r[6] & 32768u;
    { int _t = (c->r[2] != c->r[0]);  if (_t) goto L_80130298; }
    c->r[5] = c->r[4] + (uint32_t)1;
  L_80130298:;
    c->r[2] = (uint32_t)(int16_t)c->mem_r16((c->r[16] + kNodeAngleSel));
    c->r[2] = c->r[2] << 2;
    c->r[2] = c->r[16] + c->r[2];
    c->r[2] = c->mem_r32((c->r[2] + (uint32_t)192));
    c->r[2] = (uint32_t)(int16_t)c->mem_r16((c->r[2] + (uint32_t)12));
    { int _t = (c->r[2] != c->r[0]); c->r[2] = (uint32_t)32780u << 16; if (_t) goto L_801302DC; }
    c->r[2] = c->r[2] + (uint32_t)-1936;
    c->r[3] = c->r[0] + (uint32_t)1;
    c->r[3] = c->r[3] << (c->r[5] & 31);
    c->r[4] = (uint32_t)c->mem_r8((c->r[2] + (uint32_t)354));
    c->r[3] = ~(c->r[0] | c->r[3]);
    c->r[4] = c->r[4] & c->r[3]; goto L_801302F0;
  L_801302DC:;
    c->r[2] = c->r[2] + (uint32_t)-1936;
    c->r[3] = c->r[0] + (uint32_t)1;
    c->r[4] = (uint32_t)c->mem_r8((c->r[2] + (uint32_t)354));
    c->r[3] = c->r[3] << (c->r[5] & 31);
    c->r[4] = c->r[4] | c->r[3];
  L_801302F0:;
    c->mem_w8((c->r[2] + (uint32_t)354), (uint8_t)c->r[4]);
  L_801302F4:;
    c->r[2] = (uint32_t)(int16_t)c->mem_r16((c->r[16] + kNodeAngleSel));
    c->mem_w16((c->r[16] + (uint32_t)118), (uint16_t)c->r[0]);
    c->mem_w16((c->r[16] + (uint32_t)120), (uint16_t)c->r[0]);
    c->mem_w16((c->r[16] + (uint32_t)72), (uint16_t)c->r[0]);
    c->mem_w16((c->r[16] + (uint32_t)78), (uint16_t)c->r[0]);
    c->r[2] = c->r[2] << 2;
    c->r[2] = c->r[16] + c->r[2];
    c->r[5] = c->mem_r32((c->r[2] + (uint32_t)192));
    c->r[3] = (uint32_t)c->mem_r8((c->r[5] + (uint32_t)62));
    c->r[2] = c->r[3] & 128u;
    { int _t = (c->r[2] == c->r[0]); c->r[2] = c->r[3] & 1u; if (_t) goto L_801303BC; }
    { int _t = (c->r[2] == c->r[0]); c->r[2] = c->r[3] & 16u; if (_t) goto L_801303A0; }
    c->r[31] = 0x8013033Cu;
    c->r[4] = c->r[16] + c->r[0]; ov_a00_func_80127384(c);
    c->r[2] = c->r[2] << 16;
    { int _t = (c->r[2] == c->r[0]); c->r[4] = c->r[16] + c->r[0]; if (_t) goto L_80130374; }
    c->r[2] = (uint32_t)(int16_t)c->mem_r16((c->r[16] + kNodeAngleSel));
    c->mem_w16((c->r[16] + kNodePendingCmd), (uint16_t)c->r[0]);
    c->r[2] = c->r[2] << 2;
    c->r[2] = c->r[16] + c->r[2];
    c->r[3] = c->mem_r32((c->r[2] + (uint32_t)192));
    c->r[2] = (uint32_t)c->mem_r8((c->r[3] + (uint32_t)62));
    c->r[2] = c->r[2] & 254u;
    c->mem_w8((c->r[3] + (uint32_t)62), (uint8_t)c->r[2]); goto L_801303B0;
  L_80130374:;
    c->r[2] = (uint32_t)(int16_t)c->mem_r16((c->r[16] + kNodeAngleSel));
    c->r[2] = c->r[2] << 2;
    c->r[2] = c->r[16] + c->r[2];
    c->r[3] = c->mem_r32((c->r[2] + (uint32_t)192));
    c->r[2] = (uint32_t)c->mem_r8((c->r[3] + (uint32_t)62));
    c->r[2] = c->r[2] & 127u;
    c->mem_w8((c->r[3] + (uint32_t)62), (uint8_t)c->r[2]); goto L_801303BC;
  L_801303A0:;
    { int _t = (c->r[2] != c->r[0]); c->r[4] = c->r[16] + c->r[0]; if (_t) goto L_801303BC; }
    c->r[2] = c->r[3] | 16u;
    c->mem_w8((c->r[5] + (uint32_t)62), (uint8_t)c->r[2]);
  L_801303B0:;
    c->r[5] = (uint32_t)(int16_t)c->mem_r16((c->r[16] + kNodeAngleSel));
    c->r[31] = 0x801303BCu;
    c->r[6] = c->r[0] + c->r[0]; ov_a00_func_80131768(c);
  L_801303BC:;
    c->mem_w8((c->r[16] + kNodeSubState), (uint8_t)c->r[0]);
    c->mem_w8((c->r[16] + (uint32_t)6), (uint8_t)c->r[0]);
    c->mem_w8((c->r[16] + kNodeModeByte), (uint8_t)c->r[0]); goto L_801303F8;
  L_801303CC:;
    c->r[2] = (uint32_t)(int16_t)c->mem_r16((c->r[16] + kNodeAngleSel));
    c->r[2] = c->r[2] << 2;
    c->r[2] = c->r[16] + c->r[2];
    c->r[3] = c->mem_r32((c->r[2] + (uint32_t)192));
    c->r[2] = (uint32_t)c->mem_r16((c->r[3] + (uint32_t)20));
    c->r[4] = (uint32_t)c->mem_r16((c->r[3] + (uint32_t)18));
    c->r[2] = c->r[2] - c->r[4];
    c->mem_w16((c->r[3] + (uint32_t)20), (uint16_t)c->r[2]);
  L_801303F8:;
    c->r[31] = 0x80130400u;
    c->r[4] = c->r[16] + c->r[0]; ov_a00_func_801308E0(c);
    { int _t = (c->r[2] == c->r[0]);  if (_t) goto L_801304AC; }
    c->r[2] = (uint32_t)(int16_t)c->mem_r16((c->r[16] + kNodeAngleSel));
    c->r[3] = (uint32_t)c->mem_r16((c->r[16] + kNodeAngleParam));
    c->r[2] = c->r[2] << 2;
    c->r[2] = c->r[16] + c->r[2];
    c->r[2] = c->mem_r32((c->r[2] + (uint32_t)192));
    c->r[3] = c->r[3] & 4095u;
    c->mem_w16((c->r[2] + (uint32_t)12), (uint16_t)c->r[3]);
    c->r[4] = (uint32_t)c->mem_r16((c->r[16] + (uint32_t)98));
    c->r[3] = (uint32_t)(int16_t)c->mem_r16((c->r[16] + kNodeAngleSel));
    c->r[2] = c->r[4] & 32512u;
    c->r[6] = c->r[2] >> 8;
    c->r[2] = c->r[0] + (uint32_t)3;
    { int _t = (c->r[3] != c->r[2]); c->r[5] = c->r[6] + c->r[0]; if (_t) goto L_80130450; }
    c->r[2] = c->r[4] & 32768u;
    { int _t = (c->r[2] != c->r[0]);  if (_t) goto L_80130450; }
    c->r[5] = c->r[6] + (uint32_t)1;
  L_80130450:;
    c->r[2] = (uint32_t)(int16_t)c->mem_r16((c->r[16] + kNodeAngleSel));
    c->r[2] = c->r[2] << 2;
    c->r[2] = c->r[16] + c->r[2];
    c->r[2] = c->mem_r32((c->r[2] + (uint32_t)192));
    c->r[2] = (uint32_t)(int16_t)c->mem_r16((c->r[2] + (uint32_t)12));
    { int _t = (c->r[2] != c->r[0]); c->r[2] = (uint32_t)32780u << 16; if (_t) goto L_80130494; }
    c->r[2] = c->r[2] + (uint32_t)-1936;
    c->r[3] = c->r[0] + (uint32_t)1;
    c->r[3] = c->r[3] << (c->r[5] & 31);
    c->r[4] = (uint32_t)c->mem_r8((c->r[2] + (uint32_t)354));
    c->r[3] = ~(c->r[0] | c->r[3]);
    c->r[4] = c->r[4] & c->r[3]; goto L_801304A8;
  L_80130494:;
    c->r[2] = c->r[2] + (uint32_t)-1936;
    c->r[3] = c->r[0] + (uint32_t)1;
    c->r[4] = (uint32_t)c->mem_r8((c->r[2] + (uint32_t)354));
    c->r[3] = c->r[3] << (c->r[5] & 31);
    c->r[4] = c->r[4] | c->r[3];
  L_801304A8:;
    c->mem_w8((c->r[2] + (uint32_t)354), (uint8_t)c->r[4]);
  L_801304AC:;
    c->r[3] = (uint32_t)(int16_t)c->mem_r16((c->r[16] + (uint32_t)116));
    { int _t = (c->r[3] == c->r[0]); c->r[2] = c->r[0] + (uint32_t)1; if (_t) goto L_801304CC; }
    { int _t = (c->r[3] == c->r[2]);  if (_t) goto L_801304DC; }
     goto L_80130514;
  L_801304CC:;
    c->r[3] = c->mem_r32((c->r[16] + (uint32_t)196));
    c->r[2] = (uint32_t)c->mem_r16((c->r[16] + (uint32_t)112));
    c->mem_w16((c->r[3] + (uint32_t)10), (uint16_t)c->r[2]); goto L_80130514;
  L_801304DC:;
    c->r[3] = c->mem_r32((c->r[16] + (uint32_t)196));
    c->r[2] = (uint32_t)c->mem_r16((c->r[3] + (uint32_t)10));
    c->r[4] = (uint32_t)c->mem_r16((c->r[3] + (uint32_t)18));
    c->r[2] = c->r[2] + c->r[4];
    c->mem_w16((c->r[3] + (uint32_t)10), (uint16_t)c->r[2]);
    c->r[3] = c->mem_r32((c->r[16] + (uint32_t)196));
    c->r[2] = (uint32_t)c->mem_r16((c->r[3] + (uint32_t)18));
    c->r[4] = (uint32_t)c->mem_r16((c->r[3] + (uint32_t)20));
    c->r[2] = c->r[2] + c->r[4];
    c->mem_w16((c->r[3] + (uint32_t)18), (uint16_t)c->r[2]);
  L_80130514:;
    c->r[31] = c->mem_r32((c->r[29] + (uint32_t)20));
    c->r[16] = c->mem_r32((c->r[29] + (uint32_t)16));
    c->r[29] = c->r[29] + (uint32_t)24; return;
    return;
}

// FUN_0x80130524 — the node[5]==3 sub-state tick.
// ORACLE: ov_a00_gen_80130524
void SubstateEdgeLeaves::substate3Tick(Core* c) {
    c->r[29] = c->r[29] + (uint32_t)-24;
    c->mem_w32((c->r[29] + (uint32_t)16), c->r[16]);
    c->r[16] = c->r[4] + c->r[0];
    c->mem_w32((c->r[29] + (uint32_t)20), c->r[31]);
    c->r[3] = (uint32_t)c->mem_r8((c->r[16] + (uint32_t)6));
    c->r[2] = c->r[0] + (uint32_t)1;
    { int _t = (c->r[3] == c->r[2]); c->r[2] = (uint32_t)((int32_t)c->r[3] < 2); if (_t) goto L_80130610; }
    { int _t = (c->r[2] == c->r[0]); c->r[2] = c->r[0] + (uint32_t)2; if (_t) goto L_8013055C; }
    { int _t = (c->r[3] == c->r[0]);  if (_t) goto L_8013056C; }
     goto L_80130758;
  L_8013055C:;
    { int _t = (c->r[3] == c->r[2]);  if (_t) goto L_801306C4; }
     goto L_80130758;
  L_8013056C:;
    c->r[3] = (uint32_t)c->mem_r8((c->r[16] + (uint32_t)71));
    c->r[2] = c->r[3] & 64u;
    { int _t = (c->r[2] == c->r[0]); c->r[2] = c->r[0] + (uint32_t)32; if (_t) goto L_8013058C; }
    c->r[3] = c->mem_r32((c->r[16] + (uint32_t)196));
    c->mem_w16((c->r[3] + (uint32_t)18), (uint16_t)c->r[2]); goto L_801305B0;
  L_8013058C:;
    c->r[2] = c->r[3] & 128u;
    { int _t = (c->r[2] == c->r[0]); c->r[2] = c->r[0] + (uint32_t)96; if (_t) goto L_801305A4; }
    c->r[3] = c->mem_r32((c->r[16] + (uint32_t)196));
    c->mem_w16((c->r[3] + (uint32_t)18), (uint16_t)c->r[2]); goto L_801305B0;
  L_801305A4:;
    c->r[3] = c->mem_r32((c->r[16] + (uint32_t)196));
    c->r[2] = c->r[0] + (uint32_t)64;
    c->mem_w16((c->r[3] + (uint32_t)18), (uint16_t)c->r[2]);
  L_801305B0:;
    c->mem_w16((c->r[16] + (uint32_t)64), (uint16_t)c->r[2]);
    c->r[2] = (uint32_t)(int16_t)c->mem_r16((c->r[16] + kNodeAngleSel));
    c->r[6] = c->r[0] + c->r[0];
    c->r[2] = c->r[2] << 2;
    c->r[2] = c->r[16] + c->r[2];
    c->r[2] = c->mem_r32((c->r[2] + (uint32_t)192));
    c->r[4] = (uint32_t)c->mem_r8((c->r[16] + (uint32_t)70));
    c->r[5] = (uint32_t)(int16_t)c->mem_r16((c->r[2] + (uint32_t)10));
    c->r[31] = 0x801305D8u;
    c->r[4] = c->r[4] << 4; rec_dispatch(c, 0x80077768u);
    { int _t = (c->r[2] == c->r[0]);  if (_t) goto L_801305F8; }
    c->r[3] = c->mem_r32((c->r[16] + (uint32_t)196));
    c->r[2] = (uint32_t)c->mem_r16((c->r[3] + (uint32_t)18));
    c->r[2] = c->r[0] - c->r[2];
    c->mem_w16((c->r[3] + (uint32_t)18), (uint16_t)c->r[2]);
  L_801305F8:;
    c->r[2] = (uint32_t)c->mem_r8((c->r[16] + (uint32_t)6));
    c->r[3] = c->r[0] + (uint32_t)4;
    c->mem_w8((c->r[16] + kNodeModeByte), (uint8_t)c->r[3]);
    c->r[2] = c->r[2] + (uint32_t)1;
    c->mem_w8((c->r[16] + (uint32_t)6), (uint8_t)c->r[2]); goto L_80130758;
  L_80130610:;
    c->r[3] = c->mem_r32((c->r[16] + (uint32_t)196));
    c->r[2] = (uint32_t)c->mem_r16((c->r[3] + (uint32_t)10));
    c->r[4] = (uint32_t)c->mem_r16((c->r[3] + (uint32_t)18));
    c->r[2] = c->r[2] + c->r[4];
    c->mem_w16((c->r[3] + (uint32_t)10), (uint16_t)c->r[2]);
    c->r[5] = c->mem_r32((c->r[16] + (uint32_t)196));
    c->r[8] = (uint32_t)(int16_t)c->mem_r16((c->r[16] + (uint32_t)112));
    c->r[7] = (uint32_t)(int16_t)c->mem_r16((c->r[16] + (uint32_t)64));
    c->r[4] = (uint32_t)c->mem_r16((c->r[16] + (uint32_t)112));
    c->r[3] = (uint32_t)c->mem_r16((c->r[16] + (uint32_t)64));
    c->r[6] = (uint32_t)(int16_t)c->mem_r16((c->r[5] + (uint32_t)10));
    c->r[2] = c->r[8] + c->r[7];
    c->r[2] = (uint32_t)((int32_t)c->r[2] < (int32_t)c->r[6]);
    { int _t = (c->r[2] != c->r[0]); c->r[2] = c->r[8] - c->r[7]; if (_t) goto L_80130660; }
    c->r[2] = (uint32_t)((int32_t)c->r[6] < (int32_t)c->r[2]);
    { int _t = (c->r[2] == c->r[0]);  if (_t) goto L_80130758; }
  L_80130660:;
    c->r[2] = (uint32_t)(int16_t)c->mem_r16((c->r[5] + (uint32_t)18));
    { int _t = ((int32_t)c->r[2] > 0); c->r[2] = c->r[4] + c->r[3]; if (_t) goto L_80130674; }
    c->r[2] = c->r[4] - c->r[3];
  L_80130674:;
    c->mem_w16((c->r[5] + (uint32_t)10), (uint16_t)c->r[2]);
    c->r[2] = c->mem_r32((c->r[16] + (uint32_t)196));
    c->r[3] = (uint32_t)c->mem_r16((c->r[2] + (uint32_t)18));
    c->r[3] = c->r[0] - c->r[3];
    c->mem_w16((c->r[2] + (uint32_t)18), (uint16_t)c->r[3]);
    c->r[2] = (uint32_t)c->mem_r16((c->r[16] + (uint32_t)64));
    c->r[2] = c->r[2] + (uint32_t)-16;
    c->mem_w16((c->r[16] + (uint32_t)64), (uint16_t)c->r[2]);
    c->r[2] = c->r[2] << 16;
    { int _t = ((int32_t)c->r[2] > 0); c->r[2] = c->r[0] + (uint32_t)10; if (_t) goto L_80130758; }
    c->r[3] = (uint32_t)c->mem_r8((c->r[16] + (uint32_t)6));
    c->mem_w16((c->r[16] + (uint32_t)64), (uint16_t)c->r[2]);
    c->mem_w8((c->r[16] + kNodeModeByte), (uint8_t)c->r[0]);
    c->r[3] = c->r[3] + (uint32_t)1;
    c->mem_w8((c->r[16] + (uint32_t)6), (uint8_t)c->r[3]); goto L_80130758;
  L_801306C4:;
    c->r[4] = c->mem_r32((c->r[16] + (uint32_t)196));
    c->r[2] = (uint32_t)(int16_t)c->mem_r16((c->r[16] + (uint32_t)112));
    c->r[3] = (uint32_t)(int16_t)c->mem_r16((c->r[4] + (uint32_t)10));
    c->r[2] = (uint32_t)((int32_t)c->r[2] < (int32_t)c->r[3]);
    c->r[3] = (uint32_t)c->mem_r16((c->r[4] + (uint32_t)10));
    { int _t = (c->r[2] != c->r[0]); c->r[2] = c->r[3] + (uint32_t)-8; if (_t) goto L_801306E8; }
    c->r[2] = c->r[3] + (uint32_t)8;
  L_801306E8:;
    c->mem_w16((c->r[4] + (uint32_t)10), (uint16_t)c->r[2]);
    c->r[2] = (uint32_t)c->mem_r16((c->r[16] + (uint32_t)64));
    c->r[2] = c->r[2] + (uint32_t)-1;
    c->mem_w16((c->r[16] + (uint32_t)64), (uint16_t)c->r[2]);
    c->r[2] = c->r[2] << 16;
    { int _t = (c->r[2] != c->r[0]);  if (_t) goto L_80130758; }
    c->r[2] = c->mem_r32((c->r[16] + (uint32_t)196));
    c->r[3] = (uint32_t)c->mem_r16((c->r[16] + (uint32_t)112));
    c->mem_w16((c->r[2] + (uint32_t)10), (uint16_t)c->r[3]);
    c->r[2] = (uint32_t)(int16_t)c->mem_r16((c->r[16] + (uint32_t)120));
    { int _t = (c->r[2] == c->r[0]); c->r[2] = c->r[0] + (uint32_t)1; if (_t) goto L_80130744; }
    c->r[3] = c->mem_r32((c->r[16] + kChildTableOff));
    c->mem_w8((c->r[16] + kNodeSubState), (uint8_t)c->r[2]);
    c->r[2] = (uint32_t)c->mem_r8((c->r[3] + (uint32_t)62));
    c->mem_w8((c->r[16] + (uint32_t)6), (uint8_t)c->r[2]);
    c->mem_w8((c->r[3] + (uint32_t)62), (uint8_t)c->r[0]); goto L_80130758;
  L_80130744:;
    c->mem_w8((c->r[16] + kNodeSubState), (uint8_t)c->r[0]);
    c->mem_w8((c->r[16] + (uint32_t)6), (uint8_t)c->r[0]);
    c->mem_w16((c->r[16] + (uint32_t)118), (uint16_t)c->r[0]);
    c->mem_w16((c->r[16] + (uint32_t)72), (uint16_t)c->r[0]);
    c->mem_w16((c->r[16] + (uint32_t)78), (uint16_t)c->r[0]);
  L_80130758:;
    c->r[31] = 0x80130760u;
    c->r[4] = c->r[16] + c->r[0]; ov_a00_func_801308E0(c);
    { int _t = (c->r[2] == c->r[0]);  if (_t) goto L_80130778; }
    c->r[3] = c->mem_r32((c->r[16] + (uint32_t)196));
    c->r[2] = (uint32_t)c->mem_r16((c->r[16] + (uint32_t)112));
    c->mem_w16((c->r[3] + (uint32_t)10), (uint16_t)c->r[2]);
  L_80130778:;
    c->r[31] = c->mem_r32((c->r[29] + (uint32_t)20));
    c->r[16] = c->mem_r32((c->r[29] + (uint32_t)16));
    c->r[29] = c->r[29] + (uint32_t)24; return;
    return;
}

// FUN_0x801313C4 — clears the pending-command word and its derived fields.
// ORACLE: ov_a00_gen_801313C4
void SubstateEdgeLeaves::pendingCommandClear(Core* c) {
    c->r[2] = c->mem_r32((c->r[4] + (uint32_t)196));
    c->r[2] = (uint32_t)(int16_t)c->mem_r16((c->r[2] + (uint32_t)8));
    c->r[5] = c->r[2] + c->r[0];
    c->r[2] = (uint32_t)((int32_t)c->r[2] < 2049);
    { int _t = (c->r[2] != c->r[0]);  if (_t) goto L_801313E8; }
    c->r[5] = c->r[5] | 61440u;
  L_801313E8:;
    c->r[2] = (uint32_t)c->mem_r8((c->r[4] + (uint32_t)6));
    { int _t = (c->r[2] != c->r[0]); c->r[3] = c->r[0] + (uint32_t)64; if (_t) goto L_801314AC; }
    c->r[2] = (uint32_t)c->mem_r16((c->r[4] + kNodeConfig));
    c->r[2] = c->r[2] & 240u;
    { int _t = (c->r[2] != c->r[3]); c->mem_w8((c->r[4] + kNodeModeByte), (uint8_t)c->r[0]); if (_t) goto L_80131414; }
    c->mem_w8((c->r[4] + kNodeSubState), (uint8_t)c->r[0]); return;
  L_80131414:;
    c->r[2] = (uint32_t)c->mem_r16((c->r[4] + (uint32_t)104));
    c->r[3] = c->r[2] & 32767u;
    c->r[2] = c->r[0] + (uint32_t)1;
    { int _t = (c->r[3] == c->r[2]); c->r[2] = (uint32_t)((int32_t)c->r[3] < 2); if (_t) goto L_80131460; }
    { int _t = (c->r[2] == c->r[0]); c->r[2] = c->r[0] + (uint32_t)2; if (_t) goto L_80131444; }
    { int _t = (c->r[3] == c->r[0]); c->r[2] = c->r[6] - c->r[5]; if (_t) goto L_80131454; }
    c->r[2] = c->r[2] << 4; goto L_80131490;
  L_80131444:;
    { int _t = (c->r[3] == c->r[2]); c->r[2] = c->r[6] - c->r[5]; if (_t) goto L_80131480; }
    c->r[2] = c->r[2] << 4; goto L_80131490;
  L_80131454:;
    c->r[6] = (uint32_t)c->mem_r16((c->r[4] + (uint32_t)100));
    c->r[2] = c->r[6] - c->r[5]; goto L_8013148C;
  L_80131460:;
    c->r[2] = (uint32_t)(int16_t)c->mem_r16((c->r[4] + (uint32_t)100));
    c->r[3] = (uint32_t)(int16_t)c->mem_r16((c->r[4] + (uint32_t)102));
    c->r[2] = c->r[2] + c->r[3];
    c->r[3] = c->r[2] >> 31;
    c->r[2] = c->r[2] + c->r[3];
    c->r[6] = c->r[2] >> 1; goto L_80131484;
  L_80131480:;
    c->r[6] = (uint32_t)c->mem_r16((c->r[4] + (uint32_t)102));
  L_80131484:;
    c->r[2] = c->r[6] - c->r[5];
  L_8013148C:;
    c->r[2] = c->r[2] << 4;
  L_80131490:;
    c->mem_w16((c->r[4] + (uint32_t)72), (uint16_t)c->r[2]);
    c->r[2] = c->r[0] + (uint32_t)16;
    c->mem_w16((c->r[4] + (uint32_t)64), (uint16_t)c->r[2]);
    c->r[2] = c->r[0] + (uint32_t)1;
    c->mem_w8((c->r[4] + kNodeSubState), (uint8_t)c->r[2]);
    c->r[2] = c->r[0] + (uint32_t)4;
    c->mem_w8((c->r[4] + (uint32_t)6), (uint8_t)c->r[2]);
  L_801314AC:;
     return;
}

// FUN_0x80146348 — the assembly post-tick called after the sub-state work.
// ORACLE: ov_a00_gen_80146348
void SubstateEdgeLeaves::assemblyPostTick(Core* c) {
    c->r[29] = c->r[29] + (uint32_t)-32;
    c->mem_w32((c->r[29] + (uint32_t)20), c->r[17]);
    c->r[17] = c->r[4] + c->r[0];
    c->r[2] = (uint32_t)32780u << 16;
    c->mem_w32((c->r[29] + (uint32_t)16), c->r[16]);
    c->r[16] = c->r[2] + (uint32_t)-2040;
    c->mem_w32((c->r[29] + (uint32_t)28), c->r[31]);
    c->mem_w32((c->r[29] + (uint32_t)24), c->r[18]);
    c->r[2] = (uint32_t)c->mem_r8((c->r[16] + (uint32_t)27));
    c->r[2] = c->r[2] & 128u;
    { int _t = (c->r[2] == c->r[0]); c->r[18] = c->r[5] + c->r[0]; if (_t) goto L_80146384; }
    c->r[2] = c->r[0] + c->r[0]; goto L_801463D4;
  L_80146384:;
    c->r[4] = c->r[17] + c->r[0];
    c->r[5] = c->r[0] + (uint32_t)3;
    c->r[6] = c->r[0] + (uint32_t)2;
    c->r[31] = 0x80146398u;
    c->r[7] = c->r[5] + c->r[0]; rec_dispatch(c, 0x80072DDCu);
    c->r[3] = c->r[2] + c->r[0];
    { int _t = (c->r[3] == c->r[0]); c->r[2] = c->r[3] + c->r[0]; if (_t) goto L_801463D4; }
    c->r[2] = (uint32_t)c->mem_r8((c->r[16] + (uint32_t)27));
    c->r[2] = c->r[2] | 128u;
    c->mem_w8((c->r[16] + (uint32_t)27), (uint8_t)c->r[2]);
    c->r[2] = c->r[0] + (uint32_t)128;
    c->mem_w8((c->r[3] + (uint32_t)3), (uint8_t)c->r[2]);
    c->r[2] = (uint32_t)32788u << 16;
    c->r[2] = c->r[2] + (uint32_t)21040;
    c->mem_w32((c->r[3] + (uint32_t)16), c->r[17]);
    c->mem_w32((c->r[3] + (uint32_t)20), c->r[18]);
    c->mem_w32((c->r[3] + (uint32_t)28), c->r[2]);
    c->r[2] = c->r[3] + c->r[0];
  L_801463D4:;
    c->r[31] = c->mem_r32((c->r[29] + (uint32_t)28));
    c->r[18] = c->mem_r32((c->r[29] + (uint32_t)24));
    c->r[17] = c->mem_r32((c->r[29] + (uint32_t)20));
    c->r[16] = c->mem_r32((c->r[29] + (uint32_t)16));
    c->r[29] = c->r[29] + (uint32_t)32; return;
    return;
}

// FUN_0x8018C820 — the assembly's OPN-overlay hook, and the TWELFTH and last leaf of the chain
// kanban #8 names as its blocker.
//
// Lives in the OPN overlay, not A00, so it cannot use engine_set_override_a00. recomp_iface.h exposes
// no OPN setter, but none is needed: the address has exactly ONE caller in the whole image and it is
// `rec_dispatch(c, 0x8018C820u)` from the orchestrator itself (ov_a00_shard_0.c:16659), with no direct
// ov_opn_func_8018C820(c) call site anywhere. So rec_dispatch-only interception (setter = nullptr) is
// complete here rather than partial — verified by grep, not assumed because it was convenient.
// ORACLE: ov_opn_gen_8018C820
void SubstateEdgeLeaves::opnAssemblyHook(Core* c) {
    c->r[29] = c->r[29] + (uint32_t)-40;
    c->mem_w32((c->r[29] + (uint32_t)20), c->r[17]);
    c->r[17] = c->r[4] + c->r[0];
    c->mem_w32((c->r[29] + (uint32_t)32), c->r[31]);
    c->mem_w32((c->r[29] + (uint32_t)28), c->r[19]);
    c->mem_w32((c->r[29] + (uint32_t)24), c->r[18]);
    c->mem_w32((c->r[29] + (uint32_t)16), c->r[16]);
    c->r[3] = (uint32_t)c->mem_r8((c->r[17] + (uint32_t)6));
    c->r[19] = c->r[0] + (uint32_t)1;
    { int _t = (c->r[3] == c->r[19]); c->r[2] = (uint32_t)((int32_t)c->r[3] < 2); if (_t) goto L_8018C8AC; }
    { int _t = (c->r[2] == c->r[0]); c->r[18] = c->r[0] + (uint32_t)2; if (_t) goto L_8018C864; }
    { int _t = (c->r[3] == c->r[0]); c->r[2] = (uint32_t)32780u << 16; if (_t) goto L_8018C87C; }
     goto L_8018CA00;
  L_8018C864:;
    { int _t = (c->r[3] == c->r[18]); c->r[2] = c->r[0] + (uint32_t)3; if (_t) goto L_8018C8FC; }
    { int _t = (c->r[3] == c->r[2]); c->r[2] = (uint32_t)32780u << 16; if (_t) goto L_8018C9CC; }
     goto L_8018CA00;
  L_8018C87C:;
    c->r[2] = (uint32_t)c->mem_r8((c->r[2] + (uint32_t)-1555));
    c->r[2] = (uint32_t)(c->r[2] < (uint32_t)70);
    { int _t = (c->r[2] != c->r[0]);  if (_t) goto L_8018CA00; }
    c->r[3] = c->mem_r32((c->r[17] + (uint32_t)196));
    c->r[2] = (uint32_t)c->mem_r16((c->r[17] + (uint32_t)100));
    c->mem_w16((c->r[3] + (uint32_t)8), (uint16_t)c->r[2]);
    c->r[2] = (uint32_t)c->mem_r8((c->r[17] + (uint32_t)6));
    c->r[2] = c->r[2] + (uint32_t)1; goto L_8018C95C;
  L_8018C8AC:;
    c->r[2] = (uint32_t)32780u << 16;
    c->r[2] = (uint32_t)c->mem_r8((c->r[2] + (uint32_t)-1555));
    c->r[2] = (uint32_t)(c->r[2] < (uint32_t)75);
    { int _t = (c->r[2] != c->r[0]); c->r[5] = c->r[0] + c->r[0]; if (_t) goto L_8018CA00; }
    c->r[4] = c->r[0] + (uint32_t)129;
    c->r[6] = c->r[0] + (uint32_t)42;
    c->r[31] = 0x8018C8D4u;
    c->mem_w16((c->r[17] + (uint32_t)118), (uint16_t)c->r[19]); rec_dispatch(c, 0x80074590u);
    c->r[3] = (uint32_t)32789u << 16;
    c->mem_w32((c->r[3] + (uint32_t)-23952), c->r[2]);
    c->r[3] = (uint32_t)c->mem_r8((c->r[17] + (uint32_t)6));
    c->r[2] = c->r[0] + (uint32_t)256;
    c->mem_w16((c->r[17] + (uint32_t)72), (uint16_t)c->r[2]);
    c->mem_w16((c->r[17] + (uint32_t)78), (uint16_t)c->r[2]);
    c->mem_w16((c->r[17] + (uint32_t)64), (uint16_t)c->r[0]);
    c->r[3] = c->r[3] + (uint32_t)1;
    c->mem_w8((c->r[17] + (uint32_t)6), (uint8_t)c->r[3]); goto L_8018CA00;
  L_8018C8FC:;
    c->r[3] = (uint32_t)c->mem_r16((c->r[17] + (uint32_t)72));
    c->r[4] = c->mem_r32((c->r[17] + (uint32_t)196));
    c->r[3] = c->r[3] << 16;
    c->r[2] = (uint32_t)c->mem_r16((c->r[4] + (uint32_t)8));
    c->r[3] = (uint32_t)((int32_t)c->r[3] >> 24);
    c->r[2] = c->r[2] + c->r[3];
    c->mem_w16((c->r[4] + (uint32_t)8), (uint16_t)c->r[2]);
    c->r[4] = c->mem_r32((c->r[17] + (uint32_t)196));
    c->r[3] = (uint32_t)(int16_t)c->mem_r16((c->r[17] + (uint32_t)102));
    c->r[2] = (uint32_t)(int16_t)c->mem_r16((c->r[4] + (uint32_t)8));
    c->r[2] = (uint32_t)((int32_t)c->r[2] < (int32_t)c->r[3]);
    c->r[3] = (uint32_t)c->mem_r16((c->r[17] + (uint32_t)102));
    { int _t = (c->r[2] != c->r[0]);  if (_t) goto L_8018C99C; }
    c->mem_w16((c->r[4] + (uint32_t)8), (uint16_t)c->r[3]);
    c->r[2] = (uint32_t)(int16_t)c->mem_r16((c->r[17] + (uint32_t)64));
    { int _t = (c->r[2] == c->r[0]); c->r[16] = (uint32_t)32789u << 16; if (_t) goto L_8018C964; }
    c->r[2] = (uint32_t)c->mem_r8((c->r[17] + (uint32_t)6));
    c->mem_w16((c->r[17] + (uint32_t)72), (uint16_t)c->r[0]);
    c->mem_w16((c->r[17] + (uint32_t)78), (uint16_t)c->r[0]);
    c->r[2] = c->r[2] + (uint32_t)1;
  L_8018C95C:;
    c->mem_w8((c->r[17] + (uint32_t)6), (uint8_t)c->r[2]); goto L_8018CA00;
  L_8018C964:;
    c->r[4] = c->mem_r32((c->r[16] + (uint32_t)-23952));
    c->r[31] = 0x8018C970u;
     rec_dispatch(c, 0x80074AF0u);
    c->r[4] = c->r[0] + (uint32_t)130;
    c->r[5] = c->r[0] + c->r[0];
    c->r[6] = c->r[5] + c->r[0];
    c->r[31] = 0x8018C984u;
    c->mem_w32((c->r[16] + (uint32_t)-23952), c->r[0]); rec_dispatch(c, 0x80074590u);
    c->r[2] = c->r[0] + (uint32_t)-2560;
    c->mem_w16((c->r[17] + (uint32_t)72), (uint16_t)c->r[2]);
    c->r[2] = c->r[0] + (uint32_t)512;
    c->mem_w16((c->r[17] + (uint32_t)78), (uint16_t)c->r[2]);
    c->mem_w16((c->r[17] + (uint32_t)118), (uint16_t)c->r[18]);
    c->mem_w16((c->r[17] + (uint32_t)64), (uint16_t)c->r[19]);
  L_8018C99C:;
    c->r[2] = (uint32_t)c->mem_r16((c->r[17] + (uint32_t)72));
    c->r[3] = (uint32_t)c->mem_r16((c->r[17] + (uint32_t)78));
    c->r[2] = c->r[2] + c->r[3];
    c->mem_w16((c->r[17] + (uint32_t)72), (uint16_t)c->r[2]);
    c->r[2] = c->r[2] << 16;
    c->r[2] = (uint32_t)((int32_t)c->r[2] >> 16);
    c->r[2] = (uint32_t)((int32_t)c->r[2] < 5633);
    { int _t = (c->r[2] != c->r[0]); c->r[2] = c->r[0] + (uint32_t)5632; if (_t) goto L_8018CA00; }
    c->mem_w16((c->r[17] + (uint32_t)72), (uint16_t)c->r[2]); goto L_8018CA00;
  L_8018C9CC:;
    c->r[2] = (uint32_t)c->mem_r8((c->r[2] + (uint32_t)-1892));
    { int _t = (c->r[2] == c->r[18]); c->r[2] = c->r[0] + (uint32_t)240; if (_t) goto L_8018CA00; }
    c->r[4] = c->r[17] + c->r[0];
    c->r[3] = c->mem_r32((c->r[17] + (uint32_t)196));
    c->mem_w16((c->r[17] + (uint32_t)118), (uint16_t)c->r[0]);
    c->r[31] = 0x8018C9F0u;
    c->mem_w16((c->r[3] + (uint32_t)8), (uint16_t)c->r[2]); rec_dispatch(c, 0x801314B4u);
    c->r[31] = 0x8018C9F8u;
    c->r[4] = c->r[17] + c->r[0]; rec_dispatch(c, 0x8013892Cu);
    c->mem_w8((c->r[17] + kNodeSubState), (uint8_t)c->r[0]);
    c->mem_w8((c->r[17] + (uint32_t)6), (uint8_t)c->r[0]);
  L_8018CA00:;
    c->r[31] = c->mem_r32((c->r[29] + (uint32_t)32));
    c->r[19] = c->mem_r32((c->r[29] + (uint32_t)28));
    c->r[18] = c->mem_r32((c->r[29] + (uint32_t)24));
    c->r[17] = c->mem_r32((c->r[29] + (uint32_t)20));
    c->r[16] = c->mem_r32((c->r[29] + (uint32_t)16));
    c->r[29] = c->r[29] + (uint32_t)40; return;
    return;
}

void SubstateEdgeLeaves::registerOverrides(Game*) {
  { extern void ov_opn_gen_8018C820(Core*);
    overrides::install(0x8018C820u, "SubstateEdgeLeaves::opnAssemblyHook",
                       &SubstateEdgeLeaves::opnAssemblyHook, ov_opn_gen_8018C820, nullptr); }
  engine_set_override_a00(0x8012E8A8u, &SubstateEdgeLeaves::perChildTransformPropagate, ov_a00_gen_8012E8A8);
  engine_set_override_a00(0x8012ED84u, &SubstateEdgeLeaves::stateZeroInit, ov_a00_gen_8012ED84);
  engine_set_override_a00(0x8012F5B4u, &SubstateEdgeLeaves::substate1Tick, ov_a00_gen_8012F5B4);
  engine_set_override_a00(0x8012FD88u, &SubstateEdgeLeaves::substate2Tick, ov_a00_gen_8012FD88);
  engine_set_override_a00(0x80130524u, &SubstateEdgeLeaves::substate3Tick, ov_a00_gen_80130524);
  engine_set_override_a00(0x801313C4u, &SubstateEdgeLeaves::pendingCommandClear, ov_a00_gen_801313C4);
  engine_set_override_a00(0x80146348u, &SubstateEdgeLeaves::assemblyPostTick, ov_a00_gen_80146348);
  engine_set_override_a00(0x8012F494u, &SubstateEdgeLeaves::substate0Tick, ov_a00_gen_8012F494);
  engine_set_override_a00(0x80130AC4u, &SubstateEdgeLeaves::visibilityGate,        ov_a00_gen_80130AC4);
  engine_set_override_a00(0x801316CCu, &SubstateEdgeLeaves::tickChildOscillators,  ov_a00_gen_801316CC);
  engine_set_override_a00(0x80131134u, &SubstateEdgeLeaves::armPendingChildPair,   ov_a00_gen_80131134);
}
