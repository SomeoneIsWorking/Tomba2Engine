---
id: I035
kind: instrument
status: trusted
created: 2026-08-06
---

## Instrument

external/psxport/tools/port/present_geometry.py — the only SHAPE instrument in this workspace: measures the presented picture's ASPECT. Every other capture check here (coverage %, colour count, brightness, tile richness) is INVARIANT under an aspect bug. Refuses (rc 3) when black margins make band-vs-picture ambiguous; give it --active/--display or --guest-frame (the frame's own drawn extent) for a verdict. ONE FILE, IN THE FRAMEWORK: it used to be copied into `spyro/tools/`, `Tomba2Engine/tools/` and `spider1/tools/`, and by 2026-09-19 all three hashes differed. The copies are deleted; this port runs the framework's file through its own `psxport.pin`, so a fix there reaches every port.

## Validated by

uv run --frozen python external/psxport/tools/port/present_geometry.py --selftest = 16/16. Runs BOTH directions on synthetic frames: fills-sink 4:3 -> OK rc0; fills-sink 1.600x -> STRETCHED rc1; all-black -> REFUSED rc2; a frame whose guest draws only 224/240 lines with NO guest info -> AMBIGUOUS guest call (the OLD spider1 copy printed a confident STRETCHED 1.714x on that exact frame); the SAME frame with --active 512x224 --display 512x240 -> STRETCHED 1.600x rc1; and the NEGATIVE CONTROL, the FIXED present with the SAME flags -> OK rc0. NOT MEASURED, and do not assume it: whether Tomba!2's guest actually draws all 240 of its display lines. Nobody has checked. Until someone dumps a guest framebuffer and looks, use --guest-frame so the drawn extent comes from the frame instead of from that assumption. Mutation-tested: 3 injected defects each drop the selftest to 14-15/16.

## Known failure modes

- **It CANNOT separate "black because the game drew black" from "black because it is a letterbox
  bar" from pixels alone.** That is why it refuses (rc 3) instead of guessing. Tomba!2's guest draws
  all 240 lines, so `--active 320x240 --display 320x240` is the no-op correction and the band IS the
  picture — but that is a CLAIM about the frame you must actually check, not a default.
- **The `--active` correction assumes the present is a UNIFORM SCALE of the guest display rect.** If
  the presenter crops, pans, or scales the axes differently inside the picture rect, the correction is
  silently wrong and the tool cannot detect it.
- **It is a geometry check on the FRAME, not on the CONTENT.** It cannot tell "correctly 4:3" from
  "the game happens to be drawing a square thing".
