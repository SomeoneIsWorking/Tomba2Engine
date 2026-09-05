// class ActorTomba — implementation. See actor_tomba.h for the class overview.
//
// This file consolidates every previously-owned Tomba primitive under one class:
//   * interactWalk + 3 collision helpers (was game/player/tomba_interact.cpp)
//   * velocityIntegrate  (was game/player/engine_player.cpp's static player_move_56b48)
//   * growthStep         (was Engine::playerGrowthStep in game/core/engine.cpp)
//
// Ghidra decomp references:
//   scratch/decomp/tomba_perframe_22760.c   (interactWalk = FUN_80022760)
//   scratch/decomp/tomba_interact_subs.c    (proximityCheck / subHitboxCheck)
//   scratch/decomp/fun_80114e74.c           (type4GuardedCheck)
//   scratch/decomp/batch_leaves.c           (growthStep = FUN_80057DC0)
//   disas.py 0x80056B48                    (velocityIntegrate)

#include "actor_tomba.h"
#include "cfg.h"
#include "core.h"
#include "core/engine.h"
#include "game.h"
#include "game_ctx.h"
#include "guest_abi.h"
#include "guest_call.h"
#include "guest_jal.h"               // guest call-site argument and return-address handling through PSXPort dispatch
#include "native_override_catalog.h" // title declarations; TombaRuntime owns resident-image binding

namespace {

// -- Shared constants (guest addresses) -------------------------------------------------------
constexpr uint32_t AUX_LIST_HEAD_SPAD = 0x1F800154u;
constexpr uint32_t AUX_LIST_COUNT_SPAD = 0x1F80015Cu;
constexpr uint32_t AUX_WALK_COUNTER = 0x1F800182u;
constexpr uint32_t GATE_BF80C_hi = 0x800BF80Du;

constexpr uint32_t LEAF_ISQRT = 0x80084080u;
constexpr uint32_t LEAF_ATAN2 = 0x80085690u;
constexpr uint32_t LEAF_COLL_CB = 0x8004D19Cu;
constexpr uint32_t LEAF_PROX_F04 = 0x80022F04u;

// Sub-hitbox parameter table (u8[16], 2 bytes per hitbox: (size_xz, size_y)) — MAIN.EXE .rodata.
constexpr uint32_t SUB_HITBOX_PARAMS = 0x800A29D0u;

// Shared scratchpad outputs.
constexpr uint32_t OUT_DIST_SPAD = 0x1F80008Cu;
constexpr uint32_t OUT_HEADING_SPAD = 0x1F80009Cu;

// Growth-mode rescale target (a shared BSS halfword read by scenery/props).
constexpr uint32_t GROWTH_MIRROR_HW = 0x800E802Au;

inline void mark_item_consumed(Core *c, uint32_t item) {
  c->mem_w8(item + 0, 2);
  c->mem_w8(item + 4, 2);
  c->mem_w8(item + 5, 0);
  c->mem_w8(item + 6, 0);
}

// TombaState — named-field lens over Tomba's G block (ActorTomba::G_ADDR / a `G` local passed in
// by the per-frame driver). Guest addresses are UNCHANGED from the raw c->mem_r/w8/16 pokes this
// replaces (see actor_tomba.h's per-function doc comments for the RE of each field); this is a
// readability lens, not a state migration — every accessor is still a direct guest-RAM access.
// Scoped to the fields the frameTick / outer-transition-gate / outer-transition-commit cluster
// touches; a wider pass can grow this lens as more of the file gets ported.
struct TombaState {
  Core *c;
  uint32_t base;

  // outerState (+0x4): frameTick's own top-level FSM selector (0=INIT..7=LOAD-WAIT). Also used,
  // within LOAD-WAIT, as the settle target frameTick's case-7 sub-machine writes back to (=1).
  uint8_t outerState() const {
    return c->mem_r8(base + 0x4u);
  }
  void setOuterState(uint8_t v) {
    c->mem_w8(base + 0x4u, v);
  }

  // loadStep/loadSub/loadSub2 (+0x5/+0x6/+0x7): the 3-state LOAD-WAIT sub-machine counter
  // (frameTick case 7) — also the "G+5=1,G+6=0" pair outerTransitionGate/Commit write when they
  // commit a fresh walk-state.
  uint8_t loadStep() const {
    return c->mem_r8(base + 0x5u);
  }
  void setLoadStep(uint8_t v) {
    c->mem_w8(base + 0x5u, v);
  }
  void setLoadSub(uint8_t v) {
    c->mem_w8(base + 0x6u, v);
  }
  void setLoadSub2(uint8_t v) {
    c->mem_w8(base + 0x7u, v);
  }

  // statusFlags (+0x0): walk-state / cutscene-lock byte. Bit 4 and the 0xC mask gate the
  // outer-transition commit path; literal 3 is the "reset to walk" stamp.
  uint8_t statusFlags() const {
    return c->mem_r8(base + 0x0u);
  }
  void setStatusFlags(uint8_t v) {
    c->mem_w8(base + 0x0u, v);
  }

  // latchFlags (+0xD): stop-motion / facing-lock bitfield (0x80 busy, 0x50 lock mask, 0x82 armed).
  uint8_t latchFlags() const {
    return c->mem_r8(base + 0xDu);
  }
  void setLatchFlags(uint8_t v) {
    c->mem_w8(base + 0xDu, v);
  }

  // stopMotionAux (+0x61): companion byte cleared alongside latchFlags on a walk-state reset.
  void setStopMotionAux(uint8_t v) {
    c->mem_w8(base + 0x61u, v);
  }

  // facing (+0x140, s16): Tomba's current heading — turnBiasCompute's `facing` arg source.
  int16_t facing() const {
    return (int16_t)c->mem_r16(base + 0x140u);
  }

  // turnSuppressGate (+0x146): "already turn-suppressed" flag frameTick checks post-turnBias.
  uint8_t turnSuppressGate() const {
    return c->mem_r8(base + 0x146u);
  }
  void setTurnSuppressGate(uint8_t v) {
    c->mem_w8(base + 0x146u, v);
  }

  // transitionSlot (+0x164): the interaction-slot state outerTransitionGate branches on (1 = a
  // specific slot -> a stop-motion spawn without the busy-latch check; else gated by 0x800BF80D).
  uint8_t transitionSlot() const {
    return c->mem_r8(base + 0x164u);
  }

  void setExtraClear(uint8_t v) {
    c->mem_w8(base + 0x16Au, v);
  }

  // turnCurrent/turnTarget (+0x16E/+0x170, s16): the pending-frame turn counter and its commit
  // target. outerTransitionGate bails while turnCurrent is still positive; outerTransitionCommit
  // arms turnTarget = turnCurrent when they differ.
  int16_t turnCurrent() const {
    return (int16_t)c->mem_r16(base + 0x16Eu);
  }
  void setTurnCurrent(uint16_t v) {
    c->mem_w16(base + 0x16Eu, v);
  }
  int16_t turnTarget() const {
    return (int16_t)c->mem_r16(base + 0x170u);
  }
  void setTurnTarget(uint16_t v) {
    c->mem_w16(base + 0x170u, v);
  }

  // settleCounter (+0x172, s16): outerTransitionCommit's decrement-and-settle counter; reaching 0
  // either commits walk-state 1 or re-arms to 1 depending on statusFlags.
  int16_t settleCounter() const {
    return (int16_t)c->mem_r16(base + 0x172u);
  }
  void setSettleCounter(uint16_t v) {
    c->mem_w16(base + 0x172u, v);
  }

  // committing (+0x17B): frameTick case-2 (COMMITTING) latch, set on entry to that state.
  void setCommitting(uint8_t v) {
    c->mem_w8(base + 0x17Bu, v);
  }

  // posAddr(): +0x2C — Tomba's position triple, passed BY ADDRESS to the stop-motion spawn call
  // (guest FUN_800312D4 takes a dest pointer, not a value).
  uint32_t posAddr() const {
    return base + 0x2Cu;
  }

  // -- Extended 2026-07-15 (2nd code-quality pass): interactWalk/proximityCheck/subHitboxCheck/
  // postInteractWalk/postFrameWaterCheck/type8Interact/type7Interact cluster. TombaState is
  // constructed over G_ADDR for Tomba himself, but proximityCheck/subHitboxCheck/stepModeInteract/
  // type8Interact all read an ITEM node at the SAME field offsets (0x2E/0x32/0x36/0x80/0x84/0x86)
  // — items share Tomba's node layout, so this lens doubles as a generic actor-node view: wrap it
  // over `item` too (`TombaState other{c, item}`) rather than reading item+0xNN raw.

  // posX/posY/posZ (+0x2E/+0x32/+0x36, s16): world position triple interactWalk's proximity math
  // and postFrameWaterCheck's off-map check read/write. NOT the same triple as posAddr() (+0x2C) —
  // RE unresolved why the two don't coincide; posAddr() is only ever used as a spawn dest pointer.
  int16_t posX() const {
    return (int16_t)c->mem_r16(base + 0x2Eu);
  }
  void setPosX(int16_t v) {
    c->mem_w16(base + 0x2Eu, (uint16_t)v);
  }
  int16_t posY() const {
    return (int16_t)c->mem_r16(base + 0x32u);
  }
  void setPosY(int16_t v) {
    c->mem_w16(base + 0x32u, (uint16_t)v);
  }
  int16_t posZ() const {
    return (int16_t)c->mem_r16(base + 0x36u);
  }
  void setPosZ(int16_t v) {
    c->mem_w16(base + 0x36u, (uint16_t)v);
  }

