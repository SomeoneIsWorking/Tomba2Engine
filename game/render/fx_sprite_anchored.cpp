// game/render/fx_sprite_anchored.cpp — FUN_80027CB4 and FUN_80027E5C, the two SINGLE-ANCHOR members
// of the FUN_80027A4C world-anchored scaled-sprite family. One banner per member follows; the second
// (FUN_80027E5C, "byte scale") begins at "MEMBER TWO" below.
//
// ================================ MEMBER ONE — FUN_80027CB4 =====================================
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
//
// ================================ MEMBER TWO — FUN_80027E5C =====================================
//
// WHAT IT IS, IN GAME TERMS
// The same single stamp as member one — take the node's own world position, project it once, put the
// whole 8-byte-record sprite cluster there — with ONE thing added: the pixel size that came out of
// the perspective divide is then multiplied by the node's OWN size byte (node+6) as 4.4 fixed point.
// So distance still halves the sprite when it doubles away, but on top of that each node carries a
// size of its own that does not change over its life. That is what an effect needs when several
// copies of the SAME effect must not look stamped from one die: 16 in that byte means "leave the
// size alone", and what the controllers actually write is around 8, i.e. half. Its confirmed live
// role is the SPRITE half of the weapon-impact burst (kanban #15) — the flash quad that appears
// where Tomba's weapon connects, paired with the burst's effect mesh.
//
// WHAT SEPARATES IT FROM ITS TWO SIBLINGS
//   1. SCALE — the family's whole distinguishing axis, and this member is the middle case.
//      0x80027CB4 publishes the GTE's MAC0 untouched; THIS one rescales it PER NODE by a u8 read
//      once (MAC0 * (u8)node+6 >> 4); 0x800281EC rescales it PER PARTICLE by a s16 read out of each
//      particle record (MAC0 * (s16)p+6 >> 8). Per-node vs per-particle is the real difference in
//      what you SEE: here every quad in the cluster shares one size, so the whole burst is uniformly
//      bigger or smaller than the next burst; in the swarm the embers differ from each OTHER.
//      Note the shifts differ too — 4 here (4.4 fixed point) against 8 there (8.8) — so the two
//      multipliers are not interchangeable units even though the code shape rhymes.
//   2. IT RE-PUBLISHES scaleX. This is the one place the store SEQUENCE differs from member one and
//      it is not cosmetic. Member one leaves 0x1F800084 holding the raw MAC0 it stored during the
//      OT gate and only copies it to scaleY. This one reads that word back, multiplies, and writes
//      the product to BOTH 0x1F800084 and 0x1F800088 — an extra guest store on the emit path that
//      SBS compares. (The inherited note in this file's header called the difference "only a
//      `* (u8)node+6 >> 4` on the scale", which is true of the ARITHMETIC and understates the
//      store trail; corrected here.)
//   3. SIZE CLASS: none, same as member one — DQA is the constant 6, always. Only 0x800281EC reads
//      a node tag (node+3 == '!') to drop DQA to 4.
//   4. Everything else is byte-for-byte the same function as member one: same prologue, same scene
//      camera load, same DQB=0, same RTPS of node+0x2C/+0x30, the same six-store OT bucket gate with
//      the same bias from node+0x32, the same "culled xor emitted" pair of exits, the same
//      0x80031780 tail-retire on the culled path, and the same store of the writer's returned tail
//      to node+0x38. The two bodies are kept separate anyway — see the note on the class in the
//      header for why factoring the shared half out would break the equivalence gate.
//
// HOW IT WAS IDENTIFIED (callers first, per CLAUDE.md)
//   * Its ONLY static caller is 0x80033080, and that function is nothing but this pair:
//     `{ FUN_80027E5C(node); FUN_800288AC(node); }` (generated/shard_6.c:3286 — a 24-byte frame, two
//     jals, nothing else). 0x80033080 is the weapon-impact burst's node render fn: the native walk
//     dispatches it by name (game/render/render_walk.cpp:754 -> Render::impactBurstRender) and
//     kanban #15 identifies it as the impact effect. So this emitter draws the burst's sprite and
//     0x800288AC draws its mesh.
//   * It is ALSO a node's own render fn in its own right — render_walk.cpp:696 whitelists
//     0x80027E5Cu beside 0x80027CB4u/0x800281ECu in the type-0x20 branch, and the native overlay
//     names it FN_BYTESCALE with its size byte at kE5cScale = 0x06 (game/render/fx_sprite.cpp:100,
//     107). As with member one there is NO static xref that installs it into node+0x18, because
//     these fns are installed by COPYING a pointer out of the effect-descriptor table at
//     0x800A21C0 — tools/render_fns.py's header records exactly that, and records that a quiet run
//     of that scan therefore proves nothing.
//   * The size byte is spawn-time state, not a link-time constant. The same-layout controller
//     0x8002918C (it moves node+0x38 back into node+0x34, i.e. it recycles this family's record
//     list) writes node+6 from a randomized draw: `(FUN_8009A450() >> 11) + 8`, or `(>>12) + 3` for
//     a node tagged node+3 == '5'. NOT PROVEN to be this emitter's own controller — both are
//     table-dispatched, so no static edge exists in either direction — but it is direct evidence
//     that the byte varies per instance, which is what makes the per-node multiplier worth having.
//   * NOT a pc_render producer. The picture is drawn by Render::impactBurstRender /
//     Render::fxAltAnimSpriteRender in game/render/fx_sprite.cpp, which re-derive the same anchor
//     and the same node[6] scale host-side so they lerp at 60fps. THIS is the byte-exact port of the
//     GUEST emitter: same guest memory, gated by SBS. The two coexist by design.
//
// TRUE EXTENT: [0x80027E5C, 0x8002801C), 0x1C0 bytes / 112 instructions. Established three ways, and
// again NOT by "the next function in the shard", which is false here for the second time in this
// family: the body is generated/shard_0.c:1719-1820 and the next gen function in THAT file is
// gen_func_8002918C, while the address-adjacent neighbours live in shard_7 (0x80027CB4) and shard_1
// (0x8002801C). The recompiler's shard split is not address order. So:
//   1. The gen body's own last basic block, L_8002800C, is the single shared epilogue (restore
//      ra from sp+20, s0 from sp+16, sp += 24).
//   2. Disassembly confirms it: `jal 0x80027a4c` at 0x80028000 with delay slot at 0x80028004 (hence
//      the ra constant 0x80028008), `sw v0, 56(s0)` at 0x80028008, then `jr ra` at 0x80028014 with
//      its delay slot `addiu sp, sp, 24` at 0x80028018 — last instruction at 0x80028018.
//   3. 0x8002801C is itself a KNOWN function entry (generated/shard_1.c:2984 gen_func_8002801C) with
//      its own `addiu sp,-24` / `sw s0,16(sp)` prologue, and it is already whitelisted in
//      render_walk.cpp:697 as a FOURTH family member (separate-XY-scale). The extent cannot run on.
//   abi_extract's 4 "unreachable blocks" are the recompiler's duplicated `return;` tails, not a
//   folded sibling — the body has one shared epilogue reached from four places.
//
// TOOL DEFECT WORTH KNOWING (not worked around, just not relied on): `abi_extract.py --contract`
// reports only ONE of this function's two call sites — the func_80031780 one at ra 0x80027FC8 — and
// silently omits the func_80027A4C call at ra 0x80028008. It does the same on member one (it finds
// 0x80027E20 and omits 0x80027E48). The pattern in both is a jal that FALLS THROUGH to the epilogue
// label rather than ending its block with a goto. Both ra constants below therefore come from the
// gen body and the disassembly above, not from the contract dump.
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
// is therefore the base sprite size in disguise. BOTH single-anchor members hard-code the same 6:
// emitUniformScale publishes that base untouched (hence "uniform"), emitByteScale then rescales it
// by the node's own byte. Only the swarm sibling picks DQA from a node tag.
constexpr uint32_t kDqaBase     = 6;
constexpr int32_t  kDepthCueOff = 0;  // IR0 = 0 -> the writer's colour cue is the identity

