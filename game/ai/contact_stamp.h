// game/ai/contact_stamp.h — the CONTACT STAMP producer (guest FUN_80111304).
//
// This is the function kanban #8 identified as the producer that should write a nonzero contact index
// into an object's +0x2B, feeding the weight consumer SubstateEdgeLeaves::contactWeightApply. Owning
// it puts the whole produce->consume path in native code.
//
// SIGNATURE, established from the writes rather than assumed: a0 = G (a globals/context block), a1 =
// the candidate item. The writes land on item+0x2B and G+0x2E / G+0x36. Card #8 described the
// producer as FUN_80111304(player, obj); the first argument is the context block, not the player.
//
// WHAT IT DOES: calls an overlap test (0x8002300C) with the item's radius at +0x80 scaled by 4, and
// ONLY on a nonzero result writes item+0x2B = 1. It then reads the scratchpad byte at 0x1F800137 and
// reaches the "=2" path — the value the weight consumer actually keys on — only when that byte is
// zero. v0 differs per exit path but no caller tests it; the sole caller loads a constant into r3 and
// falls through, so it is effectively void.
//
// WHY IT NEVER FIRES, MEASURED (claim C023, 2026-07-30): its caller reads a candidate-list count from
// scratchpad 0x1F800144 and branches past the whole loop when that is zero — and that byte is 0x00 at
// all 14 samples taken every 500 frames across replays/bugs/seesaw-weight.pad, including the grab at
// ~6424. So the producer is skipped for the entire replay. Owning it changes nothing about that; it
// makes the starvation readable in native code instead of across a substrate boundary.
//
// FIELD MEANINGS BEYOND THE ABOVE ARE NOT CLAIMED. The RE write-up for this address was refuted on
// evidence — three cited meanings and one causal claim did not survive — while its mechanics passed
// every check. Only the mechanics are repeated here.
#pragma once
struct Core;
class Game;

class ContactStamp {
public:
  // FUN_80111304(G = a0, item = a1). Writes item+0x2B on overlap; G+0x2E / G+0x36 on the snap path.
  static void stampAndSnap(Core *c);

  static void registerOverrides(Game *game);
};
