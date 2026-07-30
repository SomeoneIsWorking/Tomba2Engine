// game/render/fx_sprite_anchored.cpp — FUN_80027CB4, the SINGLE-ANCHOR, UNIFORM-SCALE member of
// the FUN_80027A4C world-anchored scaled-sprite family.
//
// WHAT IT IS, IN GAME TERMS
// Tomba!2 builds its small burning/glittering effects out of a "sprite cluster": a short list of
// 8-byte quad records that one shared writer (FUN_80027A4C) turns into GPU primitives around a
// single screen anchor. THIS emitter is the family's plain case, and the one that actually draws
// the game's steady fires — the six bright hut-roof flames at the seaside water-pump vista are the
// confirmed live instances. It takes the node's OWN world position, projects it once, stamps the
// whole cluster there, and takes the size straight out of the perspective divide: a flame that is
// twice as far away is drawn half as big, and nothing else touches its size. That "nothing else" is
// its identity — the other two members exist precisely to modulate what this one leaves alone.
//
// WHAT SEPARATES IT FROM ITS TWO SIBLINGS (render.h groups all three). All three verified from the
// same Ghidra dump, scratch/decomp/fx_emit_leaves.c, which decompiles 0x80027CB4 / 0x80027E5C /
// 0x800281EC together:
//   1. SCALE — the distinguishing axis, and the reason this one is the baseline. Here the published
//      pixel scale IS the GTE's MAC0, unmultiplied. 0x80027E5C applies a per-node byte multiplier
//      (MAC0 * (u8)node+6 >> 4) so an effect can be authored large or small at a fixed distance;
//      0x800281EC applies a per-PARTICLE signed multiplier (MAC0 * (s16)p+6 >> 8) so individual
//      embers differ in size from each other. This one has no size input at all beyond depth.
//   2. SIZE CLASS. Consequently it has no size class either: DQA is the constant 6, always. Only
//      0x800281EC reads a node tag (node+3 == '!') to drop DQA to 4. Since the family repurposes
//      the GTE depth-cue divide as the sprite scale (MAC0 = n*DQA with DQB = 0), DQA is the base
//      size, so "DQA is a constant" and "the scale is uniform" are the same statement twice.
//   3. ANCHOR SOURCE / SHAPE. This one and 0x80027E5C RTPS the NODE's own world anchor (node+0x2C
//      packed VX,VY / node+0x30 VZ) exactly ONCE and emit at most once. 0x800281EC never reads
//      node+0x2C/+0x30 at all — it loops a particle array at node+0x50 and emits per particle.
//   4. WHAT IT DOES WITH THE WRITER'S RESULT. Because it emits at most once, the writer's returned
//      list tail is meaningful and is stored to node+0x38 (0x80027E5C likewise). The swarm calls the
//      writer N times and cannot store a tail, so it resolves the list through 0x80031780 up front
//      and unconditionally instead. Here 0x80031780 is called ONLY on the culled path — this
//      function's two exits are strictly "drew it" xor "retired the list".
//   5. CUE ORDER. The depth cue is cleared on the emit path only, immediately before the writer
//      call; the swarm clears it once ahead of its loop. A culled node here leaves the previous
//      node's cue word untouched.
//
// HOW IT WAS IDENTIFIED (callers first, per CLAUDE.md — not from the neighbourhood)
//   * It is a type-0x20 render node's CUSTOM RENDER FN, read from node+0x18 and dispatched by the
//     field object walk. The native mirror of that walk names this exact address in its type-0x20
//     branch (game/render/render_walk.cpp:696, `rfn == 0x80027CB4u`), and the live confirmation is
//     recorded in docs/findings/render.md (kanban #12/#23): walking HEAD 0x800F2624 at the seaside
//     water-pump vista (replays/bugs/seesaw-weight.pad f10000) finds SIX nodes whose +0x18 is
//     0x80027CB4, all vis=1, alongside eight 0x800281EC particle-flame nodes.
//   * The node+0x18 slot is not written by name anywhere: these render fns are INSTALLED BY COPYING
//     them out of the object descriptor table at 0x800A21C0/0x800A222C, where 0x80027CB4 sits beside
//     0x800281EC and 0x8010BF54 (docs/info/instruments, tools/render_fns.py). That is why a static
//     xref sweep for the address finds no writer, and why the walk is the authority on who calls it.
//   * Its two callees pin the rest. 0x80027A4C is the family's ONE packet writer (a0 = the 8-byte
//     record list, a1 = clut | tpage<<16); it is the emitter's only path to the screen. 0x80031780
//     is the generic 8-byte-list TAIL RESOLVER — it walks the list at a0+0x34 until an entry's tag
//     word at +4 has bit30|bit31 set, then either clears the list or writes the tail to a0+0x38.
//     (It is already natively owned as Collision::listScan, game/player/collision.cpp:126; the name
//     is a misfiling from the subsystem that first RE'd it, the function is not collision-specific.)
//   * NOT a pc_render producer, and deliberately so. The picture for these flames is drawn by the
//     separate native overlay Render::fxSpriteRender (game/render/fx_sprite.cpp), which projects the
//     same anchor host-side so it interpolates at 60fps. THIS file is the byte-exact port of the
//     GUEST emitter: it runs where the guest body ran, writes the same guest memory (the scratchpad
//     handoff, the node's record tail, and whatever the writer appends), and is gated by SBS. The
//     two coexist by design.
//
// TRUE EXTENT: [0x80027CB4, 0x80027E5C), 0x1A8 bytes / 106 instructions. Established three ways,
// not assumed — and NOT by "the next function in the shard", which is a false test here: port_gen's
// live-extent splitter puts the body at generated/shard_7.c:1966-2062, but the next gen function in
// THAT file is gen_func_80028E10 (line 2064), because the recompiler's shard split is not address
// order. The address-adjacent siblings live in shard_2.c. So:
//   1. The gen body's own last basic block, L_80027E4C, is the epilogue (restore ra/s0, sp += 24),
//      which places the last instruction at 0x80027E58.
//   2. Disassembly of that block confirms it exactly: `jr ra` at 0x80027E54 with its delay slot
//      `addiu sp, sp, 24` at 0x80027E58 — so the body ends at 0x80027E5C exclusive.
//   3. 0x80027E5C is itself a KNOWN function entry — it is sibling FUN_80027E5C, and it starts with
//      its own `addiu sp, sp, -24` / `sw s0, 16(sp)` prologue — so the extent cannot run on.
//   abi_extract's 4 "unreachable blocks" are the recompiler's duplicated `return;` tails inside this
//   extent (the body has one shared epilogue reached from four places), not a folded sibling.
//
// THE TRAP IN THE GATE: the OT-key scratchpad slot IS the working variable. The guest computes the
// bucket key in place, storing after every step, so even a CULLED node leaves its whole arithmetic
// trail in the scratchpad — those intermediate stores are guest-visible state and SBS compares them.
// A native rewrite that computed the key in a register and stored once would be wrong six ways.
#include "fx_sprite_anchored.h"
#include "core.h"
#include "game.h"
#include "guest_abi.h"
#include "override_registry.h"
#include "rec_decls.h"

