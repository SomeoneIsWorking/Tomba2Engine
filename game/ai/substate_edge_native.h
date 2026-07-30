// game/ai/substate_edge_native.h — natively-owned LEAVES of beh_substate_edge_orchestrator
// (guest FUN_8012EB54, A00 overlay).
//
// WHAT THIS CLASS ANIMATES, because "substate edge orchestrator" says nothing: 0x8012EB54 is the
// MULTI-PART ASSEMBLY driver, and its most visible instances are the two SEASIDE WATER PUMPS — a long
// diagonal beam with a curved arm, a hanging bucket and a counterweight. Established by observation,
// not inference: `ents` at pad frame 6424 of replays/bugs/seesaw-weight.pad shows nodes 800FB858 and
// 800FB960 at x=5562 and x=6678 with cmds=12, matching kanban #8's independently-derived pump
// positions, and 800FB960 is the node Tomba's attach pointer targets when he hangs on it. Four more
// instances live elsewhere in the area with cmds=3/7 (simpler assemblies of the same class).
// See docs/findings/ai.md and scratch/screenshots/pump_state.png.
//
// `cmds` is the sub-part count and the leaves below are per-frame logic over the child-record table
// at node+0xC0. Knowing that explains shapes that look arbitrary in the gen body — visibilityGate
// samples up to THREE offset points before culling because a beam spanning thousands of world units
// cannot be culled from one.
//
// THIS CHAIN IS KANBAN #8's BLOCKER. Card #8 (water-pump seesaw does not sink under Tomba's weight)
// says do not debug the divergence until the orchestrator's 12 leaves are owned end-to-end. Four are
// (0x8012F494/0x80130AC4/0x80131134/0x801316CC); eight remain: 0x8012E8A8, 0x8012ED84, 0x8012F5B4,
// 0x8012FD88, 0x80130524, 0x801313C4, 0x80146348, 0x8018C820.
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

  // FUN_0x8012E8A8 — per-child transform propagate: walks the sub-part table and composes each part's matrix
  static void perChildTransformPropagate(Core* c);
  // FUN_0x8012ED84 — STATE 0 init: seeds the assembly's parameter block and builds its sub-parts
  static void stateZeroInit(Core* c);
  // FUN_0x8012F5B4 — the node[5]==1 sub-state tick
  static void substate1Tick(Core* c);
  // FUN_0x8012FD88 — the node[5]==2 sub-state tick
  static void substate2Tick(Core* c);
  // FUN_0x80130524 — the node[5]==3 sub-state tick
  static void substate3Tick(Core* c);
  // FUN_0x801313C4 — angle-limit gate; compares child[1]'s angle against node-derived limits and can
  // reset the mode byte and sub-state. Named for mechanism only — the earlier "pendingCommandClear"
  // was wrong (it never touches +0x7A); see the implementation banner.
  static void angleLimitGate(Core* c);
  // FUN_0x80146348 — the assembly post-tick called after the sub-state work
  static void assemblyPostTick(Core* c);

  // FUN_0x8018C820 — the OPN-overlay hook; the twelfth and last leaf of the kanban #8 chain.
  static void opnAssemblyHook(Core* c);

  // FUN_0x801308E0 — the contact-to-weight consumer: turns the contact index at node+0x2B into the
  // weight at node+0x48. The mechanism kanban #8 is about; see the implementation banner.
  static void contactWeightApply(Core* c);

  // FUN_0x80130788 — drive-axis acceleration selector: picks one of four accelerations by the mode
  // byte and returns a 0/1/2 verdict the sub-state ticks use as their escape signal.
  static void driveAccelSelect(Core* c);

  // FUN_0x80131768 — ARM A PAIR OF SUB-PARTS BY ANGLE. Given the node, a group selector and a half-turn flag, it decides
  static void armChildPairByAngle(Core* c);
  // FUN_0x801314B4 — RE-PLACE THE DRIVEN PAIR FROM THE TILT ANGLE. Recomputes the two driven sub-parts' +4 field from the
  static void drivenPairOffsetFromTilt(Core* c);
  // FUN_0x8013892C — SPAWN THE ASSEMBLY'S COMPANION NODE. Creates a child node, seeds it at the assembly's own world
  static void spawnInnerDispatchChild(Core* c);

  // FUN_0x80130D5C — the per-sub-part oscillator the driver loop ticks.
  static void swingStrokeGroupTick(Core* c);

  static void registerOverrides(Game* game);
};
