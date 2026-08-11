---
id: 83
title: Hotkey to cycle the render path live: PC-native / PC-from-GTE / pure PSX rasterizer
status: done
labels: [render]
created: 2026-08-12
updated: 2026-08-12
---

USER ASK, first message of the 2026-08-11 session, verbatim: "one more thing, need a toggle to switch between PC render native, PC render from GTE and pure PSX restraizer".

HALF-DELIVERED and it read as done, which is why this card exists. The tri-state RenderPath { Native, Gte, Psx } was built and wired to PSXPORT_RENDER_PATH=native|gte|psx (external/psxport/runtime/recomp/render_path.cpp, config.cpp cv_render_path), so a path can be chosen AT LAUNCH. The user asked for a TOGGLE — switch between the three while the game runs, which is the entire point of comparing renderers. That part does not exist.

Known anchors: runtime switching already works from the REPL (external/psxport/runtime/recomp/repl.cpp:305 and :318 call c->rsub.mode.setPath(p)), so the state is already runtime-mutable and the missing piece is a WINDOW KEY BINDING, not new plumbing. Window key events are handled in runtime/recomp/repl.cpp and runtime/recomp/rmlui_overlay.cpp.

Process note worth keeping: this evaporated because it was never tracked. It lived only in the user's message, so when the session context compacted, the summary carried the work forward but not the outstanding request.

**2026-08-12:** IMPLEMENTED (psxport 9e64bdb8): F5 cycles native -> gte -> psx -> native, edge-detected in pad_input.cpp beside the P / '.' debug keys. Cycle order factored into render_path_next() (render_mode.h) and shared with the REPL's bare `renderpath`, which previously had its own inline arithmetic. Refuses out loud under ORACLE/SBS. Verified live mid-run cycling headless via the REPL (5 frames between switches, exit 0) + tests/test_render_path_cycle.cpp; the F5 KEY READ itself is unverified because it needs a window, so the user should confirm the keypress once.
