// game/ai/node_lifecycle_sm.h — the per-node LIFECYCLE state machine (guest FUN_80040558).
//
// 17,878 substrate dispatches per 6000 replay frames — the busiest ORPHAN-drafted address in the
// game. Dispatches on node[4] over four states: 0 initialises the node and builds its children,
// 1 and 2 are the two per-frame active tails, 3 calls Spawn::despawn.
//
// REPLACES `sm40558` FROM game/world/entity.cpp, WHICH HAD FIVE DEFECTS — including one that was not
// a stack or register artefact but a MISSING GUEST WRITE: the guest clears node[95] on every one of
// its nine state-1 exit paths and the draft never wrote it at all. The draft looked self-consistent
// because it DOES clear node[95] on the state-0 init path.
//
// Two of the other four are worth naming, because they are what hand-transliteration cannot get
// right by inspection: 31 of the 32 `ra` constants were omitted (every callee that mirrors its own
// frame would spill a stale return address into guest RAM), and the guest's FOUR runtime jump tables
// — read from guest DATA at 0x800152E0/0x80015300/0x80015318/0x80015338 — were baked in as assumed
// C switches, a mapping that cannot be verified from the gen body at all.
#pragma once
struct Core;
class  Game;

class NodeLifecycleSm {
public:
  static void step(Core* c);
  static void registerOverrides(Game* game);
};
