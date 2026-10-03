// game/core/engine_field_run.cpp — the field RUN and FRAME-X families — the per-frame field work and its guest-faithful
// mirror, plus the running sub-mode's own two cases.
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
#include "core/asset.h" // class Asset
#include "core/engine.h"
#include "core/guest_jal.h"    // tomba::guest::dispatchJalToReturn — the guest call convention
#include "core/guest_resume.h" // requestGuestContinuation
#include "core/task_sm.h"      // TaskSm — the typed lens over the task record, owned in one header
#include "game.h"              // Game — the per-Core product the Engine drives
#include "game_ctx.h"
#include "guest_abi.h"
#include "guest_call.h"
#include "level_load.h"
#include "math/rng.h" // class Rng
#include "native_override_catalog.h"
#include "placement.h" // ov_placeObjects
#include "pool.h"      // ov_poolInitRun
#include "render.h"
#include "scene/card_load_machine.h" // tomba::scene::stepCardLoadMachine
#include "scene/start_bin_stage.h"   // class StartBinStage — the task-0 file-table builder
#include <cstdint>
#include <cstdio>

// Engine::fadeSequencer moved to ScreenFade::sequence
// (game/render/screen_fade.cpp).

// FIELD RUNNING sub-machine 0x80106b98 — native control flow + state bodies
// (decomp: scratch/decomp/game/80106b98.c). A 12-way switch on sm[0x4e]; the
// running states call the native ov_field_frame (0x80108b0c) and the heavy leaf
// callees typed runtime address dispatch. NB the guest fall-throughs are faithful: case 2 -> 3,
// case 4 -> 1 (no break). sm[0x4e] >= 12 = no-op. This anchors the field frame
// natively; the leaf callees (object-placement FUN_80072a78 etc.) are the next
// descent. pc_faithful FIELD RUNNING sub-machine — exact mirror of
// overlay guest 0x80106B98 (12 states on sm[0x4e]). Guest frame (sp-24, ra@+20,
// r16@+16, live values) + every leaf dispatched at its RE'd jal site so callee
// spills byte-match core B. overlay guest 0x80108B0C (the field per-frame update)
// runs the native owner Engine::fieldFrame with the gen's r31. Notable gen
// details the rebuilt fieldRun below got WRONG (kept there for native_sync,
// fixed here): 0x1F800194 is a HALFWORD store of u16(0x800E7FEE) (not w32), the
// state-0 area check is mem_r16(0x800BF870)==21 (not r32==0x15), and state 6
// writes 0x800BF870/71 as TWO byte stores of the swapped/masked halves.
void Engine::fieldRunFaithful() {
  Core *c = core;
  uint32_t sm = c->mem_r32(0x1f800138u);
  c->r[29] -= 24;
  const uint32_t sp = c->r[29];
  c->mem_w32(sp + 20, c->r[31]);
  c->mem_w32(sp + 16, c->r[16]);
  uint16_t s4e = c->mem_r16(sm + 0x4e);
  if (s4e < 12) {
    switch (s4e) {
    case 0: { // L_80106BDC — area object/pool init chain
      c->r[31] = 0x80106BE4u;
      psx::cpu::dispatchGuestToReturn0(*c, 0x8007B18Cu, psx::cpu::ExecutionBudget::currentTurn(*c), __func__);
      c->r[31] = 0x80106BECu;
      psx::cpu::dispatchGuestToReturn0(*c, 0x800796DCu, psx::cpu::ExecutionBudget::currentTurn(*c), __func__);
      c->r[31] = 0x80106BF4u;
      psx::cpu::dispatchGuestToReturn0(*c, 0x800263E8u, psx::cpu::ExecutionBudget::currentTurn(*c), __func__);
      c->r[31] = 0x80106BFCu;
      psx::cpu::dispatchGuestToReturn0(*c, 0x80072A78u, psx::cpu::ExecutionBudget::currentTurn(*c), __func__);
      c->r[31] = 0x80106C04u;
      psx::cpu::dispatchGuestToReturn0(*c, 0x80075240u, psx::cpu::ExecutionBudget::currentTurn(*c), __func__);
      c->r[31] = 0x80106C0Cu;
      psx::cpu::dispatchGuestToReturn0(*c, 0x800783DCu, psx::cpu::ExecutionBudget::currentTurn(*c), __func__);
      c->r[31] = 0x80106C14u;
      psx::cpu::dispatchGuestToReturn0(*c, 0x80078610u, psx::cpu::ExecutionBudget::currentTurn(*c), __func__);
      sm = c->mem_r32(0x1f800138u);
      c->mem_w16(sm + 0x4e, 1);
      c->mem_w8(sm + 0x6b, 0);
      if (c->mem_r8(0x800BF89Cu) == 2) {
        sm = c->mem_r32(0x1f800138u);
        c->mem_w16(sm + 0x4e, 9);
      } else if (c->mem_r8(0x800BF870u) == 8) {
        c->r[31] = 0x80106C88u;
        psx::cpu::dispatchGuestToReturn0(*c, 0x80114B90u, psx::cpu::ExecutionBudget::currentTurn(*c), __func__);
      } else if (c->mem_r16(0x800BF870u) == 21) {
        sm = c->mem_r32(0x1f800138u);
        c->mem_w16(sm + 0x4e, 11);
        break;
      }
      c->r[4] = c->mem_r8(0x800BF870u);
      c->r[31] = 0x80106C98u;
      psx::cpu::dispatchGuestToReturn0(*c, 0x80074F24u, psx::cpu::ExecutionBudget::currentTurn(*c), __func__);
      break;
    }
    case 2: // L_80106DDC — G-state mutate, fall into 3
      c->r[4] = 0x800E7E80u;
      c->r[5] = 12;
      c->r[31] = 0x80106DECu;
      psx::cpu::dispatchGuestToReturn0(*c, 0x80058304u, psx::cpu::ExecutionBudget::currentTurn(*c), __func__);
      sm = c->mem_r32(0x1f800138u);
      c->mem_w16(sm + 0x4e, (uint16_t)(c->mem_r16(sm + 0x4e) + 1));
      /* fallthrough */
    case 3: // L_80106E08 — settle audio, arm mode state
      c->r[31] = 0x80106E10u;
      psx::cpu::dispatchGuestToReturn0(*c, 0x80074BC4u, psx::cpu::ExecutionBudget::currentTurn(*c), __func__);
      c->r[4] = 0;
      if (c->mem_r8(0x800BF870u) == 8) {
        c->r[31] = 0x80106E2Cu;
        psx::cpu::dispatchGuestToReturn0(*c, 0x80114B90u, psx::cpu::ExecutionBudget::currentTurn(*c), __func__);
        c->r[4] = 0;
      }
      c->r[5] = 0;
      c->r[6] = 0;
      sm = c->mem_r32(0x1f800138u);
      c->mem_w16(sm + 0x4a, 2);
      c->mem_w16(sm + 0x4c, 0);
      c->mem_w16(sm + 0x4e, 0);
      c->r[31] = 0x80106E54u;
      psx::cpu::dispatchGuestToReturn0(*c, 0x8005082Cu, psx::cpu::ExecutionBudget::currentTurn(*c), __func__);
      break;
    case 4: // L_80106CE0 — camera + mode-state re-arm, fall into 1
      c->r[31] = 0x80106CE8u;
      psx::cpu::dispatchGuestToReturn0(*c, 0x8006C7C4u, psx::cpu::ExecutionBudget::currentTurn(*c), __func__);
      c->r[31] = 0x80106CF0u;
      psx::cpu::dispatchGuestToReturn0(*c, 0x800508A8u, psx::cpu::ExecutionBudget::currentTurn(*c), __func__);
      sm = c->mem_r32(0x1f800138u);
      c->mem_w16(sm + 0x4e, 1);
      /* fallthrough */
    case 1: { // L_80106D00 — the RUNNING field frame
      c->r[31] = 0x80106D08u;
      eng(c).fieldFrame();
      if ((int8_t)c->mem_r8(0x800BF80Du) == 3) {
        if (c->mem_r8(0x800BF80Fu) != 0) {
          break;
        }
        c->r[31] = 0x80106D38u;
        psx::cpu::dispatchGuestToReturn0(*c, 0x80074BC4u, psx::cpu::ExecutionBudget::currentTurn(*c), __func__);
        int16_t ev = c->mem_r16s(0x800E7FEEu);
        uint16_t evu = c->mem_r16(0x800E7FEEu);
        if (ev == 0) { // L_80106F48 — s4e++
          sm = c->mem_r32(0x1f800138u);
          c->mem_w16(sm + 0x4e, (uint16_t)(c->mem_r16(sm + 0x4e) + 1));
          break;
        }
        c->mem_w8(0x800BF880u, 1);
        sm = c->mem_r32(0x1f800138u);
        c->mem_w16(0x1F800194u, evu);
        c->mem_w16(sm + 0x4e, 0);
        break;
      }
      uint8_t trig = c->mem_r8(0x800BF839u);
      if (trig == 0) {
        break;
      }
      if (c->mem_r8(0x800BF80Fu) != 0) {
        break;
      }
      if (trig == 8) {
        sm = c->mem_r32(0x1f800138u);
        c->mem_w16(sm + 0x4a, 3);
        c->mem_w16(sm + 0x4c, 0);
        c->mem_w16(sm + 0x4e, 0);
        break;
      }
      if (c->mem_r8(0x1F800236u) >= 5) {
        c->r[4] = 0;
        c->r[31] = 0x80106DCCu;
        psx::cpu::dispatchGuestToReturn0(*c, 0x80050894u, psx::cpu::ExecutionBudget::currentTurn(*c), __func__);
      }
      sm = c->mem_r32(0x1f800138u);
      c->mem_w16(sm + 0x4a, 1); // L_80106FAC join (r2 = 1)
      c->mem_w16(sm + 0x4c, 2);
      c->mem_w16(sm + 0x4e, 6);
      break;
    }
    case 5: // L_80106CA0 — area-7 mode re-arm + field frame
      if (c->mem_r8(0x800BF870u) == 7) {
        c->r[31] = 0x80106CBCu;
        psx::cpu::dispatchGuestToReturn0(*c, 0x801128BCu, psx::cpu::ExecutionBudget::currentTurn(*c), __func__);
        c->r[31] = 0x80106CC4u;
        psx::cpu::dispatchGuestToReturn0(*c, 0x800508A8u, psx::cpu::ExecutionBudget::currentTurn(*c), __func__);
      }
      sm = c->mem_r32(0x1f800138u);
      c->mem_w16(sm + 0x4e, 1);
      c->r[31] = 0x80106CD8u;
      eng(c).fieldFrame();
      break;
    case 6: { // L_80106E5C — zone-change settle + next-area select
      int16_t ev = c->mem_r16s(0x800E7FEEu);
      uint16_t evu = c->mem_r16(0x800E7FEEu);
      if (ev != 0) {
        c->mem_w8(0x800BF880u, 1);
        c->mem_w16(0x1F800194u, evu);
      }
      c->r[31] = 0x80106E88u;
      psx::cpu::dispatchGuestToReturn0(*c, 0x80074BC4u, psx::cpu::ExecutionBudget::currentTurn(*c), __func__);
      c->r[16] = 0x800BF808u;
      c->mem_w8(0x800BF870u, (uint8_t)((c->mem_r16(0x800BF83Au) >> 8) & 31u));
      c->mem_w8(0x800BF871u, (uint8_t)(c->mem_r8(0x800BF83Au) & 63u));
      uint8_t trig = c->mem_r8(0x800BF839u);
      if (trig == 7) {
        c->r[31] = 0x80106ECCu;
        psx::cpu::dispatchGuestToReturn0(*c, 0x80114B90u, psx::cpu::ExecutionBudget::currentTurn(*c), __func__);
        c->mem_w8(0x800BF839u, 3);
        trig = c->mem_r8(0x800BF839u);
      }
      if (trig != 3) {
        sm = c->mem_r32(0x1f800138u);
        c->mem_w16(sm + 0x48, 2);
        uint8_t b = c->mem_r8(0x1F800236u);
        c->mem_w16(sm + 0x4a, 5);
        c->mem_w16(sm + 0x4e, 0);
        c->mem_w16(sm + 0x4c, b);
      } else { // L_80106F0C
        c->r[31] = 0x80106F14u;
        psx::cpu::dispatchGuestToReturn0(*c, 0x8005245Cu, psx::cpu::ExecutionBudget::currentTurn(*c), __func__);
        sm = c->mem_r32(0x1f800138u);
        c->mem_w16(sm + 0x48, 2);
        c->mem_w16(sm + 0x4a, 1);
        c->mem_w16(sm + 0x4c, 1);
        c->mem_w16(sm + 0x4e, 0);
      }
      break;
    }
    case 7: // L_80106F38 — poll 0x80045580(1)
      c->r[4] = 1;
      c->r[31] = 0x80106F40u;
      psx::cpu::dispatchGuestToReturn0(*c, 0x80045580u, psx::cpu::ExecutionBudget::currentTurn(*c), __func__);
      if (c->r[2] == 0) {
        break;
      }
      sm = c->mem_r32(0x1f800138u); // L_80106F48
      c->mem_w16(sm + 0x4e, (uint16_t)(c->mem_r16(sm + 0x4e) + 1));
      break;
    case 8: // L_80106F68 — wait done_flag, arm transition
      if (c->mem_r8(0x1F80019Bu) == 0) {
        break;
      }
      c->mem_w8(0x800BF89Cu, 4);
      c->mem_w8(0x1F800236u, 0);
      c->mem_w8(0x800BF839u, 3);
      c->mem_w16(0x800BF83Au, 0);
      sm = c->mem_r32(0x1f800138u);
      c->mem_w16(sm + 0x4a, 1); // L_80106FAC join (r2 = 1)
      c->mem_w16(sm + 0x4c, 2);
      c->mem_w16(sm + 0x4e, 6);
      break;
    case 9: // L_80106FC4 — field frame + gate on pad bit 3
      c->r[31] = 0x80106FCCu;
      eng(c).fieldFrame();
      if (c->mem_r8(0x800BF89Cu) == 2 && (c->mem_r16(0x800E7E68u) & 8u) != 0) {
        sm = c->mem_r32(0x1f800138u);
        c->mem_w16(sm + 0x4e, (uint16_t)(c->mem_r16(sm + 0x4e) + 1));
        c->mem_w8(0x800BF809u, 1);
        c->mem_w8(sm + 0x6e, 31);
      }
      break;
    case 10: { // L_80107020 — fade-out ramp then CD settle
      c->r[31] = 0x80107028u;
      c->r[16] = 0x1F800000u;
      eng(c).fieldFrame();
      sm = c->mem_r32(0x1f800138u);
      uint32_t u = ((uint32_t)c->mem_r8(sm + 0x6e) * (uint32_t)-8) & 0xFFu;
      c->r[4] = (u << 16) | (u << 8) | u;
      c->r[5] = 0;
      c->r[6] = 0;
      c->r[31] = 0x80107058u;
      psx::cpu::dispatchGuestToReturn0(*c, 0x8007E9C8u, psx::cpu::ExecutionBudget::currentTurn(*c), __func__);
      sm = c->mem_r32(0x1f800138u);
      c->mem_w8(sm + 0x6e, (uint8_t)(c->mem_r8(sm + 0x6e) - 1));
      sm = c->mem_r32(0x1f800138u);
      if (c->mem_r8(sm + 0x6e) == 0) {
        c->mem_w16(sm + 0x4e, 7);
        c->r[31] = 0x80107090u;
        psx::cpu::dispatchGuestToReturn0(*c, 0x8001CF2Cu, psx::cpu::ExecutionBudget::currentTurn(*c), __func__);
      }
      break;
    }
    case 11: // L_80107098 — a0l fade sequencer on the BG node
      c->r[4] = 0x800E8008u;
      c->r[31] = 0x801070A4u;
      psx::cpu::dispatchGuestToReturn0(*c, 0x8010957Cu, psx::cpu::ExecutionBudget::currentTurn(*c), __func__);
      break;
    default:
      break;
    }
  }
  c->r[31] = c->mem_r32(sp + 20);
  c->r[16] = c->mem_r32(sp + 16);
  c->r[29] += 24;
}

