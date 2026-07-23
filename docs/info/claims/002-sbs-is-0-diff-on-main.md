---
id: C002
kind: claim
status: falsified
created: 2026-07-23
tags: 
falsified_on: 2026-07-23
---

## Claim

SBS is 0-diff on main

## Evidence

cited from runs predating 2026-07-23

## What would falsify it

a rebuild that changes exec order, or a run on a DIFFERENT binary than the one measured

## FALSIFIED 2026-07-23

kanban #50 proved SBS is RED on main and was already red before its change (built the same tree twice, both diverge identically at f169). Any pre-2026-07-23 0-diff citation is stale.

> Anything that cited this claim as proof must be re-checked. Grep the repo for it.
