---
id: 83
title: Hotkey to cycle the render path live: PC-native / PC-from-GTE / pure PSX rasterizer
status: done
labels: [render]
created: 2026-08-12
updated: 2026-08-20
---

USER ASK, first message of the 2026-08-11 session, verbatim: "one more thing, need a toggle to switch between PC render native, PC render from GTE and pure PSX restraizer".

HALF-DELIVERED and it read as done, which is why this card exists. The tri-state RenderPath { Native, Gte, Psx } was built and wired to PSXPORT_RENDER_PATH=native|gte|psx (external/psxport/runtime/psx/render_path.cpp, config.cpp cv_render_path), so a path can be chosen AT LAUNCH. The user asked for a TOGGLE — switch between the three while the game runs, which is the entire point of comparing renderers. That part does not exist.

Known anchors: runtime switching already works from the REPL (external/psxport/runtime/psx/repl.cpp:305 and :318 call c->rsub.mode.setPath(p)), so the state is already runtime-mutable and the missing piece is a WINDOW KEY BINDING, not new plumbing. Window key events are handled in runtime/psx/repl.cpp and runtime/psx/rmlui_overlay.cpp.

Process note worth keeping: this evaporated because it was never tracked. It lived only in the user's message, so when the session context compacted, the summary carried the work forward but not the outstanding request.

**2026-08-12:** IMPLEMENTED (psxport 9e64bdb8): F5 cycles native -> gte -> psx -> native, edge-detected in pad_input.cpp beside the P / '.' debug keys. Cycle order factored into render_path_next() (render_mode.h) and shared with the REPL's bare `renderpath`, which previously had its own inline arithmetic. Refuses out loud under ORACLE/SBS. Verified live mid-run cycling headless via the REPL (5 frames between switches, exit 0) + tests/test_render_path_cycle.cpp; the F5 KEY READ itself is unverified because it needs a window, so the user should confirm the keypress once.

**2026-08-19:** 2026-08-19: the toggle was still UNREACHABLE IN PRACTICE and the user reported it as not wired. F5 needs the window focused and prints its confirmation to the launching terminal; the REPL blocks the frame loop so it cannot be attached to a live session at all. Added `renderpath [native|gte|psx]` to the DEBUG SERVER (external/psxport/runtime/psx/dbg_server.cpp) — the one channel that works on a running window. Same cycle order / refusals / CVar mirror as F5 and the REPL (render_mode.h); no new mechanism.

VERIFIED live over the wire, headless instance at AUTO_SKIP free-roam f3102, and against BOTH classes rather than declared: native vs gte 51519/76800 px differ, native vs psx 50021/76800, gte vs psx 9420/76800 (the two guest paths differ only by the rasterizer, as designed), all three non-black (76786-76797 of 76800). Shots: scratch/screenshots/mach/rp_{native,gte,psx}.png.

Also closed a TRAP found while doing it: `cvar PSXPORT_RENDER_PATH <v>` reports ok and changes NOTHING, because render_path_install() consumes that knob once at boot. The dbg `cvar` handler now says so and points at `renderpath`.

STILL UNVERIFIED: the F5 key read itself (needs a window; unchanged since this card was closed).

**2026-08-20:** USER reports that selecting GTE/PSX from the live RmlUi menu inside the first hut freezes everything and authorizes removing it. ROOT CAUSE OF THE PLAYER-FACING DEFECT: card #83 promoted the software-rasterized Psx oracle/diagnostic path into the shipping live cycle on evidence limited to area-0 headless screenshots; C028's window/other-area falsifier has now fired. psxport removes Psx from the RmlUi player cycle (Native/PC <-> GTE/PC) while preserving explicit PSXPORT_RENDER_PATH=psx and REPL/debug-server diagnostics for oracle work.
