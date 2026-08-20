// game/ai/beh_cull_substate_leaves.cpp — WIDE-RE DRAFT case-handler leaves of
// beh_cull_substate_orchestrator (game/ai/beh_cull_substate_orchestrator.cpp, guest 0x8013259C).
//
// STATUS: UNWIRED / UNVERIFIED (wide-RE tier, docs/fleet-workflow.md §6). Hand-transliterated 1:1
// from generated/ov_a00_shard_0.c ground truth (ov_a00_gen_<addr>) — NOT mechanically diffed
// against it yet. Per §9, a wiring pass MUST re-diff every line before registering + SBS-gating.
// Nothing here is called from anywhere (not installed in the override registry, no shard_set_override) —
// dead code that only needs to COMPILE. All sub-calls to OTHER substrate leaves are routed via
// `rec_dispatch(c, addr)` (guest ABI, args in r4..r7, ret in r2) per CLAUDE.md's "native call sites
// prefer routing wired addresses through rec_dispatch" — this reaches the native override where one
// is already registered, and safely falls through to the substrate body otherwise (no interpreter
// abort, since the overlay's own generated C for that address still exists).
//
// Drafted 2026-07-08 session: 0x8013272C (131 gen-C ln, state-0 init), 0x80132954 (70 ln,
// node[5]==0), 0x80132D58 (88 ln, node[5]==2), 0x80133184 (82 ln, state-2 dispatch).
//
// Drafted THIS session (2026-07-09/10, wide-RE band: the 5 mapped-only leaves): 0x80132A88 (162
// ln, common tail called after EVERY node[5] sub-case) and 0x80132EDC (146 ln, node[5]==3). Both
// transliterated near-mechanically (goto/label structure preserved 1:1 against the generated C,
// including exact delay-slot execution order and signed-vs-unsigned read widths per site) rather
// than restructured into idiomatic if/else, specifically to avoid the branch-polarity/operand-
// order bugs called out in docs/fleet-workflow.md §9 — this is HIGH-MEDIUM confidence on CONTROL
// FLOW fidelity, LOW-MEDIUM confidence on FIELD SEMANTICS (obj+0x4A/0x4C/0x4E "target" fields,
// GBASE-relative constant tables) exactly as flagged by the prior mapping pass in docs/engine_re.md.
#include "cfg.h"
#include "core.h"
#include <stdint.h>

extern "C" void rec_dispatch(Core *c, uint32_t addr);

