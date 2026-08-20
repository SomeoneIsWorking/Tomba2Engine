// perobj_billboard.cpp — SUBSTRATE MIRROR for the per-object render-TYPE dispatch (FUN_8003CCA4) and
// 3 of its "special effect" billboard/particle-quad leaf renderers (FUN_8003C2D4, FUN_8003C464,
// FUN_8003C8F4). Same band (0x8003xxxx) as perobj_dispatch.cpp's cmdListDispatch/perModeDispatch;
// same ownership mechanism (shard_set_override — these are reached as PLAIN intra-shard C calls from
// the still-substrate walk cluster gen_func_8003BF00/etc, never through rec_dispatch).
//
// RE method: Ghidra headless decompile of a live free-roam RAM dump cross-checked against the ACTUAL
// recompiled body in generated/shard_0.c (C2D4), shard_1.c (C464), shard_4.c (C8F4), shard_5.c (CCA4),
// generated/shard_disp.c (g_override slot wiring) — the recompiler's gte_write_ctrl/gte_write_data/
// gte_op/gte_read_data calls are ground truth, not Ghidra's COP2 pseudo-C. All 4 addresses confirmed
// unowned via tools/codemap.py before porting.
//
// ==================================================================================================
// FUN_8003CCA4 (perObjRenderDispatch, a0=node r4): stores node into the "current render node" scratch
// (0x1F80028C), then selects one of 6 cases by
// `mem8(node+13) & 0xB` (bound-checked < 9) via a 9-slot table at 0x80014EC8. Every valid case runs
// cmdListDispatch() (already owned, FUN_8003CDD8) then, for 4 of the 6 cases, calls one of 5
// still-substrate "special effect" leaves (FUN_8003D584/F344/F3F4/F4C4/F594) with
// (node, poolPtrBeforeCmdListDispatch, poolPtrAfter) — i.e. the packet-pool span cmdListDispatch just
// emitted. None of these 5 leaves fire at seaside (perobj_dispatch.cpp's prior finding); they stay
// substrate, reached as plain guest-ABI calls so they still see the correct pool-pointer bracket.
//
// FUN_8003C2D4 / FUN_8003C464 (billboardCompose1/2, a0=node r4): each builds a "local" transform for a
// billboard-type node and composes it with a persistent camera MATRIX before handing off to
// billboardEmit. Both use a shared per-instance SCRATCHPAD region at 0x1F800000
// (BUF below) that holds ordinary libgte MATRIX structs (m[3][3] int16 row-major + 2 pad bytes + t[3]
// int32 — exactly what Mtx::identity's 8-word write pattern and Math::rotZ/matMul's byte
// reads agree on):
//   BUF+0x00 MAT_A     — C2D4: identity (Mtx::identity). C464: seeded by the still-substrate
//                         FUN_800517BC(node+122/124/126 as s16 x,y,z) instead of identity.
//   BUF+0x20 MAT_ROTZ  — identity, then Z-rotated in place by mem16(node+90) via Math::rotZ.
//   BUF+0x40 MAT_OUT   — Math::matMul(MAT_ROTZ, MAT_A, MAT_OUT) (= MAT_ROTZ for C2D4, since MAT_A is
//                         identity there); .t (=MAT_OUT+0x14) becomes the composed WORLD translation.
//   BUF+0xC0 WORLD_POS — object's world position triple (s16 x3, from node+46/50/54).
//   BUF+0xF8 CAM2      — the SCENE CAMERA view MATRIX at scratchpad 0x1F8000F8 — the EXACT bytes
//                         Fps60::sceneCam reads (m rows @+0xF8..0x109, t @+0x10C..0x117). Read-only
//                         here. (#67 correction: an earlier note called this "main-RAM 0x800C0000";
//                         that was the same mis-base the f117 fix corrected for BUF itself.)
// Both then: load CAM2.m into CR0-4, MVMVA-transform WORLD_POS by it (same opcode as cmdListDispatch's
// world-translate), add CAM2.t into MAT_OUT.t, reload CR0-7 from MAT_OUT (rotation + composed
// translation), and call billboardEmit(node, mem8(node+71)&1).
//
// FUN_8003C8F4 (billboardEmit, a0=node r4, a1=flag r5): resolves the node's active particle SUB-LIST
// (node+56 -> {count:s16@0, byteOff:s16@2}[] indexed by *(s16*)(*(node+56)); node+60 = sub-list base),
// then for each particle (16-byte stride):
//   1. still-substrate FUN_8003B220(a0=guest scratch, a1=0, a2=particle) fills 4 quad-corner vectors
//      (V0..V2 packed for RTPT, V3 for a second RTPS) into REAL GUEST STACK memory at r29+16..+45 —
//      genuine stack addresses (not a host buffer) because a substrate callee writes them via
//      Core::mem_w16/32, and because SBS compares guest RAM including live stack frames (see
//      docs/findings — Animation::attach's guest-stack residual). A real GuestFrame(96) allocation
//      backs this (found the hard way: omitting it — and the callers' own frame allocations —
//      shifted this frame relative to the recomp path and produced a real, reproducible SBS diff).
//   2. RTPT (0x4A280030) projects V0-2 -> SXY0-2; on success stash them at BUF+8/16/24, AVSZ3
//      (0x4B400006) gives a first depth estimate; RTPS (0x4A180001) projects V3, and on success AVSZ4
//      (0x4B68002E) gives the final OTZ-style depth (else the particle is invalid, depth=-1).
//   3. off-screen cull: skip if all 4 corners' X>=320 (unsigned) or all 4 corners' Y>=240.
//   4. quantize the depth into an OT bucket (node+8 signed per-node depth-bias byte, >>10/<<9 rebucket,
//      clamped to the valid <2044 range else reset to -1 and skip).
//   5. still-substrate FUN_8003B054(BUF, particle, flag) fills the packet's color/UV fields; an
//      optional node+92 half-word override and a node+13-selected small case table (6 labels) patch a
//      couple of BUF bytes (sprite index / extra color word) before emission.
//   6. emit a 10-word packet (1 tag word [size=9 | old-OT-head] + 9 data words copied from
//      BUF+4..+36) at the packet-pool tail (0x800BF544), prepended into the OT bucket
//      (*0x800ED8C8 + depth*4) — the identical packet-chain mechanism perobj_dispatch.cpp's
//      cmdListDispatch/perModeDispatch already documents.
#include "core.h"
#include "game.h"
#include "game_ctx.h"
#include "proj_params.h" // ProjParams::pzToOrd — billboardsRender depth normalize
#include "render.h"
#include "render_internal.h" // withObjScope / cur_render_node
#include <cmath>
#include <lucent/log.h> // `bbrot` — the node-rotation rebuild instrument (see BbObjectRot)

void rec_dispatch(Core *, uint32_t);
void shard_set_override(uint32_t addr, OverrideFn fn); // generated/shard_disp.c (C++ linkage)

// gen_func_* fallbacks for the psx_fallback gate. g_override[] is a single PROCESS-GLOBAL table
// shared by EVERY Core (SBS core A AND core B), so the trampolines below MUST defer to the real
// recompiled body on core B (the pure-substrate oracle) — otherwise the oracle runs this native
// mirror and SBS compares native-vs-native (a false 0-div) instead of native-vs-substrate. Same
// discipline as every other shard_set_override cluster (gte_math/node_xform/cull/...). The oracle
// may carry ONLY async→sync conversions (sync_overrides.cpp) + HLE BIOS — nothing engine/game.
extern void gen_func_8003CCA4(Core *);
extern void gen_func_8003C2D4(Core *);
extern void gen_func_8003C464(Core *);
extern void gen_func_8003C788(Core *);
extern void gen_func_8003C5F8(Core *);
extern void gen_func_8003C8F4(Core *);

// Still-substrate leaves called by these 4 (declared, called via plain guest-ABI intra-shard calls —
// exactly as the generated code reaches them; g_override still gates each, so if one is ever owned
// later these calls transparently pick that up).
void func_800517BC(Core *); // C464's MAT_A seed (node+122/124/126 s16 xyz)
void func_8003B220(Core *); // billboardEmit's quad-corner builder (writes real guest stack memory)
void func_8003B054(Core *); // billboardEmit's color/UV fill

