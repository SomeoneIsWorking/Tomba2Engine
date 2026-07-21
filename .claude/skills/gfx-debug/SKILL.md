---
name: gfx-debug
description: Rendering-bug workflow for the Tomba!2 port — inspect the LIVE game's render state (debug channels, scene dump, provat, VRAM readback) instead of diffing GP0 streams. Use for any "renders wrong" problem: corruption, wrong color/order/geometry, missing layer, fade/water/semi-transparency artifacts.
---

# Graphics debugging

**Read `docs/gfx-debug.md` first** — it is the full canonical workflow (methodology, every diagnostic
channel, the per-symptom playbooks, the tool inventory). This skill exists so that doc gets read
before a render bug is chased; it does not restate it.

The three rules that decide whether you're debugging correctly:

1. **There is no render oracle.** The engine owns world, projection, and ordering PC-native, so there
   is nothing PSX to diff against. Inspect the RUNNING port's state; never replay a GP0 stream
   through a second renderer. Reasoning about GP0/OT to explain a picture is a red flag — rebuild the
   explanation from GAME STATE.
2. **Ordering bugs are fixed by giving the engine the right ordering RULE** (its own depth buffer for
   3D, its own layer/sort for 2D) — never by re-syncing with the PSX OT.
3. **The USER is the visual authority.** You build, capture, and look — but a single still hides
   bugs. Do capture and read the screenshot (never claim "I can't verify headless"); do not declare a
   render correct off one frame.

If the user's game is already open, attach to it rather than launching your own — see the
`live-session` skill (`provat`, `vkvram`, `scene`, `shot` all work against the live window).

When the workflow or a tool falls short, extend `docs/gfx-debug.md` and `tools/` in the same session
— that doc is the living record.