  // boundXZ (+0x80, s16): horizontal (X/Z) cylinder-proximity radius. boundYUp (+0x84, READ
  // UNSIGNED — ground truth never sign-extends this field) / boundYDown (+0x86, s16): the two
  // halves of the vertical proximity band, summed between two nodes in proximityCheck/
  // subHitboxCheck's Y-band test.
  int16_t boundXZ() const {
    return (int16_t)c->mem_r16s(base + 0x80u);
  }
  uint16_t boundYUp() const {
    return c->mem_r16(base + 0x84u);
  }
  int16_t boundYDown() const {
    return (int16_t)c->mem_r16s(base + 0x86u);
  }

  // growthFlags (+0x17E, u16): bit 0x200 = "paused/frozen" (interactWalk/stepModeInteract/
  // type8Interact all early-out or branch on it), bit 0x8000 = "grown" (growthStep toggles it;
  // stepModeInteract/type8Interact branch on it to route to the grown-state delegate leaves).
  uint16_t growthFlags() const {
    return c->mem_r16(base + 0x17Eu);
  }

  // justTransitioned (+0x144, u8): "just entered this interaction state" latch — stepModeInteract/
  // type8Interact both special-case `==1 && v0<2` as the just-triggered transition frame.
  uint8_t justTransitioned() const {
    return c->mem_r8(base + 0x144u);
  }

  // groundedGate (+0x145, u8) bit 0: postFrameWaterCheck/type8Interact/growthYSnap check bit 0
  // clear before snapping the position to a water/growth-offset target.
  uint8_t groundedGate() const {
    return c->mem_r8(base + 0x145u);
  }
  void setGroundedGate(uint8_t v) {
    c->mem_w8(base + 0x145u, v);
  }

  // frozenFlag (+0x78, u8): "not frozen" gate type8Interact/growthYSnap check before a niladic
  // cue / a growth-offset re-snap.
  uint8_t frozenFlag() const {
    return c->mem_r8(base + 0x78u);
  }

  // flag95 (+0x5F, u8) / groundContactFlag (+0x60, u8): the header's own names (see actor_tomba.h
  // "settleStep"/"type8Interact" doc comments) for the ground-probe response pair type8Interact's
  // proximity-hit branch stamps.
  void setFlag95(uint8_t v) {
    c->mem_w8(base + 0x5Fu, v);
  }
  void setGroundContactFlag(uint8_t v) {
    c->mem_w8(base + 0x60u, v);
  }

  // physOffsetY (+0x62, s16): the "physics constant" growthStep rescales (header: "G+0x62/64/66/
  // 68 (physics constants)") — postFrameWaterCheck reuses it as the water-surface-to-feet offset.
  int16_t physOffsetY() const {
    return (int16_t)c->mem_r16s(base + 0x62u);
  }

  // committing (+0x17B, u8): frameTick case-2 (COMMITTING) latch — see setCommitting() above;
  // postFrameWaterCheck's off-map trigger gates on it being clear.
  uint8_t committing() const {
    return c->mem_r8(base + 0x17Bu);
  }
};

} // namespace

// =================================================================================
// Per-frame interaction walk (FUN_80022760)
// =================================================================================
void ActorTomba::interactWalk() {
  Core *c = core;
  const uint32_t G = G_ADDR;
  TombaState tomba{c, G};

  // Early-outs.
  if (tomba.turnCurrent() == 0) {
    return;
  }
  if (c->mem_r8(GATE_BF80C_hi) != 0) {
    return;
  }
  if (tomba.growthFlags() & 0x200u) {
    return;
  }

  const uint32_t listBase = c->mem_r32(AUX_LIST_HEAD_SPAD);
  const uint8_t count0 = c->mem_r8(AUX_LIST_COUNT_SPAD);
  c->mem_w8(GATE_BF80C_hi, 0); // guest instruction path's initial `uVar2=0` write
  c->mem_w8(AUX_WALK_COUNTER, count0);

  uint32_t cursor = listBase;
  while (c->mem_r8(AUX_WALK_COUNTER) != 0) {
    const uint32_t item = c->mem_r32(cursor);
    c->mem_w8(AUX_WALK_COUNTER, (uint8_t)(c->mem_r8(AUX_WALK_COUNTER) - 1));
    cursor += 4;
    if (c->mem_r8(item) != 1) {
      continue; // item[0]!=1 → skip
    }

    const uint8_t typ = c->mem_r8(item + 2u);
    switch (typ) {
    case 0:
    case 1:
    case 2:
    case 3:
    case 7:
      proximityCheck(item);
      break;
    case 4:
      if (c->mem_r8(item + 0x5Eu) == 2) {
        type4GuardedCheck(item);
      } else {
        proximityCheck(item);
      }
      break;
    case 6:
      subHitboxCheck(item);
      break;
    default:
      break;
    }
  }
}

// FUN_80022060 — cylinder proximity + Y-band check.
//
// BOTH gates compare an UNSIGNED 16-bit quantity (`andi …,0xffff` at 0x800220E0 and 0x80022118)
// against a sign-extended 32-bit limit. That is what makes the vertical gate one-sided in the way
// the game depends on: the vertical term is Tomba-above-item PLUS both up-extents, so once Tomba
// clears the item (jumping over it) the sum goes NEGATIVE, `andi 0xffff` turns it into ~0xFFxx =
// a huge positive, and the `slt limit, band` rejects the touch. Sign-extending it instead makes it
// a small negative that sails under the limit — i.e. every jump-over collects the item (kanban #1
// / #30). The distance gate is the same shape: isqrt can return > 0x7FFF, and sign-extending that
// turns a very distant object into a negative "distance" that passes. Keep both zero-extended.
// The scratchpad output at +0x8C is the one place the guest DOES sign-extend it (sll/sra at
// 0x80022134), so that store keeps its own sign-extended value.
void ActorTomba::proximityCheck(uint32_t item) {
  Core *c = core;
  const uint32_t G = G_ADDR;
  if (c->mem_r8(0x1F80027Au) != 0) {
    return;
  }

  TombaState tomba{c, G};
  TombaState other{c, item};

  const int32_t dx = (int32_t)(int16_t)(tomba.posX() - other.posX());
  const int32_t dz = (int32_t)(int16_t)(tomba.posZ() - other.posZ());
  c->r[4] = (uint32_t)(dx * dx + dz * dz);
  psx::cpu::dispatchGuestToReturn0(*c, LEAF_ISQRT, psx::cpu::ExecutionBudget::currentTurn(*c), __func__);
  const uint16_t distBits = (uint16_t)c->r[2];
  const int32_t dist = (int32_t)(uint32_t)distBits; // `andi a0,a2,0xffff` — UNSIGNED

  const int32_t rxz = (int32_t)tomba.boundXZ() + (int32_t)other.boundXZ();
  if (dist > rxz) {
    return;
  }

  const int32_t vbandRaw = (int32_t)(uint32_t)(uint16_t)((tomba.posY() - other.posY()) + tomba.boundYUp() +
                                                         other.boundYUp()); // `andi v1,v1,0xffff`
  const int32_t vbandLim = (int32_t)tomba.boundYDown() + (int32_t)other.boundYDown();
  if (vbandRaw > vbandLim) {
    return;
  }

  c->mem_w32(OUT_DIST_SPAD, (uint32_t)(int32_t)(int16_t)distBits); // sll/sra 16 — sign-extended
  c->r[4] = (uint32_t)(-dz);
  c->r[5] = (uint32_t)dx;
  psx::cpu::dispatchGuestToReturn0(*c, LEAF_ATAN2, psx::cpu::ExecutionBudget::currentTurn(*c), __func__);
  c->mem_w32(OUT_HEADING_SPAD, c->r[2]);
  mark_item_consumed(c, item);
  c->mem_w8(0x800BF81Eu, 0);
}

// FUN_80114E74 — type-4 guarded proximity.
void ActorTomba::type4GuardedCheck(uint32_t item) {
  Core *c = core;
  const uint32_t G = G_ADDR;
  if (c->mem_r8(G + 0x164u) == 5 && c->mem_r8(G + 0x147u) == c->mem_r8(item + 0x47u)) {
    return;
  }
  c->r[4] = G;
  c->r[5] = item;
  psx::cpu::dispatchGuestToReturn0(*c, LEAF_PROX_F04, psx::cpu::ExecutionBudget::currentTurn(*c), __func__);
  if (c->r[2] == 0) {
    return;
  }
  mark_item_consumed(c, item);
}

