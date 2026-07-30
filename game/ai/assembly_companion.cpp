// game/ai/assembly_companion.cpp — AssemblyCompanion::endCamHoldAndRearmOnStroke, guest FUN_80138A64.
//
// WHAT IT IS, IN GAME TERMS
// A field assembly — the seaside water-pump/seesaw class, a swinging beam with an arm, a bucket and a
// counterweight — does not just animate itself. When it is built it also SPAWNS A SECOND, JOINTED
// ACTOR at its own position and keeps it as a child, and that companion performs a short routine each
// time the beam completes the first half of its stroke. The routine is meant to be watched: while it
// runs, the companion takes the field camera's vertical-look mode away from the ordinary follow
// height and pulls the look target 200 units DOWN, then hands it back afterwards.
//
// THIS function is the companion's IDLE tick — the one that runs on every frame it is NOT performing
// — and it is the whole arming-and-releasing half of that arrangement. It does exactly two things:
//
//   1. IT ENDS THE PREVIOUS PERFORMANCE'S CAMERA HOLD. When the performance (guest 0x80137198)
//      finishes it drops the companion back to this sub-state, leaves a 30-frame cooldown in the
//      node's +0x40 timer, and leaves its sequence marker at +0x6A still set — i.e. "the camera is
//      still mine". This leaf counts that timer down one per frame and, on the frame it passes below
//      zero, writes the camera mode back to 0 and clears the marker. So the lowered camera outlives
//      the routine by about half a second rather than snapping back with it.
//   2. IT RE-ARMS ON THE PARENT'S NEXT STROKE. Once nothing is held, it watches the parent assembly's
//      stroke-phase field: when the beam reports phase 1 (first half-stroke reached) with its flag
//      word clear, the companion claims the lowered camera mode again, zeroes its step counter and
//      advances its sub-state — which is what starts the next performance.
//
// THE CLAIM IS A COPY, NOT A CONSTANT — this is the detail that explains the whole design. The guest
// does not store a literal 1 into the camera byte; it stores the halfword it just loaded from the
// PARENT's stroke-phase field. Assembly phase and camera mode are one shared 0/1/2 vocabulary, and
// the companion is the wire between them. (The preceding branch has already proved that value is 1,
// so the store is byte-identical either way — but written as a copy it says what the engine means.)
//
// The two halves are sequenced, not independent: the release re-reads the marker so that a claim
// expiring THIS frame can be re-armed on the SAME frame, and the guard is one shared "is anything
// held" test. A role-0 companion is the quiet variant — it runs the same state machine but neither of
// its camera writes fires (see AssemblyCompanion::role), so the camera never moves for it.
//
// HOW IT WAS IDENTIFIED — FROM THE CALLERS AND THE SPAWN CHAIN, NOT FROM THE NEIGHBOURHOOD
// Being a 0x8013xxxx address in the A00 overlay says only which overlay it lives in. Every statement
// above comes from a link that can be re-walked:
//   * THE CALLER. Its single caller in A00 is ov_a00_gen_80136D9C, already ported as
//     beh_pure_inner_dispatch (game/ai/beh_pure_inner_dispatch.cpp:65). It reaches this address from
//     exactly one place: outer state 1, inner sub-state node[5] == 0, node[3] != 3. That is what makes
//     this "the idle tick", and why role 3 is never seen here — role 3 goes to 0x8018CDC4 instead.
//   * THE SPAWN CHAIN, which is what names the node itself. The assembly FUN_8012EB54
//     (game/ai/beh_substate_edge_orchestrator.cpp; the class is identified as the seaside pumps in
//     game/ai/assembly_node.h) calls SubstateEdgeLeaves::spawnInnerDispatchChild — guest 0x8013892C,
//     game/ai/substate_edge_native.cpp, whose own banner already calls the result "THE ASSEMBLY'S
//     COMPANION NODE" — from its state-0 init leaf ov_a00_gen_8012ED84 (generated/ov_a00_shard_1.c:
//     19119, inside the body that starts at :18777) and again from the OPN-overlay assembly hook
//     0x8018C820 (generated/ov_opn_shard_1.c:1083). The spawner allocates through
//     Placement::spawnWithParent (guest FUN_80072DDC, game/world/placement.cpp), whose body contains
//     the decisive store `mem_w32(newNode + 16, a0)` — the new node's +0x10 IS the spawner. It then
//     writes 0x80136D9C into the new node's +0x1C handler slot and copies the assembly's position.
//     So node+0x10 read here is the parent assembly, as a fact.
//   * IT IS CONFIRMED ON LIVE STATE, not only statically. Scanning the committed 2 MB RAM dumps for
//     nodes whose +0x1C handler is 0x80136D9C finds exactly THREE, at 0x800FCCF8 / 0x800FCE00 /
//     0x800FCF08, identical across all 23 dumps. Each has node[2]==10 and node[0x28]==3 — matching
//     the spawner's own (a3=10, a1=3) arguments — roles 0/1/2, and a +0x10 pointing at
//     0x800FB858 / 0x800FB960 / 0x800FBA68, EVERY ONE of which has 0x8012EB54 as its own handler.
//     Their world x ≈ 5899 / 7003 / 7988 sit in the pump x-bands independently derived in
//     docs/kanban/cards/008-water-pump-seesaw-tomba-s-weight-doesn-t-pull-it.md.
//   * WHAT +0x76 AND +0x78 ARE ON THAT PARENT — established from the assembly's OWN writers, not from
//     this reader. 0x8018C820 sets +0x76 to 1 as it starts the stroke (beside +0x48=256, +0x4E=256)
//     and to 2 at the second phase (+0x48=-2560, +0x4E=512); 0x8012F5B4 promotes 1 -> 2 at the far
//     limit and clears it on every path that parks the beam; the init clears it right before spawning
//     this companion. +0x78 is a bitfield on the same pair (bit1, bit2 and bit15 are tested; bit2 is
//     cleared with an AND 0xFFFB), so `== 0` here reads as "the assembly has no stroke flags
//     pending". Written up on AssemblyNode::strokePhase/strokeFlags.
//   * WHAT 0x800BF821 IS. CutsceneCamera::pitch (guest FUN_8006D654) is its consumer and switches its
//     height target on it — see the constant's comment in assembly_companion.h. The sibling leaf
//     0x80137198 is the only other A00 writer, and its two writes corroborate the reading directly:
//     it releases the mode when the companion's visibility byte node[1] is 0 (culled ⇒ give the
//     camera back) and again on the path that returns to this sub-state.
//
// TRUE EXTENT: [0x80138A64, 0x80138B04), 0xA0 bytes / 40 instructions. Established three ways, and
// deliberately NOT by "the next gen function in the shard" — that test is false here, because the
// recompiler's shard split is not address order: the body sits in ov_a00_shard_1.c but the next
// function BY ADDRESS, 0x80138B04, is emitted in ov_a00_shard_0.c:20730, while the next function in
// shard_1 is the much later 0x80138C70. So:
//   1. port_gen's live-extent splitter puts the body at generated/ov_a00_shard_1.c:23677-23709 and
//      trims one folded-sibling tail line after the real `return;`.
//   2. Every label the body branches to lies in [0x80138A64, 0x80138AFC], the last being the return.
//   3. Disassembly of the overlay image confirms it instruction-for-instruction (tools/disasm_overlay.py
//      scratch/bin/overlays/A00.BIN --base=0x80108F9C): `jr ra` at 0x80138AFC with its delay-slot nop
//      at 0x80138B00, and 0x80138B04 opening a fresh prologue (`addiu sp, sp, -0x20` /
//      `sw s0, 0x10(sp)`) — so the extent cannot run on into it. 0x80138B04 is also its own entry in
//      the overlay dispatch table (generated/ov_a00_disp.c:421).
// frame_size = 0: the guest body never touches sp, so there is no guest stack frame to mirror
// (tools/abi_extract.py 80138A64 --contract). abi_extract's 2 "unreachable blocks" are the
// recompiler's duplicated `return;` tail plus that folded-sibling line, not a folded-in sibling body.
//
// SHAPE OF THE PORT: a rebuild over the typed lens, in the panel_fill.cpp house style — argument
// register in, named fields out, no scratch-register maintenance. The static store sequence is
// unchanged from the guest (w16 +0x40, w8 camera, w16 +0x6A, w8 camera, w8 +0x06, w8 +0x05), which is
// what tools/port_check.py gates.
#include "assembly_companion.h"
#include "assembly_node.h"
#include "core.h"
#include "override_registry.h"
#include "ov_a00_decls.h"

