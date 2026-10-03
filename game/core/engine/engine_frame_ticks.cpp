// game/core/engine_frame_ticks.cpp — the frame-BOUNDARY work — everything that runs once per field on a fixed phase
// rather than as part of the state machine — plus the stage bootstrap.
//
// Split out of game/core/engine.cpp on 2026-09-28. `Engine` is one class and stays one class: this
// is a change of TRANSLATION UNIT, not of ownership, and nothing here moved to a different class or
// to a different subsystem. The split is by the level of the guest's own three-level state machine
// the bodies serve, so a reader looking for "what runs in the running sub-mode" has one file to open.
//
// The class's own header (game/core/engine.h) remains the single declaration of every method below;
// nothing is redeclared here.
#include "c_subsys.h"               // disc_findFile — the native ISO9660 resolver
#include "camera/cutscene_camera.h" // class CutsceneCamera — the field/follow camera
#include "core.h"
#include "core/assets/asset.h" // class Asset
#include "core/engine/engine.h"
#include "core/engine/task_sm.h"         // TaskSm — the typed lens over the task record, owned in one header
#include "core/overrides/guest_jal.h"    // tomba::guest::dispatchJalToReturn — the guest call convention
#include "core/overrides/guest_resume.h" // requestGuestContinuation
#include "entry/game_ctx.h"
#include "game.h" // Game — the per-Core product the Engine drives
#include "guest_abi.h"
#include "guest_call.h"
#include "level_load.h"
#include "math/rng.h" // class Rng
#include "overrides/native_override_catalog.h"
#include "placement.h" // ov_placeObjects
#include "pool.h"      // ov_poolInitRun
#include "render.h"
#include "scene/card_load_machine.h" // tomba::scene::stepCardLoadMachine
#include "scene/start_bin_stage.h"   // class StartBinStage — the task-0 file-table builder
#include <cstdint>
#include <cstdio>

// Engine::postRenderTick — 3-state fx-trigger + countdown on byte 0x800BF842 at
// guest 0x80077D8C. Faithful to the disasm: low 7 bits select (== 1: fire FX
// 41, set b42 = 0x87), (== 2: fire FX 42, clear b42), other/nonzero: decrement
// b42. Zero = no-op. FX 41/42 leaf FUN_80074590 stays substrate.
void Engine::postRenderTick() {
  Core *c = core;
  uint8_t b = c->mem_r8(0x800BF842u);
  if (b == 0) {
    return;
  }
  uint8_t low = (uint8_t)(b & 0x7F);
  if (low == 1) {
    eng(c).sfx.trigger(41, 2, -65); // FUN_80074590 (native; pitchBend -65)
    c->mem_w8(0x800BF842u, 0x87);
    return;
  }
  if (low == 2) {
    eng(c).sfx.trigger(42, 2, -65); // FUN_80074590 (native; pitchBend -65)
    c->mem_w8(0x800BF842u, 0);
    return;
  }
  c->mem_w8(0x800BF842u, (uint8_t)(b - 1));
}