namespace {

// ==================================================================================================
// BbObjectRot — the billboard particle layer's OBJECT ROTATION, rebuilt from the node's own fields.
//
// WHY THIS EXISTS (2026-08-05). The record capture in billboardEmit used to take this rotation out of
// GTE CR0-4 and the world anchor out of CR5-7 un-composed against the scene camera. That is the
// mechanism the USER banned outright ("never do this please NEVER"): the camera enters and leaves the
// arithmetic through an s16-quantised matrix, so cam-then-camᵀ is identity only in exact arithmetic
// and the residue is a FUNCTION OF THE CAMERA — the effect vibrates while panning and nothing in the
// game is moving it. The tap was deleted (break-first), the layer measured absent, and this is the
// rebuild. Same shape as CubeTextBanner: recompute from the fields the behaviour owns.
//
// THE FOUR COMPOSE VARIANTS, RE'd from their own already-native bodies below plus the leaves they
// call (game/math/gte_math.cpp is the ground truth for each leaf's element formulas):
//   FUN_8003C2D4 billboardCompose1  MAT_A = identity                       -> R = Rz(node+90)
//   FUN_8003C464 billboardCompose2  MAT_A = FUN_800517BC(node+122/124/126) -> R = Rz(node+90)·diag(s)
//   FUN_8003C788 billboardCompose3  MAT_ROTZ = matMul(node+152, identity)  -> UNRESOLVED, see below
//   FUN_8003C5F8 billboardComposeC5F8 MAT_ROTZ = rotMatSoft(node+84)       -> R = rotmat(node+84..88)
// and each then runs the SHARED tail, which is the only place a camera appears — it composes CAM2
// onto the node's WORLD_POS. That tail's camera work is exactly what the native display pass
// (Render::billboardsRender, via Fps60::sceneCam) does for itself, so the producer needs NONE of it:
// it takes the node's own world triple straight out of node+46/+50/+54, the same halfwords the tail
// copies into WORLD_POS.
//
// FUN_800517BC IS A DIAGONAL SCALE, not a rotation (generated/shard_5.c: it sign-extends a1/a2/a3 and
// stores them as words at +0/+8/+16 with every other word zero — i.e. m00/m11/m22 and a zero
// translation, the packed-halfword MATRIX layout's diagonal). 4096 is unity.
//
// COMPOSE3 IS DELIBERATELY NOT REBUILT, and the reason is measured. node+152 (= node+0x98) is written
// as Math::rotmat(euler @node+0x54) by GraphicsBind::renderUpdateBody (FUN_800517F8), so the obvious
// rebuild is rotmat(node+84/86/88) — the same three angles composeC5F8 uses. The `bbrot` channel was
// added to CHECK that rather than assert it, and it FALSIFIED it: see the note at the compose3 call
// site. So compose3's particles are left absent instead of being drawn under a plausible-looking
// wrong matrix. Math::rotmat (FUN_80085480) and Math::rotMatSoft (FUN_800847F0) do build the same
// Rx·Ry·Rz product (gte_math.cpp, one on the GTE and one in software), so ONE float builder still
// serves composeC5F8; that part of the claim survived.
// The `bbrot` channel prints each variant's recomputation beside the s16 matrix at node+152. It is a
// diagnostic and never feeds the picture — and it is the instrument that caught the compose3 error,
// which is why it stays.
constexpr float kFixedOne = 4096.0f;   // libgte 1.3.12: 4096 == 1.0
constexpr uint32_t kAngleTurn = 4096u; // libgte angle unit: 4096 == one full turn

constexpr uint32_t NODE_ROT_Z = 90u;       // s16 — compose1/2's single Z angle
constexpr uint32_t NODE_EULER = 84u;       // 3× s16 — compose3/C5F8's Euler triple (= node+0x54)
constexpr uint32_t NODE_SCALE_DIAG = 122u; // 3× s16 — compose2's diagonal scale (1.3.12)
constexpr uint32_t NODE_WORLD_X = 46u;     // s16 ×3 (+46/+50/+54) — the node's own world position,
constexpr uint32_t NODE_WORLD_Y = 50u;     //   the exact halfwords billboardComposeTail copies into
constexpr uint32_t NODE_WORLD_Z = 54u;     //   WORLD_POS before composing the camera onto them
constexpr uint32_t NODE_OWN_MATRIX = 152u; // the node's own 3×3 (diagnostic cross-check only)

struct Mat3f {
  float m[3][3];
};

Mat3f mul3(const Mat3f &a, const Mat3f &b) { // a · b
  Mat3f o{};
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 3; j++) {
      o.m[i][j] = a.m[i][0] * b.m[0][j] + a.m[i][1] * b.m[1][j] + a.m[i][2] * b.m[2][j];
    }
  }
  return o;
}

float angleToRadians(int16_t a) {
  return (float)a * 6.28318530718f / (float)kAngleTurn;
}

// Math::rotZ (FUN_80085050) applied to the identity: rotpair rotates matrix ROWS 0 and 1 as
// row0' = cos·row0 − sin·row1, row1' = sin·row0 + cos·row1, which on the identity leaves the plain
// Z rotation. Float instead of the s16/LUT chain because a native producer owns its own precision.
Mat3f rotZ3(int16_t angle) {
  const float a = angleToRadians(angle), s = std::sin(a), c = std::cos(a);
  return Mat3f{{{c, -s, 0}, {s, c, 0}, {0, 0, 1}}};
}

// Math::rotMatSoft (FUN_800847F0) / Math::rotmat (FUN_80085480) — Rx(vx)·Ry(vy)·Rz(vz), transcribed
// from rotMatSoft's element formulas in gte_math.cpp with the >>12 fixed-point steps replaced by
// exact float. (m02 is +sinB there because the asm stores the RAW sinB.)
Mat3f rotEuler3(int16_t ax, int16_t ay, int16_t az) {
  const float A = angleToRadians(ax), B = angleToRadians(ay), C = angleToRadians(az);
  const float sA = std::sin(A), cA = std::cos(A);
  const float sB = std::sin(B), cB = std::cos(B);
  const float sC = std::sin(C), cC = std::cos(C);
  Mat3f r{};
  r.m[0][0] = cC * cB;
  r.m[0][1] = -sC * cB;
  r.m[0][2] = sB;
  r.m[1][0] = sC * cA + cC * sB * sA;
  r.m[1][1] = cC * cA - sC * sB * sA;
  r.m[1][2] = -cB * sA;
  r.m[2][0] = sC * sA - cC * sB * cA;
  r.m[2][1] = cC * sA + sC * sB * cA;
  r.m[2][2] = cB * cA;
  return r;
}

// FUN_800517BC's diagonal scale, in unit scale.
Mat3f diagScale3(int16_t x, int16_t y, int16_t z) {
  return Mat3f{{{(float)x / kFixedOne, 0, 0}, {0, (float)y / kFixedOne, 0}, {0, 0, (float)z / kFixedOne}}};
}

// Publish the active compose variant's object rotation for billboardEmit's record capture, and clear
// it again on the way out so no node can inherit a neighbour's matrix. RAII because each compose
// method has several exits once its epilogue restore is counted.
class BbRotScope {
public:
  BbRotScope(Core *c, uint32_t node, const char *variant, const Mat3f &r) : mCore(c) {
    Render *rr = rend(c);
    for (int i = 0; i < 3; i++) {
      for (int j = 0; j < 3; j++) {
        rr->mBbRot[i][j] = r.m[i][j];
      }
    }
    rr->mBbRotValid = true;
    // The instrument for the node+152 claim in the banner above: it prints the recomputation NEXT TO
    // the s16 matrix the guest holds, so "these are the same matrix" is checkable on live data. The
    // guest read happens only inside the log arguments, which lucent does not evaluate when the
    // channel is off, and it never reaches the picture.
    // guest_node152 is only the NODE'S OWN matrix for the compose3 variant (the only one that reads
    // it); for the others that memory belongs to something else, which is why the variant is printed
    // beside it — a reader comparing the two columns on a compose1 line would be comparing noise.
    lucent::debug("bbrot",
                  "node={:08X} variant={} rebuilt=[{:.4f} {:.4f} {:.4f} | {:.4f} {:.4f} {:.4f} | "
                  "{:.4f} {:.4f} {:.4f}] guest_node152=[{} {} {} | {} {} {} | {} {} {}]",
                  node,
                  variant,
                  r.m[0][0],
                  r.m[0][1],
                  r.m[0][2],
                  r.m[1][0],
                  r.m[1][1],
                  r.m[1][2],
                  r.m[2][0],
                  r.m[2][1],
                  r.m[2][2],
                  c->mem_r16s(node + NODE_OWN_MATRIX + 0),
                  c->mem_r16s(node + NODE_OWN_MATRIX + 2),
                  c->mem_r16s(node + NODE_OWN_MATRIX + 4),
                  c->mem_r16s(node + NODE_OWN_MATRIX + 6),
                  c->mem_r16s(node + NODE_OWN_MATRIX + 8),
                  c->mem_r16s(node + NODE_OWN_MATRIX + 10),
                  c->mem_r16s(node + NODE_OWN_MATRIX + 12),
                  c->mem_r16s(node + NODE_OWN_MATRIX + 14),
                  c->mem_r16s(node + NODE_OWN_MATRIX + 16));
  }
  ~BbRotScope() {
    rend(mCore)->mBbRotValid = false;
  }

private:
  Core *mCore;
};

