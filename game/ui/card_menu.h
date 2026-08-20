// class CardMenu — native display producer for the IN-GAME MEMORY-CARD MENU (kanban #102): the
// save/load browser the save sign raises ("Checking MEMORY CARD…", "Select slot", "Select file to
// save", "OK to save?", the load and format pages).
//
// THE SYMPTOM. USER, 2026-08-19, live at the seaside save sign: "the save window background and
// button prompts are missing". Measured against the reference build
// (docs/reference/issues/save-browser-oracle.png vs save-browser-pcrender.png): pc_render drew the
// TEXT and the slot rows and nothing else — no memory-card backdrop (the live field showed through
// where the reference has a full-screen dark-maroon motif), no "Save" title badge, no X/[]/O/^
// button-prompt icons on the legend row, no per-slot save icon.
//
// THE GUEST SIDE. The card menu lives in the runtime LOAD-MENU overlay at 0x8018A000. Its per-frame
// entry is FUN_8018FBCC, a 17-state machine that calls the UI driver FUN_8018F660 and then the
// current page's drawer; the save-slot page is FUN_8018D418. Every missing piece above is emitted
// through the game-wide 2D SPRITE group leaf FUN_8007E6DC:
//   * the BACKDROP is a 5x7 grid of menu template 238/239 (the phase bit at ctx+60 picks which) at
//     (24 + 48*col, 24 + 48*row) — FUN_8018D418's tail loop.
//   * the "Save" TITLE BADGE is template 52/53 at (48, 24), by the same phase bit.
//   * the BUTTON PROMPTS come from FUN_80033AFC(buttonMask, x, y, sizeClass), which maps the PSX pad
//     bit to a template — 0x2000 circle -> 116, 0x4000 cross -> 117, 0x8000 square -> 119, anything
//     lower (0x1000 triangle) -> 118 — and hands it to the same leaf. FUN_8018D418 calls it four
//     times: cross at x=20, square at x=96, circle at x=172, triangle at x=248, all y=60.
//
// WHY THE FIX IS A SCOPE AND NOTHING MORE. FUN_8007E6DC is already owned and already tapped
// (ui_sprite.cpp's ov_compose) — tapping it again would be the dual-ownership bug that broke the
// dialog box (kanban #28). That tap hands every group to UiGroupCapture::route, which files it under
// whichever PAGE SCOPE is raised and DROPS it when none is. No scope covered the card menu, so every
// group it emitted was discarded; the text survived only because the Font taps produce it
// independently. So this page needs exactly what StartPage needed: raise a capture around the
// overlay's own per-frame entry, run the untouched gen body, lower it, draw what was filed.
//
// The scope sits on FUN_8018FBCC rather than on the individual page drawers so that every card
// screen — not just the save-slot page the bug was reported on — is covered by one wrapper.
#pragma once
#include "ui_group_capture.h"
class Core;

class CardMenu {
public:
  // The menu's chrome groups, captured off the shared 2D sprite emitter for as long as the card
  // overlay's frame entry FUN_8018FBCC is on the stack. Per-Core, so SBS's two cores cannot see
  // each other's scope.
  UiGroupCapture capture;

  // install(): registers the FUN_8018FBCC scope wrapper on the CRD-overlay override table.
  // Idempotent; called from games_tomba2_init alongside the other *_install() wirings.
  static void install();

  // drawCollected: draw the whole page in the guest's paint order (descending OT bucket, LIFO
  // within a bucket) at RQ_OVERLAY — ONE layer for the whole screen, one band below the RQ_HUD
  // glyphs the Font taps produce, so chrome can never paint over its own text (kanban #28 / #64).
  // The stacking WITHIN the page is the guest's ordering table and nothing else.
  void drawCollected(Core *c);
};
