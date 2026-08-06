// game/render/guest_face_gate.h — THE TESTS THE REAL GAME RUNS ON EVERY FACE BEFORE IT IS DRAWN.
//
// Tomba! 2's own geometry submitters (`gen_func_8007FDB0` = POLY_GT3, `gen_func_8008007C` = POLY_GT4)
// do not draw a face just because it projected. Each face passes four gates, in this order, and a face
// that fails ANY of them is never linked into the ordering table — i.e. the real game DOES NOT DRAW IT:
//
//   1. GTE ERROR      `lw t4, CR31 ; bltz t4, next`   — read straight after the RTPT (and, for GT4,
//                     again after the extra RTPS of v3).  CR31 bit31 is the GTE's error summary, the
//                     OR of bits 30..23 and 18..13.  THIS IS THE ENGINE'S NEAR-PLANE / OFF-SCALE CULL:
//                     a vertex closer than H/2 overflows the perspective divide (bit 17), a vertex
//                     projecting past ±1024 saturates SX2/SY2 (bits 14/13), a view depth outside
//                     0..65535 saturates SZ3 (bit 18), an over-range view coordinate saturates IR1..3
//                     (bits 24..22).  Any of those and the face is dropped WHOLE.
//   2. BACKFACE       NCLIP, `blez MAC0, next`.                          (already ported — submit.cpp)
//   3. SCREEN BOUNDS  at least one vertex with SX in [0,320) and one with SY in [0,240).
//                                                                        (partly ported — submit.cpp)
//   4. OT KEY RANGE   the compressed depth key must satisfy `(unsigned)(key-4) < 2044`; otherwise the
//                     submitter stores -1 and `gen_func_80080000` skips the OT link entirely.
//
// Gate 1 and gate 4 were NOT reproduced by the native producers: `submitPolyGt3Native`/`Gt4Native`
// computed the key, got -1, and drew the face anyway with ord 0 (i.e. AT THE VERY FRONT).
//
// WHAT THIS IS AND IS NOT EVIDENCE FOR. Landing these two gates closes a NAMED, RE'd hole in the port
// (kanban #77's own "named, honestly unported" entry) and it demonstrably fires — 7302 faces over one
// 3880-frame route, moving 10909/76800 px at f2200 where a cutscene camera sits inside a character's
// wing. It is NOT, on the evidence taken so far, the cause of kanban #77's reported blocker: at the
// USER'S OWN CAMERA (area 7, readback 0x1F8000D2 = 13031/-2860/7257 against the reported
// 13014/-2860/7237) the census reads `faces=2113 droppedGTE=0 droppedOTKEY=0`, and the picture is
// 0/76800 px different with the gates on. Do not let this header become the answer to that card.
//
// NOTHING HERE IS A CHOSEN THRESHOLD. Every constant is the guest's own: H/2 and ±1024 are the GTE's
// saturation points, [4,2048) is the submitter's literal range test. The native projection already
// applies each of these clamps (projection.cpp) — it just never reported that it had, so the caller
// could not run the guest's test. `GteFlag` is that missing report.
#pragma once
#include <stdint.h>
#include "proj_vtx.h"

