// class SavePrompt — native chrome producer for the IN-FIELD SAVE PROMPT, the little "Save? / Yes /
// No" dialog the save sign raises over the live field (guest task FUN_800739AC).
//
// THE SYMPTOM. USER, 2026-09-19: "I can see save menu for example not being like the oracle."
// Reproduced from their own replays/bugs/save-card-pages.pad and localised against the console
// reference: the product draws the three words and NOT the Cross and Circle button glyphs beside
// "Yes" and "No". Measured in the glyph column, 165.7 mean absolute difference with 41.2% of pixels
// past threshold, against 24.1 / 5.6% for the world outside the dialog box — a ~7x concentration.
// The RAM oracle agrees on 0 divergences over the same 1,699 frames, correctly: a chrome group that
// is never drawn writes no different guest state. See docs/issues/0013.
//
// THE GUEST SIDE. FUN_800739AC is the prompt's per-frame task: a 6-state machine dispatched from the
// table at 0x80016B50 with a0 = the task struct. Its drawing state calls FUN_800738B0, which emits
//   * the CROSS prompt  — FUN_80033AFC(0x4000, 188, 88, 0)
//   * the CIRCLE prompt — FUN_80033AFC(0x2000, 188, 108, 0)
//   * "Save?" / "Yes" / "No" — three FUN_80079374 (drawText) calls
//   * the dialog PANEL — FUN_8005019C
// FUN_80033AFC maps the pad bit to a template (0x2000 circle -> 116, 0x4000 cross -> 117, 0x8000
// square -> 119, lower -> 118) and hands it to the game-wide 2D sprite group leaf FUN_8007E6DC —
// the same leaf, and the same mapping, the card menu's prompts use (see card_menu.h).
//
// WHY THE FIX IS A SCOPE AND NOTHING MORE. FUN_8007E6DC is already owned and already tapped
// (ui_sprite.cpp's ov_compose), and a second owner of one guest address is the dual-ownership bug
// that broke the dialog box (kanban #28). The tap hands every group to UiGroupCapture::route, which
// files it under whichever page scope is raised and DROPS it when none is. No scope covered this
// prompt — it is drawn over the FIELD scene, so route's one classified-scene fallback
// (SaveContinueMenu) does not claim it either. Measured on the save route: both prompts were routed
// and dropped 1,415 times each, exactly as often as the dialog drew its three words. The text
// survived only because the Font taps produce it independently.
//
// So this page needs what StartPage and CardMenu needed: raise a capture around the drawer, run the
// untouched guest-visible behavior, lower it, draw what was filed.
//
// THE SCOPE IS ON FUN_800738B0, NOT ON THE TASK FUN_800739AC. The task was the first candidate, for
// the same "cover every state with one wrapper" reason the card menu's scope sits on its frame
// entry, and a scope there fired ZERO times and left all 2,830 prompt groups dropped. The reason is
// ownership, not reachability: **FUN_800739AC is already native** — `beh_scene_ui_trigger`
// (game/ai/beh_scene_ui_trigger.cpp), the per-object behaviour handler the field placement driver
// installs at node+0x1c. There is no guest body left at that address for a scope wrapper to run
// around; the native handler IS the task, and it reaches the drawer by typed dispatch. (Nothing in
// MAIN.EXE reaches 0x800739AC by `jal` or through a static table either — 178,688 words scanned for
// both — which is consistent: it is called indirectly through the node's behaviour pointer.)
//
// FUN_800738B0 is a real jal target, reached 1,415 times on the save route, still guest, and the
// only function that emits this prompt's chrome. So it is both wrappable and sufficient.
#pragma once
#include "ui_group_capture.h"
class Core;

class SavePrompt {
public:
  // The prompt's chrome groups, captured off the shared 2D emitters for as long as the guest drawer
  // FUN_800738B0 is on the stack. Per-Core, so SBS's two cores cannot see each other's scope.
  UiGroupCapture capture;

  // install(): registers the FUN_800738B0 scope wrapper. Idempotent; called from games_tomba2_init
  // alongside the other *_install() wirings.
  static void install();

  // drawCollected: the whole prompt in the guest's paint order at RQ_OVERLAY, one band below the
  // RQ_HUD glyphs the Font taps produce, so the panel can never paint over its own text.
  void drawCollected(Core *c);
};
