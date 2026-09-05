// game/render/fx_sprite_swarm.cpp — FUN_800281EC, the PER-PARTICLE member of the FUN_80027A4C
// world-anchored scaled-sprite family.
//
// WHAT IT IS, IN GAME TERMS
// Tomba!2 builds its small burning/glittering effects — the seaside torch and hut-roof flames are
// the confirmed live case — out of a "sprite cluster": a short list of 8-byte quad records that one
// shared writer (FUN_80027A4C) turns into GPU primitives around a single screen anchor. Three
// emitters feed that writer. Two of them stamp the cluster ONCE, at the node's own position. THIS
// one is the SWARM: the node carries an array of particles, and the emitter stamps the whole sprite
// cluster once per particle, each particle supplying its OWN world position and its OWN size. That
// is what makes a flame a cluster of independently drifting, independently sized embers instead of
// one billboard — a particle system whose "texture" is the node's sprite cluster.
//
// WHAT SEPARATES IT FROM ITS TWO SIBLINGS (render.h groups all three; they are NOT just three
// scale rules — this one has a different SHAPE). All three verified from the same Ghidra dump,
// scratch/decomp/fx_emit_leaves.c, which decompiles 0x80027CB4 / 0x80027E5C / 0x800281EC together:
//   1. ANCHOR SOURCE. 0x80027CB4 and 0x80027E5C RTPS the NODE's own world anchor (node+0x2C packed
//      VX,VY / node+0x30 VZ) exactly once. 0x800281EC never touches node+0x2C/+0x30 at all. Its
//      anchors are the particle array at node+0x50, stride 8, count (s16) node+0x4E, each particle
//      RTPS'd on its own.
//   2. MULTI-EMIT, AND WHAT THAT COSTS. The siblings call the writer at most once and store its
//      return value into node+0x38 (the record list's tail). This one calls the writer once per
//      SURVIVING particle and DISCARDS the return — after N appends a single returned tail would be
//      meaningless. So it instead resolves the tail UP FRONT and UNCONDITIONALLY through 0x80031780,
//      where the siblings call that only on their culled path. Every particle re-walks the SAME
//      record list (the head is captured before the resolver runs, which may clear node+0x34).
//   3. SCALE. Sibling scale is MAC0 (uniform) or MAC0*(u8)node+6 >> 4. This one is
//      MAC0*(s16)particle+6 >> 8 — per particle, signed, and a twice-finer fraction, because it is
//      sizing individual embers rather than picking one size for the whole effect.
//   4. SIZE CLASS. It is the only member with a per-node size class: node+3 == '!' programs DQA=4
//      instead of 6. Since the family repurposes the GTE depth-cue divide as the sprite scale
//      (MAC0 = n*DQA, DQB=0), that is a flat 2/3 base size for the whole node.
//   5. It clears the depth-cue slot ONCE, before the loop; the siblings clear it per emit. One cue
//      serves the whole swarm.
//
// HOW IT WAS IDENTIFIED (callers first, per CLAUDE.md — not from the neighbourhood)
//   * It is a type-0x20 render node's CUSTOM RENDER FN, read from node+0x18 and dispatched by the
//     field object walk. The native mirror of that walk names this exact address in its type-0x20
//     branch (game/render/render_walk.cpp, `rfn == 0x800281ECu`), and the live confirmation is
//     recorded in docs/findings/render.md (kanban #12/#23): walking HEAD 0x800F2624 at the seaside
//     water-pump vista (replays/bugs/seesaw-weight.pad f10000) finds EIGHT nodes whose +0x18 is
//     0x800281EC, all vis=1, alongside six 0x80027CB4 roof-flame nodes.
//   * Its output was attributed to actual pixels the same way: packets 0x800C9xxx, tp=(896,0),
//     clut=(1008,192), emitter 0x800281EC, object node 0x800F06D8 / beh_cull_tick_render, via
//     `debug otattr` + the REPL `otattr` command (docs/findings/render.md).
//   * Its two callees pin the rest. 0x80027A4C is the family's ONE packet writer (a0 = the 8-byte
//     record list, a1 = clut | tpage<<16); it is the emitter's only path to the screen. 0x80031780
//     is the generic 8-byte-list TAIL RESOLVER — it walks the list at a0+0x34 until an entry's tag
//     word at +4 has bit30|bit31 set, then either clears the list or writes the tail to a0+0x38.
//     (It is already natively owned as Collision::listScan, game/player/collision.cpp:126; the name
//     is a misfiling from the subsystem that first RE'd it, the function is not collision-specific.)
//   * NOT a pc_render producer, and deliberately so. The picture for these flames is drawn by the
//     separate native overlay Render::fxSpriteRender (game/render/fx_sprite.cpp), which projects the
//     same anchors host-side so they interpolate at 60fps. THIS file is the byte-exact port of the
//     GUEST emitter: it runs where the guest body ran, writes the same guest memory (the scratchpad
//     handoff plus whatever the writer appends), and is gated by SBS. The two coexist by design.
//
// TRUE EXTENT: [0x800281EC, 0x8002847C), 0x290 bytes / 164 instructions. Established three ways,
// not assumed:
//   1. authenticated instruction extents puts the body at authenticated executable/overlay evidence, with
//      guest 0x80027144 before it (line 1658) and guest 0x800293F4 after it (line 1882) — no
//      adjacent sibling was folded in.
//   2. `jr ra` sits at 0x80028474 with its delay slot `addiu sp,sp,56` at 0x80028478 (disas
//      spot-check, run only after the Ghidra decompile).
//   3. 0x8002847C is itself a KNOWN function entry — the four-corner record writer FUN_8002847C
//      that game/render/fx_sprite.cpp's fxAnimSpriteRender reproduces — so the extent cannot run on.
//   abi_extract's 3 "unreachable blocks" are the recorded binary evidence's duplicated `return;` tails inside this
//   extent, not a folded sibling.
//
// THE TRAP IN THE LOOP: the particle walk is a DO-WHILE, not a while. A node whose particle count is
// 0 (or negative) still stamps particle[0]. That is the guest's behaviour and it is load-bearing —
// the count is only ever tested after an emit.
#include "fx_sprite_swarm.h"
#include "core.h"
#include "game.h"
#include "guest_abi.h"
#include "guest_jal.h"
#include "native_override_catalog.h"

