// FIELD BACKDROP producer subsystem (Tomba2Engine) — the native sky/parallax TILEMAP drawer.
//
// Extracted from render_walk.cpp (the scene renderer) as its own cohesive owner: the backdrop is a
// self-contained producer cluster — the guest-dispatch RESOLVER (backdropTilemapDrawer, which reads
// the field bg-state jump table exactly as gen_func_8003DF04 does), the per-frame texpage FACT
// publisher both render modes consume (backdropTexpagePublishTick), and the RQ_BACKGROUND tile
// producer itself (backdropRender). All method declarations live in render.h; nothing here changes.
#include "core.h"
#include "fps60.h"
#include "producer_scope.h" // ProducerScope — graphics-producer DB, native leg
#include "render.h"
#include "render_internal.h" // sil_bbox_log_i
#include "render_queue.h"
#include <lucent/log.h> // `bgtp` diagnostic channel

// ===================================================================================================
// ONE NATIVE RENDER PATH — world-data-driven scene render (Phase 1, user 2026-06-24 architecture:
// [[one-native-render-path-decoupled]]). Driven from the GAME's WORLD DATA, NOT from PSX GP0 packets:
// walk the 3 active entity lists, and render each live object's 3D model (geomblk via node+0xC0 cmds)
// through the native float-projection submitters (eproj + D32 depth + engine lighting). This is the
// single mechanism depth/60fps/ires/lighting attach to. It runs as its OWN pass (not bolted onto the
// PSX OT-walk) so the draw state is the native pass's. Gated `debug scenenative` while standing it up.
// (g_scene_native_diag was defined here but never read; dead — removed 2026-07-02)
// g_sn_objs/g_sn_cmds retired 2026-07-03 — Render::stats.snObjs/snCmds (RenderStats).
// Resolve the resident backdrop tilemap drawer exactly as the guest field dispatcher does
// (gen_func_8003DF04 @0x8003DF04) and confirm it is the SHARED tilemap routine, reporting its baked
// per-tile V bias. See render.h for the contract. Read-only (guest RAM + resident code words only).
bool Render::backdropTilemapDrawer(int &vAdd, uint32_t *drawerVAOut) {
  Core *c = mCore;
  constexpr uint32_t kBgGate = 0x800BF873u;      // field dispatch gate (!=0 -> no backdrop this beat)
  constexpr uint32_t kBgSelector = 0x800BF870u;  // field bg-state selector (== the area's bg-state)
  constexpr uint32_t kBgJumpTable = 0x80014FC0u; // 16-entry bg-state -> MAIN dispatch-stub table
  constexpr uint32_t kBgStateCount = 16u;        // guest draws nothing for state >= this
  constexpr uint32_t kBgStateComposite = 21u;    // wolf-ride: phase-dependent A0L drawer (see below)
  constexpr uint32_t kSopSig = 0x80109450u;      // SOP overlay first-instruction signature word
  constexpr uint32_t kSopSigVal = 0x3C021F80u;
  constexpr uint32_t kSopDrawer = 0x8010C26Cu;   // SOP narration's own tilemap drawer (V bias 0)
  constexpr uint32_t kJalOp = 0x03u;             // MIPS jal opcode (top 6 bits)
  constexpr uint32_t kVDecodeAndi = 0x30E200F0u; // `andi r2,r7,0xF0` — the tilemap V-decode site
  constexpr uint32_t kVAdd8Insn = 0x24420008u;   // `addiu r2,r2,8` immediately after it (V bias 8)
  constexpr uint32_t kDrawerScan = 0x400u;       // drawer body size to search for the V-decode

  uint32_t drawerVA;
  if (c->mem_r32(kSopSig) == kSopSigVal) {
    // SOP narration draws its backdrop from its OWN overlay drawer (the field jump table's state-0 slot
    // points at 0x80115598, which is NOT resident under the SOP overlay), so resolve it directly.
    drawerVA = kSopDrawer;
  } else {
    if (c->mem_r8(kBgGate) != 0) {
      return false;
    }
    const uint32_t st = c->mem_r8(kBgSelector);
    // State 21 (wolf-ride) is special-cased ahead of the table to the phase-dependent A0L drawer
    // (0x8010BE30). The reached variant-1 / phase<4 branch calls the gouraud helper 0x8010BB64 and
    // returns; area21SkyGradientRender owns that picture separately. The tilemap loop is another branch,
    // and drawing it in the early-phase repro made the frame worse (61375 -> 64650 px >8/255), so this
    // plain-tilemap resolver must continue to reject state 21 until that branch is visibly reached.
    if (st == kBgStateComposite) {
      return false;
    }
    if (st >= kBgStateCount) {
      return false;
    }
    const uint32_t stub = c->mem_r32(kBgJumpTable + st * 4);
    // The dispatch stub's 2nd instruction is `jal <overlay drawer>`; the no-backdrop stub (0x8003E020)
    // has none. Decode the jal target rather than hardcoding each overlay's drawer address.
    const uint32_t jal = c->mem_r32(stub + 4);
    if ((jal >> 26) != kJalOp) {
      return false;
    }
    drawerVA = (stub & 0xF0000000u) | ((jal & 0x03FFFFFFu) << 2);
  }
  // Scan the resident drawer for the tilemap V-decode. Absent -> not the shared tilemap routine (a
  // different, still-unported backdrop mechanism) -> no native producer, the far plane stays black.
  for (uint32_t p = drawerVA; p < drawerVA + kDrawerScan; p += 4) {
    if (c->mem_r32(p) != kVDecodeAndi) {
      continue;
    }
    vAdd = (c->mem_r32(p + 4) == kVAdd8Insn) ? 8 : 0;
    // Report the drawer only on SUCCESS — on any reject path above it is either undecoded or a routine
    // this producer does not draw, and handing that address to the producer DB would key real native
    // prims to a guest function that emits something else entirely.
    if (drawerVAOut) {
      *drawerVAOut = drawerVA;
    }
    return true;
  }
  return false;
}