void Engine::fieldRun() {
  Core *c = core;
  // fieldRun can yield transitively (case 0's substrate init chain descends
  // into loaders) — plain call, not strict replay check: fieldRunFaithful is not
  // byte-exact yet (12+ diffs at 0x801FE8xx / v0 / v1,
  // scratch/logs/mv_tdd.log), and strict replay check's synchronous compare is wrong
  // across a yield boundary regardless. SBS-gated; fix fieldRunFaithful and
  // re-arm strict replay check once it's byte-exact and known yield-free (or drop this
  // call for a plain one permanently if it always yields).
  uint32_t sm = c->mem_r32(0x1f800138u);
  uint16_t s4e = c->mem_r16(sm + 0x4e);
  switch (s4e) {
  case 0:
    eng(c).pool.init();                  // OWNED native (game/world/pool.cpp) — replaces
                                         // typed runtime address dispatch(0x8007b18c)
    eng(c).pool.resetControlBlock();     // OWNED native (game/world/pool.cpp) —
                                         // replaces typed runtime address dispatch(0x800796dc)
    eng(c).pool.seedAreaObjects();       // OWNED native (game/world/pool.cpp) —
                                         // replaces typed runtime address dispatch(0x800263e8)
    eng(c).placement.placeAreaObjects(); // OWNED native
                                         // (game/world/placement.cpp) —
                                         // replaces typed runtime address dispatch(0x80072a78)
    eng(c).pool.reset75240();            // OWNED native (game/world/pool.cpp) — replaces
                                         // typed runtime address dispatch(0x80075240)
    eng(c).pool.setupViewScroll();       // OWNED native (game/world/pool.cpp) —
                                         // replaces typed runtime address dispatch(0x800783dc)
    eng(c).pool.finalViewInit();         // OWNED native (game/world/pool.cpp) —
                                         // replaces typed runtime address dispatch(0x80078610)
    sm = c->mem_r32(0x1f800138u);
    c->mem_w16(sm + 0x4e, 1);
    c->mem_w8(sm + 0x6b, 0);
    if (c->mem_r8(0x800bf89cu) == 2) {
      c->mem_w16(sm + 0x4e, 9);
    } else if (c->mem_r8(0x800bf870u) == 8) {
      psx::cpu::callGuestNow(*c, __func__, 0x80114b90u);
    }
    // HALFWORD read, matching gen (overlay guest 0x80106B98: `mem_r16((r4 +
    // -1936))`) and the faithful mirror above. It was `mem_r32(...) == 0x15`,
    // which also covers bf872/73 and so was effectively never true — this
    // transition never fired under native_sync. (A first pass "fixed" it to
    // mem_r8 on a bad audit hit; gen settles it: r16.)
    else if (c->mem_r16(0x800bf870u) == 21) {
      c->mem_w16(sm + 0x4e, 0xb);
      return;
    }
    eng(c).pool.selectStateIndex(c->mem_r8(0x800bf870u)); // OWNED native — replaces the guest call 0x80074f24(area)
    break;
  case 2:
    eng(c).gStateMutate(0x800E7E80u,
                        0xC); // native — was typed runtime address dispatch 0x80058304(G, 0xC)
    sm = c->mem_r32(0x1f800138u);
    c->mem_w16(sm + 0x4e, (uint16_t)(c->mem_r16(sm + 0x4e) + 1));
    /* fallthrough */
  case 3:
    eng(c).audioDispatch.settleField(); // native — was typed runtime address dispatch 0x80074BC4
    if (c->mem_r8(0x800bf870u) == 8) {
      psx::cpu::callGuestNow(*c, __func__, 0x80114b90u);
    }
    sm = c->mem_r32(0x1f800138u);
    c->mem_w16(sm + 0x4a, 2);
    c->mem_w16(sm + 0x4c, 0);
    c->mem_w16(sm + 0x4e, 0);
    eng(c).modeStateArm.arm(); // native — was typed runtime address dispatch 0x8005082C(0,0,0)
    break;
  case 4:
    psx::cpu::callGuestNow(*c, __func__, 0x8006c7c4u);
    eng(c).modeStateArm.armFromAreaTable(); // native — was typed runtime address dispatch
                                            // 0x800508A8
    c->mem_w16(c->mem_r32(0x1f800138u) + 0x4e, 1);
    /* fallthrough */
  case 1: {
    eng(c).fieldFrame(); // native field per-frame update (0x80108b0c)
    sm = c->mem_r32(0x1f800138u);
    if (c->mem_r8(0x800bf80du) == 3) { // (signed byte) special mode 3
      if (c->mem_r8(0x800bf80fu) == 0) {
        eng(c).audioDispatch.settleField(); // native — was typed runtime address dispatch
                                            // 0x80074BC4
        sm = c->mem_r32(0x1f800138u);
        if (c->mem_r16s(0x800e7feeu) == 0) { // halfword: guest uses lh here, never lw
          c->mem_w16(sm + 0x4e,
                     (uint16_t)(c->mem_r16(sm + 0x4e) + 1)); // LAB_80106f48
        } else {
          c->mem_w8(0x800bf880u, 1);
          c->mem_w16(0x1f800194u, (uint16_t)c->mem_r16(0x800e7feeu)); // halfword store, per gen
          c->mem_w16(sm + 0x4e, 0);
        }
      }
    } else if (c->mem_r8(0x800bf839u) != 0 && c->mem_r8(0x800bf80fu) == 0) {
      if (c->mem_r8(0x800bf839u) == 8) {
        c->mem_w16(sm + 0x4a, 3);
        c->mem_w16(sm + 0x4c, 0);
        c->mem_w16(sm + 0x4e, 0);
      } else {
        if (c->mem_r8(0x1f800236u) > 4) {
          psx::cpu::callGuestNow(*c, __func__, 0x80050894u, 0); // LAB_80106fac join
        }
        sm = c->mem_r32(0x1f800138u);
        c->mem_w16(sm + 0x4a, 1);
        c->mem_w16(sm + 0x4c, 2);
        c->mem_w16(sm + 0x4e, 6);
      }
    }
    break;
  }
  case 5:
    if (c->mem_r8(0x800bf870u) == 7) {
      psx::cpu::callGuestNow(*c, __func__, 0x801128bcu);
      eng(c).modeStateArm.armFromAreaTable();
    }
    c->mem_w16(c->mem_r32(0x1f800138u) + 0x4e, 1);
    eng(c).fieldFrame();
    break;
  case 6: {
    if (c->mem_r16s(0x800e7feeu) != 0) {
      c->mem_w8(0x800bf880u, 1);
      c->mem_w16(0x1f800194u, (uint16_t)c->mem_r16(0x800e7feeu));
    }
    eng(c).audioDispatch.settleField(); // native — was typed runtime address dispatch 0x80074BC4
    // _DAT_800bf870 = CONCAT11(...) & 0x3f1f  — the decomp's byte-swap-and-mask
    // of *0x800bf83a into bf870
    uint16_t b83a = c->mem_r16(0x800bf83au);
    uint32_t v = (((uint32_t)(b83a & 0xff) << 8) | (b83a >> 8)) & 0x3f1f;
    // TWO BYTE stores, not a word: bf870 is a byte field and bf872/73 are live
    // state a 32-bit store would zero. (Documented in fieldRunFaithful's
    // header; confirmed by the guest only ever using sb here.)
    c->mem_w8(0x800bf870u, (uint8_t)(v & 0xff));
    c->mem_w8(0x800bf871u, (uint8_t)((v >> 8) & 0xff));
    if (c->mem_r8(0x800bf839u) == 7) {
      psx::cpu::callGuestNow(*c, __func__, 0x80114b90u);
      c->mem_w8(0x800bf839u, 3);
    }
    sm = c->mem_r32(0x1f800138u);
    if (c->mem_r8(0x800bf839u) == 3) {
      psx::cpu::callGuestNow(*c, __func__, 0x8005245cu);
      sm = c->mem_r32(0x1f800138u);
      c->mem_w16(sm + 0x48, 2);
      c->mem_w16(sm + 0x4a, 1);
      c->mem_w16(sm + 0x4c, 1);
      c->mem_w16(sm + 0x4e, 0);
    } else {
      c->mem_w16(sm + 0x48, 2);
      uint8_t b = c->mem_r8(0x1f800236u);
      c->mem_w16(sm + 0x4a, 5);
      c->mem_w16(sm + 0x4e, 0);
      c->mem_w16(sm + 0x4c, b);
    }
    break;
  }
  case 7: {
    psx::cpu::callGuestNow(*c, __func__, 0x80045580u, 1);
    if (c->r[2] == 0) {
      return;
    }
    sm = c->mem_r32(0x1f800138u);
    c->mem_w16(sm + 0x4e,
               (uint16_t)(c->mem_r16(sm + 0x4e) + 1)); // goto LAB_80106f48
    break;
  }
  case 8:
    if (c->mem_r8(0x1f80019bu) == 0) {
      return;
    }
    c->mem_w8(0x800bf89cu, 4);
    c->mem_w8(0x1f800236u, 0);
    c->mem_w8(0x800bf839u, 3);
    c->mem_w16(0x800bf83au, 0);
    sm = c->mem_r32(0x1f800138u);
    c->mem_w16(sm + 0x4a, 1);
    c->mem_w16(sm + 0x4c, 2);
    c->mem_w16(sm + 0x4e, 6); // LAB_80106fac
    break;
  case 9:
    eng(c).fieldFrame();
    sm = c->mem_r32(0x1f800138u);
    if (c->mem_r8(0x800bf89cu) == 2 && (c->mem_r16(0x800e7e68u) & 8) != 0) {
      c->mem_w16(sm + 0x4e, (uint16_t)(c->mem_r16(sm + 0x4e) + 1));
      c->mem_w8(0x800bf809u, 1);
      c->mem_w8(sm + 0x6e, 0x1f);
    }
    break;
  case 10: {
    eng(c).fieldFrame();
    sm = c->mem_r32(0x1f800138u);
    uint32_t u = ((uint32_t)c->mem_r8(sm + 0x6e) * (uint32_t)-8) & 0xff;
    cfg_logf("fadesites", "[fadesite] fieldRun-case10 u=%02x sm6e=%u", u, c->mem_r8(sm + 0x6e));
    fade(c).applyLeafCall((u << 16) | (u << 8) | u,
                          0); // = guest FUN_8007e9c8(color, 0, 4):
                              // area-transition subtractive fade-out ramp
    uint8_t nv = (uint8_t)(c->mem_r8(sm + 0x6e) - 1);
    c->mem_w8(sm + 0x6e, nv);
    if (nv == 0) {
      c->mem_w16(sm + 0x4e, 7);
      psx::cpu::callGuestNow(*c, __func__, 0x8001cf2cu);
      // NOTE: no persistent hold_black on fade-out exit — see sop.cpp state 3
      // note (regression 2026-07-01).
    }
    break;
  }
  case 0xb:
    fade(c).sequence(0x800e8008u); // OWNED native — replaces the guest call 0x8010957c(
                                   // node) (a0l fade sequencer)
    break;
  default:
    break;
  }
}