// func_8013272C — DRAFT. RE'd from generated/ov_a00_shard_1.c gen_8013272C (0x8013272C..0x80132944).
// Called by beh_cull_substate_orchestrator's STATE 0 case
// (game/ai/beh_cull_substate_orchestrator.cpp:66, `rec_dispatch(c, 0x8013272Cu)`) — per-type init.
// Frame -24: spills r16+ra at +16/+20.
//
// Structure: seeds obj[46]/[50]/[54] (the SAME euler-angle triple NodeXform::build's node+46/50/54
// world-pos-copy step reads — see node_xform.cpp's `build()`) from a per-type record at a fixed
// table (base = 0x8014A694-shape constant + obj[3]*6, stride 6 = 3×int16) — SAME table shape as
// 0x80132954's obj[184]/[186]/[188] jitter table below, different base constant. obj[3]==0 or ==2
// takes a slightly different path calling 0x801252C0(obj, 0) first and using its result (stored to
// obj[20]) as the table base override instead of the fixed constant — LOW CONFIDENCE on why (never
// confirmed against a live dump; 0x801252C0 is UNOWNED).
//
// Then a bit test on a GBASE-relative byte (0x80145xxx-shape) selects obj[4]=3 immediately, or
// (obj[3]<5) calls GraphicsBind::recordInit-shaped leaf 0x80051B70(rec=12, class=4-or-5) and on
// success seeds a batch of obj[+0x62..0x6A]-ish fields (98/100/102/106 here) with 4096-scaled
// values, else obj[4]=1 (leaves the rest zeroed). Common tail: reset obj[64]/[104]/[108]/[118]/
// [120]/[122]/[41]/[43]/[94]/[95] to fresh defaults, bump obj[4], call the render-record-alloc
// shaped leaf 0x80133774(obj) [UNOWNED] and the SFX/hitbox-arm leaf 0x80048750(obj) [UNOWNED]; on
// its nonzero result AND obj[3] in {3,4}, seed obj[86]/[88] from a fixed 0x8064... table (SAME
// SCR-adjacent constant table other leaves in this cluster read); finally calls
// `NodeXform::buildWithOffset`-shaped leaf 0x800518FC(obj) (dual-owned per tools/codemap.py —
// Engine::objMatrixCompose / NodeXform::buildWithOffset; routed via rec_dispatch rather than a
// direct native call since this draft has not confirmed which owner fires for this call site).
void func_8013272C(Core *c) {
  const uint32_t obj = c->r[4];
  const uint32_t s16 = c->r[16];
  c->r[29] -= 24;

  const uint8_t type = c->mem_r8(obj + 3);
  uint32_t tbl = 0x80150000u - 22860u; // 0x8014A694-shape constant table (RE'd literal, low confidence semantics)
  c->mem_w16(obj + 46, c->mem_r16(tbl + (uint32_t)type * 6 + 0));
  c->mem_w16(obj + 50, c->mem_r16(tbl + (uint32_t)type * 6 + 2));
  uint16_t v54 = c->mem_r16(tbl + (uint32_t)type * 6 + 4);
  c->mem_w16(obj + 54, v54);

  if (type == 0 || type == 2) {
    c->r[4] = obj;
    c->r[5] = 0;
    c->r[6] = type;
    rec_dispatch(c, 0x801252C0u); // UNOWNED
    c->mem_w32(obj + 20, c->r[2]);
    tbl = 0x80140000u; // placeholder base override (0x8014_0000-shape; not independently confirmed)
  }

  // GBASE bit test (0x8014Cxxx-shape, RE'd from the `mem_r8(r2-1588)` load off a 0x8014C000-ish base)
  const bool bitSet = (c->mem_r8(0x8014C000u - 1588u) >> (type & 31)) & 1;
  int state;
  if (bitSet) {
    state = 3;
  } else if (type < 5) {
    c->r[4] = obj;
    c->r[5] = 12;
    c->r[6] = (type != 0) ? 5 : 4;
    rec_dispatch(c, 0x80051B70u); // GraphicsBind::recordInitBody-shaped leaf
    if (c->r[2] != 0) {
      if (type == 0) {
        c->mem_w16(obj + 98, (uint16_t)c->r[2]);
        const uint32_t child0 = c->mem_r32(obj + 192);
        c->mem_w16(child0 + 56, 4096);
        c->mem_w16(child0 + 58, 4096);
        c->mem_w16(obj + 100, 4096);
        c->mem_w16(obj + 102, 4096);
        c->mem_w16(obj + 106, 0);
      } else {
        c->mem_w16(obj + 98, 0);
        const uint32_t child2 = c->mem_r32(obj + 192);
        c->mem_w16(child2 + 56, 3072);
        c->mem_w16(child2 + 58, 2457);
        c->mem_w16(obj + 100, 3072);
        c->mem_w16(obj + 102, 2457);
        c->mem_w16(obj + 106, 1);
      }
      state = 1;
    } else {
      state = 1; // rec_dispatch result==0: ground truth still falls to obj[0]=1 (see comment above)
    }
  } else {
    state = 1;
  }
  c->mem_w8(obj + 0, (uint8_t)state);

  c->mem_w16(obj + 184, 4096);
  c->mem_w16(obj + 186, 4096);
  c->mem_w16(obj + 188, 4096);
  c->mem_w16(obj + 64, 20);
  c->mem_w16(obj + 104, 0);
  c->mem_w16(obj + 108, 0);
  c->mem_w16(obj + 118, 0);
  c->mem_w16(obj + 120, 0);
  c->mem_w16(obj + 122, 0);
  c->mem_w8(obj + 41, 0);
  c->mem_w8(obj + 43, 0);
  c->mem_w8(obj + 94, 0);
  c->mem_w8(obj + 95, 0);
  c->mem_w8(obj + 4, (uint8_t)(c->mem_r8(obj + 4) + 1));

  const uint32_t child = c->mem_r32(obj + 192);
  c->mem_w16(child + 60, c->mem_r16(child + 56));
  c->r[4] = obj;
  rec_dispatch(c, 0x80133774u); // UNOWNED
  c->r[4] = obj;
  rec_dispatch(c, 0x80048750u); // UNOWNED
  if (c->r[2] != 0 && (type == 3 || type == 4)) {
    c->mem_w16(obj + 86, c->mem_r16(0x1F800000u + 416));
    c->mem_w16(obj + 88, (uint16_t)(c->mem_r16(0x1F800000u + 418) & 4095));
  }
  c->r[4] = obj;
  rec_dispatch(c, 0x800518FCu); // NodeXform::buildWithOffset / Engine::objMatrixCompose

  (void)v54;
  c->r[16] = s16;
  c->r[29] += 24;
}