constexpr uint32_t CUR_NODE_SCR = 0x1F80028Cu; // "current render node" scratch
constexpr uint32_t PKT_POOL_PTR = 0x800BF544u; // packet-pool bump-allocator write pointer
constexpr uint32_t OTBASE_PTR = 0x800ED8C8u;   // *this = the active ordering-table base
constexpr uint32_t BUF = 0x1F800000u;          // SCRATCHPAD MATRIX-compose buffer (C2D4/C464/C8F4) —
                                               // gen_func_8003C2D4/8003C8F4 base r16/r17 = 8064<<16
                                               // = 0x1F800000, NOT main RAM. (Was wrongly 0x800C0000;
                                               // the mis-base made every emitted packet's data differ
                                               // from the substrate — the f117 divergence, masked by the
                                               // false 0-div until the oracle-gate fix surfaced it.)
constexpr uint32_t MAT_A = BUF + 0x00u;
constexpr uint32_t MAT_ROTZ = BUF + 0x20u;
constexpr uint32_t MAT_OUT = BUF + 0x40u;
constexpr uint32_t WORLD_POS = BUF + 0xC0u;
constexpr uint32_t CAM2 = BUF + 0xF8u;        // persistent camera MATRIX mirror (read-only here)
constexpr uint32_t MVMVA_TRANS = 0x4A486012u; // same opcode cmdListDispatch uses for the world-translate

// RAII guest-stack frame: real recomp bodies allocate their own stack frame (r29 -= size) before
// running, and callees compute their OWN frame relative to the CALLER'S post-allocation r29 — so a
// callee reached from here (billboardEmit reads its scratch as c->r[29]-96+off) needs r29 to reflect
// the SAME depth the recomp path would have at that call, even though nothing in THIS function's own
// body reads/writes through r29 itself. Symmetric allocate/restore, net-zero like the recomp's own
// push/pop (found empirically: 8003C2D4/8003C464 omitting this shifted 8003C8F4's frame by their own
// size and produced a real, reproducible SBS diff at f118 in the task-0 stack region).
struct GuestFrame {
  Core *c;
  uint32_t size;
  GuestFrame(Core *c_, uint32_t size_) : c(c_), size(size_) {
    c->r[29] -= size;
  }
  ~GuestFrame() {
    c->r[29] += size;
  }
};

// FUN_8003CCA4's REAL prologue (register-faithfulness, f118 root cause, 2026-07-09): unlike the
// call sites above where GuestFrame's bare sp-adjust is enough (their bodies spill live-injected
// values inline themselves), gen_func_8003CCA4 (generated/shard_5.c) actually SPILLS its caller's
// live r16/r17/r18/r31 to guest memory at entry (mem_w32 sp+16/20/24/28) and restores them at every
// exit (L_8003CDC0) — a plain MIPS callee-save prologue/epilogue, not a value injection. The bare
// GuestFrame(c,32) this call site used only adjusted c->r[29] and never wrote those 4 words, leaving
// WHATEVER STALE bytes were already sitting in that guest-stack region (leftover from an unrelated
// earlier writer) instead of the caller's real r16/r17/r18/r31 — the exact SBS diff at
// 0x801FE8B8../0x801FE8E8.. (task-0 stack, several frames into this call chain) that unmasked once
// the f62 register-faithfulness gap (cmdListDispatch's r16=loop-index/r17=SCR, see
// perobj_dispatch.cpp) was fixed and the SBS gate advanced past it. r18 is reassigned to `node`
// immediately after the spill (matching gen's `r18 = r4` right after `mem_w32(sp+24,r18)`); r16/r17
// are pure save/restore (this function's own body never sets them — case 0x8003CD00, the only case
// seaside objects hit, doesn't either, per gen).
struct CCA4Frame {
  Core *c;
  uint32_t s16, s17, s18, sra;
  explicit CCA4Frame(Core *c_) : c(c_), s16(c_->r[16]), s17(c_->r[17]), s18(c_->r[18]), sra(c_->r[31]) {
    c->r[29] -= 32;
    c->mem_w32(c->r[29] + 24, s18);
    c->mem_w32(c->r[29] + 28, sra);
    c->mem_w32(c->r[29] + 20, s17);
    c->mem_w32(c->r[29] + 16, s16);
  }
  ~CCA4Frame() {
    c->r[31] = c->mem_r32(c->r[29] + 28);
    c->r[18] = c->mem_r32(c->r[29] + 24);
    c->r[17] = c->mem_r32(c->r[29] + 20);
    c->r[16] = c->mem_r32(c->r[29] + 16);
    c->r[29] += 32;
  }
};

} // namespace

