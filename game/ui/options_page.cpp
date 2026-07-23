// class OptionsPage — implementation. See options_page.h for the RE of the five page builders, the
// guest-data measurement that identified the missing backdrop, and where paint order comes from.
#include "options_page.h"
#include "core.h"
#include "game.h"
#include "game_ctx.h"        // eng(c) / rend(c)
#include "engine.h"
#include "render.h"          // Render::optionsBackdrop / optionsSolidBox + the psxRender() gate
#include "render_queue.h"    // RQ_OVERLAY
#include "cfg.h"             // `optionspage` diagnostic channel
#include "override_registry.h"

extern void gen_func_8007F104(Core*);
extern void gen_func_8007F250(Core*);
extern void gen_func_8007F498(Core*);
extern void gen_func_8007F73C(Core*);
extern void gen_func_8007F8F8(Core*);
extern void gen_func_8007FC24(Core*);
extern void engine_set_override_main(uint32_t, OverrideFn, OverrideFn);

namespace {

// ---- FUN_8007FC24's guest state (the same pool/table the dialog backdrop uses) ------------------
constexpr uint32_t kPacketPoolPtr  = 0x800BF544u;  // u32: bump allocator for GPU packets
constexpr uint32_t kOtTablePtr     = 0x800ED8C8u;  // u32 -> base of the ordering table
constexpr uint32_t kPacketBytes    = 36u;          // tag + 8 command words (POLY_G4)
constexpr uint32_t kOtBucketNear   = 1u;           // the bucket the backdrop links into
constexpr uint8_t  kGp0PolyG4      = 0x38u;        // shaded four-point polygon, opaque
constexpr uint8_t  kTagWordCount   = 8u;

// The gradient's four vertex colours and corners, verbatim from the packet the guest builds: three
// corners the page blue, the bottom-right one darker.
constexpr uint8_t  kBlueBright     = 70u;
constexpr uint8_t  kBlueDark       = 16u;
constexpr uint16_t kScreenW        = 320u;
constexpr uint16_t kScreenH        = 240u;

}  // namespace

// ORACLE: gen_func_8007FC24
void OptionsPage::pushBackdrop(Core* c) {
  // Take a packet off the pool and bump it, exactly as the guest allocator does.
  const uint32_t packet = c->mem_r32(kPacketPoolPtr);
  c->mem_w32(kPacketPoolPtr, packet + kPacketBytes);

  // POLY_G4 body: per-vertex colour word (RGB in the low three bytes, the GP0 opcode riding in the
  // top byte of the FIRST one) followed by that vertex's screen XY. The 4th byte of the other three
  // colour words is left alone — the guest never writes it, so neither does this.
  c->mem_w8(packet + 7,  kGp0PolyG4);
  c->mem_w8(packet + 6,  kBlueBright);      // v0 blue
  c->mem_w8(packet + 14, kBlueBright);      // v1 blue
  c->mem_w8(packet + 22, kBlueBright);      // v2 blue
  c->mem_w8(packet + 30, kBlueDark);        // v3 blue (the darkened bottom-right corner)
  c->mem_w8(packet + 4,  0); c->mem_w8(packet + 5,  0);   // v0 red/green
  c->mem_w8(packet + 12, 0); c->mem_w8(packet + 13, 0);   // v1
  c->mem_w8(packet + 20, 0); c->mem_w8(packet + 21, 0);   // v2
  c->mem_w8(packet + 28, 0); c->mem_w8(packet + 29, 0);   // v3
  c->mem_w16(packet + 8,  0);        c->mem_w16(packet + 10, 0);          // v0 (0,0)
  c->mem_w16(packet + 16, kScreenW); c->mem_w16(packet + 18, 0);          // v1 (320,0)
  c->mem_w16(packet + 24, 0);        c->mem_w16(packet + 26, kScreenH);   // v2 (0,240)
  c->mem_w16(packet + 32, kScreenW); c->mem_w16(packet + 34, kScreenH);   // v3 (320,240)

  // Link it at the head of its bucket: the packet inherits whatever was there, and becomes the head.
  const uint32_t slot = c->mem_r32(kOtTablePtr) + kOtBucketNear * 4u;
  c->mem_w32(packet, c->mem_r32(slot) | ((uint32_t)kTagWordCount << 24));
  c->mem_w32(slot, packet);

  // Display half: the page owns the paint order, so the picture is deferred to drawCollected rather
  // than pushed here (the guest calls this AFTER the cursor but links it deeper — see options_page.h).
  eng(c).optionsPage.mBackdrop = true;
}

