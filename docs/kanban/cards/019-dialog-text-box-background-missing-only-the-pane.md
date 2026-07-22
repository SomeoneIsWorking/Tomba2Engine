---
id: 19
title: Dialog text-box background missing — only the panel corners render
status: todo
labels: [render]
created: 2026-07-22
updated: 2026-07-22
evidence: docs/reference/issues/issue19_panel_fill_missing.png
---

USER 2026-07-22 with a capture of the 'Bucket acquired!' message. The text panel's BACKGROUND is absent: only the four corner pieces draw, the fill and the edge runs between them are gone, so the glyphs sit directly on the world. STRONG HYPOTHESIS, and it fits every other missing layer found this week: the panel is a 9-SLICE (corners + edge runs + centre fill) and only the slices with a NATIVE producer survive. Since the break-first render rebuild pc_render does not walk the guest OT at all, so any slice still emitted only into the OT is simply never drawn - exactly how the torch flame (#12) went missing. The corners rendering proves a producer exists for part of it, which makes this a partially-ported panel rather than a broken one. RELEVANT PRIOR WORK, read before touching anything: Panel::fillQuad (guest 0x8004FFB4, game/ui/panel_fill.cpp) is the 9-slice FILL primitive, 505 dispatches per 1200 frames, and was proven byte-identical to the guest body by A/B dump-diff - so the emitter itself is correct and must NOT be 'fixed'. The question is whether its output reaches the picture, i.e. whether the fill has a display-pass producer at all. Also see Panel::pushDialogBackdrop (0x8007FCC8, game/ui/dialog_backdrop.cpp, 1556 hits). Fix pattern is the tap used for the torch flame and HUD strip: run the gen body, re-derive the quads host-side from the contract it publishes (game/render/fx_sprite.cpp, game/render/field_hud.cpp). Do NOT add a span registry or tag to rescue it - banned by the NATIVE PRESENTATION directive. NOTE: this is the same bucket cutscene that softlocks (#2), being worked separately - the two may be independent, do not assume either way.

**2026-07-22:** 2026-07-22 (sweep agent) — see the evidence on #21, it applies here too: Panel::fillQuad (0x8004FFB4) is NOT the emitter for the menu panel (0 dispatches with the menu open). Whether it is the emitter for the DIALOG box is still open and untested — the bucket cutscene was not reached this session. If #21 and #19 really are one family, the shared producer is whatever draws the menu background, and panel_fill.cpp is a red herring for both. Do not spend time adding a display pass to Panel::fillQuad without first proving it fires during the symptom: I did exactly that, it was inert, and I reverted it.
