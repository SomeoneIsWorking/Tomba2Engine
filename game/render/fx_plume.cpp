// game/render/fx_plume.cpp — NATIVE PRODUCER for the FOUR-COPY RADIAL PLUME (guest FUN_8002BC9C).
//
// WHAT IT DRAWS. A small packed mesh — the flare/plume an effect node carries — stamped FOUR TIMES
// about the node's own vertical axis, a quarter turn apart, so the four copies read as a radial burst.
// It is the most RESIDENT of the effect-mesh controllers (five nodes carried it as their +0x18 render
// fn in a single field dump, kanban #15's census) and it is the first row of
// docs/unported-render-inventory.md R1: the family the 2026-08-04 tap retirement (abf3cf9) left with
// NO producer at all, because the one file that owned the shared writer FUN_80027768 went with it.
//
// ────────────────────────────────────────────────────────────────────────────────────────────────
// RE — ground truth generated/shard_0.c `gen_func_8002BC9C`, read instruction by instruction, plus
// generated/shard_5.c `gen_func_80027768` for the writer it feeds (whose record format is already
// owned by Render::meshQuadRecordsEmit). The controller does exactly this:
//
//   1. Publishes depth-cue IR0 = 0 (scratchpad 0x1F800090) and far colour CR21-23 = 0, i.e. the
//      writer's DPCT/DPCS cue is programmed to the IDENTITY — the mesh's authored colours pass
//      through untouched. (Contrast the dust puff, which publishes IR0 = 0xFFF to have its colours
//      REPLACED by the surface material's far colour.)
//   2. Reads the animation script byte at *(node+0x3C). Its low seven bits index the node's own mesh
//      pointer table at *(node+0x50); bit 7 marks the script's last frame. Script pointer null =
//      nothing to draw, and the guest returns without emitting.
//   3. Four times: build the node's rotation from its three Euler angles at node+0x48, column-scale it
//      by the authored triple at 0x800A1CD4, compose with the scene camera, translate by the node's
//      own world position at node+0x2C, and hand the mesh to the writer with sort bias (s16)node+0x32
//      and no U scroll. Between copies it advances the node's Y angle by a QUARTER TURN.
//
// WHY THIS IS A PORT AND NOT A TAP. The deleted producer took its transform from `gte_read_ctrl(0..7)`
// — whatever the substrate controller had just composed into the GTE control registers — which is the
// banned shape (coord/PROTOCOL.md: resolve from what SUBMITS to the GTE, never from what it produced).
// Every input above is instead the game's OWN pre-quantisation state: three node angle fields, a node
// world position, a table index, and one authored scale triple. The camera arrives through
// projComposeObjectHost, i.e. the fps60-lerped native scene camera, so the plume interpolates with the
// rest of the frame instead of stepping at the logic rate. No gen_func body runs to make this picture
// and no guest memory is written — the guest's own FUN_8002BC9C still executes underneath and still
// owns every store it makes (script cursor at node+0x40, Y-angle advance at node+0x4A, packet pool).
//
// THE Y-ANGLE READ IS SOUND EVEN THOUGH THE GUEST MUTATED IT. The controller leaves node+0x4A at
// base + 4 quarter turns = base + a full turn, and the engine's sine table is a full 4096-entry turn,
// so `node+0x4A + i*quarter` at display time names the same four orientations the guest used, merely
// relabelled by one revolution. (It also wraps harmlessly in s16: 65536 is a whole number of turns.)
//
// NOT COVERED, stated rather than hidden: FUN_8002BC9C has a SECOND, gated half after the four-copy
// loop — when node+3 is 0x14 or 0x15 it projects the anchor at node+0x58 and hands the list at
// node+0x34 to the SPRITE writer FUN_80027A4C (or, when that projection rejects, calls FUN_80031780).
// That half is a different emitter family and is NOT ported here. The `plumefx` diagnostic reports the
// subtype byte on every call, so a scene that reaches the sprite half is visible rather than silent.
#include "core.h"
#include "fps60.h"
#include "game.h"
#include "mesh_quads.h"
#include "producer_scope.h" // ProducerScope — producer identity for the graphics-producer DB
#include "projection.h"
#include "render.h"
#include "render_internal.h" // ObjScope — prim identity is the emitting node
#include <cstdint>
#include <lucent/log.h>

