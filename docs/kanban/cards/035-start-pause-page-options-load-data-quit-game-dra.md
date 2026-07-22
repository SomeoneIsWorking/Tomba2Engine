---
id: 35
title: START pause page (Options / Load data / Quit game) draws with no panel background
status: done
labels: []
created: 2026-07-22
updated: 2026-07-23
---

**2026-07-22:** 2026-07-22 (from the #21 fix session, observed not chased): tapping START from free-roam draws the three option strings over the LIVE field with no background — scratch/screenshots/uilayer/hunt/btn_start.png. Same family as #21 (a menu whose text has a producer and whose panel does not) but a DIFFERENT drawer: #21's fix is a tap SCOPED to the item-menu controller FUN_800346BC and deliberately does not cover this page, whose content comes from FUN_8007EAE4 under the same 0x8010810C page dispatcher. The #21 recipe applies verbatim: scope a capture around this page's drawer, tap FUN_8007E1B8/FUN_8007E6DC, order by OT bucket descending with LIFO inside a bucket, draw at RQ_OVERLAY. See game/ui/pause_menu.cpp and docs/findings/render.md '#21'.

**2026-07-23:** 2026-07-23 FIXED (StartPage, game/ui/start_page.cpp). Root cause: the page chrome is emitted by still-recomp FUN_8007EAE4 through the ALREADY-TAPPED shared FT4 leaf FUN_8007E1B8, but the collector dropped every group unless the item-menu scope (FUN_800346BC) was raised — so this page had no producer. Fix = a scope wrapper on 0x8007EAE4 plus factoring the capture out of PauseMenu into the shared UiGroupCapture (route() files a group under whichever page scope is raised). No second owner on 8007E1B8/8007E6DC. NO backdrop and NO subtractive dim here (unlike #21) — this page emits zero full-screen tiles and must composite over the live field. Evidence: panel bbox (106,81)-(208,145) vs the psx leg, px differing >16 3819/6695 -> 1466/6695, mean|d| 20.49 -> 7.60, box mean RGB (26.4,25.0,15.4) -> (26.1,26.4,53.4) vs psx (26.8,27.0,53.8); scratch/screenshots/c35/. Regression gates 0/76800 differing px pre- vs post-change: plain field f300, triangle menu, bucket-softlock.pad f1200 dialog. SBS full A/B identical to f19020 (boot only). Separate follow-up: the in-game Options sub-page still has no backdrop (Render::optionsBackdrop is only driven from renderTitle).
