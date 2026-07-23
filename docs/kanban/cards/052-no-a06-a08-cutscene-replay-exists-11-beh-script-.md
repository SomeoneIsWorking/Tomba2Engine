---
id: 52
title: No A06/A08 cutscene replay exists — 11 beh_* script handlers are unreachable for A/B
status: todo
labels: [verification]
created: 2026-07-23
updated: 2026-07-23
---

Split from #10 2026-07-23. Coverage measured across the whole replay library (bucket-softlock 53, save-sign 54, seesaw 54, weapon-impact 54, house-on-the-point 53, hut-entry 53, boot-smoke 4): union 54 of 65 table entries reached, 11 NEVER reached. All 11 are cutscene-script handlers, and the reason is simply that the library has NO A06/A08 CUTSCENE CAPTURE: 0x80041098 (its body IS exercised via ScriptInterp::step direct calls, but the table entry never dispatches so BEH_SUBSTRATE cannot swap it), 0x801189E8, 0x801280D0, 0x8013AA14, and the seven op-0x3E fnptrs 0x80139728/8013AEF0/8013AFD8/8013B074/8013B178/8013B274/8013B29C. ACTION: capture a cutscene replay (a story beat that runs the A06/A08 script overlays) — ideally cut from the user's live session with padrec save, the way house-on-the-point.pad was — then re-run tools/beh_ab.sh sweep against it. Until then these 11 rebuilds are UNVERIFIED, and the confirmed bug class (#2, #1/#30, the ZonedAttacker arm-shift) says unverified rebuilds carry real defects.