// --- emitByteScale's per-node size multiplier ------------------------------------------------------
// node+6 is a 4.4 fixed-point numerator: the published pixel scale becomes (MAC0 * node[6]) >> 4, so
// 16 is unity. The guest uses a real MIPS `mult` and consumes only LO, so the product is truncated to
// 32 bits BEFORE the arithmetic shift — reproduced exactly below (hi/lo are guest-visible state).
constexpr int32_t kScaleByteShift = 4;

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

// --- guest ABI (tools/abi_extract.py <addr> --contract / --scaffold --guestabi) ----------------------
// Return addresses at each member's two jal sites. See the TOOL DEFECT note in the banner: the
// contract dump omits the second jal of each member, so these come from the gen body + disassembly.
constexpr uint32_t kRaUniformListTailResolve = 0x80027E20u;  // 0x80027CB4: jal 0x80031780
constexpr uint32_t kRaUniformSpriteWriter    = 0x80027E48u;  // 0x80027CB4: jal 0x80027A4C
constexpr uint32_t kRaByteListTailResolve    = 0x80027FC8u;  // 0x80027E5C: jal 0x80031780
constexpr uint32_t kRaByteSpriteWriter       = 0x80028008u;  // 0x80027E5C: jal 0x80027A4C

// Guest stack frame: 24 bytes spilling s0 and ra, in program order. Identical for both members.
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
  gte_write_ctrl(kGteDqa, kDqaBase);
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
    guest_call(c, kRaUniformListTailResolve, func_80031780);
    return;
  }

  // UNIFORM: the depth-derived scale goes through untouched, and both axes get it — the sprite is
  // square, and its only size input was how far away the anchor turned out to be.
  const int32_t scale = publish.scaleX();
  publish.setDepthCue(kDepthCueOff);
  publish.setScaleY(scale);

  c->r[4] = node.recordHead();                          // a0 = the sprite-record list
  c->r[5] = (node.texturePage() << 16) | node.clut();   // a1 = the texture binding
  guest_call(c, kRaUniformSpriteWriter, func_80027A4C);

  // Emitting once means the writer's returned list tail is meaningful; remember it on the node.
  node.setRecordTail(c->r[2]);
}