// Engine::postRenderTickFaithful -- byte-exact mirror of guest 0x80077D8C.
// Descends the gen's own 24-byte guest frame, spills s0 (r16, the 0x800BF808
// struct base) at sp+16 and the caller's ra at sp+20 exactly like the
// reference-mirror shape (Engine::submode1Faithful), sets r31 to the gen's
// jal-site constant before the FX-trigger leaf, and dispatches that leaf via
// typed runtime address dispatch so it runs the literal substrate body guest 0x80074590 (no
// override-registry entry exists for 0x80074590, so this is guaranteed
// byte-identical -- including guest 0x80074590's own ra-to-stack spill, which
// the native Sfx::trigger port does not reproduce and must not be used here).
// v0/v1 (r2/r3) are dead to every known caller but guest 0x80077D8C leaves
// them holding whatever value each branch happened to compute (b, b&0x7f, 135,
// 0, 0x800C0000, 0x800BF808, b-1) instead of restoring/clearing them -- the
// mirror must reproduce that leftover ABI end-state per-branch or strict replay check's
// v0/v1 compare fails.
void Engine::postRenderTickFaithful() {
  Core *c = core;
  c->r[29] -= 24;
  const uint32_t sp = c->r[29];
  c->mem_w32(sp + 16, c->r[16]); // spill s0
  c->mem_w32(sp + 20, c->r[31]); // spill caller ra
  c->r[16] = 0x800BF808u;        // s0 = struct base

  uint8_t b = c->mem_r8(c->r[16] + 58); // == 0x800BF842
  c->r[2] = b;                          // v0 = loaded byte (gen never re-clears it on the
                                        // early-exit path -- the mirror must reproduce that
                                        // leftover ABI value, not leave v0/v1 untouched from
                                        // the caller)
  if (b != 0) {
    uint8_t low = (uint8_t)(b & 0x7F);
    c->r[3] = low; // v1 = andi r3,r2,0x7f (gen's unconditional delay slot)
    if (low == 1) {
      c->r[4] = 41;
      c->r[5] = 2;
      c->r[6] = (uint32_t)-65;
      c->r[31] = 0x80077DDCu; // gen's jal-site ra for this call
      psx::cpu::dispatchGuestToReturn0(*c,
                                       0x80074590u,
                                       psx::cpu::ExecutionBudget::currentTurn(*c),
                                       __func__); // -> guest 0x80074590 (substrate, un-owned leaf)
      c->r[2] = 135;                              // v0 = 135 (gen literal, r3/v1 stays == 1)
      c->mem_w8(c->r[16] + 58, (uint8_t)c->r[2]);
    } else if (low == 2) {
      c->r[3] = 0x800C0000u; // v1 = lui r3,0x800c (gen's delay-slot clobber of low)
      c->r[4] = 42;
      c->r[5] = 2;
      c->r[6] = (uint32_t)-65;
      c->r[31] = 0x80077DF8u; // gen's jal-site ra for this call
      psx::cpu::dispatchGuestToReturn0(*c, 0x80074590u, psx::cpu::ExecutionBudget::currentTurn(*c), __func__);
      c->r[2] = 0; // v0 = r0
      c->mem_w8(c->r[16] + 58, 0);
    } else {
      c->r[3] = 0x800C0000u - 2040;        // v1 = 0x800BF808 (gen: r3 = 0x800c0000 + -2040)
      uint8_t v = c->mem_r8(c->r[3] + 58); // same address as b (0x800BF842); re-read per gen
      c->r[2] = (uint32_t)v - 1;           // v0 = byte - 1 (full 32-bit, not just the stored 8 bits)
      c->mem_w8(c->r[3] + 58, (uint8_t)c->r[2]);
    }
  } else {
    c->r[3] = 0; // v1 = 0 & 127 (gen's delay slot on the b==0 branch)
  }

  c->r[31] = c->mem_r32(sp + 20); // restore ra
  c->r[16] = c->mem_r32(sp + 16); // restore s0
  c->r[29] += 24;
}