namespace {
// The one stroke phase the companion arms on: the assembly's "first half-stroke reached" code.
constexpr int32_t kStrokePhaseFirstHalfReached = 1;
}  // namespace

// ORACLE: ov_a00_gen_80138A64
void AssemblyCompanion::endCamHoldAndRearmOnStroke(Core* c) {
  const AssemblyCompanion companion(c, c->r[4]);

  // (1) Expire the hold the finished performance left behind. Role 0 never holds the camera, so it
  // never runs the countdown either — its marker, if set, simply keeps it parked.
  int32_t held = companion.seqMarker();
  if (held != 0 && companion.role() != 0) {
    const uint16_t remaining = (uint16_t)(companion.camHoldTimer() - 1u);
    companion.setCamHoldTimer(remaining);
    if ((int16_t)remaining < 0) {                    // the guest tests the sign of the low 16 bits
      c->mem_w8(kCamPitchMode, kCamPitchNormal);     // give the camera back
      companion.setSeqMarker(0);
    }
    held = companion.seqMarker();                    // re-read: a claim expiring now may re-arm now
  }

  const AssemblyNode assembly(c, companion.parent());
  if (held != 0) return;                             // still holding — stay idle
  const int32_t phase = assembly.strokePhase();
  if (phase != kStrokePhaseFirstHalfReached) return; // wait for the beam's first half-stroke
  // (2) Re-arm. The claim is the assembly's own phase code copied into the camera mode; it is skipped
  // for role 0 and while the assembly raises any stroke flag, but the sub-state advances either way —
  // the performance starts, it just may start uncinematically.
  if (companion.role() != 0 && assembly.strokeFlags() == 0)
    c->mem_w8(kCamPitchMode, (uint8_t)phase);
  const uint8_t nextSubState = (uint8_t)(companion.subState() + 1u);
  companion.setStep(0);
  companion.setSubState(nextSubState);
}

void AssemblyCompanion::registerOverrides() {
  overrides::install(0x80138A64u, "AssemblyCompanion::endCamHoldAndRearmOnStroke",
                     &AssemblyCompanion::endCamHoldAndRearmOnStroke, ov_a00_gen_80138A64,
                     ov_a00_set_override);
}