namespace {

// ── the node's own fields (byte offsets), named rather than open-coded at the use site ─────────────
// This controller's own guest address — the type-0x20 node render fn (node+0x18) the whitelist
// dispatches to, and therefore the row the guest leg attributes its prims to (see ProducerScope below).
constexpr uint32_t kGuestControllerAddr = 0x8002BC9Cu;

constexpr uint32_t kNodeSubtype = 0x03u;    // u8  — 0x14/0x15 additionally enable the sprite half
constexpr uint32_t kNodePosX = 0x2Cu;       // s16 world position; Y and Z follow at +0x2E and +0x30
constexpr uint32_t kNodeSortBias = 0x32u;   // s16 the controller hands the writer as its sort bias
constexpr uint32_t kNodeAnimScript = 0x3Cu; // u32 -> the animation script's current byte
constexpr uint32_t kNodeAngles = 0x48u;     // s16 x3 Euler angles (X at +0x48, Y at +0x4A, Z at +0x4C)
constexpr uint32_t kNodeMeshTable = 0x50u;  // u32 -> array of mesh pointers, indexed by the script byte

// ── authored data the controller reads ────────────────────────────────────────────────────────────
constexpr uint32_t kColumnScale = 0x800A1CD4u; // three bytes, each << 2 = a per-column 1.3.12 factor
constexpr uint8_t kScriptLast = 0x80u;         // script byte bit 7: this is the script's final frame
constexpr uint8_t kScriptIndex = 0x7Fu;        // ...and its low bits are the mesh-table index
constexpr int kCopies = 4;
constexpr int kQuarterTurn = 0x400; // 4096 angle units to the turn
constexpr int kUScroll = 0;         // this controller animates by MESH, not by texture scroll

// The cue the controller programs before every call: identity, so the mesh keeps its own colours.
constexpr int32_t kCueOff = 0;
const int32_t kNoFarColour[3] = {0, 0, 0};

// The gate on the unported sprite half, kept as a named range so the diagnostic can say "this call
// reached a branch this producer does not draw" instead of leaving it invisible.
constexpr uint8_t kSpriteHalfSubtypeLo = 0x14u, kSpriteHalfSubtypeHi = 0x15u;

// The controller column-scales its rotation with nothing else composed in; composeScaled multiplies two
// matrices before scaling, so the second one is the identity in the engine's 1.3.12 units.
const int32_t kIdentity[3][3] = {{4096, 0, 0}, {0, 4096, 0}, {0, 0, 4096}};

} // namespace

