---
id: C022
kind: claim
status: holds
created: 2026-07-29
tags: re
---

## Claim

The recompiler folds contiguous guest functions into one gen_func_ body in 13% of cases, and in 2.3% live code jumps past the first return — so 'trim at the first return;' is unsound without a label guard

## Evidence

Measured over all 2119 main-EXE gen functions 2026-07-29 via port_gen._split_live_extent: 1788 clean, 282 with a folded sibling tail (largest 16,861 foreign lines at 0x8009D06C), 49 where a label defined after the first bare return; is targeted by a goto in the head — i.e. reachable code that a naive trim would delete. abi_extract.py:869-885 already documented the artifact; this quantifies it.

## What would falsify it

a re-run of the sweep after a recompiler change producing materially different proportions, or any function in the 282 where the trimmed tail turns out to be reachable