// Engine::frameStartTick — per-frame prologue at guest 0x80059D28 (FIRST call
// in ov_field_frame's gameplay-update block). Faithful port of the disasm; see
// engine.h for the step-by-step contract. Callees kept substrate: the
// mode-keyed overlay handler (branches at (e)), FUN_8005950C default, and the
// rand LFSR advance at 0x8009A450 (the top recdep hit — 86 calls/frame — a
// future target).
void Engine::frameStartTick() {
  Core *c = core;
  // dispatch (d) reaches dynamically-loaded overlay handlers
  // (0x8010F63C/0x80109024/0x80112220/ 0x8010F654) that can scheduler_yield;
  // strict replay check's synchronous strictCheck only supports yield-free mirrors
  // (strictCheck aborts while inCheck), so this fork is a plain call, proven by
  // SBS full (core A vs core B), not strict replay check.
  static constexpr uint32_t G = 0x800E7E80u; // master G block base (== s0 in the guest)

  // (a) counter@0x800BF819: if nonzero, decrement + mask two 12-bit heading
  // fields.
  uint8_t cnt = c->mem_r8(0x800BF819u);
  if (cnt != 0) {
    c->mem_w8(0x800BF819u, (uint8_t)(cnt - 1));
    c->mem_w16(0x800ECF54u, (uint16_t)(c->mem_r16(0x800ECF54u) & 0x0FFFu));
    c->mem_w16(0x800E7E68u, (uint16_t)(c->mem_r16(0x800E7E68u) & 0x0FFFu));
  }
  // (b) zero frame-scoped flag bank.
  c->mem_w8(G + 0x177u, 0);
  c->mem_w8(G + 0x179u, 0);
  c->mem_w8(G + 0x17Au, 0);
  c->mem_w8(G + 0x17Bu, 0);
  c->mem_w8(0x1F80027Au, 0);
  // (c) per-frame stamp++.
  c->mem_w8(0x1F800247u, (uint8_t)(c->mem_r8(0x1F800247u) + 1u));

  // (d) if 0x800BF841 == 0: mode-keyed per-frame handler dispatch, clear
  // 0x1F800230.
  if (c->mem_r8(0x800BF841u) == 0) {
    uint8_t mode = c->mem_r8(0x800BF870u);
    uint32_t target;
    switch (mode) {
    case 2:
      target = 0x8010F63Cu;
      break;
    case 3:
      target = 0x80109024u;
      break;
    case 7:
      target = 0x80112220u;
      break;
    case 20:
      target = 0x8010F654u;
      break;
    default:
      target = 0x8005950Cu;
      break;
    }
    c->r[4] = G;
    psx::cpu::dispatchGuestToReturn0(*c, target, psx::cpu::ExecutionBudget::currentTurn(*c), __func__);
    c->mem_w8(0x1F800230u, 0);
  }

  // (e) master position + heading -> scratchpad (for projection/cull).
  c->mem_w16(0x1F800160u, c->mem_r16(G + 0x2Eu));
  c->mem_w16(0x1F800162u, c->mem_r16(G + 0x32u));
  c->mem_w16(0x1F800164u, c->mem_r16(G + 0x36u));
  c->mem_w16(0x1F80016Au, c->mem_r16(G + 0x58u));
  c->mem_w16(0x1F800168u,
             c->mem_r16(G + 0x56u)); // written unconditionally (delay slot)
  // (f) latch 0x800BF81E = 1 when 0x800BF9C3 & 0x80.
  if (c->mem_r8(0x800BF9C3u) & 0x80u) {
    c->mem_w8(0x800BF81Eu, 1);
  }

  // (g) tick sub-counter G+0x180 when 0x1F800137 (pause) == 0.
  if (c->mem_r8(0x1F800137u) == 0) {
    uint8_t v = c->mem_r8(G + 0x180u);
    if (v != 0) {
      c->mem_w8(G + 0x180u, (uint8_t)(v - 1));
    }
  }
  // (h) advance rand LFSR (native class Rng — shared seed at 0x80105EE8 with
  // substrate callers).
  (void)rngOf(c).next();
}