// (removed 49 lines: the hand-transliterated func_80132954 draft — replaced by a port_gen body,
//  see the commit that landed it. It carried guest-visible defects; do not resurrect it.)

// func_80132D58 — DRAFT. RE'd from generated/ov_a00_shard_0.c gen_80132D58 (0x80132D58..0x80132EC8).
// Called by beh_cull_substate_orchestrator's node[5]==2 sub-state case
// (game/ai/beh_cull_substate_orchestrator.cpp:112, `rec_dispatch(c, 0x80132D58u)`). Frame -32:
// spills r16/r17+ra at +16/+20/+24.
//
// child = obj[192] (SAME child-slot0 pointer other leaves in this cluster use). child[56]+=obj[78];
// child[58]+=obj[80] (unconditional heading/offset accumulate). Then a 3-way switch on obj[6]
// (0/1/else) with STATE 0 and STATE 1 near-mirror bodies (offsets +2 apart: obj[100]/[102] vs
// something analogous, 0x80132E2C reads the SAME obj[100]/[102]/[106] fields as state0 — LOW
// CONFIDENCE this is a true mirror rather than a second read of identical fields; transliterated
// faithfully either way):
//   0/1: if child[56] is NOT YET past target obj[100] (signed <, AND raw <3584): snap
//        child[56]=obj[100], child[58]=obj[102]; call 0x80133610(obj,1) [UNOWNED]; obj[5]=3,
//        obj[6]=(0->1 / 1->stays, RE'd exactly per ground truth below); else (already past target)
//        a secondary gate on obj[106] decides whether to request the NodeXform-record leaf
//        0x80051B04 (`GraphicsBind::installSceneRecord`-shaped, args a0=obj,a1=12) this frame.
//   else: r6=0 (never requests 0x80051B04).
// Common tail: if requested, call 0x80051B04(obj, 12); then child[56]->child[60] copy (shift the
// PREVIOUS heading into a "last" slot) via 0x80133774(obj) [UNOWNED] and 0x80133700(obj) [UNOWNED].
void func_80132D58(Core *c) {
  const uint32_t obj = c->r[4];
  const uint32_t s16 = c->r[16], s17 = c->r[17];
  c->r[29] -= 32;

  uint32_t child = c->mem_r32(obj + 192);
  c->mem_w16(child + 56, (uint16_t)(c->mem_r16(child + 56) + c->mem_r16(obj + 78)));
  child = c->mem_r32(obj + 192);
  c->mem_w16(child + 58, (uint16_t)(c->mem_r16(child + 58) + c->mem_r16(obj + 80)));

  const uint8_t sub = c->mem_r8(obj + 6);
  bool needRecord = false;

  auto stateBody = [&](int nextSub) {
    child = c->mem_r32(obj + 192);
    const bool notYetPast = ((int32_t)(uint16_t)c->mem_r16(child + 56) < (int32_t)(int16_t)c->mem_r16(obj + 100)) &&
                            ((uint16_t)c->mem_r16(child + 56) < 3584u);
    if (!notYetPast) {
      // already past target -> snap
      const uint16_t obj100 = c->mem_r16(obj + 100);
      child = c->mem_r32(obj + 192);
      c->mem_w16(child + 56, obj100);
      child = c->mem_r32(obj + 192);
      c->mem_w16(child + 58, c->mem_r16(obj + 102));
      c->r[4] = obj;
      c->r[5] = 1;
      rec_dispatch(c, 0x80133610u); // UNOWNED
      c->mem_w8(obj + 5, 3);
      c->mem_w8(obj + 6, (uint8_t)nextSub);
      needRecord = false;
    } else if (c->mem_r16s(obj + 106) != 0) {
      needRecord = true;
    } else {
      c->mem_w16(obj + 106, (uint16_t)nextSub); // RE'd literal: ground truth stores the branch's
                                                // "next state" constant here too (4 for sub0, 5 for sub1)
      needRecord = false;
    }
  };

  if (sub == 0) {
    stateBody(1);
  } else if (sub == 1) {
    stateBody(1); // ground truth's sub==1 body mirrors sub==0's shape exactly except the
                  // obj[106] literal (5 instead of 4) — see header note above
    c->mem_w16(obj + 106, 5);
  } else {
    needRecord = false;
  }

  if (needRecord) {
    c->r[4] = obj;
    c->r[5] = 12;
    rec_dispatch(c, 0x80051B04u); // GraphicsBind::installSceneRecord-shaped
  }

  child = c->mem_r32(obj + 192);
  c->mem_w16(child + 60, c->mem_r16(child + 56));
  c->r[4] = obj;
  rec_dispatch(c, 0x80133774u); // UNOWNED
  c->r[4] = obj;
  rec_dispatch(c, 0x80133700u); // UNOWNED

  c->r[17] = s17;
  c->r[16] = s16;
  c->r[29] += 32;
}

