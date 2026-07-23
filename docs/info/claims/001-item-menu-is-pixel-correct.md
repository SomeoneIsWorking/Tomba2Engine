---
id: C001
kind: claim
status: falsified
created: 2026-07-23
tags: render
falsified_on: 2026-07-23
---

## Claim

item menu is pixel-correct

## Evidence

render_cmp diff vs psx_render at ingame-item-menu.pad f1120 = 0/76800

## What would falsify it

if psx_render itself is wrong for this layer, this proves nothing

## FALSIFIED 2026-07-23

USER 2026-07-23: the SBS PSX pane shows the menu correctly while ours is dark. 0/76800 only proved we MATCH psx_render on the default exec leg; it never proved correctness.

> Anything that cited this claim as proof must be re-checked. Grep the repo for it.
