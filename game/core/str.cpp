// game/core/str.cpp — see str.h. WIRED (2026-07-10 verify pass, docs/fleet-workflow.md §9).
#include "str.h"
#include "core.h"

// FUN_80079528 — strlen. RE (tools/disas.py 0x80079528 --all 20, cross-checked against
// generated/shard_2.c:10049 gen_func_80079528, which is instruction-exact ground truth):
//   v0 = *a0;
//   if (v0 == 0) { v1 = 0; goto done; }
//   do { a0++; v0 = *a0; v1++; } while (v0 != 0);
//   done: v0 = v1; return;
// i.e. a byte-for-byte transcription of libc strlen(). No stack frame, no sub-calls — a true
// leaf. Return value goes in v0 (c->r[2]); this native form returns it directly AND mirrors it
// into c->r[2] so a call site that inspects the ABI register after `rec_dispatch`-style plumbing
// still sees the right value if this draft is later wired as an override.
uint32_t Str::length(Core *c, uint32_t addr) {
  uint32_t p = addr;
  uint32_t len = 0;
  while (c->mem_r8(p) != 0) {
    p += 1;
    len += 1;
  }
  c->r[2] = len;
  return len;
}

// FUN_8009A3E0 — memcpy(dst, src, n). RE from generated/shard_0.c:15524 gen_func_8009A3E0, a 15
// instruction leaf with no stack frame:
//   if (dst == 0) { v0 = 0; return; }          // the null-destination guard, and it returns 0
//   v1 = dst;                                   // saved so the return value survives the loop
//   if ((int32_t)n <= 0) { v0 = v1; return; }   // SIGNED — a negative length copies nothing
//   do { *dst++ = *src++; } while (--n > 0);    // plain byte loop, signed counter
//   v0 = v1;                                    // returns the ORIGINAL dst, not the end cursor
// The signedness is the only subtle part: the guest tests `n > 0` as a signed compare both before
// and inside the loop, so a negative length is a no-op. An unsigned transcription would turn one
// into a 4-billion-byte copy that walks off the end of guest RAM.
uint32_t Str::copyBytes(Core *c, uint32_t dst, uint32_t src, int32_t n) {
  if (dst == 0) {
    c->r[2] = 0;
    return 0;
  }
  uint32_t d = dst;
  while (n > 0) {
    c->mem_w8(d, (uint8_t)c->mem_r8(src));
    d += 1;
    src += 1;
    n -= 1;
  }
  c->r[2] = dst;
  return dst;
}

// ------------------------------------------------------------------------------------------------
// WIRING (verify pass, 2026-07-10, docs/fleet-workflow.md §9): re-diffed byte-for-byte against
// generated/shard_2.c:10049 -- exact strlen transcription, no bugs found. Checked every call site
// (generated/shard_0.c:12595, shard_5.c:4668, shard_6.c:4476, shard_7.c:12065/12089): a0(r4) is
// never read back for a "leftover cursor" the way Font::measureLineWidth's caller does -- every
// call site overwrites r4 before its next use, so no leftover-register mirroring is needed here.
// PLAIN intra-shard C calls at every call site (func_80079528(c), not rec_dispatch) -> wired via
// the oracle-gated engine_set_override_main thunk, SBS core B keeps running the pure gen_func_* body.
namespace {
void ov_strLength(Core *c) {
  Str::length(c, c->r[4]);
  // Mirror gen_func_80079528's v1 (r3) output (shard_2.c): the substrate's loop counter ends up
  // in r3 == r2 (the length) at return. The native C++ body only sets r2.
  c->r[3] = c->r[2];
}
} // namespace

// FUN_8009A3E0's own wiring note: the guest leaves its WORKING registers advanced at return —
// r4/r5 point one past the last byte copied and r6 is 0 (or the untouched negative length). Those
// are caller-saved argument registers, so no ABI-conforming caller reads them back, and the native
// body below leaves them as the caller set them. Called as a plain intra-shard func_8009A3E0(c) at
// every site as well as via rec_dispatch, so it is wired with the main-module setter like the rest.
namespace {
void ov_copyBytes(Core *c) {
  Str::copyBytes(c, c->r[4], c->r[5], (int32_t)c->r[6]);
}
} // namespace

extern void gen_func_80079528(Core *);
extern void gen_func_8009A3E0(Core *);

// FUN_0x8009A640 — byte compare, sibling of the memcpy already owned here.
// ORACLE: gen_func_8009A640
void Str::compareBytes(Core *c) {
  {
    int _t = (c->r[4] == c->r[0]);
    if (_t) {
      goto L_8009A650;
    }
  }
  {
    int _t = (c->r[5] != c->r[0]);
    if (_t) {
      goto L_8009A668;
    }
  }
L_8009A650:;
  {
    int _t = (c->r[4] == c->r[5]);
    c->r[2] = c->r[0] + c->r[0];
    if (_t) {
      goto L_8009A6B8;
    }
  }
  {
    int _t = (c->r[4] == c->r[0]);
    c->r[2] = c->r[0] + (uint32_t)-1;
    if (_t) {
      goto L_8009A6B8;
    }
  }
  c->r[2] = c->r[0] + (uint32_t)1;
  goto L_8009A6B8;
L_8009A668:;
  c->r[6] = c->r[6] + (uint32_t)-1;
  {
    int _t = ((int32_t)c->r[6] < 0);
    c->r[2] = c->r[0] + c->r[0];
    if (_t) {
      goto L_8009A6B8;
    }
  }
L_8009A674:;
  c->r[3] = (uint32_t)(int8_t)c->mem_r8((c->r[4] + (uint32_t)0));
  c->r[2] = (uint32_t)(int8_t)c->mem_r8((c->r[5] + (uint32_t)0));
  {
    int _t = (c->r[3] != c->r[2]);
    c->r[5] = c->r[5] + (uint32_t)1;
    if (_t) {
      goto L_8009A69C;
    }
  }
  {
    int _t = (c->r[3] == c->r[0]);
    c->r[4] = c->r[4] + (uint32_t)1;
    if (_t) {
      goto L_8009A6B4;
    }
  }
  c->r[6] = c->r[6] + (uint32_t)-1;
  {
    int _t = ((int32_t)c->r[6] >= 0);
    if (_t) {
      goto L_8009A674;
    }
  }
L_8009A69C:;
  {
    int _t = ((int32_t)c->r[6] < 0);
    if (_t) {
      goto L_8009A6B4;
    }
  }
  c->r[3] = (uint32_t)(int8_t)c->mem_r8((c->r[4] + (uint32_t)0));
  c->r[2] = (uint32_t)(int8_t)c->mem_r8((c->r[5] + (uint32_t)-1));
  c->r[2] = c->r[3] - c->r[2];
  goto L_8009A6B8;
L_8009A6B4:;
  c->r[2] = c->r[0] + c->r[0];
L_8009A6B8:;
  return;
}

void str_wide_re_install() {
  static bool done = false;
  if (done) {
    return;
  }
  done = true;
  extern void engine_set_override_main(uint32_t, OverrideFn, OverrideFn);
  engine_set_override_main(0x80079528u, ov_strLength, gen_func_80079528);
  engine_set_override_main(0x8009A3E0u, ov_copyBytes, gen_func_8009A3E0);
  {
    extern void gen_func_8009A640(Core *);
    engine_set_override_main(0x8009A640u, &Str::compareBytes, gen_func_8009A640);
  }
}
