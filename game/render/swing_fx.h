// class SwingFx — native display producer for the WEAPON-CHARGE STARBURST (kanban #14).
//
// THE GUEST SIDE. Holding the attack button (CIRCLE) spawns an effect object whose behaviour is
// FUN_8002A584 (10 particle records at node+0x50, each {rotX, rotY, _, life&0x1F}, seeded from the
// guest RNG, ticked -3/frame and respawned) and whose RENDER fn is FUN_8002A834, reached from the
// render walk at 0x8003F9A8. FUN_8002A834 is a pure display function: per particle it builds a
// rotation matrix (FUN_80085480) + a uniform column scale (life<<2, FUN_80084520), composes it with
// the pure scene camera held in scratchpad 0x1F8000F8.., writes the GTE far-colour registers from
// the palette table 0x800A1FC4[node[0x10][2]*4]<<4, sets the depth-cue factor uRam1F800090 = 0xFFF
// (FULL far-colour lerp — unlike the FUN_80027A4C sprite family, whose callers all publish 0), and
// calls the shared packed-mesh emitter FUN_80027768(model = 0x8009FB0C, clutBias = 0,
// sortBias = (s16)node+0x32, uBias = 0) ten times. Measured on the psx_render leg mid-swing
// (`debug otattr`, scratch/logs/c14_otattr2.txt): exactly 60 op-0x3E POLY_GT4 packets per frame,
// fn=0x8002A834, caller=0x8003F9A8, node=0x800EEA60, screen v0 clustered at (93..104, 127..139) —
// the starburst spikes around the swinging mace.
//
// WHY IT WAS INVISIBLE. pc_render does not walk the guest OT (break-first render rebuild
// 2026-07-15) and nothing natively produced this family, so the whole effect layer was dropped on
// the pc leg while psx_render drew it plainly. (Card #14's own note claimed psx_render showed no
// effect either — that is FALSE, and it was FALSE because the repro line said "hold square"; attack
// is CIRCLE. See docs/findings/render.md.)
//
// THE PRODUCER = A SCOPED LEAF TAP on the shared mesh emitter. `meshEmitTap` overrides FUN_80027768:
// it runs the untouched gen body (the guest packet pool / OT / pool cursor stay byte-exact for SBS)
// and then, only on the pc_render leg and only inside the effect scope, re-derives the same faces
// HOST-SIDE and draws them through the native world path (float projection + real per-vertex depth,
// RenderQueue::emitOrQueue at RQ_WORLD/RQ_OM_DEPTH) — the same shape as Render::narrationSwirlRender,
// which is a native producer for this very emitter's SOP-swirl caller.
//
// WHY emitOrQueue AND NOT drawWorldQuad (this cost a whole debug cycle, do not undo it): a prim
// submitted through drawWorldQuad carries has_xyf = 1, and Fps60::isTier1Owned treats EVERY
// RQ_WORLD prim with has_xyf = 1 as owned by the display-pass re-render — it is skipped in mRqCur on
// BOTH presents and expected to come out of tier1Render's sink, which never re-runs this effect. The
// first build submitted 60 correct quads per frame and produced a BYTE-IDENTICAL picture. A
// guest-execution-time producer submits resolved screen-space integers (xsf == nullptr) and replays
// verbatim on both presents, like the #12 torch flame — the fps60 REDIRECT backlog.
//
// THE SCOPE IS MANDATORY, NOT AN OPTIMISATION. FUN_80027768 has ~16 callers across the shards, and
// one of them (0x8002AB5C, the field terrain) is ALREADY natively produced by
// NativeScenePass::terrainRender. An unscoped tap would draw terrain twice. `effectDrawTick` wraps
// FUN_8002A834 and raises the per-Core scope for exactly the ten calls it makes.
//
// THE CONTRACT PUBLISHED AT THE CALL (RE'd from generated/shard_5.c gen_func_80027768, ground truth
// for GTE-bearing code; Ghidra's COP2 decompile is in scratch/decomp/c14_27768.c):
//   r4 model base, 36 bytes per face, drawn until a face whose word1 has bit31 set (that face IS
//   drawn — do/while); r5 clutBias (added <<22 into the uv0 word, i.e. + clutBias to the CLUT row);
//   r6 sortBias (s16, added to the AVSZ4 depth before the OT-bucket quantize); r7 uBias (added to
//   EVERY U byte — the animated texture scroll).
//   face words:  w0 = u0 | v0<<8 | clut<<16      w1 = u1 | v1<<8 | tpage<<16 (bits16-22),
//                                                     bit30 = semi (op 0x3E vs 0x3C), bit31 = last
//                w2 = u2 | v2<<8 | u3<<16 | v3<<24
//                w3..w5 = RGB0..RGB2 (byte3 of each = that vertex's model Z, s8)
//                w6 = RGB3 (byte3 = vertex 3's model Z)
//                bytes 28..35 = the four vertices' model X/Y: x0@28 x1@29 y0@30 y1@31
//                                                             x2@32 x3@33 y2@34 y3@35
//                every model coordinate is a signed byte * 256.
//   the COMPOSED camera×object transform is live in the GTE control regs at call time (CR0-4
//   rotation in 1.3.12, CR5-7 view translation, CR24/25/26 = OFX/OFY/H) — the caller writes it and
//   this leaf never changes it, so the producer reads it as its transform INPUT (it never reads a
//   GTE result or a GP0 packet back);
//   the far colour is CR21/22/23 and the depth-cue factor IR0 is scratchpad 0x1F800090. The guest
//   runs DPCT on w3..w5 and DPCS on w6, i.e. per channel
//       out = clamp255( ((clamp16(((FC<<12) - (c<<16)) >> 12) * IR0) + (c<<16)) >> 12 / 16 )
//   which at IR0 = 0xFFF is very nearly the far colour itself — that is what makes the starburst
//   read as a bright white/lavender flash rather than as the model's own texture colours.
//
// READ-ONLY OVERLAY: everything above is a read; the only guest writes come from the untouched gen
// body. No span stamping, no depth tagging, no packet read-back — identity is structural (we are
// inside our own emitter's scope) and the geometry is re-derived from the model data.
#pragma once
#include <cstdint>
class Core;

class SwingFx {
public:
  Core* core = nullptr;

  // Raised for the duration of the guest effect render fn FUN_8002A834 (see effectDrawTick). Only
  // while it is up does the shared-emitter tap draw anything, so the emitter's other ~16 callers
  // (terrain above all) are untouched. Per-Core — never a file-scope flag — so SBS's two cores
  // cannot see each other's scope.
  bool mInEffectDraw = false;

  // install(): wires the FUN_8002A834 scope wrapper and the FUN_80027768 emitter tap into the ONE
  // override registry (both carry their gen body, so SBS core B keeps running pure substrate).
  static void install();

  // The two override entry points (guest ABI: args in c->r[4..7]).
  static void effectDrawTick(Core* c);   // FUN_8002A834 — the charge-effect render fn (scope only)
  static void meshEmitTap(Core* c);      // FUN_80027768 — the shared packed-mesh quad emitter

  // drawMesh: re-derive one FUN_80027768 call's faces and submit them to the render queue.
  void drawMesh(uint32_t model, uint32_t clutBias, int32_t sortBias, uint8_t uBias);
};