// ==================================================================================================
// FUN_8003CCA4
void Render::perObjRenderDispatch() {
  Core *c = mCore;
  const uint32_t node = c->r[4];
  withObjScope(c, node, [](Core *c) {
    CCA4Frame frame(c);
    const uint32_t node = c->r[4];
    // Register-faithfulness (2026-07-10, the f118 residual root cause — one level deeper than the
    // FUN_8003C048 ownership fix): gen_func_8003CCA4's REAL prologue (generated/shard_5.c:5060-5071)
    // reassigns r18 = r4 (node) IMMEDIATELY after its own spill, and computes r5 = ((mem8(node+13) ^
    // 15) < 1) ONCE, before the case switch — both values stay LIVE (plain MIPS register lifetime,
    // never re-set per case) all the way to whichever case's `func_8003CDD8(c)` call. This function's
    // own C++ body only ever needed the local `node`, so a prior draft never wrote c->r[18]/c->r[5] —
    // meaning cmdListDispatch's CmdListFrame (which spills "caller r18" as part of its own real
    // prologue) span stale bytes instead of gen's real node/flag, and cmdListDispatch's `flag` param
    // (c->r[5]) silently held garbage instead of gen's real per-node flag. Confirmed via
    // PSXPORT_SBS_PREWATCH=0x801FE8B8: core B's write came from gen_func_8003CDD8+0x18 (its own r18
    // spill) with the caller (gen_func_8003CCA4, reached via FUN_8003C048) holding r18=node, while
    // core A held whatever renderWalk's own r18 (CASE188_SCR, an unrelated constant) still was.
    c->r[18] = node;
    // gen: r3=mem8(node+11); r3^=15; r5=(r3<1) — the FLAG field is node+11 (NOT node+13, which is the
    // separate `sel` case-table index below). A prior draft of this fix used node+13 for both,
    // routing cmdListDispatch's flag&1 test the wrong way and making perModeDispatch pick the
    // per-mode table (native) instead of gen's real generic-fallback path (func_800803DC) for nodes
    // whose real flag has bit0 set — confirmed via PSXPORT_SBS_PREWATCH: core B's chain ended in
    // gen_func_800803DC while core A's ended in the per-mode target ov_a00_gen_80146478.
    const uint32_t flag = ((c->mem_r8(node + 11) ^ 15u) < 1u) ? 1u : 0u;
    c->mem_w32(CUR_NODE_SCR, node);
    const uint32_t sel = c->mem_r8(node + 13) & 11u;
    if (sel >= 9u) {
      return;
    }
    constexpr uint32_t TABLE = 0x80014EC8u;
    const uint32_t target = c->mem_r32(TABLE + sel * 4u);
    // RE'd return-address constants gen sets in r31 immediately before each nested call (see
    // generated/shard_5.c gen_func_8003CCA4). Register-faithfulness (2026-07-09, the f118 residual
    // root cause): a prior draft called cmdListDispatch()/the special-effect leaves without ever
    // setting c->r[31], leaving whatever stale value the OUTER caller (FUN_8003C048) left there
    // instead — a real, reproducible SBS diff at FUN_80146478's own ra spill slot (0x801FE8D0..),
    // several frames deep in this call chain. Mirrored per CLAUDE.md ("MIRROR THE GUEST STACK...
    // register-faithfulness"), same discipline as billboardCompose1/2's own fix (commit bef7769).
    switch (target) {
    case 0x8003CD00u: {
      c->r[4] = node;
      c->r[5] = flag;
      c->r[31] = 0x8003CD08u;
      rend(c)->cmdListDispatch();
      break;
    }
    case 0x8003CD10u: {
      const uint32_t pre = c->mem_r32(PKT_POOL_PTR);
      c->r[4] = node;
      c->r[5] = flag;
      c->r[31] = 0x8003CD20u;
      rend(c)->cmdListDispatch();
      const uint32_t post = c->mem_r32(PKT_POOL_PTR);
      c->r[4] = node;
      c->r[5] = pre;
      c->r[6] = post;
      c->r[31] = 0x8003CD30u;
      rend(c)->effectColorAdd(node, pre, post);
      break;
    }
    case 0x8003CD38u: {
      const uint32_t pre = c->mem_r32(PKT_POOL_PTR);
      c->r[4] = node;
      c->r[5] = flag;
      c->r[31] = 0x8003CD48u;
      rend(c)->cmdListDispatch();
      const uint32_t post = c->mem_r32(PKT_POOL_PTR);
      c->r[4] = node;
      c->r[5] = pre;
      c->r[6] = post;
      c->r[31] = 0x8003CD58u;
      rend(c)->effectClutSwap(node, pre, post);
      break;
    }
    case 0x8003CD60u: {
      const uint32_t pre = c->mem_r32(PKT_POOL_PTR);
      c->r[4] = node;
      c->r[5] = flag;
      c->r[31] = 0x8003CD70u;
      rend(c)->cmdListDispatch();
      const uint32_t post = c->mem_r32(PKT_POOL_PTR);
      c->r[4] = node;
      c->r[5] = pre;
      c->r[6] = post;
      // Branch polarity (2026-07-09, found during the same audit): gen_func_8003CCA4 L_8003CD60
      // tests node+27==0 -> func_8003F4C4 (the L_8003CD90 target), node+27!=0 -> func_8003F3F4 —
      // a prior draft had this INVERTED. Neither leaf fires at seaside (this file's own banner),
      // so the flip was never caught by the autonav gate; fixed here to match gen exactly.
      if (c->mem_r8(node + 27) == 0) {
        c->r[31] = 0x8003CD98u;
        rend(c)->effectSemiOff(node, pre, post);
      } else {
        c->r[31] = 0x8003CD88u;
        rend(c)->effectSemiOn(node, pre, post);
      }
      break;
    }
    case 0x8003CDA0u: {
      const uint32_t pre = c->mem_r32(PKT_POOL_PTR);
      c->r[4] = node;
      c->r[5] = flag;
      c->r[31] = 0x8003CDB0u;
      rend(c)->cmdListDispatch();
      const uint32_t post = c->mem_r32(PKT_POOL_PTR);
      c->r[4] = node;
      c->r[5] = pre;
      c->r[6] = post;
      c->r[31] = 0x8003CDC0u;
      rend(c)->effectFlatTint(node, pre, post);
      break;
    }
    case 0x8003CDC0u:
      break; // no-op case: the recomp body falls straight to the epilogue
    default:
      // Defensive mirror of the recomp's raw `jr` fallback for an unrecognized table entry — never
      // hit by live game data (only the 6 cases above ever appear in the live table).
      rec_dispatch(c, target);
      return;
    }
  });
}

// ==================================================================================================
// Shared tail: compose CAM2 (camera rotation+translation) onto WORLD_POS, add it into outMat.t,
// reload CR0-7 from outMat, then hand off to billboardEmit(node, flag).
// outMat = the BUF slot holding the composed matrix. C2D4/C464 land it in MAT_OUT (BUF+0x40); C788
// (billboardCompose3) composes in place in MAT_ROTZ (BUF+0x20) — its gen loads CR0-7 from BUF+0x20,
// not BUF+0x40 — so the tail is parameterized by that slot instead of hardcoding MAT_OUT.
static void billboardComposeTail(Core *c, uint32_t node, uint32_t flag, uint32_t outMat = MAT_OUT) {
  c->mem_w16(WORLD_POS + 0, c->mem_r16(node + 46));
  c->mem_w16(WORLD_POS + 2, c->mem_r16(node + 50));
  c->mem_w16(WORLD_POS + 4, c->mem_r16(node + 54));
  gte_write_ctrl(0, c->mem_r32(CAM2 + 0));
  gte_write_ctrl(1, c->mem_r32(CAM2 + 4));
  gte_write_ctrl(2, c->mem_r32(CAM2 + 8));
  gte_write_ctrl(3, c->mem_r32(CAM2 + 12));
  gte_write_ctrl(4, c->mem_r32(CAM2 + 16));
  gte_write_data(0, c->mem_r32(WORLD_POS + 0));
  gte_write_data(1, c->mem_r32(WORLD_POS + 4));
  gte_op(c, MVMVA_TRANS);
  c->mem_w32(outMat + 0x14, gte_read_data(25));
  c->mem_w32(outMat + 0x18, gte_read_data(26));
  c->mem_w32(outMat + 0x1C, gte_read_data(27));
  c->mem_w32(outMat + 0x14, c->mem_r32(outMat + 0x14) + c->mem_r32(CAM2 + 0x14));
  c->mem_w32(outMat + 0x18, c->mem_r32(outMat + 0x18) + c->mem_r32(CAM2 + 0x18));
  c->mem_w32(outMat + 0x1C, c->mem_r32(outMat + 0x1C) + c->mem_r32(CAM2 + 0x1C));
  gte_write_ctrl(0, c->mem_r32(outMat + 0));
  gte_write_ctrl(1, c->mem_r32(outMat + 4));
  gte_write_ctrl(2, c->mem_r32(outMat + 8));
  gte_write_ctrl(3, c->mem_r32(outMat + 12));
  gte_write_ctrl(4, c->mem_r32(outMat + 16));
  gte_write_ctrl(5, c->mem_r32(outMat + 0x14));
  gte_write_ctrl(6, c->mem_r32(outMat + 0x18));
  gte_write_ctrl(7, c->mem_r32(outMat + 0x1C));
  c->r[4] = node;
  c->r[5] = flag;
  rend(c)->billboardEmit();
}

// FUN_8003C2D4
void Render::billboardCompose1() {
  Core *c = mCore;
  const uint32_t node = c->r[4];
  if (c->mem_r32(node + 56) == 0) {
    return;
  }
  withObjScope(c, node, [](Core *c) {
    GuestFrame frame(c, 40);
    // Register-faithfulness (gen_func_8003C2D4 prologue, L4509-4514): spill the caller's
    // r16..r19/ra at sp+16..+32. The GuestFrame only allocates the frame; the spill BYTES are
    // what SBS compares (gen writes them; the bare RAII left stale bytes there).
    const uint32_t sp = c->r[29];
    c->mem_w32(sp + 16, c->r[16]);
    c->mem_w32(sp + 20, c->r[17]);
    c->mem_w32(sp + 24, c->r[18]);
    c->mem_w32(sp + 28, c->r[19]);
    c->mem_w32(sp + 32, c->r[31]);
    const uint32_t node = c->r[4];
    mtxOf(c).identity(MAT_A);
    mtxOf(c).identity(MAT_ROTZ);
    mathOf(c).rotZ((int16_t)c->mem_r16(node + 90), MAT_ROTZ);
    const uint32_t flag = c->mem_r8(node + 71) & 1u;
    mathOf(c).matMul(MAT_ROTZ, MAT_A, MAT_OUT);
    // MAT_A is the identity here, so the object rotation IS the Z rotation this node carries.
    BbRotScope bbRot(c, node, "compose1_rotZ", rotZ3(c->mem_r16s(node + NODE_ROT_Z)));
    // gen's live callee-saved state at the func_8003C8F4 call site (L4593-4595): billboardEmit
    // spills these as its "caller" registers, so they must hold gen's values here.
    c->r[16] = MAT_OUT;
    c->r[17] = MAT_A;
    c->r[18] = flag;
    c->r[19] = node;
    c->r[31] = 0x8003C448u;
    billboardComposeTail(c, node, flag);
    // Epilogue restore (gen_func_8003C2D4 L4597-4601): read the caller's values back from the spill
    // slots. MUST restore — the reassignments above (esp. r31=0x8003C448) would otherwise leak to the
    // substrate render-walk caller and corrupt its control flow (registers aren't SBS-compared, but
    // the substrate reads them).
    c->r[16] = c->mem_r32(sp + 16);
    c->r[17] = c->mem_r32(sp + 20);
    c->r[18] = c->mem_r32(sp + 24);
    c->r[19] = c->mem_r32(sp + 28);
    c->r[31] = c->mem_r32(sp + 32);
  });
}

