// Differential acceptance test for Font::iconGlyphEmit (FUN_80078988).
//
// The native body is a clean rebuild, not a transcription, so the static store-sequence checker
// cannot prove it. This test feeds the same MAIN.EXE-backed token strings to the native method and
// the real MIPS interpreter, then compares every byte of main RAM and scratchpad plus the complete
// register file and hi/lo. It also proves the host queue can show both answers: a mapped direct
// glyph must enqueue one RQ_HUD item, while an unmapped token must enqueue none.
//
// Selected by PSXPORT_SELFTEST=iconglyph. No disc, renderer, audio, or game window is needed.
#include "cfg.h"
#include "core.h"
#include "game.h"
#include "render_queue.h"
#include "ui/font.h"
#include <array>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

void load_exe(const char *path, Core *c);

namespace {
constexpr uint32_t kGuestFunction = 0x80078988u;
constexpr uint32_t kPacketPoolPtr = 0x800BF544u;
constexpr uint32_t kOtBasePtr = 0x800ED8C8u;
constexpr uint32_t kPacketPool = 0x801D0000u;
constexpr uint32_t kOtBase = 0x801D1000u;
constexpr uint32_t kString = 0x801D2000u;
constexpr uint32_t kStack = 0x801FF000u;
constexpr uint32_t kBucket = 3u;

struct Snapshot {
  std::vector<uint8_t> ram = std::vector<uint8_t>(0x200000);
  std::array<uint8_t, 0x400> scratch{};
  std::array<uint32_t, 32> regs{};
  uint32_t hi = 0;
  uint32_t lo = 0;
};

void capture(Core *c, Snapshot &out) {
  std::memcpy(out.ram.data(), c->ram, out.ram.size());
  std::memcpy(out.scratch.data(), c->scratch, out.scratch.size());
  std::memcpy(out.regs.data(), c->r, sizeof(c->r));
  out.hi = c->hi;
  out.lo = c->lo;
}

void restore(Core *c, const Snapshot &in) {
  std::memcpy(c->ram, in.ram.data(), in.ram.size());
  std::memcpy(c->scratch, in.scratch.data(), in.scratch.size());
  std::memcpy(c->r, in.regs.data(), sizeof(c->r));
  c->hi = in.hi;
  c->lo = in.lo;
  c->pending_work = 0;
  c->recMissed = false;
  c->recMissTolerant = false;
}

void seed(Core *c, const std::vector<uint16_t> &pairs, int sizeClass = 8) {
  for (uint32_t i = 0; i < 512u; ++i) {
    c->mem_w8(kPacketPool + i, (uint8_t)(0xA5u ^ i));
  }
  for (uint32_t i = 0; i < 64u; i += 4u) {
    c->mem_w32(kOtBase + i, 0x00FFF000u + i);
  }
  for (uint32_t i = 0; i < 96u; ++i) {
    c->mem_w8(kStack - 64u + i, (uint8_t)(0x39u + i));
  }
  for (uint32_t i = 0; i < 16u; ++i) {
    c->mem_w8(0x1F800020u + i, (uint8_t)(0xC0u + i));
  }
  c->mem_w32(kPacketPoolPtr, kPacketPool);
  c->mem_w32(kOtBasePtr, kOtBase);
  uint32_t stringCursor = kString;
  for (uint16_t pair : pairs) {
    c->mem_w8(stringCursor++, (uint8_t)(pair >> 8));
    c->mem_w8(stringCursor++, (uint8_t)pair);
  }
  c->mem_w8(stringCursor, 0u);

  for (uint32_t i = 0; i < 32u; ++i) {
    c->r[i] = 0x13570000u + i * 0x101u;
  }
  c->r[0] = 0;
  c->r[4] = 37u;
  c->r[5] = 52u;
  c->r[6] = (uint32_t)sizeClass;
  c->r[7] = kString;
  c->r[29] = kStack;
  c->mem_w32(kStack + 16u, kBucket);
  c->hi = 0x76543210u;
  c->lo = 0x89ABCDEFu;
}

int compare(const char *name, const Snapshot &native, const Snapshot &oracle) {
  int mismatches = 0;
  for (size_t i = 0; i < native.ram.size(); ++i) {
    if (native.ram[i] != oracle.ram[i]) {
      if (mismatches < 12) {
        cfg_loge("iconglyphtest",
                 "%s RAM %08X native=%02X oracle=%02X",
                 name,
                 (uint32_t)(0x80000000u + i),
                 native.ram[i],
                 oracle.ram[i]);
      }
      ++mismatches;
    }
  }
  for (size_t i = 0; i < native.scratch.size(); ++i) {
    if (native.scratch[i] != oracle.scratch[i]) {
      if (mismatches < 12) {
        cfg_loge("iconglyphtest",
                 "%s SPAD %08X native=%02X oracle=%02X",
                 name,
                 (uint32_t)(0x1F800000u + i),
                 native.scratch[i],
                 oracle.scratch[i]);
      }
      ++mismatches;
    }
  }
  for (size_t i = 0; i < native.regs.size(); ++i) {
    if (native.regs[i] != oracle.regs[i]) {
      if (mismatches < 12) {
        cfg_loge("iconglyphtest", "%s r%zu native=%08X oracle=%08X", name, i, native.regs[i], oracle.regs[i]);
      }
      ++mismatches;
    }
  }
  if (native.hi != oracle.hi || native.lo != oracle.lo) {
    cfg_loge("iconglyphtest",
             "%s hi/lo native=%08X/%08X oracle=%08X/%08X",
             name,
             native.hi,
             native.lo,
             oracle.hi,
             oracle.lo);
    ++mismatches;
  }
  return mismatches;
}

struct Case {
  std::string name;
  std::vector<uint16_t> pairs;
  int sizeClass = 8;
  uint16_t syntheticCode = 0;
};
} // namespace

