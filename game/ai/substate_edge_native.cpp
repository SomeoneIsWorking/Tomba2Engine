// Native assembly child-oscillator driver, guest entry 0x801316CC in the A00 overlay.
// The authored AssemblyNode loop preserves its guest frame and live registers across runtime calls.
#include "substate_edge_native.h"
#include "assembly_node.h"
#include "core.h"
#include "guest_call.h"
#include "native_override_catalog.h"

namespace {
constexpr uint32_t kOscFrame = 32;
constexpr uint32_t kOscSpillSlot = 16;
constexpr uint32_t kOscSpillNode = 20;
constexpr uint32_t kOscSpillRa = 24;
constexpr uint32_t kRaAfterPartTick = 0x80131728u;
constexpr int32_t kFirstDrivenPart = 2;
constexpr int32_t kPartLimit = 4;
} // namespace

void SubstateEdgeLeaves::tickChildOscillators(Core *c) {
  // Guest frame, mirrored exactly: sp descends 32, r17/ra/r16 spill at +20/+24/+16 in that order.
  const uint32_t sp0 = c->r[29];
  c->r[29] = sp0 - kOscFrame;
  c->mem_w32(c->r[29] + kOscSpillNode, c->r[17]);
  c->r[17] = c->r[4]; // node stays in r17 for the whole body — see below
  c->mem_w32(c->r[29] + kOscSpillRa, c->r[31]);
  c->mem_w32(c->r[29] + kOscSpillSlot, c->r[16]);

  // LIVE-REGISTER LAW (docs/findings/sbs.md, game/render/subpart_walk.cpp): the sub-part tick is
  // runtime guest code and its prologue SPILLS its incoming r16/r17 into its own guest frame. So the
  // loop counter and the node pointer are guest-visible state at the call, not bookkeeping — they
  // live in the guest registers and are only NAMED here.
  uint32_t &partIndex = c->r[16]; // the guest's k: 2, then 3
  const AssemblyNode node(c, c->r[17]);

  partIndex = kFirstDrivenPart;
  if (node.hasOscillatingParts()) {
    for (;;) {
      // slot = k*4 - 4, one lower when the assembly is NOT in pair mode. The <<16 >>14 the guest
      // writes is sext16(k) * 4; the shifts are the sign-extension, not a scale trick.
      const int32_t k = (int16_t)(uint16_t)partIndex;
      const int32_t slot = k * 4 - 4 - (node.oscillatorPairMode() ? 0 : 1);

      c->r[4] = node.addr();
      c->r[5] = (uint32_t)(int32_t)(int16_t)(uint16_t)slot;
      c->r[31] = kRaAfterPartTick;
      psx::cpu::dispatchGuestToReturn0(*c,
                                       0x80130D5Cu,
                                       psx::cpu::ExecutionBudget::currentTurn(*c),
                                       __func__); // the per-sub-part oscillator, runtime guest code

      // Re-read the config word AFTER the call — the tick above can clear pair mode, and when it is
      // clear this loop runs exactly once. Caching it across the call would change behaviour.
      if (!node.oscillatorPairMode()) {
        break;
      }
      partIndex = (uint32_t)(int32_t)(int16_t)(uint16_t)(partIndex + 1);
      if ((int16_t)(uint16_t)partIndex >= kPartLimit) {
        break;
      }
    }
  }

  c->r[31] = c->mem_r32(c->r[29] + kOscSpillRa);
  c->r[17] = c->mem_r32(c->r[29] + kOscSpillNode);
  c->r[16] = c->mem_r32(c->r[29] + kOscSpillSlot);
  c->r[29] = sp0;
}

void SubstateEdgeLeaves::registerOverrides(Game *) {
  tomba::native::declareOverride(
      0x801316CCu, "SubstateEdgeLeaves::tickChildOscillators", &SubstateEdgeLeaves::tickChildOscillators);
}
