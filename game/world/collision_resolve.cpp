// collision_resolve.cpp — PORT_GEN draft, byte-faithful transcription of gen_func_80023D48.
// Drafted by port_gen from gen_func_80023D48 (../../generated/shard_1.c:2502-2728).
// The machine-readable ORACLE markers live immediately above each method, not here: port_check caps
// the marker-to-definition gap at 60 lines and the constant tables below now exceed that.
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



// ─────────────────────────────────────────────────────────────────────────────────────────────────
// FIELD NAMES for the three records this function walks. r17/r22/r30 hold actor/other/anchor for the
// whole body (set in the prologue, callee-saved across all eight calls), so the base register is what
// identifies the record — which is why these are THREE separate tables and not one.
//
// That distinction is load-bearing, not tidiness: actor+0x30 is the 16.16 Y position while
// anchor+0x30 is the anchor's own Y, and +0x80/+0x84/+0x86 mean different things on the actor than
// on the other object. docs/findings/object.md records this hazard directly — "the offset is not the
// identity; the record is" — after a near-miss converting one struct's offsets with another's lens.
namespace {
// actor (a0, held in r17)
constexpr uint32_t kActorType         = 12;    // 0x0C — case 2 additionally updates the facing byte
constexpr uint32_t kActorLanded       = 41;    // 0x29 — set to 1 on the "landed on top" outcome
constexpr uint32_t kActorX            = 46;    // 0x2E
constexpr uint32_t kActorY32          = 48;    // 0x30 as a 32-bit 16.16 value (NOT anchor's +0x30)
constexpr uint32_t kActorY            = 50;    // 0x32 as the u16 half
constexpr uint32_t kActorZ            = 54;    // 0x36
constexpr uint32_t kActorAngle        = 86;    // 0x56 — rotates the sample offset when flags&1
constexpr uint32_t kActorSampleRadius = 124;   // 0x7C
constexpr uint32_t kActorYBias        = 126;   // 0x7E
constexpr uint32_t kActorHeightLo     = 128;   // 0x80
constexpr uint32_t kActorHeightHi     = 130;   // 0x82
constexpr uint32_t kActorExtentLo     = 132;   // 0x84
constexpr uint32_t kActorExtentHi     = 134;   // 0x86
constexpr uint32_t kActorFacing       = 95;    // 0x5F — written as angleCmp() + 2
constexpr uint32_t kActorFacingRef    = 96;    // 0x60
constexpr uint32_t kActorVelY         = 74;    // 0x4A, SIGNED — negative means launched upward
// other (a1, held in r22)
// The other object is itself an OBJECT RECORD, so it carries X/Y/Z at the same offsets the actor
// does. Named separately from the kActor* set because the ROLE differs even where the number does
// not — see the note above about the record being the identity, not the offset.
constexpr uint32_t kOtherX            = 46;    // 0x2E
constexpr uint32_t kOtherY            = 50;    // 0x32
constexpr uint32_t kOtherZ            = 54;    // 0x36
constexpr uint32_t kOtherRadius       = 128;   // 0x80
constexpr uint32_t kOtherExtentLo     = 132;   // 0x84
constexpr uint32_t kOtherExtentHi     = 134;   // 0x86
// anchor (a2, held in r30)
constexpr uint32_t kAnchorX           = 44;    // 0x2C
constexpr uint32_t kAnchorY           = 48;    // 0x30
constexpr uint32_t kAnchorZ           = 52;    // 0x34
}  // namespace