// FUN_8003C464
void Render::billboardCompose2() {
  Core *c = mCore;
  const uint32_t node = c->r[4];
  if (c->mem_r32(node + 56) == 0) {
    return;
  }
  withObjScope(c, node, [](Core *c) {
    GuestFrame frame(c, 32);
    // Register-faithfulness (gen_func_8003C464 prologue, L5907-5911): spill caller's
    // r16/r17/r18/ra at sp+16/+20/+24/+28. (C464's prologue does NOT spill r19 — it passes through.)
    const uint32_t sp = c->r[29];
    c->mem_w32(sp + 16, c->r[16]);
    c->mem_w32(sp + 20, c->r[17]);
    c->mem_w32(sp + 24, c->r[18]);
    c->mem_w32(sp + 28, c->r[31]);
    const uint32_t node = c->r[4];
    c->r[4] = MAT_A;
    c->r[5] = (uint32_t)c->mem_r16s(node + 122);
    c->r[6] = (uint32_t)c->mem_r16s(node + 124);
    c->r[7] = (uint32_t)c->mem_r16s(node + 126);
    func_800517BC(c);
    mtxOf(c).identity(MAT_ROTZ);
    mathOf(c).rotZ((int16_t)c->mem_r16(node + 90), MAT_ROTZ);
    const uint32_t flag = c->mem_r8(node + 71) & 1u;
    mathOf(c).matMul(MAT_ROTZ, MAT_A, MAT_OUT);
    // MAT_A is FUN_800517BC's diagonal scale, so the object transform is the Z rotation times it.
    BbRotScope bbRot(c,
                     node,
                     "compose2_rotZ_diag",
                     mul3(rotZ3(c->mem_r16s(node + NODE_ROT_Z)),
                          diagScale3(c->mem_r16s(node + NODE_SCALE_DIAG + 0),
                                     c->mem_r16s(node + NODE_SCALE_DIAG + 2),
                                     c->mem_r16s(node + NODE_SCALE_DIAG + 4))));
    // gen's live callee-saved state at the func_8003C8F4 call site (L5993-5995) — NOTE C464 differs
    // from C2D4: r17=flag (not MAT_A) and r18=node (gen reassigns r17 to flag at L5931 and keeps
    // r18=node from the prologue). billboardEmit spills these, so match gen exactly.
    c->r[16] = MAT_OUT;
    c->r[17] = flag;
    c->r[18] = node;
    c->r[31] = 0x8003C5E0u;
    billboardComposeTail(c, node, flag);
    // Epilogue restore (gen_func_8003C464): read the caller's values back from the spill slots
    // (r16/r17/r18/ra — C464 does not save r19). Same anti-leak discipline as billboardCompose1.
    c->r[16] = c->mem_r32(sp + 16);
    c->r[17] = c->mem_r32(sp + 20);
    c->r[18] = c->mem_r32(sp + 24);
    c->r[31] = c->mem_r32(sp + 28);
  });
}

// ==================================================================================================
// FUN_8003C788 — billboardCompose3. Third compose sibling of C2D4/C464. Unlike them (which build a
// Z-rotation LOCAL matrix), C788 seeds MAT_A = identity and folds the node's OWN stored 3x3+t matrix
// (node+152) through it — matMul(node+152, MAT_A, MAT_ROTZ) leaves MAT_ROTZ = the node matrix — then
// runs the SAME CAM2 world-translate tail as its siblings, but composing in place in MAT_ROTZ (BUF+0x20)
// rather than MAT_OUT. All callees owned: Mtx::identity (0x80051794), Math::matMul (0x80084110),
// billboardEmit (0x8003C8F4). Frame 32, spills r16/r17/r18/ra like C464 (no r19). See abi_extract
// 0x8003C788 --contract + generated/shard_3.c.
void Render::billboardCompose3() {
  Core *c = mCore;
  const uint32_t node = c->r[4];
  if (c->mem_r32(node + 56) == 0) {
    return;
  }
  withObjScope(c, node, [](Core *c) {
    GuestFrame frame(c, 32);
    // Register-faithfulness (gen_func_8003C788 prologue): spill caller's r16/r17/r18/ra at
    // sp+16/+20/+24/+28 — same 32-byte / 4-spill shape as C464.
    const uint32_t sp = c->r[29];
    c->mem_w32(sp + 16, c->r[16]);
    c->mem_w32(sp + 20, c->r[17]);
    c->mem_w32(sp + 24, c->r[18]);
    c->mem_w32(sp + 28, c->r[31]);
    const uint32_t node = c->r[4];
    mtxOf(c).identity(MAT_A);
    const uint32_t flag = c->mem_r8(node + 71) & 1u;
    mathOf(c).matMul(node + 152, MAT_A, MAT_ROTZ); // MAT_ROTZ = (node+152 matrix) x identity
    // NO ROTATION IS PUBLISHED FOR THIS VARIANT, so its particles are NOT recorded and this class's
    // billboard layer stays ABSENT. Reason, measured not assumed (portmap
    // render-producer-billboard-compose3): compose3's transform is the node's OWN matrix at node+152,
    // and the obvious candidate for recomputing it — GraphicsBind::renderUpdateBody's
    // Math::rotmat(node+0x54 -> node+0x98), i.e. the same three Euler angles composeC5F8 uses — is
    // WRONG here. Checked on live data with the `bbrot` channel on replays/bugs/walk-dust-puff.pad:
    // for node 800F1B84 the guest holds node+152 = [0.4734 -0.2603 0.8413 | 0.5669 0.8208 -0.0654 |
    // -0.6738 0.5076 0.5359] (sinY = 0.8413) while rotmat(node+84..88) gives sinY = 1.0000 — a
    // different Y angle entirely, on every one of the 25 compose3 calls in that run. So some other
    // writer owns node+152 for this node class and it has not been identified yet. Reading node+152
    // back would be reading a matrix the engine composed, which is the banned mechanism; inventing a
    // plausible substitute is worse. The layer is left honestly missing until that writer is RE'd.
    lucent::debug("bbrot",
                  "node={:08X} variant=compose3_node152 NOT PUBLISHED (writer of node+152 "
                  "unidentified; particles not recorded) guest_node152=[{} {} {} | {} {} {} "
                  "| {} {} {}] node84={} node86={} node88={}",
                  node,
                  c->mem_r16s(node + NODE_OWN_MATRIX + 0),
                  c->mem_r16s(node + NODE_OWN_MATRIX + 2),
                  c->mem_r16s(node + NODE_OWN_MATRIX + 4),
                  c->mem_r16s(node + NODE_OWN_MATRIX + 6),
                  c->mem_r16s(node + NODE_OWN_MATRIX + 8),
                  c->mem_r16s(node + NODE_OWN_MATRIX + 10),
                  c->mem_r16s(node + NODE_OWN_MATRIX + 12),
                  c->mem_r16s(node + NODE_OWN_MATRIX + 14),
                  c->mem_r16s(node + NODE_OWN_MATRIX + 16),
                  c->mem_r16s(node + NODE_EULER + 0),
                  c->mem_r16s(node + NODE_EULER + 2),
                  c->mem_r16s(node + NODE_EULER + 4));
    // gen's live callee-saved state at the billboardEmit call site (abi_extract call [2]): r16=MAT_ROTZ,
    // r17=flag, r18=node. billboardEmit spills these as its caller regs, so match gen exactly.
    c->r[16] = MAT_ROTZ;
    c->r[17] = flag;
    c->r[18] = node;
    c->r[31] = 0x8003C8DCu;
    billboardComposeTail(c, node, flag, MAT_ROTZ); // same GTE world-translate tail as C2D4/C464, on MAT_ROTZ
    // Epilogue restore (gen_func_8003C788 L_8003C8DC): read the caller's r16/r17/r18/ra back.
    c->r[16] = c->mem_r32(sp + 16);
    c->r[17] = c->mem_r32(sp + 20);
    c->r[18] = c->mem_r32(sp + 24);
    c->r[31] = c->mem_r32(sp + 28);
  });
}

