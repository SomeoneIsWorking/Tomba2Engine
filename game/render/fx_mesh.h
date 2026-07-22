// class FxMesh — native display producer for the EFFECT MESH family (kanban #15: the weapon
// IMPACT burst was missing its radial plume under pc_render).
//
// THE TWO HALVES OF ONE EFFECT. The swing/impact effect object (node 0x800EE9D8, behaviour
// FUN_800293F4) alternates its per-node renderer (node+0x18) between two emitters:
//   * FUN_80027E5C -> FUN_80027A4C — the 8-byte-record SCALED SPRITE writer. Already owned:
//     game/render/fx_sprite.cpp taps it, so its three 0x2E prims DO reach the picture.
//   * FUN_800288AC -> FUN_80027768 — the MESH writer: a 9-word-per-record template list of
//     gouraud-textured quads (0x3C, or 0x3E when the record asks for semi-transparency),
//     projected through a transform FUN_800288AC composes from the effect's own world pos /
//     rotation / scale. NOTHING owned this, and pc_render does not walk the guest OT
//     (break-first render rebuild 2026-07-15) — so this half of every such effect was absent.
//
// MEASURED (2026-07-22, replays/bugs/weapon-impact-bucket.pad pad-frame 657, the frame Tomba's
// swing connects with the seaside bucket). `PSXPORT_PRIMAT="150,118,657"` at the burst pixel:
//   psx_render leg — FIVE effect prims cover it, all semi=1 tp=(320,256):
//     0x800CB968 op=2E   0x800CB990 op=2E   0x800CB9B8 op=2E
//     0x800CB9E0 op=3E   0x800CBA14 op=3E     <-- the two MESH quads
//   pc_render leg — exactly the three 0x2E ones and NEITHER 0x3E.
// `debug otattr` attributes all five to fn=0x80033080 caller=0x8003F9A8 node=0x800EE9D8, and
// FUN_80033080 is literally `{ FUN_80027E5C(); FUN_800288AC(node); }` — the sprite/mesh pair.
//
// THE PRODUCER = A SCOPED LEAF TAP (the PauseMenu pattern, game/ui/pause_menu.cpp).
// FUN_80027768 is a GAME-WIDE mesh writer with 17 static callers, several already owned
// (0x8002AB5C is the terrain path). Tapping it unscoped would double-draw everything that
// already has a producer. So:
//   * `armTap` overrides FUN_800288AC: raise the capture scope, run the untouched gen body
//     (which direct-calls func_80027768, so the inner tap fires inside the scope), lower it.
//   * `writeTap` overrides FUN_80027768: run the untouched gen body first (the guest packet
//     pool / OT / pool cursor stay byte-exact), then — only inside the scope, and only on the
//     pc_render leg — RE-DERIVE the same quads host-side and hand them to the render queue.
// Read-only overlay: not one guest write outside the gen body.
//
// RE-DERIVED, NOT TRANSCRIBED. The quads come from the effect's own state, in float:
//   * geometry — the 4 model-space corners packed in each template record, through the
//     transform FUN_800288AC left in the GTE control registers (CR0-4 rotation, CR5-7
//     translation, CR24/25 screen centre, CR26 projection distance), projected by
//     EObjXform::project. No gte_op is issued for the picture.
//   * depth — real per-vertex view depth (proj_pz_to_ord), not an OT bucket.
//   * colour — the record's own per-corner RGB, scaled by the effect's fade factor (see
//     kDepthCueScratch in the .cpp): FUN_800288AC zeroes the GTE far colour, so the guest's
//     DPCT/DPCS depth-cue collapses to exactly that scale.
// Two things are taken verbatim from the guest, both of them the effect's own authored data:
//   * its DEPTH CULL — a quad whose bucket index falls outside the game's own [4, 0x7FF] range is
//     one the game does not draw, so the producer must not draw it either.
//   * its DEPTH OFFSET — the effect carries a sort bias at node+0x32 (-80 for the impact burst)
//     that the game adds before deciding where the quads sit. It is what makes a burst spawned
//     just behind the object it hit still paint in front of it. Applied in view units through the
//     AVSZ4 scale; without it the burst measures 0.0892 ord against the bucket's 0.0968 and the
//     depth buffer hides the whole plume.
//
// Diagnostic channel `fxmesh` (PSXPORT_DEBUG=fxmesh): one line per tapped call (template list,
// clut row, sort bias, U scroll, fade factor) and one per submitted quad (record address, semi
// bit, texpage/clut, screen corners, shade, first colour/UV, depth). Silent = this producer drew
// nothing, which is the first thing to check when an effect's mesh half is missing.
#pragma once
#include <stdint.h>
struct Core;

class FxMesh {
public:
  // Install both taps into the one override registry. Called once per Game.
  static void install();

  // Capture-scope depth: non-zero only while the effect-mesh controller FUN_800288AC is running,
  // so the shared writer's other 16 callers never reach this producer.
  int mScope = 0;

  // Re-derive one FUN_80027768 call's quads and submit them. Called by the leaf's SINGLE owner,
  // game/render/mesh_emit_tap.cpp, when mScope is up — not installed as an override here, because
  // SwingFx (#14) needs the same leaf and two installs on one address silently drop one of them.
  static void draw(Core* c, uint32_t list, uint32_t clutRow, int32_t sortBias, uint8_t uScroll);
};
