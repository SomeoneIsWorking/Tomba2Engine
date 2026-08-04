---
id: 73
title: Score-pickup point popups: anchoring to Tomba is wrong in widescreen
status: todo
labels: [render, widescreen]
created: 2026-08-04
updated: 2026-08-04
---

**2026-08-04:** USER-REPORTED 2026-08-04: 'score pickups showing points on screen anchored to Tomba (this was working but wrongly anchored in widescreen)'. So the popup RENDERS but its anchor is wrong specifically under widescreen — i.e. a screen-space position derived from a 4:3 assumption while the display is wider. Prime suspects, all in the absence window and all about exactly this: psxport a0b88136 'the widescreen width must scale from the game's own 4:3 width, not 320', 94e52472 'the 2D widescreen widen must use the game's own 4:3 width, not 320', 2c54ce71 'widen the draw-area clip by the game's own 4:3 width, not by 320', 6dda8528 'the widescreen backdrop fill was queueing VRAM coordinates as display-local'. VERIFY by measurement, do not assume which.
