// game/render/minimap.cpp — NATIVE PRODUCER for the field HUD MINIMAP (kanban #43).
//
// The symptom: under pc_render areas 2 and 7 lost the bottom-right minimap that psx_render draws. It was
// not a mystery gap — field_hud.cpp said so in a comment and bailed: the HUD dispatcher FUN_80025D98
// routes mode 2 to 0x80113628 and mode 7 to 0x801140A0, both OVERLAY-RESIDENT drawers, and neither had a
// native producer, so pc_render (which does not walk the guest OT) drew nothing.
//
// RE — Ghidra on live area-2 / area-7 RAM dumps (scratch/decomp/minimap.c, minimap7.c) plus the guest's
// own dispatcher (game/core/field_owned_leaves.cpp leaf_80025D98, L_80025F0C / L_80025F40). The two
// drawers are the SAME routine compiled into each area's overlay with different data — each emits three
// packets into the near HUD bucket:
//   * a DR_MODE (FUN_80083DE0(p,0,0,tpage,0)) selecting the area's map texture page;
//   * one 64x64 RAW textured rect (GP0 0x65) — the map image, at a fixed screen corner;
//   * one 8x8 colour-modulated rect (GP0 0x64) — the BLINKING PLAYER DOT, positioned by mapping the
//     player's world position (scratchpad 0x1F800160 / 0x1F800164, s16) through the area's own linear
//     map transform: /0xB6 per map pixel, plus a per-area origin, with the axes and signs the area's
//     map orientation needs. The blink is the frame counter's bit 1: (0xFF,0x20,0x20) / (0x80,0x40,0x40).
// The OT insert order (dot, map, mode — LIFO into the same bucket) means the guest paints mode, then map,
// then dot; this producer submits them in that painted order.
//
// PORTED, NOT TAPPED: nothing here runs a gen body — the producer reads the player's world position and
// the area's own map constants and emits two host quads. Read-only: no guest write. The layer is
// RQ_OVERLAY (HUD chrome sits one band BELOW glyph text at RQ_HUD, so dialogue never ends up under it).
#include "core.h"
#include "game.h"
#include "render.h"
#include "render_queue.h"
#include <stdint.h>

namespace {

// The per-area map constants, straight out of the two overlay drawers. `dotXFromZ` says which world axis
// drives the dot's screen X (area 7's map is turned a quarter turn relative to area 2's) and
// `dotYNegated` carries the screen-Y direction (area 2's map runs the other way).
struct MiniMapArea {
  uint8_t  areaMode;                    // 0x800BF870 value this drawer belongs to
  uint16_t tpage;                       // FUN_80083DE0's tpage argument
  int      mapX, mapY;                  // screen position of the 64x64 map image
  int      mapU, mapV;                  // its texture origin in the page
  uint16_t mapClut;
  int      dotU, dotV;                  // the 8x8 player dot's texture origin
  uint16_t dotClut;
  bool     dotXFromZ;                   // true: screen X comes from world Z (area 7)
  int      dotXOrigin, dotXBase;        // screenX = (world - origin)/kMapUnit + base
  int      dotYOrigin, dotYBase;
  bool     dotYNegated;                 // true: screenY = base - (world - origin)/kMapUnit (area 2)
};

constexpr int kMapUnit = 0xB6;          // world units per map pixel (both areas)
constexpr int kMapSize = 64;
constexpr int kDotSize = 8;

constexpr MiniMapArea kAreas[] = {
  // area 2 (0x80113628): X drives screen X, Z drives screen Y and is negated.
  { 2, 0x0F, 240, 144, 0x00, 0xB0, 0x3D7F, 0x40, 0x68, 0x3D3F,
    /*dotXFromZ=*/false, /*dotXOrigin=*/0x840, /*dotXBase=*/0xEC,
    /*dotYOrigin=*/0xA7F, /*dotYBase=*/0xCC, /*dotYNegated=*/true },
  // area 7 (0x801140A0): the map is turned — Z drives screen X, X drives screen Y, neither negated.
  { 7, 0x19, 240, 164, 0xC0, 0x50, 0x7527, 0x50, 0x58, 0x74E7,
    /*dotXFromZ=*/true, /*dotXOrigin=*/0x1680, /*dotXBase=*/0xEC,
    /*dotYOrigin=*/0xA00, /*dotYBase=*/0xA0, /*dotYNegated=*/false },
};

constexpr uint32_t kPlayerMapX = 0x1F800160u;   // s16 world X the map transform reads
constexpr uint32_t kPlayerMapZ = 0x1F800164u;   // s16 world Z
constexpr uint32_t kFrameCtr   = 0x1F80017Cu;   // the blink phase

}  // namespace

