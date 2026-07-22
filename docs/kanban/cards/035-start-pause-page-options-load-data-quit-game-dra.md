---
id: 35
title: START pause page (Options / Load data / Quit game) draws with no panel background
status: todo
labels: []
created: 2026-07-22
updated: 2026-07-22
---

**2026-07-22:** 2026-07-22 (from the #21 fix session, observed not chased): tapping START from free-roam draws the three option strings over the LIVE field with no background — scratch/screenshots/uilayer/hunt/btn_start.png. Same family as #21 (a menu whose text has a producer and whose panel does not) but a DIFFERENT drawer: #21's fix is a tap SCOPED to the item-menu controller FUN_800346BC and deliberately does not cover this page, whose content comes from FUN_8007EAE4 under the same 0x8010810C page dispatcher. The #21 recipe applies verbatim: scope a capture around this page's drawer, tap FUN_8007E1B8/FUN_8007E6DC, order by OT bucket descending with LIFO inside a bucket, draw at RQ_OVERLAY. See game/ui/pause_menu.cpp and docs/findings/render.md '#21'.