// ORACLE: gen_func_80023D48
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
    c->r[4] = (uint32_t)(int16_t)c->mem_r16((c->r[17] + kActorAngle));
    c->r[31] = 0x80023D94u;
     func_80083F50(c);
    c->r[3] = (uint32_t)(int16_t)c->mem_r16((c->r[17] + kActorSampleRadius));
    { int64_t _p = (int64_t)(int32_t)c->r[2] * (int64_t)(int32_t)c->r[3]; c->lo = (uint32_t)_p; c->hi = (uint32_t)((uint64_t)_p >> 32); }
    c->r[4] = (uint32_t)(int16_t)c->mem_r16((c->r[17] + kActorAngle));
    c->r[3] = c->lo;
    c->r[31] = 0x80023DB0u;
    c->r[16] = (uint32_t)((int32_t)c->r[3] >> 12); func_80083E80(c);
    c->r[3] = (uint32_t)(int16_t)c->mem_r16((c->r[17] + kActorSampleRadius));
    { int64_t _p = (int64_t)(int32_t)c->r[2] * (int64_t)(int32_t)c->r[3]; c->lo = (uint32_t)_p; c->hi = (uint32_t)((uint64_t)_p >> 32); }
    c->r[4] = (uint32_t)c->mem_r16((c->r[17] + kActorX));
    c->r[2] = (uint32_t)c->mem_r16((c->r[30] + kAnchorX));
    c->r[4] = c->r[4] + c->r[16];
    c->r[4] = c->r[4] - c->r[2];
    c->r[3] = c->lo;
    c->r[2] = c->r[4] << 16;
    c->r[2] = (uint32_t)((int32_t)c->r[2] >> 16);
    { int64_t _p = (int64_t)(int32_t)c->r[2] * (int64_t)(int32_t)c->r[2]; c->lo = (uint32_t)_p; c->hi = (uint32_t)((uint64_t)_p >> 32); }
    c->r[5] = (uint32_t)((int32_t)c->r[3] >> 12);
    c->r[3] = (uint32_t)c->mem_r16((c->r[17] + kActorZ));
    c->r[2] = (uint32_t)c->mem_r16((c->r[30] + kAnchorZ));
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
    c->r[2] = (uint32_t)(int16_t)c->mem_r16((c->r[17] + kActorHeightHi));
    c->r[3] = (uint32_t)(int16_t)c->mem_r16((c->r[17] + kActorHeightLo));
    c->r[4] = (uint32_t)(int16_t)c->mem_r16((c->r[22] + kOtherRadius));
    c->r[2] = c->r[2] - c->r[3];
    c->r[2] = c->r[2] + c->r[4];
    c->r[2] = (uint32_t)((int32_t)c->r[2] < (int32_t)c->r[5]);
    { int _t = (c->r[2] != c->r[0]); c->r[2] = c->r[0] + c->r[0]; if (_t) goto L_800240CC; }
    c->r[23] = (uint32_t)c->mem_r16((c->r[17] + kActorYBias));
    c->r[5] = (uint32_t)c->mem_r16((c->r[17] + kActorY));
    c->r[2] = (uint32_t)c->mem_r16((c->r[30] + kAnchorY));
    c->r[3] = (uint32_t)c->mem_r16((c->r[17] + kActorExtentHi));
    c->r[4] = (uint32_t)c->mem_r16((c->r[22] + kOtherExtentLo));
    c->r[5] = c->r[5] + c->r[23]; goto L_80023EF4;
  L_80023E60:;
    c->r[4] = (uint32_t)c->mem_r16((c->r[17] + kActorX));
    c->r[2] = (uint32_t)c->mem_r16((c->r[30] + kAnchorX));
    c->r[4] = c->r[4] - c->r[2];
    c->r[2] = c->r[4] << 16;
    c->r[2] = (uint32_t)((int32_t)c->r[2] >> 16);
    { int64_t _p = (int64_t)(int32_t)c->r[2] * (int64_t)(int32_t)c->r[2]; c->lo = (uint32_t)_p; c->hi = (uint32_t)((uint64_t)_p >> 32); }
    c->r[3] = (uint32_t)c->mem_r16((c->r[17] + kActorZ));
    c->r[2] = (uint32_t)c->mem_r16((c->r[30] + kAnchorZ));
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
    c->r[2] = (uint32_t)(int16_t)c->mem_r16((c->r[17] + kActorHeightHi));
    c->r[3] = (uint32_t)(int16_t)c->mem_r16((c->r[17] + kActorHeightLo));
    c->r[4] = (uint32_t)(int16_t)c->mem_r16((c->r[22] + kOtherRadius));
    c->r[2] = c->r[2] - c->r[3];
    c->r[2] = c->r[2] + c->r[4];
    c->r[2] = (uint32_t)((int32_t)c->r[2] < (int32_t)c->r[5]);
    { int _t = (c->r[2] != c->r[0]); c->r[2] = c->r[0] + c->r[0]; if (_t) goto L_800240CC; }
    c->r[5] = (uint32_t)c->mem_r16((c->r[17] + kActorY));
    c->r[2] = (uint32_t)c->mem_r16((c->r[30] + kAnchorY));
    c->r[3] = (uint32_t)c->mem_r16((c->r[17] + kActorExtentHi));
    c->r[4] = (uint32_t)c->mem_r16((c->r[22] + kOtherExtentLo));
  L_80023EF4:;
    c->r[5] = c->r[5] - c->r[2];
    c->r[2] = (uint32_t)c->mem_r16((c->r[17] + kActorExtentLo));
    c->r[2] = c->r[3] - c->r[2];
    c->r[4] = c->r[4] + c->r[2];
    c->r[16] = c->r[4] + c->r[0];
    c->r[4] = c->r[5] + c->r[4];
    c->r[4] = c->r[4] & 65535u;
    c->r[3] = c->r[3] << 16;
    c->r[2] = (uint32_t)(int16_t)c->mem_r16((c->r[22] + kOtherExtentHi));
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
    c->r[2] = (uint32_t)c->mem_r16((c->r[22] + kOtherExtentHi));
    c->r[4] = (uint32_t)c->mem_r16((c->r[22] + kOtherExtentLo));
    c->r[3] = (uint32_t)c->mem_r16((c->r[17] + kActorExtentLo));
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
    c->r[4] = (uint32_t)(int16_t)c->mem_r16((c->r[17] + kActorHeightHi));
    c->r[19] = (uint32_t)8064u << 16;
    c->mem_w32((c->r[19] + (uint32_t)156), c->r[2]);
    c->r[2] = (uint32_t)(int16_t)c->mem_r16((c->r[17] + kActorHeightLo));
    c->r[3] = (uint32_t)(int16_t)c->mem_r16((c->r[22] + kOtherRadius));
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
    c->r[3] = (uint32_t)c->mem_r8((c->r[17] + kActorType));
    { int _t = (c->r[3] != c->r[2]);  if (_t) goto L_80023FF8; }
    c->r[4] = (uint32_t)(int16_t)c->mem_r16((c->r[19] + (uint32_t)156));
    c->r[5] = (uint32_t)(int16_t)c->mem_r16((c->r[17] + kActorFacingRef));
    c->r[31] = 0x80023FF0u;
    c->r[6] = c->r[0] + (uint32_t)1; func_80077768(c);
    c->r[2] = c->r[2] + (uint32_t)2;
    c->mem_w8((c->r[17] + kActorFacing), (uint8_t)c->r[2]);
  L_80023FF8:;
    c->r[4] = c->mem_r32((c->r[19] + (uint32_t)156));
    c->r[31] = 0x80024004u;
     func_80083F50(c);
    c->r[3] = (uint32_t)(int16_t)c->mem_r16((c->r[17] + kActorHeightLo));
    c->r[4] = (uint32_t)(int16_t)c->mem_r16((c->r[22] + kOtherRadius));
    c->r[3] = c->r[3] + c->r[4];
    { int64_t _p = (int64_t)(int32_t)c->r[2] * (int64_t)(int32_t)c->r[3]; c->lo = (uint32_t)_p; c->hi = (uint32_t)((uint64_t)_p >> 32); }
    c->r[4] = c->mem_r32((c->r[19] + (uint32_t)156));
    c->r[8] = c->lo;
    c->r[31] = 0x80024028u;
    c->r[16] = (uint32_t)((int32_t)c->r[8] >> 12); func_80083E80(c);
    c->r[3] = (uint32_t)(int16_t)c->mem_r16((c->r[17] + kActorHeightLo));
    c->r[4] = (uint32_t)(int16_t)c->mem_r16((c->r[22] + kOtherRadius));
    c->r[3] = c->r[3] + c->r[4];
    { int64_t _p = (int64_t)(int32_t)c->r[2] * (int64_t)(int32_t)c->r[3]; c->lo = (uint32_t)_p; c->hi = (uint32_t)((uint64_t)_p >> 32); }
    c->r[8] = (uint32_t)c->mem_r16((c->r[29] + (uint32_t)16));
    c->r[3] = (uint32_t)c->mem_r16((c->r[30] + kAnchorX));
    c->r[2] = c->r[0] + (uint32_t)1;
    c->r[3] = c->r[3] + c->r[16];
    c->r[3] = c->r[3] - c->r[8];
    c->mem_w16((c->r[17] + kActorX), (uint16_t)c->r[3]);
    c->r[3] = (uint32_t)c->mem_r16((c->r[30] + kAnchorZ));
    c->r[8] = c->lo;
    c->r[4] = (uint32_t)((int32_t)c->r[8] >> 12);
    c->r[8] = (uint32_t)c->mem_r16((c->r[29] + (uint32_t)24));
    c->r[3] = c->r[3] - c->r[4];
    c->r[3] = c->r[3] - c->r[8];
    c->mem_w16((c->r[17] + kActorZ), (uint16_t)c->r[3]); goto L_800240CC;
  L_80024074:;
    c->r[2] = c->r[16] << 16;
    c->r[5] = (uint32_t)((int32_t)c->r[2] >> 16);
    { int _t = ((int32_t)c->r[5] <= 0); c->r[2] = c->r[0] + (uint32_t)3; if (_t) goto L_800240A4; }
    c->r[3] = c->r[23] << 16;
    c->r[4] = c->mem_r32((c->r[30] + kAnchorY));
    c->r[3] = (uint32_t)((int32_t)c->r[3] >> 16);
    c->r[4] = c->r[4] + c->r[5];
    c->r[4] = c->r[4] - c->r[3];
    c->r[4] = c->r[4] << 16;
    c->mem_w32((c->r[17] + kActorY32), c->r[4]); goto L_800240CC;
  L_800240A4:;
    c->r[2] = c->r[0] + (uint32_t)2;
    c->r[3] = c->mem_r32((c->r[30] + kAnchorY));
    c->r[4] = c->r[0] + (uint32_t)1;
    c->mem_w8((c->r[17] + kActorLanded), (uint8_t)c->r[4]);
    c->r[4] = c->r[23] << 16;
    c->r[4] = (uint32_t)((int32_t)c->r[4] >> 16);
    c->r[3] = c->r[3] + c->r[5];
    c->r[3] = c->r[3] - c->r[4];
    c->r[3] = c->r[3] << 16;
    c->mem_w32((c->r[17] + kActorY32), c->r[3]);
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


