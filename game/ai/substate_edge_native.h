// game/ai/substate_edge_native.h — natively-owned LEAVES of beh_substate_edge_orchestrator
// (guest FUN_8012EB54, A00 overlay).
//
// These are the orchestrator's per-frame tail and gate calls. Between them they accounted for 45,900
// substrate dispatches per 6000 frames of replays/bugs/seesaw-weight.pad — three of the top entries
// on the recdep histogram, all with identical counts because the orchestrator calls each exactly
// once per iteration.
//
// DELIBERATELY NOT IN game/ai/beh_substate_edge_leaves.cpp, which holds hand-transliterated drafts of
// OTHER leaves of the same orchestrator. All four of those drafts descend sp and write none of their
// 22 guest stack spills (see that file's banner and docs/findings/sbs.md). These bodies are port_gen
// output instead — the prologue is emitted verbatim, so the spills cannot go missing by construction.
#pragma once
struct Core;
class  Game;

class SubstateEdgeLeaves {
public:
  // FUN_80130AC4(obj = a0) -> v0 boolean. Multi-point visibility gate: samples the node at 1, 2 or 3
  // offset points (deltas between child-record[i] and child-record[0]) and asks the camera-relative
  // cull wrapper FUN_80077A4C whether each is on screen. Forks on bit0 of the u16 type-flags at
  // obj+0x60 and on the global story-phase byte at 0x800E7EAA.
  static void visibilityGate(Core* c);

  // FUN_801316CC(obj = a0). Driver loop over the child-record pointer table at obj+0xC0: if bit2 of
  // the u16 at obj+0x60 is clear it does nothing, else it calls FUN_80130D5C(obj, slot) for the
  // group-head slots, biasing the slot by -1 when bit1 is clear. Re-reads obj+0x60 on every use.
  static void tickChildOscillators(Core* c);

  // FUN_80131134(obj = a0). Arms a pending child pair from the 2-bit command at obj+0x7A, gated on
  // the node being one of the two masters (u8[obj+3] < 2) and the target child being idle.
  static void armPendingChildPair(Core* c);

  // FUN_8012F494 — the node[5]==0 sub-state tick. Replaces a draft with eight defects.
  static void substate0Tick(Core* c);

  static void registerOverrides(Game* game);
};
