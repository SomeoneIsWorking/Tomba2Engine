// class SceneTransition — see scene_transition.h. Implementations only.
//
// Instance methods on the per-Core SceneTransition subsystem (embedded via Engine).
// `core` is the wired back-pointer to the owning Core; all guest-memory access goes through it.
//
// `areaMaskTrigger` is RE'd 1:1 from disas 0x800782F0. The sub-scene swap methods
// (resetSwap / beginSwap / completeSwap / stepSwapWaiter) are RE'd from 0x80073260 / 2C0 / 300 / 328.
// A handful of substrate leaves stay rec_dispatched (FUN_80074590 SFX, FUN_80054198 scene-block
// reset, FUN_80072E60/EFC/F14 case-1/5 helpers) — top-down ownership will absorb them next.
//
// Verify gates:
//   * `debug scene_transitionverify` — A/Bs areaMaskTrigger vs rec_super_call(0x800782F0).
//   * `debug subswapverify`          — A/Bs stepSwapWaiter vs rec_super_call(0x80073328).
#include "scene_transition.h"
#include "game_ctx.h"
#include "game.h"      // c->game->verify — the shared A/B verify scaffold
#include "core.h"
#include "cfg.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void rec_super_call(Core*, uint32_t);
void rec_dispatch(Core*, uint32_t);

// ─────────────────────────────────────────────────────────────────────────────
// FUN_800782F0 — area-mask "seen" register + area-mode bit
// ─────────────────────────────────────────────────────────────────────────────

void SceneTransition::areaMaskTrigger(uint8_t area, uint8_t sub) {
  Core* c = core;

  auto impl = [&](uint8_t a, uint8_t s) {
    if (a < 9) {
      uint32_t tbl_entry = c->mem_r32(0x800A54A8u + (uint32_t)a * 4u);
      uint16_t h         = c->mem_r16(tbl_entry + (uint32_t)s * 8u + 6u);
      uint8_t  base      = c->mem_r8 (0x800A55B0u + a);
      uint32_t shift     = ((uint32_t)base + (((uint32_t)h & 0x0600u) >> 9)) & 0x1Fu;
      uint32_t reg       = c->mem_r32(0x800BFE50u);
      c->mem_w32(0x800BFE50u, reg | (1u << shift));
    }
    uint8_t cur = c->mem_r8(0x800BF870u);
    if (cur == 5) { c->mem_w8(0x800BF9DBu, (uint8_t)(c->mem_r8(0x800BF9DBu) | 0x02)); return; }
    if (cur == 6) { c->mem_w8(0x800BF9DBu, (uint8_t)(c->mem_r8(0x800BF9DBu) | 0x04)); return; }
    if (cur == 7) { c->mem_w8(0x800BF9DBu, (uint8_t)(c->mem_r8(0x800BF9DBu) | 0x08)); return; }
    if (cur == 8) { c->mem_w8(0x800BF9DBu, (uint8_t)(c->mem_r8(0x800BF9DBu) | 0x10));         }
  };

  int s_v = c->game->verify.on("scene_transitionverify");
  if (!s_v) { impl(area, sub); return; }

  uint8_t* ram0 = c->game->verify.ram0();
  uint8_t* ramN = c->game->verify.ramN();
  uint8_t spad0[0x400], spadN[0x400];
  uint32_t regs0[32]; memcpy(regs0, c->r, sizeof regs0);
  memcpy(ram0, c->ram, 0x200000); memcpy(spad0, c->scratch, 0x400);

  impl(area, sub);

  memcpy(ramN, c->ram, 0x200000); memcpy(spadN, c->scratch, 0x400);
  memcpy(c->ram, ram0, 0x200000); memcpy(c->scratch, spad0, 0x400); memcpy(c->r, regs0, sizeof regs0);
  c->r[4] = area; c->r[5] = sub;
  rec_super_call(c, 0x800782F0u);

  uint32_t sp = regs0[29] & 0x1FFFFFu, flo = (sp >= 0x800) ? sp - 0x800 : 0;
  int ro = -1;
  for (uint32_t a = 0; a < 0x200000; a++) if (c->ram[a] != ramN[a] && !(a >= flo && a < sp)) { ro = (int)a; break; }
  int so = -1;
  for (uint32_t a = 0; a < 0x400; a++) if (c->scratch[a] != spadN[a]) { so = (int)a; break; }
  VerifyHarness::Check& chk = c->game->verify.check("scene_transitionverify");
  long &ng = chk.nMatch, &nb = chk.nMismatch;
  if (ro >= 0 || so >= 0) {
    if (nb++ < 40) cfg_logi("scene_transitionverify", "MISMATCH area=%u sub=%u ram@%x spad@%x", area, sub, ro, so);
  } else if (++ng % 50 == 0) cfg_logi("scene_transitionverify", "%ld matches", ng);
}