// FUN_80022190 — per-sub-hitbox collision variant.
void ActorTomba::subHitboxCheck(uint32_t item) {
  Core *c = core;
  const uint32_t G = G_ADDR;
  TombaState tomba{c, G};
  const int16_t hitboxCount = (int16_t)c->mem_r16(item + 0x6Au);
  if (hitboxCount <= 0) {
    return;
  }
  uint32_t hitboxArr = c->mem_r32(item + 0x6Cu);

  for (int32_t i = 0; i < hitboxCount; i++, hitboxArr += 0x10u) {
    const uint32_t mask = 1u << (i & 0x1F);
    if ((c->mem_r32(item + 0x70u) & mask) == 0) {
      continue;
    }

    const uint32_t typeParam = (uint32_t)c->mem_r8(hitboxArr + 3u) * 8u;
    const int32_t dx = (int32_t)(int16_t)(tomba.posX() - c->mem_r16(hitboxArr + 4u));
    const int32_t dz = (int32_t)(int16_t)(tomba.posZ() - c->mem_r16(hitboxArr + 8u));
    c->r[4] = (uint32_t)(dx * dx + dz * dz);
    psx::cpu::dispatchGuestToReturn0(*c, LEAF_ISQRT, psx::cpu::ExecutionBudget::currentTurn(*c), __func__);
    const int32_t dist = (int32_t)(uint32_t)(c->r[2] & 0xFFFFu);

    const int32_t rxz = (int32_t)tomba.boundXZ() + (int32_t)c->mem_r8(SUB_HITBOX_PARAMS + typeParam + 0u);
    if (dist > rxz) {
      continue;
    }

    const uint32_t vbandRaw = (uint32_t)((tomba.posY() - c->mem_r16(hitboxArr + 6u)) + tomba.boundYUp() +
                                         c->mem_r8(SUB_HITBOX_PARAMS + typeParam + 1u));
    const int32_t vbandLim = (int32_t)tomba.boundYDown() + (int32_t)c->mem_r8(SUB_HITBOX_PARAMS + typeParam + 1u) * 2;
    if ((int32_t)(uint16_t)vbandRaw > vbandLim) {
      continue;
    }

    c->mem_w32(item + 0x74u, c->mem_r32(item + 0x74u) | mask);
    c->mem_w32(item + 0x70u, c->mem_r32(item + 0x70u) & ~mask);
    c->r[4] = item;
    c->r[5] = hitboxArr;
    c->r[6] = 0;
    psx::cpu::dispatchGuestToReturn0(*c, LEAF_COLL_CB, psx::cpu::ExecutionBudget::currentTurn(*c), __func__);
    return;
  }
}

// =================================================================================
// Post-interact walk (FUN_801130C4) — the default-mode "post-tick" that runs after interactWalk
// =================================================================================
void ActorTomba::postInteractWalk() {
  Core *c = core;
  const uint32_t G = G_ADDR;
  TombaState tomba{c, G};

  // This walker uses a DIFFERENT aux list than interactWalk: the render/interaction queue at
  // *0x1F80013C with count *0x1F800144 (vs 0x1F800154 / 0x1F80015C for interactWalk).
  constexpr uint32_t LIST_HEAD_SPAD = 0x1F80013Cu;
  constexpr uint32_t LIST_COUNT_SPAD = 0x1F800144u;

  constexpr uint32_t LEAF_TYPE_9_SPECIAL = 0x80111304u; // item[0xC]==9 guarded handler
  constexpr uint32_t LEAF_TYPE_3 = 0x8010E258u;
  constexpr uint32_t LEAF_TYPE_4_PROX_STEP = 0x8001F40Cu; // case-4 proximity + return code
  constexpr uint32_t LEAF_TYPE_4_TAG_SET = 0x8001FDB4u;   // case-4 alt-tag write
  constexpr uint32_t LEAF_TYPE_7 = 0x800235A0u;
  constexpr uint32_t LEAF_TYPE_8 = 0x800205CCu;
  constexpr uint32_t LEAF_TYPE_0F_14_56 = 0x80020364u;
  constexpr uint32_t LEAF_TYPE_13 = 0x8010EA80u;

  uint32_t cursor = c->mem_r32(LIST_HEAD_SPAD);
  c->mem_w8(AUX_WALK_COUNTER, c->mem_r8(LIST_COUNT_SPAD));

  while (c->mem_r8(AUX_WALK_COUNTER) != 0) {
    const uint32_t item = c->mem_r32(cursor);
    c->mem_w8(AUX_WALK_COUNTER, (uint8_t)(c->mem_r8(AUX_WALK_COUNTER) - 1));
    cursor += 4;
    if ((c->mem_r8(item) & 1) == 0) {
      continue; // active-flag gate
    }
    if (c->mem_r8(item + 0xCu) == 9) { // special-type items
      if ((tomba.growthFlags() & 0x8200u) == 0) {
        tomba::guest::dispatchLeafToReturn(*c, LEAF_TYPE_9_SPECIAL, G, item);
      }
      continue; // keep walking
    }
    const uint8_t typ = c->mem_r8(item + 2u);
    switch (typ) {
    case 3:
      tomba::guest::dispatchLeafToReturn(*c, LEAF_TYPE_3, G, item);
      break;
    case 4: {
      // Detailed guarded state-transition (case 4). Faithful to the guest instruction path:
      //   * dispatch the proximity/step leaf with a2=1; if v0 < 0 → skip (no interaction).
      //   * stop the walk (WALK_COUNTER = 0) unconditionally after we enter case 4.
      //   * bonus-tag path: DAT_800BF9E5 == 6 && G+0x144 == 1 && v0 < 2 →
      //         FUN_8001FDB4(item, 0xFFFF8001, 0x10, 0x20); continue walking.
      //   * silence path: skip if 0x1F800137 != 0 OR G[0] & 6 OR G+0x144 > 1 OR G+0x164 != 0.
      //   * else: DAT_800BF9E5 != 6 → announcer cue 0x2A/0x41; stamp G/item state as the
      //     "type-4 hit" transition (see writes below).
      const int32_t v0 = (int32_t)tomba::guest::dispatchLeafToReturn(*c, LEAF_TYPE_4_PROX_STEP, G, item, 1u);
      if (v0 < 0) {
        break; // no hit
      }
      c->mem_w8(AUX_WALK_COUNTER, 0); // stop the walk
      const uint8_t bf9e5 = c->mem_r8(0x800BF9E5u);
      const uint8_t g144 = tomba.justTransitioned();
      if (bf9e5 == 6 && g144 == 1 && v0 < 2) {
        tomba::guest::dispatchLeafToReturn(*c, LEAF_TYPE_4_TAG_SET, item, 0xFFFF8001u, 0x10u, 0x20u);
        break; // continue at loop top (via while)
      }
      if (c->mem_r8(0x1F800137u) != 0) {
        break;
      }
      if ((tomba.statusFlags() & 6) != 0) {
        break;
      }
      if (g144 > 1) {
        break;
      }
      if (tomba.transitionSlot() != 0) {
        break;
      }
      if (bf9e5 != 6) {
        eng(c).announcerCue(0x2A, 0x41); // native FUN_8004ED94
      }
      // Type-4 hit state transition on G + item.
      c->mem_w8(G + 4, 2);
      c->mem_w8(G + 5, 2);
      tomba.setStatusFlags(3);
      c->mem_w8(G + 6, 0);
      c->mem_w8(G + 0x172u, 0x78);
      c->mem_w8(G + 0x173u, 0);
      c->mem_w8(G + 0x2Bu, (uint8_t)((int32_t)c->mem_r32(OUT_HEADING_SPAD) >> 4));
      break;
    }
    case 7:
      tomba::guest::dispatchLeafToReturn(*c, LEAF_TYPE_7, G, item);
      break;
    case 8:
      tomba::guest::dispatchLeafToReturn(*c, LEAF_TYPE_8, G, item);
      break;
    case 0x0F:
    case 0x14:
    case 0x56:
      tomba::guest::dispatchLeafToReturn(*c, LEAF_TYPE_0F_14_56, G, item, 0u);
      break;
    case 0x13:
      tomba::guest::dispatchLeafToReturn(*c, LEAF_TYPE_13, G, item);
      break;
    case 0x2F:
      tomba::guest::dispatchLeafToReturn(*c, LEAF_TYPE_0F_14_56, G, item, 2u);
      break;
    default:
      break; // no interaction for other types
    }
  }
}

// =================================================================================
// Growth / shrink transformation (FUN_80057DC0)
// =================================================================================
void ActorTomba::growthStep(int32_t mode) {
  Core *c = core;
  const uint32_t G = G_ADDR;

  const uint16_t f17E = c->mem_r16(G + 0x17Eu);
  const int16_t posY = (int16_t)c->mem_r16(G + 0x32u);
  uint16_t newFlag;
  if (mode == 0) {
    if (f17E & 0x8000) {
      c->mem_w16(G + 0x32u, (uint16_t)(posY - 0x46)); // shrink → drop feet
    }
    newFlag = (uint16_t)(f17E & 0x7FFF);
  } else {
    if ((f17E & 0x8000) == 0) {
      c->mem_w16(G + 0x32u, (uint16_t)(posY + 0x46)); // grow → raise feet
    }
    newFlag = (uint16_t)(f17E | 0x8000);
  }
  c->mem_w16(G + 0x17Eu, newFlag);

  const int32_t divisor = mode + 1;
  const int16_t s1000 = (int16_t)(0x1000 / divisor);
  const int16_t s32_ = (int16_t)(0x32 / divisor);
  const int16_t s100 = (int16_t)(100 / divisor);
  const int16_t s8C = (int16_t)(0x8C / divisor);
  const int16_t s10E = (int16_t)(0x10E / divisor);
  const int16_t s1E = (int16_t)(0x1E / divisor);
  const int16_t sF0 = (int16_t)(0xF0 / divisor);
  c->mem_w16(G + 0xB8u, (uint16_t)s1000);
  c->mem_w16(G + 0xBAu, (uint16_t)s1000);
  c->mem_w16(G + 0xBCu, (uint16_t)s1000);
  c->mem_w16(G + 0x80u, (uint16_t)s32_);
  c->mem_w16(G + 0x82u, (uint16_t)s100);
  c->mem_w16(G + 0x84u, (uint16_t)s8C);
  c->mem_w16(G + 0x86u, (uint16_t)s10E);
  c->mem_w16(G + 0x62u, (uint16_t)s8C);
  c->mem_w16(G + 0x64u, (uint16_t)s8C);
  c->mem_w16(G + 0x66u, (uint16_t)s100);
  c->mem_w16(G + 0x68u, (uint16_t)s1E);
  c->mem_w16(GROWTH_MIRROR_HW, (uint16_t)sF0);
}