// The four-copy radial plume of guest FUN_8002BC9C, rebuilt from the node's own state. Read-only;
// dispatched from Render::fieldObjectsRender's type-0x20 walk, so it runs in the DISPLAY pass and is
// re-run under the lerped camera on an fps60 in-between frame.
void Render::radialPlumeRender(uint32_t node) {
  Core *c = mCore;

  const uint32_t script = c->mem_r32(node + kNodeAnimScript);
  const uint32_t table = c->mem_r32(node + kNodeMeshTable);
  const uint8_t subtype = c->mem_r8(node + kNodeSubtype);
  if (!script || !table) { // the guest's own `if (script == 0)` skip of the loop
    lucent::debug("plumefx",
                  "f{} node={:08X} sub={:02X} script={:08X} table={:08X} — no mesh loop",
                  c->game->gpu.s_frame,
                  node,
                  subtype,
                  script,
                  table);
    return;
  }

  const uint8_t frame = c->mem_r8(script);
  const uint32_t mesh = c->mem_r32(table + (uint32_t)(frame & kScriptIndex) * 4u);
  const MeshOtBias ot{/*known=*/true, /*bias=*/(int32_t)c->mem_r16s(node + kNodeSortBias)};

  const int32_t colScale[3] = {(int32_t)c->mem_r8(kColumnScale + 0u) << 2,
                               (int32_t)c->mem_r8(kColumnScale + 1u) << 2,
                               (int32_t)c->mem_r8(kColumnScale + 2u) << 2};
  const int16_t angleX = c->mem_r16s(node + kNodeAngles + 0u);
  const int16_t angleY = c->mem_r16s(node + kNodeAngles + 2u);
  const int16_t angleZ = c->mem_r16s(node + kNodeAngles + 4u);
  const float pos[3] = {(float)c->mem_r16s(node + kNodePosX + 0u),
                        (float)c->mem_r16s(node + kNodePosX + 2u),
                        (float)c->mem_r16s(node + kNodePosX + 4u)};

  // The graphics-producer DB's NATIVE leg (external/psxport/docs/plans/graphics-producer-db.md).
  // KEYED BY THE CONTROLLER'S GUEST ADDRESS, NOT THE WRITER'S, and that is not a style choice: on the
  // GUEST leg OtAttr's shadow stack pushes only on INDIRECT dispatch (interp_diag.h — "direct
  // recompiler-emitted func_XXXX(c) calls do NOT push"). This controller is a type-0x20 node render fn
  // reached by function pointer, so it pushes; the shared writer FUN_80027768 it calls is a plain `jal`
  // and does not. So the guest leg attributes these prims to 0x8002BC9C, and the native leg must key
  // the same row or the two legs never line up — which is the entire point of the comparison.
  // COROLLARY, and it is why meshQuadRecordsEmit opens NO scope of its own: a shared writer must
  // inherit its caller's scope. If it opened one it would shadow every controller and collapse the
  // whole mesh family into a single meaningless row.
  ProducerScope producerScope(&c->rsub.producerScope, kGuestControllerAddr, "radialPlumeRender");
  ObjScope objScope(c, node);
  int drawn = 0;
  float bbox[4] = {1e9f, 1e9f, -1e9f, -1e9f}; // seeded inverted; the walk unions what it emits
  for (int i = 0; i < kCopies; i++) {
    int32_t rot[3][3];
    MeshQuads::rotmat(c, angleX, (int16_t)(angleY + i * kQuarterTurn), angleZ, rot);
    float Robj[3][3];
    MeshQuads::composeScaled(rot, kIdentity, colScale, Robj);
    EObjXform w;
    projComposeObjectHost(Robj, pos, &w);
    projSetActive(&w);
    drawn += meshQuadRecordsEmit(mesh, MeshQuadStyle{kUScroll, kNoFarColour, kCueOff}, ot, bbox);
    projClearActive();
  }

  // The denominator matters: `quads=0` with a live mesh means every record failed the writer's own
  // ordering reject (off-screen / out of the linkable depth range), which is a different finding from
  // `mesh=00000000` (the script frame selects no mesh) and from the producer never being called at all
  // — and `sprite-half` names the branch of this controller that is deliberately not ported. `screen=`
  // is the box this call actually emitted into, so an A/B pixel diff is checkable against the
  // producer's own prediction rather than merely being non-zero somewhere.
  lucent::debug("plumefx",
                "f{} t={:.2f} node={:08X} sub={:02X} frame={:02X}{} mesh={:08X} bias={} "
                "ang=({},{},{}) pos=({:.0f},{:.0f},{:.0f}) copies={} quads={} "
                "screen=[{:.1f},{:.1f}]..[{:.1f},{:.1f}]{}",
                c->game->gpu.s_frame,
                (double)fps60(*c->game).mT,
                node,
                subtype,
                (unsigned)(frame & kScriptIndex),
                (frame & kScriptLast) ? " last" : "",
                mesh,
                ot.bias,
                angleX,
                angleY,
                angleZ,
                (double)pos[0],
                (double)pos[1],
                (double)pos[2],
                kCopies,
                drawn,
                (double)(drawn ? bbox[0] : 0.0f),
                (double)(drawn ? bbox[1] : 0.0f),
                (double)(drawn ? bbox[2] : 0.0f),
                (double)(drawn ? bbox[3] : 0.0f),
                (subtype >= kSpriteHalfSubtypeLo && subtype <= kSpriteHalfSubtypeHi)
                    ? " [sprite-half reached — NOT PORTED]"
                    : "");
}
