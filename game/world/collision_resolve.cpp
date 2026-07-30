// collision_resolve.cpp — actor-vs-object CYLINDER COLLISION RESOLVE (guest FUN_80023D48) and its
// vertical relative, the landing snap (guest FUN_8002423C). Both are wired by address at the bottom
// of this file, so every caller — substrate included — reaches these bodies; the oracle leg runs
// gen_func_80023D48 / gen_func_8002423C.
//
// RE, field map and outcome codes: docs/re/collision-resolve-23d48.md.
//
// WHAT cylinderResolve DOES, in one paragraph. The actor is a vertical cylinder; the thing it may
// be hitting is described by a RADIUS on `other` and a POSITION on `anchor` (two records, because
// the caller resolves against a hitbox whose dimensions and whose placement live in different
// objects). Two gates run first: an XZ distance test against the summed radii, then a vertical band
// test. If both say "overlapping", the two penetration depths are compared and the SHALLOWER axis
// wins — a horizontal push-out onto the contact circle (outcome 1), or a vertical snap that either
// rests the actor on the top face (outcome 2, sets the landed flag) or pushes it out underneath
// (outcome 3). Guest Y grows DOWNWARD, which is why "resting on top" is the NEGATIVE snap.
//
// ─────────────────────────────────────────────────────────────────────────────────────────────────
// WHAT MAY AND MAY NOT BE FOLDED HERE (the readability pass, 2026-07-30 — read before editing)
//
// This body was a verbatim port_gen transcription; the pass that named its fields left the register
// dataflow alone, and this one lifts it. The rule the lift follows, and the evidence for it:
//
//   * CALLEE-SAVED registers (r16..r23, r30) and the ARGUMENT registers at each call site are
//     written to c->r[N] exactly where the gen body writes them. A callee may spill its caller's
//     callee-saved registers, and O32 lets a callee stash its incoming a0..a3 into the CALLER's
//     16-byte argument-save area — both would put a stale register value into guest RAM, which SBS
//     compares. GuestReg<N> proxies give those a name without moving them out of the register file.
//   * hi/lo go through guest_mult. They are guest-visible state (guest_abi.h §5).
//   * PURE SCRATCH (r2/r3/r8/r9, and the dead intermediate values of r4/r5 that no call and no
//     later statement reads) becomes ordinary named C++ locals. Audited 2026-07-30, transitively,
//     over all five callees and the one real nested call: rcos/sqrtLzc/ratan2/angleCmp descend NO
//     frame and write NO memory at all; rsin descends 24 and writes exactly one word (its own ra,
//     at its sp+16, i.e. BELOW our sp); its callee 0x80083EBC writes nothing. So nothing this
//     function calls can observe a t-register, and the one guest-stack word any of them writes is
//     the one the func_XXXX wrappers below exist to preserve.
//
// The equivalence gate for the lift is `port_check.py game/world/collision_resolve.cpp` (frame
// sizes, ordered call sites with their ra constants and targets, ordered store-width sequence).
// ─────────────────────────────────────────────────────────────────────────────────────────────────
#include "world/collision_resolve.h"
#include "core.h"
#include "game.h"
#include "guest_abi.h"           // GuestFrame / GuestReg / guest_call / guest_mult
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

// HOW THE ACTOR'S HORIZONTAL SIZE IS BUILT, recorded as OBSERVED rather than renamed: this function
// uses the actor's +0x80/+0x82 pair as XZ radii, not as a height band. The overlap GATE compares the
// distance against `(actor+0x82 - actor+0x80) + other+0x80`, while the push-out places the actor on a
// contact circle of radius `actor+0x80 + other+0x80` — two different radii from the same pair.
// FUN_8002423C below independently uses `actor+0x80 + other+0x80` as its XZ reach. The kActorHeight*
// names are kept because no source (USER or guest data) has been found for what the pair really is;
// an offset is a fact, a field name is a claim. See docs/re/collision-resolve-23d48.md.

// ── this function's own guest stack ──────────────────────────────────────────────────────────────
// Frame contract from `abi_extract.py 0x80023D48 --scaffold --guestabi`, program order. Not
// hand-derived; regenerate if the gen body changes.
constexpr uint32_t kResolveFrameBytes = 80;
constexpr GuestFrameSpill kResolveSpills[10] = {
  { 17, 44 }, { 22, 64 }, { 30, 72 }, { 31 /*ra*/, 76 }, { 23, 68 },
  { 21, 60 }, { 20, 56 }, { 19, 52 }, { 18, 48 }, { 16, 40 },
};
// Three s16 locals in that frame. The facing-offset entry writes the offset it applied and the raw
// X delta; the plain entry writes zeroes for the offsets. All three are read back at the push-out,
// which is why the offset has to be subtracted out of the final position again.
constexpr uint32_t kLocalSampleOffsetX = 16;
constexpr uint32_t kLocalSampleOffsetZ = 24;
constexpr uint32_t kLocalDeltaX        = 32;

