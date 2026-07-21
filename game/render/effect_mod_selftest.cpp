// Oracle-based UNIT TEST for the five secondary-effect modifiers (game/render/effect_mod.cpp).
//
// These passes rewrite already-emitted GP0 packets in the render pool. They are byte-faithful ports,
// but NONE of them fires in any scenario the replay library can reach (see docs/findings/render.md),
// so the live game cannot verify them — exactly the gap this kind of test exists to close.
//
// Method, per the CutsceneCamera precedent: seed a SYNTHETIC packet pool + effect params in guest
// RAM, run the native method, snapshot the result, restore the inputs, run the ORIGINAL guest
// function over the identical inputs via rec_interp (the interpreter reading the real MAIN.EXE — an
// oracle independent of both my reading and the recompiler), and diff the whole pool byte-for-byte.
// 0 mismatches over a sweep that hits every opcode = the port reproduces the guest exactly.
//
// Deterministic (fixed LCG seed), render-free, no disc needed beyond MAIN.EXE.
// Selected by PSXPORT_SELFTEST=effectmod. Exit 0 = pass, 1 = fail.
#include "render.h"
#include "core.h"
#include "game.h"
#include "game_ctx.h"
#include "cfg.h"
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <functional>

void load_exe(const char* path, Core* c);

namespace {

constexpr uint32_t POOL     = 0x800A0000u;   // synthetic packet pool (free RAM in the test)
constexpr uint32_t POOL_END = 0x800A1000u;   // 4 KB — comfortably more than the packets we build
constexpr uint32_t NODE     = 0x800B0000u;   // the object node carrying the effect parameters

struct Region { uint32_t base, words; const char* name; };
constexpr Region REGIONS[] = {
  { POOL, 1024, "pool" },     // 4 KB — the packets, which is what every pass rewrites
  { NODE,   64, "node" },     // the effect params; a pass must NOT write here
};
constexpr int NREG = sizeof(REGIONS) / sizeof(REGIONS[0]);
constexpr uint32_t MAXW = 1024;

// Every opcode the guest jump tables recognise, with its stride. Colour-bearing ones are marked so
// the sweep can guarantee it emits plenty of them (a pool of pure skip-packets would verify nothing).
struct Op { uint8_t code; uint8_t stride; bool colour; };
constexpr Op OPS[] = {
  { 0x20u, 20, false }, { 0x24u, 32, true  }, { 0x28u, 24, false }, { 0x2Cu, 40, true  },
  { 0x30u, 28, false }, { 0x34u, 40, true  }, { 0x38u, 36, false }, { 0x3Cu, 52, true  },
};
constexpr int NOPS = sizeof(OPS) / sizeof(OPS[0]);

struct PoolRng {
  uint32_t s;
  uint32_t next() { s = s * 1664525u + 1013904223u; return s >> 8; }
  uint32_t below(uint32_t n) { return next() % n; }
};

// Fill [POOL, end) with a random but WELL-FORMED packet chain, and return the end of the last packet
// (the `hi` the guest passes). Every packet gets random colour bytes and a random command byte whose
// low two bits are also random — the guest masks with 0xFC, so those bits must survive untouched
// except where a pass deliberately rewrites the byte.
uint32_t buildPool(Core* c, PoolRng& rng) {
  for (uint32_t a = POOL; a < POOL_END; a += 4) c->mem_w32(a, 0);
  uint32_t p = POOL;
  for (;;) {
    const Op& op = OPS[rng.below(NOPS)];
    if (p + op.stride > POOL_END - 64) break;
    for (uint32_t k = 0; k < op.stride; k += 4) c->mem_w32(p + k, rng.next());
    // command byte: the opcode, plus random low bits that the mask ignores
    c->mem_w8(p + 7u, (uint8_t)(op.code | rng.below(4)));
    p += op.stride;
  }
  return p;
}

void snap(Core* c, uint32_t buf[NREG][MAXW]) {
  for (int r = 0; r < NREG; r++)
    for (uint32_t i = 0; i < REGIONS[r].words; i++) buf[r][i] = c->mem_r32(REGIONS[r].base + i * 4);
}
void restore(Core* c, uint32_t buf[NREG][MAXW]) {
  for (int r = 0; r < NREG; r++)
    for (uint32_t i = 0; i < REGIONS[r].words; i++) c->mem_w32(REGIONS[r].base + i * 4, buf[r][i]);
}

int reported = 0;

// Run native vs oracle on identical inputs and diff. Returns the mismatching word count.
int check(Core* c, const char* name, uint32_t guestAddr, const std::function<void()>& nat,
          uint32_t lo, uint32_t hi) {
  static uint32_t in[NREG][MAXW], mine[NREG][MAXW];
  uint32_t rs[32];
  snap(c, in);
  memcpy(rs, c->r, sizeof rs);

  nat();
  snap(c, mine);

  restore(c, in);
  memcpy(c->r, rs, sizeof rs);
  c->r[4] = NODE; c->r[5] = lo; c->r[6] = hi;
  c->recMissed = false; c->recMissTolerant = true;
  rec_interp(c, guestAddr);
  c->recMissTolerant = false;
  if (c->recMissed) return -1;    // oracle couldn't evaluate -> skip, never a silent pass

  int bad = 0;
  for (int r = 0; r < NREG; r++)
    for (uint32_t i = 0; i < REGIONS[r].words; i++) {
      const uint32_t o = c->mem_r32(REGIONS[r].base + i * 4);
      if (mine[r][i] != o) {
        bad++;
        if (reported++ < 30)
          cfg_logi("effecttest", "MISMATCH %-9s %s+0x%04x  mine=%08x oracle=%08x",
                   name, REGIONS[r].name, i * 4, mine[r][i], o);
      }
    }
  return bad;
}

}  // namespace