// ── GTE FLAG (CR31) ────────────────────────────────────────────────────────────────────────────────
// The bits an RTPS/RTPT can raise for ONE vertex, named. Layout per the GTE register documentation;
// the game only ever tests the summary bit, but the individual bits are what makes a drop explicable
// ("dropped for divide overflow" is a fact about the scene, "dropped" is not).
namespace GteFlag {
constexpr uint32_t MAC3_OVF   = 1u << 25 | 1u << 28;   // MAC3 result outside 43 bits (neg | pos)
constexpr uint32_t MAC2_OVF   = 1u << 26 | 1u << 29;
constexpr uint32_t MAC1_OVF   = 1u << 27 | 1u << 30;
constexpr uint32_t IR3_SAT    = 1u << 22;              // IR3 clamped to -8000h..+7FFFh
constexpr uint32_t IR2_SAT    = 1u << 23;
constexpr uint32_t IR1_SAT    = 1u << 24;
constexpr uint32_t SZ3_SAT    = 1u << 18;              // SZ3/OTZ clamped to 0..FFFFh
constexpr uint32_t DIV_OVF    = 1u << 17;              // H/SZ3 saturated to 1FFFFh — THE NEAR PLANE
constexpr uint32_t MAC0_OVF   = 1u << 15 | 1u << 16;   // MAC0 result outside 31 bits (neg | pos)
constexpr uint32_t SX2_SAT    = 1u << 14;              // screen X clamped to -400h..+3FFh
constexpr uint32_t SY2_SAT    = 1u << 13;              // screen Y clamped to -400h..+3FFh

// CR31 bit31 — "Error Flag: bits 30..23 and 18..13 ORed together". The guest tests `(int32_t)CR31 < 0`,
// which is exactly this OR. Note bit 12 (IR0 saturation) is deliberately OUTSIDE the mask, as on hardware.
constexpr uint32_t ERROR_MASK = 0x7F800000u | 0x0007E000u;
inline bool isError(uint32_t f) { return (f & ERROR_MASK) != 0u; }
}  // namespace GteFlag

// ── Depth key ──────────────────────────────────────────────────────────────────────────────────────
// The submitter's key computation has two outcomes the caller must not confuse, and conflating them is
// what let the OT-range drop go unimplemented: "the guest computed a key and refused to link it" is a
// CULL, while "this port could not work out which bucket the guest would have used" is an UNKNOWN and
// must never cull anything. `SortKey` keeps them apart by construction.
struct SortKey {
  int  key;         // OT bucket the guest files this face under; meaningful only when `linked`
  bool linked;      // the guest links it: the key passed the [4,2048) range test
  bool guestDrop;   // the guest DROPPED it: the key failed that range test. A real cull.
  // !linked && !guestDrop  =>  unknown to this port (no ZSF captured yet, foreign OT base). Draw it:
  // an unknown must not become a cull, or a bookkeeping gap would masquerade as the game's own rule.
  static SortKey linkedAt(int k)  { return SortKey{ k, true,  false }; }
  static SortKey droppedByGuest() { return SortKey{ -1, false, true  }; }
  static SortKey unknown()        { return SortKey{ -1, false, false }; }
};

// ── The census ─────────────────────────────────────────────────────────────────────────────────────
// "How many of the faces THIS PORT DRAWS would the real game have refused to draw?" — asked per frame,
// per submitter, with the denominator attached. It is deliberately a counter and not a `lucent::debug`
// per face: at 1e4 faces/frame a per-face line is unreadable, and the number that settles the question
// is a ratio.
//
// A ZERO IN `dropGte`/`dropOtKey` IS ONLY MEANINGFUL NEXT TO `faces`. The verdict line therefore always
// prints the denominator, and the `unknownKey` column — faces whose OT key this port could not compute
// (no ZSF captured, foreign OT base). Those are NOT culls and are NOT counted as drops; printing them
// separately is what keeps "we could not tell" from reading as "the guest kept it".
struct GuestFaceGateCensus {
  long faces = 0;         // DENOMINATOR: every face the submitters projected this frame
  long dropGte = 0;       // ... dropped at gate 1, the CR31 error bit
  long dropOtKey = 0;     // ... dropped at gate 4, the OT-key range test (counted among gate-1 survivors)
  long unknownKey = 0;    // ... NOT dropped: faces whose key this port could not compute. Never a cull.
  uint32_t flagsSeen = 0; // OR of every CR31 bit raised, so a drop can be named by its cause
  void reset() { faces = dropGte = dropOtKey = unknownKey = 0; flagsSeen = 0; }
  // Gate 1, called on EVERY projected face (before backface/screen), which is where the guest reads CR31.
  void noteGte(bool dropped, uint32_t faceFlags) {
    faces++; flagsSeen |= faceFlags; if (dropped) dropGte++;
  }
  // Gate 4, called on the faces that reached the key computation.
  void noteKey(const SortKey& k) {
    if (k.guestDrop) dropOtKey++;
    else if (!k.linked) unknownKey++;
  }
};