// FIELD PER-FRAME UPDATE VARIANT 0x80108be4 — the mid-TRANSITION field frame,
// used by the state-3 running sub-machine while the screen is faded for an area
// change. Lighter than ov_field_frame (0x80108b0c): a reduced gameplay-update
// set (the state-transition object update 0x8007b04c instead of the full
// Tomba+object walk 0x8007a904) then the SAME render-submit 0x8010810c.
// Faithful to disasm. Owned so the transition path is native+traceable (the
// door freeze lives below here): the heavy callees stay typed runtime address dispatch leaves to
// descend into next — esp. 0x8007b04c (the per-object update that must, but
// currently does not, tick the screen-transition sequencer FUN_80026ad0 to
// completion). pc_faithful mirror of overlay guest 0x80108BE4
// (authenticated executable/overlay evidence). Reduced twin of fieldFrameFaithful
// (0x80108B0C): same frame descent/spill/jal-site-ra shape, 9 gameplay-update
// calls (drops sceneStateStep 0x80050de4 and areaModeDispatch 0x8001cac0, which
// the full variant has and this one does not per gen), render orchestrator
// dispatched as mRender->frameX() (0x8003FA44) under the <2 gate, then
// submitPage810c/postRenderTick/areaSlots.updateTail tail — all with jal-site
// ras matching the gen's constants so any nested unowned leaf's own frame-spill
// stays byte-exact with core B.
void Engine::fieldFrameXFaithful() {
  Core *c = core;
  c->r[29] -= 24;
  const uint32_t sp = c->r[29];
  c->mem_w32(sp + 16, c->r[16]);
  c->r[16] = 0x1F800000u;
  c->mem_w32(sp + 20, c->r[31]);
  c->mem_w16(0x1f80017cu,
             (uint16_t)(c->mem_r16(0x1f80017cu) + 1)); // frame counter
  c->mem_w32(0x800bf878u, c->mem_r32(0x800bf878u) + 1);
  if (c->mem_r8(0x1f800136u) == 0) { // not paused: reduced gameplay update
    c->r[31] = 0x80108C28u;
    eng(c).frameStartTick();
    c->r[31] = 0x80108C30u;
    eng(c).objectList.walkAux();
    c->r[31] = 0x80108C38u;
    eng(c).array8Dispatch.tick();
    c->r[31] = 0x80108C40u;
    eng(c).transitionState3.walkOnce();
    c->r[31] = 0x80108C48u;
    eng(c).sceneEventFifo();
    c->r[31] = 0x80108C50u;
    eng(c).sceneRenderListBuilder();
    c->r[31] = 0x80108C58u;
    eng(c).objectTable.dispatch();
    c->r[31] = 0x80108C60u;
    eng(c).modePerFrameDispatch();
    c->r[31] = 0x80108C68u;
    CutsceneCamera(c, CutsceneCamera::CAM_OBJ).update();
  }
  if (c->mem_r8(0x1f800136u) < 2) {
    c->r[31] = 0x80108C84u;
    rend(c)->frameX();
  } // 0x8003FA44 underneath
  c->r[31] = 0x80108C8Cu;
  eng(c).submitPage810c();
  c->r[31] = 0x80108C94u;
  eng(c).postRenderTick();
  c->r[31] = 0x80108C9Cu;
  eng(c).areaSlots.updateTail();
  c->r[31] = c->mem_r32(sp + 20);
  c->r[16] = c->mem_r32(sp + 16);
  c->r[29] += 24;
}