// Engine::frameStartTickFaithful — byte-exact mirror of guest 0x80059D28
// (guest 0x80059D28), the FIRST call in ov_field_frame's gameplay-update block.
// Descends the guest frame (sp-=24), spills incoming r31/r16 at +20/+16 per the
// gen prologue, and sets the gen's per-case jal-site r31 constant at every
// dispatch/call boundary so callee guest-stack ra spills (e.g. guest 0x8005950C's
// spill at its own sp+28) byte-match core B. Callees kept substrate: the
// mode-keyed overlay handler (targets at
// 0x8010F63C/0x80109024/0x80112220/0x8010F654 — dynamically-loaded overlay
// code, not in authenticated executable/overlay evidence), FUN_8005950C default, and the rand LFSR advance at
// 0x8009A450 — dispatched to the real gen leaf (not the native Rng class) so
// its v0/v1/hi/lo side effects, which are still live at this function's own
// return (rand is the last statement before the epilogue), byte-match core B;
// guest 0x8009A450 never touches r31, so the r31 set before it is
// precautionary discipline only.
void Engine::frameStartTickFaithful() {
  Core *c = core;
  c->r[29] -= 24;
  const uint32_t sp = c->r[29];
  c->mem_w32(sp + 20, c->r[31]);
  c->mem_w32(sp + 16, c->r[16]);             // spill CALLER's r16 (not yet reassigned)
  static constexpr uint32_t G = 0x800E7E80u; // master G block base
  c->r[16] = G;                              // gen: delay-slot `r16 = r3 + 32384` — must land in
                                             // the guest register itself (not just the local `G`
                                             // constant), since callees (FUN_8005950C / the mode
                                             // handlers below) are callee-saved on s0/r16 and spill
                                             // it verbatim onto their own frames.

  // (a) counter@0x800BF819: if nonzero, decrement + mask two 12-bit heading
  // fields.
  uint8_t cnt = c->mem_r8(0x800BF819u);
  if (cnt != 0) {
    c->mem_w8(0x800BF819u, (uint8_t)(cnt - 1));
    c->mem_w16(0x800ECF54u, (uint16_t)(c->mem_r16(0x800ECF54u) & 0x0FFFu));
    c->mem_w16(0x800E7E68u, (uint16_t)(c->mem_r16(0x800E7E68u) & 0x0FFFu));
  }
  // (b) zero frame-scoped flag bank.
  c->mem_w8(G + 0x177u, 0);
  c->mem_w8(G + 0x179u, 0);
  c->mem_w8(G + 0x17Au, 0);
  c->mem_w8(G + 0x17Bu, 0);
  c->mem_w8(0x1F80027Au, 0);
  // (c) per-frame stamp++ (unconditional — gen's branch-delay-slot store).
  c->mem_w8(0x1F800247u, (uint8_t)(c->mem_r8(0x1F800247u) + 1u));

  // (d) if 0x800BF841 == 0: mode-keyed per-frame handler dispatch (each case
  // sets its own gen jal-site r31 constant before the call), then clear
  // 0x1F800230.
  if (c->mem_r8(0x800BF841u) == 0) {
    uint8_t mode = c->mem_r8(0x800BF870u);
    c->r[4] = G;
    switch (mode) {
    case 2:
      c->r[31] = 0x80059DF8u;
      psx::cpu::dispatchGuestToReturn0(*c, 0x8010F63Cu, psx::cpu::ExecutionBudget::currentTurn(*c), __func__);
      break;
    case 3:
      c->r[31] = 0x80059E08u;
      psx::cpu::dispatchGuestToReturn0(*c, 0x80109024u, psx::cpu::ExecutionBudget::currentTurn(*c), __func__);
      break;
    case 7:
      c->r[31] = 0x80059E18u;
      psx::cpu::dispatchGuestToReturn0(*c, 0x80112220u, psx::cpu::ExecutionBudget::currentTurn(*c), __func__);
      break;
    case 20:
      c->r[31] = 0x80059E28u;
      psx::cpu::dispatchGuestToReturn0(*c, 0x8010F654u, psx::cpu::ExecutionBudget::currentTurn(*c), __func__);
      break;
    default:
      c->r[31] = 0x80059E38u;
      psx::cpu::dispatchGuestToReturn0(*c, 0x8005950Cu, psx::cpu::ExecutionBudget::currentTurn(*c), __func__);
      break;
    }
    c->mem_w8(0x1F800230u, 0);
  }

  // (e) master position + heading -> scratchpad (for projection/cull).
  c->mem_w16(0x1F800160u, c->mem_r16(G + 0x2Eu));
  c->mem_w16(0x1F800162u, c->mem_r16(G + 0x32u));
  c->mem_w16(0x1F800164u, c->mem_r16(G + 0x36u));
  c->mem_w16(0x1F80016Au, c->mem_r16(G + 0x58u));
  c->mem_w16(0x1F800168u,
             c->mem_r16(G + 0x56u)); // written unconditionally (delay slot)
  // (f) latch 0x800BF81E = 1 when 0x800BF9C3 & 0x80.
  if (c->mem_r8(0x800BF9C3u) & 0x80u) {
    c->mem_w8(0x800BF81Eu, 1);
  }

  // (g) tick sub-counter G+0x180 when 0x1F800137 (pause) == 0.
  if (c->mem_r8(0x1F800137u) == 0) {
    uint8_t v = c->mem_r8(G + 0x180u);
    if (v != 0) {
      c->mem_w8(G + 0x180u, (uint8_t)(v - 1));
    }
  }
  // (h) advance rand LFSR. guest 0x8009A450 is the LAST statement in the guest
  // body before the epilogue, so its v0/v1/hi/lo side effects (masked value in
  // v0, the 0x41C64E6D multiplier loaded into v1, MULT's hi/lo product) are
  // still live in those registers when this function returns — the strict
  // ABI-register gate compares them unconditionally. rngOf(c).next() only
  // performs the guest-memory seed update (mem_r32/mem_w32 on 0x80105EE8,
  // matching the shared stream with substrate callers) but is a plain C++ call
  // with no register-level side effects, so it silently dropped v0/v1/hi/lo.
  // Dispatch the real gen leaf instead so those registers land bit-for-bit like
  // core B (guest 0x8009A450 is a pure no-yield leaf — a plain call/
  // typed runtime address dispatch is safe here, same as the mode-dispatch calls above).
  c->r[31] = 0x80059EC8u;
  psx::cpu::dispatchGuestToReturn0(*c, 0x8009A450u, psx::cpu::ExecutionBudget::currentTurn(*c), __func__);

  c->r[31] = c->mem_r32(sp + 20);
  c->r[16] = c->mem_r32(sp + 16);
  c->r[29] += 24;
}