// ── the leaves ───────────────────────────────────────────────────────────────────────────────────
// Called through their generated func_XXXX wrappers, NEVER the Trig methods — see the header banner
// for why that is load-bearing rather than laziness. One ra constant per jal site, from the same
// abi_extract contract.
constexpr uint32_t kRaSampleCos    = 0x80023D94u;   // Trig::rcos    0x80083F50
constexpr uint32_t kRaSampleSin    = 0x80023DB0u;   // Trig::rsin    0x80083E80
constexpr uint32_t kRaSampleDist   = 0x80023E1Cu;   // Math::sqrtLzc 0x80084080
constexpr uint32_t kRaRawDist      = 0x80023EBCu;   // Math::sqrtLzc 0x80084080
constexpr uint32_t kRaContactAngle = 0x80023F88u;   // Trig::ratan2  0x80085690
constexpr uint32_t kRaFacingCmp    = 0x80023FF0u;   // Trig::angleCmp 0x80077768
constexpr uint32_t kRaPushCos      = 0x80024004u;   // Trig::rcos
constexpr uint32_t kRaPushSin      = 0x80024028u;   // Trig::rsin

// The contact angle is published to the scratchpad, where the caller reads it back.
constexpr uint32_t kScratchpadBase    = 0x1F800000u;
constexpr uint32_t kContactAngleSlot  = 156;        // 0x1F80009C

// v0 is an outcome code, not a boolean.
constexpr uint32_t kNoCollision   = 0;
constexpr uint32_t kPushedOutInXz = 1;
constexpr uint32_t kLandedOnTop   = 2;
constexpr uint32_t kSnappedInY    = 3;

// The one actor type that also re-aims its facing byte at the contact angle.
constexpr uint32_t kActorTypeTracksFacing = 2;

// A guest record reached through the register that identifies it — the base is re-read from c->r[N]
// on every access, exactly as the guest does, so the lens can never go stale across a call.
struct Rec {
  Core* c;
  int   reg;
  uint32_t base() const { return c->r[reg]; }
  uint32_t u8 (uint32_t field) const { return c->mem_r8 (base() + field); }
  uint32_t u16(uint32_t field) const { return c->mem_r16(base() + field); }
  int32_t  s16(uint32_t field) const { return (int16_t)c->mem_r16(base() + field); }
  uint32_t u32(uint32_t field) const { return c->mem_r32(base() + field); }
};

// ── FUN_8002423C's own frame ─────────────────────────────────────────────────────────────────────
constexpr uint32_t kLandFrameBytes = 32;
constexpr GuestFrameSpill kLandSpills[3] = { { 16, 16 }, { 31 /*ra*/, 24 }, { 17, 20 } };
constexpr uint32_t kRaLandDist   = 0x800242A8u;     // Math::sqrtLzc 0x80084080
constexpr uint32_t kLandRejected = 0xFFFFFFFFu;     // v0 = -1

// ─────────────────────────────────────────────────────────────────────────────────────────────────
// FUN_8001F40C — classifyBodyContact's own tables. Two more actor fields than the pair above needs,
// both already named elsewhere in the port; the citations are what make them claims with a source.
constexpr uint32_t kActorStateFlags   = 382;   // 0x17E — game/player/actor_tomba.h's "G+0x17E"
constexpr uint32_t kActorPausedBit    = 0x200; // the same bit ActorTomba::interactWalk early-outs on
constexpr uint32_t kActorWalkState    = 325;   // 0x145 — the walk/jump state byte ActorTomba::
                                               // growthYSnap and type8Interact clear together with
                                               // +0x4A/+0x50/+0x148. Only `== 1` is tested here.

// Two more shared scratchpad words, alongside kContactAngleSlot above.
constexpr uint32_t kContactLockSlot = 152;     // 0x1F800098 — the "current lock owner" word
                                               // ActorMeleeEngage::doIt compares against 1 and
                                               // ActorTomba::framePreTick republishes every frame
                                               // from the actor's landed flag (+0x29). Non-zero =
                                               // some other interaction owns the actor this frame.
constexpr uint32_t kContactKindSlot = 595;     // 0x1F800253 — a per-frame contact-kind byte.
                                               // OBSERVED, not sourced: ActorTomba::framePreTick
                                               // (gen_func_8002288C) masks it `&= 3` once per frame,
                                               // FUN_800541F4 writes 1/3 there, and its readers
                                               // (e.g. gen_func_80060C60) gate on `< 2`. Writing 4
                                               // therefore parks it outside every `< 2` reader for
                                               // the rest of the frame. No source names this slot,
                                               // so the constant is named for what this body does
                                               // with it and nothing more.
constexpr uint32_t kContactKindUndersideBump = 4;

