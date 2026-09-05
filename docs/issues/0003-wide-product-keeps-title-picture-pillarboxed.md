---
id: 3
title: Wide Tomba 2 product keeps the title picture pillarboxed
status: fix-verified
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

## Root cause

`Render::menuChrome` emits the title as two fixed-width textured quads covering the retail picture's
authored 320-pixel width. The shared 2D transform correctly centers such textured 4:3 content in the
wide canvas; only an untextured flat background is eligible for uniform full-canvas expansion. The
result was therefore not a broken wide transform: the title had no owner for the additional side
canvas, while gameplay already used native wide projection and culling producers.

## Resolution

`game/render/title_wide_composition.cpp` now owns that title-specific composition. In wide mode it
fills only the additional side canvas with dim mirrored continuations of the retail picture's outer
texture strips. The central picture retains its original scale, crop, texture coordinates, and draw
order. The enhancement is a separate `pc/title-wide-margins` producer and performs no guest writes;
there is no shared-renderer or final-present special case.

The seam-corrected combined Clang product ran the real executable and disc for 620 frames and exited
normally. `scratch/logs/tomba2-live-titlewide-final-3a8256e9.log` (SHA-256
`7613ca8404b97add18f03535a01552f7ef3215167959aa668db73ec559380da0`) records Native rendering,
interpolated 60fps, 620/620 reconciled frame fences, zero dropped layers, no guest-VSync violation or
timeout, and 900 margin primitives across the same 450 title frames as `menuChrome`.

- Frame 400 now has non-black content across the full 960-pixel sink width. A pixel comparison against
  the pre-fix capture reports zero changed pixels in the original 718x520 title region and 121,513
  changed pixels outside it. The final PPM SHA-256 is
  `1e120f230106013a67ef804923f0d47ea058dddb2f51eb9b1ace9f0722955915`.
- Frame 600 remains byte-for-byte identical at the pixel level to the pre-fix live-gameplay control.
  Its PPM SHA-256 remains `caf7e911c68ed70c98e1b4c310f03be1655aa72274961fd44198a82e7d512937`.
- The normal C++ policy gate and both native-frame-contract gates passed on the combined tree.

The implementation is verified locally but remains unlanded until the operator integrates and
commits the shared dirty batch.