// BACKDROP ATLAS TEXPAGE — a per-frame FACT ABOUT GUEST STATE, published for BOTH render modes.
//
// "Which VRAM page holds this frame's sky/sea tiles" is not part of building a picture: it is the key
// the OT walk uses to recognise the GUEST background drawer's own 16x16 tiles (gpu_native.cpp
// sprite_is_bg_texpage) and band them RQ_BACKGROUND. Without it a screen-space sprite falls through to
// `bg_2d`'s coverage heuristic, which a 16x16 tile can never satisfy, so it lands in RQ_HUD — the
// TOPMOST 2D band — and is painted over the world.
//
// It used to be published only from Render::backdropRender, which runs only inside renderScene(), i.e.
// only on the pc_render arm of Engine::drawOTag. psx_render returns from drawOTag before renderScene,
// so on the reference leg it was NEVER published and every backdrop tile the guest emitted occluded the
// whole frame. MEASURED 2026-08-06, area 0 free-roam f3100, PSXPORT_GATE=1 PSXPORT_RENDER_PSX=1,
// PSXPORT_PRIMDUMP: 972 prims walked = 617 is3d world polys + 355 sprites, and 355/355 sprites carried
// bg=0 — 352 of them the 16x16 tiles of texpage (896,0), the backdrop atlas named in the banner below.
// The reference renderer therefore drew the whole village and then painted the sea over it.
//
// This is the SAME defect, and the same fix, as areaCacheTrustTick (kanban #41): per-logic-frame guest
// state tracking that both render modes need must be ticked BEFORE the render-mode branch, never from
// inside one arm. It is the ONLY publisher — backdropRender no longer publishes.
//
// Gate: the guest's OWN dispatch resolution — backdropTilemapDrawer, which resolves the resident drawer
// through the field bg-state jump table and its dispatch gate exactly as the guest field dispatcher does
// (see that function; it owns those addresses). NOT
// mBackdropTrusted. That latch answers "may OUR native producer draw", which is a different question —
// the guest emitted its tiles from this struct whether or not we trust it, so the page it sampled is a
// fact about what is in the OT. Read-only: guest RAM + resident code words, no guest write.
void Render::backdropTexpagePublishTick() {
  Core *c = mCore;
  constexpr uint32_t kParallaxBgSm = 0x800ED018u; // the backdrop tilemap state struct (see banner below)
  constexpr uint32_t kBgTpageOff = 0x04u;         // +0x04 hword tpage
  int vAdd = 0;
  const bool tilemapBackdrop = backdropTilemapDrawer(vAdd);
  int tp_x = -1, tp_y = -1;
  if (tilemapBackdrop) {
    const uint16_t tpage = c->mem_r16(kParallaxBgSm + kBgTpageOff);
    tp_x = (tpage & 0xF) * 64;
    tp_y = ((tpage >> 4) & 1) * 256;
    void gpu_bg_texpage_set(Core *, int, int);
    gpu_bg_texpage_set(c, tp_x, tp_y);
  }
  // A silent negative here is indistinguishable from "the tick never ran", and the consequence of a
  // negative is invisible-world, so the line always carries WHICH branch was taken and the guest bytes
  // it turned on: tilemap=0 means the resident drawer is not the shared tilemap routine (area 14/21 and
  // the state>=16 areas — an honest unported-backdrop gap, and those areas' guest tiles, if any, will
  // still band as HUD), tilemap=1 names the page that was published.
  lucent::debug("bgtp",
                "tilemap={} tp=({},{}) bgstate={} bggate={}",
                (int)tilemapBackdrop,
                tp_x,
                tp_y,
                c->mem_r8(0x800BF870u),
                c->mem_r8(0x800BF873u));
}