// ── this function's own guest stack ──────────────────────────────────────────────────────────────
// From `abi_extract.py 0x8001F40C --scaffold --guestabi`, program order. Not hand-derived.
constexpr uint32_t kClassifyFrameBytes = 56;
constexpr GuestFrameSpill kClassifySpills[10] = {
  { 17, 20 }, { 20, 32 }, { 31 /*ra*/, 52 }, { 30, 48 }, { 23, 44 },
  { 22, 40 }, { 21, 36 }, { 19, 28 }, { 18, 24 }, { 16, 16 },
};
constexpr uint32_t kRaBodyDist         = 0x8001F48Cu;   // Math::sqrtLzc 0x80084080
constexpr uint32_t kRaBodyContactAngle = 0x8001F500u;   // Trig::ratan2  0x80085690

// v0 is a two-bit classification, and BOTH bits are read by callers:
//   bit 0 — WHICH AXIS is the shorter way out: 0 = push apart in XZ, 1 = resolve in Y.
//           ActorTomba::type8Interact switches on exactly `v0 & 1`.
//   bit 1 — WHICH SIDE the actor is on: clear = clearly above the other body, set = level or below.
//           ActorTomba::stepModeInteract's "just transitioned" branch tests exactly `v0 < 2`.
// -1 is the miss, and every caller tests it as `(int32_t)v0 < 0` before looking at the bits.
constexpr uint32_t kNoContact            = 0xFFFFFFFFu;
constexpr uint32_t kAboveResolveInXz     = 0;
constexpr uint32_t kAboveResolveInY      = 1;
constexpr uint32_t kBelowResolveInXz     = 2;
constexpr uint32_t kBelowResolveInY      = 3;
}  // namespace