// =================================================================================
// Post-frame water/sea check (FUN_8010E904 — final call in area_seaside_perframe)
// =================================================================================
void ActorTomba::postFrameWaterCheck() {
  Core *c = core;
  const uint32_t G = G_ADDR;
  TombaState tomba{c, G};

  constexpr uint32_t WATER_MODE_BYTE = 0x800BF816u;
  constexpr uint32_t WATER_LEVEL_S16 = 0x800BF812u; // water surface Y (s16)
  constexpr uint32_t WATER_STATE_BYTE = 0x800BF817u;
  constexpr uint32_t PAUSE_FLAG_SPAD = 0x1F800137u;
  constexpr uint32_t LEAF_DRY_TICK = 0x8010E408u;     // per-frame Tomba tick when not in water
  constexpr uint32_t LEAF_WATER_SPLASH = 0x80022C78u; // Y-snap tail (particle spawn?)

  const int16_t waterLevel = (int16_t)c->mem_r16(WATER_LEVEL_S16);
  const uint8_t waterMode = c->mem_r8(WATER_MODE_BYTE);
  const uint8_t waterState = c->mem_r8(WATER_STATE_BYTE);

  if (waterMode == 0) {
    // Dry land: run the per-frame Tomba tick if not paused.
    if (c->mem_r8(PAUSE_FLAG_SPAD) == 0) {
      tomba::guest::dispatchLeafToReturn(*c, LEAF_DRY_TICK, G);
    }
  } else {
    // Water/sea mode. When water-state is 2 with a specific 800E7FEB (== 8) config, clamp
    // Tomba's Z to the water-region edge — matches the guest instruction path's `< 0x1a05 → 0x1a04` snap.
    bool skipYSnap = false;
    if (waterState > 1) {
      if (waterState == 2 && c->mem_r8(0x800E7FEBu) == 8) {
        if (tomba.posZ() < 0x1A05) {
          tomba.setPosZ(0x1A04);
        }
      } else {
        skipYSnap = true; // guest instruction path: `goto LAB_8010E9D4;` skips the Y block
      }
    }
    if (!skipYSnap) {
      if ((tomba.groundedGate() & 1) == 0 &&
          (int32_t)waterLevel - (int32_t)tomba.physOffsetY() <= (int32_t)tomba.posY()) {
        tomba.setPosY((int16_t)(waterLevel - tomba.physOffsetY())); // Y = waterLevel - G+0x62
        tomba::guest::dispatchLeafToReturn(*c, LEAF_WATER_SPLASH, G);
      }
    }
  }

  // Area-exit trigger — fires only in water-mode 2 when Tomba is off-map.
  if (tomba.committing() != 0) {
    return;
  }
  if (c->mem_r8(0x800BF80Du) != 0) {
    return;
  }
  if (c->mem_r8(0x800BF839u) != 0) {
    return;
  }
  if (waterMode == 0) {
    return;
  }
  if (waterState != 2) {
    return;
  }
  if (tomba.posY() >= -0xE74) {
    return;
  }
  if (tomba.posZ() >= 0x1451) {
    return;
  }

  c->mem_w8(PAUSE_FLAG_SPAD, waterState);
  c->mem_w8(0x800BF80Fu, waterState);
  c->mem_w16(0x800BF83Au, 0x100);
  c->mem_w8(0x800BF839u, 1);
  c->mem_w8(0x1F800236u, 1);
}

// =================================================================================
// postInteractWalk sub-handlers — band 0x80020000-0x8002FFFF. RE'd + drafted 2026-07-08 from
// Ghidra headless (scratch/decomp/region_8002.c) cross-checked against authenticated executable/overlay evidence
// (ground truth for the guest-stack frame + jal-site `ra` constants). UNWIRED: postInteractWalk
// above still reaches these via typed runtime address dispatch(c, LEAF_TYPE_*) — wiring these methods in requires
// adding override-registry entries, deliberately left for the next frontier
// pass so this draft compiles as dead code only.
// =================================================================================
namespace {
constexpr uint32_t LEAF_PROX_STEP =
    0x8001F40Cu; // FUN_8001F40C — shared proximity+step (== postInteractWalk's LEAF_TYPE_4_PROX_STEP)
constexpr uint32_t LEAF_ALT_TAG_SET =
    0x8001FDB4u; // FUN_8001FDB4 — alt-tag stamp (== postInteractWalk's LEAF_TYPE_4_TAG_SET)
constexpr uint32_t LEAF_GROWN_PUSH =
    0x8001F054u; // FUN_8001F054 — grown-state push (stepModeInteract's 0x8000-set/mode&3 branch)
constexpr uint32_t LEAF_NILADIC_CUE = 0x8001F830u; // FUN_8001F830 — niladic cue (type8Interact's item[0]==5 branch)
constexpr uint32_t LEAF_GROWN_DELEGATE =
    0x8001EC3Cu; // FUN_8001EC3C — whole-hog grown-state delegate (type8Interact's 0x8000-set branch)
constexpr uint32_t LEAF_STEP_MODE_FLAG = 0x8001FF7Cu; // FUN_8001FF7C — type7Interact's mode/flag call
} // namespace

// FUN_80020364 — postInteractWalk case 0xF/0x14/0x56 (mode=0) / 0x2F (mode=2).
uint8_t ActorTomba::stepModeInteract(uint32_t item, uint32_t mode) {
  Core *c = core;
  const uint32_t G = G_ADDR;

  // Guest frame: addiu sp,-40; spill s0,s1,s2,s3,ra (mirrored for completeness though this
  // draft has no re-entrant native call that would observe the guest stack bytes yet).
  const uint32_t sp0 = c->r[29];
  c->r[29] = sp0 - 40;
  c->mem_w32(c->r[29] + 20, c->r[17]);
  c->mem_w32(c->r[29] + 24, c->r[18]);
  c->mem_w32(c->r[29] + 32, c->r[31]);
  c->mem_w32(c->r[29] + 28, c->r[19]);
  c->mem_w32(c->r[29] + 16, c->r[16]);
  c->r[17] = G;
  c->r[18] = item;
  c->r[19] = mode;

  uint8_t result;
  if (c->mem_r16(G + 0x17Eu) & 0x200u) {
    result = 0; // paused — no interaction
  } else {
    c->r[4] = G;
    c->r[5] = item;
    c->r[6] = 1;
    c->r[31] = 0x800203A8u;
    psx::cpu::dispatchGuestToReturn0(*c, LEAF_PROX_STEP, psx::cpu::ExecutionBudget::currentTurn(*c), __func__);
    const int32_t v0 = (int32_t)c->r[2];
    if (v0 < 0) {
      result = 0; // no hit
    } else if (c->mem_r8(G + 0x144u) == 1 && v0 < 2) {
      // Just-transitioned state.
      if ((c->mem_r16(G + 0x17Eu) & 0x8000u) == 0) {
        c->r[4] = item;
        c->r[5] = 1;
        c->r[6] = 0x10;
        c->r[7] = 0x20;
        c->r[31] = 0x80020418u;
        psx::cpu::dispatchGuestToReturn0(*c, LEAF_ALT_TAG_SET, psx::cpu::ExecutionBudget::currentTurn(*c), __func__);
        result = 1;
      } else if (mode & 3u) {
        c->r[4] = G;
        c->r[5] = item;
        c->r[31] = 0x800203FCu;
        psx::cpu::dispatchGuestToReturn0(*c, LEAF_GROWN_PUSH, psx::cpu::ExecutionBudget::currentTurn(*c), __func__);
        result = 1;
      } else {
        result = 1;
      }
    } else {
      // Steady-state: optional trig-offset separation, then a mode-bit-keyed result/state stamp.
      // Heading is the FULL 32-bit word proximityCheck stamped into OUT_HEADING_SPAD (a raw
      // Trig::ratan2 result register width, not a 16-bit angle) — read as mem_r32 throughout.
      if (mode & 0x3Fu) {
        const int32_t heading = (int32_t)c->mem_r32(0x1F80009Cu); // OUT_HEADING_SPAD
        const int32_t cosv = trigOf(c).rcos(heading);
        const int32_t sinv = trigOf(c).rsin(heading);
        const int32_t sum80 = (int32_t)c->mem_r16s(G + 0x80u) + (int32_t)c->mem_r16s(item + 0x80u);
        const int16_t dx = (int16_t)((cosv * sum80) >> 12);
        const int16_t dz = (int16_t)((sinv * sum80) >> 12);
        if ((mode & 0x7Fu) == 1) {
          c->mem_w16(item + 0x2Eu, (uint16_t)((int16_t)c->mem_r16(G + 0x2Eu) - dx));
          c->mem_w16(item + 0x36u, (uint16_t)((int16_t)c->mem_r16(G + 0x36u) + dz));
        } else if ((c->mem_r8(G) & 4u) == 0) {
          c->mem_w16(G + 0x2Eu, (uint16_t)((int16_t)c->mem_r16(item + 0x2Eu) + dx));
          c->mem_w16(G + 0x36u, (uint16_t)((int16_t)c->mem_r16(item + 0x36u) - dz));
        }
      }
      // bVar6 (gen: `(byte)(_DAT_1f80009c >> 4)`) — truncate the 32-bit heading word, not a byte load.
      const uint8_t bVar6 = (uint8_t)((uint32_t)c->mem_r32(0x1F80009Cu) >> 4);
      if ((mode & 0x40u) == 0) {
        // mode&0x80 ladder — gate 0x1F80027A (proximityCheck's own "already consumed" guard).
        if (mode & 0x80u) {
          if (c->mem_r8(0x1F80027Au) != 0) {
            result = 2;
            goto done;
          }
          if (c->mem_r8(G + 4u) != 1) {
            result = 2;
            goto done;
          }
          if (c->mem_r8(G + 5u) != 0x13) {
            c->mem_w8(G + 5u, 0x13);
            c->mem_w8(G + 6u, 0);
            c->mem_w8(G + 7u, 0);
            c->mem_w8(G + 0x2Bu, bVar6);
            result = 3;
            goto done;
          }
        }
        result = 2;
      } else {
        uint8_t bVar3 = c->mem_r8(0x1F800137u); // PAUSE_FLAG_SPAD
        if (bVar3 == 0) {
          bVar3 = c->mem_r8(G) & 6u;
          if (bVar3 == 0) {
            bVar3 = c->mem_r8(item) & 2u;
            if (bVar3 == 0) {
              bVar3 = 4;
              c->mem_w8(G + 4u, 2);
              c->mem_w8(G + 5u, 2);
              c->mem_w8(G, 3);
              c->mem_w8(G + 6u, 0);
              c->mem_w16(G + 0x172u, 0x78u); // single u16 store covers both G+0x172(=0x78)/G+0x173(=0)
              c->mem_w8(G + 0x2Bu, bVar6);
            }
          }
        }
        result = bVar3;
      }
    }
  }
done:
  c->r[31] = c->mem_r32(c->r[29] + 32);
  c->r[19] = c->mem_r32(c->r[29] + 28);
  c->r[18] = c->mem_r32(c->r[29] + 24);
  c->r[17] = c->mem_r32(c->r[29] + 20);
  c->r[16] = c->mem_r32(c->r[29] + 16);
  c->r[29] = sp0;
  return result;
}

