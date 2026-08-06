---
id: C022
kind: claim
status: holds
created: 2026-07-29
tags: re
reconfirmed: 2026-08-06
verified_at: 2026-08-06
depends: external/psxport/tools/port_gen.py#_split_live_extent
---

## Claim

The recompiler folds contiguous guest functions into one gen_func_ body in 13% of cases, and in 2.3% live code jumps past the first return — so 'trim at the first return;' is unsound without a label guard

## Evidence

Measured over all 2119 main-EXE gen functions 2026-07-29 via port_gen._split_live_extent: 1788 clean, 282 with a folded sibling tail (largest 16,861 foreign lines at 0x8009D06C), 49 where a label defined after the first bare return; is targeted by a goto in the head — i.e. reachable code that a naive trim would delete. abi_extract.py:869-885 already documented the artifact; this quantifies it.

## What would falsify it

a re-run of the sweep after a recompiler change producing materially different proportions, or any function in the 282 where the trimmed tail turns out to be reachable

## Re-confirmed 2026-08-06

RE-VERIFIED 2026-08-06 by RE-RUNNING THE SWEEP, not by re-reading the note. Driver: enumerate every 'void gen_func_<hex>(Core* c) {' body in generated/shard_*.c (main-EXE only, overlays excluded as in the original), brace-match each body, and run port_gen._split_live_extent on it. RESULT, all four numbers identical to 2026-07-29: corpus 2119 bodies across 9 shard files; 1788 clean (84.4%); 282 with a folded sibling tail trimmed (13.3%); 49 where trailing text exists but the soundness guard HELD, i.e. a label after the first bare return; is targeted from the head (2.3%). ONE DIFFERENCE, and it is not a discrepancy in the claim: the largest foreign tail is 18482 lines at 0x8009D06C today vs the 16,861 recorded then — SAME address, different line count, which is expected because generated/ is re-emitted from the operator's own disc rather than committed. The two abi_extract commits that flagged this stale (58fd3184, 0e8ec525) fixed regex bugs in abi_extract/port_check; the sweep runs through port_gen._split_live_extent, which they did not touch, and the reproduction confirms that. DENOMINATOR DISCIPLINE: the driver exits non-zero if it finds no shard files or parses zero bodies, so a 'clean proportions' report over an empty corpus is impossible.