// Register the GAME-stage area-init overrides when this just-loaded overlay is
// GAME.BIN at the stage base. Detect by the fixed entry + handler signatures
// (START.BIN/DEMO.BIN are smaller and hold stale bytes at these addresses, so
// they never match). Called from the overlay-load scan (submit.cpp); registered
// AUTO so it is flushed when GAME.BIN unloads and another overlay reuses the
// base (mirrors the M3 scan).
#include "scheduler.h" // CUR_TASK + scheduler_yield

// --- PC-native task-0 bootstrap: own the START.BIN resolve + stage-0 overlay
// load top-down ---------- Replaces the FUN_800499e8 -> FUN_80052078 ->
// FUN_800450bc CD subtree, which (run as a pure-PSX leaf now that the CD
// overrides are unregistered) busy-waits forever on the libcd Init/Read
// handshake. Behavioural reference (read via tools/disas.py):
//   FUN_800499e8 : CdSearchFile("\BIN\START.BIN") -> {MSF,size}; store
//   {LBA,size} at 0x800be1e0/e4;
//                  FUN_80052078(0).
//   FUN_80052078 : FUN_800450bc(task+0xc, 0); task.state=3; task[0x6f]=0; a few
//   libgpu/BIOS resets. FUN_800450bc : FUN_8001db8c(0x80106228, LBA, size) [=
//   cd_loadfile]; entry = STAGE_ENTRY[0]
//                  (0x8010649c); task+0xc = entry, task+0x10 = caller gp.
// The per-stage {LBA,size} table lives at 0x800be1e0 (stride 8); the
// stage-entry table at 0x800a3ecc.
// FUN_80052078: switch task 0 to the given stage (load overlay + reset the
// display/BIOS bits). Public entry: called by DEMO's LEAVE-to-GAME substate
// (demo.cpp s5), by task0Bootstrap after the START.BIN file-table build, and by
// Engine::startBinStage once StartBinStage::runNative has built the file table. Was `native_start_stage`
// (static) + `demo_start_stage` (public wrapper).
void Engine::startStage(uint32_t stage) {
  Core *c = core;
  uint32_t task = c->mem_r32(0x1f800138); // current task (= task 0, 0x801fe000)
  tomba::stage::loadOverlay(*c, activeStageOverlay, task + 0xc, stage);
  c->mem_w16(task, 3); // task state = 3 (active)
  c->mem_w8(task + 0x6f, 0);
  psx::cpu::dispatchGuestToReturn0(
      *c, 0x80080890u, psx::cpu::ExecutionBudget::currentTurn(*c), __func__); // EnterCriticalSection (BIOS leaf)
  c->r[4] = c->mem_r32(task + 4);
  psx::cpu::dispatchGuestToReturn0(
      *c, 0x80080870u, psx::cpu::ExecutionBudget::currentTurn(*c), __func__); // B(0Fh) reset (BIOS leaf)
  psx::cpu::dispatchGuestToReturn0(
      *c, 0x800808a0u, psx::cpu::ExecutionBudget::currentTurn(*c), __func__); // ExitCriticalSection (BIOS leaf)
  c->r[4] = 0xff000000u;
  scheduler_yield(c); // ChangeThread — native scheduler yield
}

// FUN_800499e8: resolve \BIN\START.BIN natively, record its {LBA,size}, switch
// task 0 to stage 0. Called once from native_boot.cpp's game_init (the
// boot-init prefix). Was `native_task0_bootstrap`.
void Engine::task0Bootstrap() {
  Core *c = core;
  uint32_t lba = 0, size = 0;
  if (!disc_find_file(&c->game->disc, "\\BIN\\START.BIN", &lba, &size)) {
    cfg_loge("native_boot", "FATAL: cannot resolve \\BIN\\START.BIN on disc");
    return;
  }
  c->mem_w32(tomba::stage::kFileTable, lba);      // 0x800be1e0 = START.BIN LBA
  c->mem_w32(tomba::stage::kFileTable + 4, size); // 0x800be1e4 = START.BIN size
  cfg_logi("native_boot", "START.BIN resolved: LBA %u, %u bytes", lba, size);
  startStage(0);
}