int run_effectmod_selftest(const char* path) {
  Game* game = new Game();
  Core* c = &game->core;
  load_exe(path, c);
  Render* rendr = rend(c);

  const int ITERS = cfg_str("PSXPORT_SELFTEST_ITERS") ? atoi(cfg_str("PSXPORT_SELFTEST_ITERS")) : 400;
  PoolRng rng{ 0x13572468u };
  cfg_logi("effecttest", "effect-modifier oracle test: %d iterations/pass, seed=0x%08x", ITERS, rng.s);

  struct Case { const char* name; uint32_t addr; std::function<void(uint32_t, uint32_t)> nat; };
  const Case cases[] = {
    { "semiOn",   0x8003F3F4u, [&](uint32_t lo, uint32_t hi) { rendr->effectSemiOn  (NODE, lo, hi); } },
    { "semiOff",  0x8003F4C4u, [&](uint32_t lo, uint32_t hi) { rendr->effectSemiOff (NODE, lo, hi); } },
    { "clutSwap", 0x8003F344u, [&](uint32_t lo, uint32_t hi) { rendr->effectClutSwap(NODE, lo, hi); } },
    { "flatTint", 0x8003F594u, [&](uint32_t lo, uint32_t hi) { rendr->effectFlatTint(NODE, lo, hi); } },
    { "colorAdd", 0x8003D584u, [&](uint32_t lo, uint32_t hi) { rendr->effectColorAdd(NODE, lo, hi); } },
  };
  const int NC = sizeof(cases) / sizeof(cases[0]);

  int total_bad = 0, total_runs = 0, total_skip = 0;
  for (int t = 0; t < NC; t++) {
    int bad = 0, ran = 0, skip = 0;
    for (int it = 0; it < ITERS; it++) {
      const uint32_t hi = buildPool(c, rng);
      // Effect params. Sweep the coloradd byte encoding deliberately across its three regimes
      // (<=0x7F darken / ==0x80 identity / >=0x81 brighten) instead of hoping random bytes hit them.
      const uint8_t r8 = (it % 3 == 0) ? (uint8_t)rng.below(0x80)
                       : (it % 3 == 1) ? 0x80u : (uint8_t)(0x81u + rng.below(0x7F));
      const uint8_t g8 = (uint8_t)rng.below(256), b8 = (uint8_t)rng.below(256);
      c->mem_w8(NODE + 0x18u, r8);
      c->mem_w8(NODE + 0x19u, g8);
      c->mem_w8(NODE + 0x1Au, b8);
      c->mem_w8(NODE + 0x1Bu, (uint8_t)rng.below(256));
      c->mem_w16(NODE + 0x5Cu, (uint16_t)rng.below(0x10000));

      const int m = check(c, cases[t].name, cases[t].addr,
                          [&] { cases[t].nat(POOL, hi); }, POOL, hi);
      if (m < 0) skip++; else { bad += m; ran++; }
      total_runs++;
    }
    cfg_logi("effecttest", "%-9s : %s  (%d mismatching words over %d verified iters, %d oracle-skipped)",
             cases[t].name, bad ? "FAIL" : "ok", bad, ran, skip);
    total_bad += bad; total_skip += skip;
  }

  cfg_logi("effecttest", "DONE: %d passes, %d runs (%d oracle-skipped), %d mismatching words -> %s",
           NC, total_runs, total_skip, total_bad, total_bad ? "FAIL" : "PASS");
  if (total_skip == total_runs) {
    cfg_loge("effecttest", "every run was oracle-skipped — this verified NOTHING");
    return 1;
  }
  return total_bad ? 1 : 0;
}