// ORACLE: gen_func_80023D48
void CollisionResolve::cylinderResolve(Core* c) {
  const uint32_t actorArg  = c->r[4];
  const uint32_t otherArg  = c->r[5];
  const uint32_t anchorArg = c->r[6];

  GuestFrame<kResolveFrameBytes, 10> frame(c, kResolveSpills);
  c->r[17] = actorArg;
  c->r[22] = otherArg;
  c->r[30] = anchorArg;
  c->r[7]  = c->r[7] & 1u;   // gen narrows a3 in place and never writes it again; a3 stays live
                             // into every callee, so the narrowing has to happen on the register
  const bool sampleAtFacingOffset = c->r[7] != 0;

  const Rec actor { c, 17 };
  const Rec other { c, 22 };
  const Rec anchor{ c, 30 };

  // Values the guest keeps in callee-saved registers because they outlive a call.
  GuestReg<19> deltaZ(c);       // s3 — sampled point -> anchor, on Z
  GuestReg<21> xzDistance(c);   // s5 — |sampled point -> anchor| in XZ, from Math::sqrtLzc
  GuestReg<23> yBias(c);        // s7 — actor +0x7E on the facing-offset entry, 0 on the raw entry

  // ── entry geometry ─────────────────────────────────────────────────────────────────────────────
  // flags&1 picks which point is tested. Set: a point pushed `sampleRadius` out along the actor's
  // facing angle (used when the caller is resolving the actor's leading edge rather than its
  // centre). Clear: the actor's own position, and no offset to undo later.
  if (sampleAtFacingOffset) {
    GuestReg<16> sampleOffsetX(c);   // s0 — live across the rsin call below

    c->r[4] = (uint32_t)actor.s16(kActorAngle);                        // a0 = facing angle
    guest_call(c, kRaSampleCos, func_80083F50);
    guest_mult(c, (int32_t)c->r[2], actor.s16(kActorSampleRadius));
    c->r[4] = (uint32_t)actor.s16(kActorAngle);                        // a0 = facing angle
    sampleOffsetX = (uint32_t)((int32_t)c->lo >> 12);
    guest_call(c, kRaSampleSin, func_80083E80);
    guest_mult(c, (int32_t)c->r[2], actor.s16(kActorSampleRadius));
    const int32_t sinTimesRadius = (int32_t)c->lo;

    // World coordinates are stored as u16, so each delta is truncated to 16 bits and sign-extended
    // BEFORE squaring — squaring the raw difference would square a value near 65535 instead of -1.
    const uint32_t dx = actor.u16(kActorX) + sampleOffsetX - anchor.u16(kAnchorX);
    guest_mult(c, (int16_t)dx, (int16_t)dx);
    const uint32_t deltaXSq = c->lo;

    const uint32_t sampleOffsetZ = (uint32_t)(sinTimesRadius >> 12);
    const uint32_t dz = actor.u16(kActorZ) - sampleOffsetZ - anchor.u16(kAnchorZ);
    c->r[6] = deltaXSq;              // gen leaves dx² in a2, and it is still there at the ratan2
                                     // call further down — keep it on the register, not in a local
    guest_mult(c, (int16_t)dz, (int16_t)dz);

    c->mem_w16(c->r[29] + kLocalSampleOffsetX, (uint16_t)sampleOffsetX);
    c->mem_w16(c->r[29] + kLocalDeltaX,        (uint16_t)dx);
    c->mem_w16(c->r[29] + kLocalSampleOffsetZ, (uint16_t)sampleOffsetZ);

    deltaZ  = dz;
    c->r[5] = sampleOffsetZ;         // a1 still holds the Z offset at the call below
    c->r[4] = deltaXSq + c->lo;      // a0 = dx² + dz²
    guest_call(c, kRaSampleDist, func_80084080);
    xzDistance = c->r[2];
    yBias      = actor.u16(kActorYBias);
  } else {
    const uint32_t dx = actor.u16(kActorX) - anchor.u16(kAnchorX);
    guest_mult(c, (int16_t)dx, (int16_t)dx);
    const uint32_t deltaXSq = c->lo;
    const uint32_t dz = actor.u16(kActorZ) - anchor.u16(kAnchorZ);
    guest_mult(c, (int16_t)dz, (int16_t)dz);

    yBias = 0u;                      // no offset was applied, so nothing to bias the Y test by
    c->mem_w16(c->r[29] + kLocalSampleOffsetX, 0u);
    c->mem_w16(c->r[29] + kLocalSampleOffsetZ, 0u);
    c->mem_w16(c->r[29] + kLocalDeltaX, (uint16_t)dx);

    deltaZ  = dz;
    c->r[5] = deltaXSq;              // a1 leftover, exactly as gen leaves it
    c->r[4] = deltaXSq + c->lo;      // a0 = dx² + dz²
    guest_call(c, kRaRawDist, func_80084080);
    xzDistance = c->r[2];
  }

  // ── gate 1: XZ overlap ─────────────────────────────────────────────────────────────────────────
  const int32_t xzReach = actor.s16(kActorHeightHi) - actor.s16(kActorHeightLo) + other.s16(kOtherRadius);
  if (xzReach < (int32_t)(xzDistance & 0xFFFFu)) {
    c->r[2] = kNoCollision;
    return;
  }

  // ── gate 2: vertical band ──────────────────────────────────────────────────────────────────────
  // restOffsetOnTop is how far above the anchor the actor sits when it is resting on the top face.
  const uint32_t deltaY = actor.u16(kActorY) + yBias - anchor.u16(kAnchorY);
  const uint32_t restOffsetOnTop =
      other.u16(kOtherExtentLo) + (actor.u16(kActorExtentHi) - actor.u16(kActorExtentLo));
  const uint32_t bandProbe = (deltaY + restOffsetOnTop) & 0xFFFFu;
  const int32_t  bandSpan  = actor.s16(kActorExtentHi) + other.s16(kOtherExtentHi);
  if (bandSpan < (int32_t)bandProbe) {
    c->r[2] = kNoCollision;
    return;
  }

  // ── which way out ──────────────────────────────────────────────────────────────────────────────
  // Split on the sign of Δy. Guest Y grows DOWNWARD, so Δy < 0 means the actor is ABOVE the anchor
  // and the resolve is a snap UP onto the top face (a negative offset); Δy >= 0 means it is below,
  // and it gets pushed further under. verticalReach stays positive on both sides — it is the
  // separation the two bands may still have and count as touching.
  GuestReg<16> verticalSnap(c);    // s0 — SIGNED: < 0 rest on top, > 0 pushed underneath
  GuestReg<18> verticalGap(c);     // s2 — |Δy|
  GuestReg<20> verticalReach(c);   // s4
  verticalSnap  = restOffsetOnTop;
  verticalGap   = deltaY;
  verticalReach = restOffsetOnTop;
  if ((int16_t)deltaY < 0) {
    verticalGap  = 0u - deltaY;
    verticalSnap = 0u - restOffsetOnTop;
  } else {
    verticalSnap  = actor.u16(kActorExtentLo) + (other.u16(kOtherExtentHi) - other.u16(kOtherExtentLo));
    verticalReach = verticalSnap;
  }

  // The contact angle from the anchor to the sampled point, published to the scratchpad for the
  // caller. Z is negated because the guest's ratan2 takes (y, x) in screen-ish orientation.
  const int32_t contactDz = (int16_t)(uint32_t)deltaZ;
  c->r[4] = (uint32_t)(-contactDz);                                            // a0
  c->r[5] = (uint32_t)(int16_t)c->mem_r16(c->r[29] + kLocalDeltaX);            // a1
  guest_call(c, kRaContactAngle, func_80085690);
  GuestReg<19> scratchpad(c);      // s3 — dz is consumed, and gen reuses the register as the base
  scratchpad = kScratchpadBase;
  c->mem_w32(scratchpad + kContactAngleSlot, c->r[2]);

  // Resolve along whichever axis is LESS deeply penetrated — the shorter way out.
  const int32_t xzPenetration =
      (actor.s16(kActorHeightHi) - actor.s16(kActorHeightLo) + other.s16(kOtherRadius))
      - (int32_t)(int16_t)(uint32_t)xzDistance;
  const int32_t yPenetration =
      (int32_t)(int16_t)(uint32_t)verticalReach - (int32_t)(int16_t)(uint32_t)verticalGap;

  if (xzPenetration < yPenetration) {
    // ── horizontal push-out: put the actor back on the contact circle around the anchor ──────────
    if (actor.u8(kActorType) == kActorTypeTracksFacing) {
      c->r[4] = (uint32_t)(int16_t)c->mem_r16(scratchpad + kContactAngleSlot);  // a0 = contact angle
      c->r[5] = (uint32_t)actor.s16(kActorFacingRef);                           // a1 = reference
      c->r[6] = 1u;                                                             // a2
      guest_call(c, kRaFacingCmp, func_80077768);
      c->mem_w8(actor.base() + kActorFacing, (uint8_t)(c->r[2] + 2));
    }

    const int32_t contactRadius = actor.s16(kActorHeightLo) + other.s16(kOtherRadius);
    GuestReg<16> contactOffsetX(c);   // s0 — live across the rsin call

    c->r[4] = c->mem_r32(scratchpad + kContactAngleSlot);                       // a0 = contact angle
    guest_call(c, kRaPushCos, func_80083F50);
    guest_mult(c, (int32_t)c->r[2], contactRadius);
    c->r[4] = c->mem_r32(scratchpad + kContactAngleSlot);                       // a0 = contact angle
    contactOffsetX = (uint32_t)((int32_t)c->lo >> 12);
    guest_call(c, kRaPushSin, func_80083E80);
    guest_mult(c, (int32_t)c->r[2], contactRadius);
    const uint32_t contactOffsetZ = (uint32_t)((int32_t)c->lo >> 12);

    // The facing offset applied on the way in has to come back out of the position written.
    const uint32_t undoOffsetX = c->mem_r16(c->r[29] + kLocalSampleOffsetX);
    c->mem_w16(actor.base() + kActorX,
               (uint16_t)(anchor.u16(kAnchorX) + contactOffsetX - undoOffsetX));
    const uint32_t undoOffsetZ = c->mem_r16(c->r[29] + kLocalSampleOffsetZ);
    c->mem_w16(actor.base() + kActorZ,
               (uint16_t)(anchor.u16(kAnchorZ) - contactOffsetZ - undoOffsetZ));
    c->r[2] = kPushedOutInXz;
    return;
  }

  // ── vertical resolve: sit the actor at the snap offset from the anchor's own Y ─────────────────
  const int32_t snap    = (int16_t)(uint32_t)verticalSnap;
  const int32_t biasOut = (int16_t)(uint32_t)yBias;
  if (snap > 0) {
    c->mem_w32(actor.base() + kActorY32,
               (anchor.u32(kAnchorY) + (uint32_t)snap - (uint32_t)biasOut) << 16);
    c->r[2] = kSnappedInY;
    return;
  }
  // snap <= 0: the actor came to rest ON TOP of the anchor, which is what "landed" means.
  c->mem_w8(actor.base() + kActorLanded, 1u);
  c->mem_w32(actor.base() + kActorY32,
             (anchor.u32(kAnchorY) + (uint32_t)snap - (uint32_t)biasOut) << 16);
  c->r[2] = kLandedOnTop;
}