int run_icon_glyph_selftest(const char *path) {
  Game *game = new Game();
  Core *c = &game->core;
  load_exe(path, c);
  game->oracle = 1; // suppress host queue pushes during the guest-state differential

  std::vector<Case> cases = {
      {"empty", {}},
      {"direct-A", {0x8260u}},
      {"direct-a", {0x8281u}},
      {"direct-0", {0x824Fu}},
      {"newline", {0x8260u, 0x0A0Au, 0x824Fu}},
      {"table-miss", {0xFEFEu}},
  };
  const uint32_t firstTableToken = c->mem_r32(0x800A55E0u);
  const uint16_t firstTableCode = c->mem_r16(0x800A55E4u);
  uint32_t tableCount = 0;
  for (uint32_t entry = 0x800A55E0u; c->mem_r32(entry) != 0; entry += 8u, ++tableCount) {
    const uint32_t token = c->mem_r32(entry);
    const uint16_t pair = (uint16_t)(((uint32_t)c->mem_r8(token) << 8) | c->mem_r8(token + 1u));
    cases.push_back({"table-hit-" + std::to_string(tableCount), {pair}, (tableCount & 1u) ? 17 : 8});
  }
  cases.push_back({"combining-left", {0xFEFDu}, 8, 0x8001u});
  cases.push_back({"combining-right", {0xFEFDu}, 8, 0x9001u});

  Snapshot initial, native, oracle;
  int totalMismatches = 0;
  for (const Case &test : cases) {
    seed(c, test.pairs, test.sizeClass);
    if (test.syntheticCode != 0u) {
      c->mem_w32(0x800A55E0u, kString);
      c->mem_w16(0x800A55E4u, test.syntheticCode);
    }
    capture(c, initial);

    Font::iconGlyphEmit(c);
    capture(c, native);

    restore(c, initial);
    c->recMissTolerant = true;
    rec_interp(c, kGuestFunction);
    c->recMissTolerant = false;
    if (c->recMissed) {
      cfg_loge("iconglyphtest", "%s interpreter missed a nested target", test.name.c_str());
      ++totalMismatches;
      continue;
    }
    capture(c, oracle);
    const int mismatches = compare(test.name.c_str(), native, oracle);
    totalMismatches += mismatches;
    cfg_logi("iconglyphtest", "%-18s %s (%d mismatches)", test.name.c_str(), mismatches ? "FAIL" : "ok", mismatches);
  }

  // Opposite-answer proof for the host half: a direct glyph queues exactly one item; an unknown
  // token advances the cursor but queues none. This prevents a broken always-empty queue from passing.
  c->mem_w32(0x800A55E0u, firstTableToken);
  c->mem_w16(0x800A55E4u, firstTableCode);
  game->oracle = 0;
  RenderQueue &rq = game->activeRq();
  seed(c, {0x8260u});
  const unsigned long long beforeGlyph = rq.pushed_total;
  Font::iconGlyphEmit(c);
  const unsigned long long glyphPushes = rq.pushed_total - beforeGlyph;
  bool glyphOk = false;
  if (glyphPushes == 1u && rq.n > 0) {
    const RqItem &glyph = rq.items[rq.n - 1];
    glyphOk = glyph.layer == RQ_HUD && glyph.nv == 4 && glyph.xs[0] == 37 && glyph.ys[0] == 52 && glyph.us[0] == 8 &&
              glyph.vs[0] == 8;
  }

  seed(c, {0xFEFDu});
  c->mem_w32(0x800A55E0u, kString);
  c->mem_w16(0x800A55E4u, 0x8001u);
  const unsigned long long beforeLeft = rq.pushed_total;
  Font::iconGlyphEmit(c);
  const unsigned long long leftPushes = rq.pushed_total - beforeLeft;
  const bool leftOk = leftPushes == 2u && rq.n > 0 && rq.items[rq.n - 1].us[0] == 56;

  seed(c, {0xFEFDu});
  c->mem_w32(0x800A55E0u, kString);
  c->mem_w16(0x800A55E4u, 0x9001u);
  const unsigned long long beforeRight = rq.pushed_total;
  Font::iconGlyphEmit(c);
  const unsigned long long rightPushes = rq.pushed_total - beforeRight;
  const bool rightOk = rightPushes == 2u && rq.n > 0 && rq.items[rq.n - 1].us[0] == 64;

  c->mem_w32(0x800A55E0u, firstTableToken);
  c->mem_w16(0x800A55E4u, firstTableCode);
  seed(c, {0xFEFEu});
  const unsigned long long beforeMiss = rq.pushed_total;
  Font::iconGlyphEmit(c);
  const unsigned long long missPushes = rq.pushed_total - beforeMiss;
  const bool oppositeAnswer = glyphOk && leftOk && rightOk && missPushes == 0u;
  cfg_logi("iconglyphtest",
           "host queue: mapped=%llu combine-left=%llu combine-right=%llu unmapped=%llu -> %s",
           glyphPushes,
           leftPushes,
           rightPushes,
           missPushes,
           oppositeAnswer ? "PASS" : "FAIL");
  if (!oppositeAnswer) {
    ++totalMismatches;
  }

  cfg_logi("iconglyphtest",
           "DONE: %zu guest cases, %d mismatches -> %s",
           cases.size(),
           totalMismatches,
           totalMismatches ? "FAIL" : "PASS");
  return totalMismatches ? 1 : 0;
}
