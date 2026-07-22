// mesh_emit_tap.cpp — the SINGLE owner of guest FUN_80027768, the game-wide packed-mesh quad writer.
//
// WHY THIS FILE EXISTS. Two native producers landed independently and both needed the same leaf:
//   * SwingFx  (game/render/swing_fx.cpp, kanban #14) — the weapon-CHARGE starburst, whose meshes are
//     written by this leaf while the charge render fn FUN_8002A834 is on the stack.
//   * FxMesh   (game/render/fx_mesh.cpp,  kanban #15) — the weapon-IMPACT plume, written by the same
//     leaf while the effect-mesh controller FUN_800288AC is on the stack.
// Each tapped 0x80027768 itself. Two `engine_set_override_main` calls on one address do not fail, do
// not warn and do not break SBS — the second silently replaces the first, so ONE of the two effects
// would simply never draw, and which one depends on install order. That is exactly the failure mode
// of kanban #28 (the dialog fill drawing over its own text), and it is invisible until someone
// notices a missing effect. So the address gets one owner, here, and the two producers become
// consumers of it.
//
// THE DISPATCH IS THE PRODUCERS' OWN SCOPES, not a new mechanism. Each producer already raises a
// per-Core scope flag around its controller (SwingFx::mInEffectDraw, FxMesh::mScope) precisely so the
// leaf's ~16 other callers — the field terrain above all — are untouched. Those scopes are disjoint
// by construction: they are raised by different guest functions. This tap simply asks which one is up.
// If neither is, nothing is drawn and the leaf behaves exactly as it did before either card.
#include "core.h"
#include "game.h"
#include "engine.h"
#include "render/swing_fx.h"
#include "render/fx_mesh.h"

extern void gen_func_80027768(Core*);
extern void engine_set_override_main(uint32_t, void (*)(Core*), void (*)(Core*));

namespace {

// Guest ABI: model/list ptr in r4, CLUT bias r5, sort bias r6 (signed halfword), U bias r7.
void meshEmitTap(Core* c) {
  const uint32_t model    = c->r[4];
  const uint32_t clutBias = c->r[5];
  const int32_t  sortBias = (int32_t)(int16_t)c->r[6];
  const uint8_t  uBias    = (uint8_t)c->r[7];

  gen_func_80027768(c);   // the untouched guest body: packet pool / OT / pool cursor stay byte-exact

  if (c->game->oracle || c->rsub.mode.psxRender()) return;   // read-only overlay gate
  if (!model) return;

  if (eng(c).swingFx.mInEffectDraw) { eng(c).swingFx.drawMesh(model, clutBias, sortBias, uBias); return; }
  if (eng(c).fxMesh.mScope)         { FxMesh::draw(c, model, clutBias, sortBias, uBias); return; }
  // No producer's scope is up — another caller (terrain, …) owns its own path or has none yet.
}

}  // namespace

void mesh_emit_tap_install() {
  engine_set_override_main(0x80027768u, meshEmitTap, gen_func_80027768);
}
