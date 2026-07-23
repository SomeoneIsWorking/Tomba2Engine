---
id: C004
kind: claim
status: holds
created: 2026-07-23
tags: sbs
---

## Claim

Render::subPartWalk mirrors the guest live callee-saved registers s0..s4

## Evidence

SBS-full 0-diff to f19140 on the seesaw leg after the change; was f169 before

## What would falsify it

any further native port that calls a still-substrate callee inside a loop without mirroring its live registers will reintroduce this class
