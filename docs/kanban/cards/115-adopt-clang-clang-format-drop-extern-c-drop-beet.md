---
id: 115
title: Adopt clang + clang-format, drop extern C, drop beetle
status: todo
labels: []
created: 2026-08-20
updated: 2026-08-20
---

USER DECISIONS, 2026-08-20, taken while ending a session mid-change: "apply clang format and use clang from now on", "I don't think we need extern \"C\"", "Don't use beetle". Not proposals — do not re-open them as questions.

FULL NOTE, with the measurements, lives in the framework so it reaches every port:
    external/psxport/docs/plans/toolchain-and-oracle-decisions.md

THE FRAMEWORK TREE WAS LEFT DELIBERATELY BROKEN AND UNCOMMITTED. psxport is at 6557b585 (the note) plus ~265 uncommitted modified files that do NOT build. That was the instruction. Nothing was committed because the formatting is one command to regenerate and a non-building main would break every port that builds off that one checkout.
    to discard:  cd $PSX/psxport && git checkout -- .
    to resume:   delete the remaining extern "C" (see the note), keeping the beetle-adapter exception

Tomba2Engine itself is UNTOUCHED by this and still builds. Its only dirty files are the unrelated fx_rope_strip.cpp work in progress.

THE FOUR PIECES, in the order they are least entangled:

1. clang-format. Config is already in the repo and was never applied: 280 first-party files, 56,735 violation sites. Two traps measured — .clang-format sets no PointerAlignment so LLVM's Right default rewrites the whole tree from `Core* c` to `Core *c` (decide that BEFORE sweeping, it is most of the diff), and the sweep needs TWO passes to converge.

2. clang as the compiler. NOT STARTED, NOT MEASURED. Build is GCC today. Expect real work; the vendored beetle C is only known to build under GCC here.

3. Drop extern "C". It buys nothing — the shards are ".c files holding C++ content" compiled as CXX, and no C translation unit includes core.h — and it cost a real bug: rec_coro_run is declared both inside core.h's extern "C" and in scheduler.h with C++ linkage, which conflicts the moment include sorting puts scheduler.h first. ONE EXCEPTION that is load-bearing: the beetle adapters link against vendored code compiled as real C. That exception disappears with item 4.

4. Drop beetle. SCOPE UNSETTLED — ask before starting. Beetle is vendored for FOUR subsystems: the GPU oracle (self-contained tee, cheap to remove), and the GTE / MDEC / SPU backends, which are the port's actual geometry, FMV and audio with no replacement written. "Don't use beetle" was said about the oracle.

WHAT DROPPING THE GPU ORACLE COSTS — recorded as fact, not as an argument against a decision already made. It found five real defects in one session that a presented-frame comparison structurally cannot: #110 (its own calibration), #111 (a black screen recorded as "psx_render draws literally nothing" that was a correct picture being cleared away), #112 (interpolated colour truncated where hardware rounds), #113 (dither disabled by a primitive's texpage word), #114 (the named residual floor). Whatever replaces it has to answer "is this our rasterizer or the game's packets" — the question our own rasterizer cannot answer about itself. It is off by default (PSXPORT_GPU_BEETLE), so nothing is urgent.

**2026-08-20:** 2026-08-20 — MOSTLY DONE, and TWO OF THE FOUR DECISIONS WERE REVERSED BY THE USER. psxport b049892a.

USER: "revert 'Don't use beetle' and extern \"C\" but make sure to accomodate to the formatter, you should accomodate to the formatter not the other way around", then "Never duplicate code no matter the reason".

FINAL POSITION:
    clang-format          STANDS — DONE, whole tree, and the gate now enforces it
    clang as compiler     STANDS — NOT STARTED
    drop extern "C"       REVERSED — kept
    drop beetle           REVERSED — kept, GPU oracle included
    never duplicate code  NEW standing rule, recorded in psxport/CLAUDE.md

THE TREE IS NO LONGER BROKEN. psxport builds, 62 of 63 tests pass, tomba2_port builds. The RESUME HERE banners are removed from both CLAUDE.md files.

WHAT WAS ACTUALLY WRONG, and it was never the formatter. Include sorting broke the build because rec_coro_run was declared TWICE — core.h inside its extern "C", and scheduler.h with ordinary C++ linkage. They disagreed about linkage; which won depended on include order. It compiled by luck and sorting spent the luck. Accommodating the formatter means fixing the CODE, so a sweep for the same shape (any function declared in more than one first-party header) found three more, two with the identical latent mismatch:
    rec_coro_run       core.h + scheduler.h                  -> one owner: core.h
    rec_dispatch       core.h + guest_abi.h + guest_call.h   -> one owner: core.h
    xa_decode_sector   c_subsys.h + fmv_decode.h             -> one owner: c_subsys.h
Both guest_*.h already #include "core.h" and re-declared it anyway. fmv_decode.h's copy carried a comment saying it was "identical" — a copy you have documented is still a copy.

THE GATE NOW HAS A DENOMINATOR. check_cpp_style.py only ever format-checked the six files in its FILE_CAPS dict, which is how 280 files came to hold 56,735 violations while it stayed green. It checks every first-party source now and says how many. vendor/ stays out: reformatting it would make every future upstream diff unreadable.

ACCOMMODATED RATHER THAN OVERRIDDEN: .clang-format sets no PointerAlignment, so LLVM's Right default applies and the tree is now `Core *c`. Adding `PointerAlignment: Left` would have bent the config to the code — the opposite of the instruction — so it was left alone even though it is most of the diff.

WHAT REMAINS ON THIS CARD:
  1. clang as the compiler. Not started, not measured. Build is GCC (/usr/lib64/ccache/cc). The vendored beetle C is only known to build under GCC here, so expect real work.
  2. game_iface.h is 520 lines against a 500-line ownership cap — the ONE remaining test failure, pre-existing, unrelated to formatting, and a real architectural signal asking for a module to be extracted. Bumping the cap to go green would be a hardcoded expected value; do the extraction or leave it failing honestly.