void Engine::fieldFrameX() {
  Core *c = core;
  c->mem_w16(0x1f80017cu,
             (uint16_t)(c->mem_r16(0x1f80017cu) + 1)); // frame counter
  c->mem_w32(0x800bf878u, c->mem_r32(0x800bf878u) + 1);
  if (c->mem_r8(0x1f800136u) == 0) { // not paused: reduced gameplay update
    eng(c).frameStartTick();
    eng(c).objectList.walkAux();
    psx::cpu::callGuestNow(*c, __func__, 0x80026368u);
    eng(c).transitionState3.walkOnce(); // 0x80059d28/0x80069b28/0x8007b04c NATIVE
    eng(c).sceneEventFifo();
    eng(c).sceneRenderListBuilder();
    eng(c).objectTable.dispatch();
    eng(c).modePerFrameDispatch();                       // 25588/4fe84/26c88/22a80 NATIVE
    CutsceneCamera(c, CutsceneCamera::CAM_OBJ).update(); // 0x8006ec44 NATIVE (CutsceneCamera::update)
  }
  if (c->mem_r8(0x1f800136u) < 2) {
    rend(c)->frameX(); // 0x8003fa44 — NATIVE render orchestrator twin
  }
  eng(c).submitPage810c();       // render submit (page-1 dim-fade owned; other pages
                                 // guest instruction path)
  eng(c).postRenderTick();       // 0x80077D8C NATIVE (was d0)
  eng(c).areaSlots.updateTail(); // 0x80075a80 NATIVE
}