namespace {

// --- the scene camera the emitters project through ------------------------------------------------
// Eight consecutive words parked in the scratchpad by the scene pass: the 3x3 rotation matrix
// (packed two shorts per word) followed by the translation. They land in GTE control registers 0..7.
constexpr uint32_t kSceneCameraCrs = 0x1F8000F8u;
constexpr uint32_t kSceneCameraWords = 8;

// --- GTE registers this body touches, by name -----------------------------------------------------
constexpr uint32_t kGteVxy0 = 0;           // data  — world X,Y of the vertex to project
constexpr uint32_t kGteVz0 = 1;            // data  — world Z
constexpr uint32_t kGteSxy2 = 14;          // data  — projected screen X,Y
constexpr uint32_t kGteSz3 = 19;           // data  — projected depth
constexpr uint32_t kGteMac0 = 24;          // data  — the depth-cue product, used here as the pixel scale
constexpr uint32_t kGteDqa = 27;           // ctrl  — depth-cue scale numerator
constexpr uint32_t kGteDqb = 28;           // ctrl  — depth-cue offset
constexpr uint32_t kGteFlag = 31;          // ctrl  — saturation/overflow flags; bit31 = any error
constexpr uint32_t kGteRtps = 0x4A180001u; // RTPS, sf=1 lm=0 — project one vertex

// --- the depth-cue-as-scale contract ---------------------------------------------------------------
// DQB is forced to 0, so after RTPS the GTE leaves MAC0 = n*DQA with n the perspective divide. DQA
// is therefore this node's base sprite size in disguise, and node+3 selects between two size classes.
constexpr uint32_t kSmallSizeClassTag = 0x21u; // '!'
constexpr uint32_t kDqaSmall = 4;
constexpr uint32_t kDqaNormal = 6;
constexpr int32_t kDepthCueOff = 0; // IR0 = 0 -> the writer's colour cue is the identity

// --- the OT bucket key gate ------------------------------------------------------------------------
// The projected depth is folded into a LOGARITHMIC bucket index: the top bits pick a band, the band
// index both shifts the key down and offsets it into that band's slice of the table. Keys outside
// [4, 0x7FF] mean "too near or too far to draw" and the particle is dropped.
constexpr int32_t kOtKeyMin = 4;
constexpr int32_t kOtKeyBandBits = 10;
constexpr int32_t kOtKeyBandSize = 0x200;
constexpr uint32_t kOtKeySpan = 0x7FCu; // valid keys are kOtKeyMin .. kOtKeyMin+kOtKeySpan-1
constexpr int32_t kOtKeyCulled = -1;
constexpr int32_t kOtDepthShift = 2; // SZ3 >> 2 before the node's bias is added

// --- per-particle size ------------------------------------------------------------------------------
constexpr int32_t kParticleSizeShift = 8; // scale = MAC0 * particle.sizeMul >> 8

// --- guest ABI (tools/binary ABI evidence 800281EC --contract / --scaffold --guestabi) --------------------
// Return addresses at this function's two jal sites.
constexpr uint32_t kRaListTailResolve = 0x80028318u; // jal 0x80031780
constexpr uint32_t kRaSpriteWriter = 0x8002842Cu;    // jal 0x80027A4C

// Guest stack frame: 56 bytes spilling s0..s7, fp and ra, in program order.
constexpr GuestFrameSpill kSpills[10] = {
    {17, 20},
    {21, 36},
    {31 /*ra*/, 52},
    {30, 48},
    {23, 44},
    {22, 40},
    {20, 32},
    {19, 28},
    {18, 24},
    {16, 16},
};

// The scratchpad base the guest keeps in s0 and offsets from; kept as its own constant so the
// register mirroring below reads as what it is rather than as a bare literal.
constexpr uint32_t kScratchpadBase = 0x1F800000u;

} // namespace