// ─────────────────────────────────────────────────────────────────────────────────────────────────
// FUN_8002423C — actor-vs-object VERTICAL LANDING SNAP. The close relative of cylinderResolve above:
// same record shapes, same sqrt helper, but it answers "should this actor come to rest on top of
// that object" rather than pushing it out sideways.
//
// Three gates, each rejecting with v0 = -1: the actor's velY (+0x4A, SIGNED) must be >= 0, so a
// rising actor never lands; an XZ cylinder-overlap test via Math::sqrtLzc; and a vertical-band test.
// On acceptance v0 = 2, the actor's Y is snapped onto the object's rest height and the landed flag
// at +0x29 is set.
//
// TWO THINGS HERE WOULD HAVE BEEN GOT WRONG BY HAND, and are right because port_gen took the body
// verbatim rather than anyone re-typing it:
//
//  1. The dx and dz DIFFERENCES are truncated to 16 bits and sign-extended before squaring. Both
//     operands load UNSIGNED (lhu), so the natural rendering `int dx = A.x - B.x;` is wrong for the
//     negative world coordinates that are stored as u16 — it squares a value near 65535 instead of
//     one near -1.
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
//
// ORACLE: gen_func_8002423C
void CollisionResolve::landOnObjectTop(Core* c) {
  const uint32_t actorArg = c->r[4];
  const uint32_t otherArg = c->r[5];

  GuestFrame<kLandFrameBytes, 3> frame(c, kLandSpills);
  c->r[16] = actorArg;
  c->r[17] = otherArg;

  const Rec actor{ c, 16 };
  const Rec other{ c, 17 };

  // gate 1 — a rising actor never lands on anything.
  if (actor.s16(kActorVelY) < 0) {
    c->r[2] = kLandRejected;
    return;
  }

  // gate 2 — XZ cylinder overlap. Same u16 truncate-then-sign-extend as cylinderResolve.
  const uint32_t dx = actor.u16(kActorX) - other.u16(kOtherX);
  guest_mult(c, (int16_t)dx, (int16_t)dx);
  const uint32_t deltaXSq = c->lo;
  const uint32_t dz = actor.u16(kActorZ) - other.u16(kOtherZ);
  guest_mult(c, (int16_t)dz, (int16_t)dz);
  c->r[4] = deltaXSq + c->lo;                    // a0 = dx² + dz²
  guest_call(c, kRaLandDist, func_80084080);
  const uint32_t xzDistance = c->r[2] & 0xFFFFu;
  const int32_t  xzReach    = actor.s16(kActorHeightLo) + other.s16(kOtherRadius);
  if (xzReach < (int32_t)xzDistance) {
    c->r[2] = kLandRejected;
    return;
  }

  // gate 3 — vertical band. restOffset is where the actor's origin ends up when it rests on top.
  const uint32_t restOffset =
      (other.u16(kOtherExtentLo) + actor.u16(kActorExtentHi)) - actor.u16(kActorExtentLo);
  const uint32_t bandProbe = (actor.u16(kActorY) - other.u16(kOtherY) + restOffset) & 0xFFFFu;
  const int32_t  bandSpan  = actor.s16(kActorExtentHi) + other.s16(kOtherExtentHi);
  if (bandSpan < (int32_t)bandProbe) {
    c->r[2] = kLandRejected;
    return;
  }

  c->mem_w16(actor.base() + kActorY, (uint16_t)(other.u16(kOtherY) - restOffset));
  c->mem_w8(actor.base() + kActorLanded, 1u);
  c->r[2] = kLandedOnTop;
}