// FUN_800205CC — postInteractWalk case 8.
void ActorTomba::type8Interact(uint32_t item) {
  Core *c = core;
  const uint32_t G = G_ADDR;
  // Guest frame per abi_extract --contract 0x800205CC: single epilogue label -> RAII is safe.
  static constexpr GuestFrameSpill kSpills[] = {{17, 20}, {18, 24}, {31, 28}, {16, 16}};
  GuestFrame<32, 4> frameGuard(c, kSpills);
  TombaState tomba{c, G};
  TombaState other{c, item};

  if (c->mem_r8(item) == 5) {
    if ((tomba.growthFlags() & 0x200u) == 0 && tomba.frozenFlag() == 0) {
      tomba::guest::dispatchJalToReturn(*c, LEAF_NILADIC_CUE, 0x80020620u);
    }
  } else if (tomba.growthFlags() & 0x8000u) {
    tomba::guest::dispatchJalToReturn(*c, LEAF_GROWN_DELEGATE, 0x80020644u, G, item);
  } else {
    const int32_t v0 = (int32_t)tomba::guest::dispatchJalToReturn(*c, LEAF_PROX_STEP, 0x80020658u, G, item, 0u);
    if (v0 >= 0) {
      if (c->mem_r8(item) == 1) {
        if (tomba.justTransitioned() == 1 && v0 < 2) {
          tomba::guest::dispatchJalToReturn(*c, LEAF_ALT_TAG_SET, 0x8002069Cu, item, (uint32_t)-32766, 3u, 30u);
        } else if ((tomba.growthFlags() & 0x200u) == 0) {
          if ((v0 & 1) == 0) {
            if ((tomba.statusFlags() & 4u) == 0) {
              const int32_t heading = (int32_t)c->mem_r32(0x1F80009Cu); // full 32-bit word
              const int32_t cosv = trigOf(c).rcos(heading);
              const int32_t sinv = trigOf(c).rsin(heading);
              const int32_t sum80 = (int32_t)tomba.boundXZ() + (int32_t)other.boundXZ();
              tomba.setPosX((int16_t)(other.posX() + (int16_t)((cosv * sum80) >> 12)));
              tomba.setPosZ((int16_t)(other.posZ() - (int16_t)((sinv * sum80) >> 12)));
            }
            tomba.setGroundContactFlag(1);
            // Heading arg is the full 32-bit OUT_HEADING_SPAD word (Ghidra: `iVar7 = (int)_DAT_1f80009c`
            // — a straight int cast, no 16-bit truncation, matching stepModeInteract's bVar6 fix).
            const int32_t cmp = Trig::angleCmp((int32_t)c->mem_r32(0x1F80009Cu), (int32_t)tomba.facing(), 1);
            tomba.setFlag95((uint8_t)(cmp + 2));
          } else if (v0 == 1 && (tomba.groundedGate() & 1u) == 0) {
            // G+0x32 = item[0x32] - (G[0x84] + item[0x84]) (all u16, unsigned per gen), THEN
            // growthYSnap()'s own reset+gated-Y-resnap tail — this branch's G+0x29/0x145/0x4A/
            // 0x50/0x148 reset (v0==1 here) plus the G+0x78/DAT_800BF816-gated const-140/70 snap
            // on G+0x32 are BYTE-IDENTICAL to guest FUN_80022C78 (growthYSnap), reused rather than
            // duplicated (authenticated executable/overlay evidence lines 1-19 == authenticated executable/overlay
            // evidence lines 1-16).
            tomba.setPosY((int16_t)(other.posY() - (tomba.boundYUp() + other.boundYUp())));
            growthYSnap();
          }
        }
      } else if ((tomba.growthFlags() & 0x200u) == 0 && (tomba.groundedGate() & 1u) == 0) {
        c->mem_w8(item + 0x29u, 1);
      }
    }
  }
}

// FUN_800235A0 — postInteractWalk case 7.
uint8_t ActorTomba::type7Interact(uint32_t item) {
  Core *c = core;
  const uint32_t G = G_ADDR;
  // Guest frame per abi_extract --contract 0x800235A0: single epilogue label -> RAII is safe.
  static constexpr GuestFrameSpill kSpills[] = {{16, 16}, {17, 20}, {31, 24}};
  GuestFrame<32, 3> frameGuard(c, kSpills);
  TombaState tomba{c, G};

  const int32_t v0 = (int32_t)tomba::guest::dispatchJalToReturn(*c, LEAF_PROX_STEP, 0x800235C0u, G, item, 1u);
  uint8_t result = 0;
  if (v0 >= 0) {
    const uint32_t flag = (tomba.transitionSlot() == 0x0Cu) ? 4u : 1u;
    c->r[4] = G;
    c->r[5] = item;
    c->r[7] = flag;
    tomba::guest::dispatchJalToReturn(*c, LEAF_STEP_MODE_FLAG, 0x80023600u);
    result = 1;
  }
  return result;
}

// FUN_80022C78 — leaf, no guest-stack frame. Operates on G (postFrameWaterCheck's
// LEAF_WATER_SPLASH call site).
void ActorTomba::growthYSnap() {
  Core *c = core;
  const uint32_t G = G_ADDR;

  c->mem_w8(G + 0x29u, 1);
  c->mem_w8(G + 0x145u, 0);
  c->mem_w16(G + 0x4Au, 0);
  c->mem_w16(G + 0x50u, 0);
  c->mem_w8(G + 0x148u, 0);

  if (c->mem_r8(G + 0x78u) != 0) {
    return;
  }
  if (c->mem_r8(0x800BF816u) != 0) {
    return;
  }

  // BUG FIX (RE cross-check against authenticated executable/overlay evidence guest 0x80022C78): the ground
  // truth's `if (g17E<0) goto L_80022CD8` branch jumps to a block that explicitly re-sets r3=70
  // (0x46) for the comparison/subtraction constant; the FALLTHROUGH (g17E>=0) keeps r3=140
  // (0x8C) from the branch's own delay-slot preset. So g17E<0 -> 0x46, g17E>=0 -> 0x8C — the
  // original draft had this backwards. Same polarity bug was inlined at type8Interact's "just
  // left growth" tail (which reuses this constant indirectly via growthYSnap()), so this one fix
  // corrects both call sites.
  const int16_t g17E = (int16_t)c->mem_r16(G + 0x17Eu);
  const int16_t k = (g17E < 0) ? 0x46 : 0x8C;
  const int16_t g84 = (int16_t)c->mem_r16(G + 0x84u);
  if (g84 == k) {
    return; // no-op — already at the snap point
  }
  c->mem_w16(G + 0x32u, (uint16_t)(g84 + ((int16_t)c->mem_r16(G + 0x32u) - k)));
}