namespace {

// --- the scene camera the emitters project through ------------------------------------------------
// Eight consecutive words parked in the scratchpad by the scene pass: the 3x3 rotation matrix
// (packed two shorts per word) followed by the translation. They land in GTE control registers 0..7.
constexpr uint32_t kSceneCameraCrs   = 0x1F8000F8u;
constexpr uint32_t kSceneCameraWords = 8;

// --- GTE registers this body touches, by name -----------------------------------------------------
constexpr uint32_t kGteVxy0 = 0;   // data  — world X,Y of the vertex to project
constexpr uint32_t kGteVz0  = 1;   // data  — world Z
constexpr uint32_t kGteSxy2 = 14;  // data  — projected screen X,Y
constexpr uint32_t kGteSz3  = 19;  // data  — projected depth
constexpr uint32_t kGteMac0 = 24;  // data  — the depth-cue product, used here as the pixel scale
constexpr uint32_t kGteDqa  = 27;  // ctrl  — depth-cue scale numerator
constexpr uint32_t kGteDqb  = 28;  // ctrl  — depth-cue offset
constexpr uint32_t kGteFlag = 31;  // ctrl  — saturation/overflow flags; bit31 = any error
constexpr uint32_t kGteRtps = 0x4A180001u;  // RTPS, sf=1 lm=0 — project one vertex

// --- the depth-cue-as-scale contract ---------------------------------------------------------------
// DQB is forced to 0, so after RTPS the GTE leaves MAC0 = n*DQA with n the perspective divide. DQA
// is therefore the base sprite size in disguise — and this member has exactly one, hence "uniform".
constexpr uint32_t kDqaUniform  = 6;
constexpr int32_t  kDepthCueOff = 0;  // IR0 = 0 -> the writer's colour cue is the identity

// --- the OT bucket key gate ------------------------------------------------------------------------
// The projected depth is folded into a LOGARITHMIC bucket index: the top bits pick a band, the band
// index both shifts the key down and offsets it into that band's slice of the table. Keys outside
// [4, 0x7FF] mean "too near or too far to draw" and the node is dropped.
constexpr int32_t  kOtKeyMin      = 4;
constexpr int32_t  kOtKeyBandBits = 10;
constexpr int32_t  kOtKeyBandSize = 0x200;
constexpr uint32_t kOtKeySpan     = 0x7FCu;  // valid keys are kOtKeyMin .. kOtKeyMin+kOtKeySpan-1
constexpr int32_t  kOtKeyCulled   = -1;
constexpr int32_t  kOtDepthShift  = 2;       // SZ3 >> 2 before the node's bias is added

// --- guest ABI (tools/abi_extract.py 80027CB4 --contract / --scaffold --guestabi) --------------------
// Return addresses at this function's two jal sites.
constexpr uint32_t kRaListTailResolve = 0x80027E20u;  // jal 0x80031780
constexpr uint32_t kRaSpriteWriter    = 0x80027E48u;  // jal 0x80027A4C

// Guest stack frame: 24 bytes spilling s0 and ra, in program order.
constexpr GuestFrameSpill kSpills[2] = {
  { 16, 16 },
  { 31 /*ra*/, 20 },
};

}  // namespace

