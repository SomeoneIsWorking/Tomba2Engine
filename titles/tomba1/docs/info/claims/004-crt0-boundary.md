---
id: C004
kind: claim
status: holds
created: 2026-08-27
tags: tomba1,crt0,oracle
depends: tools/compare_crt0_boundary.py
---

## Claim

Tomba! 1's recorded pre-migration CRT0 reaches the same first `A(39h)` call boundary as the
independent oracle.

## Evidence

`compare_crt0_boundary.py` compared target, PC, 31 writable registers, LO, and HI: 35/35 fields
agreed. Two oracle runs were byte-identical. Mutating the recorded product's `gp` produced exactly one reported
difference, and a 100-step oracle run refused before reaching the boundary.

## What would falsify it

The Lightrec product disagrees on any of the 35 fields, either oracle run differs, the too-short run
claims a boundary, or the forced mismatch is not detected.