// Task-0's START.BIN stage bodies — the file-table builder and boot preloads are owned by
// StartBinStage (game/scene/start_bin_stage.*); Engine keeps the stage lifecycle around them.
void Engine::startBinStage() {
  StartBinStage(*core, asset).runNative();
  startStage(1);
}

void Engine::startBinStageFaithful() {
  StartBinStage(*core, asset).runFaithful();
}

// FUN_80078824 — Engine::setAreaStartPos. Loads the player's per-area spawn
// position + facing from the start-pos table 0x800A54A8[area] (word = the
// area's 8-byte-record sub-table), record[sub]: three int16 coords stored <<16
// fixed to 0x800BF890/894/898, facing byte (masked 0x7F) to 0x800BFE38. Leaf,
// no frame, no callees. ORACLE: guest 0x80078824
void Engine::setAreaStartPos() {
  Core *c = this->core;
  const uint32_t START_TABLE = 0x800A54A8u;                             // per-area start-pos table base
  uint32_t area = c->mem_r8(0x800BF870);                                // current area index
  uint32_t sub = c->mem_r8(0x800BF871);                                 // current sub-area index
  uint32_t rec = c->mem_r32(START_TABLE + (area << 2));                 // area's 8-byte-record sub-table
  rec += sub << 3;                                                      // record[sub]
  c->mem_w32(0x800BF890, (uint32_t)(int16_t)c->mem_r16(rec + 0) << 16); // start X (<<16 fixed)
  c->mem_w32(0x800BF894, (uint32_t)(int16_t)c->mem_r16(rec + 2) << 16); // start Y
  c->mem_w32(0x800BF898, (uint32_t)(int16_t)c->mem_r16(rec + 4) << 16); // start Z
  c->mem_w8(0x800BFE38, c->mem_r8(rec + 6) & 0x7Fu);                    // facing byte
}

// ── Engine anim-leaf overrides (phase-3 fallthrough-for-already-native,
// 2026-07-15) ───────────────── animTick (FUN_8004190C) and walkStart
// (FUN_80054D14) are native Engine methods, but the guest addresses were
// registered NOWHERE — so their typed runtime address dispatch/callObj callers
// (beh_actor_tomba_proximity_ combat, beh_a06_scripted_actor) + the 5/9 direct
// substrate a direct guest-address call shard sites all ran the EMULATED body while direct
// native callers (beh_sop_intro_pilot) ran the port (a split). Found by
// `codemap.py --substrate-fallthrough`. One `tomba::native::declareOverride` entry covers
// both the registry's typed runtime address dispatch path and tomba::native::declareOverride (image-qualified runtime
// dispatcher); core B stays pure substrate. MIRROR_VERIFY-gated.
namespace {
void ov_engineAnimTick(Core *c) {
  c->r[2] = eng(c).animTick(c->r[4]);
}
void ov_engineWalkStart(Core *c) {
  c->r[2] = eng(c).walkStart(c->r[4], c->r[5], (int16_t)c->r[6]);
}
void ov_engineSetAreaStartPos(Core *c) {
  eng(c).setAreaStartPos();
}
void ov_engineActiveModeCtx(Core *c) {
  c->r[2] = eng(c).activeModeCtx();
}
void ov_engineInstallModeHandlers(Core *c) {
  eng(c).installModeHandlers();
}
void ov_engineRunModeEnter(Core *c) {
  c->r[2] = eng(c).runModeEnter();
}
} // namespace

void RegisterEngineAnimLeafOverrides(Game * /*game*/) {
  tomba::native::declareOverride(0x8004190Cu, "Engine::animTick", ov_engineAnimTick);
  tomba::native::declareOverride(0x80054D14u, "Engine::walkStart", ov_engineWalkStart);
  tomba::native::declareOverride(0x80078824u, "Engine::setAreaStartPos", ov_engineSetAreaStartPos);
  tomba::native::declareOverride(0x80086604u, "Engine::activeModeCtx", ov_engineActiveModeCtx);
  tomba::native::declareOverride(0x80086738u, "Engine::installModeHandlers", ov_engineInstallModeHandlers);
  tomba::native::declareOverride(0x80086764u, "Engine::runModeEnter", ov_engineRunModeEnter);
}