// FIELD RUNNING sub-machine VARIANT 0x801070b4 (sm[0x4c]==3, the mid-transition
// running handler reached when a door/edge sets up an area change). A switch on
// sm[0x4e]: state 0 = init (scene reset + input reset) then fall into state 1;
// state 1 = run ov_field_frame_x and check the mode-3 / area-change exit
// conditions to hand back to the normal running handler (sm[0x4c]=2); state 2 =
// bump to 1. Faithful to the disasm (hand-decompiled from the field overlay).
// pc_faithful mirror of overlay guest 0x801070B4 (mid-transition running
// sub-machine, sm[0x4c]==3). Guest frame descent (24, ra spilled at sp+16) +
// r31 set to the exact gen jal-site constant before every dispatch/native-call
// boundary, so every callee's own frame push lands with the right ra at the
// right (correctly-descended) address. Store shape/branch conditions are
// unchanged from the existing native_sync body -- only frame/ra discipline was
// missing.
void Engine::fieldRunXFaithful() {
  Core *c = core;
  c->r[29] -= 24;
  const uint32_t sp = c->r[29];
  c->mem_w32(sp + 16, c->r[31]);

  uint32_t sm = c->mem_r32(0x1f800138u);
  uint16_t s4e = c->mem_r16(sm + 0x4e);

  if (s4e >= 2) { // L_801070EC
    if (s4e == 2) {
      c->mem_w16(sm + 0x4e, 1); // L_8010721C: re-arm to running
    }
  } else {
    if (s4e == 0) { // L_80107100: init
      c->r[31] = 0x80107108u;
      c->mem_w16(sm + 0x4e, 1);
      psx::cpu::dispatchGuestToReturn0(*c, 0x8006c77cu, psx::cpu::ExecutionBudget::currentTurn(*c), __func__);
      c->r[4] = 0;
      c->r[5] = 0;
      c->r[6] = 0;
      c->r[31] = 0x80107118u;
      psx::cpu::dispatchGuestToReturn0(
          *c, 0x8005082cu, psx::cpu::ExecutionBudget::currentTurn(*c), __func__); // input reset
      // fall through to state 1 (L_80107118)
    }

    c->r[31] = 0x80107120u;
    eng(c).fieldFrameX(); // overlay guest 0x80108BE4 -- native owner

    if (c->mem_r8(0x800bf80du) == 3) { // mode-3 exit (0x80107138)
      if (c->mem_r8(0x800bf80fu) == 0) {
        c->r[31] = 0x80107150u;
        eng(c).audioDispatch.settleField(); // native owner -- was typed runtime address dispatch
                                            // 0x80074BC4
        sm = c->mem_r32(0x1f800138u);
        c->mem_w16(sm + 0x4c, 2); // back to normal running handler
        int16_t e_s = c->mem_r16s(0x800e7feeu);
        uint16_t e_u = c->mem_r16(0x800e7feeu);
        if (e_s != 0) {
          c->mem_w8(0x800bf880u, 1);
          c->mem_w16(0x1f800194u, e_u);
          c->mem_w16(sm + 0x4e, 0);
        } else {
          c->mem_w16(sm + 0x4e, 2);
        }
      }
    } else { // L_80107194: area-change request via bf839
      uint8_t bf839 = c->mem_r8(0x800bf839u);
      if (bf839 != 0 && c->mem_r8(0x800bf80fu) == 0) {
        if (bf839 == 8) { // 0x801071bc
          sm = c->mem_r32(0x1f800138u);
          c->mem_w16(sm + 0x4a, 1);
          c->mem_w16(sm + 0x4c, 2);
          c->mem_w16(sm + 0x4e, 3);
        } else {
          if (c->mem_r8(0x1f800236u) >= 5) { // 0x801071f0
            c->r[31] = 0x801071f8u;
            c->r[4] = 0;
            psx::cpu::dispatchGuestToReturn0(*c, 0x80050894u, psx::cpu::ExecutionBudget::currentTurn(*c), __func__);
          }
          sm = c->mem_r32(0x1f800138u);
          c->mem_w16(sm + 0x4a, 1);
          c->mem_w16(sm + 0x4c, 2);
          c->mem_w16(sm + 0x4e, 6);
        }
      }
    }
  }

  c->r[31] = c->mem_r32(sp + 16);
  c->r[29] += 24;
}