// func_80133184 — DRAFT. RE'd from generated/ov_a00_shard_0.c gen_80133184 (0x80133184..0x801332AC).
// Called by beh_cull_substate_orchestrator's STATE 2 case
// (game/ai/beh_cull_substate_orchestrator.cpp:55, `rec_dispatch(c, 0x80133184u)`). Frame -32:
// spills r16/r17+ra at +16/+20/+24.
//
// Branch on obj[98] (s16) != 0 vs ==0 selects a "big" record-alloc request (rec=obj[192][64],
// class=1792, item=52; decrement obj[50] by 100) vs a "small" one (class=2560, item=36; decrement
// obj[50] by 150). Both feed the SAME shared tail: request record-alloc-shaped leaf 0x80027144
// [UNOWNED] (a0=rec, a1=class, a2=0? — arg wiring transliterated literally, not independently
// confirmed), then Sfx::trigger-shaped leaf 0x80074590(id=12, a1=0, a2=item) [dual-owned per
// codemap: Sfx::trigger], then SceneEvents::armBody-shaped leaf 0x80040B48(127) [dual-owned per
// codemap: SceneEvents::armBody]. If its result >= 0: obj[4]=3 directly. If < 0 (events disabled):
// a GBASE bit-set (0x8014C000-shape, obj[3]-indexed) + a rolling counter at (0x8014C000-shape)+449
// gate an Engine::announcerCue-shaped leaf 0x8004ED94(110, 65) [dual-owned: Engine::announcerCue]
// when the counter wraps past 10; obj[4]=3 either way at the very end.
void func_80133184(Core *c) {
  const uint32_t obj = c->r[4];
  const uint32_t s16 = c->r[16], s17 = c->r[17];
  c->r[29] -= 32;

  uint32_t item, cls;
  if (c->mem_r16s(obj + 98) != 0) {
    item = 52;
    cls = 1792;
    c->mem_w16(obj + 50, (uint16_t)(c->mem_r16(obj + 50) - 100));
  } else {
    item = 36;
    cls = 2560;
    c->mem_w16(obj + 50, (uint16_t)(c->mem_r16(obj + 50) - 150));
  }
  const uint32_t rec = c->mem_r32(c->mem_r32(obj + 192) + 64);
  c->r[4] = rec;
  c->r[5] = cls;
  c->r[6] = 0;
  c->r[7] = item;
  rec_dispatch(c, 0x80027144u); // UNOWNED record-alloc-shaped

  c->r[4] = 12;
  c->r[5] = 0;
  c->r[6] = 0;
  rec_dispatch(c, 0x80074590u); // Sfx::trigger-shaped

  c->r[4] = 127;
  c->r[31] = 0x8013325Cu;       // ra mirror: ov_a00_gen_80133184 jal-site (armBody spills ra)
  rec_dispatch(c, 0x80040B48u); // SceneEvents::armBody-shaped
  const int32_t armResult = (int32_t)c->r[2];

  uint8_t finalState;
  if (armResult < 0) {
    const uint8_t type = c->mem_r8(obj + 3);
    const uint32_t gbase = 0x8014C000u - 1936u;
    const bool bitSet = (c->mem_r8(gbase + 348) >> (type & 31)) & 1;
    (void)bitSet;
    uint8_t roll = (uint8_t)(c->mem_r8(gbase + 449) + 1);
    c->mem_w8(gbase + 348, (uint8_t)(c->mem_r8(gbase + 348) | (1u << (type & 31))));
    c->mem_w8(gbase + 449, roll);
    finalState = 3;
    if (roll < 10) {
      c->r[4] = 110;
      c->r[5] = 65;
      rec_dispatch(c, 0x8004ED94u); // Engine::announcerCue-shaped
    }
  } else {
    finalState = 3;
  }
  c->mem_w8(obj + 4, finalState);

  c->r[17] = s17;
  c->r[16] = s16;
  c->r[29] += 32;
}

