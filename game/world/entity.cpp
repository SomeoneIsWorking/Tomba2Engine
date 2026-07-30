// game/world/entity.cpp — PC-native per-object ENTITY STATE-MACHINE subsystem.
// The per-object behavior cluster that drives each entity's logic: the child-node spawn / sub-object
// builder (FUN_80040410), the per-object dispatcher loop over the 0x800ec188 table (FUN_80026C88), and
// the oscillate / frame-toggle sub-behavior (FUN_8003FD10). The placed-prop state-machine head that
// drives them now lives in game/ai/placed_prop_sm.cpp (PlacedPropSm::step).
// Control flow + object memory owned native; the per-state sub-behaviors stay reachable by address via
// rec_dispatch (each honors its own override identically). NO GTE, NO render packets. Extracted verbatim
// from game_tomba2.cpp (one behavior, byte-identical) into its own module for PC-game code structure.
// Diagnostic A/B gates (child40410/disp26c88/sm40558/fd10) are REPL channels, unchanged.
#include "core.h"
#include "game_ctx.h"
#include "object/actor.h"    // Actor::boundsCull (FUN_8007778C)
#include "cfg.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "entity.h"
#include "spawn.h"     // class Spawn (eng(c).spawn.despawn / dispatch / spawnAndInit)
#include "graphics_bind.h"   // ov_obj_render_update
#include "rng.h"       // class Rng (via rngOf(c).next())
#include "render/cull.h"     // class Cull (eng(c).cull.enqueueQueueA — FUN_80077E7C)
void rec_super_call(Core*, uint32_t);
void rec_dispatch(Core*, uint32_t);

// (removed 55 lines: child_spawn_40410 — sm40558 was its ONLY caller, so it died with it. 0x80040410 is
//  reached from the new port through its generated func_80040410 wrapper.)


// (removed 231 lines: the hand-transliterated sm40558 draft for 0x80040558 — REPLACED by PlacedPropSm::step
//  (game/ai/placed_prop_sm.cpp). It had five defects, one a MISSING GUEST WRITE of node[95]=0
//  on every state-1 exit. Do not resurrect it.)


// FUN_8003FD10 — per-object OSCILLATE / FRAME-TOGGLE sub-behavior (PlacedPropSm STATE-1's node[5] jump-table
// handlers JT1[0], reached ~thousands×/run from the hot active-behavior path). a0 = obj. NO GTE, NO render
// packets — pure object/scratchpad memory ops + ONE dispatched callee (0x8009A450 = ov_rand, owned). A
// 3-way micro state-machine on the phase byte obj[6]:
//   obj[6]==0 (@fd40): if obj[43]==0 return; else obj[6]=1, obj[43]=0, obj[64](sh)=16, return.
//   obj[6]==1 (@fd64): if obj[43]!=0 { obj[43]=0; obj[64](sh)=16; }  @fd7c: v0=obj[64](lhu); v0--; obj[64]=v0;
//     if (int16)v0 == -1 obj[6] += -1 (i.e. obj[6]--);  @fdb0: r = ((u16*)0x1F80017C & 1); node=*(obj+0xC0);
//     node[2](sh) = r*6; rr = ov_rand(); node[0](sh) = ((rr&3)-2)*6; return.
//   obj[6] other (@fdf0): return.
// GOTCHAs: (1) the `sh v1,2(node)` at 0x8003fdd0 is in the ov_rand jal DELAY SLOT — node and v1 (=r*6) are
//   computed BEFORE the call, the store happens with the pre-call values (node loaded @0x8003fdc4). (2) the
//   obj[6]-- at @fdac uses v1=-1 added to obj[6] (only on the v0==-1 branch). (3) node[2]/[0] are halfword
//   stores of v0*6 == (v0*3)<<1. `fd10` gate = full RAM+scratchpad A/B vs rec_super_call (same family
//   rationale as sm40558: the dispatched ov_rand runs in BOTH passes + this fn's 24-byte frame is dead below
//   entry sp on return -> exclude [sp-0x800, sp)).
static void osc_fd10(Core* c) {
  const uint32_t obj = c->r[4];
  uint8_t p6 = c->mem_r8(obj + 6);
  if (p6 == 0) {                                  // @fd40
    if (c->mem_r8(obj + 43) == 0) return;         // @fdf0
    c->mem_w8 (obj + 6, 1);
    c->mem_w16(obj + 64, 16);
    c->mem_w8 (obj + 43, 0);
    return;
  }
  if (p6 != 1) return;                            // @fdf0
  // @fd64
  if (c->mem_r8(obj + 43) != 0) {
    c->mem_w8 (obj + 43, 0);
    c->mem_w16(obj + 64, 16);
  }
  // @fd7c
  uint16_t cnt = c->mem_r16(obj + 64);
  cnt = (uint16_t)(cnt - 1);
  c->mem_w16(obj + 64, cnt);
  if ((int16_t)cnt == -1) {                       // @fda4 (obj[6] += -1)
    c->mem_w8(obj + 6, (uint8_t)(c->mem_r8(obj + 6) - 1));
  }
  // @fdb0
  uint32_t r = c->mem_r16(0x1F80017Cu) & 1u;      // scratchpad halfword & 1
  uint32_t node = c->mem_r32(obj + 0xC0);
  c->mem_w16(node + 2, (uint16_t)(r * 6u));       // sh in the ov_rand delay slot (pre-call node/value)
  uint32_t rr = (uint32_t)rngOf(c).next() & 3u;   // FUN_8009A450 -> native class Rng
  uint32_t v0 = (uint32_t)((int32_t)rr - 2);
  c->mem_w16(node + 0, (uint16_t)(v0 * 6u));
}