// ─────────────────────────────────────────────────────────────────────────────
// Sub-scene swap: FUN_80073260/2C0/300/328
// ─────────────────────────────────────────────────────────────────────────────

void SceneTransition::resetSwap(uint32_t node) {
  Core* c = core;
  c->mem_w8(node + 0, 2);
  if (c->mem_r8(node + 0xBF) != 0) {
    eng(c).sfx.trigger(0x17, 0, 0x0F);           // FUN_80074590 (native)
  }
  c->mem_w8(E7FC5_FLAG, 0);
  clearSwapBlock(SCENE_BLOCK);          // FUN_80054198 (native)
  c->mem_w8(SCENE_BLOCK + 5, 0);
  c->mem_w8(SCENE_BLOCK + 6, 0);
  c->mem_w8(SCENE_BLOCK + 7, 0);
}

// FUN_80054198 — small swap-block ephemeral clear. RE'd from disas 0x80054198..0x800541F0.
// Note the (v0=2 into 329(a0)) short branch vs the (v = node[+0x147]+2, into 329/330(a0)) long
// branch — the DELAY-SLOT constant reload (`addiu v0, zero, 2` after `bne v1, v0, ...`) is what
// lets both paths share the eventual `sb v0, 329(a0)`. Ports verbatim.
void SceneTransition::clearSwapBlock(uint32_t node) {
  Core* c = core;
  if (c->mem_r8(node + 0x146) == 4 && c->mem_r8(node + 0) == 2) return;
  c->mem_w16(node + 0x44, 0);
  c->mem_w16(node + 0x182, 0);
  if (c->mem_r8(node + 2) != 0) {
    c->mem_w8(node + 0x149, 2);
    return;
  }
  c->mem_w16(node + 0x50, 0);
  c->mem_w8(node + 0x148, 0);
  uint8_t v = (uint8_t)(c->mem_r8(node + 0x147) + 2);
  c->mem_w8(node + 0x14A, v);
  c->mem_w8(node + 0x149, v);
}

void SceneTransition::beginSwap(uint32_t node) {
  Core* c = core;
  resetSwap(node);
  c->mem_w8(BF818_SWAP_PHASE, 1);
  c->mem_w8(BF817_SWAP_KEY,   c->mem_r8(node + 3));
}

void SceneTransition::completeSwap(uint32_t node) {
  // >>> THE 2→3 HANDSHAKE. This is the ONLY writer of bf818=3 in the whole game.
  //     If bf818 stays 2 forever, the door freezes — see docs/findings/scene.md.
  Core* c = core;
  resetSwap(node);
  c->mem_w8(BF818_SWAP_PHASE, 3);
}