// =================================================================================
// Settle helper — velocityIntegrate's tail dispatch (FUN_80054650)
// =================================================================================
uint32_t ActorTomba::settleStep(int32_t mode) {
  Core *c = core;
  const uint32_t G = G_ADDR;

  c->mem_w8(0x1F800258u, 0);                                    // clear sink-mark
  c->mem_w8(G + 0x5Fu, (uint8_t)(c->mem_r8(G + 0x5Fu) & 0xFB)); // flag95 &= ~0x04

  if (c->mem_r8(G + 0x16Bu) != 0) {
    c->r[2] = 0;
    return 0;
  } // flag363 gate

  // Probe offset selector: mode==0 default 0x1E / 0x3C (when G+0x17E has high bit clear),
  // mode!=0 always 0.
  uint32_t probeOffset = 0;
  if (mode == 0) {
    probeOffset = ((int16_t)c->mem_r16(G + 0x17Eu) >= 0) ? 0x3Cu : 0x1Eu;
  }

  // Probe base: G+0x62 (u16) unless G+0x78 (state) != 0, in which case pull the "hooked item" at
  // G+0x10 and compute `(item[+0x86] - item[+0x84]) - (G+0x32 - item[+0x32])`.
  int32_t base;
  if (c->mem_r8(G + 0x78u) == 0) {
    base = (int32_t)c->mem_r16s(G + 0x62u);
  } else {
    const uint32_t item = c->mem_r32(G + 0x10u);
    base = ((int32_t)c->mem_r16s(item + 0x86u) - (int32_t)c->mem_r16s(item + 0x84u)) -
           ((int32_t)c->mem_r16s(G + 0x32u) - (int32_t)c->mem_r16s(item + 0x32u));
  }
  const int16_t half = (int16_t)(base / 2);

  auto probe = [&](int16_t offset) -> int32_t {
    c->r[4] = G;
    c->r[5] = probeOffset;
    c->r[6] = (uint32_t)(int32_t)offset;
    psx::cpu::dispatchGuestToReturn0(
        *c, 0x8004954Cu, psx::cpu::ExecutionBudget::currentTurn(*c), __func__); // grid probe
    return (int32_t)c->r[2];
  };

  if (probe(half) != 0 || probe((int16_t)(-half)) != 0) {
    uint8_t bV = (uint8_t)(c->mem_r8(G + 0x149u) & 1);
    if ((c->mem_r8(G + 0x149u) & 4) == 0) {
      bV = c->mem_r8(G + 0x147u);
    }
    c->mem_w8(G + 0x60u, 1);
    c->mem_w8(G + 0x5Fu, (uint8_t)(bV + 4));
    c->r[2] = 1;
    return 1;
  }

  // No probe hit — check the sink-mark and fall through with 0.
  if (c->mem_r8(0x1F800258u) != 0) {
    const int8_t v = (int8_t)(5 - (int8_t)c->mem_r8(G + 0x147u));
    c->mem_w8(G + 0x5Fu, (uint8_t)v);
  }
  c->r[2] = 0;
  return 0;
}

// =================================================================================
// Movement — velocity integrate (FUN_80056B48)
// =================================================================================
void ActorTomba::velocityIntegrate(bool suppressY) {
  Core *c = core;
  const uint32_t G = G_ADDR;

  const int32_t speed = c->mem_r16s(G + 0x44u);
  const int32_t dirX = c->mem_r16s(G + 0x48u);
  const int32_t dirZ = c->mem_r16s(G + 0x4Cu);
  c->mem_w32(G + 0x2Cu, c->mem_r32(G + 0x2Cu) + (uint32_t)(dirX * speed)); // posX
  c->mem_w32(G + 0x34u, c->mem_r32(G + 0x34u) + (uint32_t)(dirZ * speed)); // posZ

  if (!suppressY) {
    const int32_t dirY = c->mem_r16s(G + 0x4Au);
    c->mem_w32(G + 0x30u, c->mem_r32(G + 0x30u) + (uint32_t)(dirY * speed)); // posY
  }

  // Tail: settle-helper dispatch OR flag95 &= ~0x04.
  if (c->mem_r8(G + 0x16Bu) == 0 && c->mem_r8(G + 0x61u) == 0) {
    settleStep(0); // native FUN_80054650
  } else {
    const uint8_t f = (uint8_t)(c->mem_r8(G + 0x5Fu) & 0xFB);
    c->mem_w8(G + 0x5Fu, f);
    c->r[2] = f;
  }
}

void ActorTomba::mode0ActionGate() {
  Core *c = core;
  const uint32_t G = G_ADDR;
  static constexpr GuestFrameSpill kSpills[] = {{31, 16}};
  GuestFrame<24, 1> frame(c, kSpills);
  bool pathA = c->mem_r8(0x800BF816u) != 0                // water mode on
               || c->mem_r8(G + 0x17Cu) == 0              // action-enable byte clear
               || (c->mem_r16(G + 0x17Eu) & 0x640u) != 0; // a suppress bit set
  if (pathA) {
    tomba::guest::dispatchJalToReturn(*c, 0x8005A970u, 0x8005A950u); // normal handler (direct same-shard in gen)
  } else {
    tomba::guest::dispatchJalToReturn(*c, 0x80112B50u, 0x8005A960u); // swim/water-interaction handler
  }
}

void ActorTomba::ov_stepModeInteract(Core *c) {
  const uint32_t item = c->r[5];
  const uint32_t mode = c->r[6];
  c->r[2] = eng(c).actorTomba.stepModeInteract(item, mode);
}
void ActorTomba::ov_type8Interact(Core *c) {
  const uint32_t item = c->r[5];
  eng(c).actorTomba.type8Interact(item);
}
void ActorTomba::ov_type7Interact(Core *c) {
  const uint32_t item = c->r[5];
  c->r[2] = eng(c).actorTomba.type7Interact(item);
}
void ActorTomba::ov_growthYSnap(Core *c) {
  eng(c).actorTomba.growthYSnap();
}

void ActorTomba::ov_frameTick(Core *c) {
  eng(c).actorTomba.frameTick();
}

// ov_turnBiasCompute/ov_outerTransitionGate/ov_outerTransitionCommit/ov_assetReady — guest ABI
// trampolines for the frameTick sub-callee cluster (§9 re-verified + wired 2026-07-10). Guest ABI
// per the cited guest instructions (see the definitions above for the cited call sites): turnBiasCompute takes
// facing in a1 (a0=G is unused — the guest body never reads r4 in this leaf); outerTransitionGate/
// outerTransitionCommit always operate on Tomba's single fixed G block (a0=G is always G_ADDR, so
// the instance methods read G_ADDR directly rather than c->r[4]); outerTransitionCommit takes mode
// in a1; assetReady takes slot in a0 (NOT a1 — guest 0x80045580 uses r4<<3 directly).
void ActorTomba::ov_turnBiasCompute(Core *c) {
  turnBiasCompute(c, (int16_t)c->r[5]);
}
void ActorTomba::ov_outerTransitionGate(Core *c) {
  c->r[2] = eng(c).actorTomba.outerTransitionGate() ? 1u : 0u;
}
void ActorTomba::ov_outerTransitionCommit(Core *c) {
  eng(c).actorTomba.outerTransitionCommit((int32_t)c->r[5]);
}
void ActorTomba::ov_assetReady(Core *c) {
  c->r[2] = assetReady(c, (int32_t)c->r[4]) ? 1u : 0u;
}

void ActorTomba::gov_turnBiasCompute(Core *c) {
  ov_turnBiasCompute(c);
}
void ActorTomba::gov_outerTransitionGate(Core *c) {
  ov_outerTransitionGate(c);
}
void ActorTomba::gov_outerTransitionCommit(Core *c) {
  ov_outerTransitionCommit(c);
}
void ActorTomba::gov_assetReady(Core *c) {
  ov_assetReady(c);
}

void ActorTomba::gov_mode0ActionGate(Core *c) {
  eng(c).actorTomba.mode0ActionGate();
}
void ActorTomba::registerOverrides(Game * /*game*/) {
  // typed runtime address dispatch-only postInteractWalk sub-handlers + frameTick (no direct same-module caller ->
  // setter omitted). turnBiasCompute/outerTransitionGate/outerTransitionCommit/assetReady are
  // dual-wired via tomba::native::declareOverride below (direct callers exist).
  tomba::native::declareOverride(0x80020364u, "ActorTomba::stepModeInteract", ov_stepModeInteract);
  tomba::native::declareOverride(0x800205CCu, "ActorTomba::type8Interact", ov_type8Interact);
  tomba::native::declareOverride(0x800235A0u, "ActorTomba::type7Interact", ov_type7Interact);
  tomba::native::declareOverride(0x80022C78u, "ActorTomba::growthYSnap", ov_growthYSnap);
  tomba::native::declareOverride(0x8005950Cu, "ActorTomba::frameTick", ov_frameTick);

  tomba::native::declareOverride(0x80055C9Cu, "gov_turnBiasCompute", gov_turnBiasCompute);
  tomba::native::declareOverride(0x80053E50u, "gov_outerTransitionGate", gov_outerTransitionGate);
  tomba::native::declareOverride(0x80053FDCu, "gov_outerTransitionCommit", gov_outerTransitionCommit);
  tomba::native::declareOverride(0x80045580u, "gov_assetReady", gov_assetReady);
  tomba::native::declareOverride(0x8005A910u, "gov_mode0ActionGate", gov_mode0ActionGate);
}

// turnBiasCompute — guest FUN_80055C9C. See actor_tomba.h for the full RE writeup. Frameless leaf
// (frame_size=0 per abi_extract) — no stack, purely fixed-address reads + a bias-pair write.
namespace {
constexpr uint32_t UI_MODE_BYTE = 0x800E806Cu;      // ==5 selects the wide/menu delta formula
constexpr uint32_t VIEW_HEADING_SPAD = 0x1F8000F2u; // cached view heading, subtracted from facing
constexpr uint32_t CLOSE_MASK_WORD = 0x800E805Au;   // bit 0x800 widens the "close" threshold
constexpr uint32_t TURN_BIAS_IN_SPAD = 0x1F80016Cu;
constexpr uint32_t TURN_BIAS_OUT_SPAD = 0x1F80016Eu;
} // namespace
void ActorTomba::turnBiasCompute(Core *c, int16_t facing) {
  bool closeIn;
  if (c->mem_r8(UI_MODE_BYTE) == 5) {
    // Wide/menu variant: delta from a fixed 0xC00(3072) reference minus the cached view heading
    // and facing.
    const uint32_t d = (3072u - c->mem_r16(VIEW_HEADING_SPAD) - (uint32_t)(int32_t)facing) & 4095u;
    closeIn = (int32_t)d < 2048;
  } else {
    uint32_t r3 = (3072u - (uint32_t)(int32_t)facing) & 4095u;
    const uint32_t r2m = c->mem_r16(VIEW_HEADING_SPAD) & 4095u;
    r3 = r3 - r2m;
    const uint32_t r4 = ((int16_t)r3 < 0) ? r3 : (r3 - 512u);
    const uint32_t d = r4 & 4095u;
    if (c->mem_r16(CLOSE_MASK_WORD) & 0x800u) {
      closeIn = (int32_t)d < 2560;
    } else {
      closeIn = (int32_t)d < 1536;
    }
  }
  if (closeIn) {
    c->mem_w16(TURN_BIAS_IN_SPAD, 128);
    c->mem_w16(TURN_BIAS_OUT_SPAD, 32);
  } else {
    c->mem_w16(TURN_BIAS_IN_SPAD, 32);
    c->mem_w16(TURN_BIAS_OUT_SPAD, 128);
  }
}

