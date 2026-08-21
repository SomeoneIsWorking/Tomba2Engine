#include "scene_kind.h"

GameStageSceneKind classifyGameStageScene(std::uint16_t subMode, std::uint16_t state, std::uint32_t overlaySignature) {
  constexpr std::uint32_t kSopNarrationSignature = 0x3C021F80u;

  if (subMode == 0 && overlaySignature == kSopNarrationSignature) {
    return GameStageSceneKind::SopNarration;
  }
  if (subMode == 1 && state == 3) {
    return GameStageSceneKind::HutInterior;
  }
  // GAME.BIN's sub-mode 2 is FUN_80106478. Its states 3..8 own the black-backed
  // Save/Continue/Load/Quit dialogs; state 3 is not the identically-numbered hut state.
  if (subMode == 2 && state >= 3 && state <= 8) {
    return GameStageSceneKind::SaveContinueMenu;
  }
  return GameStageSceneKind::Field;
}