int SceneTransition::stepSwapWaiter(uint32_t node) {
  Core* c = core;

  auto impl = [&](uint32_t n) -> int {
    const uint8_t case_id = c->mem_r8(n + 6);
    if (case_id >= 6) return 0;

    auto second_branch_gate = [&]() -> bool {
      if (c->mem_r8(n + 0x29) == 0) return false;
      if (c->mem_r8(E7EA9_ARMED) == 0) return false;
      if (c->mem_r8(E7FFB_INHIBIT) != 0) return false;
      return true;
    };
    auto pause_latch_tail = [&]() {
      if (c->mem_r8(IF800137_PAUSE) == 0) c->mem_w8(IF800137_PAUSE, 2);
    };

    switch (case_id) {
      case 0: {
        if (second_branch_gate()) {
          c->mem_w8(n + 6, (uint8_t)(case_id + 1));
          beginSwap(n);
          pause_latch_tail();
          return 0;
        }
        if (c->mem_r8(BF818_SWAP_PHASE) != 5) return 0;
        if (c->mem_r8(BF817_SWAP_KEY)  != c->mem_r8(n + 3)) return 0;
        c->mem_w8(n + 6, (uint8_t)(case_id + 1));
        beginSwap(n);
        return 0;
      }
      case 1: {
        // FUN_80072E60 (inlined; disas 0x80072E60..0x80072EF8): ramp obj[+0x50] toward ±1024 by 64
        // per tick, driven by direction byte obj[+0x46] (0 = decrement, 1 = increment, else no-op).
        // Returns 1 if the clamp was reached this tick (a1 = 1 in the recomp), 0 otherwise. Tail
        // always sets obj[+0x56] = obj[+0x5A] + obj[+0x50] (the ADD form; FUN_80072EFC is the SUB
        // form for the completion path below).
        uint32_t clampHit = 0;
        uint8_t dir = c->mem_r8(n + 0x46);
        if (dir == 0) {
          int16_t v = (int16_t)(c->mem_r16(n + 0x50) - 64);
          c->mem_w16(n + 0x50, (uint16_t)v);
          if (v < -1024) { c->mem_w16(n + 0x50, (uint16_t)-1024); clampHit = 1; }
        } else if (dir == 1) {
          int16_t v = (int16_t)(c->mem_r16(n + 0x50) + 64);
          c->mem_w16(n + 0x50, (uint16_t)v);
          if (v >= 1025) { c->mem_w16(n + 0x50, (uint16_t)1024); clampHit = 1; }
        }
        // TAIL (always): obj[+0x56] = obj[+0x5A] + obj[+0x50] (ADD, not the sub of FUN_80072EFC)
        c->mem_w16(n + 0x56, (uint16_t)(c->mem_r16(n + 0x5A) + c->mem_r16(n + 0x50)));

        if (clampHit != 0 && c->mem_r8(0x800BF816u) != 0) {
          c->mem_w8(n + 6, (uint8_t)(case_id + 1));
          // FUN_80072EFC: n[+0x56] = n[+0x5A] - n[+0x50]  (SUB form, disas 0x80072EFC..0x80072F10)
          c->mem_w16(n + 0x56, (uint16_t)(c->mem_r16(n + 0x5A) - c->mem_r16(n + 0x50)));
        }
        return 0;
      }
      case 2: {
        if (c->mem_r8(BF818_SWAP_PHASE) != 2) return 0;
        if (c->mem_r8(BF80F_SUSPEND)    != 0) return 0;
        c->mem_w8 (n + 0, 1);
        c->mem_w8 (n + 6, (uint8_t)(case_id + 1));
        c->mem_w8 (n + 0x5F, (uint8_t)(1u - (uint32_t)c->mem_r8(n + 0x5F)));
        c->mem_w16(n + 0x84, (uint16_t)(c->mem_r16(n + 0x84) + 800));
        c->mem_w16(n + 0x86, (uint16_t)(c->mem_r16(n + 0x86) + 800));
        return 0;
      }
      case 3: {
        // >>> THE 2→3 HANDSHAKE point. Second-branch gate + not-suspended → completeSwap fires.
        //     Under the door_freeze.pad repro this never fires (docs/findings/scene.md).
        if (second_branch_gate()) {
          if (c->mem_r8(BF80F_SUSPEND) != 0) return 0;
          c->mem_w8(n + 6, (uint8_t)(case_id + 1));
          completeSwap(n);        // bf818 := 3   >>> release the waiter
          pause_latch_tail();
          return 0;
        }
        if (c->mem_r8(BF818_SWAP_PHASE) != 6) return 0;
        if (c->mem_r8(BF80F_SUSPEND) == 0) {
          c->mem_w8(n + 6, (uint8_t)(case_id + 1));
          completeSwap(n);
        }
        return 0;
      }
      case 4: {
        if (c->mem_r8(BF818_SWAP_PHASE) != 4) return 0;
        if (c->mem_r8(BF80F_SUSPEND)    != 0) return 0;
        c->mem_w8(n + 6, (uint8_t)(case_id + 1));
        c->mem_w8(BF818_SWAP_PHASE, 0);
        return 0;
      }
      case 5: {
        // FUN_80072F14 (inlined; disas 0x80072F14..0x80072FDC): REVERSE ramp — sibling of
        // FUN_80072E60. Ramps obj[+0x50] TOWARD zero (dir==0 → was neg, add 64; dir==1 → was pos,
        // sub 64) driven by direction byte obj[+0x46]. On zero-cross clamps to 0 and sets
        // clampHit=1; if obj[+0xBF] != 0 also fires SFX 24 (0x18) via FUN_80074590(24,0,15). Tail
        // writes obj[+0x56] = obj[+0x5A] - obj[+0x50] (the SUB form; FUN_80072EFC's semantic).
        uint32_t clampHit = 0;
        uint8_t dir = c->mem_r8(n + 0x46);
        if (dir == 0) {
          int16_t v = (int16_t)(c->mem_r16(n + 0x50) + 64);
          c->mem_w16(n + 0x50, (uint16_t)v);
          if (v > 0) { c->mem_w16(n + 0x50, 0); clampHit = 1; }
        } else if (dir == 1) {
          int16_t v = (int16_t)(c->mem_r16(n + 0x50) - 64);
          c->mem_w16(n + 0x50, (uint16_t)v);
          if (v < 0) { c->mem_w16(n + 0x50, 0); clampHit = 1; }
        }
        // ZERO-CROSS SFX: only when clampHit && obj[+0xBF] != 0
        if (clampHit != 0 && c->mem_r8(n + 0xBF) != 0) {
          eng(c).sfx.trigger(24, 0, 15);                 // FUN_80074590 (native)
        }
        // TAIL: obj[+0x56] = obj[+0x5A] - obj[+0x50]  (SUB form; matches FUN_80072EFC)
        c->mem_w16(n + 0x56, (uint16_t)(c->mem_r16(n + 0x5A) - c->mem_r16(n + 0x50)));

        if (clampHit != 0) {
          c->mem_w8 (n + 0, 1);
          c->mem_w8 (n + 6, 0);
          c->mem_w8 (n + 0x5F, (uint8_t)(1u - (uint32_t)c->mem_r8(n + 0x5F)));
          c->mem_w16(n + 0x84, (uint16_t)(c->mem_r16(n + 0x84) - 800));
          c->mem_w16(n + 0x86, (uint16_t)(c->mem_r16(n + 0x86) - 800));
          return 1;
        }
        return 0;
      }
    }
    return 0;
  };

  int s_v = c->game->verify.on("subswapverify");
  if (!s_v) return impl(node);

  uint8_t* ram0 = c->game->verify.ram0();
  uint8_t* ramN = c->game->verify.ramN();
  uint8_t spad0[0x400], spadN[0x400];
  uint32_t regs0[32]; memcpy(regs0, c->r, sizeof regs0);
  memcpy(ram0, c->ram, 0x200000); memcpy(spad0, c->scratch, 0x400);

  int rv_n = impl(node);

  memcpy(ramN, c->ram, 0x200000); memcpy(spadN, c->scratch, 0x400);
  memcpy(c->ram, ram0, 0x200000); memcpy(c->scratch, spad0, 0x400); memcpy(c->r, regs0, sizeof regs0);
  c->r[4] = node;
  rec_super_call(c, 0x80073328u);
  int rv_s = (int)c->r[2];

  uint32_t sp = regs0[29] & 0x1FFFFFu, flo = (sp >= 0x800) ? sp - 0x800 : 0;
  int ro = -1;
  for (uint32_t a = 0; a < 0x200000; a++) if (c->ram[a] != ramN[a] && !(a >= flo && a < sp)) { ro = (int)a; break; }
  int so = -1;
  for (uint32_t a = 0; a < 0x400; a++) if (c->scratch[a] != spadN[a]) { so = (int)a; break; }
  VerifyHarness::Check& chk = c->game->verify.check("subswapverify");
  long &ng = chk.nMatch, &nb = chk.nMismatch;
  if (ro >= 0 || so >= 0 || rv_n != rv_s) {
    if (nb++ < 40) cfg_logi("subswapverify", "MISMATCH node=%08x case=%u ram@%x spad@%x rv_n=%d rv_s=%d", node, c->mem_r8(node + 6), ro, so, rv_n, rv_s);
  } else if (++ng % 50 == 0) cfg_logi("subswapverify", "%ld matches", ng);
  return rv_n;
}