// ==================================================================================================
// FUN_8003C5F8 — billboardComposeC5F8. Fourth compose sibling. STRUCTURALLY IDENTICAL to C2D4
// (billboardCompose1): identity(MAT_A), identity(MAT_ROTZ), build a local rotation into MAT_ROTZ,
// then matMul(MAT_ROTZ, MAT_A, MAT_OUT) and the shared CAM2 world-translate tail → billboardEmit.
// The ONLY difference from C2D4: instead of a single-angle Z rotation (Math::rotZ on mem16(node+90)),
// C5F8 builds a FULL 3-Euler-angle rotation from the SVECTOR at node+84 via Math::rotMatSoft
// (FUN_800847F0, the non-GTE software RotMatrix owned this session). Frame 40, spills r16/r17/r18/
// r19/ra like C2D4. All callees owned: Mtx::identity (0x80051794), Math::rotMatSoft (0x800847F0),
// Math::matMul (0x80084110), billboardEmit (0x8003C8F4). See generated/shard_2.c gen_func_8003C5F8.
void Render::billboardComposeC5F8() {
  Core *c = mCore;
  const uint32_t node = c->r[4];
  if (c->mem_r32(node + 56) == 0) {
    return;
  }
  withObjScope(c, node, [](Core *c) {
    GuestFrame frame(c, 40);
    // Register-faithfulness (gen_func_8003C5F8 prologue): spill caller's r16..r19/ra at sp+16..+32 —
    // same 40-byte / 5-spill shape as C2D4.
    const uint32_t sp = c->r[29];
    c->mem_w32(sp + 16, c->r[16]);
    c->mem_w32(sp + 20, c->r[17]);
    c->mem_w32(sp + 24, c->r[18]);
    c->mem_w32(sp + 28, c->r[19]);
    c->mem_w32(sp + 32, c->r[31]);
    const uint32_t node = c->r[4];
    mtxOf(c).identity(MAT_A);
    mtxOf(c).identity(MAT_ROTZ);
    mathOf(c).rotMatSoft(node + 84, MAT_ROTZ); // MAT_ROTZ = software RotMatrix(SVECTOR @ node+84)
    const uint32_t flag = c->mem_r8(node + 71) & 1u;
    mathOf(c).matMul(MAT_ROTZ, MAT_A, MAT_OUT);
    // MAT_A is the identity here, so the object rotation is rotMatSoft's own Euler product.
    BbRotScope bbRot(c,
                     node,
                     "composeC5F8_euler",
                     rotEuler3(c->mem_r16s(node + NODE_EULER + 0),
                               c->mem_r16s(node + NODE_EULER + 2),
                               c->mem_r16s(node + NODE_EULER + 4)));
    // gen's live callee-saved state at the billboardEmit (func_8003C8F4) call site: r16=MAT_OUT,
    // r17=MAT_A, r18=flag, r19=node — identical to C2D4. billboardEmit spills these as its caller regs.
    c->r[16] = MAT_OUT;
    c->r[17] = MAT_A;
    c->r[18] = flag;
    c->r[19] = node;
    c->r[31] = 0x8003C76Cu;
    billboardComposeTail(c, node, flag);
    // Epilogue restore (gen_func_8003C5F8 L_8003C76C): read the caller's r16..r19/ra back from the
    // spill slots — same anti-leak discipline as C2D4.
    c->r[16] = c->mem_r32(sp + 16);
    c->r[17] = c->mem_r32(sp + 20);
    c->r[18] = c->mem_r32(sp + 24);
    c->r[19] = c->mem_r32(sp + 28);
    c->r[31] = c->mem_r32(sp + 32);
  });
}