// ─────────────────────────────────────────────────────────────────────────────────────────────────
// ORACLE: gen_func_8002423C
// FUN_8002423C — actor-vs-object VERTICAL LANDING SNAP. The close relative of cylinderResolve above:
// same record shapes, same sqrt helper, but it answers "should this actor come to rest on top of
// that object" rather than pushing it out sideways.
//
// Three gates, each rejecting with v0 = -1: the actor's velY (+0x4A, SIGNED) must be >= 0, so a
// rising actor never lands; an XZ cylinder-overlap test via FUN_80084080; and a vertical-band test.
// On acceptance v0 = 2, the actor's Y is snapped onto the object's rest height and the landed flag
// at +0x29 is set.
//
// TWO THINGS HERE WOULD HAVE BEEN GOT WRONG BY HAND, and are right because port_gen took the body
// verbatim rather than anyone re-typing it:
//
//  1. The dx and dz DIFFERENCES are truncated to 16 bits and sign-extended before squaring
//     (`<<16 >>16` below). Both operands load UNSIGNED (lhu), so the natural rendering
//     `int dx = A.x - B.x;` is wrong for the negative world coordinates that are stored as u16 —
//     it squares a value near 65535 instead of one near -1.
//
//  2. v0 IS LIVE, not a formality. 28 of the 29 call sites discard it, but generated/
//     ov_a06_shard_0.c:2389 does `if (v0 == 2) mem_w8(r17 + 386, 0)` — zeroing the node-scan counter
//     to break its loop on a successful landing. A wrong v0 therefore changes overlay a06's control
//     flow and its guest writes, not merely a register compare.
//
// Both were caught by the adversarial verify pass over an RE spec that had them wrong; recorded here
// because the next person to touch this body will be tempted to "simplify" exactly those two things.
//
// BOTH RECORDS ARE OBJECT RECORDS. r16 = actor (a0), r17 = other (a1), and +0x2E/+0x32/+0x36 mean X/Y/Z
// on each — which is why kOtherX/Y/Z exist alongside kActorX/Y/Z at the same numeric offsets rather
// than one table being reused for both roles.
void CollisionResolve::landOnObjectTop(Core* c) {
    c->r[29] = c->r[29] + (uint32_t)-32;
    c->mem_w32((c->r[29] + (uint32_t)16), c->r[16]);
    c->r[16] = c->r[4] + c->r[0];
    c->mem_w32((c->r[29] + (uint32_t)24), c->r[31]);
    c->mem_w32((c->r[29] + (uint32_t)20), c->r[17]);
    c->r[2] = (uint32_t)(int16_t)c->mem_r16((c->r[16] + kActorVelY));
    { int _t = ((int32_t)c->r[2] < 0); c->r[17] = c->r[5] + c->r[0]; if (_t) goto L_80024320; }
    c->r[2] = (uint32_t)c->mem_r16((c->r[16] + kActorX));
    c->r[3] = (uint32_t)c->mem_r16((c->r[17] + kOtherX));
    c->r[2] = c->r[2] - c->r[3];
    c->r[2] = c->r[2] << 16;
    c->r[2] = (uint32_t)((int32_t)c->r[2] >> 16);
    { int64_t _p = (int64_t)(int32_t)c->r[2] * (int64_t)(int32_t)c->r[2]; c->lo = (uint32_t)_p; c->hi = (uint32_t)((uint64_t)_p >> 32); }
    c->r[2] = (uint32_t)c->mem_r16((c->r[16] + kActorZ));
    c->r[3] = (uint32_t)c->mem_r16((c->r[17] + kOtherZ));
    c->r[2] = c->r[2] - c->r[3];
    c->r[4] = c->lo;
    c->r[2] = c->r[2] << 16;
    c->r[2] = (uint32_t)((int32_t)c->r[2] >> 16);
    { int64_t _p = (int64_t)(int32_t)c->r[2] * (int64_t)(int32_t)c->r[2]; c->lo = (uint32_t)_p; c->hi = (uint32_t)((uint64_t)_p >> 32); }
    c->r[3] = c->lo;
    c->r[31] = 0x800242A8u;
    c->r[4] = c->r[4] + c->r[3]; func_80084080(c);
    c->r[3] = (uint32_t)(int16_t)c->mem_r16((c->r[16] + kActorHeightLo));
    c->r[4] = (uint32_t)(int16_t)c->mem_r16((c->r[17] + kOtherRadius));
    c->r[2] = c->r[2] & 65535u;
    c->r[3] = c->r[3] + c->r[4];
    c->r[3] = (uint32_t)((int32_t)c->r[3] < (int32_t)c->r[2]);
    { int _t = (c->r[3] != c->r[0]); c->r[2] = c->r[0] + (uint32_t)-1; if (_t) goto L_80024324; }
    c->r[4] = (uint32_t)c->mem_r16((c->r[16] + kActorY));
    c->r[6] = (uint32_t)c->mem_r16((c->r[17] + kOtherY));
    c->r[3] = (uint32_t)c->mem_r16((c->r[17] + kOtherExtentLo));
    c->r[2] = (uint32_t)c->mem_r16((c->r[16] + kActorExtentHi));
    c->r[5] = (uint32_t)c->mem_r16((c->r[16] + kActorExtentLo));
    c->r[4] = c->r[4] - c->r[6];
    c->r[3] = c->r[3] + c->r[2];
    c->r[5] = c->r[3] - c->r[5];
    c->r[4] = c->r[4] + c->r[5];
    c->r[4] = c->r[4] & 65535u;
    c->r[2] = c->r[2] << 16;
    c->r[3] = (uint32_t)(int16_t)c->mem_r16((c->r[17] + kOtherExtentHi));
    c->r[2] = (uint32_t)((int32_t)c->r[2] >> 16);
    c->r[2] = c->r[2] + c->r[3];
    c->r[2] = (uint32_t)((int32_t)c->r[2] < (int32_t)c->r[4]);
    { int _t = (c->r[2] != c->r[0]); c->r[2] = c->r[0] + (uint32_t)-1; if (_t) goto L_80024324; }
    c->r[2] = c->r[0] + (uint32_t)2;
    c->r[3] = c->r[6] - c->r[5];
    c->mem_w16((c->r[16] + kActorY), (uint16_t)c->r[3]);
    c->r[3] = c->r[0] + (uint32_t)1;
    c->mem_w8((c->r[16] + kActorLanded), (uint8_t)c->r[3]); goto L_80024324;
  L_80024320:;
    c->r[2] = c->r[0] + (uint32_t)-1;
  L_80024324:;
    c->r[31] = c->mem_r32((c->r[29] + (uint32_t)24));
    c->r[17] = c->mem_r32((c->r[29] + (uint32_t)20));
    c->r[16] = c->mem_r32((c->r[29] + (uint32_t)16));
    c->r[29] = c->r[29] + (uint32_t)32; return;
    return;
}

void CollisionResolve::registerOverrides(Game*) {
  engine_set_override_main(0x80023D48u, &CollisionResolve::cylinderResolve, gen_func_80023D48);
  engine_set_override_main(0x8002423Cu, &CollisionResolve::landOnObjectTop, gen_func_8002423C);
}
