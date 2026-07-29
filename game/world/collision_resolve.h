// game/world/collision_resolve.h — actor-vs-object CYLINDER COLLISION RESOLVE (guest FUN_80023D48).
//
// The busiest unowned function in the game before this port: 29,869 substrate dispatches per 6000
// frames of replays/bugs/seesaw-weight.pad. Full RE — signature, 4-valued return, field map, frame
// contract — in docs/re/collision-resolve-23d48.md.
#pragma once
#include <cstdint>
struct Core;
class  Game;

class CollisionResolve {
public:
  // FUN_80023D48(actor = a0, other = a1, anchor = a2, flags = a3) -> v0, where v0 is an OUTCOME
  // rather than a boolean:
  //   0 = no collision (separation test failed, nothing written)
  //   1 = horizontal push-out applied (actor X/Z moved onto the contact circle)
  //   2 = vertical resolve AND the landed flag actor+0x29 set
  //   3 = vertical resolve only
  // flags&1 selects the entry geometry: clear uses the actor's raw position, set offsets the sample
  // point by radius(+0x7C) rotated by angle(+0x56) and carries that offset back out of the final
  // write.
  static void cylinderResolve(Core* c);

  // WHY THIS CANNOT CALL THE Trig METHODS DIRECTLY, even though all five of its callees are owned
  // natively (Math::sqrtLzc, Trig::rcos/rsin/ratan2/angleCmp): Trig::registerOverrides is
  // deliberately an EMPTY body because those substrate bodies descend guest stack frames the native
  // methods do not mirror. Calling trigOf(c).rcos(...) here would leave the callees' frame bytes
  // below this function's sp unwritten while substrate core B writes them, and SBS compares that
  // memory. The body therefore invokes them through their generated func_XXXX wrappers, which is
  // what port_gen emits and what keeps the guest stack byte-identical.
  static void registerOverrides(Game* game);
};