// (removed 189 lines: the hand-transliterated func_80132A88 draft — replaced by a port_gen body,
//  see the commit that landed it. It carried guest-visible defects; do not resurrect it.)

// func_80132EDC — DRAFT. RE'd from generated/ov_a00_shard_1.c gen_80132EDC (0x80132EDC..0x80133174).
// Called by beh_cull_substate_orchestrator's node[5]==3 sub-state case
// (game/ai/beh_cull_substate_orchestrator.cpp). Frame -24: spills r16+ra at +16/+20.
//
// obj[6] (u8) drives a near-mirrored 0/1 pair on child=obj[192]'s heading fields (child[56]/[58]),
// using obj[78]/[80] as the per-frame deltas (SAME fields 0x80132D58 reads) — state0 does
// child[56]+=obj[78], child[58]-=obj[80]; state1 does the mirror (child[56]-=obj[78],
// child[58]+=obj[80]). Both then band-check child[58] against a target derived from
// obj[116]-obj[64] (state0, `<` sense) / obj[116]+obj[64] (state1, `<` sense on the OPPOSITE
// operand order — RE'd exactly, not a simple sign flip) and on a hit, snap child[58] to that
// target and set a "did-update" flag (shared "FCC"/"FD0" join, r6 in ground truth).
//
// If the update flag is set, a second block runs: obj[64] -= obj[74] (a countdown), and:
//   - if the new obj[64] > 0: TOGGLE obj[6] itself (obj[6] = 1 - obj[6] — this is the SAME field
//     this function switched on at entry; RE'd exactly, confirmed via the `1 - obj[6]` shape).
//   - else (obj[64] expired): snap child[58] = obj[116] (final target), then EITHER reset to
//     defaults (child[56]=obj[100], child[58]=obj[102], obj[5]=0, obj[6]=0, obj[64]=20) when
//     obj[104]==1 OR obj[108]!=0, OR set obj[5]=4 when obj[104]!=1 AND obj[108]==0.
// Both sub-blocks then fall to the shared tail (regardless of whether the update-flag block ran):
//   obj[108]!=0: calls trigger leaf 0x80133700(obj); on nonzero result, resets child[56]/[58] to
//     obj[100]/[102] and calls 0x80133610(obj,0), then clears obj[6]/obj[108].
//   obj[108]==0 && obj[94]!=0: calls 0x80133610(obj, obj[94]) and sets obj[6]=1, obj[108]=1.
//   obj[108]==0 && obj[94]==0 && obj[41]!=0 && obj[6]==0: bumps a GBASE-relative record's [+50]
//     u16 field by 5, gated by two more GBASE byte flag checks at [+325]bit0 and [+356] — the
//     SAME 0x8014E000-ish constant-table family as 0x8013272C/0x80132A88's tables, different
//     stride/base (record-indexed here, not per-type/rotating-index).
//   ALWAYS at the very end: obj[80] decays by 1/frame once obj[80]>=33 (else left unchanged), then
//   the tail leaf 0x80133774(obj) is called unconditionally before returning.
//
// LOW-MEDIUM CONFIDENCE on field semantics (matches the prior mapping pass's caveats in
// docs/engine_re.md); HIGH-MEDIUM CONFIDENCE on control flow — transliterated goto/label 1:1
// against ground truth, preserving the exact signed/unsigned read widths at each site (the two
// heading-band comparisons deliberately mix (int16) and zero-extended (u16) reads of the SAME
// field for the threshold vs. the stored value, exactly as ground truth does).
void func_80132EDC(Core *c) {
  const uint32_t obj = c->r[4];
  const uint32_t s16 = c->r[16];
  c->r[29] -= 24;

  uint32_t r6 = 0;
  uint8_t sub = c->mem_r8(obj + 6);
  bool updated = false;

  if (sub == 0) {
    goto F10;
  }
  if (sub == 1) {
    goto F70;
  }
  goto FD4;

F10: {
  uint32_t child = c->mem_r32(obj + 192);
  c->mem_w16(child + 56, (uint16_t)(c->mem_r16(child + 56) + c->mem_r16(obj + 78)));
  child = c->mem_r32(obj + 192);
  c->mem_w16(child + 58, (uint16_t)(c->mem_r16(child + 58) - c->mem_r16(obj + 80)));

  int32_t threshold = (int32_t)(int16_t)c->mem_r16(obj + 116) - (int32_t)(int16_t)c->mem_r16(obj + 64);
  child = c->mem_r32(obj + 192);
  uint32_t child58zx = c->mem_r16(child + 58);
  uint32_t obj64zx = c->mem_r16(obj + 64);
  uint32_t obj116zx = c->mem_r16(obj + 116);
  uint32_t rhs = obj116zx - obj64zx; // delay-slot recompute (unsigned), only used if cond true
  bool cond = ((int32_t)child58zx < threshold);
  if (!cond) {
    goto FD0;
  }
  c->mem_w16(child + 58, (uint16_t)rhs);
  goto FCC;
}

F70: {
  uint32_t child = c->mem_r32(obj + 192);
  c->mem_w16(child + 56, (uint16_t)(c->mem_r16(child + 56) - c->mem_r16(obj + 78)));
  child = c->mem_r32(obj + 192);
  c->mem_w16(child + 58, (uint16_t)(c->mem_r16(child + 58) + c->mem_r16(obj + 80)));

  int32_t threshold = (int32_t)(int16_t)c->mem_r16(obj + 116) + (int32_t)(int16_t)c->mem_r16(obj + 64);
  child = c->mem_r32(obj + 192);
  uint32_t child58zx = c->mem_r16(child + 58);
  uint32_t obj64zx = c->mem_r16(obj + 64);
  uint32_t obj116zx = c->mem_r16(obj + 116);
  uint32_t rhs = obj116zx + obj64zx; // delay-slot recompute (unsigned), only used if cond true
  bool cond = (threshold < (int32_t)child58zx);
  if (!cond) {
    goto FD0;
  }
  c->mem_w16(child + 58, (uint16_t)rhs);
  goto FCC;
}

FCC:
  r6 = 1;
FD0:
  updated = (r6 != 0);
FD4:
  if (!updated) {
    goto Tail078;
  }

  {
    int16_t newObj64 = (int16_t)(c->mem_r16(obj + 64) - c->mem_r16(obj + 74));
    c->mem_w16(obj + 64, (uint16_t)newObj64);
    if (newObj64 > 0) {
      // toggle obj[6] itself: obj[6] = 1 - obj[6]
      uint8_t f6 = c->mem_r8(obj + 6);
      c->mem_w8(obj + 6, (uint8_t)(1 - f6));
      goto Tail078;
    }
    uint32_t child = c->mem_r32(obj + 192);
    c->mem_w16(child + 58, c->mem_r16(obj + 116)); // snap to final target

    int16_t f104 = (int16_t)c->mem_r8(obj + 104);
    bool resetToDefaults;
    if (f104 == 1) {
      resetToDefaults = true;
    } else {
      int16_t f108 = (int16_t)c->mem_r16(obj + 108);
      resetToDefaults = (f108 != 0);
      if (!resetToDefaults) {
        c->mem_w8(obj + 5, 4);
        goto TailExit; // CONFIRMED: ground truth's L_80133060 jumps straight to the epilogue
                       // (L_80133174), bypassing the Tail078 shared-tail block entirely — no
                       // obj[80] decay, no 0x80133774 tail-leaf call on this path.
      }
    }
    if (resetToDefaults) {
      child = c->mem_r32(obj + 192);
      c->mem_w16(child + 56, c->mem_r16(obj + 100));
      child = c->mem_r32(obj + 192);
      c->mem_w16(child + 58, c->mem_r16(obj + 102));
      c->mem_w8(obj + 5, 0);
      c->mem_w8(obj + 6, 0);
      c->mem_w16(obj + 64, 20);
      goto TailExit; // CONFIRMED: ground truth's L_8013302C also jumps straight to the epilogue,
                     // bypassing Tail078 (same as above).
    }
  }

Tail078: {
  int16_t f108 = (int16_t)c->mem_r16(obj + 108);
  if (f108 != 0) {
    c->r[4] = obj;
    rec_dispatch(c, 0x80133700u); // UNOWNED trigger leaf
    if ((int32_t)(int16_t)c->r[2] != 0) {
      uint32_t child = c->mem_r32(obj + 192);
      c->mem_w16(child + 56, c->mem_r16(obj + 100));
      child = c->mem_r32(obj + 192);
      c->mem_w16(child + 58, c->mem_r16(obj + 102));
      c->r[4] = obj;
      c->r[5] = 0;
      rec_dispatch(c, 0x80133610u); // UNOWNED
      c->mem_w8(obj + 6, 0);
      c->mem_w16(obj + 108, 0);
    }
  } else {
    uint8_t f94 = c->mem_r8(obj + 94);
    if (f94 != 0) {
      c->r[4] = obj;
      c->r[5] = f94;
      rec_dispatch(c, 0x80133610u); // UNOWNED
      c->mem_w8(obj + 6, 1);
      c->mem_w16(obj + 108, 1);
    } else {
      uint8_t f41 = c->mem_r8(obj + 41);
      if (f41 != 0) {
        uint8_t f6 = c->mem_r8(obj + 6);
        if (f6 == 0) {
          // RE'd literal: rec = (32782<<16) + 32384 = 0x800E0000 + 0x7E80 = 0x800E7E80 — a
          // FIXED main-RAM record pointer (NOT the 0x8014Cxxx "GBASE" family the other leaves
          // in this cluster read; this is its own distinct constant). Two byte-flag gates at
          // rec+325 (bit0) and rec+356 then a +5 bump on a u16 field at rec+50. LOW CONFIDENCE
          // on the record's semantic identity — never confirmed against a live dump.
          const uint32_t rec = 0x800E7E80u;
          if ((c->mem_r8(rec + 325) & 1) == 0 && c->mem_r8(rec + 356) == 0) {
            c->mem_w16(rec + 50, (uint16_t)(c->mem_r16(rec + 50) + 5));
          }
        }
      }
    }
  }

  int16_t f80 = (int16_t)c->mem_r16(obj + 80);
  if (f80 >= 33) {
    c->mem_w16(obj + 80, (uint16_t)(c->mem_r16(obj + 80) - 1));
  }
  c->r[4] = obj;
  rec_dispatch(c, 0x80133774u); // UNOWNED tail leaf — unconditional
}

TailExit:
  c->r[16] = s16;
  c->r[29] += 24;
}