void Engine::fieldRunX() {
  Core *c = core;
  uint32_t sm = c->mem_r32(0x1f800138u);
  uint16_t s4e = c->mem_r16(sm + 0x4e);
  if (s4e >= 2) {
    if (s4e == 2) {
      c->mem_w16(sm + 0x4e, 1); // 0x8010721c: re-arm to running
    }
    return;
  }
  if (s4e == 0) { // 0x80107100: init
    c->mem_w16(sm + 0x4e, 1);
    // 0x8006c77c — the per-area transition-ENTER hook (every area's, 0..21).
    // Its cooperative FUN_80044BD4 spawn-and-wait completes on this leg too:
    // PcScheduler::spawnAndWait runs the spawned body inline when the waiting
    // task cannot suspend (kanban #50).
    c->r[31] = 0x80107108u;
    psx::cpu::dispatchGuestToReturn0(*c, 0x8006c77cu, psx::cpu::ExecutionBudget::currentTurn(*c), __func__);
    psx::cpu::callGuestNow(*c, __func__, 0x8005082cu, 0, 0, 0); // input reset
    // fall through to state 1
  }
  eng(c).fieldFrameX();              // 0x80108be4 per-frame (state 1, 0x80107118)
  if (c->mem_r8(0x800bf80du) == 3) { // mode-3 exit (0x80107138)
    if (c->mem_r8(0x800bf80fu) != 0) {
      return;
    }
    eng(c).audioDispatch.settleField(); // native — was typed runtime address dispatch 0x80074BC4
    sm = c->mem_r32(0x1f800138u);
    c->mem_w16(sm + 0x4c, 2); // back to normal running handler
    int16_t e_s = c->mem_r16s(0x800e7feeu);
    uint16_t e_u = c->mem_r16(0x800e7feeu);
    if (e_s != 0) {
      c->mem_w8(0x800bf880u, 1);
      c->mem_w16(0x1f800194u, e_u); // sh (16-bit per disasm)
      c->mem_w16(sm + 0x4e, 0);
    } else {
      c->mem_w16(sm + 0x4e, 2);
    }
    return;
  }
  // not mode-3 (0x80107194): area-change request via bf839
  uint8_t bf839 = c->mem_r8(0x800bf839u);
  if (bf839 == 0) {
    return;
  }
  if (c->mem_r8(0x800bf80fu) != 0) {
    return;
  }
  if (bf839 == 8) { // 0x801071bc
    sm = c->mem_r32(0x1f800138u);
    c->mem_w16(sm + 0x4a, 1);
    c->mem_w16(sm + 0x4c, 2);
    c->mem_w16(sm + 0x4e, 3);
    return;
  }
  if (c->mem_r8(0x1f800236u) >= 5) {
    psx::cpu::callGuestNow(*c, __func__, 0x80050894u, 0); // 0x801071f0
  }
  sm = c->mem_r32(0x1f800138u);
  c->mem_w16(sm + 0x4a, 1);
  c->mem_w16(sm + 0x4c, 2);
  c->mem_w16(sm + 0x4e, 6);
}

