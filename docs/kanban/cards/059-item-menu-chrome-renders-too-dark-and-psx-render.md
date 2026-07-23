---
id: 59
title: Item menu chrome renders TOO DARK — and psx_render shares it, so the 0/76800 gate is blind
status: done
labels: [render, bug]
created: 2026-07-23
updated: 2026-07-23
evidence: docs/reference/issues/issue59_item_menu_too_dark.png
---

USER 2026-07-23 with a side-by-side against a real-game reference: the item/pause menu's gold frame and chrome render DARKER than they should. The layout, text and item rows are all correct — it is a brightness/tint fault on the panel chrome.

WHY THIS MATTERS BEYOND THE MENU — IT INVALIDATES A GATE WE HAVE BEEN TRUSTING ALL DAY. The item menu at replays/bugs/ingame-item-menu.pad f1120 measures 0/76800 differing pixels against psx_render, and that number was cited repeatedly on 2026-07-23 (in #21, #38, #43/#44, #50 and the dust port) as proof the menu is correct. It is not: it proves only that we MATCH psx_render, and psx_render is dark too. This is the same trap as #22 (health wheel too transparent, "reproduces on psx_render too, so the oracle is NOT the reference") and exactly what docs/gfx-debug.md "THE THREE GROUND TRUTHS" warns about — ground truth #1 is cheap and usually right, but a clean diff is not a clean bill of health. Ground truth here is the USER'S REFERENCE IMAGE, not the oracle.
ACTION FOR OTHER CARDS: do not treat "item menu 0/76800 vs psx_render" as a correctness gate any more. It is still a valid NO-REGRESSION gate (it proves a change did not alter the menu), which is what it was mostly used for — but it must not be quoted as "the menu is correct".

PRIME SUSPECT — THE SUBTRACTIVE DIM (from #21's RE, high confidence): the menu emits a full-screen SUBTRACTIVE dim, read back off the psx leg as a GP0 0xE1 draw-mode word `E1000040` (blend bits = 2, i.e. B - F) followed by `62404040 00000000 00F00140` — colour 0x404040 over the whole 320x240 screen — linked at OT bucket 132 (`kDimBucket` in game/ui/pause_menu.cpp). It paints OVER everything in HIGHER buckets and UNDER everything in lower ones. #21 recorded that without it, every element above the panel came out exactly 0x40 too bright — so the dim is real and required. The failure mode now is the opposite and the obvious candidate: MORE is being dimmed than the guest intends, i.e. the bucket boundary at which drawCollected inserts the dim is off by one group, so chrome that should sit UNDER the dim (undimmed) is being painted before it and darkened.
CHECK: in PauseMenu::drawCollected, the dim is inserted when the first group with `a.otBucket < kDimBucket` is reached. Log the per-group buckets (`PSXPORT_DEBUG=pausemenu` already prints bucket/templ/xy per group) and compare against which groups the GUEST actually links above vs below bucket 132 on the psx leg (`debug otattr` gives per-packet bucket + emitter). If a frame/chrome group sits at exactly kDimBucket, the `<` vs `<=` choice decides whether it gets dimmed — that is a one-character bug with exactly this symptom.
SECOND CANDIDATE: the blend mode. B - F is subtractive; if a producer submits the dim (or the chrome) with the wrong blend of the four PSX modes (0.5B+0.5F, B+F, B-F, B+0.25F) the result is uniformly too dark. game/ui/panel.cpp's decodeAttr / the tpage blend bits are where that is decided.

VERIFY AGAINST THE USER'S IMAGE, not psx_render: docs/reference/issues/issue59_item_menu_too_dark.png. Sample the gold frame's RGB in both halves and compare; the fix should move our value toward the reference's, and the 0/76800-vs-psx number will necessarily STOP being zero — that is expected and correct here, not a regression.
