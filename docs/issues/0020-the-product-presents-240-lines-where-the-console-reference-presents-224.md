---
id: 20
title: The product presents 240 lines where the console reference presents 224
status: open
symptom: at every picture checkpoint the product's framebuffer is 320x240 and the reference's active display area is 320x224. The comparison survives only because it crops the product to its own reported guest scan count; nobody has established which height is right for this title
state_items: S004
tags: render,display,oracle,picture
created: 2026-09-19
---

## Measured

Every run of `tools/picture_oracle.py`:

```
product   320x240
reference 320x224
```

The picture oracle handles this honestly — it crops the product using the product's OWN `guest_scan`
count from its GPU state, never by fitting one image to the other, and refuses outright if the two
sides still disagree after that. So the comparisons in issues 0018 and 0019 are over a real 320x224
region on both sides and are not affected by this.

## Why it is still a defect to resolve

The crop makes the comparison possible; it does not make the difference go away. A PSX title selects
its vertical display range, and 224 versus 240 visible lines is a real, user-visible difference in how
much of the world is on screen. One of these is what the title does on hardware and the other is not.

This also matters to issue 0019: the offset search there found a dy=+1 component, and a vertical
display-origin difference is a plausible shared cause.

## What is NOT established

Which one is correct. The reference reports its own active area (`crop_overscan=smart`) and the
product reports its own scan count; both are self-reported and neither has been checked against the
title's actual GPU display-range register writes. Do that before changing anything.

## Next

1. Read the guest's display-range register writes (GP1 0x06/0x07) on both cores at a settled frame
   and see what the title actually asks for.
2. Whichever side does not match the guest's request is the one to fix.
