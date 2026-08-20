---
id: C049
kind: claim
status: holds
created: 2026-08-20
tags: behaviour
depends: game/object/behavior_dispatch.cpp
---

## Claim

The three formerly open BEH A/B residuals in kanban 51 are currently resolved on covered deterministic runs

## Evidence

house-on-the-point 700f covered 8011D988 and 80121978 with fresh A/B ram=0 spad=0; bucket-softlock 2100f covered 8013C9C0 with fresh A/B ram=0 spad=0

## What would falsify it

a fresh covered A/B run at any of 8011D988, 80121978, or 8013C9C0 produces a nonzero RAM or scratchpad residual
