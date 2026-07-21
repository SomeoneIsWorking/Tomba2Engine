// game/ui/loading_text.cpp — the blinking "Loading....." indicator.
//
// Guest FUN_8007FD54, RE'd 2026-07-22 from gen_func_8007FD54. Found while tracing what actually runs
// on the dialog path in the bucket-pickup capture.
//
// WHAT IT IS. Three lines of guest code: read a frame counter, look at bit 2, and draw the string
// "Loading....." (0x80017304) at a fixed screen position in one of two palettes. Bit 2 of a counter
// that ticks per frame flips every 4 frames, so the text BLINKS while the disc is being read.
//
// THE PC_SKIP FORK (USER 2026-07-22). On this port the "load" is a host file read that has already
// finished by the time anything could be drawn, so the indicator advertises a wait that does not
// exist. With pc_skip ON we therefore draw nothing; with it OFF we emit the guest's exact packet, so
// the faithful path and SBS stay byte-identical. Per CLAUDE.md the fork is two methods each running
// one path top-to-bottom, not an if/else inside one body.
#include "core.h"
#include "game.h"
#include "ui/loading_text.h"
#include "override_registry.h"

void func_80079374(Core*);   // generated/shard_disp.c — text draw (x, y, palette, str, flags)

namespace {

constexpr uint32_t FRAME_COUNTER  = 0x1F800198u;  // u16, ticks per frame
constexpr uint32_t BLINK_BIT      = 1u << 2;      // flips every 4 frames
constexpr uint32_t STR_LOADING    = 0x80017304u;  // "Loading....."
constexpr uint32_t TEXT_DRAW_FN   = 0x80079374u;  // FUN_80079374(x, y, palette, str, flags)

constexpr int32_t  TEXT_X = 160;                  // screen centre-ish, fixed by the guest
constexpr int32_t  TEXT_Y = 180;
constexpr int32_t  PALETTE_DIM = 0;                // shown on the "off" half of the blink
constexpr int32_t  PALETTE_LIT = 6;                // shown on the "on" half

}  // namespace

// The guest body: blink the palette, draw the string. `mode` (5th arg of the text draw) is passed on
// the stack at sp+16 under o32, which is why it is written there rather than into a register.
// ORACLE: gen_func_8007FD54
void LoadingText::drawFaithful(Core* c) {
  const uint32_t counter = c->mem_r16(FRAME_COUNTER);

  c->r[29] -= 32;
  const uint32_t sp = c->r[29];
  c->mem_w32(sp + 24, c->r[31]);        // guest frame: ra spilled at +24

  // The guest writes the stack argument separately on each side of the blink test rather than once
  // before it; kept that way so the store sequence matches (port_check compares it).
  int32_t palette;
  if (counter & BLINK_BIT) { c->mem_w32(sp + 16, 0); palette = PALETTE_DIM; }
  else                     { c->mem_w32(sp + 16, 0); palette = PALETTE_LIT; }

  c->r[4] = (uint32_t)TEXT_X;
  c->r[5] = (uint32_t)TEXT_Y;
  c->r[6] = (uint32_t)palette;
  c->r[7] = STR_LOADING;
  c->r[31] = 0x8007FDA0u;               // jal-site ra, so the callee spills the right value
  func_80079374(c);

  c->r[31] = c->mem_r32(sp + 24);
  c->r[29] += 32;
}

// pc_skip: nothing to indicate — the host read finished before this could be drawn.
void LoadingText::drawSkip(Core* c) { (void)c; }

void LoadingText::draw(Core* c) {
  if (c->game && c->game->pc_skip) { drawSkip(c); return; }
  drawFaithful(c);
}

static void ov_loading_text(Core* c) { LoadingText::draw(c); }

void loading_text_install() {
  extern void gen_func_8007FD54(Core*);
  extern void shard_set_override(uint32_t, void (*)(Core*));
  overrides::install(0x8007FD54u, "LoadingText::draw", ov_loading_text, gen_func_8007FD54,
                     shard_set_override);
}