void Engine::submode1Case0Native() {
  Core *c = core;
  psx::cpu::dispatchGuestToReturn0(*c, 0x8005245cu, psx::cpu::ExecutionBudget::currentTurn(*c), __func__);
  // The owned load is synchronous: retain FUN_80044BD4's flag-2 RNG stamp, then
  // continue without manufacturing a wait frame or loading-screen service.
  c->game->pcSched.completeSyncWait(c->mem_r32(0x1f800138u), /*flag=*/2);
  sop.transitionAreaLoad();
}

void Engine::submode1() {
  Core *c = core;
  uint32_t sm = c->mem_r32(0x1f800138u);
  uint16_t s4c = c->mem_r16(sm + 0x4c);
  if (s4c <= 1) {
    cfg_logf("stage",
             "submode1 case %u: bf870=%u nexttab[bf870]=%u",
             s4c,
             c->mem_r8(0x800bf870u),
             c->mem_r8(0x80108f60u + c->mem_r8(0x800bf870u)));
  }
  switch (s4c) {
  case 0:
    submode1Case0Native();
    /* fallthrough */
  case 1: {
    c->mem_w8(0x1f800234u, 0);
    uint8_t area = c->mem_r8(0x800bf870u);
    uint8_t next = c->mem_r8(0x80108f60u + area);
    c->mem_w16(sm + 0x4c, next);
    break;
  }
  case 2:
    eng(c).fieldRun();
    break; // field RUNNING sub-machine (sm[0x4e]) — native
  case 3:
    eng(c).fieldRunX();
    break; // mid-transition running sub-machine 0x801070b4 — native
  case 4:
    psx::cpu::dispatchGuestToReturn0(*c, 0x80107230u, psx::cpu::ExecutionBudget::currentTurn(*c), __func__);
    break;
  case 5:
    psx::cpu::dispatchGuestToReturn0(*c, 0x8010766cu, psx::cpu::ExecutionBudget::currentTurn(*c), __func__);
    break;
  case 6:
    psx::cpu::dispatchGuestToReturn0(*c, 0x80107790u, psx::cpu::ExecutionBudget::currentTurn(*c), __func__);
    break;
  default:
    break; // >=7: no-op
  }
}

// One native loop iteration of the guest body 0x801063F4: dispatch sm[0x48]
// handler, bump frame counter. Returns 1 if handled natively, 0 if the current
// state is NOT yet owned and the task must hand back to the cooperative guest
// loop. OWNED so far: sm[0x48] area-init (0/1) + the RUNNING SOP-intro path
// (sm[0x48]==2, sm[0x4a]==0, SOP loaded). The transition sub-modes
// (sm[0x4a]!=0), the area machine (sm[0x4c] 0x80106478), and the non-SOP field
// overlays YIELD DEEP and aren't owned yet — own them (RE in
// scratch/gameplay_start_flow_re.md) to extend native ownership and shrink the
// cooperative fallback. Returning 0 keeps the field REACHABLE (no derail) until
// those are owned.