// ─────────────────────────────────────────────────────────────────────────────
// The per-area SCREEN-TRANSITION ENTER hook: FUN_8006C77C + area 0's ov_a00 hook 0x8010CB60
//
// RE (kanban #47 — "House on the Point" parks in sm[0x4c]==3 forever). The screen transition is
// a handshake between three state machines:
//
//   1. gen_func_80026AD0 (the screen-transition sequencer, slot 3 of the 0x80100400 array).
//      Its case 1 writes sm[0x4c]=3 (screen faded, mid-transition) + bf818=2; its case 3 WAITS
//      for bf818==3; its case 4 is the only writer of sm[0x4c]=2 — i.e. the transition's end.
//   2. SceneTransition::stepSwapWaiter (FUN_80073328), driven per frame by the door object.
//      Its case 2 consumes bf818==2, flips the door's side byte door[+0x5F] and shifts its
//      extents by +800. Its case 3 is the ONLY caller of completeSwap -> bf818=3.
//   3. gen_func_80020868 (the Tomba x door interaction leaf, reached from areaSeasidePerframe).
//      It sets door[+0x29] — the flag stepSwapWaiter case 3 gates on — only when
//      `door[+0x5F] != G[+0x147]`, i.e. only when Tomba FACES INTO the door.
//
//   Because step 2 flipped door[+0x5F], step 3 can only re-arm once Tomba's facing G[+0x147]
//   has ALSO flipped. That flip is the job of Tomba action-state 36 (TOMBA_STATE_DOOR_TRANSIT):
//   gen_func_80058918 dispatches it to gen_func_80065A54, whose sub-state 0 does
//   `G[+0x147] = *(u8*)0x800BF81F >> 4` — reading back the hint gen_func_80020868 stamped as
//   `(1 - door[+0x5F]) << 4`. And the ONLY thing that puts Tomba in state 36 on the way IN is
//   the tail of this hook.
//
// THE BUG: area 0's hook parks on FUN_80044BD4(0x8010DA70, 26, 0, flag=3) — a cooperative
// spawn-and-wait. Under pc_skip the GAME task is a FLAT setjmp task, so PcScheduler::yieldPrim
// longjmps back into PcScheduler::runGameStanza and the C++ stack — hook tail included — is
// discarded (`PSXPORT_DEBUG=sched` prints "caught a GAME substate yield" at that exact frame).
// G[5] therefore never becomes 36, Tomba never turns around, door[+0x29] is never set again,
// bf818 never reaches 3, and sm[0x4c] parks at 3 for the rest of the session (BGM faded out,
// camera frozen, only the reduced fieldFrameX body running).
//
// THE FIX: own the hook and run its sub-scene load SYNCHRONOUSLY, so the tail executes in the
// same call. This is the port's standing rule for every PSX async wait (CLAUDE.md FAIL-FAST),
// and the same collapse Engine's native_area_load_bd4 already applies at the other bd4 sites.
// pc_faithful is deliberately NOT routed here — it runs the gen bodies on the stage FIBER, where
// the yield parks and resumes correctly, so its cadence stays byte-exact with the oracle.
// ─────────────────────────────────────────────────────────────────────────────
namespace {

// Guest addresses this hook touches (all RE'd from gen_func_8006C77C / ov_a00_gen_8010CB60).
constexpr uint32_t AREA_ID              = 0x800BF870u;   // live area index (0..21)
constexpr uint32_t AREA_ENTER_HOOK_TBL  = 0x800A4AF8u;   // per-area ENTER hook fn-ptr table
constexpr uint32_t AREA_NO_ENTER_HOOK   = 3u;            // area 3 has no hook (gen bails)
constexpr uint32_t BF816_SWAP_ARM       = 0x800BF816u;   // "a sub-scene swap is running"
constexpr uint32_t POST_SWAP_FACE_HINT  = 0x800BF81Fu;   // (1 - door[0x5F]) << 4; read >>4 into G[0x147]
constexpr uint32_t A00_DEST_LATCH       = 0x800BF87Cu;   // 0xFF = no scripted destination
constexpr uint32_t A00_FX_BLOCK         = 0x800E8008u;   // area-0 effect block
constexpr uint32_t SPAD_CAM_TARGET      = 0x1F800184u;   // cleared on enter
constexpr uint32_t SPAD_FACE_ANGLE      = 0x1F800210u;   // scratch mirror of G[0x140]
constexpr uint32_t SPAD_LOAD_KIND       = 0x1F80019Cu;   // 3 while the sub-scene load runs, else 0

// Master-block (G = SCENE_BLOCK) fields.
constexpr uint32_t G_STATE      = 0x05u;    // Tomba action-state index (jump table at 0x80015CC4)
constexpr uint32_t G_SUBSTATE   = 0x06u;
constexpr uint32_t G_TYPE       = 0x2Au;
constexpr uint32_t G_FACE_ANGLE = 0x140u;
constexpr uint32_t G_FACING     = 0x147u;   // 0/1 — compared against door[0x5F] by FUN_80020868
constexpr uint32_t G_ANIM_SEL   = 0x16Bu;

// The state whose sub-state 0 (gen_func_80065A54) reloads G[0x147] from POST_SWAP_FACE_HINT.
constexpr uint8_t  TOMBA_STATE_DOOR_TRANSIT = 36u;

// FUN_80044BD4 latches + completion flag (same anchors PcScheduler uses).
constexpr uint32_t BD4_PARAM2    = 0x801FE0DEu;   // task-1 slot +0x6E — read by the spawned body
constexpr uint32_t BD4_PARAM3    = 0x801FE0DDu;   // task-1 slot +0x6D
constexpr uint32_t BD4_DONE_FLAG = 0x1F80019Bu;
constexpr uint32_t CUR_TASK_PTR  = 0x1F800138u;

// Which half the sub-scene loader pulls (bd4's param_2). 0 = this key needs no load at all.
constexpr uint32_t SUBSCENE_LOAD_DOOR   = 26u;   // swap key 1 — a plain door
constexpr uint32_t SUBSCENE_LOAD_SCRIPT = 28u;   // swap key 2 — a scripted destination
constexpr uint32_t SUBSCENE_LOAD_NONE   = 0u;

}  // namespace