// resetLoadGate — guest FUN_80042310. See actor_tomba.h for the full RE writeup. Guest frame
// (abi_extract --contract): 24 B, ra@sp+16 only.
void ActorTomba::resetLoadGate(Core *c) {
  static constexpr GuestFrameSpill kSpills[] = {{31, 16}};
  GuestFrame<24, 1> frameGuard(c, kSpills);

  tomba::guest::dispatchLeafToReturn(*c, 0x8001CF78u);                            // niladic cue
  tomba::guest::dispatchJalToReturn(*c, 0x80074590u, 0x80042320u, 0x7Fu, 0u, 0u); // FUN_80074590(0x7F, 0, 0)
  const uint8_t areaMode = c->mem_r8(0x800BF870u);                                // read before the unpause write below
  c->mem_w8(0x1F800137u, 0);                                                      // unpause
  tomba::guest::dispatchJalToReturn(*c, 0x80074F24u, 0x80042320u, areaMode);      // FUN_80074F24(DAT_800BF870)
}

// assetReady — guest FUN_80045580. See actor_tomba.h for the full RE writeup. Guest frame: 24 B,
// ra@sp+16 only.
bool ActorTomba::assetReady(Core *c, int32_t slot) {
  static constexpr GuestFrameSpill kSpills[] = {{31, 16}};
  GuestFrame<24, 1> frameGuard(c, kSpills);

  constexpr uint32_t TABLE_8018A000 = 0x8018A000u;
  constexpr uint32_t DAT_800A3EC8 = 0x800A3EC8u;
  constexpr uint32_t SLOT_TABLE = 0x800BE118u; // &DAT_800be11c - 4 (slot*8 + 4 == +0xC)
  const uint32_t rec = c->mem_r32(SLOT_TABLE + (uint32_t)slot * 8u + 4u);
  const bool ready = tomba::guest::dispatchJalToReturn(
                         *c, 0x80044CD4u, 0x800455B0u, TABLE_8018A000, c->mem_r32(DAT_800A3EC8), rec) > 0;
  return ready;
}

// outerTransitionGate — guest FUN_80053E50(G). See actor_tomba.h for the full RE writeup. Guest
// frame (abi_extract --contract): 32 B; s0/s1/s2/ra spilled but never written by this body — pure
// passthrough preservation of the caller's values, which GuestFrame reproduces for free.
namespace {
constexpr uint32_t BUSY_LATCH_HI = 0x800BF81Eu;
constexpr uint32_t BUSY_LATCH = 0x800BF80Du;        // global "outer transition busy" latch
constexpr uint32_t LEAF_CUE_800521F4 = 0x800521F4u; // transition-cue dispatch (4 call sites)
constexpr uint32_t LEAF_WALK_RESET = 0x80053D90u;   // walk-state reset leaf
constexpr uint32_t LEAF_STOPMOTION = 0x800312D4u;   // stop-motion task spawn (dest ptr, magnitude)
} // namespace
bool ActorTomba::outerTransitionGate() {
  Core *c = core;
  const uint32_t G = G_ADDR;
  static constexpr GuestFrameSpill kSpills[] = {{16, 16}, {17, 20}, {18, 24}, {31, 28}};
  GuestFrame<32, 4> frameGuard(c, kSpills);
  TombaState tomba{c, G};

  if (tomba.turnCurrent() > 0) {
    return false; // still mid-turn — nothing to do yet
  }

  c->mem_w8(BUSY_LATCH_HI, 0);
  eng(c).gStateMutate(G, 0xB);

  if (tomba.transitionSlot() == 1) {
    if ((tomba.statusFlags() & 4u) == 0) {
      if ((tomba.latchFlags() & 0x80u) == 0) {
        tomba::guest::dispatchJalToReturn(*c, LEAF_CUE_800521F4, 0x80053F14u, 0u, 0x81u, 0x81u, 0x0Fu);
      }
      tomba.setLatchFlags(0);
      tomba.setStopMotionAux(0);
      tomba::guest::dispatchJalToReturn(*c, LEAF_WALK_RESET, 0x80053F24u, G);
      tomba.setStatusFlags(3);
      tomba.setTurnCurrent(0);
      tomba.setTurnTarget(0);
      tomba.setTurnSuppressGate(0);
      tomba.setOuterState(2);
      tomba.setLoadStep(1);
      tomba.setLoadSub(0);
      c->mem_w8(BUSY_LATCH, 1);
      tomba::guest::dispatchJalToReturn(*c, LEAF_STOPMOTION, 0x80053FC0u, 6u, tomba.posAddr(), (uint32_t)-80);
      return true;
    }
    if ((tomba.latchFlags() & 0x80u) != 0) {
      return true;
    }
    tomba::guest::dispatchJalToReturn(*c, LEAF_CUE_800521F4, 0x80053ED8u, 0u, 0x81u, 0x81u, 0x0Fu);
    tomba.setLatchFlags((uint8_t)(tomba.latchFlags() | 0x82u));
    return true;
  }

  if (c->mem_r8s(BUSY_LATCH) < 1) {
    tomba::guest::dispatchJalToReturn(*c, LEAF_CUE_800521F4, 0x80053F74u, 0u, 0x81u, 0x81u, 0x0Fu);
    tomba.setLatchFlags(0);
    tomba.setStopMotionAux(0);
    tomba::guest::dispatchJalToReturn(*c, LEAF_WALK_RESET, 0x80053F84u, G);
    tomba.setStatusFlags(3);
    tomba.setTurnCurrent(0);
    tomba.setTurnTarget(0);
    tomba.setTurnSuppressGate(0);
    tomba.setExtraClear(0); // extra clear only on this path (verified vs shard)
    tomba.setOuterState(2);
    tomba.setLoadStep(1);
    tomba.setLoadSub(0);
    c->mem_w8(BUSY_LATCH, 1);
    tomba::guest::dispatchJalToReturn(*c, LEAF_STOPMOTION, 0x80053FC0u, 6u, tomba.posAddr(), (uint32_t)-80);
  }
  // else (busy latch already set): nothing to do.
  return true;
}

// outerTransitionCommit — guest FUN_80053FDC(G, mode). See actor_tomba.h for the full RE writeup
// (incl. the Ghidra-vs-ground-truth correction in the decrement/settle tail below). Guest frame:
// 32 B; s0(r16)/s1(r17)/ra spilled but, like outerTransitionGate, never written by this body —
// pure passthrough (no s2 slot here, a smaller frame than outerTransitionGate's).
void ActorTomba::outerTransitionCommit(int32_t mode) {
  Core *c = core;
  const uint32_t G = G_ADDR;
  static constexpr GuestFrameSpill kSpills[] = {{16, 16}, {17, 20}, {31, 24}};
  GuestFrame<32, 3> frameGuard(c, kSpills);
  TombaState tomba{c, G};

  c->r[31] = 0x80053FF8u;
  if (outerTransitionGate()) {
    return;
  }

  if (tomba.turnCurrent() != tomba.turnTarget()) {
    // "reset to new target" — cue + gStateMutate(0xB) + conditional stop-motion clear.
    tomba::guest::dispatchJalToReturn(*c, LEAF_CUE_800521F4, 0x80054024u, 0u, 0x81u, 0x81u, 0x0Fu);
    eng(c).gStateMutate(G, 0xB);
    c->mem_w8(BUSY_LATCH_HI, 0);
    if ((tomba.statusFlags() & 4u) == 0) {
      tomba::guest::dispatchJalToReturn(*c, LEAF_WALK_RESET, 0x80054054u, G);
      tomba.setStopMotionAux(0);
    }
    if (mode != 1 && tomba.outerState() == 2) {
      return; // already committing — nothing to do
    }
    // Commit a new target (shared by mode==1 and the mode!=1-but-not-yet-committing path).
    tomba.setSettleCounter(0x5A);
    tomba.setTurnTarget((uint16_t)tomba.turnCurrent());
    tomba.setLatchFlags((uint8_t)(tomba.latchFlags() | 0x82u));
    if ((tomba.statusFlags() & 0xCu) != 0) {
      tomba::guest::dispatchJalToReturn(*c, 0x80074590u, 0x80054108u, 0x23u, 0u, 0u);
      tomba::guest::dispatchJalToReturn(*c, LEAF_STOPMOTION, 0x80054118u, 6u, tomba.posAddr(), (uint32_t)-80);
      return;
    }
    tomba.setStatusFlags(3);
    tomba.setTurnSuppressGate(0);
    c->mem_w8(G + 0x145u, 0);
    tomba.setOuterState(2);
    tomba.setLoadStep(0);
    tomba.setLoadSub(0);
    return;
  }

  // Pending counter already at target — decrement-and-settle path.
  const int16_t remaining = tomba.settleCounter();
  if (remaining == 0) {
    return;
  }
  const int16_t newRemaining = (int16_t)(remaining - 1);
  tomba.setSettleCounter((uint16_t)newRemaining);
  if (newRemaining != 0) {
    return;
  }

  const uint8_t g0 = tomba.statusFlags();
  if (g0 != 2 && (g0 & 4u) == 0) {
    // "unobstructed" — commit to walk-state 1, clearing/masking the stop-motion latch bits.
    tomba.setStatusFlags(1);
    if ((tomba.latchFlags() & 0x50u) != 0) {
      tomba.setLatchFlags((uint8_t)(tomba.latchFlags() & 0x7Fu));
    } else {
      tomba.setLatchFlags(0);
    }
  } else {
    // g0==2 OR (g0&4)!=0 — re-arm the settle counter instead of committing.
    tomba.setSettleCounter(1);
  }
}

