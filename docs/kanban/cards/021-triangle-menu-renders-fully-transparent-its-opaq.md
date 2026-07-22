---
id: 21
title: Triangle menu renders fully transparent — its opaque background is missing
status: todo
labels: [render]
created: 2026-07-22
updated: 2026-07-22
---

USER 2026-07-22: the menu opened with TRIANGLE is supposed to be FULLY OPAQUE but renders totally transparent - the world shows straight through it. Almost certainly the same root as #19 (dialog text-box background missing, only the panel corners render): since the break-first render rebuild pc_render does not walk the guest OT at all, so any panel/background slice still emitted ONLY into the OT is never drawn, while the slices that do have a native producer survive. A menu whose text and frame appear but whose fill does not is that fault in its most complete form. Repro is trivial and needs no replay: reach free-roam, tap triangle, shot; compare pc_render against psx_render IN ONE PROCESS via the renderpsx REPL toggle on the DEFAULT leg (GATE-vs-ORACLE is blind to tap-based producers). RELEVANT PRIOR WORK - read before touching: Panel::fillQuad (guest 0x8004FFB4, game/ui/panel_fill.cpp) is the 9-slice FILL primitive and is proven BYTE-IDENTICAL to the guest body by A/B dump-diff, so the emitter is correct and must not be 'fixed'; the question is whether its output reaches the picture. Also Panel::pushDialogBackdrop (0x8007FCC8) and the menu emitters that game/render/field_hud.cpp now delegates to. If one display-pass producer for the panel fill fixes BOTH this and #19, close them together with one capture each. Fix pattern is the tap - run the gen body, re-derive host-side from the published contract (game/render/fx_sprite.cpp). No span registry or tag to rescue it: banned by the NATIVE PRESENTATION directive.
