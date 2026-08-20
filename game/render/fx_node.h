// game/render/fx_node.h — the TYPE-0x20 RENDER-NODE HEADER lens, and nothing else.
//
// A type-0x20 node's header is WALK-OWNED: render_walk.cpp reads it to decide whether and how to
// dispatch, so those fields mean the same thing for every node in the game. Everything from +0x2C
// onward is CONTROLLER-OWNED, and its meaning is a property of the render fn at +0x18 — not of the
// family. The same byte is an OT bias in one controller, a base RADIUS in the next and an anchor Y in
// a third:
//
//     +0x2C  packed VX|VY (sprite family) · fieldAnchorX (dot haze) · plane world position
//     +0x2E  anchor Y in the PACKED form, but anchor X in the WIDE form — the two overlap in memory
//     +0x32  OT bias (most members) · base RADIUS (fxRingSpriteRender) · wide-anchor Y
//     +0x34  8-byte record list · packed 8.8 scale pair · the ring's 21-item table
//     +0x50  particle array base · wind magnitude · LCG SEED (dot haze, motes) · X scale numerator
//     +0x60  alt scale pair · jet mode selector · packed anchor · 8-point chain array
//
// SO THERE IS DELIBERATELY NO family-level `anchor()` OR `otBias()` HERE. A single lens spanning
// those slots would hand back a plausible, wrong number for half the family and nothing would crash —
// worse than the raw offsets it replaced. This lens therefore STOPS at the walk-owned header, and a
// controller that needs more declares its OWN lens deriving from this one, naming its own fields.
// That is the whole design: shared where the meaning is shared, per-controller where it is not.
//
// Sign-extension goes through Core::mem_r16s / mem_r8s (they already exist) — no producer should
// write `(int16_t)c->mem_r16(...)` again.
#pragma once
#include "core.h"
#include <cstdint>

class FxNode {
public:
  static constexpr uint8_t kTypeCustomRenderFn = 0x20u;

  FxNode(Core *c, uint32_t at) : mCore(c), mAt(at) {}

  uint32_t addr() const {
    return mAt;
  }
  Core *core() const {
    return mCore;
  }

  // ---- walk-owned header: identical meaning for every type-0x20 node ----------------------------
  bool visible() const {
    return mCore->mem_r8(mAt + 0x01u) != 0;
  } // per-frame marker the walk gates on
  uint8_t variant() const {
    return mCore->mem_r8(mAt + 0x03u);
  } // selector byte; its VALUES are per controller
  uint8_t state() const {
    return mCore->mem_r8(mAt + 0x04u);
  }
  uint8_t subState() const {
    return mCore->mem_r8(mAt + 0x05u);
  }
  uint8_t type() const {
    return mCore->mem_r8(mAt + 0x0Bu);
  }
  uint32_t renderFn() const {
    return mCore->mem_r32(mAt + 0x18u);
  }
  uint32_t next() const {
    return mCore->mem_r32(mAt + 0x24u);
  }

protected:
  // The ONLY way a derived controller lens reaches its own fields — so every such read is NAMED in
  // the derived class instead of being an anonymous offset at the call site.
  int32_t s16(uint32_t off) const {
    return mCore->mem_r16s(mAt + off);
  }
  uint32_t u16(uint32_t off) const {
    return mCore->mem_r16(mAt + off);
  }
  int32_t s8(uint32_t off) const {
    return mCore->mem_r8s(mAt + off);
  }
  uint32_t u8(uint32_t off) const {
    return mCore->mem_r8(mAt + off);
  }
  uint32_t u32(uint32_t off) const {
    return mCore->mem_r32(mAt + off);
  }

  Core *mCore;
  uint32_t mAt;
};