// ─────────────────────────────────────────────────────────────────────────────────────────────────
// FUN_8001F40C — the actor-vs-BODY CONTACT CLASSIFIER. The third member of this family, and the one
// the other two are not: it decides HOW a contact should be resolved and hands the answer back, it
// does not (mostly) resolve it. 4,275 substrate dispatches per replays/bugs/seesaw-weight.pad run.
//
// WHAT IT DOES, in game terms. Tomba walks into a solid thing — a crate, a block, an enemy's body.
// Both are cylinders with a vertical band. This asks three questions in order: are they overlapping
// at all (XZ radii, then vertical band — either miss returns -1); which way is the SHORTER way out
// (compare the XZ penetration against the Y penetration); and which SIDE of the thing he is on (is
// he clearly above its middle, or level with / under it). The answer is packed into v0's two bits
// and the contact HEADING is published to the shared scratchpad word 0x1F80009C, because the caller
// is the one that performs the horizontal push — it reads that heading back through Trig::rsin/rcos
// and rotates the two apart itself (ActorTomba::stepModeInteract and ::type8Interact both do exactly
// that, and game/player/actor_tomba.h documents the shape).
//
// The ONE resolve it does perform itself is the one the caller cannot express: when he is level or
// below and Y is the shorter way out, he has BUMPED HIS HEAD on the thing's underside — his Y is
// snapped to rest against that underside and, if he was still travelling upward (+0x4A signed, < 0
// means rising), that upward velocity is cancelled. Jump into the bottom of a platform, stop dead.
// That snap is skipped entirely when a2 is non-zero, when he is paused, or when another interaction
// already owns him this frame (the 0x1F800098 lock word) — the classification is still returned.
//
// HOW IT WAS IDENTIFIED — from the CALLERS and from a sibling, not from the shape. A distance test
// against two radii could be almost anything:
//
//  1. THE GROWN-STATE SIBLING, FUN_8001EC3C. game/player/actor_tomba.h already records that
//     ActorTomba::type8Interact picks FUN_8001EC3C when Tomba is GROWN (+0x17E bit 0x8000) and
//     FUN_8001F40C when he is not. Ghidra headless on both together (scratch/decomp/
//     prox_class_1f40c.c) shows them running the SAME two gates against the SAME field offsets and
//     publishing to the SAME 0x1F80009C — except 8001EC3C then calls rcos/rsin itself, writes the
//     actor's +0x2E/+0x36, and returns 1. So the pair is one mechanism with the push-out inlined on
//     one side and delegated to the caller on the other. That is what fixes 0x1F80009C's role here
//     as an OUTPUT the caller consumes, rather than incidental scratch.
//  2. THE CALLERS AGREE WITH THE BIT LAYOUT. FUN_800205CC (ActorTomba::type8Interact) branches on
//     `v0 & 1` — even = rotate the two apart along the published heading, odd = the standing/landing
//     path. FUN_80020364 (::stepModeInteract) gates its "just transitioned" branch on `v0 < 2`.
//     FUN_80023548 is a two-line wrapper: classify, and on any non-negative code forward the code
//     itself to FUN_8001FF7C. Three independent callers, three different bits, all consistent.
//  3. THE FIELD MAP IS THE ONE ALREADY PROVEN HERE. +0x2E/+0x32/+0x36, +0x80, +0x84/+0x86 and the
//     signed +0x4A are the same offsets cylinderResolve and landOnObjectTop above use, in the same
//     roles, on the same kind of record — which is why this body reuses their tables rather than
//     re-deriving a third set.
//
// WHY THIS IS AN ORDINARY PORT AND NOT PlatformHle: no MMIO, no completion-flag poll, no spin, no
// IRQ wait. Two calls, both to already-owned pure math leaves. It is arithmetic over guest records.
//
// EXTENT, established before anything was diffed. MAIN.EXE's own dispatch table
// (generated/shard_disp.c) has consecutive entries 0x8001F40C and 0x8001F650 with nothing between,
// so the body is [0x8001F40C, 0x8001F650) — 145 instructions, one entry, no jump table. Ghidra's
// independent function boundary agrees. Do NOT be misled by generated/shard_3.c, which contains a
// second copy of this body's tail (labels L_8001F58C / L_8001F620) inside gen_func_80010A08: that
// is linear-sweep output over a DATA region — the block ahead of it is `/* UNHANDLED special */`
// garbage ending in `rec_dispatch(c, r0); return;`, and nothing branches to those labels. It is
// dead emitted code, not a folded sibling entry point.
//
// FOLDING RULES: identical to cylinderResolve's banner at the top of this file, and the callee audit
// it cites covers this body too — the only two callees here are Math::sqrtLzc (0x80084080) and
// Trig::ratan2 (0x80085690), both of which descend NO frame and write NO memory, so no t-register
// value of ours is observable and the pure scratch becomes ordinary locals.
//
// ORACLE: gen_func_8001F40C
void CollisionResolve::classifyBodyContact(Core* c) {
  const uint32_t actorArg        = c->r[4];
  const uint32_t otherArg        = c->r[5];
  const uint32_t suppressSnapArg = c->r[6];

  GuestFrame<kClassifyFrameBytes, 10> frame(c, kClassifySpills);
  c->r[17] = actorArg;
  c->r[20] = otherArg;
  // gen loads a2 into s8 further down, between the second mult and the sqrt call. Hoisted to here
  // because nothing in between reads s8 and no call happens in between; it must simply be live by
  // the sqrt call, which abi_extract's call-site report confirms.
  c->r[30] = suppressSnapArg;

  const Rec actor{ c, 17 };
  const Rec other{ c, 20 };

  // Values the guest keeps in callee-saved registers because they outlive a call.
  GuestReg<19> deltaX(c);         // s3 — actor -> other, on X; live into ratan2
  GuestReg<18> deltaZ(c);         // s2 — actor -> other, on Z; live into ratan2
  GuestReg<22> xzDistance(c);     // s6 — |actor -> other| in XZ, from Math::sqrtLzc
  GuestReg<16> deltaY(c);         // s0
  GuestReg<21> verticalReach(c);  // s5 — how far apart the two bands may be and still touch
  GuestReg<23> verticalGap(c);    // s7 — |Δy|
  GuestReg<30> suppressSnap(c);   // s8 — a2: classify only, do not touch the actor

  // World coordinates are stored as u16, so each delta is truncated to 16 bits and sign-extended
  // BEFORE squaring — squaring the raw difference would square a value near 65535 instead of -1.
  // (The same trap the landOnObjectTop banner records; it is the reason both bodies came from
  // port_gen rather than from anyone re-typing the arithmetic.)
  deltaX = (uint32_t)(int32_t)(int16_t)(actor.u16(kActorX) - other.u16(kOtherX));
  guest_mult(c, (int32_t)(uint32_t)deltaX, (int32_t)(uint32_t)deltaX);
  const uint32_t deltaXSq = c->lo;
  deltaZ = (uint32_t)(int32_t)(int16_t)(actor.u16(kActorZ) - other.u16(kOtherZ));
  guest_mult(c, (int32_t)(uint32_t)deltaZ, (int32_t)(uint32_t)deltaZ);
  c->r[4] = deltaXSq + c->lo;                    // a0 = dx² + dz²
  guest_call(c, kRaBodyDist, func_80084080);
  xzDistance = c->r[2];

  // ── gate 1: XZ overlap ─────────────────────────────────────────────────────────────────────────
  const int32_t xzReach = actor.s16(kActorHeightLo) + other.s16(kOtherRadius);
  if (xzReach < (int32_t)((uint32_t)xzDistance & 0xFFFFu)) {
    c->r[2] = kNoContact;
    return;
  }

  // ── gate 2: vertical band ──────────────────────────────────────────────────────────────────────
  // restOffsetUnder is the band separation to use while the actor is at or below the other body;
  // the "above" case replaces it below, once the sign of Δy is known.
  deltaY = actor.u16(kActorY) - other.u16(kOtherY);
  const uint32_t restOffsetUnder = other.u16(kOtherExtentLo) + actor.u16(kActorExtentLo);
  verticalReach = restOffsetUnder;
  const uint32_t bandProbe = ((uint32_t)deltaY + restOffsetUnder) & 0xFFFFu;
  const int32_t  bandSpan  = actor.s16(kActorExtentHi) + other.s16(kOtherExtentHi);
  verticalGap = deltaY;            // gen sets s7 in the branch delay slot, i.e. unconditionally
  if (bandSpan < (int32_t)bandProbe) {
    c->r[2] = kNoContact;
    return;
  }

  // ── the contact heading, published for the caller to push along ────────────────────────────────
  // Z is negated because the guest's ratan2 takes (y, x) in screen-ish orientation — same convention
  // as cylinderResolve's own contact-angle call.
  c->r[4] = 0u - (uint32_t)deltaZ;               // a0
  c->r[5] = (uint32_t)deltaX;                    // a1
  guest_call(c, kRaBodyContactAngle, func_80085690);
  c->mem_w32(kScratchpadBase + kContactAngleSlot, c->r[2]);

  // Note the UNSIGNED reads here: gate 1 compared the radii as s16, the penetration uses them as
  // u16. gen genuinely does both, and the difference matters for a negative radius.
  const uint32_t radiusSum      = actor.u16(kActorHeightLo) + other.u16(kOtherRadius);
  const uint32_t xzPenetration  = radiusSum - (uint32_t)xzDistance;
  const int32_t  deltaYs16      = (int16_t)(uint32_t)deltaY;

  if (deltaYs16 < 0) {
    // Guest Y grows DOWNWARD, so Δy < 0 is the actor ABOVE the other body.
    verticalGap = 0u - (uint32_t)deltaY;
  } else {
    verticalReach = (other.u16(kOtherExtentHi) - other.u16(kOtherExtentLo))
                  + (actor.u16(kActorExtentHi) - actor.u16(kActorExtentLo));
  }

  // ── which side, and which way out ──────────────────────────────────────────────────────────────
  // "Clearly above" is not simply Δy < 0: the guest allows the actor to sink half its own lower
  // extent into the other body before it stops counting as an approach from above. The halving is a
  // signed divide by two rounding toward zero, which is why the sign bit is added back in first.
  const uint32_t actorExtentLo = actor.u16(kActorExtentLo);
  const uint32_t extentLoSign  = (actorExtentLo << 16) >> 31;
  const int32_t  halfExtentLo  = ((int32_t)(int16_t)actorExtentLo + (int32_t)extentLoSign) >> 1;
  const uint32_t yPenetration  = (uint32_t)verticalReach - (uint32_t)verticalGap;
  const bool xzIsShorterWayOut = (int16_t)xzPenetration < (int16_t)yPenetration;

  if (deltaYs16 < -halfExtentLo) {
    c->r[2] = xzIsShorterWayOut ? kAboveResolveInXz : kAboveResolveInY;
    return;
  }
  if (xzIsShorterWayOut) {
    c->r[2] = kBelowResolveInXz;
    return;
  }

  // ── the underside bump: level-or-below, and Y is the shorter way out ───────────────────────────
  // Three ways to be told "classify only, don't touch him": he is paused, another interaction holds
  // the lock word this frame, or the caller passed a2 != 0. All three still return the code.
  c->r[2] = kBelowResolveInY;
  if (actor.u16(kActorStateFlags) & kActorPausedBit)          return;
  if (c->mem_r32(kScratchpadBase + kContactLockSlot) != 0)    return;
  if ((uint32_t)suppressSnap != 0)                            return;

  c->mem_w8(kScratchpadBase + kContactKindSlot, kContactKindUndersideBump);
  c->mem_w16(actor.base() + kActorY,
             (uint16_t)(other.u16(kOtherY)
                        + (other.u16(kOtherExtentHi) - other.u16(kOtherExtentLo))
                        + (actor.u16(kActorExtentHi) - actorExtentLo)));

  // Cancel a RISING velocity only — +0x4A is signed and negative means launched upward. A falling
  // actor keeps its velocity, because it is landing on something, not hitting his head on it.
  if (actor.u8(kActorWalkState) != 1) return;
  if (actor.s16(kActorVelY) >= 0)     return;
  c->mem_w16(actor.base() + kActorVelY, 0u);
}

void CollisionResolve::registerOverrides(Game*) {
  engine_set_override_main(0x80023D48u, &CollisionResolve::cylinderResolve, gen_func_80023D48);
  engine_set_override_main(0x8002423Cu, &CollisionResolve::landOnObjectTop, gen_func_8002423C);
  overrides::install(0x8001F40Cu, "CollisionResolve::classifyBodyContact",
                     &CollisionResolve::classifyBodyContact, gen_func_8001F40C, shard_set_override);
}
