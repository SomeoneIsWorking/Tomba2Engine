---
id: 1
title: Queue-A highlight had guest execution but no pc_render picture
status: resolved
symptom: Tomba! 2 pc_render omits queue-A object highlight graphics even though the guest leaf runs and writes packets
tags: render,tomba2,producer,missing-graphics
state_items: S004
created: 2026-08-22
updated: 2026-08-22
---

Root cause: FUN_8002AE0C was owned only as a byte-faithful guest leaf; pc_render does not consume its OT packets and had no state-native picture. Fixed by Render::objectHighlightRender, dispatched from fieldObjectsRender using the guest queue snapshot and live route. Evidence: generated/shard_2.c; docs/findings/render.md; docs/producers/0x8002AE0C.md; a same-binary `native highlight` A/B at replay frame 255 changes 318 pixels, 44 inside the producer's reported boxes. The 274 changes on unrelated animated geometry are explicitly unresolved and not claimed as localization. Dead end: 0x80134064 ranked much higher by guest primitive count but compiled A/B changed zero pixels in both the short replay and machinery frame 31100 because its spans were off-screen.