// FUN_80044BD4(0x8010DA70, param2, param3, flag=3), collapsed to a synchronous call.
// The spawned task body ov_a00_gen_8010DA70 is three operations —
//   `FUN_80045258(task[0x6E], 15)` (the sub-scene asset pull), `*(u8*)0x1F80019B = 1`,
//   FUN_80051FB4 (self-close) — so it is inlined here rather than armed on slot 1: arming it
// would let the scheduler run it a SECOND time on its next pass.
void SceneTransition::subSceneLoadSync(uint32_t param2, uint32_t param3) {
  Core* c = core;
  c->game->pcSched.forceClose(2);                       // bd4 closes slot 2 before arming slot 1
  c->mem_w8(BD4_PARAM2, (uint8_t)param2);
  c->mem_w8(BD4_PARAM3, (uint8_t)param3);
  c->mem_w8(BD4_DONE_FLAG, 0);
  // bd4's flag!=1 pre-wait tail: the RNG stamp on the waiting task. flag 3 draws the stamp but
  // does NOT bump the wait-frame counter (that is flag 2 only) — dropping it would be the
  // "FUN_80044BD4-collapse incompleteness" defect this file's findings entry warns about.
  c->game->pcSched.bd4Tail(c->mem_r32(CUR_TASK_PTR), 3);
  c->r[4] = param2;
  c->r[5] = 15;
  c->r[31] = 0x8010DA8Cu;                               // the gen body's own jal-site ra
  rec_dispatch(c, 0x80045258u);                         // the sub-scene asset pull
  c->mem_w8(BD4_DONE_FLAG, 1);
}

