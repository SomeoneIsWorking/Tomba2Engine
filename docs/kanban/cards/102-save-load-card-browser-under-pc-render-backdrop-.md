---
id: 102
title: Save/load card browser under pc_render: backdrop, Save title badge, button-prompt icons and the slot save-icon are all missing
status: done
labels: [render, bug, ui]
created: 2026-08-19
updated: 2026-08-19
---

USER 2026-08-19, reported live while saving at the seaside save sign: "the save window background and button prompts are missing".

REPRO (deterministic, cut from the USER's own session): replays/bugs/save-confirm-crash.pad ends with the 'Save?' prompt on screen. Then
    printf 'run 8900\ntap x 8\nrun 60\ntap x 8\nrun 120\nshot <png>\nquit\n' | \
      PSXPORT_NOAUDIO=1 PSXPORT_NOPACE=1 PSXPORT_NO_FMV=1 PSXPORT_REPL=1 \
      PSXPORT_PAD_REPLAY=replays/bugs/save-confirm-crash.pad ./scratch/bin/tomba2_port scratch/bin/tomba2/MAIN.EXE
Add PSXPORT_ORACLE=1 for the reference. Both captured: docs/reference/issues/save-browser-oracle.png (reference) vs save-browser-pcrender.png (port).

WHAT THE REFERENCE HAS AND pc_render DOES NOT (five layers, from the diff of those two shots):
  1. the full-screen memory-card BACKDROP (dark maroon, repeating controller/card motif) - pc_render leaves the live field showing through, which is what the USER called the missing window background
  2. the red/yellow 'Save' TITLE BADGE at top-left
  3. the BUTTON-PROMPT ICONS on the legend row - the reference reads 'X:OK  [] :-  O:Return  /\ :-', the port renders only ':OK  -  :Return  :' with every glyph absent
  4. the per-slot SAVE ICON (the animated card icon at the right of a used slot)
  5. the small card icon at bottom-left

WHAT IS ALREADY RIGHT: all TEXT renders (the global font tap in game/ui/font.cpp covers it) and the slot row panels render.

WHERE TO START: game/render/card_browser.cpp exists but is Render::renderCardBrowser, called from Render::renderTitle under sm[0x48]==4 - i.e. the TITLE-screen Load-Game browser only. The in-game save browser runs during the GAME stage, so that producer never runs here at all; that is why even the backdrop is absent. The screen is drawn by the runtime LOAD-MENU overlay at 0x8018A000 (ov_crd_*), entered via ov_crd_8018FBCC -> 8018F660 -> 8018BA68, so the emitters are overlay-resident and need a live dump + RE rather than a static pass over MAIN.EXE.

Icon note before assuming a font gap: game/ui/font.cpp iconGlyphTap (FUN_80078988) already mirrors button/SJIS icon glyphs into RQ_HUD, and the legend TEXT around them arrives. So establish first whether the legend icons go through 0x80078988 at all - if they do, the tap is being skipped or its clut/uv is wrong on this screen; if they do not, the overlay has its own icon emitter needing a producer.

**2026-08-19:** FIXED 2026-08-19, and the fix was NOT a layer choice. Root cause was two things, both of them fabrications on the host side rather than anything about the guest:

1. NO PAGE SCOPE COVERED THE CARD MENU, so ov_compose's UiGroupCapture::route dropped every sprite group the screen emitted (backdrop grid, 'Save' badge, the four button prompts, the slot save icon). Only the TEXT survived, because the Font taps produce it independently. Fixed by a scope on the CRD overlay's per-frame entry FUN_8018FBCC (game/ui/card_menu.cpp), the same shape as StartPage.

2. Render::emitUiSprites/emitUiFt4 CAPPED every template group at 16 pieces. The guest's own emitters (FUN_8007E6DC / FUN_8007E1B8) run 'n = cnt; do {...} while (--n)' with NO limit, so 16 was invented here and silently dropped everything past it. The backdrop tile is a 20-PIECE group, so four pieces of every tile went missing and the field showed through the holes — which reads exactly like a layering bug and got chased as one. Replaced with pieceCount(), which bounds only against a garbage count read from guest memory and SAYS SO when it fires.

AND THE THING THAT MADE IT LOOK LIKE A LAYERING BUG: the page's stacking was a per-producer guess. Panels were pushed inline by Panel's taps at their own layer while sprite groups were captured and drained at another, so the backdrop painted OVER the slot rows at RQ_OVERLAY and vanished UNDER the field at RQ_BACKGROUND. Both measured; neither is right, because neither is the guest's answer. UiGroupCapture now holds ONE ordered chrome list per page — panels and groups together, each carrying the ordering-table bucket the guest gave it — drained at ONE layer in paintOrder. The card menu chooses no layer at all. OptionsPage's private mBoxes list is the same latent bug and can now be retired onto this.

Panel deferral needed one real fix: FUN_8004FFB4/8005019C are handed a rect POINTER into the caller's guest stack frame, so a deferred panel must resolve it at file time — holding the pointer lost most of the slot panels. Hence Panel::pushFillAt/pushCornersAt.

VERIFIED against PSXPORT_ORACLE=1 on both card screens (docs/reference/issues/save-browser-oracle.png): 'Select slot' and 'Select file to save' now match — solid backdrop, badge, all four button prompts, slot save icon, clean panels. START page re-checked against the oracle for a panel-ordering regression: matches.
