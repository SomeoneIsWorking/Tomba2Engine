// UiFt4Tap — THE single owner of the game-wide templated POLY_FT4 group leaf FUN_8007E1B8.
//
// The leaf is shared by every UI family in the game (pause menu chrome, dialog chrome, field HUD,
// the score/AP popup, …). Each family that has a native display producer needs to see the leaf's
// arguments, but a guest address may have EXACTLY ONE override installed on it: a second
// overrides::install silently replaces the first — it does not fail the build and it does not fail
// SBS (both write the same guest state), it only shows up as a missing layer in the picture. That is
// the dual-ownership bug that broke the dialog box (kanban #28).
//
// So the install lives here, once, and FANS OUT to the per-family capture scopes. Each scope's
// collect() early-returns unless its own scope flag is up, so exactly one family records any given
// call. Adding a family = adding one more collect() call below, never a second install.
#pragma once

class UiFt4Tap {
public:
  // install(): wires the wrapper onto the leaf's guest address on the main-EXE override table. The
  // leaf's own host-side reproduction is Render::emitUiFt4, which stays its native owner — this
  // class only carries the scope fan-out. Idempotent; called from games_tomba2_init alongside the
  // other *_install() wirings.
  static void install();
};
