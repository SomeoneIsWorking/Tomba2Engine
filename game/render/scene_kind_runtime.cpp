#include "scene_kind.h"

#include "core.h"
#include "game.h"
#include "render.h"

Render::SceneKind Render::classifyScene() {
  Core *c = mCore;
  // A re-registered task has not run its new entry yet, so its old selectors cannot classify a scene.
  if (c->mem_r16(0x801FE000u) == 3) {
    return SceneKind::Loading;
  }

  const std::uint32_t stage = c->mem_r32(0x801FE00Cu);
  if (stage == 0x8010649Cu) {
    return SceneKind::StartBoot;
  }
  if (stage == 0x801062E4u) {
    return SceneKind::Title;
  }
  if (stage != 0x8010637Cu) {
    return SceneKind::Unknown;
  }

  const std::uint32_t taskSm = c->mem_r32(0x1F800138u);
  switch (classifyGameStageScene(
      taskSm ? c->mem_r16(taskSm + 0x4Au) : 0, taskSm ? c->mem_r16(taskSm + 0x4Cu) : 0, c->mem_r32(0x80109450u))) {
  case GameStageSceneKind::Field:
    return SceneKind::Field;
  case GameStageSceneKind::HutInterior:
    return SceneKind::HutInterior;
  case GameStageSceneKind::SaveContinueMenu:
    return SceneKind::SaveContinueMenu;
  case GameStageSceneKind::SopNarration:
    return SceneKind::SopNarration;
  }
  return SceneKind::Unknown;
}

void Render::renderSaveContinueMenu() {
  mCore->game->fps60.mTier1EligibleCur = false;
  // The native target is cleared to black before its queued UI is emitted. Do not clear guest VRAM
  // here: FUN_8007ED5C's "Save?" heading still samples its glyphs from the displayed VRAM page.
}
