---
id: 3
title: Wide Tomba 2 product keeps the title picture pillarboxed
status: open
symptom: A controlled aspect=1 product run expands gameplay to the full 16:9 width, but the title/menu picture remains centered in a narrow 4:3 region
tags: widescreen,title,ui,live-product
state_items: S005
created: 2026-08-27
updated: 2026-08-27
---

## Live evidence

The exact psxport `99a42aa3` headless product loaded `aspect=1` and `fps60=1` from
`scratch/live-wide-settings.ini`, ran the native frame loop for exactly 620 frames, and exited by
itself. Frame 400 is the title/menu picture and frame 600 is live gameplay.

- `scratch/screenshots/present_400.ppm` (SHA-256
  `0e3606d2056f872b524a59e17f0f7ef7f21f5f4b55a56950daea4deda516d490`) is visibly centered with
  black side margins; its non-black trim is 718 pixels wide.
- `scratch/screenshots/present_600.ppm` (SHA-256
  `951094548af78056e02b6bc31d5f43f475e9c97511211d9312e8450bdc58ccd7`) fills the complete 960-pixel
  output width with additional gameplay world content.
- `scratch/logs/tomba2-wide-live-99a42aa3.log` (SHA-256
  `51327431c3e3d87135a415cb1ad0dc0d4d1ff92c3b575d513b1e541ffbeb4eed`) records Native rendering,
  fps60 accepted from the settings value, the exact 620-frame cap, 0 dropped presentation layers,
  and the producer ledger.

This proves the product and gameplay-wide path work in the inspected run. It does not prove complete
true widescreen because the title composition still uses a narrow authored picture.

## Required fix

Trace the title/menu backdrop and 2D layout owners from their native producers, then give that scene
an explicit wide composition. Preserve central scale and intentional artwork framing while filling
the side regions semantically; do not stretch or crop the 4:3 image and do not special-case the final
present texture.