// FUN_8007a904 — the engine's PER-FRAME ENTITY-LIST WALK (the native entity manager / object driver).
// Each frame the engine walks the two live object lists and runs every node's handler. This is the
// top of the per-object spine: the driver that touches every active game object, so owning it puts the
// engine — not the recompiled body — in charge of iterating the world's objects (the foundation for
// PC-owned per-object render classification, issue #4). Pure list traversal; the per-type handlers stay
// reachable by address via rec_dispatch (each honors its own owned override identically).
//
// RE'd verbatim from disas 0x8007a904 (two identical loops, list0 head 0x800FB168 then list1 head
// 0x800F2624 — the SAME (head) vars the spawn primitive links into, spawn.cpp LIST_HEAD[0]/[1]):
//   for (n = *head; n != 0; n = next) {
//     next        = u32[n + 0x24];   // NEXT captured BEFORE the handler runs (a handler may relink/free n;
//                                    //   `next` is a callee-saved reg in the recomp body, so it survives)
//     handler     = u32[n + 0x1C];   // per-type update/render fn pointer
//     u8[n + 1]   = 0;               // clear the per-frame render flag (jalr delay slot — before the call)
//     handler(n);                    // a0 = n
//   }
// NB only TWO lists are walked here (list0 then list1); the third pool/list 0x800F2738 is not driven by
// this function. `walkverify` gate = full main-RAM + scratchpad A/B vs rec_super_call(0x8007a904).
static void entity_walk_7a904(Core* c) {
  static const uint32_t HEAD[2] = { 0x800FB168u, 0x800F2624u };
  for (int L = 0; L < 2; L++) {
    uint32_t n = c->mem_r32(HEAD[L]);
    while (n) {
      uint32_t next    = c->mem_r32(n + 0x24);   // capture next FIRST (handler may unlink/free n)
      uint32_t handler = c->mem_r32(n + 0x1C);
      c->mem_w8(n + 1, 0);                        // clear per-frame render flag (delay slot of the call)
      c->r[4] = n;                                // a0 = node
      rec_dispatch(c, handler);                   // run the per-type handler (stays PSX / owned override)
      n = next;
    }
  }
}
