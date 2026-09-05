---
id: C016
kind: claim
status: holds
created: 2026-07-28
tags: render
---

## Claim

Four of the six remaining sweep-found render fns are NOT sprite-family members and must not reuse SpriteAnchor

## Evidence

binary-analysis design pass 2026-07-28: 0x80116904, 0x80110CA4, 0x801110BC and 0x801113B4 never program DQA at all (so SpriteAnchor::baseScale would be a fabricated number) and each has its own OT gate differing from otKeyInRange in ways that change WHICH anchors draw (raw no-pre-clamp / 3-stage with pre-snap / no OT key at all, SX<320 clip / no gate, GTE never touched). Only 0x8010C1D8 is a full member and 0x8013B118 branch A a partial one. See docs/re/render-targets-binary-analysis.md

## What would falsify it

finding a DQA program or an otKeyInRange-equivalent gate in any of those four gen bodies
