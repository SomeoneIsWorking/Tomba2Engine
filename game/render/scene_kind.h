// GAME-stage picture classification from the task's two nested state selectors.
#pragma once

#include <cstdint>

enum class GameStageSceneKind {
  Field,
  HutInterior,
  SaveContinueMenu,
  SopNarration,
};

GameStageSceneKind classifyGameStageScene(std::uint16_t subMode, std::uint16_t state, std::uint32_t overlaySignature);
