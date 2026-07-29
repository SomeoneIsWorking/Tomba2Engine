// game/ai/cull_substate_native.h — natively-owned leaves of beh_cull_substate_orchestrator (A00).
//
// Sibling of game/ai/substate_edge_native.h, and separate from game/ai/beh_cull_substate_leaves.cpp
// for the same reason: that file holds hand-transliterated drafts, and hand transliteration is where
// this project keeps introducing defects (claim C021). These bodies are port_gen output.
#pragma once
struct Core;
class  Game;

class CullSubstateLeaves {
public:
  // FUN_80133550 — per-frame tick of a one-shot decaying swing applied to a child record's Euler Z.
  // 7,650 substrate dispatches per 6000 replay frames.
  //
  // NAMED FOR THE MECHANISM. The RE proposed "tickChildTiltSwing"; the verifier rejected "Tilt" as
  // unjustified by the body and asked for the axis the code actually touches, which node_xform.cpp
  // already calls childEulerZ. This name follows that existing vocabulary rather than inventing a
  // second word for the same field.
  static void tickChildEulerZSwing(Core* c);

  // FUN_80132A88 — phase-advancing sibling of tickChildEulerZSwing. Replaces a 5-defect draft.
  static void tickChildEulerZSwingPhase(Core* c);
  // FUN_80132954 — sub-state-zero tick. Replaces a 6-defect draft.
  static void tickSubstateZero(Core* c);

  static void registerOverrides(Game* game);
};
