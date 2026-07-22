---
id: 28
title: Dialog box background draws OVER its own text — dual ownership of 0x8004FFB4
status: done
labels: [bug, render]
created: 2026-07-22
updated: 2026-07-22
---

USER 2026-07-22 with a capture: the message box renders as a solid dark-green panel with a yellow border and NO text at all. ROOT CAUSE: guest FUN_8004FFB4 (the 9-slice panel FILL) had TWO owners. Panel::install() (game/ui/panel.cpp) taps it with panelFillTap -> Panel::pushFill, which submits the fill at RQ_OVERLAY, one layer BELOW the glyph text's RQ_HUD; that file carries a comment recording exactly why (bug #64, 2026-07-16: 'With both on RQ_HUD the fill drew over the glyphs ... dimmed text behind the fill / fully empty boxes'). On 2026-07-22 game/ui/panel_fill.cpp landed as a readable rebuild of the same guest body and ALSO installed itself on 0x8004FFB4, with its own display pass at RQ_HUD. The queue sorts (layer, seq), so the duplicate HUD fill composited over the glyphs it is supposed to sit behind - the documented mistake, re-made. FIX: one owner. panel_fill.cpp keeps the readable guest-state rebuild and no longer installs or draws; Panel::panelFillTap now calls Panel::fillQuad(c) in place of gen_func_8004FFB4(c) and keeps pushFill at RQ_OVERLAY. VERIFIED headless on replays/bugs/bucket-softlock.pad f1200: the box draws opaque with 'This'll work for carrying the Water!' legible on top (scratch/screenshots/dlg3/f1200.png). This also closes #19 - the dialog fill was the missing background, and it is now present. WORKFLOW NOTE: tools/codemap.py --conflicts exists precisely to catch this and was not run when panel_fill.cpp was added.