void OptionsPage::noteBox(Core* c, int x, int y, int w, int h, uint32_t flags) {
  OptionsPage& page = eng(c).optionsPage;
  if (!page.capture.capturing()) return;    // the dialog box's own producer owns that path
  page.mBoxes.push_back(Box{ x, y, w, h, flags });
}

void OptionsPage::drawCollected(Core* c) {
  const bool nothing = !mBackdrop && mBoxes.empty() && capture.empty();
  if (!nothing && !(c->game->oracle || c->rsub.mode.psxRender())) {
    if (mBackdrop) rend(c)->optionsBackdrop();
    // A bucket's packet list is LIFO (AddPrim prepends), so the LAST box the builder staged is the
    // one painted first.
    for (size_t i = mBoxes.size(); i-- > 0; ) {
      const Box& b = mBoxes[i];
      rend(c)->optionsSolidBox(b.x, b.y, b.w, b.h, b.flags);
    }
    for (int i : capture.paintOrder()) {
      const UiGroupArgs& a = capture.mGroups[i];
      cfg_logf("optionspage", "%s bucket=%3u templ=%08X at (%d,%d) wh=(%d,%d) attr=%02X clutSemi=%04X",
               a.sprite ? "SPR" : "FT4", a.otBucket, a.templPtr, a.x, a.y, a.wOv, a.hOv,
               a.attrByte, a.clutSemi);
      capture.emit(c, a, RQ_OVERLAY);
    }
  }
  mBackdrop = false;
  mBoxes.clear();
  capture.clear();
}

namespace {

// One page builder's scope: raise the capture, run the untouched gen body (the page owns no state of
// its own on the host side), lower it, then draw what the frame collected.
template <void (*Gen)(Core*)>
void pageScope(Core* c) {
  OptionsPage& page = eng(c).optionsPage;
  const bool outer = !page.capture.capturing();
  if (outer) { page.capture.clear(); page.mBoxes.clear(); page.mBackdrop = false; }
  page.capture.begin();
  Gen(c);
  if (!outer) return;
  page.capture.end();
  page.drawCollected(c);
}

// FUN_8007FC24 — the shared backdrop emitter. The guest half is the port above; nothing else.
void backdropEmit(Core* c) { OptionsPage::pushBackdrop(c); }

}  // namespace

void OptionsPage::install() {
  static bool done = false;
  if (done) return;
  done = true;
  engine_set_override_main(0x8007F104u, pageScope<gen_func_8007F104>, gen_func_8007F104);
  engine_set_override_main(0x8007F250u, pageScope<gen_func_8007F250>, gen_func_8007F250);
  engine_set_override_main(0x8007F498u, pageScope<gen_func_8007F498>, gen_func_8007F498);
  engine_set_override_main(0x8007F73Cu, pageScope<gen_func_8007F73C>, gen_func_8007F73C);
  engine_set_override_main(0x8007F8F8u, pageScope<gen_func_8007F8F8>, gen_func_8007F8F8);
  engine_set_override_main(0x8007FC24u, backdropEmit, gen_func_8007FC24);
  // The two shared group leaves (0x8007E1B8 / 0x8007E6DC) and the box emitter (0x8007FCC8) keep their
  // single existing owners — they route into this scope, they are not installed again here.
}