// PORT_GEN: 80027E5C generated/shard_0.c:1719-1820
// ORACLE: gen_func_80027E5C
// FUN_80027E5C — stamp this node's sprite cluster ONCE at the node's own world anchor, at the
// distance-derived size RESCALED by the node's own 4.4 size byte. a0 = the type-0x20 render node.
void FxSpriteAnchored::emitByteScale(Core* c) {
  GuestFrame<24, 2> frame(c, kSpills);

  // Same reason as member one: the node pointer must live in the real callee-saved register, because
  // the callees below spill their caller's s-registers into their own guest frames.
  GuestReg<16> nodeReg(c);
  nodeReg = c->r[4];

  FxAnchoredNode node{c, c->r[16]};
  FxSpritePublish publish{c};

  // No sprite records means there is nothing to stamp and nothing to retire.
  if (node.recordHead() == 0) return;

  // Project through the pure scene camera, with the depth-cue divide repurposed as the sprite scale:
  // DQB = 0, DQA = the family's base size. The node's own byte modulates the result, not DQA.
  loadSceneCameraToGte(c);
  gte_write_ctrl(kGteDqa, kDqaBase);
  gte_write_ctrl(kGteDqb, 0);

  gte_write_data(kGteVxy0, node.worldAnchorXY());
  gte_write_data(kGteVz0, node.worldAnchorZ());
  const int32_t otBias = node.otBias();   // the high half of the word just fed to VZ0
  gte_op(c, kGteRtps);

  // The OT key slot IS the working variable — see the gate trap in member one's banner. Identical
  // gate, identical store trail, including on the culled path.
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
    guest_call(c, kRaByteListTailResolve, func_80031780);
    return;
  }

  // BYTE-SCALED: read the depth-derived scale back out of the handoff, rescale it by the node's own
  // size byte, and RE-PUBLISH it — unlike member one this overwrites scaleX as well as scaleY, which
  // is the one extra store that separates the two bodies. The guest multiplies with a real MIPS
  // `mult` and takes LO only, so the product is truncated to 32 bits before the arithmetic shift.
  const int64_t product = guest_mult(c, publish.scaleX(), (int32_t)node.scaleByte());
  const int32_t scale   = (int32_t)(uint32_t)product >> kScaleByteShift;
  publish.setDepthCue(kDepthCueOff);
  publish.setScaleX(scale);
  publish.setScaleY(scale);

  c->r[4] = node.recordHead();                          // a0 = the sprite-record list
  c->r[5] = (node.texturePage() << 16) | node.clut();   // a1 = the texture binding
  guest_call(c, kRaByteSpriteWriter, func_80027A4C);

  // Emitting once means the writer's returned list tail is meaningful; remember it on the node.
  node.setRecordTail(c->r[2]);
}

void FxSpriteAnchored::registerOverrides(Game*) {
  overrides::install(0x80027CB4u, "FxSpriteAnchored::emitUniformScale",
                     &FxSpriteAnchored::emitUniformScale, gen_func_80027CB4, shard_set_override);
  overrides::install(0x80027E5Cu, "FxSpriteAnchored::emitByteScale",
                     &FxSpriteAnchored::emitByteScale, gen_func_80027E5C, shard_set_override);
}