// ==================================================================================================
// FUN_8003C8F4
void Render::billboardEmit() {
  Core *c = mCore;
  const uint32_t node = c->r[4];
  const uint32_t flag = c->r[5];
  if (c->mem_r32(node + 56) == 0) {
    return;
  }
  withObjScope(c, node, [](Core *c) {
    GuestFrame frame(c, 96); // real guest stack frame: func_8003B220 writes through this as a real
                             // guest address, and callers' own frames must already be allocated
                             // (see GuestFrame's comment) for this base to land on the same bytes
                             // the recomp path uses.
    // Register-faithfulness (gen_func_8003C8F4 prologue, L4367-4376): spill the caller's
    // r16..r22/ra at sp+64..+92. The spilled values are the caller's (billboardCompose1/2) live
    // callee-saved registers — which this port now sets correctly before the call (see above).
    const uint32_t sp = c->r[29];
    c->mem_w32(sp + 64, c->r[16]);
    c->mem_w32(sp + 68, c->r[17]);
    c->mem_w32(sp + 72, c->r[18]);
    c->mem_w32(sp + 76, c->r[19]);
    c->mem_w32(sp + 80, c->r[20]);
    c->mem_w32(sp + 84, c->r[21]);
    c->mem_w32(sp + 88, c->r[22]);
    c->mem_w32(sp + 92, c->r[31]);
    const uint32_t node = c->r[4];
    const uint32_t flag = c->r[5];
    auto FR = [c](uint32_t off) {
      return c->r[29] + off;
    };
    constexpr int32_t DEFAULT_DEPTH = -1;

    // Resolve the active particle sub-list.
    const uint32_t tbl = c->mem_r32(node + 56);
    const int idx = (int16_t)c->mem_r16(tbl + 0);
    const uint32_t listBase = c->mem_r32(node + 60);
    const uint32_t entry = listBase + (uint32_t)(idx << 2);
    const int16_t byteOff = (int16_t)c->mem_r16(entry + 2);
    int count = (int16_t)c->mem_r16(entry + 0);
    uint32_t particle = listBase + (uint32_t)(int32_t)byteOff;
    int bbIt = 0; // particle index within THIS billboardEmit call (bbord diag: same-call grouping)
    for (; count != 0; count--, particle += 16u, bbIt++) {
      // 1) Build the quad's 4 corner vectors (still-substrate; writes real guest stack memory).
      c->r[4] = FR(16);
      c->r[5] = 0;
      c->r[6] = particle;
      func_8003B220(c);

      gte_write_data(0, c->mem_r32(FR(16) + 0));
      gte_write_data(1, c->mem_r32(FR(16) + 4));
      gte_write_data(2, c->mem_r32(FR(16) + 8));
      gte_write_data(3, c->mem_r32(FR(16) + 12));
      gte_write_data(4, c->mem_r32(FR(16) + 16));
      gte_write_data(5, c->mem_r32(FR(16) + 20));
      gte_op(c, 0x4A280030u); // RTPT: project V0-2 -> SXY0-2
      int32_t ctrl31 = (int32_t)gte_read_ctrl(31);
      c->mem_w32(FR(48), (uint32_t)ctrl31);
      int32_t depth;
      if (ctrl31 < 0) {
        depth = DEFAULT_DEPTH;
      } else {
        c->mem_w32(BUF + 8, gte_read_data(12));
        c->mem_w32(BUF + 16, gte_read_data(13));
        c->mem_w32(BUF + 24, gte_read_data(14));
        gte_op(c, 0x4B400006u); // AVSZ3
        c->mem_w32(FR(48), gte_read_data(24));
        gte_write_data(0, c->mem_r32(FR(40) + 0));
        gte_write_data(1, c->mem_r32(FR(40) + 4));
        gte_op(c, 0x4A180001u); // RTPS: project V3
        ctrl31 = (int32_t)gte_read_ctrl(31);
        c->mem_w32(FR(48), (uint32_t)ctrl31);
        if (ctrl31 >= 0) {
          c->mem_w32(BUF + 32, gte_read_data(14));
          gte_op(c, 0x4B68002Eu); // AVSZ4 -> OTZ
          c->mem_w32(FR(52), gte_read_data(7));
          depth = (int32_t)c->mem_r32(FR(52));
        } else {
          depth = DEFAULT_DEPTH;
        }
      }
      c->mem_w32(FR(56), (uint32_t)depth);

      // 2) Off-screen cull: skip if all 4 corners' X>=xmax or all 4 corners' Y>=240 (unsigned compares —
      // matches the recomp's zero-extended 16-bit reads). xmax follows submit.cpp's submit_xmax
      // precedent (later-119 / USER 2026-07-16 "extend widescreen render culling area"): under the
      // genuine engine-wide FOV (OFX=nw/2, already a sanctioned wide-mode guest deviation; SBS legs
      // run 4:3 so byte-exactness is untouched) the screen extends to the wide width — the stock 320
      // gate was culling this class out of the right wide band.
      int gpu_vk_wide_engine(Core *), gpu_vk_wide_engine_w(Core *);
      const uint32_t xmax = gpu_vk_wide_engine(c) ? (uint32_t)gpu_vk_wide_engine_w(c) : 320u;
      bool onX = (uint32_t)c->mem_r16(BUF + 8) < xmax || (uint32_t)c->mem_r16(BUF + 16) < xmax ||
                 (uint32_t)c->mem_r16(BUF + 24) < xmax || (uint32_t)c->mem_r16(BUF + 32) < xmax;
      if (!onX) {
        continue;
      }
      bool onY = (uint32_t)c->mem_r16(BUF + 10) < 240u || (uint32_t)c->mem_r16(BUF + 18) < 240u ||
                 (uint32_t)c->mem_r16(BUF + 26) < 240u || (uint32_t)c->mem_r16(BUF + 34) < 240u;
      if (!onY) {
        continue;
      }

      // 3) Quantize into an OT bucket: node's signed per-node depth-bias byte, >>10/<<9 rebucket into
      // the valid range, else reset to invalid (-1).
      {
        int32_t a = (int32_t)c->mem_r32(FR(56)) + (int8_t)c->mem_r8(node + 8);
        int32_t shiftAmt = a >> 10;
        int32_t bucketed = a >> (shiftAmt & 31);
        int32_t d = bucketed + (shiftAmt << 9);
        c->mem_w32(FR(56), (uint32_t)d);
        if ((uint32_t)(d - 4) >= 2044u) {
          c->mem_w32(FR(56), (uint32_t)DEFAULT_DEPTH);
        }
      }
      if ((int32_t)c->mem_r32(FR(56)) < 0) {
        continue;
      }

      // 4) Fill color/UV (still-substrate), then optional overrides.
      c->r[4] = BUF;
      c->r[5] = particle;
      c->r[6] = flag;
      func_8003B054(c);
      if (c->mem_r16(node + 92) != 0) {
        c->mem_w16(BUF + 14, c->mem_r16(node + 92));
      }

      const uint32_t caseSel = c->mem_r8(node + 13);
      if (caseSel < 33u) {
        constexpr uint32_t CASE_TABLE = 0x80014E40u;
        const uint32_t caseTarget = c->mem_r32(CASE_TABLE + caseSel * 4u);
        switch (caseTarget) {
        case 0x8003CB60u:
          c->mem_w8(BUF + 7, 45);
          break;
        case 0x8003CB6Cu:
          c->mem_w8(BUF + 7, 47);
          break;
        case 0x8003CB78u:
          c->mem_w32(BUF + 4, c->mem_r32(node + 24));
          c->mem_w8(BUF + 7, 44);
          break;
        case 0x8003CB90u:
          c->mem_w32(BUF + 4, c->mem_r32(node + 24));
          c->mem_w8(BUF + 7, 46);
          break;
        case 0x8003CBA8u:
          c->mem_w8(BUF + 7, 45);
          c->mem_w16(BUF + 14, c->mem_r8(node + 24) != 0 ? 16507u : 16443u);
          break;
        case 0x8003CBC8u:
          break; // explicit no-op case: falls through to packet emission
        default:
          // Defensive mirror of the recomp's raw `jr` fallback for an unrecognized table entry: the
          // recomp body does `rec_dispatch(c, caseTarget); return` here — a FULL early return, NOT a
          // fallthrough to packet emission. Never hit by live game data (this 33-entry table's slots
          // all resolve to one of the 5 cases above or the CBC8 no-op).
          rec_dispatch(c, caseTarget);
          return;
        }
      }

      // 5) Emit the 10-word packet (tag + 9 data words) at the pool tail, prepended into the OT bucket.
      const uint32_t packetLo = c->mem_r32(PKT_POOL_PTR);
      uint32_t tail = packetLo;
      uint32_t otbase = c->mem_r32(OTBASE_PTR);
      uint32_t otslot = otbase + (c->mem_r32(FR(56)) << 2);
      uint32_t oldHead = c->mem_r32(otslot);
      c->mem_w32(tail, oldHead | 0x09000000u);
      c->mem_w32(otslot, tail);
      tail += 4;
      for (uint32_t off = 4; off <= 36; off += 4) {
        c->mem_w32(tail, c->mem_r32(BUF + off));
        tail += 4;
      }
      c->mem_w32(PKT_POOL_PTR, tail);

      // 6) RECORD this particle for the display-pass producer Render::billboardsRender. Everything
      // here is the effect's OWN state: the local quad corners func_8003B220 just built for this
      // particle, the node's own object rotation (published by whichever compose variant is running —
      // BbObjectRot above, rebuilt from the node's euler/scale fields), the node's own world
      // position, and the RESOLVED material words (post node+92 override + node+13 case patches).
      // NO GTE register is read and the camera does not appear, so billboardsRender applying the
      // native camera cannot produce a camera-dependent residue: this is the rebuild that replaced
      // the CR0-4/CR5-7 tap. Host memory only, and skipped on the SBS oracle core (core B must stay
      // the untouched reference).
      if (!c->game->oracle && rend(c)->mBbRotValid) {
        Render::BbRec rb;
        rb.node = node;
        rb.particle = particle;
        const uint32_t vbase = FR(16);
        rb.cx[0] = c->mem_r16s(vbase + 0);
        rb.cy[0] = c->mem_r16s(vbase + 2);
        rb.cx[1] = c->mem_r16s(vbase + 8);
        rb.cy[1] = c->mem_r16s(vbase + 10);
        rb.cx[2] = c->mem_r16s(vbase + 16);
        rb.cy[2] = c->mem_r16s(vbase + 18);
        rb.cx[3] = c->mem_r16s(vbase + 24);
        rb.cy[3] = c->mem_r16s(vbase + 26);
        for (int i = 0; i < 3; i++) {
          for (int j = 0; j < 3; j++) {
            rb.rotR[i][j] = rend(c)->mBbRot[i][j];
          }
        }
        rb.wx = (float)c->mem_r16s(node + NODE_WORLD_X);
        rb.wy = (float)c->mem_r16s(node + NODE_WORLD_Y);
        rb.wz = (float)c->mem_r16s(node + NODE_WORLD_Z);
        rb.wColor = c->mem_r32(BUF + 4);
        rb.wUv0 = c->mem_r32(BUF + 12);
        rb.wUv1 = c->mem_r32(BUF + 20);
        rb.wUv2 = c->mem_r32(BUF + 28);
        rb.wUv3 = c->mem_r32(BUF + 36);
        rend(c)->mBbRecs.push_back(rb);
      }
    }
  });
}

// ==================================================================================================
namespace {
// Engine/game natives installed into the process-global g_override[] table. These are NOT gated
// here — the gate lives in ONE place (the override registry, runtime/recomp/override_registry.h)
// so it can't be forgotten cluster-by-cluster. engine_set_override_main() installs into that
// registry, which runs the real gen_func_* body on the oracle (psx_fallback) and the native
// everywhere else.
void ov_perObjRenderDispatch(Core *c) {
  rend(c)->perObjRenderDispatch();
}
void ov_billboardCompose1(Core *c) {
  rend(c)->billboardCompose1();
}
void ov_billboardCompose2(Core *c) {
  rend(c)->billboardCompose2();
}
void ov_billboardCompose3(Core *c) {
  rend(c)->billboardCompose3();
}
void ov_billboardComposeC5F8(Core *c) {
  rend(c)->billboardComposeC5F8();
}
void ov_billboardEmit(Core *c) {
  rend(c)->billboardEmit();
}
} // namespace