void FxSpriteAnchored::loadSceneCameraToGte(Core* c) {
  for (uint32_t reg = 0; reg < kSceneCameraWords; reg++)
    gte_write_ctrl(reg, c->mem_r32(kSceneCameraCrs + reg * 4));
}

// PORT_GEN: 80027CB4 generated/shard_7.c:1966-2062
// ORACLE: gen_func_80027CB4
// FUN_80027CB4 — stamp this node's sprite cluster ONCE at the node's own world anchor, sized purely
// by distance. a0 = the type-0x20 render node.
void FxSpriteAnchored::emitUniformScale(Core* c) {
  GuestFrame<24, 2> frame(c, kSpills);

  // The node pointer stays LIVE in a callee-saved register across the calls below. Callees spill
  // their caller's s-registers into their own guest frames, so a plain C++ local here would leave
  // stale bytes on the guest stack (guest_abi.h's raison d'etre).
  GuestReg<16> nodeReg(c);
  nodeReg = c->r[4];

  FxAnchoredNode node{c, c->r[16]};
  FxSpritePublish publish{c};

  // No sprite records means there is nothing to stamp and nothing to retire.
  if (node.recordHead() == 0) return;

  // Project through the pure scene camera, with the depth-cue divide repurposed as the sprite scale:
  // DQB = 0, DQA = the one and only size this member has.
  loadSceneCameraToGte(c);
  gte_write_ctrl(kGteDqa, kDqaUniform);
  gte_write_ctrl(kGteDqb, 0);

  gte_write_data(kGteVxy0, node.worldAnchorXY());
  gte_write_data(kGteVz0, node.worldAnchorZ());
  const int32_t otBias = node.otBias();   // the high half of the word just fed to VZ0
  gte_op(c, kGteRtps);

  // The OT key slot IS the working variable — see the gate trap in the banner.
  publish.setOtKey((int32_t)gte_read_ctrl(kGteFlag));
  bool culled = true;
  if (publish.otKey() >= 0) {
    publish.setOtKey((int32_t)gte_read_data(kGteSz3));
    const int32_t depth = publish.otKey();
    if (depth > 0) {
      publish.setOtKey((depth >> kOtDepthShift) + otBias);
      if (publish.otKey() < kOtKeyMin) publish.setOtKey(kOtKeyMin);

      const int32_t key  = publish.otKey();
      const int32_t band = key >> kOtKeyBandBits;
      publish.setOtKey((key >> (band & 31)) + band * kOtKeyBandSize);
      if ((uint32_t)(publish.otKey() - kOtKeyMin) >= kOtKeySpan) publish.setOtKey(kOtKeyCulled);

      if (publish.otKey() >= 0) {
        publish.setScreenXY(gte_read_data(kGteSxy2));
        publish.setScaleX((int32_t)gte_read_data(kGteMac0));
        culled = false;
      }
    }
  }

  // Off screen / out of depth range: draw nothing, and hand the record list to the generic tail
  // resolver instead. This is the exclusive alternative to emitting — never both.
  if (culled) {
    c->r[4] = c->r[16];
    guest_call(c, kRaListTailResolve, func_80031780);
    return;
  }

  // UNIFORM: the depth-derived scale goes through untouched, and both axes get it — the sprite is
  // square, and its only size input was how far away the anchor turned out to be.
  const int32_t scale = publish.scaleX();
  publish.setDepthCue(kDepthCueOff);
  publish.setScaleY(scale);

  c->r[4] = node.recordHead();                          // a0 = the sprite-record list
  c->r[5] = (node.texturePage() << 16) | node.clut();   // a1 = the texture binding
  guest_call(c, kRaSpriteWriter, func_80027A4C);

  // Emitting once means the writer's returned list tail is meaningful; remember it on the node.
  node.setRecordTail(c->r[2]);
}

void FxSpriteAnchored::registerOverrides(Game*) {
  overrides::install(0x80027CB4u, "FxSpriteAnchored::emitUniformScale",
                     &FxSpriteAnchored::emitUniformScale, gen_func_80027CB4, shard_set_override);
}