void FxSpriteSwarm::loadSceneCameraToGte(Core *c) {
  for (uint32_t reg = 0; reg < kSceneCameraWords; reg++) {
    gte_write_ctrl(reg, c->mem_r32(kSceneCameraCrs + reg * 4));
  }
}

// GUEST_ADDRESS: 800281EC authenticated executable/overlay evidence
// ORACLE: guest 0x800281EC
// FUN_800281EC — stamp this node's sprite cluster once per particle, projecting and sizing each
// particle independently. a0 = the type-0x20 render node.
void FxSpriteSwarm::emitPerParticle(Core *c) {
  GuestFrame<56, 10> frame(c, kSpills);

  // The node pointer and the scale-X slot address stay LIVE in callee-saved registers across both
  // calls below. Callees spill their caller's s-registers into their own guest frames, so a plain
  // C++ local here would leave stale bytes on the guest stack (guest_abi.h's raison d'etre).
  GuestReg<17> nodeReg(c);
  nodeReg = c->r[4];
  GuestReg<21> scaleXSlotReg(c);
  scaleXSlotReg = fxpublish::kScaleX;

  const FxSwarmNode node{c, c->r[17]};
  FxSpritePublish publish{c};

  // No sprite records means nothing to stamp, however many particles the node is carrying.
  if (node.recordHead() == 0) {
    return;
  }

  // Project through the pure scene camera, with the depth-cue divide repurposed as the sprite scale:
  // DQB = 0, DQA = this node's size class.
  loadSceneCameraToGte(c);
  gte_write_ctrl(kGteDqa, node.sizeClassTag() == kSmallSizeClassTag ? kDqaSmall : kDqaNormal);
  gte_write_ctrl(kGteDqb, 0);

  GuestReg<19> particleIndexReg(c);
  particleIndexReg = 0;
  GuestReg<18> particleReg(c);
  particleReg = node.particleArray();

  publish.setDepthCue(kDepthCueOff); // one cue for the whole swarm

  // Capture the record list head BEFORE resolving the tail — the resolver may clear node+0x34, and
  // every particle stamps this same list.
  GuestReg<22> recordHeadReg(c);
  recordHeadReg = node.recordHead();

  c->r[4] = c->r[17];
  tomba::guest::dispatchJalToReturn(*c, 0x80031780u, kRaListTailResolve);

  // The remaining publish-slot addresses the guest also parks in callee-saved registers (s0 holds
  // the scratchpad base, s4/s7/fp the individual slots; fp duplicates s5's scale-X address).
  GuestReg<16> scratchpadBaseReg(c);
  scratchpadBaseReg = kScratchpadBase;
  GuestReg<20> otKeySlotReg(c);
  otKeySlotReg = fxpublish::kOtKey;
  GuestReg<30> scaleXSlotAliasReg(c);
  scaleXSlotAliasReg = fxpublish::kScaleX;
  GuestReg<23> screenXySlotReg(c);
  screenXySlotReg = fxpublish::kScreenXY;

  do {
    const FxSwarmParticle particle{c, c->r[18]};

    gte_write_data(kGteVxy0, particle.worldXY());
    gte_write_data(kGteVz0, particle.worldZ());
    const int32_t otBias = node.otBias();
    gte_op(c, kGteRtps);

    // The OT key slot IS the working variable: the guest computes the bucket key in place, storing
    // after every step, which is why a culled particle still leaves its trail in the scratchpad.
    publish.setOtKey((int32_t)gte_read_ctrl(kGteFlag));
    bool culled = true;
    if (publish.otKey() >= 0) {
      publish.setOtKey((int32_t)gte_read_data(kGteSz3));
      const int32_t depth = publish.otKey();
      if (depth > 0) {
        publish.setOtKey((depth >> kOtDepthShift) + otBias);
        if (publish.otKey() < kOtKeyMin) {
          publish.setOtKey(kOtKeyMin);
        }

        const int32_t key = publish.otKey();
        const int32_t band = key >> kOtKeyBandBits;
        publish.setOtKey((key >> (band & 31)) + band * kOtKeyBandSize);
        if ((uint32_t)(publish.otKey() - kOtKeyMin) >= kOtKeySpan) {
          publish.setOtKey(kOtKeyCulled);
        }

        if (publish.otKey() >= 0) {
          publish.setScreenXY(gte_read_data(kGteSxy2));
          publish.setScaleX((int32_t)gte_read_data(kGteMac0));
          culled = false;
        }
      }
    }

    if (!culled) {
      // This particle's own size, applied to the depth-derived base scale. Through guest_mult
      // because hi/lo are guest-visible state.
      guest_mult(c, publish.scaleX(), particle.sizeMul());
      const int32_t scale = (int32_t)c->lo >> kParticleSizeShift;

      c->r[4] = c->r[22]; // a0 = the shared record list
      publish.setScaleX(scale);
      publish.setScaleY(scale);                           // square: one size, both axes
      c->r[5] = (node.texturePage() << 16) | node.clut(); // a1 = the texture binding
      tomba::guest::dispatchJalToReturn(*c, 0x80027A4Cu, kRaSpriteWriter);
    }

    particleIndexReg = particleIndexReg + 1;
    particleReg = particleReg + fxswarm::kParticleStride;
    // The count is compared against the index truncated to 16 bits, and only AFTER an emit — see the
    // do-while trap in the banner.
  } while ((int32_t)(int16_t)(uint32_t)particleIndexReg < node.particleCount());
}

void FxSpriteSwarm::registerOverrides(Game *) {
  tomba::native::declareOverride(0x800281ECu, "FxSpriteSwarm::emitPerParticle", &FxSpriteSwarm::emitPerParticle);
}
