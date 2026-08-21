#include "scene_kind.h"

#include <cstdio>

namespace {

int expect(GameStageSceneKind actual, GameStageSceneKind expected, const char *name) {
  if (actual == expected) {
    return 0;
  }
  std::fprintf(stderr, "scene_kind: %s failed (actual=%d expected=%d)\n", name, (int)actual, (int)expected);
  return 1;
}

} // namespace

int main() {
  int failed = 0;
  failed += expect(classifyGameStageScene(1, 2, 0x801138A4u), GameStageSceneKind::Field, "ordinary field");
  failed += expect(
      classifyGameStageScene(1, 3, 0x801138A4u), GameStageSceneKind::HutInterior, "field sub-mode state 3 is the hut");
  failed += expect(classifyGameStageScene(2, 3, 0x801138A4u),
                   GameStageSceneKind::SaveContinueMenu,
                   "area-menu state 3 is not the hut");
  failed += expect(classifyGameStageScene(2, 8, 0x801138A4u),
                   GameStageSceneKind::SaveContinueMenu,
                   "last Save/Continue machine state stays on its black-backed scene");
  failed +=
      expect(classifyGameStageScene(2, 2, 0x801138A4u), GameStageSceneKind::Field, "area-menu fade keeps the field");
  failed += expect(classifyGameStageScene(2, 9, 0x801138A4u),
                   GameStageSceneKind::Field,
                   "state outside the Save/Continue machine is not classified as menu");
  failed +=
      expect(classifyGameStageScene(0, 0, 0x3C021F80u), GameStageSceneKind::SopNarration, "SOP narration signature");
  return failed == 0 ? 0 : 1;
}
