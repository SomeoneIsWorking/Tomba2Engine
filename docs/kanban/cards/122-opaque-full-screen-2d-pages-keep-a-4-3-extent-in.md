---
id: 122
title: Opaque full-screen 2D pages keep a 4:3 extent in widescreen — the live field shows in both margins
status: done
labels: [render, widescreen, ui]
created: 2026-09-19
updated: 2026-09-19
---

MEASURED 2026-09-19 on the shipping Lightrec product at 16:9, psxport 18e8d184.

- replays/bugs/ingame-item-menu.pad f1120 (pause/item menu): the menu's authored 320x240 black backdrop covers only the 4:3 middle; the seaside field reappears in both side margins, cut off by a hard vertical edge at each end of the wooden frame.
- replays/bugs/ingame-options-page.pad f1160 and replays/bugs/title-options-page.pad f1027 (Select Options): identical, behind the dark-blue gradient.

ROOT CAUSE. Both pages are opaque and full-screen but authored 320 wide. Two call sites already pushed a private 'pillarbox' quad — the same 22-argument block, copied — and each carried the same comment claiming a flat untextured quad STRETCHES across the wide framebuffer. That is only half of rq_2d_xform's rule: the stretch applies on RQ_BACKGROUND only. Render::optionsBackdrop pushes on RQ_OVERLAY, and must (the in-game page is raised over a live field frame and the 2D-BG band sits behind the 3D world), so its copy was centred inside the page and painted no margin at all. The producer census carried a pc/options-pillarbox row for a quad that could never reach one. The pause menu had no such quad at all.

FIX. game/render/wide_page_fill.{h,cpp} — one owner, WidePageFill::pushBehindPage: a single flat black quad spanning the whole canvas, declared RQ_2D_WIDE_FINAL so the queue's 4:3 centring leaves it alone, on the page's own layer and band, emitting nothing at 4:3. The three call sites (pause_menu.cpp, render_options.cpp, card_browser.cpp) lost their private copies. One PC-only producer id, pc/wide-page-fill, replaces pc/options-pillarbox.

EVIDENCE. Item menu and both options pages are solid black to the canvas edges at 16:9; their 4:3 captures are byte-identical to the pre-fix run (sha256 2f1756d2... and 1c6a48e3...). The in-game START page, which composites over the live field on purpose, still shows the full-width field at f1090 — so the fill applies only where a caller declares it. Oracle: 405/405 checkpoints MATCH with aspect=1 and fps60=1, zero divergences.