// FUN_80113628 (area mode 2) / FUN_801140A0 (area mode 7) — the overlay-resident minimap drawers,
// rebuilt natively as one parameterised producer.
void Render::fieldHudMinimap(int areaMode) {
  Core* c = mCore;
  const MiniMapArea* a = nullptr;
  for (const MiniMapArea& m : kAreas)
    if (m.areaMode == (uint8_t)areaMode) a = &m;
  if (!a) return;                                    // an area whose drawer is not this family

  // The texture page the drawer selects, decoded from its DR_MODE word (both maps are 4bpp).
  const int tpX = (int)(a->tpage & 0xFu) * 64;
  const int tpY = ((a->tpage >> 4) & 1) * 256;
  const int texMode = (int)((a->tpage >> 7) & 3u);

  RenderQueue& rq = c->game->activeRq();

  // (1) the map image — a RAW 64x64 rect, so the packet colour never modulates it.
  {
    int xs[4] = { a->mapX, a->mapX + kMapSize, a->mapX, a->mapX + kMapSize };
    int ys[4] = { a->mapY, a->mapY, a->mapY + kMapSize, a->mapY + kMapSize };
    int us[4] = { a->mapU, a->mapU + kMapSize, a->mapU, a->mapU + kMapSize };
    int vs[4] = { a->mapV, a->mapV, a->mapV + kMapSize, a->mapV + kMapSize };
    unsigned char cc[4] = { 0x80, 0x80, 0x80, 0x80 };
    rq.push2dQuad(RQ_OVERLAY, /*order_2d_fg=*/1, xs, ys, us, vs, cc, cc, cc, tpX, tpY,
                  texMode, /*raw=*/1, (int)(a->mapClut & 0x3Fu) * 16, (int)(a->mapClut >> 6) & 0x1FF,
                  0, 0, 0, 0, 0, 0, 1023, 511);
  }

  // (2) the player dot — the world position through the area's own linear map transform, blinking.
  {
    const int worldX = (int16_t)c->mem_r16(kPlayerMapX);
    const int worldZ = (int16_t)c->mem_r16(kPlayerMapZ);
    const int alongX = a->dotXFromZ ? worldZ : worldX;
    const int alongY = a->dotXFromZ ? worldX : worldZ;
    const int dx = (alongX - a->dotXOrigin) / kMapUnit + a->dotXBase;
    const int spanY = (alongY - a->dotYOrigin) / kMapUnit;
    const int dy = a->dotYNegated ? a->dotYBase - spanY : a->dotYBase + spanY;

    const bool dim = (c->mem_r32(kFrameCtr) & 2u) != 0;
    const unsigned char red = dim ? 0x80 : 0xFF, gb = dim ? 0x40 : 0x20;
    unsigned char rr[4] = { red, red, red, red };
    unsigned char gg[4] = { gb, gb, gb, gb }, bb[4] = { gb, gb, gb, gb };
    int xs[4] = { dx, dx + kDotSize, dx, dx + kDotSize };
    int ys[4] = { dy, dy, dy + kDotSize, dy + kDotSize };
    int us[4] = { a->dotU, a->dotU + kDotSize, a->dotU, a->dotU + kDotSize };
    int vs[4] = { a->dotV, a->dotV, a->dotV + kDotSize, a->dotV + kDotSize };
    rq.push2dQuad(RQ_OVERLAY, /*order_2d_fg=*/1, xs, ys, us, vs, rr, gg, bb, tpX, tpY,
                  texMode, /*raw=*/0, (int)(a->dotClut & 0x3Fu) * 16, (int)(a->dotClut >> 6) & 0x1FF,
                  0, 0, 0, 0, 0, 0, 1023, 511);
  }
}