// ==================================================================================================
// billboardsRender — the DISPLAY-PASS billboard producer. Projects every BbRec billboardEmit captured
// this logic frame through the SAME float camera path the rest of the world uses (Fps60::sceneCam —
// the choke the fps60 interp present serves a lerp(prev,cur) camera through):
//   object rotation = the node's own R (BbObjectRot, rebuilt from its euler/scale fields)
//   view transform  = Rcam·anchor/4096 + Tcam, anchor = the node's own world position
//   sx = OFX + vx·H/pz, pz = max(H/2, vz)  — the same projection camWorldScreen/terrain use.
// THE CAMERA APPEARS EXACTLY ONCE, HERE, and it is the native camera. The record carries no camera
// term at all, so the camera-dependent quantisation residue the deleted CR0-4/CR5-7 tap produced is
// structurally impossible rather than merely small.
// Emits RQ_WORLD float quads (has_xyf=1 → tier1-owned, rebuilt on the interp present) with real
// per-particle identity (dbg_node = node, records in guest emit order).
// Read-only over guest memory (reads nothing but the camera via sceneCam); host writes only.
// emitRecQuad — shared record-quad emit for billboardsRender's two record kinds. Decodes the FT4
// record's material words RAW (mode/tp_x/tp_y/blend from the tpage half, clut x/y from the clut
// half; neutral texture-window, full draw-area) — NOT from GpuState's live s_tp_*/s_da_* fields,
// which hold unrelated stale state at display time. Float verts + real per-vertex depth + the
// drawWorldQuad draw-offset convention; dbg_node = the owning node (real identity).
void Render::emitRecordQuad(Core *c,
                            uint32_t node,
                            const uint32_t wCol[4],
                            uint32_t wUv0,
                            uint32_t wUv1,
                            uint32_t wUv2,
                            uint32_t wUv3,
                            const float *px,
                            const float *py,
                            const float *dep) {
  const uint8_t op = (uint8_t)(wCol[0] >> 24);
  const uint32_t clut = wUv0 >> 16;
  const uint32_t tp = wUv1 >> 16;
  int us[4] = {(int)(wUv0 & 0xFFu), (int)(wUv1 & 0xFFu), (int)(wUv2 & 0xFFu), (int)(wUv3 & 0xFFu)};
  int vs[4] = {
      (int)((wUv0 >> 8) & 0xFFu), (int)((wUv1 >> 8) & 0xFFu), (int)((wUv2 >> 8) & 0xFFu), (int)((wUv3 >> 8) & 0xFFu)};
  unsigned char rs[4], gsv[4], bs[4];
  for (int i = 0; i < 4; i++) {
    rs[i] = (unsigned char)(wCol[i] & 0xFF);
    gsv[i] = (unsigned char)((wCol[i] >> 8) & 0xFF);
    bs[i] = (unsigned char)((wCol[i] >> 16) & 0xFF);
  }
  GpuState &gs = c->game->gpu;
  gs.s_seen3d = 1;
  int xs[4], ys[4];
  float xsf[4], ysf[4];
  for (int i = 0; i < 4; i++) {
    xsf[i] = px[i] + (float)gs.s_off_x;
    ysf[i] = py[i] + (float)gs.s_off_y;
    xs[i] = (int)(px[i] < 0 ? px[i] - 0.5f : px[i] + 0.5f) + gs.s_off_x;
    ys[i] = (int)(py[i] < 0 ? py[i] - 0.5f : py[i] + 0.5f) + gs.s_off_y;
  }
  c->rsub.diag.beginObject(node);
  c->game->activeRq().emitOrQueue(c,
                                  1,
                                  RQ_WORLD,
                                  RQ_OM_DEPTH,
                                  4,
                                  (op & 2) ? 1 : 0,
                                  (op & 1) ? 1 : 0,
                                  xs,
                                  ys,
                                  xsf,
                                  ysf,
                                  us,
                                  vs,
                                  rs,
                                  gsv,
                                  bs,
                                  dep,
                                  (int)((tp >> 7) & 3u),
                                  (int)(tp & 0xFu) * 64,
                                  (int)((tp >> 4) & 1u) * 256,
                                  (int)(clut & 0x3Fu) * 16,
                                  (int)((clut >> 6) & 0x1FFu),
                                  0,
                                  0,
                                  0,
                                  0,
                                  0,
                                  0,
                                  1023,
                                  511,
                                  (int)((tp >> 5) & 3u));
  c->rsub.diag.endObject();
}

void Render::billboardsRender() {
  Core *c = mCore;
  if (mBbRecs.empty()) {
    return;
  }

  float Ri[3][3], T[3], ofx, ofy, H;
  c->game->fps60.sceneCam(c, Ri, T, ofx, ofy, H); // raw int16-unit rows, the sceneCam convention
  if (H <= 0.0f) {
    return;
  }
  constexpr float FX = 1.0f / 4096.0f;
  float R[3][3];
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 3; j++) {
      R[i][j] = Ri[i][j] * FX;
    }
  }

  // BbRec PARTICLES do NOT lerp — the effect sub-lists reuse/walk particle addresses every frame, so
  // no stable cross-frame identity exists; a particle-addr-keyed lerp blended DIFFERENT sprites'
  // positions (USER: "gems rendered at two different places between real and interpolated frames").
  // A particle draws at its own frame's state under the LERPED camera: world-glued, no ghosting;
  // its animation steps at the logic rate, which is what the state actually says.

  for (const BbRec &rc : mBbRecs) {
    // This frame's own state (no per-particle lerp — see banner above); camera lerp comes via R/T.
    const float wx = rc.wx, wy = rc.wy, wz = rc.wz;
    float cxf[4], cyf[4];
    for (int i = 0; i < 4; i++) {
      cxf[i] = rc.cx[i];
      cyf[i] = rc.cy[i];
    }
    // rc.rotR is already unit-scale float (rebuilt from the node's fields), so it needs no 1/4096.
    const float (&rot)[3][3] = rc.rotR;

    // t = Rcam·anchor + Tcam (the MVMVA CAM2 compose, in float).
    const float ax = R[0][0] * wx + R[0][1] * wy + R[0][2] * wz + T[0];
    const float ay = R[1][0] * wx + R[1][1] * wy + R[1][2] * wz + T[1];
    const float az = R[2][0] * wx + R[2][1] * wy + R[2][2] * wz + T[2];

    float px[4], py[4], dep[4];
    bool behind = false;
    for (int i = 0; i < 4; i++) {
      const float vx = rot[0][0] * cxf[i] + rot[0][1] * cyf[i] + ax; // corner z==0 in local space
      const float vy = rot[1][0] * cxf[i] + rot[1][1] * cyf[i] + ay;
      const float vz = rot[2][0] * cxf[i] + rot[2][1] * cyf[i] + az;
      if (vz <= 0.0f) {
        behind = true;
        break;
      }
      float pz = H * 0.5f;
      if (vz > pz) {
        pz = vz; // near-plane clamp (world convention)
      }
      const float ph = H / pz;
      px[i] = ofx + vx * ph;
      py[i] = ofy + vy * ph;
      dep[i] = c->rsub.projParams.pzToOrd(vz);
    }
    if (behind) {
      continue;
    }

    {
      const uint32_t wc[4] = {rc.wColor, rc.wColor, rc.wColor, rc.wColor};
      emitRecordQuad(c, rc.node, wc, rc.wUv0, rc.wUv1, rc.wUv2, rc.wUv3, px, py, dep);
    }
  }
}

void perobj_billboard_install() {
  static bool done = false;
  if (done) {
    return;
  }
  done = true;
  // engine_set_override_main (runtime/recomp/override_registry.h) installs into the ONE
  // process-global override registry, which runs gen_func_* on the oracle leg (core B) and the
  // native handler everywhere else — NOT a raw shard_set_override, since these are engine/game
  // natives and the oracle must run the pure recompiled body for them.
  extern void engine_set_override_main(uint32_t, OverrideFn, OverrideFn);
  engine_set_override_main(0x8003CCA4u, ov_perObjRenderDispatch, gen_func_8003CCA4);
  engine_set_override_main(0x8003C2D4u, ov_billboardCompose1, gen_func_8003C2D4);
  engine_set_override_main(0x8003C464u, ov_billboardCompose2, gen_func_8003C464);
  engine_set_override_main(0x8003C788u, ov_billboardCompose3, gen_func_8003C788);
  engine_set_override_main(0x8003C5F8u, ov_billboardComposeC5F8, gen_func_8003C5F8);
  engine_set_override_main(0x8003C8F4u, ov_billboardEmit, gen_func_8003C8F4);
}