// FUN_8010CB60 — area 0's transition-ENTER hook (ov_a00 overlay; gen body ov_a00_gen_8010CB60).
void SceneTransition::areaEnterHookA00Sync() {
  Core* c = core;
  const uint32_t G = SCENE_BLOCK;

  c->mem_w8 (BF816_SWAP_ARM, 1);
  c->mem_w8 (A00_FX_BLOCK + 100u, 128);
  c->mem_w32(SPAD_CAM_TARGET, 0);

  uint32_t load = SUBSCENE_LOAD_NONE;

  switch (c->mem_r8(BF817_SWAP_KEY)) {
    case 1:                                              // plain door swap
      c->mem_w8 (G + G_ANIM_SEL, 8);
      c->mem_w8 (SPAD_LOAD_KIND, 3);
      c->mem_w16(SPAD_FACE_ANGLE, c->mem_r16(G + G_FACE_ANGLE));
      load = SUBSCENE_LOAD_DOOR;
      break;

    case 2: {                                            // scripted-destination swap
      c->mem_w16(A00_FX_BLOCK + 6u, 0);
      c->mem_w8 (G + G_TYPE, 38);
      if (c->mem_r8(A00_DEST_LATCH) == 0xFF) {           // no scripted destination: spin 180 deg
        const uint16_t ang = (uint16_t)((c->mem_r16(G + G_FACE_ANGLE) + 2048u) & 4095u);
        c->mem_w8 (G + G_FACING, 0);
        c->mem_w8 (POST_SWAP_FACE_HINT, 0);
        c->mem_w16(G + G_FACE_ANGLE, ang);
        c->mem_w16(SPAD_FACE_ANGLE, ang);
      } else {
        c->mem_w8 (G + G_ANIM_SEL, 8);
        c->mem_w16(SPAD_FACE_ANGLE, c->mem_r16(G + G_FACE_ANGLE));
      }
      c->mem_w8 (SPAD_LOAD_KIND, 3);
      load = SUBSCENE_LOAD_SCRIPT;
      break;
    }

    default:                                             // keys 0 and 3+: no sub-scene load
      c->mem_w8 (G + G_ANIM_SEL, 8);
      c->mem_w16(SPAD_FACE_ANGLE, c->mem_r16(G + G_FACE_ANGLE));
      break;
  }

  if (load != SUBSCENE_LOAD_NONE) {
    subSceneLoadSync(load, 0);
    c->mem_w8(SPAD_LOAD_KIND, 0);
  }

  // TAIL — this is the part the cooperative yield used to eat. G[5]=36 is what makes
  // gen_func_80058918 dispatch gen_func_80065A54, whose sub-state 0 reloads Tomba's facing from
  // POST_SWAP_FACE_HINT and so re-opens the door gate that completes the transition.
  clearSwapBlock(G);
  c->mem_w8(G + G_STATE,    TOMBA_STATE_DOOR_TRANSIT);
  c->mem_w8(G + G_SUBSTATE, 0);
  c->r[4] = c->mem_r8(AREA_ID);
  c->r[31] = 0x8006C7B4u;
  rec_dispatch(c, 0x80074F24u);
}

// FUN_8006C77C — dispatch the live area's ENTER hook.
void SceneTransition::areaTransitionEnterSync() {
  Core* c = core;
  const uint8_t area = c->mem_r8(AREA_ID);
  if (area == AREA_NO_ENTER_HOOK) return;
  if (area == 0) { areaEnterHookA00Sync(); return; }     // owned (table entry 0x8010CB60)
  // Areas 1..21 still run their substrate hook. Any of those that also parks on FUN_80044BD4
  // carries the SAME truncation defect — porting them is the follow-up (docs/findings/scene.md).
  c->r[31] = 0x8006C7B4u;
  rec_dispatch(c, c->mem_r32(AREA_ENTER_HOOK_TBL + (uint32_t)area * 4u));
}