namespace {
constexpr uint32_t TURN_SUPPRESS_A = 0x800E7E68u; // "turn-suppress mask" pair, also
constexpr uint32_t TURN_SUPPRESS_B = 0x800ECF54u; // written by the enemy-engage tables
constexpr uint32_t TURN_SUPPRESS_CLEAR_GATE = 0x1F800230u;
constexpr uint32_t TURN_SUPPRESS_MASK_SPAD = 0x1F800174u;
constexpr uint32_t CUTSCENE_FLAG = 0x800BF80Fu;
constexpr uint32_t FRAME_PAUSE_FLAG = 0x1F800137u;
constexpr uint32_t CASE4_MASK_SRC_A = 0x1F800166u; // case-4's alt source for TURN_SUPPRESS_A
constexpr uint32_t CASE4_MASK_SRC_B = 0x1F800190u; // case-4's alt source for TURN_SUPPRESS_B
constexpr uint32_t TURN_SUPPRESS_ACTIVE_OUT = 0x1F800232u;
constexpr uint32_t LOAD_KICK_GATE_SPAD = 0x1F80019Bu;
constexpr uint32_t LOAD_KICK_MODE_BYTE = 0x800BF89Cu;
constexpr uint32_t ANIM_PTR_SPAD = 0x1F800138u;
} // namespace
void ActorTomba::frameTick() {
  Core *c = core;
  const uint32_t G = c->r[4]; // a0 (== G_ADDR from both callers; matches gen's r16=r4+r0)
  static constexpr GuestFrameSpill kSpills[] = {{16, 16}, {31, 28}, {18, 24}, {17, 20}};
  GuestFrame<32, 4> frameGuard(c, kSpills);
  GuestReg<16> gReg(c);
  gReg = G;
  TombaState tomba{c, G};

  const uint8_t outerState = tomba.outerState();
  if (outerState < 8) {
    switch (outerState) {
    case 0: {
      tomba::guest::dispatchJalToReturn(*c, 0x80058648u, 0x80059560u, G, 0u); // enterOuterState0
      break;
    }
    case 1: {
      const uint16_t savedA = c->mem_r16(TURN_SUPPRESS_A);
      const uint16_t savedB = c->mem_r16(TURN_SUPPRESS_B);
      GuestReg<18> r18Reg(c);
      r18Reg = savedA;
      GuestReg<17> r17Reg(c);
      r17Reg = savedB;
      if (c->mem_r8(TURN_SUPPRESS_CLEAR_GATE) != 0) {
        const uint16_t mask = c->mem_r16(TURN_SUPPRESS_MASK_SPAD);
        c->mem_w16(TURN_SUPPRESS_B, (uint16_t)(savedB & ~mask));
        c->mem_w16(TURN_SUPPRESS_A, (uint16_t)(savedA & ~mask));
      }
      // GT clears the turn-suppress pair unless BOTH CUTSCENE_FLAG==0 AND FRAME_PAUSE_FLAG==0
      // (the wide-RE draft inverted the CUTSCENE_FLAG condition — fixed).
      const bool skipClear = (c->mem_r8(CUTSCENE_FLAG) == 0) && (c->mem_r8(FRAME_PAUSE_FLAG) == 0);
      if (!skipClear) {
        c->mem_w16(TURN_SUPPRESS_A, 0);
        c->mem_w16(TURN_SUPPRESS_B, 0);
      }

      tomba::guest::dispatchJalToReturn(
          *c, 0x80055C9Cu, 0x800595DCu, G, (uint32_t)(int32_t)tomba.facing()); // turnBiasCompute
      tomba::guest::dispatchJalToReturn(*c, 0x80058918u, 0x800595E4u, G);      // mode-N dispatch table A
      if (tomba.turnSuppressGate() == 0) {
        c->mem_w8(TURN_SUPPRESS_ACTIVE_OUT, 1);
      }
      tomba::guest::dispatchJalToReturn(*c, 0x800597ACu, 0x80059604u, G);     // matrix-compose
      tomba::guest::dispatchJalToReturn(*c, 0x80053FDCu, 0x80059610u, G, 0u); // outerTransitionCommit (mode=0)

      c->mem_w16(TURN_SUPPRESS_A, savedA);
      c->mem_w16(TURN_SUPPRESS_B, savedB);
      break;
    }
    case 2: {
      tomba.setCommitting(1);
      tomba::guest::dispatchJalToReturn(*c, 0x80067CA4u, 0x80059628u, G);
      tomba::guest::dispatchJalToReturn(*c, 0x800597ACu, 0x800596D8u, G);
      break;
    }
    case 3:
      break; // unused jump-table slot (jump target = epilogue) — no-op
    case 4: {
      const uint16_t savedA = c->mem_r16(TURN_SUPPRESS_A);
      const uint16_t savedB = c->mem_r16(TURN_SUPPRESS_B);
      GuestReg<18> r18Reg(c);
      r18Reg = savedA;
      GuestReg<17> r17Reg(c);
      r17Reg = savedB;
      tomba.setLatchFlags((uint8_t)(tomba.latchFlags() & 0x7Fu));
      if (c->mem_r8(FRAME_PAUSE_FLAG) != 0) {
        c->mem_w16(TURN_SUPPRESS_A, c->mem_r16(CASE4_MASK_SRC_A));
        c->mem_w16(TURN_SUPPRESS_B, c->mem_r16(CASE4_MASK_SRC_B));
      } else {
        c->mem_w16(TURN_SUPPRESS_A, 0);
        c->mem_w16(TURN_SUPPRESS_B, 0);
      }
      tomba::guest::dispatchJalToReturn(
          *c, 0x80055C9Cu, 0x8005968Cu, G, (uint32_t)(int32_t)tomba.facing()); // turnBiasCompute
      tomba::guest::dispatchJalToReturn(*c, 0x80058F5Cu, 0x80059694u, G);      // mode-N dispatch table B
      tomba::guest::dispatchJalToReturn(*c, 0x800597ACu, 0x8005969Cu, G);      // matrix-compose
      tomba::guest::dispatchJalToReturn(*c, 0x80053E50u, 0x800596A4u, G);      // outerTransitionGate (bare tick)

      c->mem_w16(TURN_SUPPRESS_A, savedA);
      c->mem_w16(TURN_SUPPRESS_B, savedB);
      break;
    }
    case 5: {
      tomba::guest::dispatchJalToReturn(*c, 0x8018BD30u, 0x800596C0u, G); // scripted leaf (overlay)
      tomba::guest::dispatchJalToReturn(*c, 0x800597ACu, 0x800596D8u, G);
      break;
    }
    case 6: {
      tomba::guest::dispatchJalToReturn(*c, 0x8018BE40u, 0x800596D0u, G); // scripted leaf (overlay)
      tomba::guest::dispatchJalToReturn(*c, 0x800597ACu, 0x800596D8u, G);
      break;
    }
    case 7: {
      const uint8_t sub = tomba.loadStep();
      GuestReg<17> r17Reg(c);
      r17Reg = sub;
      GuestReg<18> r18Reg(c);
      r18Reg = 1u;
      bool advance = false;
      if (sub == 0) {
        tomba::guest::dispatchJalToReturn(*c, 0x8001CF2Cu, 0x80059720u, G); // engine tick cue
        advance = true;
      } else if (sub == 1) {
        advance = tomba::guest::dispatchJalToReturn(*c, 0x80045580u, 0x80059730u, 1u) != 0; // assetReady
      } else if (sub == 2) {
        if (c->mem_r8(LOAD_KICK_GATE_SPAD) != 0) {
          c->mem_w8(LOAD_KICK_MODE_BYTE, 4);
          tomba::guest::dispatchJalToReturn(*c, 0x80042310u, 0x80059768u, G); // resetLoadGate
          tomba.setOuterState(1);
          tomba.setLoadStep(0);
          tomba.setLoadSub(0);
          tomba.setLoadSub2(0);
          const uint32_t anim = c->mem_r32(ANIM_PTR_SPAD);
          c->mem_w16(anim + 0x4Cu, (uint16_t)sub); // sub==2 — matches gen's r17 trace
          c->mem_w16(anim + 0x4Eu, 1);
        }
      }
      if (advance) {
        tomba.setLoadStep((uint8_t)(tomba.loadStep() + 1));
      }
      tomba::guest::dispatchJalToReturn(*c, 0x80076D68u, 0x80059794u, G); // Animation::step
      break;
    }
    }
  }
}