// NATIVE BACKDROP tilemap drawer — overlay FUN_80115598 (the seaside field's state-0 background drawer,
// reached via 0x8003df04's 16-state jump table @0x80014fc0; state 0 → 0x8003df74 → 0x80115598). This is the
// sky + distant parallax hills (the only thing the decoupled native scene was missing — verified by SKIPPASS
// attribution, later-244). The PSX body reads the area's tile MAP (W×H grid of u16 tile entries) and a
// scroll position, then builds GP0 textured-sprite (cmd 0x7d) packets into the OT for the visible wraparound
// window of 16×16 tiles. We TRANSCRIBE the integer wrap/scroll/index math (that's scene data — what tile
// goes where), but emit NATIVE RQ_BACKGROUND 2D quads instead of GP0 packets / OT links (the engine owns the
// background layer; sky/hills sit behind the real-depth world). Struct @t4 (=0x800ed018 at the seaside):
//   +0x04 hword tpage  +0x06 hword clut-base  +0x10 byte W  +0x11 byte H
//   +0x14 word  tilemap ptr (u16[H][W])       +0x28 hword scrollX  +0x2a hword scrollY
// Tile entry bits: [0:3]=atlas col, [4:7]=atlas row, [8:11]=clut sub-index. U=(t&0xF)<<4, V=(t&0xF0)+vAdd
// (the half-tile V bias is per-DRAWER source-data layout, resolved by backdropTilemapDrawer above — 8 for
// the seaside drawer, 0 for every other area and for SOP), clut=clutbase+((t&0xF00)>>2). Texpage 0x0E =
// 4bpp @ VRAM(896,0) (set once via a GP0(0xE1) prim in the PSX body; applied per-quad here).
// TIER 1 BACKDROP (docs/fps60-rework.md): scrollX/scrollY are the ONLY per-frame-varying fields this fn
// reads (PARALLAX_BG_SM+0x28/+0x2A, computed by ParallaxBg::step from camera yaw/pitch every RUNNING
// tick) — everything else (W/H/tilemap ptr/tpage/clutbase/wrap-moduli) is static per-area config, set
// once at INIT and unchanged while running. The scroll read goes through the fps60 provider (mirrors
// sceneCam): byte-identical to the plain struct read when fps60 is off or this is the real per-logic-
// frame call (which also captures the result into Fps60::mBgCur); during Tier-1's present-time backdrop
// re-render (Fps60::tier1Render, fps60.cpp) it instead returns wrapLerp(mBgPrev,mBgCur,t), no guest read.
void Render::backdropRender(uint32_t t4) {
  Core *c = mCore;
  int W = c->mem_r8(t4 + 0x10), H = c->mem_r8(t4 + 0x11);
  if (W == 0 || H == 0) {
    return;
  }
  // kanban #33: guest-time capture-only. The only per-frame-varying state the present-time backdrop
  // re-render reads back is the scroll offset (mBgCur) — everything else (tilemap/tpage/wrap moduli) is
  // static per-area config it re-reads directly. Capture the scroll (bgScroll self-captures on a real,
  // non-override call) and skip drawing every tile; the present re-renders the backdrop from mSink.
  if (fps60(*c->game).mWorldCaptureOnly) {
    int sx, sy;
    fps60(*c->game).bgScroll(c, t4, sx, sy);
    // The backdrop texpage publish that used to live here (and in the draw path below) is gone: it is
    // per-frame guest-state tracking, so it belongs to backdropTexpagePublishTick, which Engine::drawOTag
    // runs BEFORE the render-mode branch. Publishing it from a producer made it a function of which
    // renderer was active, which is what left psx_render's own backdrop tiles in RQ_HUD.
    return;
  }
  // TILE V-OFFSET — the seaside FIELD drawer (FUN_80115598) samples each tile at v = (tile&0xF0)+8; the
  // SOP NARRATION drawer (FUN_8010C26C) and the OTHER field tilemap drawers (e.g. area 10 FUN_801142EC,
  // area 11 FUN_801141B0, plus FUN_8010BE30's later Area 21 tilemap branch) sample at v = (tile&0xF0)
  // with NO +8 (RE'd from the per-overlay drawer bodies; applying seaside's +8 elsewhere shifts the
  // atlas sample half a tile -> tile seams,
  // issue #60: sea beat RMSE 40.2 -> 18.5 with vAdd=0). Read the bias from the RESIDENT drawer's own code
  // (backdropTilemapDrawer, the same resolution the dispatch below uses) — ground truth per area, not a
  // scene heuristic. Correct in the field and narration contexts, and at both call sites (sceneNative real
  // frame + Fps60 tier-1 interp re-render).
  int vAdd = 8;
  uint32_t drawerVA = 0;
  const bool isTilemapDrawer = backdropTilemapDrawer(vAdd, &drawerVA);

  // Producer DB, native leg (external/psxport/docs/plans/graphics-producer-db.md). The backdrop was the
  // second-largest UNDECLARED block of native prims. KEYED BY THE RESIDENT DRAWER, resolved per area
  // from the guest's own bg-state jump table, NOT by a hardcoded address: every area's backdrop drawer
  // is the same routine compiled per overlay (seaside FUN_80115598, area 10 FUN_801142EC, area 11
  // FUN_801141B0, SOP narration FUN_8010C26C...), so one literal would mislabel every area but one —
  // and the DB would report a producer that is not resident as the thing that drew the sky.
  //
  // A miss opens NO scope rather than guessing: if the resolution rejected (drawer is not the shared
  // tilemap routine, or the resolver could not decode one) those prims stay in the census's counted
  // unscopedNative() total. This pass still draws in that case only when the caller decided to call it,
  // so the honest outcome is an undeclared count, never a row keyed to an unidentified drawer.
  ProducerScope backdropScope(
      (isTilemapDrawer && drawerVA) ? &c->rsub.producerScope : nullptr, drawerVA, "backdropRender");

  int rowstride = W * 2;        // s0 — bytes per map row
  int mapbytes = rowstride * H; // s3 — total map bytes (wrap modulus)
  int scrollX, scrollY;
  fps60(*c->game).bgScroll(c, t4, scrollX, scrollY);
  uint32_t map = c->mem_r32(t4 + 0x14);
  uint16_t tpage = c->mem_r16(t4 + 0x04);
  uint16_t clutbase = c->mem_r16(t4 + 0x06);
  int tp_x = (tpage & 0xF) * 64, tp_y = ((tpage >> 4) & 1) * 256;
  int mode = (tpage >> 7) & 3;
  if (mode > 2) {
    mode = 2;
  }
  // (This frame's backdrop texpage is published by backdropTexpagePublishTick, before the render-mode
  // branch — not from here. See that function.)
  // WIDESCREEN backdrop coverage (root-cause fix for the [320,nw) atlas-garbage band): the PSX body tiles
  // a 320-wide window centred at screen-x 160 (t5 = ...+0x160 = 352 = 320+32 slack). At a wide aspect the
  // engine projects the world with OFX=nw/2, so the visible field spans [0,nw) — but the sky/parallax
  // backdrop, drawn only across ~[0,344), left the right margin uncovered, exposing the raw VRAM texture
  // atlas that lives past the 320-wide FB (later-55 VRAM packing). Re-centre the backdrop on the wide
  // centre (cx=nw/2) and widen the tiled window to nw+32 so it fills the full wide FB, matching the
  // world's OFX shift. cx/winw reduce to the exact 4:3 values (160 / 0x160) when not wide, so the 4:3
  // path stays byte-identical. Gated on gpu_vk_wide_engine() (false at 4:3 / oracle / SBS legs).
  int gpu_vk_wide_engine(Core *), gpu_vk_wide_engine_w(Core *);
  int cx = 160, winw = 0x160; // screen-centre X / tiled window width (4:3 defaults)
  if (gpu_vk_wide_engine(c)) {
    int nw = gpu_vk_wide_engine_w(c);
    cx = nw / 2;
    winw = nw + 0x20;
  }
  // Starting tile row/col = (scroll - screen-center) >> 4, wrapped into [0,H) / [0,W).
  int rowtile = ((scrollY - 120) >> 4) % H;
  if (rowtile < 0) {
    rowtile += H;
  }
  int coltile = ((scrollX - cx) >> 4) % W;
  if (coltile < 0) {
    coltile += W;
  }
  int t2 = rowtile * rowstride;                       // current row byte offset (wraps mod mapbytes)
  int coloff0 = coltile * 2;                          // starting col byte offset (wraps mod rowstride)
  int xoff = (int16_t)(cx - 8 - scrollX);             // t9 — sub-tile X scroll remainder + screen offset
  int yoff = (int16_t)(112 - scrollY);                // s7 — sub-tile Y scroll remainder + screen offset
  unsigned char col[4] = {0x80, 0x80, 0x80, 0x80};    // 0x7d is raw-texture: color ignored
  int outer_bound = (int16_t)(scrollY - 120) + 0x100; // 16 rows
  int t5 = (int16_t)(scrollX - cx) + winw;            // wide-covering column window (4:3: ~22 cols)
  // Tag every backdrop tile with the reserved kBackdropDbgNode sentinel (render_queue.h) — NOT the
  // dbg_node==0 a generic OT-walk-classified RQ_BACKGROUND item gets (menu backdrop art, hut-interior
  // clear, SOP fills). This is what lets Fps60::tier1Render's queue-lerp exclusion (fps60.cpp
  // isTier1Owned) target ONLY the prims it actually re-renders, same pattern as terrain/scene-table (#54).
  c->rsub.diag.beginObject(kBackdropDbgNode);
  // These tiles are already WIDE-FINAL: every X below is built from cx = nw/2 (above), i.e. the same
  // widened centre the world is projected with, so the queue's 4:3 centring must not touch them. This
  // used to be expressed as an exemption inside the queue keyed on kBackdropDbgNode — but a debug
  // node id is an IDENTITY, not a coordinate space, and using it as one meant every other producer
  // with wide-final coordinates was silently centred a second time (kanban #73). The declaration
  // belongs here, at the producer that knows.
  RenderQueue &bgRq = c->game->rqRedirect ? *c->game->rqRedirect : c->game->rq;
  RenderQueue::Space2dScope wideFinal(bgRq, RQ_2D_WIDE_FINAL);
  for (int t8 = scrollY - 120;;) {
    int Y = (int16_t)((t8 & 0xFFF0) + yoff);
    int t6 = (int16_t)t2; // row byte offset (sign-extended)
    int t0 = coloff0;
    for (int t1 = scrollX - cx;;) {
      int X = (int16_t)((t1 & 0xFFF0) + xoff);
      uint16_t tile = c->mem_r16(map + (uint32_t)(t6 + t0));
      int u = (tile & 0xF) << 4, v = (tile & 0xF0) + vAdd; // field +8 / SOP narration +0 (see top of fn)
      uint16_t clut = (uint16_t)(clutbase + ((tile & 0xF00) >> 2));
      int clut_x = (clut & 0x3F) * 16, clut_y = (clut >> 6) & 0x1FF;
      int xs[4] = {X, X + 16, X, X + 16}, ys[4] = {Y, Y, Y + 16, Y + 16};
      int us[4] = {u, u + 16, u, u + 16}, vs[4] = {v, v, v + 16, v + 16};
      sil_bbox_log_i("bg_tilemap", xs, ys, 4);
      // Tier-1 redirect (mirrors native_terrain.cpp / fieldEntityRender's fix — see fps60-rework.md
      // "Tier 1 extended"): route through rqRedirect so re-invoking this fn at present time (Fps60::
      // tier1Render) lands in the isolated mSink, never the live queue the next real frame will build.
      bgRq.push2dQuad(RQ_BACKGROUND,
                      /*order_2d_fg=*/0,
                      xs,
                      ys,
                      us,
                      vs,
                      col,
                      col,
                      col,
                      tp_x,
                      tp_y,
                      mode,
                      /*raw=*/1,
                      clut_x,
                      clut_y,
                      0,
                      0,
                      0,
                      0,
                      0,
                      0,
                      1023,
                      511);
      c->rsub.stats.snCmds++;
      t0 += 2;
      if (t0 >= rowstride) {
        t0 = 0; // column wrap
      }
      t1 += 16;
      if (!((int16_t)t1 < t5)) {
        break;
      }
    }
    t2 += rowstride;
    if ((int16_t)t2 >= mapbytes) {
      t2 -= mapbytes; // row wrap
    }
    t8 += 16;
    if (!((int16_t)t8 < outer_bound)) {
      break;
    }
  }
  c->rsub.diag.endObject();
}
